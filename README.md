# Minecraft Legacy PE

**An attempt to port Minecraft Legacy Console Edition (PS3 / TU25) to Android ARM64.**

---

## About the Developer

- **Handle:** ikftkyfryjy65
- **Device:** Infinix Note 40 (Helio G99, 8 GB RAM, Android 16)
- **Tools:** One phone. No PC. No team.
- **Budget:** Zero. No GitHub Actions. No servers. No expensive AI subscription.

---

## The Story

On **October 3, 2026**, I started working on porting the original `libMinecraft.Client.so` binary from PS3 to Android ARM64.

**3 days later (October 6, 2026):**

- Built a complete APK that installs and runs on Android 16
- Extracted all assets (670 files) from `MediaPS3.arc`
- Wired EGL context + threading + TLS
- Survived 8 consecutive crashes
- Documented every step

**And then hit a wall:**

`MinecraftWorld_RunStaticCtors()` crashes on the first ctor. The real reason:

> **Static Initialization Order Fiasco (SIOF)** — Decompiling a PS3 binary means losing 124 of 129 static initializers, and they cannot be recovered without source code.

This is a structural limit, not a fixable bug.

---

## Progress Achieved

| Milestone | Description | Status |
|---|---|---|
| M1 | Linux binary (72 MB) runs | ✅ |
| M2 | APK installs on Android 16 | ✅ |
| M2.5 | Binary runs via linker64 | ✅ |
| M3.0 | EGL + 60 FPS display | ✅ |
| M3.1–M3.2 | State bridge + stdout reader + loading screen | ✅ |
| M3.3 | Binary runs up to `Minecraft::init` | 🔄 97% |

---

## The Real Solution

The project **[anhot11/LegacyMCPE](https://github.com/anhot11/LegacyMCPE)** (a fork of `portable-lce`) achieves the same goal — but from **source code**, not a decompiled PS3 binary.

- TU19 / 1.6.1 runs on Android ARM64
- Bedrock-style touch controls
- GPLv3 license
- Free

**If you ended up here — go there first. Don't reinvent what already exists.**

---

## What's in This Repository

- `docs/` — Full documentation of the attempt (every crash, every patch, every cause)
- `tools/` — My own code: `native_wrapper.c`, `set_arraysz.py`
- `wrapper/` — My Android entry point (`Linux_Main.cpp`)

**No 4J code here.** For deeper details, see `docs/`.

---

## Why I'm Stopping

1. **No money** — a strong AI subscription needs funds I don't have
2. **No more time** — the path is structurally blocked
3. **No equipment** — one phone isn't enough for a project the size of LCE

---

## The Promise

**God willing, if I live, I will return in a year.** With better equipment, a clearer mind, and a different approach.

**Until then — I hope this repository helps anyone who ends up here.**

---

## License

- **My code (in `tools/` and `wrapper/`)**: MIT — use it freely.
- **Documentation (in `docs/`)**: CC BY 4.0 — share it with attribution.
- **This repository contains no code or assets owned by Mojang or 4J Studios.**

---

لا إله إلا الله محمد رسول الله

---

**Date:** 2026-10-06
**Location:** Iraq
**By:** ikftkyfryjy65
**On:** Infinix Note 40
