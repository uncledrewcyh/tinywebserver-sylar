/*
 * @description: env测试
*
 */
#
#include "src/env.h"
#include "src/log.h"
#include "src/config.h"

///设置 work 路径
static pudge::ConfigVar<std::string>::ptr g_server_work_path =
    pudge::Config::Lookup("server.work_path"
            ,std::string("/home/pudge/workspace/pudge-server")
            , "server work path");
int main(int argc, char** argv) {
    pudge::EnvMgr::GetInstance()->init(argc, argv);
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << pudge::EnvMgr::GetInstance()->getAbsolutePath("a");
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << pudge::EnvMgr::GetInstance()->getAbsoluteWorkPath("a");
    return 0;
}