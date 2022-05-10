/*
 * @description: 
 */
#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace pudge {

static pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("system");

///当前线程对应的调度器
static thread_local Scheduler* t_scheduler = nullptr;
///执行调度任务的协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name) {
    PUDGE_ASSERT(threads > 0);

    ///是否使用当前调用该构造函数的线程作为调度线程
    if (use_caller) {
        ///将当前线程作为主协程
        pudge::Fiber::GetThis();
        ///减少线程数量
        --threads;
        ///确保当前线程对应的协程调度器还没绑定主协程
        PUDGE_ASSERT(GetThis() == nullptr);
        t_scheduler = this;
        
        ///use_caller的话，调用构造线程的主协程不参与协程调度中！
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        pudge::Thread::SetName(m_name);
        ///绑定执行调度任务的协程(用于GetMainFiber)
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = pudge::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    PUDGE_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    ///默认为True
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    ///确保线程池为空.
    PUDGE_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        ///线程池中的每一个线程都是协程调度线程(运行run)
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                        , m_name + "_" + std::to_string(i)));
        ///如何确保能拿到准确的线程ID，前提是一定要初始化完成
        ///因此在Thread构造函数中要放一个信号量，当线程初始化
        ///成功后再唤醒，muduo里用的是条件变量，应该比较类似
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
}

void Scheduler::stop() {
    m_autoStop = true;
    ///仅仅有一个主线程进行协程调度
    if (m_rootFiber 
        && m_threadCount == 0
        && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT)) {
        PUDGE_LOG_INFO(g_logger) << this << "stopped";
        m_stopping = true;
    
        if (stopping()) {
            return;
        }
    }
    ///保证stop方法是由caller线程发起的
    if (m_rootThread != -1) {
        /// use_caller == true
        /// m_rootThread存在绑定主线程id 
        /// 主线程此时 GetThis == this
        /// 主线程和副线程均可执行stop
        PUDGE_ASSERT(GetThis() == this);
    } else {
        /// use_caller == false, 
        /// m_rootThread == -1 不存在
        /// 主线程此时的 GetThis()结果为nullptr
        PUDGE_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    ///通知其他线程有调度任务
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    ///caller线程的调度，在这里执行
    if (m_rootFiber) {
        if(!stopping()) {
            m_rootFiber->call();
        }
    }
    ///析构掉副线程
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    ///主线程等待执行完毕
    for(auto& i : thrs) {
        i->join();
    }
}    

void Scheduler::switchTo(int thread) {
    PUDGE_ASSERT(Scheduler::GetThis() != nullptr);
}

std::ostream& Scheduler::dump(std::ostream& os) {
    return os;
}

void Scheduler::tickle() {
    PUDGE_LOG_INFO(g_logger) << "tickle";
}

void Scheduler::run() {
    PUDGE_LOG_DEBUG(g_logger) << m_name << " run";
    set_hook_enable(true);
    setThis();
    ///非caller线程进入，获取改subThread的调度携程指针
    if (pudge::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }
    ///闲置协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    ///工作协程
    Fiber::ptr cb_fiber;
    ///待执行任务，从任务队列中挑选
    FiberAndThread ft;
    while (true) {
        ft.reset(); ///复位
        bool tickle_me = false; ///是否唤醒标志
        bool is_active = false; ///是否
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                ///和任务要求线程ID不匹配
                if (it->thread != -1 && it->thread != pudge::GetThreadId()) {
                    ++it;
                    tickle_me = true; ///唤醒其他线程，有任务来了
                    continue;
                }

                PUDGE_ASSERT2(it->fiber || it->cb, "invalid task")
                ///Fiber 正在执行的话 直接跳过
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                ///激活该调度器的工作线程
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            /// 唤醒条件
            /// 1. 任务和线程ID不匹配
            /// 2. 任务队列还有剩余的话，tickle一下
            tickle_me |= it != m_fibers.end();
        }

        
        if(tickle_me) {
            tickle();
        }
        
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM
                     && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::READY) {
                // 处理 YieldToReady 切出的线程 
                // 将其再次放入任务队列
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM
                      && ft.fiber->getState() != Fiber::EXCEPT) {
                /// 中途结束的协程直接不要
                ft.fiber->m_state = Fiber::HOLD; 
            }
            ft.reset();
        } else if (ft.cb) {
            if (cb_fiber) {
                ///若存在直接reset即可
                cb_fiber->reset(ft.cb);
            } else {
                ///否则分配堆栈 第二类协程创建方式
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::TERM
                      || cb_fiber->getState() == Fiber::EXCEPT) {
                cb_fiber->reset(nullptr);
            } else {
                ///YeileToHold()会来到这里
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                PUDGE_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM   
                && idle_fiber->getState() != Fiber::EXCEPT) {
                    idle_fiber->m_state = Fiber::HOLD;
            }
        }

    }
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    PUDGE_LOG_INFO(g_logger) << "idle fiber!";
    while (!stopping()) {
        ///切回 run 函数，若无任务则来回切换
        pudge::Fiber::YieldToHold();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

}