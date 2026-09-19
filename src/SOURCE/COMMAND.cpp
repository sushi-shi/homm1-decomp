// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x0002a6d0; preferred Buka symbol ?Main@combatManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520039;margin=0.117792;shape=0.347;size=0.894;calls=0.889;alternate=pol20:int combatManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x0002a6d0
VA(0x0040f2c0, 0x311)
int combatManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x0002aa3d; preferred Buka symbol ?ValidHexToStandOn@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.417866;margin=0.483630;shape=0.194;size=0.735;calls=1.000;alternate=pol20:int combatManager::ValidHexToStandOn(int)@0x0002aa3d
VA(0x0040f5d1, 0xd6)
int combatManager::ValidHexToStandOn(int) { return 0; }

// donor PoL RVA 0x0002abbe; preferred Buka symbol ?SetCombatDirections@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.555396;margin=0.346771;shape=0.387;size=0.928;calls=1.000;alternate=pol20:void combatManager::SetCombatDirections(int)@0x0002abbe
VA(0x0040f6a7, 0x7e9)
void combatManager::SetCombatDirections(int) {}

// donor PoL RVA 0x0002b45f; preferred Buka symbol ?CheckSetMouseDirection@combatManager@@QAEXHHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.473098;margin=0.353023;shape=0.247;size=0.891;calls=0.857;alternate=pol20:void combatManager::CheckSetMouseDirection(int, int, int)@0x0002b45f
VA(0x0040fe90, 0x620)
void combatManager::CheckSetMouseDirection(int, int, int) {}

// donor PoL RVA 0x0002bb26; preferred Buka symbol ?ProcessCombatMsg@combatManager@@QAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.392432;margin=0.340460;shape=0.273;size=0.720;calls=0.630;alternate=pol20:int combatManager::ProcessCombatMsg(struct tag_message &)@0x0002bb26
VA(0x004104e4, 0x5bb)
int combatManager::ProcessCombatMsg(struct tag_message &) { return 0; }

// donor PoL RVA 0x0002c8ff; preferred Buka symbol ?GetCommand@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.402881;margin=0.493749;shape=0.255;size=0.779;calls=0.500;alternate=pol20:int combatManager::GetCommand(int)@0x0002c8ff
VA(0x00410d35, 0x316)
int combatManager::GetCommand(int) { return 0; }

// donor PoL RVA 0x0002ce19; preferred Buka symbol ?RightClick@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.497332;margin=0.914777;shape=0.275;size=0.969;calls=0.875;alternate=pol20:int combatManager::RightClick(int)@0x0002ce19
VA(0x0041104b, 0x1dc)
int combatManager::RightClick(int) { return 0; }

// donor PoL RVA 0x0002d0bf; preferred Buka symbol ?DoCommand@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479004;margin=0.375974;shape=0.217;size=0.994;calls=0.842;alternate=pol20:void combatManager::DoCommand(int)@0x0002d0bf
VA(0x00411227, 0x333)
void combatManager::DoCommand(int) {}

// donor PoL RVA 0x0002d472; preferred Buka symbol ?WinCombatHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.240792;margin=0.378009;shape=0.252;size=0.331;calls=0.261;alternate=pol20:int WinCombatHandler(struct tag_message &)@0x0002d472
VA(0x0041155a, 0x1a3)
int WinCombatHandler(struct tag_message &) { return 0; }

// donor PoL RVA 0x0002d9ed; preferred Buka symbol ?ClearWinLoseBottom@combatManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.589024;margin=0.347731;shape=0.432;size=0.971;calls=1.000;alternate=pol20:void combatManager::ClearWinLoseBottom(class heroWindow *)@0x0002d9ed
VA(0x004116fd, 0x110)
void combatManager::ClearWinLoseBottom(class heroWindow *) {}

// donor PoL RVA 0x0002e2bf; preferred Buka symbol ?ShowDeadArmies@combatManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.669049;margin=0.415746;shape=0.366;size=0.795;calls=0.971;strings=mons32.icn|smalfont.fnt;alternate=pol20:void combatManager::ShowDeadArmies(class heroWindow *)@0x0002e2bf
VA(0x00411b07, 0x7d0)
void combatManager::ShowDeadArmies(class heroWindow *) {}

// donor PoL RVA 0x0002ec8b; preferred Buka symbol ?DoVictory@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.616103;margin=0.112225;shape=0.417;size=0.688;calls=0.762;strings=wincmbt.bin;alternate=pol20:void combatManager::DoVictory(int)@0x0002ec8b
VA(0x004122d7, 0x7a1)
void combatManager::DoVictory(int) {}

// donor PoL RVA 0x0002f834; preferred Buka symbol ?DoLoseWindow@combatManager@@QAEXXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.434201;margin=0.199849;shape=0.391;size=0.664;calls=0.594;alternate=pol20:void combatManager::DoLoseWindow(void)@0x0002f834
VA(0x00412a78, 0x549)
void combatManager::DoLoseWindow(void) {}

// donor PoL RVA 0x0002fbf0; preferred Buka symbol ?DoSurrender@combatManager@@QAEHXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.579024;margin=0.030061;shape=0.292;size=0.742;calls=0.667;strings=port%04d.icn|surrendr.bin;alternate=pol20:int combatManager::DoSurrender(void)@0x0002fbf0
VA(0x00412fc1, 0x2c6)
int combatManager::DoSurrender(void) { return 0; }

// donor PoL RVA 0x000301f3; preferred Buka symbol ?CheckGetAIMove@combatManager@@QAEXXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.398729;margin=0.350217;shape=0.260;size=0.665;calls=0.750;alternate=pol20:void combatManager::CheckGetAIMove(void)@0x000301f3
VA(0x00413428, 0x79)
void combatManager::CheckGetAIMove(void) {}

// donor PoL RVA 0x000302d0; preferred Buka symbol ?GetControl@combatManager@@QAEXXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.502120;margin=0.368499;shape=0.323;size=0.925;calls=0.750;alternate=pol20:void combatManager::GetControl(void)@0x000302d0
VA(0x004134a1, 0x16a)
void combatManager::GetControl(void) {}

// donor PoL RVA 0x0003045f; preferred Buka symbol ?ResetMouse@combatManager@@QAEXXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.488306;margin=0.692886;shape=0.271;size=0.872;calls=1.000;alternate=pol20:void combatManager::ResetMouse(void)@0x0003045f
VA(0x0041360b, 0xdb)
void combatManager::ResetMouse(void) {}

// donor PoL RVA 0x00030536; preferred Buka symbol ?ProcessNextAction@combatManager@@QAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.593971;margin=0.483478;shape=0.242;size=0.886;calls=0.629;strings=Process Act;alternate=pol20:int combatManager::ProcessNextAction(struct tag_message &)@0x00030536
VA(0x004136e6, 0x55a)
int combatManager::ProcessNextAction(struct tag_message &) { return 0; }
