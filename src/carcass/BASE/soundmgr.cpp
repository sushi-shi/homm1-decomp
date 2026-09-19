// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x000cd320; preferred Buka symbol ?PollSound@soundManager@@QAEXXZ
// donor Buka TU BASE/soundmgr; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void soundManager::PollSound(void)@0x000cd320
VA(0x00478940, 0x490)
void soundManager::PollSound(void) {}
