/*
 * @description: Thread测试
 */
#include "src/thread.h"
#include "src/config.h"
#include "src/log.h"

#include <unistd.h>

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

int count = 0;
pudge::Mutex s_mutex;

void fun1() {
    PUDGE_LOG_INFO(g_logger) << "name: " << pudge::Thread::GetName()
                             << " this.name: " << pudge::Thread::GetThis()->getName()
                             << " id: " << pudge::GetThreadId()
                             << " this.id: " << pudge::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; ++i) {
        //sylar::RWMutex::WriteLock lock(s_mutex);
        pudge::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        PUDGE_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        PUDGE_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    PUDGE_LOG_INFO(g_logger) << "thread test begin";

    std::vector<pudge::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        pudge::Thread::ptr thr(new pudge::Thread(&fun1, "name_" + std::to_string(i * 1)));
        //sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        //thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    PUDGE_LOG_INFO(g_logger) << "thread test end";
    PUDGE_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
