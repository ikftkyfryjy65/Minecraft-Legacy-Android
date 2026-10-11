#ifndef COMPAT_XBOX_LEGACY_H
#define COMPAT_XBOX_LEGACY_H

/* ============================================================
 * Original Xbox (XBX) + locale + XACT symbols — no-op stubs.
 * These come from the Xbox 360 XDK. On Android they are stubs.
 * Real implementations not needed for compilation or initial boot.
 * ============================================================ */

/* ---- XBX language / user / storage ---- */
#ifndef XBX_STORAGE_DECLINED
#define XBX_STORAGE_DECLINED     ((DWORD)0xFFFFFFFE)
#endif




/* ---- XACT audio notification type ---- */
#ifndef XACT_NOTIFICATION_TYPE_DEFINED
#define XACT_NOTIFICATION_TYPE_DEFINED 1
typedef struct XACT_NOTIFICATION {
    int   type;
    void* pCue;
    void* pSoundBank;
    void* pWaveBank;
    int   cueIndex;
    int   soundBankIndex;
    int   waveBankIndex;
    void* pvContext;
} XACT_NOTIFICATION;
#endif

/* ---- Xbox 360 locale constants ---- */
#ifndef XC_LANGUAGE_ENGLISH
enum XC_LANGUAGE {
    XC_LANGUAGE_ENGLISH              = 1,
    XC_LANGUAGE_JAPANESE             = 2,
    XC_LANGUAGE_GERMAN               = 3,
    XC_LANGUAGE_FRENCH               = 4,
    XC_LANGUAGE_SPANISH              = 5,
    XC_LANGUAGE_ITALIAN              = 6,
    XC_LANGUAGE_KOREAN               = 7,
    XC_LANGUAGE_CHINESE_TRADITIONAL  = 8,
    XC_LANGUAGE_CHINESE_SIMPLIFIED   = 9,
    XC_LANGUAGE_PORTUGUESE           = 10,
    XC_LANGUAGE_POLISH               = 11,
    XC_LANGUAGE_RUSSIAN              = 12,
    XC_LANGUAGE_BRAZILIAN             = 13,
    XC_LANGUAGE_DUTCH                = 14,
    XC_LANGUAGE_CZECH                = 15,
    XC_LANGUAGE_DANISH               = 16,
    XC_LANGUAGE_FINISH               = 17,
    XC_LANGUAGE_BNORWEGIAN           = 18,
    XC_LANGUAGE_GREEK                = 19,
    XC_LANGUAGE_LATINAMERICANSPANISH = 20,
    XC_LANGUAGE_MEXICANSPANISH       = 21,
    XC_LANGUAGE_SWEDISH              = 22,
    XC_LANGUAGE_TURKISH              = 23
};
#endif

#endif /* COMPAT_XBOX_LEGACY_H */
