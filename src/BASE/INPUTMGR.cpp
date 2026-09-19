// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x000cde60; preferred Buka symbol ?MouseMessageHandler@@YIHPAXIIJ@Z
// donor Buka TU BASE/INPUTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.528083;margin=0.800438;shape=0.151;size=0.712;calls=0.800;strings=ReleaseCapture Failed;alternate=pol20:int MouseMessageHandler(void *, unsigned int, unsigned int, long int)@0x000cde60
VA(0x0047be30, 0x280)
int MouseMessageHandler(void *, unsigned int, unsigned int, long int) { return 0; }
