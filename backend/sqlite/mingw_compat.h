#ifndef MINGW_COMPAT_H
#define MINGW_COMPAT_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
WINBASEAPI ULONGLONG WINAPI GetTickCount64(VOID);
#endif

#ifdef __cplusplus
}
#endif

#endif
