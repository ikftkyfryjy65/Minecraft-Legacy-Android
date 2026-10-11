#ifndef COMPAT_WIN32_FILE_H
#define COMPAT_WIN32_FILE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(long)-1)
#endif

#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL    0x00000080
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif

#define GetFileExInfoStandard 0

#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND 2
#endif
#ifndef ERROR_PATH_NOT_FOUND
#define ERROR_PATH_NOT_FOUND 3
#endif
#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED 5
#endif
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183
#endif

typedef struct _WIN32_FIND_DATAA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
    DWORD    dwReserved0;
    DWORD    dwReserved1;
    CHAR     cFileName[MAX_PATH];
    CHAR     cAlternateFileName[14];
} WIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;

/* Thread-local last-error slot */
static inline DWORD* _compat_errno_slot(void) {
    static __thread DWORD _e = 0;
    return &_e;
}
static inline DWORD GetLastError(void) { return *_compat_errno_slot(); }
static inline void SetLastError(DWORD e) { *_compat_errno_slot() = e; }

static inline DWORD _compat_winerr_from_errno(int e) {
    switch (e) {
        case ENOENT: return ERROR_FILE_NOT_FOUND;
        case EACCES: case EPERM: return ERROR_ACCESS_DENIED;
        case EEXIST: return ERROR_ALREADY_EXISTS;
        default: return (DWORD)e;
    }
}

static inline DWORD GetFileAttributesA(const char* p) {
    struct stat st;
    if (!p || stat(p, &st) != 0) {
        SetLastError(_compat_winerr_from_errno(errno));
        return INVALID_FILE_ATTRIBUTES;
    }
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

static inline BOOL DeleteFileA(const char* p) {
    if (unlink(p) == 0) return TRUE;
    SetLastError(_compat_winerr_from_errno(errno));
    return FALSE;
}

static inline BOOL CreateDirectoryA(const char* p, void* sa) {
    (void)sa;
    if (mkdir(p, 0755) == 0) return TRUE;
    SetLastError(_compat_winerr_from_errno(errno));
    return FALSE;
}

static inline BOOL MoveFileA(const char* from, const char* to) {
    if (rename(from, to) == 0) return TRUE;
    SetLastError(_compat_winerr_from_errno(errno));
    return FALSE;
}

static inline BOOL CopyFileA(const char* from, const char* to, BOOL failIfExists) {
    if (failIfExists && access(to, F_OK) == 0) {
        SetLastError(ERROR_ALREADY_EXISTS); return FALSE;
    }
    FILE* fi = fopen(from, "rb");
    if (!fi) { SetLastError(_compat_winerr_from_errno(errno)); return FALSE; }
    FILE* fo = fopen(to, "wb");
    if (!fo) { fclose(fi); SetLastError(_compat_winerr_from_errno(errno)); return FALSE; }
    char buf[65536]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fi)) > 0) fwrite(buf, 1, n, fo);
    fclose(fi); fclose(fo);
    return TRUE;
}

typedef struct _compat_find_state { DIR* dir; } _compat_find_state;

static inline HANDLE FindFirstFileA(const char* pattern, WIN32_FIND_DATA* data) {
    if (!pattern || !data) { SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    char dirpath[MAX_PATH];
    const char* s1 = strrchr(pattern, '/');
    const char* s2 = strrchr(pattern, '\\');
    const char* sep = (s1 > s2) ? s1 : s2;
    if (sep) {
        size_t n = (size_t)(sep - pattern);
        if (n == 0) { dirpath[0] = '/'; dirpath[1] = '\0'; }
        else { if (n >= MAX_PATH) n = MAX_PATH - 1; memcpy(dirpath, pattern, n); dirpath[n] = '\0'; }
    } else {
        strcpy(dirpath, ".");
    }
    DIR* d = opendir(dirpath);
    if (!d) { SetLastError(_compat_winerr_from_errno(errno)); return INVALID_HANDLE_VALUE; }
    _compat_find_state* s = (_compat_find_state*)malloc(sizeof(*s));
    s->dir = d;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        strncpy(data->cFileName, e->d_name, MAX_PATH - 1);
        data->cFileName[MAX_PATH - 1] = '\0';
        data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        return (HANDLE)s;
    }
    closedir(d); free(s);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

static inline BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATA* data) {
    _compat_find_state* s = (_compat_find_state*)h;
    if (!s || !data) return FALSE;
    struct dirent* e;
    while ((e = readdir(s->dir)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        strncpy(data->cFileName, e->d_name, MAX_PATH - 1);
        data->cFileName[MAX_PATH - 1] = '\0';
        data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        return TRUE;
    }
    return FALSE;
}

static inline BOOL FindClose(HANDLE h) {
    _compat_find_state* s = (_compat_find_state*)h;
    if (!s) return FALSE;
    if (s->dir) closedir(s->dir);
    free(s);
    return TRUE;
}

static inline BOOL GetFileAttributesExA(const char* p, int lvl, WIN32_FILE_ATTRIBUTE_DATA* data) {
    (void)lvl;
    struct stat st;
    if (stat(p, &st) != 0) { SetLastError(_compat_winerr_from_errno(errno)); return FALSE; }
    data->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    data->nFileSizeLow  = (DWORD)( st.st_size        & 0xFFFFFFFFu);
    data->nFileSizeHigh = (DWORD)((st.st_size >> 32) & 0xFFFFFFFFu);
    memset(&data->ftCreationTime,   0, sizeof(FILETIME));
    memset(&data->ftLastAccessTime, 0, sizeof(FILETIME));
    memset(&data->ftLastWriteTime,  0, sizeof(FILETIME));
    return TRUE;
}

/* No-suffix aliases */
#define DeleteFile            DeleteFileA
#define CreateDirectory       CreateDirectoryA
#define MoveFile              MoveFileA
#define CopyFile              CopyFileA
#define GetFileAttributes     GetFileAttributesA
#define GetFileAttributesEx   GetFileAttributesExA
#define FindFirstFile         FindFirstFileA
#define FindNextFile          FindNextFileA


/* Defensive: undef any pre-existing GetFileSize macro */
#ifdef GetFileSize
#undef GetFileSize
#endif
#ifdef SetFilePointer
#undef SetFilePointer
#endif
#ifdef ReadFile
#undef ReadFile
#endif
#ifdef WriteFile
#undef WriteFile
#endif
#ifdef CreateFile
#undef CreateFile
#endif

/* ============================================================
 * Win32 file I/O (CreateFile / ReadFile / WriteFile / ...)
 * ============================================================ */

#ifndef GENERIC_READ
#define GENERIC_READ    0x80000000UL
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE   0x40000000UL
#endif
#ifndef GENERIC_ALL
#define GENERIC_ALL     0x10000000UL
#endif

#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ   0x00000001
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE  0x00000002
#endif
#ifndef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 0x00000004
#endif

#ifndef CREATE_NEW
#define CREATE_NEW          1
#endif
#ifndef CREATE_ALWAYS
#define CREATE_ALWAYS       2
#endif
#ifndef OPEN_EXISTING
#define OPEN_EXISTING       3
#endif
#ifndef OPEN_ALWAYS
#define OPEN_ALWAYS         4
#endif
#ifndef TRUNCATE_EXISTING
#define TRUNCATE_EXISTING   5
#endif

#ifndef FILE_FLAG_SEQUENTIAL_SCAN
#define FILE_FLAG_SEQUENTIAL_SCAN  0x08000000
#endif
#ifndef FILE_FLAG_RANDOM_ACCESS
#define FILE_FLAG_RANDOM_ACCESS    0x10000000
#endif

#ifndef FILE_BEGIN
#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2
#endif

typedef struct _compat_file_handle {
    int  fd;
    char path[512];
} _compat_file_handle;

static inline HANDLE _compat_alloc_handle(int fd, const char* path) {
    _compat_file_handle* h = (_compat_file_handle*)malloc(sizeof(*h));
    if (!h) return INVALID_HANDLE_VALUE;
    h->fd = fd;
    if (path) { strncpy(h->path, path, 511); h->path[511] = '\0'; }
    else h->path[0] = '\0';
    return (HANDLE)h;
}

static inline HANDLE CreateFileA(const char* path, DWORD access, DWORD share,
                                 void* sa, DWORD disp, DWORD flags, void* tmpl) {
    (void)share; (void)sa; (void)flags; (void)tmpl;
    if (!path) { SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }

    int o = 0;
    int want_read  = (access & GENERIC_READ)  != 0;
    int want_write = (access & GENERIC_WRITE) != 0;
    if (want_read && want_write) o = O_RDWR;
    else if (want_write)         o = O_WRONLY;
    else                         o = O_RDONLY;

    switch (disp) {
        case CREATE_NEW:        o |= O_CREAT | O_EXCL; break;
        case CREATE_ALWAYS:     o |= O_CREAT | O_TRUNC; break;
        case OPEN_EXISTING:                            break;
        case OPEN_ALWAYS:       o |= O_CREAT;          break;
        case TRUNCATE_EXISTING: o |= O_TRUNC;          break;
        default: break;
    }

    int fd = open(path, o, 0644);
    fprintf(stderr, "[CFA] path=%s fd=%d errno=%d\n", path, fd, errno);
    if (fd < 0) {
        SetLastError(_compat_winerr_from_errno(errno));
        return INVALID_HANDLE_VALUE;
    }
    return _compat_alloc_handle(fd, path);
}

static inline BOOL ReadFile(HANDLE h, void* buf, DWORD n, DWORD* bytesRead, void* ov) {
    (void)ov;
    if (h == INVALID_HANDLE_VALUE || !h) return FALSE;
    _compat_file_handle* fh = (_compat_file_handle*)h;
    ssize_t r = ::read(fh->fd, buf, (size_t)n);
    if (r < 0) { SetLastError(_compat_winerr_from_errno(errno)); if (bytesRead) *bytesRead = 0; return FALSE; }
    if (bytesRead) *bytesRead = (DWORD)r;
    return TRUE;
}

static inline BOOL WriteFile(HANDLE h, const void* buf, DWORD n, DWORD* bytesWritten, void* ov) {
    (void)ov;
    if (h == INVALID_HANDLE_VALUE || !h) return FALSE;
    _compat_file_handle* fh = (_compat_file_handle*)h;
    ssize_t w = ::write(fh->fd, buf, (size_t)n);
    if (w < 0) { SetLastError(_compat_winerr_from_errno(errno)); if (bytesWritten) *bytesWritten = 0; return FALSE; }
    if (bytesWritten) *bytesWritten = (DWORD)w;
    return TRUE;
}


static inline DWORD SetFilePointer(HANDLE h, LONG lo, LONG* hi, DWORD method) {
    if (h == INVALID_HANDLE_VALUE || !h) return (DWORD)-1;
    _compat_file_handle* fh = (_compat_file_handle*)h;
    int whence = (method == FILE_BEGIN) ? SEEK_SET
               : (method == FILE_END)   ? SEEK_END
                                        : SEEK_CUR;
    off_t r = lseek(fh->fd, (off_t)lo, whence);
    if (r < 0) { SetLastError(_compat_winerr_from_errno(errno)); return (DWORD)-1; }
    if (hi) *hi = (LONG)((unsigned long long)r >> 32);
    return (DWORD)((unsigned long long)r & 0xFFFFFFFFu);
}

static inline DWORD GetFileSize(HANDLE h, DWORD* hi) {
    if (h == INVALID_HANDLE_VALUE || !h) return (DWORD)-1;
    _compat_file_handle* fh = (_compat_file_handle*)h;
    struct stat st;
    if (fstat(fh->fd, &st) != 0) { SetLastError(_compat_winerr_from_errno(errno)); return (DWORD)-1; }
    if (hi) *hi = (DWORD)((unsigned long long)st.st_size >> 32);
    return (DWORD)((unsigned long long)st.st_size & 0xFFFFFFFFu);
}

static inline BOOL SetEndOfFile(HANDLE h) {
    if (h == INVALID_HANDLE_VALUE || !h) return FALSE;
    _compat_file_handle* fh = (_compat_file_handle*)h;
    off_t pos = lseek(fh->fd, 0, SEEK_CUR);
    if (pos < 0) return FALSE;
    if (ftruncate(fh->fd, pos) != 0) { SetLastError(_compat_winerr_from_errno(errno)); return FALSE; }
    return TRUE;
}

static inline BOOL FlushFileBuffers(HANDLE h) {
    if (h == INVALID_HANDLE_VALUE || !h) return FALSE;
    _compat_file_handle* fh = (_compat_file_handle*)h;
    fsync(fh->fd);
    return TRUE;
}

/* Wide-char overloads — convert wchar_t path to UTF-8 then reuse CreateFileA */
static inline HANDLE CreateFileW(const wchar_t* wpath, DWORD access, DWORD share,
                                 void* sa, DWORD disp, DWORD flags, void* tmpl) {
    if (!wpath) { SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    char path[2048];
    size_t n = wcstombs(path, wpath, sizeof(path)-1);
    if (n == (size_t)-1) { SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    path[n] = 0;
    /* Convert backslashes to forward slashes for POSIX */
    for (size_t i = 0; i < n; i++) if (path[i] == '\\') path[i] = '/';
    return CreateFileA(path, access, share, sa, disp, flags, tmpl);
}

static inline HANDLE CreateFileA(const wchar_t* wpath, DWORD access, DWORD share,
                                 void* sa, DWORD disp, DWORD flags, void* tmpl) {
    return CreateFileW(wpath, access, share, sa, disp, flags, tmpl);
}

/* CreateFile — default to ASCII variant */
#ifndef CreateFile
#define CreateFile CreateFileA
#endif

#endif /* COMPAT_WIN32_FILE_H */
