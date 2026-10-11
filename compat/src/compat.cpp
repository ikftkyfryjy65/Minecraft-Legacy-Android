#include "../include/compat.h"

#include <stdlib.h>
#include <stdint.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

/* ============================================================
 * Critical Section
 * ============================================================ */

void InitializeCriticalSection(CRITICAL_SECTION* cs)
{
    if (!cs)
        return;

    pthread_mutex_init(&cs->mutex, nullptr);
    cs->initialized = true;
}

int InitializeCriticalSectionAndSpinCount(
    CRITICAL_SECTION* cs,
    DWORD /*dwSpinCount*/)
{
    InitializeCriticalSection(cs);
    return TRUE;
}

void DeleteCriticalSection(CRITICAL_SECTION* cs)
{
    if (!cs || !cs->initialized)
        return;

    pthread_mutex_destroy(&cs->mutex);
    cs->initialized = false;
}

void EnterCriticalSection(CRITICAL_SECTION* cs)
{
    if (!cs)
        return;

    if (!cs->initialized)
        InitializeCriticalSection(cs);

    pthread_mutex_lock(&cs->mutex);
}

BOOL TryEnterCriticalSection(CRITICAL_SECTION* cs)
{
    if (!cs)
        return FALSE;

    if (!cs->initialized)
        InitializeCriticalSection(cs);

    return pthread_mutex_trylock(&cs->mutex) == 0;
}

void LeaveCriticalSection(CRITICAL_SECTION* cs)
{
    if (!cs || !cs->initialized)
        return;

    pthread_mutex_unlock(&cs->mutex);
}

/* ============================================================
 * Event implementation
 * ============================================================ */

struct CompatEvent
{
    pthread_mutex_t mutex;
    pthread_cond_t  condition;

    bool manualReset;
    bool signaled;
};

static CompatEvent* GetEvent(HANDLE handle)
{
    return reinterpret_cast<CompatEvent*>(handle);
}

HANDLE CreateEvent(
    void* /*lpEventAttributes*/,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR /*lpName*/)
{
    CompatEvent* event =
        new CompatEvent();

    pthread_mutex_init(
        &event->mutex,
        nullptr);

    pthread_cond_init(
        &event->condition,
        nullptr);

    event->manualReset =
        bManualReset != FALSE;

    event->signaled =
        bInitialState != FALSE;

    return reinterpret_cast<HANDLE>(event);
}

BOOL SetEvent(HANDLE handle)
{
    CompatEvent* event = GetEvent(handle);

    if (!event)
        return FALSE;

    pthread_mutex_lock(&event->mutex);

    event->signaled = true;

    if (event->manualReset)
        pthread_cond_broadcast(&event->condition);
    else
        pthread_cond_signal(&event->condition);

    pthread_mutex_unlock(&event->mutex);

    return TRUE;
}

BOOL ResetEvent(HANDLE handle)
{
    CompatEvent* event = GetEvent(handle);

    if (!event)
        return FALSE;

    pthread_mutex_lock(&event->mutex);

    event->signaled = false;

    pthread_mutex_unlock(&event->mutex);

    return TRUE;
}

static DWORD WaitEvent(
    CompatEvent* event,
    DWORD timeoutMs)
{
    pthread_mutex_lock(&event->mutex);

    if (!event->signaled)
    {
        if (timeoutMs == INFINITE)
        {
            while (!event->signaled)
                pthread_cond_wait(
                    &event->condition,
                    &event->mutex);
        }
        else
        {
            struct timespec ts;

            clock_gettime(
                CLOCK_REALTIME,
                &ts);

            ts.tv_sec += timeoutMs / 1000;

            ts.tv_nsec +=
                (timeoutMs % 1000) * 1000000L;

            if (ts.tv_nsec >= 1000000000L)
            {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
            }

            while (!event->signaled)
            {
                int result =
                    pthread_cond_timedwait(
                        &event->condition,
                        &event->mutex,
                        &ts);

                if (result == ETIMEDOUT)
                {
                    pthread_mutex_unlock(
                        &event->mutex);

                    return WAIT_TIMEOUT;
                }
            }
        }
    }

    if (!event->manualReset)
        event->signaled = false;

    pthread_mutex_unlock(&event->mutex);

    return WAIT_OBJECT_0;
}

DWORD WaitForSingleObject(
    HANDLE handle,
    DWORD timeoutMs)
{
    if (!handle)
        return WAIT_FAILED;

    return WaitEvent(
        GetEvent(handle),
        timeoutMs);
}

DWORD WaitForMultipleObjects(
    DWORD count,
    const HANDLE* handles,
    BOOL waitAll,
    DWORD timeoutMs)
{
    if (!handles || count == 0)
        return WAIT_FAILED;

    /*
     * Minimal implementation.
     *
     * C4JThread primarily uses the individual events.
     * We handle wait-all sequentially here.
     */

    if (waitAll)
    {
        for (DWORD i = 0; i < count; ++i)
        {
            DWORD result =
                WaitForSingleObject(
                    handles[i],
                    timeoutMs);

            if (result != WAIT_OBJECT_0)
                return result;
        }

        return WAIT_OBJECT_0;
    }

    /*
     * Wait-any fallback.
     *
     * Polling is intentional for this first compatibility
     * layer. We can replace it with a shared event mechanism
     * once the actual usage is known from the compiler/runtime.
     */

    const DWORD pollMs = 1;

    DWORD elapsed = 0;

    while (timeoutMs == INFINITE ||
           elapsed < timeoutMs)
    {
        for (DWORD i = 0; i < count; ++i)
        {
            CompatEvent* event =
                GetEvent(handles[i]);

            if (!event)
                continue;

            pthread_mutex_lock(&event->mutex);

            bool signaled =
                event->signaled;

            if (signaled &&
                !event->manualReset)
            {
                event->signaled = false;
            }

            pthread_mutex_unlock(&event->mutex);

            if (signaled)
                return WAIT_OBJECT_0 + i;
        }

        usleep(pollMs * 1000);

        if (timeoutMs != INFINITE)
            elapsed += pollMs;
    }

    return WAIT_TIMEOUT;
}

BOOL CloseHandle(HANDLE handle)
{
    if (!handle)
        return FALSE;

    CompatEvent* event =
        GetEvent(handle);

    pthread_mutex_destroy(
        &event->mutex);

    pthread_cond_destroy(
        &event->condition);

    delete event;

    return TRUE;
}

/* ============================================================
 * Thread helpers
 *
 * Temporary implementation. C4JThread has its own higher-level
 * thread abstraction, so these exist mainly for source
 * compatibility.
 * ============================================================ */

struct CompatThread
{
    std::thread* thread;
    DWORD id;
    DWORD exitCode;
};

static DWORD NextThreadId()
{
    static pthread_mutex_t mutex =
        PTHREAD_MUTEX_INITIALIZER;

    static DWORD nextId = 1;

    pthread_mutex_lock(&mutex);

    DWORD id = nextId++;

    pthread_mutex_unlock(&mutex);

    return id;
}

HANDLE CreateThread(
    void* /*lpThreadAttributes*/,
    SIZE_T /*dwStackSize*/,
    DWORD (*startAddress)(LPVOID),
    LPVOID parameter,
    DWORD /*dwCreationFlags*/,
    DWORD* threadId)
{
    if (!startAddress)
        return nullptr;

    CompatThread* thread =
        new CompatThread();

    thread->id = NextThreadId();
    thread->exitCode = 0;

    thread->thread =
        new std::thread(
            [thread, startAddress, parameter]()
            {
                thread->exitCode =
                    startAddress(parameter);
            });

    if (threadId)
        *threadId = thread->id;

    return reinterpret_cast<HANDLE>(thread);
}

BOOL GetExitCodeThread(
    HANDLE handle,
    DWORD* exitCode)
{
    if (!handle || !exitCode)
        return FALSE;

    CompatThread* thread =
        reinterpret_cast<CompatThread*>(handle);

    *exitCode =
        thread->exitCode;

    return TRUE;
}

BOOL SetThreadPriority(
    HANDLE /*handle*/,
    int /*priority*/)
{
    return TRUE;
}

SIZE_T SetThreadAffinityMask(
    HANDLE /*handle*/,
    SIZE_T /*mask*/)
{
    return 1;
}

DWORD GetCurrentThreadId()
{
    static thread_local DWORD id =
        NextThreadId();

    return id;
}

void Sleep(DWORD milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            milliseconds));
}
