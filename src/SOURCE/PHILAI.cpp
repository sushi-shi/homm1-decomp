// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x000379d0; preferred Buka symbol ?CheckDoMain@@YIXHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.463287;margin=0.627999;shape=0.348;size=0.713;calls=0.909;alternate=pol20:void CheckDoMain(int, int)@0x000379d0
VA(0x00419f16, 0x1ef)
void CheckDoMain(int firstValue, int doMain) {}

// donor PoL RVA 0x00037bb5; preferred Buka symbol ?DoAllHeroInteractions@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.414032;margin=0.418353;shape=0.188;size=0.773;calls=1.000;alternate=pol20:void philAI::DoAllHeroInteractions(void)@0x00037bb5
VA(0x0041a1ff, 0xb0)
void philAI::DoAllHeroInteractions(void) {}

// donor PoL RVA 0x00037fdf; preferred Buka symbol ?CheckBuyStuff@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.641449;margin=0.572995;shape=0.312;size=0.907;calls=0.824;strings=CheckBuy End  |CheckBuy Start;alternate=pol20:void philAI::CheckBuyStuff(void)@0x00037fdf
VA(0x0041a2af, 0x445)
void philAI::CheckBuyStuff(void) {}

// donor PoL RVA 0x0003849d; preferred Buka symbol ?GoodAdjacent@philAI@@QAEHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410367;margin=0.413392;shape=0.366;size=0.596;calls=0.600;alternate=pol20:int philAI::GoodAdjacent(int *)@0x0003849d
VA(0x0041a6f4, 0x1a9)
int philAI::GoodAdjacent(int *) { return 0; }

// donor PoL RVA 0x00038785; preferred Buka symbol ?CheckReload@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.433109;margin=0.377228;shape=0.310;size=0.859;calls=0.417;alternate=pol20:void philAI::CheckReload(void)@0x00038785
VA(0x0041a89d, 0x473)
void philAI::CheckReload(void) {}

// donor PoL RVA 0x00038c3d; preferred Buka symbol ?CheckBerserk@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.559466;margin=0.357720;shape=0.362;size=0.984;calls=1.000;alternate=pol20:void philAI::CheckBerserk(void)@0x00038c3d
VA(0x0041ad10, 0x294)
void philAI::CheckBerserk(void) {}

// donor PoL RVA 0x00039631; preferred Buka symbol ?DoAI@philAI@@QAEXH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.641984;margin=1.146879;shape=0.398;size=0.795;calls=0.741;strings====================================|DO AI|DO AI 1;alternate=pol20:void philAI::DoAI(int)@0x00039631
VA(0x0041b144, 0x8f0)
void philAI::DoAI(int) {}

// donor PoL RVA 0x0003a329; preferred Buka symbol ?GetTurnAIVars@philAI@@QAEXH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.345425;margin=0.144973;shape=0.282;size=0.515;calls=0.667;alternate=pol20:void philAI::GetTurnAIVars(int)@0x0003a329
VA(0x0041ba7f, 0x6a0)
void philAI::GetTurnAIVars(int) {}

// donor PoL RVA 0x0003b154; preferred Buka symbol ?GetBestBHC@philAI@@QAEXHAAUBHC@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.679791;margin=0.499826;shape=0.401;size=0.937;calls=0.722;strings=BestBHC |Turns Owned;alternate=pol20:void philAI::GetBestBHC(int, struct BHC &)@0x0003b154
VA(0x0041c11f, 0x600)
void philAI::GetBestBHC(int, struct BHC &) {}

// @early-stop
// Complete & correct; two residuals are /Od codegen-shape picks (verified via scratch cl,
// not source-steerable): (1) the hero-slot 2D access gpGame[0x4a0+player*283+i] — cl emits
// the full player*283 then `+i`; retail strength-reduces to (i-player)+player*284 (identical
// address). (2) the fight-value max `cmp` loads the fresh value where retail loads the
// accumulator (the same operand-memory pick parked on SetupRelativeHeroStrengths).

// donor PoL RVA 0x0003b865; preferred Buka symbol ?DetermineTargetPosition@philAI@@QAEHAAH0H0@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.393525;margin=0.196143;shape=0.308;size=0.686;calls=0.529;alternate=pol20:int philAI::DetermineTargetPosition(int &, int &, int, int &)@0x0003b865
VA(0x0041c83b, 0x932)
int philAI::DetermineTargetPosition(int &, int &, int, int &) { return 0; }

// donor PoL RVA 0x0003c6e2; preferred Buka symbol ?ProbableOutcomeOfBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@010HHHAAMAAH3333@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.549946;margin=0.581641;shape=0.449;size=0.953;calls=0.724;alternate=pol20:void philAI::ProbableOutcomeOfBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, class armyGroup *, int, int, int, float &, int &, int &, int &, int &, int &)@0x0003c6e2
VA(0x0041d16d, 0x64d)
void philAI::ProbableOutcomeOfBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, class armyGroup *, int, int, int, float &, int &, int &, int &, int &, int &) {}

// donor PoL RVA 0x0003d6b7; preferred Buka symbol ?GetBestBuilding@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526596;margin=0.075083;shape=0.323;size=0.949;calls=1.000;alternate=pol20:void philAI::GetBestBuilding(class town *, struct BHC &, float &)@0x0003d6b7
VA(0x0041dd73, 0x185)
void philAI::GetBestBuilding(town *t, BHC &bhc, float &fOut) {}

// donor PoL RVA 0x0003d852; preferred Buka symbol ?ValueOfBuyingCreature@philAI@@QAEXPAVtown@@HAAHHAAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.431663;margin=0.517459;shape=0.208;size=0.811;calls=0.923;alternate=pol20:void philAI::ValueOfBuyingCreature(class town *, int, int &, int, float &)@0x0003d852
VA(0x0041def8, 0x2f5)
void philAI::ValueOfBuyingCreature(class town *, int, int &, int, float &) {}

// donor PoL RVA 0x0003db58; preferred Buka symbol ?GetBestCreature@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.467416;margin=0.566378;shape=0.359;size=0.696;calls=1.000;alternate=pol20:void philAI::GetBestCreature(class town *, struct BHC &, float &)@0x0003db58
VA(0x0041e1ed, 0x211)
void philAI::GetBestCreature(class town *, struct BHC &, float &) {}

// donor PoL RVA 0x0003df5a; preferred Buka symbol ?MaxBuyableCreatures@philAI@@QAEHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.533802;margin=0.565073;shape=0.392;size=0.839;calls=1.000;alternate=pol20:int philAI::MaxBuyableCreatures(int)@0x0003df5a
VA(0x0041e4a5, 0x9b)
int philAI::MaxBuyableCreatures(int level) { return 0; }

// donor PoL RVA 0x0003dff6; preferred Buka symbol ?ValueOfBuyingHero@philAI@@QAEXPAVtown@@PAVhero@@AAHAAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.452634;margin=0.608146;shape=0.221;size=0.851;calls=1.000;alternate=pol20:void philAI::ValueOfBuyingHero(class town *, class hero *, int &, float &)@0x0003dff6
VA(0x0041e540, 0x1bc)
void philAI::ValueOfBuyingHero(class town *, class hero *, int &, float &) {}

// donor PoL RVA 0x0003e2a8; preferred Buka symbol ?GetBestHero@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.649528;margin=0.194728;shape=0.311;size=0.925;calls=0.800;strings=Town:%2d  Hero    : % 15i   Raw BC = %8.2f,  RandBC = %8.2f.;alternate=pol20:void philAI::GetBestHero(class town *, struct BHC &, float &)@0x0003e2a8
VA(0x0041e6fc, 0x1a0)
void philAI::GetBestHero(class town *, struct BHC &, float &) {}

// donor PoL RVA 0x0003e459; preferred Buka symbol ?LikelihoodOfEnemyAttacking@philAI@@QAEXPAVtown@@PAVhero@@AAM2AAH332@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488198;margin=0.360468;shape=0.323;size=0.832;calls=1.000;alternate=pol20:void philAI::LikelihoodOfEnemyAttacking(class town *, class hero *, float &, float &, int &, int &, int &, float &)@0x0003e459
VA(0x0041e89c, 0x65)
void philAI::LikelihoodOfEnemyAttacking(town *, hero *, float &chanceA, float &chanceB,
                                        int &nAttack, int &nValue, int &nWeeks, float &fOut) {}

// donor PoL RVA 0x0003e7a2; preferred Buka symbol ?RVConversion@philAI@@QAEHQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411394;margin=0.429857;shape=0.250;size=0.681;calls=1.000;alternate=pol20:int philAI::RVConversion(int * const)@0x0003e7a2
VA(0x0041ebdb, 0xa6)
int philAI::RVConversion(int *const p) { return 0; }

// donor PoL RVA 0x0003e918; preferred Buka symbol ?RVOfPosition@philAI@@QAEHHHHHHHHHHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513260;margin=0.500427;shape=0.306;size=0.971;calls=0.812;alternate=pol20:int philAI::RVOfPosition(int, int, int, int, int, int, int, int, int, int)@0x0003e918
VA(0x0041ed4b, 0x55e)
int philAI::RVOfPosition(int, int, int, int, int, int, int, int, int, int) { return 0; }

// donor PoL RVA 0x0003ef45; preferred Buka symbol ?StrategicValueOfPosition@philAI@@QAEHHHHHPAHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.499321;margin=0.324582;shape=0.341;size=0.829;calls=0.957;alternate=pol20:int philAI::StrategicValueOfPosition(int, int, int, int, int *, int)@0x0003ef45
VA(0x0041f2c3, 0x8bd)
int philAI::StrategicValueOfPosition(int, int, int, int, int *, int) { return 0; }

// @early-stop
// Complete & correct except the two castle-match `==` compares: cl unconditionally loads
// the byte operand (town castleX/Y) before the word operand (game field); retail evaluates
// left-to-right (word first). Verified via scratch cl: byte-first is hard-wired, not
// source-steerable. Same equality result.

// donor PoL RVA 0x0003fe81; preferred Buka symbol ?FutureDeflator@philAI@@QAEMQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.484127;margin=0.409347;shape=0.286;size=0.877;calls=1.000;alternate=pol20:float philAI::FutureDeflator(int * const)@0x0003fe81
VA(0x0041fedb, 0x51)
float philAI::FutureDeflator(int *const p) { return 0; }

// donor PoL RVA 0x0003fed2; preferred Buka symbol ?FightValueOfStack@philAI@@QAEHPAVarmyGroup@@PAVhero@@HHHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.393076;margin=0.447055;shape=0.297;size=0.768;calls=0.382;alternate=pol20:int philAI::FightValueOfStack(class armyGroup *, class hero *, int, int, int, int)@0x0003fed2
VA(0x0041ff2c, 0x764)
int philAI::FightValueOfStack(class armyGroup *, class hero *, int, int, int, int) { return 0; }

// donor PoL RVA 0x00040aca; preferred Buka symbol ?EvaluateOneTimeCreaturePurchase@philAI@@QAEXHHHAAH00@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513526;margin=0.703256;shape=0.342;size=0.854;calls=1.000;alternate=pol20:void philAI::EvaluateOneTimeCreaturePurchase(int, int, int, int &, int &, int &)@0x00040aca
VA(0x00420690, 0x1da)
void philAI::EvaluateOneTimeCreaturePurchase(int, int, int, int &, int &, int &) {}

// donor PoL RVA 0x00040cb1; preferred Buka symbol ?QuickCombat@philAI@@QAEHPAVarmyGroup@@PAVhero@@01HHAAM2@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.373791;margin=0.600389;shape=0.327;size=0.583;calls=0.548;alternate=pol20:int philAI::QuickCombat(class armyGroup *, class hero *, class armyGroup *, class hero *, int, int, float &, float &)@0x00040cb1
VA(0x0042086a, 0x3a7)
int philAI::QuickCombat(class armyGroup *, class hero *, class armyGroup *, class hero *, int, int, float &, float &) { return 0; }

// donor PoL RVA 0x0004183b; preferred Buka symbol ?HeroInteractionAtTown@philAI@@QAEXPAVhero@@PAVtown@@HPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.484024;margin=0.235954;shape=0.394;size=0.661;calls=0.952;alternate=pol20:void philAI::HeroInteractionAtTown(class hero *, class town *, int, int *)@0x0004183b
VA(0x00420c11, 0xbae)
void philAI::HeroInteractionAtTown(class hero *, class town *, int, int *) {}

// donor PoL RVA 0x000425b0; preferred Buka symbol ?ChooseEvaluateBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@01HHHAAH2@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.572029;margin=0.742196;shape=0.490;size=0.824;calls=1.000;alternate=pol20:void philAI::ChooseEvaluateBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, int, int, int, int &, int &)@0x000425b0
VA(0x00421820, 0xc7)
void philAI::ChooseEvaluateBattle(armyGroup *ag1, hero *h1, armyGroup *ag2, hero *h2,
                                  int a, int b, int c, int &outFlag, int &outValue) {}

// donor PoL RVA 0x00042ead; preferred Buka symbol ?CanBuyBHC@philAI@@QAEHAAUBHC@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.453882;margin=0.780446;shape=0.369;size=0.712;calls=0.667;alternate=pol20:int philAI::CanBuyBHC(struct BHC &)@0x00042ead
VA(0x00421e5d, 0x188)
int philAI::CanBuyBHC(BHC &bhc) { return 0; }

// donor PoL RVA 0x00043007; preferred Buka symbol ?CombatMonsterEvent@philAI@@QAEHPAVhero@@HPAHPAVmapCell@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.554517;margin=0.429398;shape=0.381;size=0.912;calls=1.000;alternate=pol20:int philAI::CombatMonsterEvent(class hero *, int, int *, class mapCell *)@0x00043007
VA(0x00421fe5, 0x177)
int philAI::CombatMonsterEvent(hero *h, int monType, int *pCount, mapCell *cell) { return 0; }

// donor PoL RVA 0x0004316b; preferred Buka symbol ?FightEvent@philAI@@QAEHPAVhero@@PAVmapCell@@H@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.277616;margin=0.834782;shape=0.262;size=0.393;calls=0.393;alternate=pol20:int philAI::FightEvent(class hero *, class mapCell *, int)@0x0004316b
VA(0x0042215c, 0x26a)
int philAI::FightEvent(class hero *, class mapCell *, int) { return 0; }

// donor PoL RVA 0x00043842; preferred Buka symbol ?DamageGroup@philAI@@QAEHPAVarmyGroup@@PAVhero@@1M@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.548912;margin=0.547381;shape=0.486;size=0.739;calls=1.000;alternate=pol20:int philAI::DamageGroup(class armyGroup *, class hero *, class hero *, float)@0x00043842
VA(0x004223c6, 0x73)
int philAI::DamageGroup(armyGroup *ag, hero *loser, hero *, float dmg) { return 0; }

// donor PoL RVA 0x00043980; preferred Buka symbol ?TownEvent@philAI@@QAEXPAVmapCell@@PAVhero@@HH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.582749;margin=0.601748;shape=0.455;size=0.921;calls=1.000;alternate=pol20:void philAI::TownEvent(class mapCell *, class hero *, int, int)@0x00043980
VA(0x0042256a, 0x221)
void philAI::TownEvent(class mapCell *, class hero *, int, int) {}

// donor PoL RVA 0x00043fc4; preferred Buka symbol ?ValueOfEventAtPosition@philAI@@QAEHHHHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.465517;margin=0.659381;shape=0.256;size=0.790;calls=0.952;alternate=pol20:int philAI::ValueOfEventAtPosition(int, int, int, int *)@0x00043fc4
VA(0x0042278b, 0x2085)
int philAI::ValueOfEventAtPosition(int, int, int, int *) { return 0; }
