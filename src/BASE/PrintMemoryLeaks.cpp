// int3-delimited single-function TU between Iconm2bClip and BASEMGR; KB's
// ShutDown calls it. Buka keeps a debug-heap report here; HoMM1 retail ships
// the empty release body.

#include <match.h>

VA(0x00473d80, 0x1)
void PrintMemoryLeaks(void) {}
