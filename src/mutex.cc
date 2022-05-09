/*
 * @description: 常用锁的封装
*
 */
#include <exception>

#include "mutex.h"

namespace pudge {

Semaphore::Semaphore(uint32_t count) {
    /// 0: 用于线程通信 count: 初始值
    if (sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}


Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    if (sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if (sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error"); 
    }
}

}