/*
 * @description: 
 */
#include <dlfcn.h>
#include <functional>

#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include "fd_manager.h"
#include "config.h"
#include "macro.h"

static pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("system");

namespace pudge {

static pudge::ConfigVar<int>::ptr g_tcp_connect_timeout =
pudge::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

/**
 * @brief: 线程级别的hook
 */
static thread_local bool t_hook_enable = false;

/**
 * @brief: 宏定义生成系统调用的函数指针
 * @details:  XX(sleep) < = > sleep_f = (sleep_fun)dlsym(RTLD_NEXT, sleep)
 * dlsym用于找回被覆盖的符号, RTLD为动态连接器，NEXT是下一个期望的符号，则为被覆盖的那一个. 
 */
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

/**
 * @brief: main函数之前进行构造初始化
 */
static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                PUDGE_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};
static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}
}
/**
 * @brief: 定时器条件变量结构体 
 */
struct timer_info {
    int cancelled = 0;
};

/**
 * @brief: 自定义超时的connect 
 */
int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if (!pudge::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    pudge::FdCtx::ptr ctx = pudge::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        return 0;
    ///非阻塞connect若没成功连接，则返回 -1
    ///若ERRNO位EINPROGRESS则说明三次握手正在进行
    } else if (n != -1 || errno != EINPROGRESS) {
        return n;
    }

    pudge::IOManager* iom = pudge::IOManager::GetThis();
    pudge::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    ///若超时时间存在的话
    if (timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
            auto t = winfo.lock();
            ///判断tinfo条件是否存在 
            ///或t->cancelled超时的话
            if (!t || t->cancelled) {
                return;
            }
            ///将条件设为超时状态
            t->cancelled = ETIMEDOUT;
            ///取消掉可写事件，会执行cb，由于下面注册空回调
            ///因此会回到该线程
            iom->cancelEvent(fd, pudge::IOManager::WRITE);
        }, winfo);
    }
    ///注册可写事件 回调为当前协程
    ///说明连接成功已经可写
    ///非阻塞connect的完成被认为是使响应套接字可写
    int rt = iom->addEvent(fd, pudge::IOManager::WRITE);
    if (rt == 0) {
        ///关键点:切换出去，让出CPU执行权
        pudge::Fiber::YieldToHold();
        if (timer) {
            timer->cancel();
        }
        ///由于 iom->cancelEvent(fd, pudge::IOManager::WRITE);引起
        ///则会触发如下的超时
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) {
            timer->cancel();
        }
        PUDGE_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 ==  getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}


template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&... args) {
    if (!pudge::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    pudge::FdCtx::ptr ctx = pudge::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }


    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);
    
retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }

    if (n == -1 && errno == EAGAIN) {
        pudge::IOManager* iom = pudge::IOManager::GetThis();
        pudge::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if (to != (uint64_t)-1) {
            /// 添加超时定时器
            /// 若超过定时的时间，则执行以下代码
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event] {
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                ///cancel掉下列的event事件并触发回调回到
                ///yeild后续的代码段
                iom->cancelEvent(fd, (pudge::IOManager::Event)(event));
            }, winfo);
        }
        /// 若读阻塞说明 缓冲区空 写阻塞 缓冲区满
        /// 添加 可读可写事件，即可获取协程唤醒的标志
        int rt = iom->addEvent(fd, (pudge::IOManager::Event)(event));
        if (PUDGE_UNLIKELY(rt)) {
            PUDGE_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            pudge::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled; ///ETIMEDOUT
                return -1;
            }
            goto retry;
        }
    }
    return n;
}


extern "C" {
/**
 * @brief: 定义用户希望覆盖的系统调用的函数指针
 * @details:  XX(sleep) < = > sleep_fun = nullptr
 * 
 */
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    ///不开启hook
    if (!pudge::t_hook_enable) {
        return sleep_f(seconds);
    }

    pudge::Fiber::ptr fiber = pudge::Fiber::GetThis();
    pudge::IOManager* iom = pudge::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind(
        (void(pudge::Scheduler::*) (pudge::Fiber::ptr, int thread))
        &pudge::IOManager::schedule, iom, fiber, -1)
    );
    pudge::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    ///不开启hook
    if (!pudge::t_hook_enable) {
        return usleep_f(usec);
    }
    pudge::Fiber::ptr fiber = pudge::Fiber::GetThis();
    pudge::IOManager* iom = pudge::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind(
        (void(pudge::Scheduler::*) (pudge::Fiber::ptr, int thread))
        &pudge::IOManager::schedule, iom, fiber, -1)
    );
    pudge::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!pudge::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    pudge::Fiber::ptr fiber = pudge::Fiber::GetThis();
    pudge::IOManager* iom = pudge::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(pudge::Scheduler::*)
            (pudge::Fiber::ptr, int thread))&pudge::IOManager::schedule
            ,iom, fiber, -1));
    pudge::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if(!pudge::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }
    // PUDGE_LOG_INFO(g_logger) << pudge::BacktraceToString();
    ///创建fd_ctx
    pudge::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, pudge::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", pudge::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    ///添加到FdManager
    if(fd >= 0) {
        pudge::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

/// read 
ssize_t read(int fd, void *buf, size_t count) {
    // PUDGE_LOG_INFO(g_logger) << pudge::BacktraceToString();
    return do_io(fd, read_f, "read", pudge::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    // PUDGE_LOG_INFO(g_logger) << pudge::BacktraceToString();
    return do_io(fd, readv_f, "readv", pudge::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    // PUDGE_LOG_INFO(g_logger) << pudge::BacktraceToString();
    return do_io(sockfd, recv_f, "recv", pudge::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    // PUDGE_LOG_INFO(g_logger) << pudge::BacktraceToString();
    return do_io(sockfd, recvfrom_f, "recvfrom", pudge::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    // PUDGE_LOG_INFO(g_logger) << pudge::BacktraceToString();
    return do_io(sockfd, recvmsg_f, "recvmsg", pudge::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

///write
ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", pudge::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", pudge::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", pudge::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", pudge::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", pudge::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                pudge::FdCtx::ptr ctx = pudge::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                pudge::FdCtx::ptr ctx = pudge::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        pudge::FdCtx::ptr ctx = pudge::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!pudge::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    ///hook部分，设置发送接收的超时时间
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            pudge::FdCtx::ptr ctx = pudge::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}

