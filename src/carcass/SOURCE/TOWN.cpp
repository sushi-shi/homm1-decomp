// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x00032c00; preferred Buka symbol ??0town@@QAE@XZ
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.468183;margin=0.431232;shape=0.273;size=0.841;calls=1.000;alternate=pol20:void town::constructor(void)@0x00032c00
VA(0x00463f10, 0x6b)
town::town(void) {}

// donor PoL RVA 0x00032c65; preferred Buka symbol ?HasGarrison@town@@QAEHXZ
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410733;margin=0.365325;shape=0.175;size=0.741;calls=1.000;alternate=pol20:int town::HasGarrison(void)@0x00032c65
VA(0x00463f7b, 0x55)
int town::HasGarrison(void) { return 0; }

// donor PoL RVA 0x00032cb9; preferred Buka symbol ?GiveSpells@town@@QAEXPAVhero@@@Z
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.430933;margin=0.601053;shape=0.167;size=0.987;calls=0.667;alternate=pol20:void town::GiveSpells(class hero *)@0x00032cb9
VA(0x00463fd0, 0xe1)
void town::GiveSpells(class hero *) {}

// donor PoL RVA 0x00032e74; preferred Buka symbol ?View@town@@QAEXH@Z
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.510810;margin=0.878787;shape=0.327;size=0.842;calls=1.000;alternate=pol20:void town::View(int)@0x00032e74
VA(0x0046422d, 0xa5)
void town::View(int) {}
