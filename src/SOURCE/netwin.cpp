// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x000a6be0; preferred Buka symbol ?is_netbios_avail@@YIHXZ
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.601182;margin=0.610067;shape=0.521;size=0.881;calls=1.000;alternate=pol20:int is_netbios_avail(void)@0x000a6be0
VA(0x00413c40, 0xa8)
int is_netbios_avail(void) { return 0; }

// donor PoL RVA 0x000a6c88; preferred Buka symbol _nb_init
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.569810;margin=0.557141;shape=0.568;size=0.810;calls=0.714;alternate=pol20:@nb_init@8@0x000a6c88
VA(0x00413ce8, 0x1b2)
H1_C_LINKAGE int __fastcall nb_init(int, int) { return 0; }

// donor PoL RVA 0x000a7186; preferred Buka symbol _nb_snd
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.424579;margin=0.396789;shape=0.244;size=0.738;calls=0.875;alternate=pol20:@nb_snd@12@0x000a7186
VA(0x0041411a, 0x104)
H1_C_LINKAGE int __fastcall nb_snd(int, int, int) { return 0; }

// donor PoL RVA 0x000a726a; preferred Buka symbol _nb_sess
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.596373;margin=0.181597;shape=0.465;size=0.963;calls=1.000;alternate=pol20:_nb_sess@0x000a726a
VA(0x0041421e, 0x4f6)
H1_C_LINKAGE int __cdecl nb_sess(void) { return 0; }
