#ifndef COMPAT_PATCH_H
#define COMPAT_PATCH_H

#include <chrono>
#include <cstdint>
#include <cstddef>

typedef unsigned char byte;
typedef unsigned char BYTE;
typedef wchar_t WCHAR;
typedef float FLOAT;
typedef long long LARGE_INTEGER;

inline void QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
    if (lpFrequency) *lpFrequency = 1000000000LL;
}

inline void QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
    if (lpPerformanceCount) {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        *lpPerformanceCount = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    }
}

#endif
