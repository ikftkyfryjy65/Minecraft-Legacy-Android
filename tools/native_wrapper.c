/* M3.3: Wrapper with dlopen for libMinecraft.Client.so */
#include <android/native_activity.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/asset_manager.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>

static int g_logfd = -1;
static int g_bootfd = -2;
static char g_bootpath[512] = "";
static void LOG(const char* fmt, ...) {
    char buf[1024];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    __android_log_print(ANDROID_LOG_ERROR, "MCE", "%s", buf);
    if (g_bootfd == -2 && g_bootpath[0]) {
        g_bootfd = open(g_bootpath, O_WRONLY|O_CREAT|O_APPEND, 0644);
    }
    if (g_bootfd >= 0) { write(g_bootfd, buf, n); write(g_bootfd, "\n", 1); fsync(g_bootfd); }
    if (g_logfd >= 0) { write(g_logfd, buf, n); write(g_logfd, "\n", 1); }
}

/* mmap command channel — same as before */
#define MCE_MAGIC   0x4D434531u
#define CMD_NONE    0
#define CMD_CLEAR   1
#define CMD_PRESENT 2
#define CMD_DRAW    3

struct MCECmd {
    volatile uint32_t magic;
    volatile uint32_t seq;
    volatile uint32_t cmd;
    volatile float    params[8];
    volatile uint32_t result;
    volatile uint32_t pad[8];
};

static struct MCECmd* g_cmd = NULL;
static int g_cmd_fd = -1;

/* M3.3N: shared EGL state between render + binary threads */
static EGLDisplay g_egl_display = EGL_NO_DISPLAY;
static EGLConfig  g_egl_config;
static EGLContext g_egl_render_ctx = EGL_NO_CONTEXT;
static pthread_mutex_t g_egl_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_egl_cv   = PTHREAD_COND_INITIALIZER;
static int g_egl_render_ready = 0;

static int cmd_channel_open(const char* path) {
    g_cmd_fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (g_cmd_fd < 0) return -1;
    if (ftruncate(g_cmd_fd, 4096) != 0) { close(g_cmd_fd); return -1; }
    void* p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, g_cmd_fd, 0);
    if (p == MAP_FAILED) { close(g_cmd_fd); return -1; }
    g_cmd = (struct MCECmd*)p;
    memset((void*)g_cmd, 0, 4096);
    g_cmd->magic = MCE_MAGIC;
    LOG("cmd channel ready: %s", path);
    return 0;
}

static ANativeWindow* g_window = NULL;
static pthread_mutex_t g_window_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int g_window_ready = 0;
static volatile int g_running = 1;
static volatile int g_binary_state = 0; /* 0=idle 1=running 2=ok 3=err */
static volatile int g_binary_stage = 0;
static volatile int g_win_w = 1080;
static volatile int g_win_h = 2400;
static pthread_t g_render_thread;
static pthread_t g_binary_thread;


static int copy_asset(AAssetManager* mgr, const char* asset, const char* dst) {
    AAsset* a = AAssetManager_open(mgr, asset, AASSET_MODE_STREAMING);
    if (!a) { LOG("Cannot open asset: %s", asset); return -1; }
    int fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd < 0) { LOG("Cannot create: %s errno=%d", dst, errno); AAsset_close(a); return -1; }
    char buf[65536]; int n; long total = 0;
    while ((n = AAsset_read(a, buf, sizeof(buf))) > 0) {
        if (write(fd, buf, n) != n) { close(fd); AAsset_close(a); return -1; }
        total += n;
    }
    close(fd); AAsset_close(a);
    LOG("Extracted %s (%ld bytes)", dst, total);
    return 0;
}

typedef int (*mce_run_fn)(void);

static void* binary_thread_fn(void* arg) {
    ANativeActivity* activity = (ANativeActivity*)arg;
    g_binary_state = 1;

    /* Paths */
    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/MCE-stdout.txt", activity->externalDataPath);

    /* Redirect stdout + stderr to file */
    int outfd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outfd >= 0) {
        dup2(outfd, 1);
        dup2(outfd, 2);
        close(outfd);
    }

    /* dlopen from lib/ (Android handles path automatically) */
    LOG("[binary] dlopen: libMinecraft.Client.so (from lib/arm64-v8a)");
    void* h = dlopen("libMinecraft.Client.so", RTLD_NOW | RTLD_GLOBAL);
    if (!h) {
        LOG("[binary] dlopen failed: %s", dlerror());
        g_binary_state = 3; return NULL;
    }
    LOG("[binary] dlopen OK, handle=%p", h);

    mce_run_fn run = (mce_run_fn)dlsym(h, "__mce_run");
    if (!run) {
        LOG("[binary] dlsym __mce_run failed: %s", dlerror());
        g_binary_state = 3; return NULL;
    }
    LOG("[binary] __mce_run found, calling...");

    /* M3.3N: install EGL context on this thread (shared with render thread) */
    {
        pthread_mutex_lock(&g_egl_lock);
        int waited = 0;
        while (!g_egl_render_ready && waited < 10000) {
            struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 200000000;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&g_egl_cv, &g_egl_lock, &ts);
            waited += 200;
        }
        EGLDisplay edpy = g_egl_display;
        EGLConfig  ecfg = g_egl_config;
        EGLContext eshr = g_egl_render_ctx;
        pthread_mutex_unlock(&g_egl_lock);

        if (edpy != EGL_NO_DISPLAY) {
            const EGLint ctxa[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
            EGLContext myctx = eglCreateContext(edpy, ecfg, eshr, ctxa);
            if (myctx == EGL_NO_CONTEXT) {
                LOG("[binary] eglCreateContext failed: 0x%x", eglGetError());
            } else {
                if (!eglMakeCurrent(edpy, EGL_NO_SURFACE, EGL_NO_SURFACE, myctx)) {
                    LOG("[binary] eglMakeCurrent(no-surface) failed: 0x%x", eglGetError());
                } else {
                    LOG("[binary] EGL context installed on binary thread");
                }
            }
        } else {
            LOG("[binary] no shared EGL display, skipping context setup");
        }
    }

    /* M3.3P: initialize thread-local storages (ctors that were skipped) */
    {
        /* M3.3U: patch uninitialized tlsIdx globals */
        struct { const char* sym; uint32_t val; } tls_patches[] = {
            { "_ZN4Vec36tlsIdxE",             1 },
            { "_ZN4AABB6tlsIdxE",             2 },
            { "_ZN5Chunk6tlsIdxE",            3 },
            { "_ZN8IntCache6tlsIdxE",         4 },
            { "_ZN11Compression6tlsIdxE",     5 },
            { "_ZN4Tile11tlsIdxShapeE",       6 },
            { "_ZN15OldChunkStorage6tlsIdxE", 7 },
            { "_ZN10Tesselator6tlsIdxE",      8 },
            { "_ZN6Entity6tlsIdxE",           9 },
            { "_ZN5Level6tlsIdxE",            10 },
            { "_ZN5Level16tlsIdxLightCacheE", 11 },
            { "_ZN12TheEndPortal6tlsIdxE",    12 },
            { "_ZN14PistonBaseTile6tlsIdxE",  13 },
        };
        for (size_t pi = 0; pi < sizeof(tls_patches)/sizeof(tls_patches[0]); pi++) {
            uint32_t* pp = (uint32_t*)dlsym(h, tls_patches[pi].sym);
            if (pp) {
                LOG("[binary] tlsIdx %s: %u -> %u", tls_patches[pi].sym, *pp, tls_patches[pi].val);
                *pp = tls_patches[pi].val;
            } else {
                LOG("[binary] tlsIdx MISSING: %s", tls_patches[pi].sym);
            }
        }

        typedef void (*tls_fn)(void);
        typedef void (*tls_fn_i)(int);
        struct { const char* name; tls_fn fn; } items[] = {
            { "_ZN4Vec322CreateNewThreadStorageEv",         NULL },
            { "_ZN4AABB22CreateNewThreadStorageEv",         NULL },
            { "_ZN5Chunk22CreateNewThreadStorageEv",        NULL },
            { "_ZN8IntCache22CreateNewThreadStorageEv",     NULL },
            { "_ZN11Compression22CreateNewThreadStorageEv", NULL },
            { "_ZN4Tile22CreateNewThreadStorageEv",         NULL },
            { "_ZN15OldChunkStorage22CreateNewThreadStorageEv", NULL },
        };
        for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
            items[i].fn = (tls_fn)dlsym(h, items[i].name);
            if (items[i].fn) {
                items[i].fn();
                LOG("[binary] TLS init OK: %s", items[i].name);
            } else {
                LOG("[binary] TLS MISSING: %s", items[i].name);
            }
        }
        tls_fn_i tess_fn = (tls_fn_i)dlsym(h, "_ZN10Tesselator22CreateNewThreadStorageEi");
        if (tess_fn) {
            tess_fn(1024);
            LOG("[binary] TLS init OK: Tesselator(1024)");
        } else {
            LOG("[binary] TLS MISSING: Tesselator");
        }
    }

    /* M3.3Q: check Vec3::tlsIdx after init */
    {
        uint32_t* pIdx = (uint32_t*)dlsym(h, "_ZN4Vec36tlsIdxE");
        void**   pDef = (void**)dlsym(h, "_ZN4Vec310tlsDefaultE");
        LOG("[dbg] Vec3::tlsIdx = %u", pIdx ? *pIdx : 999u);
        LOG("[dbg] Vec3::tlsDefault = %p", pDef ? *pDef : NULL);
    }

    int rc = run();
    LOG("[binary] __mce_run returned %d", rc);
    g_binary_state = (rc == 0) ? 2 : 3;
    return NULL;
}


static void* render_thread_fn(void* arg) {
    LOG("[render] Waiting for window...");
    int waited = 0;
    while (!g_window_ready && g_running) {
        usleep(50000); waited += 50;
        if (waited > 10000) { LOG("[render] timeout"); return NULL; }
    }
    if (!g_running) return NULL;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);
    const EGLint cfg[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RED_SIZE,8, EGL_GREEN_SIZE,8, EGL_BLUE_SIZE,8, EGL_ALPHA_SIZE,8,
        EGL_DEPTH_SIZE,16, EGL_NONE };
    EGLConfig config; EGLint n;
    eglChooseConfig(display, cfg, &config, 1, &n);
    const EGLint ctxa[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxa);

    pthread_mutex_lock(&g_window_lock);
    ANativeWindow* win = g_window;
    pthread_mutex_unlock(&g_window_lock);

    EGLSurface surface = eglCreateWindowSurface(display, config, win, NULL);
    eglMakeCurrent(display, surface, surface, ctx);
    pthread_mutex_lock(&g_egl_lock);
    g_egl_display = display;
    g_egl_config = config;
    g_egl_render_ctx = ctx;
    g_egl_render_ready = 1;
    pthread_cond_broadcast(&g_egl_cv);
    pthread_mutex_unlock(&g_egl_lock);
    LOG("[render] EGL ready, shared context published");
    LOG("[render] === RENDER LOOP STARTED ===");

    uint32_t last_seq = 0;
    int frame = 0;
    while (g_running) {
        int got_cmd = 0;
        if (g_cmd) {
            uint32_t s = g_cmd->seq;
            if (s != last_seq && g_cmd->magic == MCE_MAGIC) {
                last_seq = s;
                got_cmd = 1;
                uint32_t c = g_cmd->cmd;
                float r = g_cmd->params[0], g = g_cmd->params[1];
                float b = g_cmd->params[2], a = g_cmd->params[3];
                if (c == CMD_CLEAR || c == CMD_DRAW) {
                    glClearColor(r, g, b, a);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    if (frame % 30 == 0)
                        LOG("[cmd] clear rgb(%.2f,%.2f,%.2f) seq=%u", r,g,b,s);
                }
                if (c == CMD_PRESENT || c == CMD_DRAW) {
                    eglSwapBuffers(display, surface);
                }
                g_cmd->result = 0;
            }
        }
        if (!got_cmd) {
            /* Draw status UI */
            int st = g_binary_state;
            int sg = g_binary_stage;
            float r, g, b;
            if (st == 0) {
                float t = (float)(frame % 300) / 300.0f;
                r = 0.30f+0.25f*t; g = 0.55f+0.20f*t; b = 0.85f+0.15f*t;
            } else if (st == 1) {
                r = 0.08f; g = 0.08f; b = 0.12f;
            } else if (st == 2) {
                r = 0.08f; g = 0.08f; b = 0.12f;
            } else {
                r = 0.9f; g = 0.1f; b = 0.1f;
            }
            glClearColor(r, g, b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            eglSwapBuffers(display, surface);
            if (frame % 60 == 0 && st != 0)
                LOG("[render] state=%d stage=%d rgb(%.2f,%.2f,%.2f)", st, sg, r, g, b);
        }
        frame++;
        usleep(16000);
    }
    LOG("[render] Loop ended");
    return NULL;
}

static void cb_onNativeWindowCreated(ANativeActivity* a, ANativeWindow* w) {
    LOG("=== onNativeWindowCreated ===");
    pthread_mutex_lock(&g_window_lock);
    g_window = w; g_window_ready = 1;
    pthread_mutex_unlock(&g_window_lock);
}
static void cb_onNativeWindowDestroyed(ANativeActivity* a, ANativeWindow* w) {
    LOG("=== onNativeWindowDestroyed ===");
    pthread_mutex_lock(&g_window_lock);
    g_window = NULL; g_window_ready = 0;
    pthread_mutex_unlock(&g_window_lock);
}
static void cb_onResume(ANativeActivity* a) { LOG("onResume"); }
static void cb_onPause(ANativeActivity* a) { LOG("onPause"); }
static void cb_onStart(ANativeActivity* a) { LOG("onStart"); }
static void cb_onStop(ANativeActivity* a) { LOG("onStop"); }
static void cb_onInputQueueCreated(ANativeActivity* a, AInputQueue* q) { LOG("onInputQueueCreated"); }
static void cb_onInputQueueDestroyed(ANativeActivity* a, AInputQueue* q) { LOG("onInputQueueDestroyed"); }
static void cb_onContentRectChanged(ANativeActivity* a, const ARect* r) {
    g_win_w = r->right - r->left;
    g_win_h = r->bottom - r->top;
    LOG("onContentRectChanged: %dx%d", g_win_w, g_win_h);
}
static void cb_onConfigurationChanged(ANativeActivity* a) { LOG("onConfigurationChanged"); }
static void cb_onLowMemory(ANativeActivity* a) { LOG("onLowMemory"); }

void ANativeActivity_onCreate(ANativeActivity* activity, void* s, size_t sz) {
    if (activity->externalDataPath) {
        snprintf(g_bootpath, sizeof(g_bootpath), "%s/MCE-boot.log", activity->externalDataPath);
        mkdir(activity->externalDataPath, 0755);
    }
    LOG("[boot] onCreate ENTERED pid=%d", (int)getpid());
    char logpath[1024], cmdp[1024];
    if (activity->externalDataPath) {
        snprintf(logpath, sizeof(logpath), "%s/MCE-log.txt", activity->externalDataPath);
        snprintf(cmdp, sizeof(cmdp), "%s/mce_cmd.bin", activity->externalDataPath);
    } else {
        snprintf(logpath, sizeof(logpath), "%s/MCE-log.txt", activity->internalDataPath);
        snprintf(cmdp, sizeof(cmdp), "%s/mce_cmd.bin", activity->internalDataPath);
    }
    g_logfd = open(logpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    LOG("=== ANativeActivity_onCreate ===");
    LOG("internalDataPath: %s", activity->internalDataPath);

    /* MCE-ASSET-EXTRACT: MediaPS3.arc from APK assets -> internalDataPath */
    if (activity->assetManager) {
        char adst[768];
        snprintf(adst, sizeof(adst), "%s/MediaPS3.arc", activity->internalDataPath);
        struct stat ast;
        int need = 1;
        if (stat(adst, &ast) == 0 && ast.st_size == 10951528) {
            need = 0;
            LOG("[asset] MediaPS3.arc present (%ld bytes)", (long)ast.st_size);
        }
        if (need) {
            if (copy_asset(activity->assetManager, "MediaPS3.arc", adst) == 0)
                LOG("[asset] MediaPS3.arc extracted OK");
            else
                LOG("[asset] MediaPS3.arc extract FAILED");
        }
        /* Symlink: literal-backslash name -> POSIX path for CreateFileW */
        {
            char cdir[768], mdir[768], linkp[768];
            snprintf(cdir, sizeof(cdir), "%s/Common", activity->internalDataPath);
            snprintf(mdir, sizeof(mdir), "%s/Common/Media", activity->internalDataPath);
            mkdir(cdir, 0755);
            mkdir(mdir, 0755);
            snprintf(linkp, sizeof(linkp), "%s/Common/Media/MediaPS3.arc", activity->internalDataPath);
            unlink(linkp);
            int r = symlink(adst, linkp);
            LOG("[asset] symlink %s -> %s r=%d", linkp, adst, r);
        }
        /* Extract all res/ files from assets/mce-res/<rel> -> files/Common/res/<rel> */
        {
            AAsset* lst = AAssetManager_open(activity->assetManager, "mce_res_list.txt", AASSET_MODE_BUFFER);
            if (lst) {
                off_t lsz = AAsset_getLength(lst);
                char* lbuf = (char*)malloc((size_t)lsz+1);
                if (lbuf) {
                    AAsset_read(lst, lbuf, (size_t)lsz);
                    lbuf[lsz] = 0;
                    int okc = 0, failc = 0;
                    char* line = lbuf;
                    while (line && *line) {
                        char* eol = strchr(line, '\n');
                        if (eol) *eol = 0;
                        if (*line) {
                            char src[768], dst[1024], tmp[1024];
                            snprintf(src, sizeof(src), "mce-res/%s", line);
                            snprintf(dst, sizeof(dst), "%s/Common/res/%s", activity->internalDataPath, line);
                            snprintf(tmp, sizeof(tmp), "%s", dst);
                            for (char* q = tmp+1; *q; q++) if (*q == '/') { *q = 0; mkdir(tmp, 0755); *q = '/'; }
                            struct stat st;
                            if (stat(dst, &st) == 0 && st.st_size > 0) okc++;
                            else if (copy_asset(activity->assetManager, src, dst) == 0) okc++;
                            else failc++;
                        }
                        if (eol) line = eol+1; else break;
                    }
                    free(lbuf);
                    LOG("[asset] res: ok=%d fail=%d", okc, failc);
                }
                AAsset_close(lst);
            } else {
                LOG("[asset] mce_res_list.txt NOT FOUND");
            }
        }

        /* Support files -> disk under Common/res/TitleUpdate/res/ */
        {
            static const struct { const char* asset; const char* sub; } kList[] = {
                { "res/colours.col", "/Common/res/TitleUpdate/res/colours.col" },
                { NULL, NULL }
            };
            for (int i = 0; kList[i].asset; i++) {
                char dst[1024];
                snprintf(dst, sizeof(dst), "%s%s", activity->internalDataPath, kList[i].sub);
                char tmp[1024]; snprintf(tmp, sizeof(tmp), "%s", dst);
                for (char* q = tmp+1; *q; q++) { if (*q == '/') { *q = 0; mkdir(tmp, 0755); *q = '/'; } }
                struct stat st;
                if (stat(dst, &st) == 0 && st.st_size > 0) {
                    LOG("[asset] present %s (%ld)", dst, (long)st.st_size);
                    continue;
                }
                if (copy_asset(activity->assetManager, kList[i].asset, dst) == 0)
                    LOG("[asset] extracted %s", dst);
                else
                    LOG("[asset] FAILED %s", kList[i].asset);
            }
        }
    } else {
        LOG("[asset] assetManager NULL");
    }

    cmd_channel_open(cmdp);

    activity->callbacks->onNativeWindowCreated = cb_onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = cb_onNativeWindowDestroyed;
    activity->callbacks->onStart = cb_onStart;
    activity->callbacks->onResume = cb_onResume;
    activity->callbacks->onPause = cb_onPause;
    activity->callbacks->onStop = cb_onStop;
    activity->callbacks->onInputQueueCreated = cb_onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = cb_onInputQueueDestroyed;
    activity->callbacks->onContentRectChanged = cb_onContentRectChanged;
    activity->callbacks->onConfigurationChanged = cb_onConfigurationChanged;
    activity->callbacks->onLowMemory = cb_onLowMemory;

    pthread_create(&g_render_thread, NULL, render_thread_fn, activity);
    pthread_create(&g_binary_thread, NULL, binary_thread_fn, activity);
    LOG("Threads started");
}

void ANativeActivity_onDestroy(ANativeActivity* activity) {
    LOG("=== onDestroy ===");
    g_running = 0;
    pthread_join(g_render_thread, NULL);
    pthread_join(g_binary_thread, NULL);
    if (g_cmd) munmap((void*)g_cmd, 4096);
    if (g_cmd_fd >= 0) close(g_cmd_fd);
    if (g_logfd >= 0) close(g_logfd);
}
