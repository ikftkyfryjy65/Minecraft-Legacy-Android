# PATCHES LOG — Minecraft LCE → Android ARM64

> كل patch له: الهدف، الملفات، الأثر، rollback
> آخر تحديث: Patch 114 (FORCEINLINE → static inline)

---

## Phase 1: Architecture Mapping ✅
- **الهدف:** فهم بنية المشروع
- **النتيجة:** World=716 ملف, Client=542 ملف, 4J interfaces موجودة كـheaders

## Phase 2: World Compilation ✅
- **النتيجة:** 703/703 ملف، libMinecraft.World.a = 141MB
- **الملف الحيوي:** `Minecraft.World/linux/xbox_valve.h` (stubs لـXBX_*)
- **rollback:** `xbox_valve.h.bak.114` قبل تغيير FORCEINLINE

## Phase 3: Client Compilation ✅
- **النتيجة:** 314/314 ملف مُفعَّل، Objects=314
- **الملفات الحيوية:**
  - `compat/include/compat.h` — Win32/Xbox shims
  - `compat/include/platform_android_types.h` — Platform types
  - `Minecraft.Client/stdafx.h` — 6 branches لـLinux
  - `Minecraft.Client/Linux/` — 4JLibs port

## Phase 4: Client Linking ⏳ (جاري)
- **النتيجة الحالية:** 22 undefined symbol
- **سببها:** ملفات أساسية معلَّقة في CMakeLists.txt:
  - MultiPlayerGameMode.cpp → 20 symbols
  - main() → لم يُكتب بعد
- **الحل:** تفعيل الملفات + كتابة entry point

---

## أهم الـPatches (Chronological)

| # | الهدف | ملف | rollback |
|---|-------|------|----------|
| 56 | network stubs | compat_network_stub.h | .bak |
| 64 | Iggy + Social | compat_social_stub.h | .bak |
| 66 | Linux UI | Linux_UIController.h | .bak1, .bak2 |
| 71 | IDS_* bulk (1247) | compat.h | .bak4 |
| 76 | VK_PAD/XC_LOCALE/XGet* | compat.h | .bak6 |
| 81 | GL_* constants | compat.h | (لاحقاً أُزيلت) |
| 89 | CELL_FS + XCONTENT_DATA | Linux/4JStorage.h | .bak3 |
| 94 | unified PS3/Linux branches | Consoles_App.cpp | .bak6 |
| 100| إزالة C4JRender المزيف | compat.h | .bak.BEFORE_FIX100 |
| 101| حذف C4JRender فقط | compat.h | .bak.102 |
| 102| GL_* removal + 4J_Render position | compat.h + stdafx.h | .bak.102 |
| 105| CreateFile variadic | compat.h | .bak.104 |
| 108| C4JRender return types | Linux/4J_Render.h | .bak5 |
| 111| CreateFile canonical | compat.h | .bak.111 |
| 114| FORCEINLINE → static inline | xbox_valve.h | .bak.114 |

---

## الملفات الأساسية (احذر التعديل عليها)
- `compat/include/compat.h` — 7000+ سطر، مركز كل shims
- `compat/include/platform_android_types.h` — C_4JProfile + XUSER_*
- `Minecraft.Client/stdafx.h` — ترتيب الـincludes حرج
- `Minecraft.World/linux/xbox_valve.h` — World header، تعديله يفرض rebuild 141MB
- `Minecraft.Client/Linux/4JLibs/inc/4J_Render.h` — C4JRender الحقيقي

---

## Lessons Learned (من الأخطاء السابقة)
1. **لا bulk-add stubs** — تكسر return types
2. **regex يخترق أحياناً أقواس enum** — تحقق يدوياً
3. **PCH يحتاج reconfigure** بعد أي header change
4. **GL_* قيم فريدة** — 0 يسبب duplicate case
5. **-ferror-limit=0** لرؤية كل الأخطاء
6. **static inline** لـfunctions في header لمنع duplicate symbol
