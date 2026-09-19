// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x00020546; preferred Buka symbol ?ViewSpells@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/SPELLS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.380473;margin=0.603680;shape=0.231;size=0.337;calls=0.235;strings=cmbtmous.mse|spelmous.mse;alternate=pol20:int combatManager::ViewSpells(int)@0x00020546
VA(0x004154f0, 0x147)
int combatManager::ViewSpells(int) { return 0; }

// donor PoL RVA 0x000217be; preferred Buka symbol ?CastSpell@combatManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/SPELLS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.313511;margin=0.551146;shape=0.234;size=0.481;calls=0.596;alternate=pol20:void combatManager::CastSpell(int, int, int, int)@0x000217be
VA(0x00415e44, 0xd69)
void combatManager::CastSpell(int, int, int, int) {}

// donor PoL RVA 0x00023762; preferred Buka symbol ?Fireball@combatManager@@QAEXHH@Z
// donor Buka TU SOURCE/SPELLS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.573325;margin=0.154314;shape=0.268;size=0.794;calls=0.643;strings=fireball.icn;alternate=pol20:void combatManager::Fireball(int, int)@0x00023762
VA(0x004171ad, 0x432)
void combatManager::Fireball(int, int) {}

// donor PoL RVA 0x00023d85; preferred Buka symbol ?MeteorShower@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/SPELLS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.570305;margin=0.151054;shape=0.232;size=0.755;calls=0.889;strings=meteor.icn;alternate=pol20:void combatManager::MeteorShower(int)@0x00023d85
VA(0x004175df, 0x439)
void combatManager::MeteorShower(int) {}

// donor PoL RVA 0x0002414e; preferred Buka symbol ?ElementalStorm@combatManager@@QAEXXZ
// donor Buka TU SOURCE/SPELLS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.609860;margin=0.145005;shape=0.203;size=0.928;calls=0.824;strings=storm.icn;alternate=pol20:void combatManager::ElementalStorm(void)@0x0002414e
VA(0x00417a18, 0x2f3)
void combatManager::ElementalStorm(void) {}

// donor PoL RVA 0x00024449; preferred Buka symbol ?Armageddon@combatManager@@QAEXXZ
// donor Buka TU SOURCE/SPELLS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.422058;margin=0.013140;shape=0.223;size=0.375;calls=0.500;strings=kb.pal;alternate=pol20:void combatManager::Armageddon(void)@0x00024449
VA(0x00417d0b, 0x3e5)
void combatManager::Armageddon(void) {}
