// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x00054502; preferred Buka symbol ?Open@swapManager@@UAEHH@Z
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.683274;margin=0.416085;shape=0.517;size=0.729;calls=0.800;strings=port%04d.icn|swapManager|swapbtn.icn;alternate=pol20:int swapManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00054502
VA(0x0046edb0, 0x2d5)
int swapManager::Open(int) { return 0; }

// donor PoL RVA 0x000548be; preferred Buka symbol ?Close@swapManager@@UAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.610218;margin=0.602384;shape=0.500;size=0.986;calls=1.000;alternate=pol20:void swapManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x000548be
VA(0x0046f085, 0x123)
void swapManager::Close(void) {}

// donor PoL RVA 0x00054be3; preferred Buka symbol ?Main@swapManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.525982;margin=0.522986;shape=0.320;size=0.991;calls=0.960;alternate=pol20:int swapManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00054be3
VA(0x0046f3c7, 0x9ac)
int swapManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x000556d3; preferred Buka symbol ?ViewMon@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.509716;margin=0.520416;shape=0.289;size=0.921;calls=1.000;alternate=pol20:void swapManager::ViewMon(void)@0x000556d3
VA(0x0046fd73, 0xa5)
void swapManager::ViewMon(void) {}

// donor PoL RVA 0x00055b42; preferred Buka symbol ?Update@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.575258;margin=0.321463;shape=0.441;size=0.938;calls=0.900;alternate=pol20:void swapManager::Update(void)@0x00055b42
VA(0x004701b8, 0x492)
void swapManager::Update(void) {}

// donor PoL RVA 0x00055fbd; preferred Buka symbol ?SplitMons@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.761691;margin=0.047739;shape=0.487;size=0.960;calls=1.000;strings=splitwin.bin;alternate=pol20:void swapManager::SplitMons(void)@0x00055fbd
VA(0x0047064a, 0x3a6)
void swapManager::SplitMons(void) {}
