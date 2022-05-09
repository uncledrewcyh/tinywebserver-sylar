#pragma once
/**
 * @brief: 字节序操作函数(大端/小端)
 * @details 通用性，在大端主机和小端主机均可以使用
 */
#define PUDGE_LITTLE_ENDIAN 1
#define PUDGE_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>
#include <endian.h>

namespace pudge {

/**
 * @brief 8字节类型的字节序转化
 * @details 如果表达式为true，它的类型成员type推导出一个类型：
 * 如果没有传递第二个模板参数，该类型为void
 * 否则，该类型为传入的第二个模板参数
 * 如果表达式为false，它的成员类型是未定义的。
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 4字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 2字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define PUDGE_BYTE_ORDER PUDGE_BIG_ENDIAN
#else
#define PUDGE_BYTE_ORDER PUDGE_LITTLE_ENDIAN
#endif

#if PUDGE_BYTE_ORDER == PUDGE_BIG_ENDIAN

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}