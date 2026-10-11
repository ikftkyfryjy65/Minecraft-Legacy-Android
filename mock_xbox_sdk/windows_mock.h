#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long DWORD;
typedef void* HANDLE;
typedef void* HWND;
typedef int BOOL;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef wchar_t WCHAR;
typedef const WCHAR* LPCWSTR;
typedef void* LPVOID;
typedef const void* LPCVOID;

#define TRUE 1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(long)-1)
#define MAX_PATH 260

#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL    0x00000080

#define GENERIC_READ     0x80000000
#define GENERIC_WRITE    0x40000000

#define CREATE_NEW       1
#define CREATE_ALWAYS    2
#define OPEN_EXISTING    3
#define OPEN_ALWAYS      4
#define TRUNCATE_EXISTING 5

#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000

DWORD GetLastError(void);
unsigned long GetTickCount(void);
BOOL CloseHandle(HANDLE hObject);

HANDLE CreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    void* lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
);

BOOL ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    DWORD* lpNumberOfBytesRead,
    void* lpOverlapped
);

BOOL WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    DWORD* lpNumberOfBytesWritten,
    void* lpOverlapped
);

BOOL DeleteFileA(LPCSTR lpFileName);
BOOL DeleteFileW(LPCWSTR lpFileName);
BOOL CreateDirectoryA(LPCSTR lpPathName, void* lpSecurityAttributes);
BOOL CreateDirectoryW(LPCWSTR lpPathName, void* lpSecurityAttributes);
DWORD GetFileAttributesA(LPCSTR lpFileName);
DWORD GetFileAttributesW(LPCWSTR lpFileName);
BOOL MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
BOOL MoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);

#ifdef UNICODE
#define DeleteFile DeleteFileW
#define CreateDirectory CreateDirectoryW
#define GetFileAttributes GetFileAttributesW
#define MoveFile MoveFileW
#else
#define DeleteFile DeleteFileA
#define CreateDirectory CreateDirectoryA
#define GetFileAttributes GetFileAttributesA
#define MoveFile MoveFileA
#endif

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    unsigned int ftCreationTime[2];
    unsigned int ftLastAccessTime[2];
    unsigned int ftLastWriteTime[2];
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    char cFileName[260];
    char cAlternateFileName[14];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    unsigned int ftCreationTime[2];
    unsigned int ftLastAccessTime[2];
    unsigned int ftLastWriteTime[2];
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    unsigned short cFileName[260];
    unsigned short cAlternateFileName[14];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;

#define WIN32_FIND_DATA WIN32_FIND_DATAW

HANDLE FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
BOOL FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
BOOL FindClose(HANDLE hFindFile);

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD dwFileAttributes;
    unsigned int ftCreationTime[2];
    unsigned int ftLastAccessTime[2];
    unsigned int ftLastWriteTime[2];
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

BOOL GetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, void* lpFileInformation);

#define XMemCpy(dest, src, size) memcpy(dest, src, size)
#define XMemSet(dest, val, size) memset(dest, val, size)

#ifdef __cplusplus
}
#endif
