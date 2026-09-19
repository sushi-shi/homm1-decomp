#ifndef HOMM1_MATCH_H
#define HOMM1_MATCH_H

// Reconstruction metadata. The compiler receives ordinary C++.
#ifdef __clang__
#define RVA(address, size) __attribute__((annotate("rva:" #address " size:" #size), used))
#else
#define RVA(address, size)
#endif

// Generated code has explicit retail identity and an owning source RVA.
// The compiler itself supplies the body; these are not C++ implementations.
#define RVA_COMPGEN(address, size, symbol, owner)

#endif
