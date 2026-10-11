#pragma once

#include <stdint.h>
#include <cstdint>
#include <time.h>
#include <wchar.h>
#include <stdio.h>

typedef float FLOAT;
typedef double DOUBLE;

#ifndef byte
#define byte uint8_t
#endif

// Xbox 360 Lock-Free Stack Stub
template <typename T>
class XLockFreeStack {
public:
    XLockFreeStack() {}
    XLockFreeStack(int capacity) {}
    void Push(T item) {}
    bool Pop(T* item) { return false; }
    T* Pop() { return nullptr; }
    void Flush() {}
    int GetCount() const { return 0; }
};

inline bool QueryPerformanceFrequency(void* lpFrequency) {
    if (!lpFrequency) return false;
    *(int64_t*)lpFrequency = 1000000000LL;
    return true;
}

inline bool QueryPerformanceCounter(void* lpPerformanceCount) {
    if (!lpPerformanceCount) return false;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *(int64_t*)lpPerformanceCount = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return true;
}

struct DummyApp {
    void FatalLoadError() {}
    void DebugPrintf(const wchar_t*, ...) {}
    void DebugPrintf(const char*, ...) {}
};
static DummyApp app;
