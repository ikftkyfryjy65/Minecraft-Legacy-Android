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
