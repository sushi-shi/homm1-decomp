// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/Misc.h>
#include <BASE/Icon2b.h>
#include <BASE/palette.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/combatTypes.h>

#include <stdio.h>
#include <string.h>

// Buka SPELLS.cpp ViewSpells; HoMM1 has no elemental or mass-spell target
// checks before queueing the cast.
VA(0x004154f0, 0x147)
signed char combatManager::ViewSpells(int)
{
    m_selectedSpell = gpGame->ViewSpells(m_heroes[giCurGeneral], 0, CombatSpecialHandler, 0);
    if (m_selectedSpell != SPELL_NONE) {
        switch (m_selectedSpell) {
            case SPELL_CURE:
            case SPELL_DISPEL_MAGIC:
            case SPELL_ARMAGEDDON:
            case SPELL_STORM:
                giNextAction = 1;
                giNextActionExtra = m_selectedSpell;
                break;
            default:
                giNextAction = 1;
                giNextActionExtra = m_selectedSpell;
                gpMouseManager->SetPointer("spelmous.mse", m_selectedSpell);
                gpWindowManager->DoDialog(NULL, HandleCastSpell, 0);
                break;
        }
        gpMouseManager->SetPointer("cmbtmous.mse", 0);
        if (m_selectedSpell != SPELL_NONE)
            return 1;
    }
    return 0;
}

// Buka SPELLS.cpp CombatSpecialHandler: spell-book hover help.
VA(0x00415637, 0x160)
short CombatSpecialHandler(struct tag_message &message)
{
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gpWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case 2:
                        gpCombatManager->CombatMessage(cSpellHelp[0], 1);
                        break;
                    case 3:
                        gpCombatManager->CombatMessage(cSpellHelp[1], 1);
                        break;
                    case DIALOG_BUTTON_0:
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

// Buka SPELLS.cpp HandleCastSpell; HoMM1 refreshes the coordinates from the
// mouse manager before re-entering for the teleport destination.
VA(0x00415797, 0x295)
short HandleCastSpell(struct tag_message &message)
{
    short hex;

    switch (message.type) {
        case MESSAGE_MOUSE_MOVE:
            hex = gpCombatManager->GetGridIndex(message.x, message.y);
            if (indexToCastOn != hex) {
                if (!gpCombatManager->ValidSpellTarget(gpCombatManager->m_selectedSpell, hex)) {
                    indexToCastOn = -1;
                    gpMouseManager->SetPointer(0x13);
                    if (gpCombatManager->m_selectedSpell == SPELL_TELEPORT && bInTeleportGetDest)
                        gpCombatManager->CombatMessage("Invalid Teleport Destination", 1);
                    else
                        gpCombatManager->CombatMessage("Select Spell Target", 1);
                } else {
                    indexToCastOn = hex;
                    gpMouseManager->SetPointer(gpCombatManager->m_selectedSpell);
                    gpCombatManager->SpellMessage(gpCombatManager->m_selectedSpell, hex);
                }
            }
            break;
        case MESSAGE_LEFT_BUTTON_DOWN:
            if (indexToCastOn != -1) {
                if (bInTeleportGetDest)
                    giNextActionGridIndex2 = indexToCastOn;
                else {
                    giNextActionGridIndex = indexToCastOn;
                    if (gpCombatManager->m_selectedSpell == SPELL_TELEPORT) {
                        bInTeleportGetDest = 1;
                        indexToCastOn = -1;
                        message.type = MESSAGE_MOUSE_MOVE;
                        gpMouseManager->MouseCoords(message.x, message.y);
                        HandleCastSpell(message);
                        gpCombatManager->CombatMessage("Select teleport destination.", 1);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                }
                bInTeleportGetDest = 0;
                message.type = MESSAGE_WIDGET;
                message.command = WIDGET_COMMAND_DIALOG_SELECT;
                return MESSAGE_DISPATCH_FORWARD;
            }
            break;
        case MESSAGE_KEY_DOWN:
            if (message.keyCode != INPUT_SCAN_ESCAPE)
                break;
        case MESSAGE_RIGHT_BUTTON_DOWN:
            gpCombatManager->m_selectedSpell = SPELL_NONE;
            giNextAction = 0;
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_DIALOG_SELECT;
            bInTeleportGetDest = 0;
            return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka SPELLS.cpp ValidSpellTarget; HoMM1 has no resurrection corpses, and
// anti-magic, dispel and green dragons stop every spell but the area ones.
VA(0x00415a2c, 0x2f0)
signed char combatManager::ValidSpellTarget(signed char spell, signed char hex)
{
    int unused;
    army *target = 0;
    short newHex;

    if (!ValidHex(hex))
        return 0;
    if (spell != 0 && spell != 17 && m_hexCells[hex].m_occupantSide != -1) {
        target = &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
        if (target->m_spellEffect == 12 || target->m_spellEffect == 13 || target->m_creatureType == 0x17)
            return 0;
    }
    switch (spell) {
        case 3:
        case 4:
        case 5:
        case 8:
        case 9:
        case 12:
            if (m_hexCells[hex].m_occupantSide != m_currentSide)
                return 0;
            break;
        case 2:
            if (bInTeleportGetDest) {
                newHex = hex;
                if (newHex == giNextActionGridIndex
                    || !m_armies[gpCombatManager->m_hexCells[giNextActionGridIndex].m_occupantSide]
                                [gpCombatManager->m_hexCells[giNextActionGridIndex].m_occupantIndex]
                                    .CanFit(&newHex))
                    return 0;
            } else {
                if (m_hexCells[hex].m_occupantSide != m_currentSide)
                    return 0;
            }
            break;
        case 1:
        case 6:
        case 7:
        case 10:
        case 14:
        case 18:
            if (m_hexCells[hex].m_occupantSide != 1 - m_currentSide)
                return 0;
            break;
        case 11:
            if (m_hexCells[hex].m_occupantSide == -1)
                return 0;
            if (target->m_creatureType != 0x1a)
                return 0;
            break;
        case 0:
        case 17:
            if (hex == -1 || hex % 9 == 0 || hex % 9 == 8)
                return 0;
            break;
    }
    return 1;
}

// Buka SPELLS.cpp SpellMessage without the resurrection target.
VA(0x00415d1c, 0x128)
void combatManager::SpellMessage(signed char spell, signed char hex)
{
    switch (spell) {
        case SPELL_FIREBALL:
        case SPELL_ARMAGEDDON:
        case SPELL_STORM:
        case SPELL_METEOR_SHOWER:
            sprintf(gText, "Cast %s", gSpellNames[spell]);
            break;
        case SPELL_TELEPORT:
            if (bInTeleportGetDest) {
                sprintf(gText, "Teleport Here");
                break;
            }
        default:
            sprintf(gText, "Cast %s on %s", gSpellNames[spell],
                    gArmyNames[m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex].m_creatureType]);
            break;
    }
    CombatMessage(gText, 1);
}

// Buka SPELLS.cpp CastSpell; HoMM1 has nineteen spells, a single timed effect
// per stack and no eagle eye, mirror image or elementals.
VA(0x00415e44, 0xd69)
void combatManager::CastSpell(signed char spell, signed char targetHex, signed char castByCreature,
                              signed char teleportDest)
{
    army *targetArmy;
    int damage;
    int targetIndex;
    int side;
    int result;
    int armyIndex;
    SAMPLE2 sample;
    int quantity;
    short newHex;
    army *teleportArmy;

    sample = NULL_SAMPLE2;
    if (m_limitCreature) {
        ResetLimitCreature();
        if (ValidHex(m_limitCreatureHex) && m_hexCells[m_limitCreatureHex].m_occupantSide >= 0)
            m_limitCreatureCount[m_hexCells[m_limitCreatureHex].m_occupantSide]
                                [m_hexCells[m_limitCreatureHex].m_occupantIndex]++;
        m_limitCreature = 0;
        m_limitCreatureHex = -1;
        gpCombatManager->DrawFrame(1);
    }
    gpMouseManager->ReallyHidePointer();
    if (!castByCreature && m_heroes[m_currentSide])
        m_heroes[m_currentSide]->UseSpell(spell);
    targetArmy = NULL;
    if (spell == SPELL_FIREBALL || spell == SPELL_METEOR_SHOWER || spell == SPELL_STORM || spell == SPELL_ARMAGEDDON || spell == SPELL_CURE || spell == SPELL_DISPEL_MAGIC)
        targetArmy = NULL;
    else if (ValidHex(targetHex) && m_hexCells[targetHex].m_occupantSide >= 0) {
        targetArmy = &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
        side = m_hexCells[targetHex].m_occupantSide;
        targetIndex = m_hexCells[targetHex].m_occupantIndex;
    } else
        targetArmy = NULL;
    if (!castByCreature)
        m_heroCastSpell[m_currentSide] = 1;
    switch (spell) {
        case SPELL_FIREBALL:
        case SPELL_TELEPORT:
        case SPELL_CURE:
        case SPELL_RESURRECT:
        case SPELL_HASTE:
        case SPELL_BLIND:
        case SPELL_BLESS:
        case SPELL_PROTECTION:
        case SPELL_ANTI_MAGIC:
        case SPELL_ARMAGEDDON:
        case SPELL_STORM:
        case SPELL_METEOR_SHOWER:
            break;
        default:
            if (targetArmy
                && (targetArmy->m_creatureType == 0x17
                    || (targetArmy->m_creatureType == 0xd && SRandom(0, 4) == 1))) {
                sample = LoadPlaySample("RSBRYFZL.82M");
                if (targetArmy->m_creatureType == 0x17)
                    CombatMessage("Dragons are not affected by magic!", 1);
                else
                    CombatMessage("The Dwarves' magic resistance canceled the spell!", 1);
                WaitEndSample(sample, -1);
                goto done;
            }
            break;
    }
    sprintf(gText, "spell%02d.82M", spell);
    sample = LoadPlaySample(gText);
    switch (spell) {
        case SPELL_TELEPORT:
            teleportArmy = targetArmy;
            targetHex = teleportDest;
            teleportArmy->SpellEffect(2, 0);
            m_hexCells[teleportArmy->m_hex].m_occupantSide = -1;
            m_hexCells[teleportArmy->m_hex].m_occupantIndex = -1;
            if (m_hexCells[teleportArmy->m_hex].m_occupantFrame == 1) {
                m_hexCells[teleportArmy->m_hex + 1].m_occupantSide = -1;
                m_hexCells[teleportArmy->m_hex + 1].m_occupantIndex = -1;
            } else if (m_hexCells[teleportArmy->m_hex].m_occupantFrame == 0) {
                m_hexCells[teleportArmy->m_hex - 1].m_occupantSide = -1;
                m_hexCells[teleportArmy->m_hex - 1].m_occupantIndex = -1;
            }
            teleportArmy->SpellEffect(2, 0);
            WaitEndSample(sample, -1);
            sprintf(gText, "telein.82m");
            sample = LoadPlaySample(gText);
            if (teleportArmy->m_stats.attributes & 1) {
                newHex = targetHex;
                if (teleportArmy->m_facing == 0) {
                    newHex = teleportArmy->GetAdjacentCellIndex(newHex, 1);
                    if (newHex == -1
                        || (m_hexCells[newHex].m_occupantSide != -1
                            && (m_hexCells[newHex].m_occupantSide != side
                                || m_hexCells[newHex].m_occupantIndex != targetIndex))
                        || m_hexCells[newHex].m_obstacleIndex != -1)
                        targetHex--;
                }
                if (teleportArmy->m_facing == 1) {
                    newHex = teleportArmy->GetAdjacentCellIndex(newHex, 4);
                    if (newHex == -1
                        || (m_hexCells[newHex].m_occupantSide != -1
                            && (m_hexCells[newHex].m_occupantSide != side
                                || m_hexCells[newHex].m_occupantIndex != targetIndex))
                        || m_hexCells[newHex].m_obstacleIndex != -1)
                        targetHex++;
                }
                teleportArmy->m_hex = targetHex;
                switch (teleportArmy->m_facing) {
                    case 0:
                        m_hexCells[teleportArmy->m_hex].m_occupantSide = side;
                        m_hexCells[teleportArmy->m_hex].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex].m_occupantFrame = 1;
                        m_hexCells[teleportArmy->m_hex + 1].m_occupantSide = side;
                        m_hexCells[teleportArmy->m_hex + 1].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex + 1].m_occupantFrame = 0;
                        break;
                    case 1:
                        m_hexCells[teleportArmy->m_hex].m_occupantSide = side;
                        m_hexCells[teleportArmy->m_hex].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex].m_occupantFrame = 0;
                        m_hexCells[teleportArmy->m_hex - 1].m_occupantSide = side;
                        m_hexCells[teleportArmy->m_hex - 1].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex - 1].m_occupantFrame = 1;
                        break;
                }
                teleportArmy->SpellEffect(2, 0);
            } else {
                teleportArmy->m_hex = targetHex;
                m_hexCells[teleportArmy->m_hex].m_occupantSide = side;
                m_hexCells[teleportArmy->m_hex].m_occupantIndex = targetIndex;
                m_hexCells[teleportArmy->m_hex].m_occupantFrame = -1;
                teleportArmy->SpellEffect(2, 0);
            }
            teleportArmy->Stand(1);
            break;
        case SPELL_LIGHTNING_BOLT:
            sprintf(gText, "The lightning bolt does %d damage to the %s.",
                    m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25,
                    targetArmy->m_quantity > 1 ? gArmyNamesPlural[targetArmy->m_creatureType]
                                               : gArmyNames[targetArmy->m_creatureType]);
            CombatMessage(gText, 1);
            targetArmy->SpellEffect(1, 0);
            targetArmy->Damage(m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25);
            targetArmy->PowEffect(7);
            if (!(targetArmy->m_stats.attributes & 0x10))
                targetArmy->Stand(1);
            break;
        case SPELL_CURE:
            CastMassSpell(m_currentSide, 1);
            break;
        case SPELL_RESURRECT:
            targetArmy->SpellEffect(4, 0);
            targetArmy->SpellEffect(4, 0);
            quantity = targetArmy->m_quantity;
            targetArmy->m_quantity += m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50 / targetArmy->m_stats.hitPoints;
            if (targetArmy->m_initialQuantity < targetArmy->m_quantity)
                targetArmy->m_quantity = targetArmy->m_initialQuantity;
            if (targetArmy->m_quantity - quantity > 1)
                sprintf(gText, "%d %s rise from the dead!", targetArmy->m_quantity - quantity,
                        gArmyNamesPlural[targetArmy->m_creatureType]);
            else
                sprintf(gText, "%d %s rises from the dead!", targetArmy->m_quantity - quantity,
                        gArmyNames[targetArmy->m_creatureType]);
            CombatMessage(gText, 1);
            targetArmy->Stand(1);
            break;
        case SPELL_SLOW:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(6, 0);
            targetArmy->SpellEffect(6, 0);
            targetArmy->m_stats.speed = 1;
            if (targetArmy->m_stats.attributes & 2)
                targetArmy->m_stats.attributes -= 2;
            targetArmy->m_spellEffect = SPELL_SLOW;
            targetArmy->m_unknown52 = 3;
            targetArmy->Stand(1);
            break;
        case SPELL_HASTE:
            gpCombatManager->m_currentSpeed = 4;
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(6, 0);
            targetArmy->SpellEffect(6, 0);
            targetArmy->m_stats.speed = 4;
            targetArmy->m_spellEffect = SPELL_HASTE;
            targetArmy->m_unknown52 = 3;
            targetArmy->Stand(1);
            break;
        case SPELL_BLESS:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(8, 0);
            targetArmy->SpellEffect(8, 0);
            targetArmy->m_damageMode = 3;
            targetArmy->m_spellEffect = SPELL_BLESS;
            targetArmy->m_unknown52 = 3;
            targetArmy->Stand(1);
            break;
        case SPELL_PROTECTION:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(9, 0);
            targetArmy->SpellEffect(9, 0);
            targetArmy->m_spellEffect = SPELL_PROTECTION;
            targetArmy->m_unknown52 = 3;
            targetArmy->m_stats.defense += 3;
            targetArmy->Stand(1);
            break;
        case SPELL_CURSE:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(10, 0);
            targetArmy->m_unknown09 = 2;
            targetArmy->SpellEffect(10, 0);
            targetArmy->m_damageMode = 1;
            targetArmy->m_spellEffect = SPELL_CURSE;
            targetArmy->m_unknown52 = 3;
            targetArmy->Stand(1);
            break;
        case SPELL_BERZERKER:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(14, 0);
            targetArmy->m_unknown09 = 2;
            targetArmy->SpellEffect(14, 0);
            targetArmy->m_spellEffect = SPELL_BERZERKER;
            targetArmy->m_unknown52 = 1;
            targetArmy->Stand(1);
            break;
        case SPELL_PARALYZE:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(18, 0);
            targetArmy->m_unknown09 = 2;
            targetArmy->SpellEffect(18, 0);
            targetArmy->m_damageMode = 1;
            targetArmy->m_spellEffect = SPELL_PARALYZE;
            targetArmy->m_unknown52 = 2;
            targetArmy->Stand(1);
            break;
        case SPELL_BLIND:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(6, 0);
            targetArmy->SpellEffect(6, 0);
            targetArmy->m_stats.speed = 0;
            targetArmy->m_damageMode = 1;
            targetArmy->m_spellEffect = SPELL_BLIND;
            targetArmy->m_unknown52 = 2;
            targetArmy->Stand(1);
            break;
        case SPELL_TURN_UNDEAD:
            targetArmy->m_unknown09 = 2;
            targetArmy->SpellEffect(11, 0);
            targetArmy->m_powFrames = 5;
            targetArmy->PowEffect(0);
            targetArmy->m_quantity = 0;
            break;
        case SPELL_DISPEL_MAGIC:
            CastMassSpell(2, 0);
            break;
        case SPELL_ANTI_MAGIC:
            targetArmy->CancelSpell();
            targetArmy->SpellEffect(12, 0);
            targetArmy->m_spellEffect = SPELL_ANTI_MAGIC;
            targetArmy->m_unknown52 = 3;
            targetArmy->Stand(1);
            break;
        case SPELL_FIREBALL:
            Fireball(targetHex);
            break;
        case SPELL_METEOR_SHOWER:
            MeteorShower(targetHex);
            break;
        case SPELL_STORM:
            ElementalStorm();
            break;
        case SPELL_ARMAGEDDON:
            Armageddon();
            break;
        default:
            DefaultSpell(targetHex);
            break;
    }
    if (targetArmy && targetArmy->m_unknown52 >= 0) {
        if (castByCreature)
            targetArmy->m_spellRounds = 3;
        else
            targetArmy->m_spellRounds = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER];
    }
    WaitEndSample(sample, -1);
done:
    CheckChangeSelector();
}

// Buka SPELLS.cpp DefaultSpell; HoMM1 plays the effect in two frames.
VA(0x00416bad, 0xcb)
void combatManager::DefaultSpell(signed char targetHex)
{
    army *target;

    if (!ValidHex(targetHex) || m_hexCells[targetHex].m_occupantSide < 0)
        return;
    target = &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
    target->m_unknown09 = 1;
    target->SpellEffect(m_selectedSpell, 0);
    target->m_unknown09 = 2;
    target->SpellEffect(m_selectedSpell, 0);
    target->Stand(1);
}

// HoMM1 Cure and Dispel Magic: one glow over every affected stack, then the
// spells are cancelled side by side.
VA(0x00416c78, 0x407)
void combatManager::CastMassSpell(signed char castSide, signed char cureOnly)
{
    int last;
    int unused;
    short side;
    short armyIndex;
    short fileId;
    int startSide;

    m_unknown727 = m_unknown72b = 0;
    fileId = MAKEFILEID(gCombatFxNames[13]);
    if (fileId != gCurLoadedSpellEffect) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(fileId);
        gCurLoadedSpellEffect = fileId;
    }
    if (castSide == 2) {
        startSide = 0;
        last = 1;
    } else {
        startSide = m_currentSide;
        last = m_currentSide;
    }
    for (side = startSide; side <= last; side++) {
        for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
            if (m_armies[side][armyIndex].m_spellEffect != SPELL_ANTI_MAGIC && m_armies[side][armyIndex].m_spellEffect != SPELL_DISPEL_MAGIC
                && m_armies[side][armyIndex].m_creatureType != 0x17) {
                if (!cureOnly) {
                    if (m_armies[side][armyIndex].m_spellEffect != SPELL_NONE)
                        m_armies[side][armyIndex].m_unknown08 = 3;
                } else if (cureOnly == 1) {
                    if (m_armies[side][armyIndex].m_spellEffect == SPELL_SLOW || m_armies[side][armyIndex].m_spellEffect == SPELL_BLIND
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_CURSE
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_BERZERKER
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_PARALYZE)
                        m_armies[side][armyIndex].m_unknown08 = 3;
                }
            }
        }
    }
    for (armyIndex = 0; armyIndex < 10; armyIndex++) {
        m_gridUpdateRow = 0;
        giCombatFxFrame = armyIndex;
        DrawFrame(1);
    }
    for (armyIndex = 0; armyIndex < 10; armyIndex++) {
        m_gridUpdateRow = 0;
        giCombatFxFrame = armyIndex;
        DrawFrame(1);
    }
    if (castSide == 2) {
        CancelSideSpells(0, cureOnly);
        CancelSideSpells(1, cureOnly);
    } else
        CancelSideSpells(castSide, cureOnly);
    DrawFrame(1);
}

// HoMM1: lifts every stack of one side out of the glow and cancels its
// spell (only the harmful ones for Cure).
VA(0x0041707f, 0x12e)
void combatManager::CancelSideSpells(signed char side, signed char cureOnly)
{
    army *curArmy;
    short i;

    for (i = 0; i < m_numArmies[side]; i++) {
        curArmy = &m_armies[side][i];
        curArmy->m_unknown08 = 0;
        curArmy->m_unknown09 = 1;
        if (curArmy->m_spellEffect != SPELL_ANTI_MAGIC && curArmy->m_spellEffect != SPELL_DISPEL_MAGIC && curArmy->m_creatureType != 0x17) {
            if (cureOnly == 1) {
                switch (curArmy->m_spellEffect) {
                    case SPELL_SLOW:
                    case SPELL_BLIND:
                    case SPELL_CURSE:
                    case SPELL_BERZERKER:
                    case SPELL_PARALYZE:
                        curArmy->CancelSpell();
                        break;
                    default:
                        break;
                }
            } else
                curArmy->CancelSpell();
        }
    }
}

// Buka SPELLS.cpp Fireball; HoMM1 draws the clipped ball and its mirror and
// always hits the target hex and its six neighbours.
VA(0x004171ad, 0x432)
void combatManager::Fireball(signed char targetHex)
{
    int damage;
    icon *fireballIcon;
    short x;
    army *curArmy;
    short y;
    short i;
    short adjHexes[7];
    signed char hit;

    if (!ValidHex(targetHex))
        return;
    fireballIcon = gpResourceManager->GetIcon("fireball.icn");
    x = m_hexCells[targetHex].m_x;
    y = m_hexCells[targetHex].m_y - 30;
    for (i = 0; i < 7; i++) {
        glTimers[0] = KBTickCount() + 75;
        m_gridUpdateRow = 0;
        ClippedIconToBitmap(fireballIcon, gpWindowManager->m_screen, x, y, i, 0);
        FlipClippedIconToBitmap(fireballIcon, gpWindowManager->m_screen, x, y, i, 0);
        UpdateCombatArea();
        DrawFrame(0);
        DelayTil(&glTimers[0]);
    }
    gpResourceManager->Dispose(fireballIcon);
    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    adjHexes[0] = targetHex;
    for (i = 0; i < 6; i++)
        adjHexes[i + 1] = curArmy->GetAdjacentCellIndex(targetHex, i);
    damage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 10;
    ClearEffects();
    hit = 0;
    for (i = 0; i < 7; i++) {
        if (adjHexes[i] != -1 && m_hexCells[adjHexes[i]].m_occupantSide != -1) {
            curArmy = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex];
            if (curArmy->m_creatureType != 0x17 && curArmy->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmy->m_creatureType != 0xd || SRandom(0, 127) % 4 != 1)
                && !gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex]) {
                gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex] = 1;
                if (curArmy->m_powFrames == -1) {
                    curArmy->Damage(damage);
                    hit = 1;
                }
            }
        }
    }
    if (hit) {
        sprintf(gText, "The fireball does %d damage.", damage);
        CombatMessage(gText, 1);
    }
    curArmy->PowEffect(7);
    for (i = 0; i < 7; i++) {
        if (adjHexes[i] != -1 && m_hexCells[adjHexes[i]].m_occupantSide != -1) {
            curArmy = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex];
            if (!(curArmy->m_stats.attributes & 0x10))
                curArmy->Stand(1);
        }
    }
    DrawFrame(1);
}

// Buka SPELLS.cpp MeteorShower; HoMM1 drops a meteor on each of the seven
// hexes in turn.
VA(0x004175df, 0x439)
void combatManager::MeteorShower(signed char targetHex)
{
    int damage;
    icon *rockIcon;
    army *curArmy;
    short i;
    short adjHexes[7];
    short j;
    signed char hit;

    if (!ValidHex(targetHex))
        return;
    rockIcon = gpResourceManager->GetIcon("meteor.icn");
    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    adjHexes[0] = targetHex;
    for (i = 0; i < 6; i++)
        adjHexes[i + 1] = curArmy->GetAdjacentCellIndex(targetHex, i);
    for (j = 0; j < 10; j++) {
        glTimers[0] = KBTickCount() + 112.5;
        m_gridUpdateRow = 0;
        DrawFrame(0);
        for (i = 0; i < 7; i++) {
            if (adjHexes[i] != -1)
                rockIcon->DrawToBuffer(m_hexCells[adjHexes[i]].m_x, m_hexCells[adjHexes[i]].m_y, j, 0, 0);
        }
        UpdateCombatArea();
        DelayTil(&glTimers[0]);
    }
    gpResourceManager->Dispose(rockIcon);
    damage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25;
    ClearEffects();
    hit = 0;
    for (i = 0; i < 7; i++) {
        if (adjHexes[i] != -1 && m_hexCells[adjHexes[i]].m_occupantSide != -1) {
            curArmy = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex];
            if (curArmy->m_creatureType != 0x17 && curArmy->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmy->m_creatureType != 0xd || SRandom(0, 127) % 4 != 1)
                && !gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex]) {
                gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex] = 1;
                if (curArmy->m_powFrames == -1) {
                    curArmy->Damage(damage);
                    hit = 1;
                }
            }
        }
    }
    if (hit) {
        sprintf(gText, "The meteor shower does %d damage.", damage);
        CombatMessage(gText, 1);
    }
    curArmy->PowEffect(1);
    for (i = 0; i < 7; i++) {
        if (adjHexes[i] != -1 && m_hexCells[adjHexes[i]].m_occupantSide != -1) {
            curArmy = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide][m_hexCells[adjHexes[i]].m_occupantIndex];
            if (!(curArmy->m_stats.attributes & 0x10))
                curArmy->Stand(1);
        }
    }
    DrawFrame(1);
}

// Buka SPELLS.cpp ElementalStorm over HoMM1's 10x7 grid of 64-pixel tiles.
VA(0x00417a18, 0x2f3)
void combatManager::ElementalStorm(void)
{
    int damage;
    short index;
    short x;
    army *curArmy;
    short cycle;
    short frm;
    short y;
    icon *storm;
    short sideIdx;
    signed char hit;

    storm = gpResourceManager->GetIcon("storm.icn");
    for (cycle = 0; cycle < 5; cycle++) {
        for (frm = 0; frm < 10; frm++) {
            glTimers[0] = KBTickCount() + 75;
            m_gridUpdateRow = 0;
            DrawFrame(0);
            for (y = 0; y < 7; y++) {
                for (x = 0; x < 10; x++)
                    storm->DrawToBuffer(x * 64, y * 64, frm, 0, 0);
            }
            UpdateCombatArea();
            DelayTil(&glTimers[0]);
        }
    }
    gpResourceManager->Dispose(storm);
    hit = 0;
    damage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25;
    for (sideIdx = 0; sideIdx < 2; sideIdx++) {
        for (index = 0; index < m_numArmies[sideIdx]; index++) {
            curArmy = &m_armies[sideIdx][index];
            if (curArmy->m_creatureType != 0x17 && curArmy->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmy->m_creatureType != 0xd || SRandom(0, 127) % 4 != 1)
                && !(curArmy->m_stats.attributes & 0x10)) {
                curArmy->Damage(damage);
                hit = 1;
            }
        }
    }
    if (hit) {
        sprintf(gText, "The elemental storm does %d damage.", damage);
        CombatMessage(gText, 1);
    }
    curArmy->PowEffect(8);
    for (sideIdx = 0; sideIdx < 2; sideIdx++) {
        for (index = 0; index < m_numArmies[sideIdx]; index++) {
            curArmy = &m_armies[sideIdx][index];
            if (!(curArmy->m_stats.attributes & 0x10))
                curArmy->Stand(0);
        }
    }
    DrawFrame(1);
}

// Buka SPELLS.cpp Armageddon; HoMM1 fades a copy of kb.pal to red instead of
// shaking the screen.
VA(0x00417d0b, 0x3dc)
void combatManager::Armageddon(void)
{
    short sideIdx;
    int damage;
    short index;
    signed char *palData;
    short fadeStep;
    army *curArmy;
    palette *kbPal;
    short i;
    palette *workPal;
    signed char hit;

    damage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50;
    hit = 0;
    for (sideIdx = 0; sideIdx < 2; sideIdx++) {
        for (index = 0; index < m_numArmies[sideIdx]; index++) {
            curArmy = &m_armies[sideIdx][index];
            if (curArmy->m_creatureType != 0x17 && curArmy->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmy->m_creatureType != 0xd || SRandom(0, 127) % 4 != 1)
                && !(curArmy->m_stats.attributes & 0x10)) {
                curArmy->Damage(damage);
                hit = 1;
            }
        }
    }
    if (hit) {
        sprintf(gText, "The armaggedon does %d damage.", damage);
        CombatMessage(gText, 1);
    }
    gpWindowManager->m_updateFlags = 0;
    kbPal = gpResourceManager->GetPalette("kb.pal");
    workPal = new palette;
    if (!workPal)
        MemError();
    memcpy(workPal->Data(), kbPal->Data(), 0x300);
    glTimers[0] = KBTickCount() + 75;
    palData = workPal->Data();
    for (fadeStep = 0; fadeStep < 32; fadeStep++) {
        for (i = 0; i < 256; i++) {
            if (palData[i * 3 + 1])
                palData[i * 3 + 1]--;
            if (palData[i * 3 + 2])
                palData[i * 3 + 2]--;
        }
        DelayTil(&glTimers[0]);
        SetPalette(palData, 1);
        glTimers[0] = KBTickCount() + 75;
    }
    curArmy->PowEffect(7);
    for (sideIdx = 0; sideIdx < 2; sideIdx++) {
        for (index = 0; index < m_numArmies[sideIdx]; index++) {
            curArmy = &m_armies[sideIdx][index];
            if (!(curArmy->m_stats.attributes & 0x10))
                curArmy->Stand(0);
        }
    }
    DrawFrame(1);
    SetPalette(kbPal->Data(), 1);
    gpWindowManager->m_updateFlags = 1;
    gpResourceManager->Dispose(kbPal);
    delete workPal;
}
