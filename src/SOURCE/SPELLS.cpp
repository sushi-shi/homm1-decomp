// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>

// Buka SPELLS.cpp ViewSpells; HoMM1 has no elemental or mass-spell target
// checks before queueing the cast.
VA(0x004154f0, 0x147)
signed char combatManager::ViewSpells(int)
{
    m_selectedSpell = gpGame->ViewSpells(m_heroes[giCurGeneral], 0, CombatSpecialHandler, 0);
    if (m_selectedSpell != -1) {
        switch (m_selectedSpell) {
            case 3:
            case 13:
            case 15:
            case 16:
                giNextAction = 1;
                giNextActionExtra = m_selectedSpell;
                break;
            default:
                giNextAction = 1;
                giNextActionExtra = m_selectedSpell;
                gpMouseManager->SetPointer("spelmous.mse", m_selectedSpell);
                gpWindowManager->DoDialog(0, HandleCastSpell, 0);
                break;
        }
        gpMouseManager->SetPointer("cmbtmous.mse", 0);
        if (m_selectedSpell != -1)
            return 1;
    }
    return 0;
}

// Buka SPELLS.cpp CombatSpecialHandler: spell-book hover help.
VA(0x00415637, 0x160)
short CombatSpecialHandler(struct tag_message &message)
{
    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.payload.widget.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gpWindowManager->m_lastHoverId = message.payload.widget.id;
                switch (message.payload.widget.id) {
                    case 2:
                        gpCombatManager->CombatMessage(cSpellHelp[0], 1);
                        break;
                    case 3:
                        gpCombatManager->CombatMessage(cSpellHelp[1], 1);
                        break;
                    case 0x7800:
                        gpCombatManager->CombatMessage(cSpellHelp[4], 1);
                        break;
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                        gpCombatManager->CombatMessage(cSpellHelp[6], 1);
                        break;
                    default:
                        gpCombatManager->CombatMessage(cSpellHelp[7], 1);
                        break;
                }
                return MESSAGE_DISPATCH_CONSUME;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

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
