#ifndef HOMM1_MATCH_H
#define HOMM1_MATCH_H

// Fixed-width integer aliases (H1/Ints.h). Every translation unit opens this
// header first, so defining them here makes them reachable everywhere without
// opening another header.
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
// of <string> (docs/patterns/vc6-ctype-startup.md).
#include <string>

// Reconstruction metadata. The compiler receives ordinary C++.
#ifdef __clang__
#define VA(address, size) __attribute__((annotate("va:" #address " size:" #size), used))
#define VA_DECL(address) __attribute__((annotate("decl-va:" #address)))
#define DATA(address) __attribute__((annotate("data-va:" #address), used))
#else
#define VA(address, size)
#define VA_DECL(address)
#define DATA(address)
#endif

// Generated code has explicit retail identity and an owning source VA.
// The compiler itself supplies the body; these are not C++ implementations.
#define VA_COMPGEN(address, size, symbol, owner)
// A file-scope object's compiler-emitted dynamic initializer (_$E<n>): its
// retail VA and size, pinned to the owning datum.
#define RVA_DYNINIT(address, size, owner)

#endif
