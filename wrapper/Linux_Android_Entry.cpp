/* Android NativeActivity entry — linked with existing .o files */
#include <android/log.h>
#include <android/native_activity.h>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <unistd.h>

#define LOG_TAG "MCE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern void __mce_construct_app();

static void mce_android_sig(int sig, siginfo_t* info, void* ctx) {
    ucontext_t* uc = (ucontext_t*)ctx;
    unsigned long pc = uc->uc_mcontext.pc;
    Dl_info di;
    if (dladdr((void*)pc, &di) && di.dli_fbase) {
        LOGI("SEGV PC_OFF=0x%lx ADDR=0x%lx",
             pc - (unsigned long)di.dli_fbase, (unsigned long)info->si_addr);
    } else {
        LOGI("SEGV PC=0x%lx ADDR=0x%lx", pc, (unsigned long)info->si_addr);
    }
    _exit(139);
}

extern "C" void ANativeActivity_onCreate(ANativeActivity* activity,
                                          void* savedState, size_t savedStateSize) {
    LOGI("=== ANativeActivity_onCreate ===");
    struct sigaction sa = {};
    sa.sa_sigaction = mce_android_sig;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);

    LOGI("[MCE] handler installed");
    __mce_construct_app();
    LOGI("[MCE] app constructed");
    LOGI("[MCE] === SUCCESS ===");
}

extern "C" void ANativeActivity_onDestroy(ANativeActivity*) {
    LOGI("=== onDestroy ===");
}
