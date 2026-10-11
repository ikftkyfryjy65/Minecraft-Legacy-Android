/* M3.3: Android entry with crash diagnostics */
#include "stdafx.h"
#include "Minecraft.h"
#include <string>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <exception>
#include <cstddef>
#include <cstdint>
#include <link.h>
#include <setjmp.h>
#include <cxxabi.h>
#include <typeinfo>
#include <cstdlib>

extern void __mce_construct_app();
#include "Linux_App.h"
#include "EntityRenderDispatcher.h"
#include "TileEntityRenderDispatcher.h"
#include "User.h"
#include "Common/Tutorial/Tutorial.h"
#include "Common/Colours/ColourTable.h"
#include "../../Minecraft.World/Minecraft.World.h"
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
extern CConsoleMinecraftApp& g_consoleApp;

#define MCEW(s) write(2, s, sizeof(s)-1)

extern "C" void mce_segv_handler(int sig, siginfo_t* info, void* ctx) {
    ucontext_t* uc = (ucontext_t*)ctx;
    unsigned long pc = uc->uc_mcontext.pc;
    unsigned long lr = uc->uc_mcontext.regs[30];
    unsigned long sp = uc->uc_mcontext.sp;
    unsigned long fp = uc->uc_mcontext.regs[29];
    Dl_info di;
    char buf[1024];
    int n = 0;
    unsigned long pcoff = 0, lroff = 0;
    if (dladdr((void*)pc, &di) && di.dli_fbase) pcoff = pc - (unsigned long)di.dli_fbase;
    if (dladdr((void*)lr, &di) && di.dli_fbase) lroff = lr - (unsigned long)di.dli_fbase;
    n = snprintf(buf, sizeof(buf),
        "\\n[MCE-SEGV] sig=%d PC_OFF=0x%lx LR_OFF=0x%lx ADDR=0x%lx\\n",
        sig, pcoff, lroff, (unsigned long)info->si_addr);
    write(2, buf, n);

    /* Frame pointer chain */
    unsigned long f = fp;
    for (int i = 0; i < 10 && f != 0 && f < sp + 0x100000; i++) {
        unsigned long saved_fp = *(unsigned long*)f;
        unsigned long saved_lr = *(unsigned long*)(f + 8);
        unsigned long lroff2 = 0;
        if (dladdr((void*)saved_lr, &di) && di.dli_fbase)
            lroff2 = saved_lr - (unsigned long)di.dli_fbase;
        n = snprintf(buf, sizeof(buf), "[MCE-FP] #%d LR_OFF=0x%lx\\n", i, lroff2);
        write(2, buf, n);
        f = saved_fp;
    }
    _exit(139);
}

/* MCE-MEDIA-PATCH (TEMP): copy MediaPS3.arc into internal dir under the literal name the game expects */
extern CMinecraftApp& app;
static void mce_media_msg(const char* m) { write(2, m, strlen(m)); }
static bool mce_media_copy(const char* src, const char* dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) { mce_media_msg("[mce-media] cannot open source\n"); return false; }
    struct stat si, so;
    if (fstat(in, &si) != 0) { close(in); return false; }
    if (stat(dst, &so) == 0 && so.st_size == si.st_size) { close(in); mce_media_msg("[mce-media] already copied\n"); return true; }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); mce_media_msg("[mce-media] cannot open dest\n"); return false; }
    static char buf[65536];
    bool ok = true;
    ssize_t n;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, n - off);
            if (w <= 0) { ok = false; break; }
            off += w;
        }
        if (!ok) break;
    }
    if (n < 0) ok = false;
    close(in);
    close(out);
    mce_media_msg(ok ? "[mce-media] copy done\n" : "[mce-media] copy FAILED\n");
    return ok;
}

// The release library has 129 init_array entries (1032 bytes). When the APK
// patch reduces DT_INIT_ARRAYSZ to 40, Android invokes only entries 0..4 during
// dlopen. Find this DSO's full array address from PT_DYNAMIC and invoke entries
// 5..128 here, with per-entry C++ exception and synchronous-fault guards.
using MCEInitFn = void (*)();
struct MCEInitArrayInfo {
    ElfW(Addr) base;
    MCEInitFn* array;
    size_t size;
};

static int mce_find_init_array(struct dl_phdr_info* info, size_t, void* opaque) {
    MCEInitArrayInfo* result = static_cast<MCEInitArrayInfo*>(opaque);
    if (info->dlpi_addr != result->base) return 0;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& phdr = info->dlpi_phdr[i];
        if (phdr.p_type != PT_DYNAMIC) continue;
        const ElfW(Dyn)* dyn = reinterpret_cast<const ElfW(Dyn)*>(
            result->base + phdr.p_vaddr);
        for (; dyn->d_tag != DT_NULL; ++dyn) {
            if (dyn->d_tag == DT_INIT_ARRAY)
                result->array = reinterpret_cast<MCEInitFn*>(
                    result->base + dyn->d_un.d_ptr);
            else if (dyn->d_tag == DT_INIT_ARRAYSZ)
                result->size = (size_t)dyn->d_un.d_val;
        }
        return 1;
    }
    return 1;
}

struct MCECtorSignalAction {
    int signal_number;
    struct sigaction previous;
    volatile sig_atomic_t installed;
};

static MCECtorSignalAction g_ctor_signal_actions[] = {
    { SIGSEGV, {}, false },
    { SIGBUS,  {}, false },
    { SIGFPE,  {}, false },
    { SIGILL,  {}, false },
};
static thread_local sigjmp_buf g_ctor_jump_env;
static thread_local volatile sig_atomic_t g_ctor_jump_active = 0;

static const char* mce_ctor_signal_name(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV";
        case SIGBUS:  return "SIGBUS";
        case SIGFPE:  return "SIGFPE";
        case SIGILL:  return "SIGILL";
        default:      return "SIG?";
    }
}

static void mce_ctor_signal_handler(int sig, siginfo_t* info, void* context) {
    if (g_ctor_jump_active && info && info->si_code > 0) {
        g_ctor_jump_active = 0;
        siglongjmp(g_ctor_jump_env, sig);
    }

    // Preserve the pre-existing crash handler for faults on other threads.
    for (size_t i = 0; i < sizeof(g_ctor_signal_actions) / sizeof(g_ctor_signal_actions[0]); ++i) {
        const MCECtorSignalAction& saved = g_ctor_signal_actions[i];
        if (saved.signal_number != sig || !saved.installed) continue;
        if (saved.previous.sa_handler == SIG_IGN) return;
        if (saved.previous.sa_handler != SIG_DFL) {
            if (saved.previous.sa_flags & SA_SIGINFO)
                saved.previous.sa_sigaction(sig, info, context);
            else
                saved.previous.sa_handler(sig);
            return;
        }
        break;
    }
    _exit(128 + sig);
}

static bool mce_install_ctor_signal_handlers() {
    struct sigaction action = {};
    action.sa_sigaction = mce_ctor_signal_handler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    const size_t count = sizeof(g_ctor_signal_actions) / sizeof(g_ctor_signal_actions[0]);
    // Snapshot every prior action before installing any handler, so a signal
    // arriving during setup can still be chained to the original handler.
    for (size_t i = 0; i < count; ++i) {
        MCECtorSignalAction& saved = g_ctor_signal_actions[i];
        if (sigaction(saved.signal_number, nullptr, &saved.previous) != 0) {
            fprintf(stderr, "[init-array] cannot read handler for %s\n",
                    mce_ctor_signal_name(saved.signal_number));
            return false;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        MCECtorSignalAction& saved = g_ctor_signal_actions[i];
        if (sigaction(saved.signal_number, &action, nullptr) != 0) {
            fprintf(stderr, "[init-array] cannot install handler for %s\n",
                    mce_ctor_signal_name(saved.signal_number));
            for (size_t j = i; j > 0; --j) {
                MCECtorSignalAction& prior = g_ctor_signal_actions[j - 1];
                if (prior.installed) {
                    sigaction(prior.signal_number, &prior.previous, nullptr);
                    prior.installed = false;
                }
            }
            return false;
        }
        saved.installed = true;
    }
    return true;
}

static void mce_restore_ctor_signal_handlers() {
    for (size_t i = sizeof(g_ctor_signal_actions) / sizeof(g_ctor_signal_actions[0]); i > 0; --i) {
        MCECtorSignalAction& saved = g_ctor_signal_actions[i - 1];
        if (saved.installed) {
            sigaction(saved.signal_number, &saved.previous, nullptr);
            saved.installed = false;
        }
    }
}

static void mce_log_ctor_exception(size_t index, const char* name,
                                   const std::type_info* exception_type,
                                   const char* message) {
    int status = -1;
    char* demangled = exception_type
        ? abi::__cxa_demangle(exception_type->name(), nullptr, nullptr, &status)
        : nullptr;
    const char* type_name = (demangled && status == 0)
        ? demangled : (exception_type ? exception_type->name() : "unknown");
    if (message)
        fprintf(stderr, "[init-array] #%zu EXCEPTION %s type=%s what=\"%s\"\n",
                index, name, type_name, message);
    else
        fprintf(stderr, "[init-array] #%zu EXCEPTION %s type=%s\n",
                index, name, type_name);
    free(demangled);
}

static void mce_run_remaining_init_array() {
    static bool attempted = false;
    if (attempted) return;
    attempted = true;

    Dl_info self = {};
    if (!dladdr((void*)&mce_run_remaining_init_array, &self) || !self.dli_fbase) {
        fprintf(stderr, "[init-array] dladdr could not locate this library\n");
        return;
    }

    MCEInitArrayInfo array = {
        (ElfW(Addr))self.dli_fbase, nullptr, 0
    };
    dl_iterate_phdr(mce_find_init_array, &array);
    fprintf(stderr, "[init-array] address=%p dynamic-size=%zu\n",
            (void*)array.array, array.size);

    constexpr size_t kFullEntryCount = 129;
    constexpr size_t kLoaderEntryCount = 5;
    if (!array.array) {
        fprintf(stderr, "[init-array] array address unavailable\n");
        return;
    }
    if (array.size == kFullEntryCount * sizeof(MCEInitFn)) {
        fprintf(stderr, "[init-array] full array already ran in dlopen; skipping\n");
        return;
    }
    if (array.size != kLoaderEntryCount * sizeof(MCEInitFn)) {
        fprintf(stderr, "[init-array] unexpected size=%zu; expected 40 or 1032; skipping\n",
                array.size);
        return;
    }

    if (!mce_install_ctor_signal_handlers()) return;

    size_t ok = 0, exceptions = 0, native_faults = 0;
    for (size_t i = kLoaderEntryCount; i < kFullEntryCount; ++i) {
        MCEInitFn fn = array.array[i];
        if (!fn) {
            fprintf(stderr, "[init-array] #%zu null (entry unavailable)\n", i);
            continue;
        }
        Dl_info entry = {};
        const char* name = (dladdr((void*)fn, &entry) && entry.dli_sname)
                         ? entry.dli_sname : "<unknown>";
        fprintf(stderr, "[init-array] #%zu begin %s (%p)\n", i, name, (void*)fn);

        // A native fault skips this ctor's C++ stack unwinding by design.
        const int fault_signal = sigsetjmp(g_ctor_jump_env, 1);
        if (fault_signal != 0) {
            g_ctor_jump_active = 0;
            ++native_faults;
            fprintf(stderr, "[init-array] #%zu NATIVE_FAULT %s (%d) %s (%p)\n",
                    i, mce_ctor_signal_name(fault_signal), fault_signal, name, (void*)fn);
            continue;
        }

        g_ctor_jump_active = 1;
        try {
            fn();
            g_ctor_jump_active = 0;
            ++ok;
            fprintf(stderr, "[init-array] #%zu ok %s\n", i, name);
        } catch (const std::exception& e) {
            g_ctor_jump_active = 0;
            ++exceptions;
            mce_log_ctor_exception(i, name, &typeid(e), e.what());
        } catch (...) {
            g_ctor_jump_active = 0;
            ++exceptions;
            mce_log_ctor_exception(i, name, abi::__cxa_current_exception_type(), nullptr);
        }
    }
    g_ctor_jump_active = 0;
    mce_restore_ctor_signal_handlers();
    fprintf(stderr, "[init-array] completed ok=%zu exceptions=%zu native_faults=%zu\n",
            ok, exceptions, native_faults);
}

extern "C" int __mce_run() {
    write(2, "[mce] installing handlers\\n", 26);
    struct sigaction sa = {};
    sa.sa_sigaction = mce_segv_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);

    write(2, "[mce] __mce_run entered\\n", 24);
    mce_run_remaining_init_array();
    __mce_construct_app();
    write(2, "[mce] app constructed, calling Minecraft::start\\n", 48);

    MCEW("[mce] media setup\n");
    if (chdir("/data/user/0/com.mce.legacy/files") != 0) {
        MCEW("[mce] chdir FAILED\n");
    } else {
        MCEW("[mce] chdir OK\n");
        struct stat st_src, st_dst;
        int need = 1;
        if (stat("MediaPS3.arc", &st_src) == 0 &&
            stat("Common\\Media\\MediaPS3.arc", &st_dst) == 0 &&
            st_src.st_size == st_dst.st_size) {
            need = 0;
            MCEW("[mce] media present\n");
        }
        if (need) {
            FILE* fin = fopen("MediaPS3.arc", "rb");
            if (!fin) { MCEW("[mce] fopen src FAILED\n"); }
            else {
                FILE* fout = fopen("Common\\Media\\MediaPS3.arc", "wb");
                if (!fout) { MCEW("[mce] fopen dst FAILED\n"); fclose(fin); }
                else {
                    char b[65536]; size_t tot = 0, k;
                    while ((k = fread(b, 1, sizeof(b), fin)) > 0) {
                        fwrite(b, 1, k, fout); tot += k;
                    }
                    fclose(fin); fclose(fout);
                    char m[64];
                    int ml = snprintf(m, sizeof(m), "[mce] copied %lu\n", (unsigned long)tot);
                    write(2, m, ml);
                }
            }
        }
    }
    MCEW("[mce] calling loadMediaArchive\n");
    g_consoleApp.loadMediaArchive();
    MCEW("[mce] loadMediaArchive done\n");
    g_consoleApp.loadStringTable();
    MCEW("[mce] loadStringTable done\n");
    /* MCE-MEDIA-PATCH (TEMP) */
    {
        const char* kDir = "/data/user/0/com.mce.legacy/files";
        const char* kSrc = "/data/user/0/com.mce.legacy/files/MediaPS3.arc";
        char dst[256];
        snprintf(dst, sizeof(dst), "%s/Common\\Media\\MediaPS3.arc", kDir);
        mce_media_msg("[mce-media] begin\n");
        bool ok = mce_media_copy(kSrc, dst);
        if (chdir(kDir) != 0) mce_media_msg("[mce-media] chdir FAILED\n");
        else mce_media_msg("[mce-media] chdir ok\n");
        if (ok) {
            mce_media_msg("[mce-media] calling loadMediaArchive\n");
            app.loadMediaArchive();
            mce_media_msg("[mce-media] loadMediaArchive returned\n");
            mce_media_msg("[mce-media] calling loadStringTable\n");
            app.loadStringTable();
            mce_media_msg("[mce-media] loadStringTable returned\n");
        } else {
            mce_media_msg("[mce-media] skipped (no archive)\n");
        }
    }
    static bool s_statics_inited = false;
    fprintf(stderr, "[dbg-a] pre ERD=%p\n", (void*)EntityRenderDispatcher::instance);
    if (!s_statics_inited) {
        s_statics_inited = true;
#define MCE_TRY(name, expr) do { fprintf(stderr, "[%s] in\n", name); try { expr; fprintf(stderr, "[%s] ok\n", name); } catch (const std::exception& e) { fprintf(stderr, "[%s] EXC: %s\n", name, e.what()); } catch (...) { fprintf(stderr, "[%s] EXC: unknown\n", name); } } while(0)

        MCE_TRY("s1", MinecraftWorld_RunStaticCtors());
        MCE_TRY("s2", EntityRenderDispatcher::staticCtor());
        MCE_TRY("s3", TileEntityRenderDispatcher::staticCtor());
        MCE_TRY("s4", User::staticCtor());
        MCE_TRY("s5", Tutorial::staticCtor());
        MCE_TRY("s6", ColourTable::staticCtor());
        MCE_TRY("s7", g_consoleApp.loadDefaultGameRules());
        fprintf(stderr, "[s8] done\n");
    }
    fprintf(stderr, "[dbg-b] post ERD=%p TED=%p\n",
        (void*)EntityRenderDispatcher::instance,
        (void*)TileEntityRenderDispatcher::instance);
    Minecraft::start(std::wstring(L"Player"), std::wstring(L"-"));

    write(2, "[mce] Minecraft::start returned\\n", 32);
    return 0;
}
