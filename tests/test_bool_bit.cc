#include <iostream>
class foo {
public:
    bool a: 1;
    bool b: 1;
    bool c: 1;
    bool d: 1;
    bool e: 1;
    bool f: 1;
    bool g: 1;
    bool n: 1;
};

int main() {
    foo _f;
    _f.a = true;
    std::cout << (int)(_f.a) << std::endl;
}