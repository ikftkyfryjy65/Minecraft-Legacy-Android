#include "stdafx.h"
#include "Linux_App.h"

/* Deferred construction — no static ctor */
alignas(16) static char g_app_storage[sizeof(CConsoleMinecraftApp)] = {};
CConsoleMinecraftApp& g_consoleApp = *reinterpret_cast<CConsoleMinecraftApp*>(g_app_storage);
CMinecraftApp& app = g_consoleApp;

CConsoleMinecraftApp::CConsoleMinecraftApp() : CMinecraftApp() {}
void __mce_construct_app() {
    new (&g_consoleApp) CConsoleMinecraftApp();
}

CConsoleMinecraftApp::~CConsoleMinecraftApp() {}
void CConsoleMinecraftApp::SetRichPresenceContext(int, int) {}
void CConsoleMinecraftApp::CaptureSaveThumbnail() {}
void CConsoleMinecraftApp::GetSaveThumbnail(PBYTE*, DWORD*) {}
void CConsoleMinecraftApp::ReleaseSaveThumbnail() {}
void CConsoleMinecraftApp::GetScreenshot(int, PBYTE*, DWORD*) {}
void CConsoleMinecraftApp::ReadBannedList(int, eTMSAction, bool) {}
int  CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR*) { return -1; }
int  CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR*, eFileExtensionType) { return -1; }
void CConsoleMinecraftApp::FreeLocalTMSFiles(eTMSFileType) {}
int  CConsoleMinecraftApp::GetLocalTMSFileIndex(WCHAR*, bool, eFileExtensionType) { return -1; }
