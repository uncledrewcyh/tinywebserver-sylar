/*
 * @description: 协程测试
 */
#include "src/fiber.h"
#include "src/log.h"
pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void run_in_fiber() {
    PUDGE_LOG_DEBUG(g_logger) << "run_in_fiber begin";
    pudge::Fiber::GetThis()->back();
    PUDGE_LOG_DEBUG(g_logger) << "run_in_fiber end";
    pudge::Fiber::GetThis()->back();
} 

int main() {
    PUDGE_LOG_DEBUG(g_logger) << "main begin";
    {
        pudge::Fiber::ptr mainFiber = pudge::Fiber::GetThis();
        pudge::Fiber::ptr fiber(new pudge::Fiber(run_in_fiber, 0, true));
        fiber->call();
        fiber->call();
        fiber->call();
    }
    PUDGE_LOG_DEBUG(g_logger) << "main after end";
    return 0;
}
