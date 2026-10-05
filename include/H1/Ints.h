// Ints.h - fixed-width integer aliases.
//
// Widths are this target's: 32-bit Win32, where `long` == `int` == 4 bytes and
// `unsigned long` == `unsigned int` == 4 bytes, so `i32`/`u32` cover both
// `int`/`long` and `unsigned`/`unsigned long`.
//
// NOTE: the SDK's own aliases (BOOL/DWORD/WORD/BYTE/UINT/INT/LONG/...) are left
// as-is in our sources - they pin our externs to the real Win32 signatures.
// These aliases are ONLY for our raw int/long/unsigned/short/signed|unsigned
// char. Plain `char` (text) stays `char`.
#ifndef HOMM1_H1_INTS_H
#define HOMM1_H1_INTS_H

// include/match.h defines the same aliases for every translation unit; this
// header keeps them available to code that does not open match.h.
#ifndef HOMM1_INTS_DEFINED
#define HOMM1_INTS_DEFINED
typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef __int64 i64;
typedef unsigned __int64 u64;
#endif

// Buka's project-wide prelude: every retail C++ unit (sixty /Od and two /O2)
// instantiates std::ctype<wchar_t>::id and registers its cleanup, the output
// of <string>.
#include <string>

#endif // HOMM1_H1_INTS_H
