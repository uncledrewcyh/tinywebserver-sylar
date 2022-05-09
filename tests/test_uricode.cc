#include "src/util.h"
#include "src/log.h"

void test() {
    auto str = pudge::StringUtil::UrlEncode("啊 啊");
    PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << str;
}

int main() {
    test();
    return 0;
}