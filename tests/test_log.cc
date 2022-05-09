/*
 * @description: 日志系统测试 
*
 */
#include "src/log.h"

void test() {
    PUDGE_LOG_DEBUG(PUDGE_LOG_ROOT()) << pudge::LoggerMgr::GetInstance()->toYamlString();
}
int main(int argc, char **argv) {
    test();
    return 0;
}

