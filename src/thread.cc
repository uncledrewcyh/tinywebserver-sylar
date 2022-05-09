/*
 * @description: 
*
 */
#include "thread.h"
#include "log.h"
#include "util.h"

namespace pudge {
///线程存储期的t_thread指针
static thread_local Thread* t_thread = nullptr;
///线程存储期的thread名称
static thread_local std::string t_thread_name = "UNKNOW";
///创建全局 system 日志
static pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("system");

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if (name.empty()) {
        m_name = "UNKNOW";
    }
    ///this 作为 Thread::run 的 args, 创建属性为默认(nullptr)
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        PUDGE_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}


void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = pudge::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    ///减少 cb 中的智能指针的拷贝
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;
}

Thread::~Thread() {
    if (m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            PUDGE_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }    
}


Thread* Thread::GetThis() {
    return t_thread;
}
 
const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

}