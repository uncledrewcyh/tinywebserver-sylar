/*
 * @description: 常用宏的封装
 */
#pragma once
#include <assert.h>
#include "log.h"
#include "util.h"

/// 在IF-ELSE中会将可能性更大的代码紧跟着起面的代码，
/// 从而减少指令跳转带来的性能上的下降。
#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define PUDGE_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define PUDGE_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define PUDGE_LIKELY(x)      (x)
#   define PUDGE_UNLIKELY(x)      (x)
#endif

/// 断言宏封装
#define PUDGE_ASSERT(x) \
    if(PUDGE_UNLIKELY(!(x))) { \
        PUDGE_LOG_ERROR(PUDGE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << pudge::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
/// 断言宏封装
#define PUDGE_ASSERT2(x, w) \
    if(PUDGE_UNLIKELY(!(x))) { \
        PUDGE_LOG_ERROR(PUDGE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << pudge::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
    