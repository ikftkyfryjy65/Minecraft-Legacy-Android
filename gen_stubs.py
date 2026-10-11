import subprocess, re

SKIP_NS = [r'^_ZNSt', r'^_ZNKSt', r'^_ZSt', r'^_ZN9__gnu_cxx']
def keep(s):
    if not s.startswith('_Z'): return False
    return not any(re.match(p, s) for p in SKIP_NS)

with open('undefined_symbols.txt') as f:
    symbols = [l.strip() for l in f if l.strip()]
filtered = [s for s in symbols if keep(s)]
print(f"Total: {len(symbols)}, Kept: {len(filtered)}")

HEADER = '''/* AUTO-GENERATED stubs — safe fallback before arena init */
#include <sys/mman.h>

static void* __mce_fake_vtable[8192];
static unsigned char* __mce_arena = nullptr;
static unsigned char __mce_fallback[16384] __attribute__((aligned(16)));

extern "C" void* __mce_null_method();

static inline void* __mce_obj(size_t idx) {
    if (!__mce_arena) {
        *(void**)__mce_fallback = __mce_fake_vtable;
        return __mce_fallback;
    }
    unsigned char* p = __mce_arena + (idx % 16384) * 16384;
    *(void**)p = __mce_fake_vtable;
    return p;
}

/* ctor 101: vtable setup + fallback patching (very early) */
__attribute__((constructor(101)))
static void __mce_vtable_init() {
    for (int i = 0; i < 8192; i++)
        __mce_fake_vtable[i] = (void*)&__mce_null_method;
    *(void**)__mce_fallback = __mce_fake_vtable;
}

/* ctor 102: mmap arena */
__attribute__((constructor(102)))
static void __mce_arena_init() {
    __mce_arena = (unsigned char*)mmap(nullptr, 256*1024*1024,
        PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
}

extern "C" void* __mce_null_method() { return __mce_obj(0); }
'''

out = [HEADER]
funcs = datas = 0

for i, sym in enumerate(filtered):
    try:
        dem = subprocess.check_output(['c++filt', sym], stderr=subprocess.DEVNULL).decode().strip()
    except: dem = sym
    dem_clean = dem.replace('*/', '* /').replace('\n', ' ')

    if dem.endswith(')'):
        out.append(f'/* {dem_clean} */')
        out.append(f'extern "C"  void* __f_{i}(...) asm("{sym}");')
        out.append(f' void* __f_{i}(...) {{ return __mce_obj({i}); }}')
        out.append('')
        funcs += 1
    else:
        out.append(f'/* {dem_clean} */')
        out.append(f'extern "C"  char __d_{i}[256] asm("{sym}");')
        out.append(f' char __d_{i}[256] = {{0}};')
        out.append('')
        datas += 1

with open('Minecraft.Client/Linux/Linux_AutoStubs.cpp','w') as f:
    f.write('\n'.join(out) + '\n')
print(f"Generated: {funcs} funcs + {datas} data")
