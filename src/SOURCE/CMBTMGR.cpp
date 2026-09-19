// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x000a5b95; preferred Buka symbol ?ValidFlight@army@@QAEHHH@Z
// donor Buka TU SOURCE/FLY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.458791;margin=0.507880;shape=0.224;size=0.814;calls=1.000;alternate=pol20:int army::ValidFlight(int, int)@0x000a5b95
VA(0x0044a847, 0x468)
int army::ValidFlight(int, int) { return 0; }

// donor PoL RVA 0x0008ff0a; preferred Buka symbol ?CombineGroups@combatManager@@QAEXPAVarmyGroup@@0@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.491936;margin=0.502339;shape=0.296;size=0.801;calls=1.000;alternate=pol20:void combatManager::CombineGroups(class armyGroup *, class armyGroup *)@0x0008ff0a
VA(0x0044b5f8, 0x138)
void combatManager::CombineGroups(class armyGroup *, class armyGroup *) {}

// donor PoL RVA 0x00090032; preferred Buka symbol ?SetupCombat@combatManager@@QAEXHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHH@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.451835;margin=0.439518;shape=0.448;size=0.718;calls=0.400;alternate=pol20:void combatManager::SetupCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int)@0x00090032
VA(0x0044b730, 0x3db)
void combatManager::SetupCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int) {}

// donor PoL RVA 0x00090aa0; preferred Buka symbol ?Open@combatManager@@UAEHH@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.748451;margin=0.311399;shape=0.506;size=0.976;calls=0.795;strings=PREBATTL.82M|cmbtmous.mse|cmbtwin.bin;alternate=pol20:int combatManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00090aa0
VA(0x0044bb0b, 0x40e)
int combatManager::Open(int) { return 0; }

// donor PoL RVA 0x00090edf; preferred Buka symbol ?Close@combatManager@@UAEXXZ
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.395151;margin=0.373062;shape=0.339;size=0.542;calls=0.800;alternate=pol20:void combatManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x00090edf
VA(0x0044bf19, 0x1ea)
void combatManager::Close(void) {}

// donor PoL RVA 0x00092652; preferred Buka symbol ?FreeArmies@combatManager@@QAEXXZ
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.478232;margin=0.494777;shape=0.264;size=0.859;calls=1.000;alternate=pol20:void combatManager::FreeArmies(void)@0x00092652
VA(0x0044d270, 0xdc)
void combatManager::FreeArmies(void) {}

// donor PoL RVA 0x0009290f; preferred Buka symbol ?CheckApplyGoodMorale@combatManager@@QAEXHH@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.635108;margin=0.421439;shape=0.300;size=0.934;calls=0.800;strings=goodmrle.82M;alternate=pol20:void combatManager::CheckApplyGoodMorale(int, int)@0x0009290f
VA(0x0044d422, 0x1d9)
void combatManager::CheckApplyGoodMorale(int, int) {}

// donor PoL RVA 0x00092afa; preferred Buka symbol ?CheckApplyBadMorale@combatManager@@QAEHHH@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.593667;margin=0.185214;shape=0.274;size=0.824;calls=0.800;strings=BADMRLE.82M;alternate=pol20:int combatManager::CheckApplyBadMorale(int, int)@0x00092afa
VA(0x0044d5fb, 0x1c6)
int combatManager::CheckApplyBadMorale(int, int) { return 0; }

// donor PoL RVA 0x00092cc7; preferred Buka symbol ?GetNextArmy@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.362089;margin=0.657323;shape=0.226;size=0.622;calls=0.750;alternate=pol20:int combatManager::GetNextArmy(int)@0x00092cc7
VA(0x0044d7c1, 0x209)
int combatManager::GetNextArmy(int) { return 0; }
