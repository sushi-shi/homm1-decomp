// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x000c9630; preferred Buka symbol ?SetPointer@mouseManager@@QAEXH@Z
// donor Buka TU BASE/MOUSEMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.417606;margin=1.286688;shape=0.189;size=0.849;calls=0.737;alternate=pol20:void mouseManager::SetPointer(int)@0x000c9630
VA(0x00476940, 0x490)
void mouseManager::SetPointer(int) {}

// donor PoL RVA 0x000c9ec0; preferred Buka symbol ?MouseCoords@mouseManager@@QAEXAAH0@Z
// donor Buka TU BASE/MOUSEMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.387847;margin=0.576156;shape=0.081;size=0.906;calls=1.000;alternate=pol20:void mouseManager::MouseCoords(int &, int &)@0x000c9ec0
VA(0x00476e60, 0x60)
void mouseManager::MouseCoords(int &, int &) {}
