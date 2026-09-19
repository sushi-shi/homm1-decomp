#ifndef HOMM1_MATCH_H
#define HOMM1_MATCH_H

// Reconstruction metadata. The compiler receives ordinary C++.
#ifdef __clang__
#define VA(address, size) __attribute__((annotate("va:" #address " size:" #size), used))
#define DATA(address) __attribute__((annotate("data-va:" #address), used))
#else
#define VA(address, size)
#define DATA(address)
#endif

// Generated code has explicit retail identity and an owning source VA.
// The compiler itself supplies the body; these are not C++ implementations.
#define VA_COMPGEN(address, size, symbol, owner)

#endif
