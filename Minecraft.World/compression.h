#pragma once
#include "FileHeader.h"

#if !defined(__ORBIS__) && !defined(__PS3__) && !defined(_XBOX_ONE)
typedef void* XMEMDECOMPRESSION_CONTEXT;
typedef void* XMEMCOMPRESSION_CONTEXT;
#endif

#ifdef _XBOX_ONE
#include "../Minecraft.Client/Durango/DurangoExtras/xcompress.h"
#endif

class Compression
{
public:
    enum ECompressionTypes
    {
        eCompressionType_None = 0,
        eCompressionType_RLE = 1,
        eCompressionType_LZXRLE = 2,
        eCompressionType_ZLIBRLE = 3,
        eCompressionType_PS3ZLIB = 4
    };

private:
    class ThreadStorage
    {
    public:
        Compression *compression;
        ThreadStorage();
        ~ThreadStorage();
    };

    static DWORD tlsIdx;
    static ThreadStorage *tlsDefault;

public:
    static void CreateNewThreadStorage();
    static void UseDefaultThreadStorage();
    static void ReleaseThreadStorage();
    static Compression *getCompression();

public:
    HRESULT Compress(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);
    HRESULT Decompress(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);
    HRESULT CompressLZXRLE(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);
    HRESULT DecompressLZXRLE(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);
    HRESULT CompressRLE(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);
    HRESULT DecompressRLE(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);

#ifndef _XBOX
    static VOID VitaVirtualDecompress(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);
#endif

    void SetDecompressionType(ECompressionTypes type) { m_decompressType = type; }
    ECompressionTypes GetDecompressionType() { return m_decompressType; }
    void SetDecompressionType(ESavePlatform platform);

    Compression();
    ~Compression();

private:
    HRESULT DecompressWithType(void *pDestination, unsigned int *pDestSize, void *pSource, unsigned int SrcSize);

#if defined __ORBIS__ || defined __PS3__
#else
    XMEMCOMPRESSION_CONTEXT compressionContext;
    XMEMDECOMPRESSION_CONTEXT decompressionContext;
#endif

    CRITICAL_SECTION rleCompressLock;
    CRITICAL_SECTION rleDecompressLock;

    unsigned char rleCompressBuf[1024*100];
    static const unsigned int staticRleSize = 1024*200;
    unsigned char rleDecompressBuf[staticRleSize];
    ECompressionTypes m_decompressType;
    ECompressionTypes m_localDecompressType;
};

#if defined __ORBIS__ || defined _DURANGO || defined _WIN64 || defined __PSVITA__
#define APPROPRIATE_COMPRESSION_TYPE Compression::eCompressionType_ZLIBRLE
#elif defined __PS3__
#define APPROPRIATE_COMPRESSION_TYPE Compression::eCompressionType_PS3ZLIB
#else
#define APPROPRIATE_COMPRESSION_TYPE Compression::eCompressionType_LZXRLE
#endif
