#ifndef HOMM1_BASE_BITS_H
#define HOMM1_BASE_BITS_H

// BASE/BITS.asm cdecl bit helpers (HoMM2 Buka BITS.h signatures).
extern "C" int __cdecl BitTest(const void*, unsigned int);
extern "C" void __cdecl BitSet(void*, unsigned int);

#endif
