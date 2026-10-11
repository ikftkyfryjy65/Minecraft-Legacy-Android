#pragma once
/* Xbox Live social posting stubs — real impl would use Android intents. */
#ifndef XSOCIAL_STUB_DEFINED
#define XSOCIAL_STUB_DEFINED

#include <cstdint>

#define XSOCIAL_CAPABILITY_POSTIMAGE  0x01
#define XSOCIAL_CAPABILITY_POSTLINK   0x02

typedef struct {
    unsigned char*  pbImage;
    uint32_t        cbImage;
    uint32_t        dwWidth;
    uint32_t        dwHeight;
} XSOCIAL_PREVIEWIMAGE;

typedef struct {
    uint64_t        xuid;
    const wchar_t*  wszCaption;
    const wchar_t*  wszDescription;
    unsigned char*  pbImage;
    uint32_t        cbImage;
} XSOCIAL_IMAGEPOSTPARAMS;

typedef struct {
    uint64_t        xuid;
    const wchar_t*  wszCaption;
    const wchar_t*  wszDescription;
    const wchar_t*  wszLink;
} XSOCIAL_LINKPOSTPARAMS;

#endif
