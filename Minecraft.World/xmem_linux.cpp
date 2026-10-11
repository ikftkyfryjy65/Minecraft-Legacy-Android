#include "stdafx.h"

#ifndef __ORBIS__
#ifndef __PS3__
#ifndef _XBOX_ONE

typedef VOID * XMEMDECOMPRESSION_CONTEXT;
typedef VOID * XMEMCOMPRESSION_CONTEXT;

typedef enum _XMEMCODEC_TYPE
{
    XMEMCODEC_DEFAULT = 0,
    XMEMCODEC_LZX = 1
} XMEMCODEC_TYPE;

typedef struct _XMEMCODEC_PARAMETERS_LZX
{
    DWORD Flags;
    DWORD WindowSize;
    DWORD CompressionPartitionSize;
} XMEMCODEC_PARAMETERS_LZX;

HRESULT XMemDecompress(
    XMEMDECOMPRESSION_CONTEXT,
    VOID *pDestination,
    SIZE_T *pDestSize,
    CONST VOID *pSource,
    SIZE_T SrcSize)
{
    if (!pDestination || !pDestSize || !pSource)
        return E_FAIL;

    if (*pDestSize < SrcSize)
        return E_FAIL;

    memcpy(pDestination, pSource, SrcSize);
    *pDestSize = SrcSize;
    return S_OK;
}

HRESULT XMemCompress(
    XMEMCOMPRESSION_CONTEXT,
    VOID *pDestination,
    SIZE_T *pDestSize,
    CONST VOID *pSource,
    SIZE_T SrcSize)
{
    if (!pDestination || !pDestSize || !pSource)
        return E_FAIL;

    if (*pDestSize < SrcSize)
        return E_FAIL;

    memcpy(pDestination, pSource, SrcSize);
    *pDestSize = SrcSize;
    return S_OK;
}

HRESULT XMemCreateCompressionContext(
    XMEMCODEC_TYPE,
    CONST VOID *,
    DWORD,
    XMEMCOMPRESSION_CONTEXT *pContext)
{
    if (!pContext)
        return E_FAIL;

    *pContext = NULL;
    return S_OK;
}

HRESULT XMemCreateDecompressionContext(
    XMEMCODEC_TYPE,
    CONST VOID *,
    DWORD,
    XMEMDECOMPRESSION_CONTEXT *pContext)
{
    if (!pContext)
        return E_FAIL;

    *pContext = NULL;
    return S_OK;
}

void XMemDestroyCompressionContext(XMEMCOMPRESSION_CONTEXT)
{
}

void XMemDestroyDecompressionContext(XMEMDECOMPRESSION_CONTEXT)
{
}

#endif
#endif
#endif
