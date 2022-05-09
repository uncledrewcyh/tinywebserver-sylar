/*
 * @description: config系统测试
*
 */
#include "src/config.h"
#include "src/log.h"
#include "yaml-cpp/yaml.h"

pudge::ConfigVar<std::string>::ptr g_string_value_config = pudge::Config::Lookup<std::string>("logs.name", "bcd");

pudge::ConfigVar<std::list<int>>::ptr g_list_value_config = pudge::Config::Lookup<std::list<int>>("list", {0, 0, 0, 0});

void test_config() {
    pudge::ConfigVar<float> val("test", 0.1);
    val.fromString("5");
    PUDGE_LOG_DEBUG(PUDGE_LOG_ROOT()) << val.getValue();
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/pudge/workspace/pudge-server/conf/test.yml");
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << root;
} 

void test_config_yaml() {
    YAML::Node root = YAML::LoadFile("/home/pudge/workspace/pudge-server/conf/test.yml");
    
    ///预编译器自动连接字符串, 因此可以#prefix " "(空格) #name
    #define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }
    
    XX(g_list_value_config, list, before)    

    pudge::Config::LoadFromYaml(root);

    XX(g_list_value_config, list, after) 
}

int main() {
    // test_config();
    // test_yaml();
    test_config_yaml();
    return 0;
}