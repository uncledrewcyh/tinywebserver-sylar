/*
 * @description: 定时器模块测试
 */
#include "src/iomanager.h"
#include "src/log.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

pudge::Timer::ptr s_timer;
void test_timer() {
    pudge::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        PUDGE_LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 10) {
            // s_timer->reset(2000, true);
            s_timer->cancel();
        }
    }, true);
}

int main() {
    test_timer();
    return 0;
}