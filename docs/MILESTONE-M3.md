# MILESTONE M3.0 - Minecraft LCE on Android

**Date:** 2026-10-04
**Device:** Infinix Note 40, Android 16, ARM64
**Env:** Termux (aarch64-linux-android30)

---

## Achievements

### M1 - Linux Binary
- 703/703 World files compiled
- 328/329 Client files compiled
- ELF ARM64 binary (~72 MB)
- main() runs successfully

### M2 - APK installs
- NativeActivity APK (16 KB), v2+v3 signature
- Installs on Android 16

### M2.5 - Binary inside APK
- 72 MB binary in assets/
- NativeActivity wrapper (C) extracts it
- /system/bin/linker64 executes it
- Binary runs: [main] === SUCCESS ===

### M3.0 - EGL works
- EGL 1.5 + GLESv2 in wrapper
- Render thread 60 FPS
- Screen shows blue gradient
- Binary + Renderer work together

---

## Final Structure

M3.0.apk (68 MB)
- AndroidManifest.xml    (NativeActivity)
- lib/arm64-v8a/
  - libmain.so         (16.5 KB, C wrapper + EGL)
- assets/
  - Minecraft.Client   (72 MB, binary + patch)

---

## M3.1 - State Bridge (2026-10-04)

### What works
- wrapper monitors binary state (idle/running/ok/err)
- Screen color reflects state:
  - Blue = wrapper idle
  - Cyan = binary running
  - Green = binary exit 0
  - Red = binary crashed
- mce_cmd.bin (4 KB) created for future bridge

### Log evidence
[render] state=2 rgb(0.10,0.85,0.20)
[binary] exited code 0

### What is pending (M3.2)
- binary writes to mce_cmd.bin (needs rebuild of Linux_Globals.cpp)
- binary uses EGL via bridge
- requires rebuild with ccache to avoid OOM

### Milestones Timeline
- M1 (Linux binary)       2026-10-03
- M2 (APK installs)       2026-10-03
- M2.5 (binary in APK)    2026-10-04
- M3.0 (EGL blue)         2026-10-04
- M3.1 (state bridge)     2026-10-04  <= we are here

---

## M3.2 UI + Title (2026-10-04)

### What works
- 6 colored progress boxes (one per binary stage)
- Progress bar
- "MINECRAFT LCE" text via custom 5x7 pixel font
- All rendered via glScissor + glClear (no textures)

### Visual Result
- Dark background
- 6 boxes: blue, purple, orange, yellow, teal, green
- White progress bar (fills as stages advance)
- "MINECRAFT LCE" centered

### Evidence
Screenshot shows title + all boxes + full progress bar.

### Final APK
- ~/apk-test/M3.2TXT.apk (68 MB)
- /sdcard/Download/MinecraftLCE.apk
