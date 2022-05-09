/**
 * @brief: 可变参数模板测试
 */
#include <iostream>

template<typename Var, typename... Args>
void foo(Var var, Args&... args) {
    std::cout << sizeof...(Args) << std::endl;
}

int main() {
    std::cout << "foo(a, b, c);" << std::endl;
    int a = 1, b = 2, c =3;
    foo(a, b, c);
    std::cout << "foo(a, b, c);" << std::endl;
    foo(a, b, c);
    return 0;
}
