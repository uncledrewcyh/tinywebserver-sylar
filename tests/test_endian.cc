#include "src/endian.h"
#include <iostream>

int main() {
    uint8_t a = 0b11111110;
    int8_t b = (int8_t)a;
    std::cout << (int)b << std::endl;
    return 0;
}