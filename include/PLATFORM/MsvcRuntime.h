#ifndef HOMM1_PLATFORM_MSVCRUNTIME_H
#define HOMM1_PLATFORM_MSVCRUNTIME_H

// The Microsoft C runtime extensions the game calls, for the other C runtimes
// the native build links against. The build includes this header in every
// game translation unit; Microsoft compilers have the originals, and so does
// MinGW's C runtime, which is Microsoft's.

#if defined(_WIN32)

#include <stdlib.h>
#include <string.h>

#elif !defined(_MSC_VER)

#include <stddef.h>

int stricmp(const char* left, const char* right);
int strcmpi(const char* left, const char* right);
int strnicmp(const char* left, const char* right, size_t count);
char* strrev(char* text);

#endif

#if !defined(_MSC_VER)
#ifndef __max
#define __max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#endif
