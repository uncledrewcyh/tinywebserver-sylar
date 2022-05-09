/*
 * @description: 测试时间获取
 */
#include "src/util.h"
#include "src/log.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void test() {
    PUDGE_LOG_INFO(g_logger) << pudge::GetCurrentMS();
    sleep(1);
    PUDGE_LOG_INFO(g_logger) << pudge::GetCurrentMS();
}

int main() {
    test();
    return 0;
}