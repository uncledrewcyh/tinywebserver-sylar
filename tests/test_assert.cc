/*
 * @description: 
 */
#include "src/macro.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void test_assert() {
    // PUDGE_ASSERT(0 == 1);
    PUDGE_ASSERT2(0 == 1, "abcdef xx");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}
