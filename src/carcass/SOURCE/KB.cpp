// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/KB.h>
#include <H2/_all.h>

// HoMM2 KB.cpp confirms the identity and behavior. HoMM1 differs in the timer
// comparison and placement of the re-entry guard.
extern "C" VA(0x0044f640, 0x72)
void PollSound()
{
    if (KBTickCount() < gNextSoundPollTick)
        return;
    if (gbInPollSound)
        return;
    gbInPollSound = 1;
    gNextSoundPollTick = KBTickCount() + 30;
    if (gbForegroundApp)
        gpSoundManager->PollSound();
    PollRemote();
    gbInPollSound = 0;
}

VA(0x0044f6b2, 0x20)
void ForcePollSound()
{
    gNextSoundPollTick = KBTickCount() - 1;
    PollSound();
}

// donor PoL RVA 0x000965be; preferred Buka symbol ?InitMainClasses@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.512387;margin=0.755802;shape=0.400;size=0.925;calls=0.653;alternate=pol20:void InitMainClasses(void)@0x000965be
VA(0x0044f6d2, 0x607)
void InitMainClasses(void) {}

// donor PoL RVA 0x00096e21; preferred Buka symbol ?EarlySetup@@YIHXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.257149;margin=0.511941;shape=0.213;size=0.338;calls=0.600;alternate=pol20:int EarlySetup(void)@0x00096e21
VA(0x00450046, 0x116)
int EarlySetup(void)
{ return 0; }

// donor PoL RVA 0x0009992c; preferred Buka symbol ?GetMonsterCost@@YIXHQAH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.424205;margin=0.383727;shape=0.192;size=0.855;calls=1.000;alternate=pol20:void GetMonsterCost(int, int * const)@0x0009992c
VA(0x004516d9, 0xe6)
void GetMonsterCost(int monster, int *const cost)
{}

// @early-stop
// tu-cumulative: logic + all frame slots byte-exact (reqMask@-8, haveMask@-4 match
// retail); the only residual is 2 bytes — the commutative `&` operand load-order in
// `(reqMask & haveMask) == reqMask` (retail loads reqMask first, this cl loads the
// just-OR'd haveMask first). Not source-steerable (tried both `&` orders, `==` swap).

// donor PoL RVA 0x00099a6c; preferred Buka symbol ?CanBuild@@YIHPAVtown@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.375672;margin=0.371383;shape=0.277;size=0.517;calls=1.000;alternate=pol20:int CanBuild(class town *, int)@0x00099a6c
VA(0x004517bf, 0x144)
int CanBuild(town *t, int building)
{ return 0; }

// donor PoL RVA 0x00099d21; preferred Buka symbol ?CanBuy@@YIHPAVtown@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.384626;margin=0.370647;shape=0.216;size=0.621;calls=1.000;alternate=pol20:int CanBuy(class town *, int)@0x00099d21
VA(0x00451903, 0xce)
int CanBuy(town *t, int type)
{ return 0; }

// donor PoL RVA 0x000a2565; preferred Buka symbol ?UpdateNormalDialog@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.563703;margin=0.348381;shape=0.417;size=0.972;calls=1.000;alternate=pol20:void UpdateNormalDialog(char *)@0x000a2565
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x00452934, 0x6b)
void UpdateNormalDialog(char *text)
{}

// donor PoL RVA 0x00099e81; preferred Buka symbol ?WaitHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.445743;margin=0.444520;shape=0.204;size=0.951;calls=0.688;alternate=pol20:int WaitHandler(struct tag_message &)@0x00099e81
VA(0x0045299f, 0x1c5)
int WaitHandler(tag_message &msg)
{ return 0; }

// donor PoL RVA 0x0009a52f; preferred Buka symbol ?PlayerDead@@YIXH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488269;margin=0.466685;shape=0.274;size=0.981;calls=0.750;alternate=pol20:void PlayerDead(int)@0x0009a52f
VA(0x00452c94, 0x16c)
void PlayerDead(int player)
{}

// donor PoL RVA 0x000a07e3; preferred Buka symbol ?ReceiveRemotePlayerExit@@YIXUSPlayerExit@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.368727;margin=0.249960;shape=0.192;size=0.687;calls=0.800;alternate=pol20:void ReceiveRemotePlayerExit(struct SPlayerExit)@0x000a07e3
VA(0x00452f8a, 0x1ea)
void ReceiveRemotePlayerExit(struct SPlayerExit) {}

// donor PoL RVA 0x0009a6c1; preferred Buka symbol ?CheckEndGame@@YIXHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.237398;margin=0.276870;shape=0.229;size=0.353;calls=0.309;alternate=pol20:void CheckEndGame(int, int)@0x0009a6c1
VA(0x00453174, 0x7d4)
void CheckEndGame(int, int) {}

// donor PoL RVA 0x0009c07c; preferred Buka symbol ?QuickViewWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.435968;margin=0.219505;shape=0.250;size=0.859;calls=0.600;alternate=pol20:void QuickViewWait(void)@0x0009c07c
VA(0x00453948, 0x95)
void QuickViewWait(void)
{}

// donor PoL RVA 0x0009c111; preferred Buka symbol ?InitVars@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.679533;margin=0.555045;shape=0.387;size=0.991;calls=0.692;strings=mnuAdv|mnuCmbt|mnuDflt;alternate=pol20:void InitVars(void)@0x0009c111
VA(0x004539dd, 0x1cb)
void InitVars(void)
{}

// donor PoL RVA 0x0009c312; preferred Buka symbol ?ShowMoraleInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.469331;margin=0.613523;shape=0.400;size=0.774;calls=0.649;alternate=pol20:void game::ShowMoraleInfo(class hero *, int)@0x0009c312
VA(0x00453ba8, 0x450)
void game::ShowMoraleInfo(class hero *, int) {}

// donor PoL RVA 0x0009c92d; preferred Buka symbol ?ShowLuckInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.456267;margin=0.157936;shape=0.493;size=0.606;calls=0.556;alternate=pol20:void game::ShowLuckInfo(class hero *, int)@0x0009c92d
VA(0x00453ff8, 0x1f7)
void game::ShowLuckInfo(class hero *, int) {}

// donor PoL RVA 0x0009ce14; preferred Buka symbol ?AddScoreToHighScore@@YIHHHHHPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701795;margin=0.122445;shape=0.377;size=0.950;calls=0.929;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:int AddScoreToHighScore(int, int, int, int, char *)@0x0009ce14
VA(0x004542ed, 0x3d2)
int AddScoreToHighScore(int, int, int, int, char *) { return 0; }

// donor PoL RVA 0x0009d2c0; preferred Buka symbol ?BVResMsg@@YIXPADHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.598508;margin=0.532475;shape=0.481;size=0.968;calls=1.000;alternate=pol20:void BVResMsg(char *, int, int)@0x0009d2c0
VA(0x004546bf, 0x5b)
void BVResMsg(char *s, int res, int qty)
{}

// donor PoL RVA 0x0009d3a7; preferred Buka symbol ?WaitForOtherPlayer@@YIHXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.581419;margin=0.608732;shape=0.409;size=0.995;calls=1.000;alternate=pol20:int WaitForOtherPlayer(void)@0x0009d3a7
VA(0x00454781, 0xda)
int WaitForOtherPlayer(void)
{ return 0; }

// donor PoL RVA 0x0009d4a6; preferred Buka symbol ?PopNetBox@@YIXPADH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.593152;margin=0.055238;shape=0.393;size=0.624;calls=0.688;strings=netbox.bin;alternate=pol20:void PopNetBox(char *, int)@0x0009d4a6
VA(0x0045485b, 0x6f4)
void PopNetBox(char *, int) {}

// donor PoL RVA 0x0009e0f2; preferred Buka symbol ?ShutDown@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.466886;margin=0.632520;shape=0.403;size=0.708;calls=0.667;alternate=pol20:void ShutDown(char *)@0x0009e0f2
VA(0x00454f8a, 0x14f)
void ShutDown(char *msg)
{}

// donor PoL RVA 0x0009e306; preferred Buka symbol ?FileError@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.316461;margin=0.125092;shape=0.216;size=0.484;calls=0.500;alternate=pol20:void FileError(char *)@0x0009e306
VA(0x004550d9, 0x4a)
void FileError(char *filename)
{}

// @early-stop
// tu-cumulative: logic + all 14 frame slots byte-exact (od_oracle-verified). The only
// residual (coffcmp: 40 bytes, all in the two brightness averages + the minDist test)
// is a /Od operand-evaluation-order difference this cl renders vs retail: the 3-term
// sum `p[2]+p[0]+p[1]` reads +2,+1,+0 here but +2,+0,+1 in retail, and the `d>p`
// compare loads the other operand first. Not source-steerable (probed every term
// ordering, explicit grouping, `|0`, and an inline helper — all identical here).

// donor PoL RVA 0x00009e89; preferred Buka symbol ?Overview@game@@QAEXXZ
// donor Buka TU SOURCE/Overview; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.482449;margin=0.238001;shape=0.359;size=0.797;calls=0.763;alternate=pol20:void game::Overview(void)@0x00009e89
VA(0x00455123, 0x3be)
void game::Overview(void) {}

// donor PoL RVA 0x0009e900; preferred Buka symbol ?CongratsWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.447463;margin=0.065171;shape=0.300;size=0.684;calls=1.000;alternate=pol20:void CongratsWait(void)@0x0009e900
VA(0x004554e1, 0xb1)
void CongratsWait(void)
{}

// donor PoL RVA 0x0009e999; preferred Buka symbol ?LoadPlaySample@@YIPAVsample@@PAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.524829;margin=1.189024;shape=0.423;size=0.802;calls=1.000;alternate=pol20:struct SAMPLE2 LoadPlaySample(char *)@0x0009e999
VA(0x00455932, 0x51)
SAMPLE2 LoadPlaySample(char *name)
{ return *(SAMPLE2 *)0; }

// donor PoL RVA 0x0009e9ed; preferred Buka symbol ?WaitEndSample@@YIXPAPAVsample@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479563;margin=0.490944;shape=0.207;size=0.957;calls=1.000;alternate=pol20:void WaitEndSample(struct SAMPLE2, int)@0x0009e9ed
VA(0x00455983, 0x8a)
void WaitEndSample(SAMPLE2 s, int waitTime)
{}

// donor PoL RVA 0x0009ea7c; preferred Buka symbol ?MemError@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.499168;margin=0.828160;shape=0.176;size=0.610;calls=1.000;strings=Out of Memory;alternate=pol20:void MemError(void)@0x0009ea7c
VA(0x00455a0d, 0x7b)
void MemError(void)
{}

// donor PoL RVA 0x0009ec05; preferred Buka symbol ?HandleAppSpecificMenuCommands@@YIHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.410709;margin=0.595745;shape=0.257;size=0.699;calls=0.542;alternate=pol20:int HandleAppSpecificMenuCommands(int)@0x0009ec05
VA(0x00455d22, 0x629)
int HandleAppSpecificMenuCommands(int) { return 0; }
