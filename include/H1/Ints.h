#ifndef HOMM1_H1_INTS_H
#define HOMM1_H1_INTS_H

#ifndef HOMM1_INTS_DEFINED
#define HOMM1_INTS_DEFINED
#if defined(_MSC_VER) && _MSC_VER < 1600
// Visual C++ 6 has no <stdint.h>; its own types have the same widths.
typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef __int64 i64;
typedef unsigned __int64 u64;
#else
#include <stdint.h>
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
#endif
#endif

#ifndef HOMM1_BOOL_DEFINED
#define HOMM1_BOOL_DEFINED
typedef i8 b8;
typedef i32 b32;
typedef char bchar;
#endif

#include <string>

#endif
