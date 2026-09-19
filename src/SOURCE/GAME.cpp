// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x00088607; preferred Buka symbol ?ClearEffects@combatManager@@QAEXXZ
// donor Buka TU SOURCE/SPELLAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.425101;margin=0.364782;shape=0.143;size=0.828;calls=1.000;alternate=pol20:void combatManager::ClearEffects(void)@0x00088607
VA(0x00437977, 0x63)
void combatManager::ClearEffects(void) {}

// donor PoL RVA 0x0000bd60; preferred Buka symbol ?ViewGeneral@combatManager@@QAEHHHH@Z
// donor Buka TU SOURCE/VIEW; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.677383;margin=0.274305;shape=0.458;size=0.778;calls=0.853;strings=port%04d.icn|vgenwin.bin;alternate=pol20:int combatManager::ViewGeneral(int, int, int)@0x0000bd60
VA(0x00438310, 0x56d)
int combatManager::ViewGeneral(int, int, int) { return 0; }

// donor PoL RVA 0x0000c784; preferred Buka symbol ?ViewArmy@combatManager@@QAEXPAVarmy@@H@Z
// donor Buka TU SOURCE/VIEW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.474703;margin=0.690322;shape=0.250;size=0.915;calls=1.000;alternate=pol20:void combatManager::ViewArmy(class army *, int)@0x0000c784
VA(0x00438a9f, 0x161)
void combatManager::ViewArmy(class army *, int) {}

// donor PoL RVA 0x000708b0; preferred Buka symbol ?Write@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.582104;margin=0.473963;shape=0.500;size=0.883;calls=0.885;alternate=pol20:void playerData::Write(int)@0x000708b0
VA(0x00438c00, 0x1f8)
void playerData::Write(int) {}

// donor PoL RVA 0x00070aed; preferred Buka symbol ?Read@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.555449;margin=0.750197;shape=0.444;size=0.878;calls=0.880;alternate=pol20:void playerData::Read(int)@0x00070aed
VA(0x00438df8, 0x1e8)
void playerData::Read(int) {}

// donor PoL RVA 0x00070d1a; preferred Buka symbol ?NextHero@playerData@@QAEHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.473378;margin=0.450383;shape=0.230;size=0.850;calls=1.000;alternate=pol20:int playerData::NextHero(int)@0x00070d1a
VA(0x00438fe0, 0x12c)
int playerData::NextHero(int) { return 0; }

// donor PoL RVA 0x00071d89; preferred Buka symbol ?GenerateStandardFileName@@YIXPAD0@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.427111;margin=0.052277;shape=0.267;size=0.694;calls=1.000;alternate=pol20:void GenerateStandardFileName(char *, char *)@0x00071d89
VA(0x00439d14, 0x129)
void GenerateStandardFileName(char *, char *) {}

// donor PoL RVA 0x00071eb7; preferred Buka symbol ?SaveGame@game@@QAEHPADHC@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.606443;margin=0.348788;shape=0.410;size=0.678;calls=0.698;strings=%s%s|%s.%s|%s.GM%d;alternate=pol20:int game::SaveGame(char *, int, signed char)@0x00071eb7
VA(0x00439e3d, 0x7b2)
int game::SaveGame(char *, int, signed char) { return 0; }

// donor PoL RVA 0x000735bf; preferred Buka symbol ?LoadGame@game@@QAEXPADHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.668603;margin=0.422052;shape=0.401;size=0.926;calls=0.741;strings=%s%s|.\DATA\|.\GAMES\;alternate=pol20:void game::LoadGame(char *, int, int)@0x000735bf
VA(0x0043a5ef, 0x9b2)
void game::LoadGame(char *, int, int) {}

// donor PoL RVA 0x000b88d6; preferred Buka symbol ?UpdateNewGameWindow@game@@QAEXXZ
// donor Buka TU SOURCE/Newgame; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.454910;margin=0.298327;shape=0.215;size=0.546;calls=0.480;strings=%s %d%%;alternate=pol20:void game::UpdateNewGameWindow(void)@0x000b88d6
VA(0x0043b522, 0x2c3)
void game::UpdateNewGameWindow(void) {}

// donor PoL RVA 0x000bc00e; preferred Buka symbol ?ShowInfo@ExpCampaign@@QAEXHH@Z
// donor Buka TU SOURCE/X_CAMPGN; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.710255;margin=0.146523;shape=0.500;size=0.813;calls=0.958;strings=advmice.mse;alternate=pol20:void ExpCampaign::ShowInfo(int, int)@0x000bc00e
VA(0x0043be93, 0x2ad)
void ExpCampaign::ShowInfo(int, int) {}

// donor PoL RVA 0x000bb843; preferred Buka symbol ?InitMap@ExpCampaign@@QAEXXZ
// donor Buka TU SOURCE/X_CAMPGN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.361177;margin=0.269979;shape=0.160;size=0.387;calls=0.167;strings=origdata.bin;alternate=pol20:void ExpCampaign::InitMap(void)@0x000bb843
VA(0x0043c1bf, 0x28c)
void ExpCampaign::InitMap(void) {}

// donor PoL RVA 0x00078b72; preferred Buka symbol ?LoadMap@game@@QAEHPAD@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.657346;margin=0.109543;shape=0.244;size=0.995;calls=1.000;strings=%s%s|.\MAPS\;alternate=pol20:int game::LoadMap(char *)@0x00078b72
VA(0x0043e30a, 0x43a)
int game::LoadMap(char *) { return 0; }

// donor PoL RVA 0x00078fea; preferred Buka symbol ?ClaimTown@game@@QAEXHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.415111;margin=0.758393;shape=0.164;size=0.968;calls=0.500;alternate=pol20:void game::ClaimTown(int, int, int)@0x00078fea
VA(0x0043e744, 0x321)
void game::ClaimTown(int, int, int) {}

// donor PoL RVA 0x00079856; preferred Buka symbol ?ViewSpells@game@@QAEHPAVhero@@HP6IHAAUtag_message@@@ZH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.637141;margin=0.178696;shape=0.427;size=0.725;calls=0.733;strings=spellwin.bin;alternate=pol20:int game::ViewSpells(class hero *, int, int (*)(struct tag_message &), int)@0x00079856
VA(0x0043ed2e, 0x297)
int game::ViewSpells(class hero *, int, int (*)(struct tag_message &), int) { return 0; }

// donor PoL RVA 0x0007a649; preferred Buka symbol ?ViewArmy@game@@QAEXHHHHPAVtown@@HHHPAVhero@@PAVarmy@@PAVarmyGroup@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.612909;margin=0.340762;shape=0.385;size=0.681;calls=0.829;strings= (%d)|%s%d|armywin.bin;alternate=pol20:void game::ViewArmy(int, int, int, int, class town *, int, int, int, class hero *, class army *, class armyGroup *, int)@0x0007a649
VA(0x0043f8cd, 0x8e1)
void game::ViewArmy(int, int, int, int, class town *, int, int, int, class hero *, class army *, class armyGroup *, int) {}

// donor PoL RVA 0x0007b2cf; preferred Buka symbol ?ViewArmyHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.308927;margin=0.170943;shape=0.232;size=0.493;calls=0.556;alternate=pol20:int ViewArmyHandler(struct tag_message &)@0x0007b2cf
VA(0x004401ae, 0x1b8)
int ViewArmyHandler(struct tag_message &) { return 0; }

// donor PoL RVA 0x0007bd99; preferred Buka symbol ?NextPlayer@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.506997;margin=1.216721;shape=0.284;size=0.968;calls=0.889;alternate=pol20:void game::NextPlayer(void)@0x0007bd99
VA(0x00441245, 0x4e1)
void game::NextPlayer(void) {}

// donor PoL RVA 0x00080b64; preferred Buka symbol ?SetVisibility@game@@QAEXHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.449662;margin=0.505159;shape=0.181;size=0.875;calls=1.000;alternate=pol20:void game::SetVisibility(int, int, int, int)@0x00080b64
VA(0x004440e9, 0x259)
void game::SetVisibility(int, int, int, int) {}

// @early-stop
// Logic + frame slots byte-exact; residual is 3 commutative operand-load swaps (the
// inner-loop test y<MAP_HEIGHT and the two y*MAP_WIDTH index multiplies load the OTHER
// operand into eax first). Not source-steerable (operand order / reversed compare /
// extra temp all tested - no effect): it is the TU-cumulative /Od eval-order parity of
// the partial GAME TU (most preceding functions are still placeholders, so the temp
// counter is off from retail). Same class as the ExperienceValueOfStack @early-stop;
// aligns when GAME is fuller.

// donor PoL RVA 0x00080e6c; preferred Buka symbol ?GiveArmy@game@@QAEXPAVarmyGroup@@HHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.542661;margin=0.479075;shape=0.407;size=0.794;calls=1.000;alternate=pol20:void game::GiveArmy(class armyGroup *, int, int, int)@0x00080e6c
VA(0x00444342, 0xfc)
void game::GiveArmy(armyGroup *group, int type, int count, int slot)
{}

// @early-stop
// ~96%: only the operand-load order of one ((signed char*)group)[i] read differs
// (retail loads i then group; we load group then i) — the movsbl(%eax,%ecx) bytes are
// identical (commutative address), only the two preceding movs swap. Proven to compile
// byte-exact in isolation; the flip is a TU-global eval-order effect of the partial GAME
// TU. Re-check when GAME is fuller.

// donor PoL RVA 0x00080f68; preferred Buka symbol ?ExperienceValueOfStack@game@@QAEHPAVarmyGroup@@PAVhero@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.491573;margin=0.501516;shape=0.250;size=0.900;calls=1.000;alternate=pol20:int game::ExperienceValueOfStack(class armyGroup *, class hero *)@0x00080f68
VA(0x0044443e, 0x8c)
int game::ExperienceValueOfStack(armyGroup *group, hero *h)
{ return 0; }

// donor PoL RVA 0x00080ff9; preferred Buka symbol ?GetLuck@game@@QAEHPAVhero@@PAVarmy@@PAVtown@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.340271;margin=0.529148;shape=0.188;size=0.654;calls=0.667;alternate=pol20:int game::GetLuck(class hero *, class army *, class town *)@0x00080ff9
VA(0x0044465b, 0xbf)
int game::GetLuck(class hero *, class army *, class town *) { return 0; }

// @early-stop
// Logic + frame slots byte-exact (col/row/mask + nested x/y land on retail's -0x4..-0x14
// via the {} block); residual is the same TU-cumulative /Od eval-order parity as
// MakeAllWaterVisible - the inner-loop test and the y*MAP_WIDTH multiplies load the other
// operand first. Aligns when GAME is fuller.

// donor PoL RVA 0x00069bef; preferred Buka symbol ?FindAdjacentMonster@advManager@@QAEHHHPAH0HH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.502628;margin=0.574839;shape=0.364;size=0.953;calls=0.667;alternate=pol20:int advManager::FindAdjacentMonster(int, int, int *, int *, int, int)@0x00069bef
VA(0x0044471a, 0x350)
int advManager::FindAdjacentMonster(int, int, int *, int *, int, int) { return 0; }

// donor PoL RVA 0x0008111f; preferred Buka symbol ?SetupAdjacentMons@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.519474;margin=0.741945;shape=0.279;size=0.996;calls=1.000;alternate=pol20:void game::SetupAdjacentMons(void)@0x0008111f
VA(0x00444a6a, 0xde)
void game::SetupAdjacentMons(void)
{}

// donor PoL RVA 0x00081210; preferred Buka symbol ?CancelComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526467;margin=0.434153;shape=0.371;size=0.866;calls=1.000;alternate=pol20:void game::CancelComputerScreen(void)@0x00081210
VA(0x00444b48, 0x61)
void game::CancelComputerScreen(void)
{}

// donor PoL RVA 0x00081271; preferred Buka symbol ?ShowComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.481260;margin=0.673888;shape=0.345;size=0.812;calls=0.778;alternate=pol20:void game::ShowComputerScreen(void)@0x00081271
VA(0x00444ba9, 0x115)
void game::ShowComputerScreen(void)
{}

// donor PoL RVA 0x000813fe; preferred Buka symbol ?WaitForPlayer@game@@QAEXPADH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547163;margin=0.571086;shape=0.476;size=0.842;calls=0.833;alternate=pol20:void game::WaitForPlayer(char *, int)@0x000813fe
VA(0x00444d66, 0x155)
void game::WaitForPlayer(char *, int) {}

// @early-stop
// Computation byte-exact; residual is 2 inline-accessor jmp$+0 brackets the /Ob1
// expander places leading (after the ternary test) where retail places them trailing
// (after the Extra() body) - the documented /Od /Ob1 block-boundary artifact, identical
// in kind to EDITOR/mapcell GetNewCellExtra*. See docs/patterns/inline-accessors.md.

// donor PoL RVA 0x00082547; preferred Buka symbol ?ProcessOnMapHeroes@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.296138;margin=0.478209;shape=0.233;size=0.458;calls=0.545;alternate=pol20:void game::ProcessOnMapHeroes(void)@0x00082547
VA(0x004452a9, 0x347)
void game::ProcessOnMapHeroes(void) {}

// donor PoL RVA 0x00082cbb; preferred Buka symbol ?CheckHeroConsistency@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.487324;margin=0.544395;shape=0.344;size=0.775;calls=1.000;alternate=pol20:void game::CheckHeroConsistency(void)@0x00082cbb
VA(0x004455f0, 0x3b5)
void game::CheckHeroConsistency(void) {}

// donor PoL RVA 0x00083219; preferred Buka symbol ?TransmitSaveGame@game@@QAEHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.660125;margin=0.426397;shape=0.321;size=0.898;calls=0.886;strings=%s%s|.\DATA\|PostWait;alternate=pol20:int game::TransmitSaveGame(int, int, int)@0x00083219
VA(0x004459a5, 0x6e9)
int game::TransmitSaveGame(int, int, int) { return 0; }

// donor PoL RVA 0x00083937; preferred Buka symbol ?ReceiveSaveGame@game@@QAEHHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.655741;margin=0.222523;shape=0.420;size=0.807;calls=0.714;strings=%s%s|.\DATA\|Receive End;alternate=pol20:int game::ReceiveSaveGame(int, int, int, int)@0x00083937
VA(0x0044608e, 0x579)
int game::ReceiveSaveGame(int, int, int, int) { return 0; }

// donor PoL RVA 0x00083fc4; preferred Buka symbol ?DoNewTurn@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.487826;margin=0.297906;shape=0.344;size=0.836;calls=0.867;alternate=pol20:void game::DoNewTurn(void)@0x00083fc4
VA(0x00446607, 0x42b)
void game::DoNewTurn(void) {}

// donor PoL RVA 0x000b6f40; preferred Buka symbol ?GetMap@game@@QAEXXZ
// donor Buka TU SOURCE/Newgame; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547944;margin=0.077231;shape=0.356;size=0.530;calls=0.607;strings=.\MAPS\;alternate=pol20:void game::GetMap(void)@0x000b6f40
VA(0x00446a8a, 0x36f)
void game::GetMap(void) {}

// donor PoL RVA 0x000333c0; preferred Buka symbol ?ViewWorld@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/Viewwrld; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.364493;margin=0.061615;shape=0.277;size=0.633;calls=0.682;alternate=pol20:void advManager::ViewWorld(int, int, int)@0x000333c0
VA(0x004472d8, 0x44e)
void advManager::ViewWorld(int, int, int) {}

// donor PoL RVA 0x0008480a; preferred Buka symbol ?RestoreCell@game@@QAEXHHHHPAVmapCell@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.572404;margin=0.482531;shape=0.423;size=0.977;calls=1.000;alternate=pol20:void game::RestoreCell(int, int, int, int, class mapCell *, int)@0x0008480a
VA(0x00447875, 0xab)
void game::RestoreCell(int x, int y, int obj, int barrier, mapCell *passedCell, int cellFlags)
{}

// @early-stop
// Condition (3-term &&), reinit, and realloc (BaseFree/BaseAlloc/memset) all byte-exact;
// residual is 2 redundant jumps retail /Od emits for the empty then-branch of the
// if/else - an end-of-function trampoline (jmp $+0 class) plus a dead `jmp realloc` -
// that my build collapses to one direct jmp. A /Od jump-layout artifact of the empty
// then; not behaviorally meaningful.

// donor PoL RVA 0x0008c040; preferred Buka symbol ??0armyGroup@@QAE@XZ
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.513410;margin=0.267257;shape=0.385;size=0.817;calls=1.000;alternate=pol20:void armyGroup::constructor(void)@0x0008c040
VA(0x00447920, 0x3c)
armyGroup::armyGroup(void) {}

// donor PoL RVA 0x0008c3f6; preferred Buka symbol ?IsMember@armyGroup@@QAEHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.401440;margin=0.383163;shape=0.171;size=0.719;calls=1.000;alternate=pol20:int armyGroup::IsMember(int)@0x0008c3f6
VA(0x00447ac0, 0x59)
int armyGroup::IsMember(int) { return 0; }

// donor PoL RVA 0x0008c599; preferred Buka symbol ?CanJoin@armyGroup@@QAEHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.455116;margin=0.419277;shape=0.310;size=0.702;calls=1.000;alternate=pol20:int armyGroup::CanJoin(int)@0x0008c599
VA(0x00447c6c, 0x54)
int armyGroup::CanJoin(int) { return 0; }

// donor PoL RVA 0x0008c641; preferred Buka symbol ?Add@armyGroup@@QAEHHHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418523;margin=0.356193;shape=0.176;size=0.729;calls=1.000;alternate=pol20:int armyGroup::Add(int, int, int)@0x0008c641
VA(0x00447d19, 0x132)
int armyGroup::Add(int, int, int) { return 0; }

// donor PoL RVA 0x0008c7d2; preferred Buka symbol ?DamageGroup@armyGroup@@QAEXM@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.521804;margin=0.531953;shape=0.341;size=0.892;calls=1.000;alternate=pol20:void armyGroup::DamageGroup(float)@0x0008c7d2
VA(0x00447ec8, 0x158)
void armyGroup::DamageGroup(float) {}
