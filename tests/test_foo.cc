/*
 * @description: 随便测试
 */
#include <exception>
#include <string>
#include <memory>
#include <sys/epoll.h>

#include "src/util.h"
#include "src/log.h"
#include "src/macro.h"
class foo: public std::enable_shared_from_this<foo> {
public:
    foo() {
        PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "foo construct!";
    }
    foo(const foo&) {
        PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "foo copy construct!";
    }
    foo& operator=(const foo&) {
        PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "foo copy assignment!";
        return *this;
    }
    ~foo() {
        PUDGE_LOG_INFO(PUDGE_LOG_ROOT()) << "foo deconstruct!";
    }
private:

};

void test_e_s_f_t() {
    std::shared_ptr<foo> a = std::make_shared<foo>();
    auto b = a->shared_from_this();
    PUDGE_ASSERT(b.get() == a.get());
}

void test() {
    // PUDGE_LOG_DEBUG(PUDGE_LOG_ROOT()) << "typedid:" << typeid(std::string).name();
    // PUDGE_LOG_DEBUG(PUDGE_LOG_ROOT()) << "typedid:" << pudge::TypeToName<std::string>();
    std::cout << "throw before" << std::endl;
    throw std::logic_error("error!");
    std::cout << "throw after" << std::endl;
}
int main() {
    // std::cout << "call test before" << std::endl;
    // test();
    // std::cout << "call test after" << std::endl;
    // test_e_s_f_t();
    // std::shared_ptr<std::string> _foo;
    // std::cout << (int)(_foo == nullptr) << std::endl;
    int m_epfd = epoll_create(2947);
    std::cout << m_epfd << std::endl;
    return 0;
}