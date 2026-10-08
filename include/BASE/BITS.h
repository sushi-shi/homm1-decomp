#ifndef HOMM1_BASE_BITS_H
#define HOMM1_BASE_BITS_H

// BASE/BITS.asm cdecl bit helpers.
extern "C" i32 __cdecl BitTest(const void* bits, u32 bit);
extern "C" void __cdecl BitSet(void* bits, u32 bit);

#endif // HOMM1_BASE_BITS_H
