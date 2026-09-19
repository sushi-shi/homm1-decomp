// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x000d1594; preferred Buka symbol _BitTest
// donor Buka TU BASE/BITS; HoMM1 owner inferred from contiguous order
// evidence: unique-normalized;alternate=pol20:_BitTest@0x000d1594
VA(0x0047bac8, 0x2e)
H2_C_LINKAGE int __cdecl BitTest(void *, int) { return 0; }

// donor PoL RVA 0x000d15c2; preferred Buka symbol _BitSet
// donor Buka TU BASE/BITS; HoMM1 owner inferred from contiguous order
// evidence: unique-normalized;alternate=pol20:_BitSet@0x000d15c2
VA(0x0047baf6, 0x20)
H2_C_LINKAGE void __cdecl BitSet(void *, int) {}
