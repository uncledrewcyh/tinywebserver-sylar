#include "src/address.h"
#include "src/log.h"
pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void test_host() {
    auto add = pudge::Address::LookupAnyIPAddress("www.baidu.com:80");
    
    PUDGE_LOG_INFO(g_logger) << *add;
}

int main() {
    test_host();
    return 0;
}