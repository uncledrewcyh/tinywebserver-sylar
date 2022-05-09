/*
 * @description: 
 */
#include "src/mutex.h"
#include "src/log.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();
typedef pudge::Mutex MutexType;
MutexType mutex;
void test_mutex() {
    MutexType::Lock lock(mutex);
    MutexType::Lock lock1(mutex);

}

int main() {
    test_mutex();
    return 0;
}