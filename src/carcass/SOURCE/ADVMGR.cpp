// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x00056350; preferred Buka symbol ??0advManager@@QAE@XZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.538995;margin=0.239070;shape=0.344;size=0.950;calls=1.000;alternate=pol20:void advManager::constructor(void)@0x00056350
VA(0x004252c0, 0x2cc)
advManager::advManager(void) {}

// donor PoL RVA 0x0005665f; preferred Buka symbol ?Open@advManager@@UAEHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.608500;margin=0.280356;shape=0.449;size=0.637;calls=0.611;strings=advManager|adv_wind.bin|advmice.mse;alternate=pol20:int advManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x0005665f
VA(0x004255ab, 0xea6)
int advManager::Open(int) { return 0; }

// donor PoL RVA 0x00057028; preferred Buka symbol ?Close@advManager@@UAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.555933;margin=0.510723;shape=0.387;size=0.995;calls=0.909;alternate=pol20:void advManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x00057028
VA(0x00426451, 0x3bd)
void advManager::Close(void) {}

// donor PoL RVA 0x00057432; preferred Buka symbol ?GetCursorSampleSet@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701218;margin=0.695021;shape=0.378;size=0.921;calls=1.000;strings=wsnd%1d%1d.82M;alternate=pol20:void advManager::GetCursorSampleSet(int)@0x00057432
VA(0x0042680e, 0xc7)
void advManager::GetCursorSampleSet(int) {}

// donor PoL RVA 0x0005751b; preferred Buka symbol ?DoAdvCommand@advManager@@QAEPAVmapCell@@XZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.528946;margin=1.360342;shape=0.339;size=0.921;calls=0.947;alternate=pol20:class mapCell * advManager::DoAdvCommand(void)@0x0005751b
VA(0x004268d5, 0x619)
class mapCell * advManager::DoAdvCommand(void) { return 0; }

// donor PoL RVA 0x00057d6c; preferred Buka symbol ?Main@advManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.507706;margin=0.523825;shape=0.297;size=0.996;calls=0.852;alternate=pol20:int advManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00057d6c
VA(0x00426eee, 0xe10)
int advManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x00058d68; preferred Buka symbol ?ProcessSelect@advManager@@QAEHPAUtag_message@@PAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526004;margin=0.306176;shape=0.329;size=0.923;calls=0.956;alternate=pol20:int advManager::ProcessSelect(struct tag_message *, class mapCell * *)@0x00058d68
VA(0x00427d20, 0xe39)
int advManager::ProcessSelect(struct tag_message *, class mapCell * *) { return 0; }

// donor PoL RVA 0x00059c19; preferred Buka symbol ?ProcessDeSelect@advManager@@QAEHPAUtag_message@@PAHPAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.346709;margin=0.551065;shape=0.313;size=0.541;calls=0.500;alternate=pol20:int advManager::ProcessDeSelect(struct tag_message *, int *, class mapCell * *)@0x00059c19
VA(0x00428b59, 0x1ea)
int advManager::ProcessDeSelect(struct tag_message *, int *, class mapCell * *) { return 0; }

// donor PoL RVA 0x0005a07c; preferred Buka symbol ?ProcessSearch@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.676641;margin=0.529889;shape=0.365;size=0.904;calls=0.935;strings=%s%s|DIGSOUND.82M;alternate=pol20:int advManager::ProcessSearch(int, int)@0x0005a07c
VA(0x00428d43, 0x49b)
int advManager::ProcessSearch(int, int) { return 0; }

// donor PoL RVA 0x0005a644; preferred Buka symbol ?ProcessHover@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.464646;margin=0.430920;shape=0.299;size=0.767;calls=0.971;alternate=pol20:int advManager::ProcessHover(int, int)@0x0005a644
VA(0x004291de, 0xc02)
int advManager::ProcessHover(int, int) { return 0; }

// donor PoL RVA 0x0005b094; preferred Buka symbol ?UpdateScreen@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.492392;margin=0.157827;shape=0.294;size=0.928;calls=0.818;alternate=pol20:void advManager::UpdateScreen(int, int)@0x0005b094
VA(0x00429de0, 0x265)
void advManager::UpdateScreen(int, int) {}

// donor PoL RVA 0x0005b2ae; preferred Buka symbol ?CompleteDraw@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.459172;margin=1.327145;shape=0.409;size=0.723;calls=0.706;alternate=pol20:void advManager::CompleteDraw(int, int, int, int)@0x0005b2ae
VA(0x0042a045, 0x359)
void advManager::CompleteDraw(int, int, int, int) {}

// donor PoL RVA 0x0005bb7c; preferred Buka symbol ?DrawCell@advManager@@QAEXHHHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.286321;margin=0.495913;shape=0.238;size=0.410;calls=0.525;alternate=pol20:void advManager::DrawCell(int, int, int, int, int, int)@0x0005bb7c
VA(0x0042a7e5, 0xee8)
void advManager::DrawCell(int, int, int, int, int, int) {}

// donor PoL RVA 0x0005e0da; preferred Buka symbol ?UpdateRadar@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.227177;margin=0.921091;shape=0.195;size=0.373;calls=0.222;alternate=pol20:void advManager::UpdateRadar(int, int)@0x0005e0da
VA(0x0042b74a, 0x57e)
void advManager::UpdateRadar(int, int) {}

// donor PoL RVA 0x0005f127; preferred Buka symbol ?QuickInfo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.437984;margin=0.160503;shape=0.304;size=0.325;calls=0.543;strings=qwikinfo.bin;alternate=pol20:void advManager::QuickInfo(int, int)@0x0005f127
VA(0x0042bcc8, 0x596)
void advManager::QuickInfo(int, int) {}

// donor PoL RVA 0x00060465; preferred Buka symbol ?UpdateHeroLocator@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.539169;margin=0.099782;shape=0.447;size=0.791;calls=0.917;alternate=pol20:void advManager::UpdateHeroLocator(int, int, int)@0x00060465
VA(0x0042c25e, 0x3c8)
void advManager::UpdateHeroLocator(int, int, int) {}

// donor PoL RVA 0x000607ad; preferred Buka symbol ?UpdateHeroLocators@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520706;margin=0.246406;shape=0.456;size=0.777;calls=0.750;alternate=pol20:void advManager::UpdateHeroLocators(int, int)@0x000607ad
VA(0x0042c626, 0x108)
void advManager::UpdateHeroLocators(int, int) {}

// donor PoL RVA 0x000608af; preferred Buka symbol ?UpdateTownLocators@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.535916;margin=0.510257;shape=0.379;size=0.971;calls=0.800;alternate=pol20:void advManager::UpdateTownLocators(int, int)@0x000608af
VA(0x0042c72e, 0x27f)
void advManager::UpdateTownLocators(int, int) {}

// donor PoL RVA 0x00060b97; preferred Buka symbol ?UpdBottomView@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.587774;margin=0.642167;shape=0.494;size=0.843;calls=1.000;alternate=pol20:void advManager::UpdBottomView(int, int, int)@0x00060b97
VA(0x0042c9ad, 0x19f)
void advManager::UpdBottomView(int, int, int) {}

// donor PoL RVA 0x00060d63; preferred Buka symbol ?ClearBottomView@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.613154;margin=1.071587;shape=0.489;size=0.961;calls=1.000;alternate=pol20:void advManager::ClearBottomView(void)@0x00060d63
VA(0x0042cb4c, 0x132)
void advManager::ClearBottomView(void) {}

// donor PoL RVA 0x00060e95; preferred Buka symbol ?UpdBottomViewEnemyTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.738388;margin=0.356312;shape=0.508;size=0.910;calls=0.857;strings=brcrest.icn|hourglas.icn|stonback.icn;alternate=pol20:int advManager::UpdBottomViewEnemyTurn(void)@0x00060e95
VA(0x0042cc7e, 0x5bf)
int advManager::UpdBottomViewEnemyTurn(void) { return 0; }

// donor PoL RVA 0x000613b0; preferred Buka symbol ?UpdBottomViewNewTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.861364;margin=0.145966;shape=0.778;size=0.910;calls=0.840;strings=%s: %d|%s: %d  %s: %d|bigfont.fnt;alternate=pol20:int advManager::UpdBottomViewNewTurn(void)@0x000613b0
VA(0x0042d23d, 0x3e0)
int advManager::UpdBottomViewNewTurn(void) { return 0; }

// donor PoL RVA 0x00061716; preferred Buka symbol ?UpdBottomViewResMsg@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.790087;margin=0.239399;shape=0.649;size=0.884;calls=0.793;strings=resource.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewResMsg(void)@0x00061716
VA(0x0042d61d, 0x3fa)
int advManager::UpdBottomViewResMsg(void) { return 0; }

// donor PoL RVA 0x00061a75; preferred Buka symbol ?UpdBottomViewKingdom@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.769757;margin=0.069402;shape=0.564;size=0.915;calls=0.850;strings=ressmall.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewKingdom(void)@0x00061a75
VA(0x0042da17, 0x3ce)
int advManager::UpdBottomViewKingdom(void) { return 0; }

// donor PoL RVA 0x00061dd8; preferred Buka symbol ?UpdBottomViewHero@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.635193;margin=0.117414;shape=0.302;size=0.961;calls=0.625;strings=mons32.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewHero(void)@0x00061dd8
VA(0x0042dde5, 0x62c)
int advManager::UpdBottomViewHero(void) { return 0; }

// donor PoL RVA 0x0006235b; preferred Buka symbol ?HeroQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:13;base=0.654225;margin=1.910013;shape=0.309;size=0.949;calls=0.880;strings=mons32.icn|qhero0.bin|qhero1.bin;alternate=pol20:void advManager::HeroQuickView(int, int, int, int)@0x0006235b
VA(0x0042e411, 0xd46)
void advManager::HeroQuickView(int, int, int, int) {}

// donor PoL RVA 0x0006308d; preferred Buka symbol ?GetArmySizeName@advManager@@QAEPADHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463915;margin=0.489878;shape=0.171;size=0.973;calls=1.000;alternate=pol20:char * advManager::GetArmySizeName(int, int)@0x0006308d
VA(0x0042f157, 0xe2)
char * advManager::GetArmySizeName(int, int) { return 0; }

// donor PoL RVA 0x000631ad; preferred Buka symbol ?TownQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.675336;margin=0.051432;shape=0.330;size=0.961;calls=0.941;strings=mons32.icn|qtown1.bin|smalfont.fnt;alternate=pol20:void advManager::TownQuickView(int, int, int, int)@0x000631ad
VA(0x0042f239, 0xc51)
void advManager::TownQuickView(int, int, int, int) {}

// donor PoL RVA 0x00063dd6; preferred Buka symbol ?RedrawAdvScreen@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.562981;margin=0.429826;shape=0.444;size=0.906;calls=0.909;alternate=pol20:void advManager::RedrawAdvScreen(int, int)@0x00063dd6
VA(0x0042fe8a, 0xe8)
void advManager::RedrawAdvScreen(int, int) {}

// donor PoL RVA 0x00063f3b; preferred Buka symbol ?MobilizeCurrHero@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.511468;margin=0.529744;shape=0.406;size=0.742;calls=1.000;alternate=pol20:void advManager::MobilizeCurrHero(int)@0x00063f3b
VA(0x0042ffb8, 0x59)
void advManager::MobilizeCurrHero(int) {}

// donor PoL RVA 0x00063f95; preferred Buka symbol ?DemobilizeCurrHero@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.450729;margin=0.647673;shape=0.295;size=0.807;calls=0.800;alternate=pol20:void advManager::DemobilizeCurrHero(void)@0x00063f95
VA(0x00430011, 0x199)
void advManager::DemobilizeCurrHero(void) {}

// donor PoL RVA 0x00064101; preferred Buka symbol ?SetTownContext@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.489027;margin=0.082311;shape=0.312;size=0.827;calls=0.923;alternate=pol20:void advManager::SetTownContext(int)@0x00064101
VA(0x004301aa, 0x255)
void advManager::SetTownContext(int) {}

// donor PoL RVA 0x00064318; preferred Buka symbol ?SetHeroContext@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.499995;margin=0.151356;shape=0.325;size=0.844;calls=0.947;alternate=pol20:void advManager::SetHeroContext(int, int)@0x00064318
VA(0x004303ff, 0x3e6)
void advManager::SetHeroContext(int, int) {}

// donor PoL RVA 0x000646aa; preferred Buka symbol ?DoHeroKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.410865;margin=0.380092;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoHeroKnob(void)@0x000646aa
VA(0x004307e5, 0x290)
void advManager::DoHeroKnob(void) {}

// donor PoL RVA 0x000648d9; preferred Buka symbol ?DoTownKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.410865;margin=0.000000;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoTownKnob(void)@0x000648d9
VA(0x00430a75, 0x290)
void advManager::DoTownKnob(void) {}

// donor PoL RVA 0x0006a1dd; preferred Buka symbol ?ViewPuzzle@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634399;margin=0.712778;shape=0.353;size=0.798;calls=0.917;strings=advmice.mse|puzzle.icn|viewpuzl.bin;alternate=pol20:void advManager::ViewPuzzle(void)@0x0006a1dd
VA(0x00430d05, 0x3da)
void advManager::ViewPuzzle(void) {}

// donor PoL RVA 0x00064b08; preferred Buka symbol ?CastSpell@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.339671;margin=1.295843;shape=0.207;size=0.620;calls=0.615;alternate=pol20:void advManager::CastSpell(int)@0x00064b08
VA(0x00431315, 0x1f2)
void advManager::CastSpell(int) {}

// donor PoL RVA 0x00064e9f; preferred Buka symbol ?SaveGame@@YIHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.731974;margin=0.187030;shape=0.477;size=0.957;calls=0.889;strings=.GM%d|.\GAMES\|advmice.mse;alternate=pol20:int SaveGame(void)@0x00064e9f
VA(0x00432bb7, 0x232)
int SaveGame(void) { return 0; }

// donor PoL RVA 0x000650eb; preferred Buka symbol ?CheckCastSpell@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.587490;margin=0.208808;shape=0.255;size=0.813;calls=0.857;strings=advmice.mse;alternate=pol20:void advManager::CheckCastSpell(void)@0x000650eb
VA(0x00433334, 0xab)
void advManager::CheckCastSpell(void) {}

// donor PoL RVA 0x0006a724; preferred Buka symbol ?AdvPanel@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.644374;margin=0.446733;shape=0.427;size=0.786;calls=0.720;strings=advmice.mse|apanel.bin;alternate=pol20:void advManager::AdvPanel(void)@0x0006a724
VA(0x004333df, 0x213)
void advManager::AdvPanel(void) {}

// donor PoL RVA 0x00065191; preferred Buka symbol ?DimensionDoorHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.481967;margin=0.254497;shape=0.313;size=0.765;calls=1.000;alternate=pol20:int DimensionDoorHandler(struct tag_message &)@0x00065191
VA(0x004337c5, 0x34b)
int DimensionDoorHandler(struct tag_message &) { return 0; }

// donor PoL RVA 0x000654ad; preferred Buka symbol ?ComboDraw@advManager@@QAEHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.384237;margin=0.212683;shape=0.299;size=0.586;calls=0.778;alternate=pol20:int advManager::ComboDraw(int, int, int)@0x000654ad
VA(0x00433b10, 0xaf6)
int advManager::ComboDraw(int, int, int) { return 0; }

// donor PoL RVA 0x0006668e; preferred Buka symbol ?SetEnvironmentOrigin@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.577153;margin=0.265756;shape=0.425;size=0.947;calls=1.000;alternate=pol20:void advManager::SetEnvironmentOrigin(int, int, int)@0x0006668e
VA(0x00434640, 0x2dd)
void advManager::SetEnvironmentOrigin(int, int, int) {}

// donor PoL RVA 0x000669c6; preferred Buka symbol ?CheckLoadSample@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.741786;margin=0.490066;shape=0.533;size=0.857;calls=1.000;strings=loop%04d.82M;alternate=pol20:void advManager::CheckLoadSample(int)@0x000669c6
VA(0x0043491d, 0x69)
void advManager::CheckLoadSample(int) {}

// donor PoL RVA 0x00066ef0; preferred Buka symbol ?InsertSound@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.476286;margin=0.526376;shape=0.266;size=0.902;calls=0.750;alternate=pol20:void advManager::InsertSound(int, int, int, int)@0x00066ef0
VA(0x00434986, 0x251)
void advManager::InsertSound(int, int, int, int) {}

// donor PoL RVA 0x0006712a; preferred Buka symbol ?TeleportTo@advManager@@QAEXPAVhero@@HHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.494469;margin=0.364782;shape=0.352;size=0.864;calls=0.864;alternate=pol20:void advManager::TeleportTo(class hero *, int, int, int, int)@0x0006712a
VA(0x00434bd7, 0x340)
void advManager::TeleportTo(class hero *, int, int, int, int) {}

// donor PoL RVA 0x00067539; preferred Buka symbol ?DimensionDoor@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.671113;margin=0.501597;shape=0.372;size=0.883;calls=0.867;strings=dimdoor.bin;alternate=pol20:void advManager::DimensionDoor(void)@0x00067539
VA(0x00434f17, 0x246)
void advManager::DimensionDoor(void) {}

// donor PoL RVA 0x0006785d; preferred Buka symbol ?TownGate@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.385467;margin=0.208066;shape=0.290;size=0.657;calls=0.526;alternate=pol20:void advManager::TownGate(int)@0x0006785d
VA(0x0043515d, 0x2a6)
void advManager::TownGate(int) {}

// donor PoL RVA 0x00067c9b; preferred Buka symbol ?SummonBoat@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.502597;margin=0.051626;shape=0.294;size=0.965;calls=0.867;alternate=pol20:void advManager::SummonBoat(void)@0x00067c9b
VA(0x00435403, 0x51c)
void advManager::SummonBoat(void) {}

// donor PoL RVA 0x00068247; preferred Buka symbol ?ShowRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.438878;margin=0.464254;shape=0.279;size=0.721;calls=1.000;alternate=pol20:void advManager::ShowRoute(int, int, int)@0x00068247
VA(0x0043591f, 0x31b)
void advManager::ShowRoute(int, int, int) {}

// donor PoL RVA 0x00068720; preferred Buka symbol ?HideRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.524051;margin=0.948488;shape=0.403;size=0.809;calls=1.000;alternate=pol20:void advManager::HideRoute(int, int, int)@0x00068720
VA(0x00435c3a, 0x106)
void advManager::HideRoute(int, int, int) {}

// donor PoL RVA 0x00068827; preferred Buka symbol ?CheckDimHero@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.465466;margin=0.233620;shape=0.306;size=0.738;calls=1.000;alternate=pol20:void advManager::CheckDimHero(void)@0x00068827
VA(0x00435d40, 0x91)
void advManager::CheckDimHero(void) {}

// donor PoL RVA 0x000688b4; preferred Buka symbol ?CheckDimNextHeroBut@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.454682;margin=0.437763;shape=0.225;size=0.845;calls=1.000;alternate=pol20:void advManager::CheckDimNextHeroBut(void)@0x000688b4
VA(0x00435dd1, 0x6e)
void advManager::CheckDimNextHeroBut(void) {}

// donor PoL RVA 0x0006891f; preferred Buka symbol ?SeedTo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.530039;margin=0.750383;shape=0.400;size=0.820;calls=1.000;alternate=pol20:void advManager::SeedTo(int, int)@0x0006891f
VA(0x00435e3f, 0x152)
void advManager::SeedTo(int, int) {}

// donor PoL RVA 0x00068ab6; preferred Buka symbol ?ScreenScroll@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463708;margin=0.069995;shape=0.184;size=0.968;calls=1.000;alternate=pol20:void advManager::ScreenScroll(int, int)@0x00068ab6
VA(0x00435fe0, 0x1b6)
void advManager::ScreenScroll(int, int) {}

// donor PoL RVA 0x00068c5c; preferred Buka symbol ?CheckScreenScroll@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.475412;margin=0.607960;shape=0.306;size=0.761;calls=1.000;alternate=pol20:void advManager::CheckScreenScroll(void)@0x00068c5c
VA(0x00436196, 0x1e1)
void advManager::CheckScreenScroll(void) {}

// donor PoL RVA 0x00068e17; preferred Buka symbol ?MouseInScrollZone@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.376012;margin=0.441568;shape=0.180;size=0.620;calls=1.000;alternate=pol20:int advManager::MouseInScrollZone(void)@0x00068e17
VA(0x00436377, 0xa3)
int advManager::MouseInScrollZone(void) { return 0; }

// donor PoL RVA 0x00068ea8; preferred Buka symbol ?SetInitialMapOrigin@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.512925;margin=0.905583;shape=0.330;size=0.958;calls=0.778;alternate=pol20:void advManager::SetInitialMapOrigin(void)@0x00068ea8
VA(0x0043641a, 0x283)
void advManager::SetInitialMapOrigin(void) {}

// donor PoL RVA 0x00069160; preferred Buka symbol ?LoadRemote@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.559158;margin=0.708697;shape=0.333;size=0.638;calls=0.706;strings=advmice.mse;alternate=pol20:void advManager::LoadRemote(void)@0x00069160
VA(0x0043669d, 0x152)
void advManager::LoadRemote(void) {}

// donor PoL RVA 0x0006931e; preferred Buka symbol ?CheckHandleNet@advManager@@QAEPADXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410587;margin=0.478746;shape=0.288;size=0.750;calls=0.692;alternate=pol20:char * advManager::CheckHandleNet(void)@0x0006931e
VA(0x004367ef, 0x178)
char * advManager::CheckHandleNet(void) { return 0; }

// donor PoL RVA 0x0006952a; preferred Buka symbol ?CheckHandleNetPlayerWait@advManager@@QAEHAAUtag_message@@H@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.447497;margin=1.157842;shape=0.264;size=0.731;calls=1.000;alternate=pol20:int advManager::CheckHandleNetPlayerWait(struct tag_message &, int)@0x0006952a
VA(0x00436967, 0xd4)
int advManager::CheckHandleNetPlayerWait(struct tag_message &, int) { return 0; }

// donor PoL RVA 0x000695f7; preferred Buka symbol ?TrimLoopingSounds@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.593881;margin=0.540898;shape=0.483;size=0.978;calls=1.000;alternate=pol20:void advManager::TrimLoopingSounds(int)@0x000695f7
VA(0x00436a3b, 0x1c2)
void advManager::TrimLoopingSounds(int) {}

// donor PoL RVA 0x00069976; preferred Buka symbol ?SaveAdventureBorder@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.567475;margin=0.473800;shape=0.423;size=0.969;calls=1.000;alternate=pol20:void advManager::SaveAdventureBorder(void)@0x00069976
VA(0x00436d9d, 0x138)
void advManager::SaveAdventureBorder(void) {}

// donor PoL RVA 0x00069abb; preferred Buka symbol ?DrawAdventureBorder@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.567997;margin=0.477634;shape=0.397;size=0.978;calls=1.000;alternate=pol20:void advManager::DrawAdventureBorder(void)@0x00069abb
VA(0x00436ed5, 0x13b)
void advManager::DrawAdventureBorder(void) {}
