// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x000a26a0; preferred Buka symbol ?SeedPosition@searchArray@@QAEXHHHHHHHHHHHH@Z
// donor Buka TU SOURCE/SEARCH; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418459;margin=0.401354;shape=0.193;size=0.813;calls=0.875;alternate=pol20:void searchArray::SeedPosition(int, int, int, int, int, int, int, int, int, int, int, int)@0x000a26a0
VA(0x00402be0, 0xa60)
void searchArray::SeedPosition(int, int, int, int, int, int, int, int, int, int, int, int) {}

// donor PoL RVA 0x0003532b; preferred Buka symbol ?CreatePrimary@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.466160;margin=0.260025;shape=0.308;size=0.761;calls=1.000;alternate=pol20:void CreatePrimary(void)@0x0003532b
VA(0x004036df, 0x9b)
void CreatePrimary(void) {}

// donor PoL RVA 0x000353bf; preferred Buka symbol ?SetupClipper@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462387;margin=0.608484;shape=0.302;size=0.757;calls=1.000;alternate=pol20:void SetupClipper(void)@0x000353bf
VA(0x0040377a, 0xeb)
void SetupClipper(void) {}

// donor PoL RVA 0x000354a2; preferred Buka symbol ?DDInitGraphics@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.478180;margin=0.785524;shape=0.323;size=0.780;calls=1.000;alternate=pol20:void DDInitGraphics(void)@0x000354a2
VA(0x00403865, 0x171)
void DDInitGraphics(void) {}

// donor PoL RVA 0x00035601; preferred Buka symbol ?DDAppPaint@@YIHPAX0@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.713701;margin=0.241032;shape=0.479;size=0.871;calls=0.917;strings=ResetDisplayMode;alternate=pol20:int DDAppPaint(void *, void *)@0x00035601
VA(0x004039d6, 0x592)
int DDAppPaint(void *, void *) { return 0; }

// donor PoL RVA 0x00035d1c; preferred Buka symbol ?DDCreateSurface@@YIPAUIDirectDrawSurface@@KKH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.607502;margin=0.616633;shape=0.536;size=0.889;calls=1.000;alternate=pol20:struct IDirectDrawSurface * DDCreateSurface(unsigned long int, unsigned long int, int)@0x00035d1c
VA(0x0040415b, 0x12a)
struct IDirectDrawSurface * DDCreateSurface(unsigned long int, unsigned long int, int) { return 0; }

// donor PoL RVA 0x00035e4f; preferred Buka symbol ?DDSD@@YIXHPADH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.296609;margin=0.202074;shape=0.224;size=0.647;calls=0.194;alternate=pol20:void DDSD(int, char *, int)@0x00035e4f
VA(0x00404285, 0x3ee)
void DDSD(int, char *, int) {}

// donor PoL RVA 0x00036421; preferred Buka symbol ?DDUpdatePalette@@YAXPAC@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.482125;margin=0.532523;shape=0.296;size=0.838;calls=1.000;alternate=pol20:void DDUpdatePalette(signed char *)@0x00036421
VA(0x00404673, 0x11c)
void DDUpdatePalette(signed char *) {}

// donor PoL RVA 0x00036539; preferred Buka symbol ?DDCleanUpWinGraphics@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.511358;margin=0.374406;shape=0.341;size=0.862;calls=1.000;alternate=pol20:void DDCleanUpWinGraphics(void)@0x00036539
VA(0x0040478f, 0x17f)
void DDCleanUpWinGraphics(void) {}

// donor PoL RVA 0x000366b0; preferred Buka symbol ?DDSetFullScreenStatus@@YIXH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.474854;margin=0.471351;shape=0.269;size=0.849;calls=1.000;alternate=pol20:void DDSetFullScreenStatus(int)@0x000366b0
VA(0x0040490e, 0x2ea)
void DDSetFullScreenStatus(int) {}

// donor PoL RVA 0x0003728a; preferred Buka symbol ?GetGraphicsInfo@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.553360;margin=0.492664;shape=0.455;size=0.829;calls=1.000;alternate=pol20:void GetGraphicsInfo(void)@0x0003728a
VA(0x004054ab, 0x81)
void GetGraphicsInfo(void) {}

// donor PoL RVA 0x00037483; preferred Buka symbol ?SetFullScreenStatus@@YIXH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.430220;margin=0.650390;shape=0.175;size=0.870;calls=0.800;alternate=pol20:void SetFullScreenStatus(int)@0x00037483
VA(0x0040566a, 0xb9)
void SetFullScreenStatus(int) {}

// donor PoL RVA 0x00037595; preferred Buka symbol ?SetGraphicsType@@YIHH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.565182;margin=0.258783;shape=0.434;size=0.909;calls=1.000;alternate=pol20:int SetGraphicsType(int)@0x00037595
VA(0x00405754, 0x1fc)
int SetGraphicsType(int) { return 0; }
