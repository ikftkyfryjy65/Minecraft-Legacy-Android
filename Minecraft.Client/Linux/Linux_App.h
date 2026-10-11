#pragma once
#include "../Common/Consoles_App.h"

/* Linux/Android: platform-specific app class.
 * Mirrors Windows64_App.h — implements all pure virtuals from CMinecraftApp. */
class CConsoleMinecraftApp : public CMinecraftApp
{
public:
    CConsoleMinecraftApp();
    virtual ~CConsoleMinecraftApp();

    virtual void SetRichPresenceContext(int iPad, int contextId);
    virtual void CaptureSaveThumbnail();
    virtual void GetSaveThumbnail(PBYTE*, DWORD*);
    virtual void ReleaseSaveThumbnail();
    virtual void GetScreenshot(int iPad, PBYTE *pbData, DWORD *pdwSize);
    virtual void ReadBannedList(int iPad, eTMSAction action, bool bCallback);
    virtual int  LoadLocalTMSFile(WCHAR *wchTMSFile);
    virtual int  LoadLocalTMSFile(WCHAR *wchTMSFile, eFileExtensionType eExt);
    virtual void FreeLocalTMSFiles(eTMSFileType eType);
    virtual int  GetLocalTMSFileIndex(WCHAR *wchTMSFile, bool bFilenameIncludesExtension, eFileExtensionType eEXT);
};
