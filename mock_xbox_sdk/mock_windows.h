#ifndef MOCK_WINDOWS_H
#define MOCK_WINDOWS_H

#include <cstdint>
#include <cstdio>
#include <string>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif

// نكتفي بتعريف DWORD و HANDLE فقط إن لم تكن موجودة، ونترك BOOL و WCHAR لـ wlinux.h
#ifndef DWORD
typedef uint32_t DWORD;
#endif

#ifndef HANDLE
typedef void* HANDLE;
#endif

struct WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    uint32_t ftCreationTime[2];
    uint32_t ftLastAccessTime[2];
    uint32_t ftLastWriteTime[2];
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    unsigned short cFileName[MAX_PATH];
    unsigned short cAlternateFileName[14];
};

typedef WIN32_FIND_DATAW* PWIN32_FIND_DATAW;
typedef WIN32_FIND_DATAW* LPWIN32_FIND_DATAW;

// دوال المحاكاة
inline int DeleteFileW(const wchar_t*) { return 1; }
inline int DeleteFileW(const char*) { return 1; }

inline int CreateDirectoryW(const wchar_t*, void*) { return 1; }
inline int CreateDirectoryW(const char*, void*) { return 1; }

inline DWORD GetFileAttributesW(const wchar_t*) { return 0xFFFFFFFF; }
inline DWORD GetFileAttributesW(const char*) { return 0xFFFFFFFF; }

inline int MoveFileW(const wchar_t*, const wchar_t*) { return 1; }
inline int MoveFileW(const char*, const char*) { return 1; }

inline void* FindFirstFileW(const wchar_t*, LPWIN32_FIND_DATAW) { return (void*)-1; }
inline void* FindFirstFileW(const char*, LPWIN32_FIND_DATAW) { return (void*)-1; }

inline int FindNextFileW(void*, LPWIN32_FIND_DATAW) { return 0; }
inline int FindClose(void*) { return 1; }

// ميكروهات للتحويل التلقائي
#define DeleteFile DeleteFileW
#define CreateDirectory CreateDirectoryW
#define GetFileAttributes GetFileAttributesW
#define MoveFile MoveFileW
#define FindFirstFile FindFirstFileW
#define FindNextFile FindNextFileW

#endif // MOCK_WINDOWS_H
