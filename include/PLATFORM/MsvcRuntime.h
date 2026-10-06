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
// Declared here, before rand and srand are renamed below, so that later
// includes of <stdlib.h> and <cstdlib> see the C library's own names.
#include <stdlib.h>

int stricmp(const char* left, const char* right);
int strcmpi(const char* left, const char* right);
int strnicmp(const char* left, const char* right, size_t count);
char* strrev(char* text);

// The Microsoft C runtime's random numbers: a linear congruential generator
// with 15-bit results (RAND_MAX 0x7fff), seeded with 1. The game's outcomes
// (Random, combat, network battles seeded by both peers) depend on the exact
// sequence, so every native build uses this one instead of its C library's.
// (MinGW links Microsoft's own.)
int MsvcRand(void);
void MsvcSrand(unsigned int seed);
#define rand MsvcRand
#define srand MsvcSrand
#ifdef __cplusplus
// The C++ library's own templates name std::rand.
namespace std {
using ::MsvcRand;
using ::MsvcSrand;
}
#endif

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
