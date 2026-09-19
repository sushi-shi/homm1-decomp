#ifndef HOMM1_MATCH_H
#define HOMM1_MATCH_H

// Reconstruction metadata. The compiler receives ordinary C++.
#ifdef __clang__
#define RVA(address, size) __attribute__((annotate("rva:" #address " size:" #size), used))
#define DATA(address) __attribute__((annotate("data:" #address), used))
#else
#define RVA(address, size)
#define DATA(address)
#endif

// Generated code has explicit retail identity and an owning source RVA.
// The compiler itself supplies the body; these are not C++ implementations.
#define RVA_COMPGEN(address, size, symbol, owner)

#endif
