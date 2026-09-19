// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x0000e198; preferred Buka symbol ?GetCursorBaseFrame@advManager@@QAEHH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.375377;margin=0.466673;shape=0.186;size=0.574;calls=1.000;alternate=pol20:int advManager::GetCursorBaseFrame(int)@0x0000e198
VA(0x004061ed, 0x88)
int advManager::GetCursorBaseFrame(int) { return 0; }

// donor PoL RVA 0x0000e21d; preferred Buka symbol ?TurnTo@advManager@@QAEXH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.461800;margin=0.347232;shape=0.223;size=0.872;calls=1.000;alternate=pol20:void advManager::TurnTo(int)@0x0000e21d
VA(0x00406275, 0x261)
void advManager::TurnTo(int) {}

// donor PoL RVA 0x0000e51f; preferred Buka symbol ?MoveHero@advManager@@QAEPAVmapCell@@HHPAH00H0H@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.461867;margin=0.353958;shape=0.281;size=0.831;calls=0.879;alternate=pol20:class mapCell * advManager::MoveHero(int, int, int *, int *, int *, int, int *, int)@0x0000e51f
VA(0x0040660c, 0xe1e)
class mapCell * advManager::MoveHero(int, int, int *, int *, int *, int, int *, int) { return 0; }

// donor PoL RVA 0x0000f753; preferred Buka symbol ?CheckAdjacentMon@advManager@@QAEXPAH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.530646;margin=0.751795;shape=0.360;size=0.888;calls=1.000;alternate=pol20:void advManager::CheckAdjacentMon(int *)@0x0000f753
VA(0x0040742a, 0x181)
void advManager::CheckAdjacentMon(int *) {}

// donor PoL RVA 0x00013900; preferred Buka symbol ??0townObject@@QAE@HHPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.615649;margin=0.314990;shape=0.431;size=0.799;calls=0.333;strings=%s.icn;alternate=pol20:void townObject::constructor(int, int, char *)@0x00013900
VA(0x00407d90, 0x1f1)
townObject::townObject(int, int, char *) {}

// donor PoL RVA 0x00013a6a; preferred Buka symbol ??1townObject@@QAE@XZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.564007;margin=0.293257;shape=0.438;size=0.896;calls=1.000;alternate=pol20:void townObject::~destructor(void)@0x00013a6a
VA(0x00407f81, 0x60)
townObject::~townObject() {}

// donor PoL RVA 0x0001436f; preferred Buka symbol ?SetupTown@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.644440;margin=0.052173;shape=0.362;size=0.803;calls=0.887;strings=%s%s|port%04d.icn|strip.icn;alternate=pol20:void townManager::SetupTown(void)@0x0001436f
VA(0x0040816c, 0x7ec)
void townManager::SetupTown(void) {}

// donor PoL RVA 0x00014cc9; preferred Buka symbol ?UnloadTown@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.470224;margin=0.176996;shape=0.284;size=0.924;calls=0.667;alternate=pol20:void townManager::UnloadTown(void)@0x00014cc9
VA(0x00408958, 0x1c4)
void townManager::UnloadTown(void) {}

// donor PoL RVA 0x000158e0; preferred Buka symbol ?ShowText@townManager@@QAEXPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.613333;margin=0.109874;shape=0.519;size=1.000;calls=1.000;alternate=pol20:void townManager::ShowText(char *)@0x000158e0
VA(0x0040933a, 0x74)
void townManager::ShowText(char *) {}

// donor PoL RVA 0x0001595d; preferred Buka symbol ?Main@townManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.513026;margin=0.490356;shape=0.272;size=0.823;calls=0.467;strings=caslwind.bin|magewind.bin|thiefwin.bin;alternate=pol20:int townManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x0001595d
VA(0x004093ae, 0x131f)
int townManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x0001718d; preferred Buka symbol ?DoCommand@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.443170;margin=0.301918;shape=0.326;size=0.721;calls=0.800;alternate=pol20:void townManager::DoCommand(int)@0x0001718d
VA(0x0040a6cd, 0x65f)
void townManager::DoCommand(int) {}

// donor PoL RVA 0x0001771d; preferred Buka symbol ?SplitArmy@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.732616;margin=0.021648;shape=0.430;size=0.991;calls=1.000;strings=splitwin.bin;alternate=pol20:void townManager::SplitArmy(void)@0x0001771d
VA(0x0040add1, 0x37e)
void townManager::SplitArmy(void) {}

// donor PoL RVA 0x00017ab2; preferred Buka symbol ?ResetStrips@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.515580;margin=0.398432;shape=0.375;size=0.860;calls=1.000;alternate=pol20:void townManager::ResetStrips(void)@0x00017ab2
VA(0x0040b21d, 0xab)
void townManager::ResetStrips(void) {}

// donor PoL RVA 0x00017c9d; preferred Buka symbol ?BuyBuild@townManager@@QAEHHHH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.668468;margin=0.054253;shape=0.329;size=0.894;calls=0.897;strings=bigfont.fnt|buybuil%d.bin|resource.icn;alternate=pol20:int townManager::BuyBuild(int, int, int)@0x00017c9d
VA(0x0040b455, 0x1023)
int townManager::BuyBuild(int, int, int) { return 0; }

// donor PoL RVA 0x00018bd2; preferred Buka symbol ?BuildObj@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.630156;margin=0.342681;shape=0.206;size=0.999;calls=0.933;strings=buildtwn.82M;alternate=pol20:void townManager::BuildObj(int)@0x00018bd2
VA(0x0040c478, 0x3a0)
void townManager::BuildObj(int) {}

// donor PoL RVA 0x0001d040; preferred Buka symbol ?SetupCastle@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/Castle; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.228280;margin=0.655471;shape=0.234;size=0.312;calls=0.264;alternate=pol20:void townManager::SetupCastle(class heroWindow *, int)@0x0001d040
VA(0x0040c818, 0x4b5)
void townManager::SetupCastle(class heroWindow *, int) {}

// donor PoL RVA 0x00018fbb; preferred Buka symbol ?SetupMage@townManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.426387;margin=0.115621;shape=0.211;size=0.919;calls=0.625;alternate=pol20:void townManager::SetupMage(class heroWindow *)@0x00018fbb
VA(0x0040cf1a, 0x331)
void townManager::SetupMage(class heroWindow *) {}

// donor PoL RVA 0x0001a783; preferred Buka symbol ?SetupThievesGuild@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.350822;margin=0.362412;shape=0.263;size=0.210;calls=0.163;strings=townwind.icn;alternate=pol20:void townManager::SetupThievesGuild(class heroWindow *, int)@0x0001a783
VA(0x0040d3d1, 0x2ec)
void townManager::SetupThievesGuild(class heroWindow *, int) {}

// donor PoL RVA 0x0001b692; preferred Buka symbol ?GetCategoryStats@@YIXHQAJQAC@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.459458;margin=0.479043;shape=0.193;size=0.976;calls=0.800;alternate=pol20:void GetCategoryStats(int, long int * const, signed char * const)@0x0001b692
VA(0x0040d6bd, 0x484)
void GetCategoryStats(int, long int * const, signed char * const) {}

// donor PoL RVA 0x00019523; preferred Buka symbol ?RecruitHero@townManager@@QAEHHH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.555715;margin=0.218228;shape=0.276;size=0.741;calls=0.578;strings=port%04d.icn|rcrthero.bin;alternate=pol20:int townManager::RecruitHero(int, int)@0x00019523
VA(0x0040dc5a, 0x981)
int townManager::RecruitHero(int, int) { return 0; }

// donor PoL RVA 0x00019c29; preferred Buka symbol ?TavernHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.461090;margin=0.267847;shape=0.244;size=0.845;calls=1.000;alternate=pol20:int TavernHandler(struct tag_message &)@0x00019c29
VA(0x0040e5db, 0x155)
int TavernHandler(struct tag_message &) { return 0; }

// donor PoL RVA 0x00019d7c; preferred Buka symbol ?DoTavern@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.728216;margin=0.164549;shape=0.467;size=0.965;calls=0.889;strings=tavwin.bin;alternate=pol20:void townManager::DoTavern(void)@0x00019d7c
VA(0x0040e730, 0x136)
void townManager::DoTavern(void) {}

// donor PoL RVA 0x0001e0fb; preferred Buka symbol ?CastleHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/Castle; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.424517;margin=0.478912;shape=0.381;size=0.626;calls=0.675;alternate=pol20:int CastleHandler(struct tag_message &)@0x0001e0fb
VA(0x0040e866, 0x726)
int CastleHandler(struct tag_message &) { return 0; }
