/*
 * @description: 
 */
#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace pudge {
static Logger::ptr g_logger = PUDGE_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            PUDGE_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    ///确保该事件已经注册
    PUDGE_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(2947);
    PUDGE_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    PUDGE_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN || EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    PUDGE_ASSERT(!rt);

    ///添加pipe 读通道的 读事件
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    PUDGE_ASSERT(!rt);
    ///单线程状态
    contextResize(32);

    start();
}

IOManager::~IOManager() {
    PUDGE_LOG_INFO(g_logger) << "IOManager deconstrcut...";
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() > (size_t)fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }
    
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (PUDGE_UNLIKELY(fd_ctx->events & event)) {
        PUDGE_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        PUDGE_ASSERT2(!(fd_ctx->events & event), "addEvent ");
    }
    ///为 0 则为刚初始化的事件，则需要添加
    ///非 0 则为已绑定的事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD: EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    ///用户data：epoll事件绑定的该指针
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PUDGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    PUDGE_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);
    
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        ///cb 为空的话，绑定自身协程
        event_ctx.fiber = Fiber::GetThis();
        PUDGE_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                ,"state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= (size_t) fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (PUDGE_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    ///改变epoll注册的事件
    Event new_events = (Event)(fd_ctx->events & ~event);
    ///为0说明读写事件标志位均无，则可以直接去除掉该事件
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PUDGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    ///改变网络事件类
    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= (size_t)fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (PUDGE_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PUDGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    ///触发事件回调
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= (size_t)fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PUDGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    ///触发事件回调
    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);        
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }    
    PUDGE_ASSERT(fd_ctx->events == 0);
    return true;    
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    ///先判断是否有空闲线程
    ///有则唤醒，无则return
    if (!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    PUDGE_ASSERT(rt == 1);
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout) {
    ///根据返回的下一个定时器任务需要多久执行来判断是否有定时器任务。
    //~0ull表示无任务
    timeout = getNextTimer();
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}    

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

void IOManager::idle() {
    PUDGE_LOG_DEBUG(g_logger) << "idle";
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while (true) {
        uint64_t next_timeout = 0;
        ///下一个定时任务的时间间隔.
        if (PUDGE_UNLIKELY(stopping(next_timeout))) {
            PUDGE_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            ///最长超时时间为 3s
            static const int MAX_TIMEOUT = 3000;
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT: next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            ///陷入内核的时间由定时器任务和最大定时时间共同决定
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            if (rt < 0 && errno == EINTR) {

            } else {
                break;
            }
        } while(true);

        /// 定时器回调函数
        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if (!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                ///由于是ET模式，要把缓冲区读干净
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            // PUDGE_ASSERT(fd_ctx != nullptr);
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                PUDGE_LOG_INFO(g_logger) << "EPOLLER | EPOLLHUP";
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }            
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            } 
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }
            ///剩下还未响应的events
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD: EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            int rt1 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt1) {
                PUDGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            ///触发回调函数
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        ///防止协程无法析构
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();
        // Fiber::GetThis()->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}