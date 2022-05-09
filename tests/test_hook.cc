/*
 * @description: 
 */
#include "src/hook.h"
#include "src/log.h"
#include "src/iomanager.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void test_sleep() {
    pudge::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        PUDGE_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        PUDGE_LOG_INFO(g_logger) << "sleep 3";
    });
    PUDGE_LOG_INFO(g_logger) << "test_sleep";
}

int main() {
    test_sleep();
    return 0;
}