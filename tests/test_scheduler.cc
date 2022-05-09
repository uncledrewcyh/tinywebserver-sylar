/*
 * @description: 协程调度测试
 */
#include "src/log.h"
#include "src/scheduler.h"
pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    PUDGE_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        pudge::Scheduler::GetThis()->schedule(&test_fiber, pudge::GetThreadId());
    }
}


void taskFuc1() {
    PUDGE_LOG_INFO(g_logger) << "task execute";
}

void taskFuc2() {
    PUDGE_LOG_INFO(g_logger) << "task execute";
}

void taskFuc3() {
    PUDGE_LOG_INFO(g_logger) << "task execute";
}

int main(int argc, char** argv) {
    pudge::Scheduler sc(3, true, "scheduler");
    sc.start();
    sc.schedule(test_fiber);
    sc.stop();
    return 0;
}
