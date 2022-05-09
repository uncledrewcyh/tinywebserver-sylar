/*
 * @description: 测试导入logs配置文件
 */
#include "src/env.h"
#include "src/log.h"
#include "src/config.h"

///设置 work 路径
static pudge::ConfigVar<std::string>::ptr g_server_work_path =
    pudge::Config::Lookup("server.work_path"
            ,std::string("/home/pudge/workspace/pudge-server")
            , "server work path");

void test_configForLogs() {
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "config dir is: " << pudge::EnvMgr::GetInstance()->getAbsoluteWorkPath("conf");
    pudge::Config::LoadFromConfDir(pudge::EnvMgr::GetInstance()->getAbsoluteWorkPath("conf"));
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "------分界线-----";
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "\n" <<pudge::LoggerMgr::GetInstance()->toYamlString();
}
int main() {
    test_configForLogs();
    return 0;
}