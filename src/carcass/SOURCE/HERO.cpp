// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x000c0790; preferred Buka symbol ?AICheckRetreat@combatManager@@QAEHXZ
// donor Buka TU SOURCE/AI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.550932;margin=0.199965;shape=0.407;size=0.920;calls=0.857;alternate=pol20:int combatManager::AICheckRetreat(void)@0x000c0790
VA(0x00464520, 0x783)
int combatManager::AICheckRetreat(void) { return 0; }

// donor PoL RVA 0x0004b36e; preferred Buka symbol ?FreeResources@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463954;margin=0.140795;shape=0.318;size=0.842;calls=0.750;alternate=pol20:void army::FreeResources(void)@0x0004b36e
VA(0x0046693b, 0xf1)
void army::FreeResources(void) {}

// donor PoL RVA 0x0004c7e5; preferred Buka symbol ?SpecialAttack@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.357822;margin=0.373223;shape=0.266;size=0.572;calls=0.585;alternate=pol20:void army::SpecialAttack(void)@0x0004c7e5
VA(0x00467b97, 0xcca)
void army::SpecialAttack(void) {}

// donor PoL RVA 0x0004e1a1; preferred Buka symbol ?DoAttack@army@@QAEXH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.427643;margin=0.977018;shape=0.217;size=0.817;calls=0.724;alternate=pol20:void army::DoAttack(int)@0x0004e1a1
VA(0x00468ff3, 0x108d)
void army::DoAttack(int) {}

// donor PoL RVA 0x0004f93e; preferred Buka symbol ?CheckLuck@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.723107;margin=0.234495;shape=0.473;size=0.925;calls=0.875;strings=badluck.82m|goodluck.82m;alternate=pol20:void army::CheckLuck(void)@0x0004f93e
VA(0x0046a3dc, 0x249)
void army::CheckLuck(void) {}

// donor PoL RVA 0x0004fbc0; preferred Buka symbol ?DamageEnemy@army@@QAEXPAV1@PAH1HH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.387284;margin=0.228483;shape=0.296;size=0.628;calls=0.714;alternate=pol20:void army::DamageEnemy(class army *, int *, int *, int, int)@0x0004fbc0
VA(0x0046a625, 0x2ae)
void army::DamageEnemy(class army *, int *, int *, int, int) {}

// donor PoL RVA 0x0005012e; preferred Buka symbol ?Damage@army@@QAEHJH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.419408;margin=1.157007;shape=0.216;size=0.693;calls=1.000;alternate=pol20:int army::Damage(long int, int)@0x0005012e
VA(0x0046a8d3, 0x176)
int army::Damage(long int, int) { return 0; }

// donor PoL RVA 0x00052ad9; preferred Buka symbol ?MoveAttack@army@@QAEXHH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.522810;margin=0.525862;shape=0.325;size=0.919;calls=0.929;alternate=pol20:void army::MoveAttack(int, int)@0x00052ad9
VA(0x0046b6e7, 0x3a9)
void army::MoveAttack(int, int) {}

// donor PoL RVA 0x0006c3a0; preferred Buka symbol ??0hero@@QAE@XZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493986;margin=0.210035;shape=0.273;size=0.962;calls=1.000;alternate=pol20:void hero::constructor(void)@0x0006c3a0
VA(0x0046ba90, 0x68)
hero::hero(void) {}

// donor PoL RVA 0x0006c4cd; preferred Buka symbol ?HasArtifact@hero@@QAEHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.403615;margin=0.791427;shape=0.171;size=0.731;calls=1.000;alternate=pol20:int hero::HasArtifact(int)@0x0006c4cd
VA(0x0046bb10, 0x5d)
int hero::HasArtifact(int) { return 0; }

// donor PoL RVA 0x0006c526; preferred Buka symbol ?CalcMobility@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418178;margin=0.237740;shape=0.210;size=0.862;calls=0.714;alternate=pol20:int hero::CalcMobility(void)@0x0006c526
VA(0x0046bb6d, 0x1ed)
int hero::CalcMobility(void) { return 0; }

// donor PoL RVA 0x0006f305; preferred Buka symbol ?RedrawHeroScreen@@YIXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.519414;margin=0.462427;shape=0.417;size=0.819;calls=1.000;alternate=pol20:void RedrawHeroScreen(void)@0x0006f305
VA(0x0046c29a, 0x53)
void RedrawHeroScreen(void) {}

// donor PoL RVA 0x0006f354; preferred Buka symbol ?HeroView@@YIHHHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.391018;margin=1.082891;shape=0.247;size=0.310;calls=0.359;strings=herowind.bin;alternate=pol20:int HeroView(int, int, int)@0x0006f354
VA(0x0046c2ed, 0x6c2)
int HeroView(int, int, int) { return 0; }

// donor PoL RVA 0x0006cab1; preferred Buka symbol ?HeroMessageUpdate@@YIXPAD@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570065;margin=0.065812;shape=0.448;size=0.919;calls=1.000;alternate=pol20:void HeroMessageUpdate(char *)@0x0006cab1
VA(0x0046c9af, 0x7c)
void HeroMessageUpdate(char *) {}

// donor PoL RVA 0x0006cb33; preferred Buka symbol ?HeroScreenUpdate@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.542456;margin=0.317795;shape=0.400;size=0.865;calls=1.000;alternate=pol20:void hero::HeroScreenUpdate(void)@0x0006cb33
VA(0x0046ca2b, 0xab)
void hero::HeroScreenUpdate(void) {}

// donor PoL RVA 0x0006cbdb; preferred Buka symbol ?UpdateArmies@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520009;margin=0.290164;shape=0.295;size=0.980;calls=0.909;alternate=pol20:void hero::UpdateArmies(void)@0x0006cbdb
VA(0x0046cad6, 0x1ba)
void hero::UpdateArmies(void) {}

// donor PoL RVA 0x0006ce8b; preferred Buka symbol ?Dismiss@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462026;margin=0.671820;shape=0.242;size=0.843;calls=1.000;alternate=pol20:int hero::Dismiss(void)@0x0006ce8b
VA(0x0046ce89, 0x59)
int hero::Dismiss(void) { return 0; }

// donor PoL RVA 0x0006cee8; preferred Buka symbol ?Deallocate@hero@@QAEXH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.482098;margin=0.987612;shape=0.300;size=0.883;calls=0.875;alternate=pol20:void hero::Deallocate(int)@0x0006cee8
VA(0x0046cee2, 0x452)
void hero::Deallocate(int) {}

// donor PoL RVA 0x0006d50d; preferred Buka symbol ?GetLevel@hero@@QAEHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411151;margin=0.242527;shape=0.205;size=0.711;calls=1.000;alternate=pol20:int hero::GetLevel(int)@0x0006d50d
VA(0x0046d404, 0xf2)
int hero::GetLevel(int) { return 0; }

// donor PoL RVA 0x0006d83f; preferred Buka symbol ?CheckLevel@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.312130;margin=0.246272;shape=0.276;size=0.445;calls=0.500;alternate=pol20:void hero::CheckLevel(void)@0x0006d83f
VA(0x0046d663, 0x2f4)
void hero::CheckLevel(void) {}

// donor PoL RVA 0x0006e0be; preferred Buka symbol ?UpdateHeroScreenStatusBar@@YIXAAUtag_message@@@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.389885;margin=0.637226;shape=0.282;size=0.714;calls=0.718;alternate=pol20:void UpdateHeroScreenStatusBar(struct tag_message &)@0x0006e0be
VA(0x0046d9ae, 0x52e)
void UpdateHeroScreenStatusBar(struct tag_message &) {}

// donor PoL RVA 0x0006e816; preferred Buka symbol ?HeroHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.360602;margin=0.481730;shape=0.266;size=0.702;calls=0.568;alternate=pol20:int HeroHandler(struct tag_message &)@0x0006e816
VA(0x0046dedc, 0x6d4)
int HeroHandler(struct tag_message &) { return 0; }
