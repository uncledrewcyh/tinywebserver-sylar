/*
 * @description: 线程+协程测试
 */
#include "src/log.h"
#include "src/thread.h"
#include "src/fiber.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void test_fiber() {
    PUDGE_LOG_INFO(g_logger) << "back to mainFiber ";
    sleep(1);
    pudge::Fiber::GetThis()->back();
    sleep(1);
    PUDGE_LOG_INFO(g_logger) << "back to mainFiber ";
    sleep(1);
    pudge::Fiber::GetThis()->back();
}

void test_thread() {
    PUDGE_LOG_INFO(g_logger) << "name: " << pudge::Thread::GetName();
    pudge::Fiber::ptr mainFiber = pudge::Fiber::GetThis();
    PUDGE_LOG_INFO(g_logger) << "create Fiber: " << pudge::Thread::GetName();
    {
        pudge::Fiber::ptr subFiber(new pudge::Fiber(test_fiber, 0, true));
        PUDGE_LOG_INFO(g_logger) << "back to subFiber ";
        subFiber->call();
        PUDGE_LOG_INFO(g_logger) << "back to subFiber ";
        subFiber->call();
        PUDGE_LOG_INFO(g_logger) << "back to subFiber ";
        subFiber->call();
    }
}

int main() {
    pudge::Thread::SetName("main thr");
    PUDGE_LOG_INFO(g_logger) << "main begin";
    std::vector<pudge::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i) {
        pudge::Thread::ptr thr(new pudge::Thread(test_thread, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    for (auto& i: thrs) {
        i->join();
    }
    PUDGE_LOG_INFO(g_logger) << "main end";
}