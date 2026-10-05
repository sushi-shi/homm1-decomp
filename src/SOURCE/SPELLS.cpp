// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/display.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/inputManager.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/PATH.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Buka SPELLS.cpp ViewSpells; HoMM1 has no elemental or mass-spell target
// checks before queueing the cast.
VA(0x00459e90, 0x11f)
i8 combatManager::ViewSpells(i32) {
    m_selectedSpell = gpGame->ViewSpells(m_heroes[giCurGeneral], 0, CombatSpecialHandler, 0);
    if (m_selectedSpell != SPELL_NONE) {
        switch (m_selectedSpell) {
            case SPELL_CURE:
            case SPELL_DISPEL_MAGIC:
            case SPELL_ARMAGEDDON:
            case SPELL_STORM:
                giNextAction = ACTION_CAST_SPELL;
                giNextActionExtra = m_selectedSpell;
                break;
            default:
                giNextAction = ACTION_CAST_SPELL;
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
VA(0x00459faf, 0x101)
i16 CombatSpecialHandler(struct tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gpWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case SPELL_BOOK_PREVIOUS_PAGE:
                        gpCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_PREVIOUS_PAGE], 1);
                        break;
                    case SPELL_BOOK_NEXT_PAGE:
                        gpCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_NEXT_PAGE], 1);
                        break;
                    case DIALOG_BUTTON_0:
                        gpCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_CLOSE], 1);
                        break;
                    case SPELL_BOOK_ENTRY_FIRST:
                    case SPELL_BOOK_ENTRY_FIRST + 1:
                    case SPELL_BOOK_ENTRY_FIRST + 2:
                    case SPELL_BOOK_ENTRY_LAST:
                        gpCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_SELECT_SPELL], 1);
                        break;
                    default:
                        gpCombatManager->CombatMessage(
                            gSpellHelp[SPELL_HELP_VIEW_COMBAT_SPELLS],
                            1
                        );
                        break;
                }
                return MESSAGE_DISPATCH_CONSUME;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka SPELLS.cpp HandleCastSpell; HoMM1 refreshes the coordinates from the
// mouse manager before re-entering for the teleport destination.
VA(0x0045a0b0, 0x25d)
i16 HandleCastSpell(struct tag_message& message) {
    DATA(0x0049f97c)
    static i8 indexToCastOn = -1;
    i16 hex;

    switch (message.type) {
        case MESSAGE_MOUSE_MOVE:
            hex = gpCombatManager->GetGridIndex(message.x, message.y);
            if (hex != indexToCastOn) {
                if (!gpCombatManager->ValidSpellTarget(gpCombatManager->m_selectedSpell, hex)) {
                    indexToCastOn = ARMY_HEX_INVALID;
                    gpMouseManager->SetPointer(SPELL_POINTER_NO_TARGET);
                    if (gpCombatManager->m_selectedSpell == SPELL_TELEPORT && gInTeleportGetDest)
                        gpCombatManager->CombatMessage(
                            localization::Tr("spell.teleport.invalid"),
                            1
                        );
                    else
                        gpCombatManager->CombatMessage(localization::Tr("spell.target.select"), 1);
                } else {
                    indexToCastOn = hex;
                    gpMouseManager->SetPointer(gpCombatManager->m_selectedSpell);
                    gpCombatManager->SpellMessage(gpCombatManager->m_selectedSpell, hex);
                }
            }
            break;
        case MESSAGE_LEFT_BUTTON_DOWN:
            if (indexToCastOn != ARMY_HEX_INVALID) {
                if (gInTeleportGetDest)
                    giNextActionGridIndex2 = indexToCastOn;
                else {
                    giNextActionGridIndex = indexToCastOn;
                    if (gpCombatManager->m_selectedSpell == SPELL_TELEPORT) {
                        gInTeleportGetDest = 1;
                        indexToCastOn = ARMY_HEX_INVALID;
                        message.type = MESSAGE_MOUSE_MOVE;
                        gpMouseManager->MouseCoords(message.x, message.y);
                        HandleCastSpell(message);
                        gpCombatManager->CombatMessage(
                            localization::Tr("spell.teleport.select"),
                            1
                        );
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                }
                gInTeleportGetDest = 0;
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
            giNextAction = ACTION_NONE;
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_DIALOG_SELECT;
            gInTeleportGetDest = 0;
            return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka SPELLS.cpp ValidSpellTarget; HoMM1 has no resurrection corpses, and
// anti-magic, dispel and green dragons stop every spell but the area ones.
VA(0x0045a30d, 0x265)
i8 combatManager::ValidSpellTarget(i8 spell, i8 hex) {
    i32 unused;
    army* victim = NULL;
    i16 newHex;

    if (!ValidHex(hex))
        return 0;
    if (spell != SPELL_FIREBALL && spell != SPELL_METEOR_SHOWER
        && m_hexCells[hex].m_occupantSide != COMBAT_SIDE_NONE) {
        victim = &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
        if (victim->m_spellEffect == SPELL_ANTI_MAGIC || victim->m_spellEffect == SPELL_DISPEL_MAGIC
            || victim->m_creatureType == CREATURE_DRAGON)
            return 0;
    }
    switch (spell) {
        case SPELL_CURE:
        case SPELL_RESURRECT:
        case SPELL_HASTE:
        case SPELL_BLESS:
        case SPELL_PROTECTION:
        case SPELL_ANTI_MAGIC:
            if (m_hexCells[hex].m_occupantSide != m_currentSide)
                return 0;
            break;
        case SPELL_TELEPORT:
            if (gInTeleportGetDest) {
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
        case SPELL_LIGHTNING_BOLT:
        case SPELL_SLOW:
        case SPELL_BLIND:
        case SPELL_CURSE:
        case SPELL_BERZERKER:
        case SPELL_PARALYZE:
            if (m_hexCells[hex].m_occupantSide != 1 - m_currentSide)
                return 0;
            break;
        case SPELL_TURN_UNDEAD:
            if (m_hexCells[hex].m_occupantSide == COMBAT_SIDE_NONE)
                return 0;
            if (victim->m_creatureType != CREATURE_GHOST)
                return 0;
            break;
        case SPELL_FIREBALL:
        case SPELL_METEOR_SHOWER:
            if (hex == ARMY_HEX_INVALID || hex % COMBAT_GRID_COLUMNS == 0
                || hex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN)
                return 0;
            break;
    }
    return 1;
}

// Buka SPELLS.cpp SpellMessage without the resurrection target.
VA(0x0045a572, 0xf6)
void combatManager::SpellMessage(i8 spell, i8 hex) {
    switch (spell) {
        case SPELL_FIREBALL:
        case SPELL_ARMAGEDDON:
        case SPELL_STORM:
        case SPELL_METEOR_SHOWER:
            sprintf(gText, localization::Tr("combat.spell.cast"), gSpellNames[spell]);
            break;
        case SPELL_TELEPORT:
            if (gInTeleportGetDest) {
                sprintf(gText, localization::Tr("combat.spell.teleport_here"));
                break;
            }
        default:
            sprintf(
                gText,
                localization::Tr("combat.spell.cast_target"),
                gSpellNames[spell],
                gArmyNames[m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex]
                               .m_creatureType]
            );
            break;
    }
    CombatMessage(gText, 1);
}

// Buka SPELLS.cpp CastSpell; HoMM1 has nineteen spells, a single timed effect
// per stack and no eagle eye, mirror image or elementals.
VA(0x0045a668, 0xc5e)
void combatManager::CastSpell(i8 spell, i8 targetHex, i8 castByCreature, i8 teleportDest) {
    army* targetCreature;
    i32 unusedStack;
    i32 occupantIndex;
    i32 unusedSlot;
    i32 armySide;
    i32 unusedValue;
    class sample* sample;
    i32 quantity;
    class sample* immuneSample;
    i16 destination;
    army* teleported;

    sample = NULL;
    if (m_limitCreature) {
        ResetLimitCreature();
        if (ValidHex(m_limitCreatureHex) && m_hexCells[m_limitCreatureHex].m_occupantSide >= 0)
            m_limitCreatureCount[m_hexCells[m_limitCreatureHex].m_occupantSide]
                                [m_hexCells[m_limitCreatureHex].m_occupantIndex]++;
        m_limitCreature = 0;
        m_limitCreatureHex = ARMY_HEX_INVALID;
        gpCombatManager->DrawFrame(1);
    }
    gpMouseManager->ReallyHidePointer();
    if (!castByCreature && m_heroes[m_currentSide])
        m_heroes[m_currentSide]->UseSpell(spell);
    targetCreature = NULL;
    if (spell == SPELL_FIREBALL || spell == SPELL_METEOR_SHOWER || spell == SPELL_STORM
        || spell == SPELL_ARMAGEDDON || spell == SPELL_CURE || spell == SPELL_DISPEL_MAGIC)
        targetCreature = NULL;
    else if (ValidHex(targetHex) && m_hexCells[targetHex].m_occupantSide >= 0) {
        targetCreature =
            &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
        armySide = m_hexCells[targetHex].m_occupantSide;
        occupantIndex = m_hexCells[targetHex].m_occupantIndex;
    } else
        targetCreature = NULL;
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
            if (targetCreature
                && (targetCreature->m_creatureType == CREATURE_DRAGON
                    || (targetCreature->m_creatureType == CREATURE_DWARF && SRandom(0, 4) == 1))) {
                immuneSample = LoadPlaySample("RSBRYFZL.82M");
                if (targetCreature->m_creatureType == CREATURE_DRAGON)
                    CombatMessage(localization::Tr("spell.dragon.immune"), 1);
                else
                    CombatMessage(localization::Tr("spell.dwarf.resisted"), 1);
                WaitSample(immuneSample);
                goto done;
            }
            break;
    }
    sprintf(gText, "spell%02d.82M", spell);
    sample = LoadPlaySample(gText);
    switch (spell) {
        case SPELL_TELEPORT:
            teleported = targetCreature;
            targetHex = teleportDest;
            teleported->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            m_hexCells[teleported->m_hex].m_occupantSide = COMBAT_SIDE_NONE;
            m_hexCells[teleported->m_hex].m_occupantIndex = COMBAT_ARMY_INDEX_NONE;
            if (m_hexCells[teleported->m_hex].m_occupantFrame == ARMY_FACING_LEFT) {
                m_hexCells[teleported->m_hex + 1].m_occupantSide = COMBAT_SIDE_NONE;
                m_hexCells[teleported->m_hex + 1].m_occupantIndex = COMBAT_ARMY_INDEX_NONE;
            } else if (m_hexCells[teleported->m_hex].m_occupantFrame == ARMY_FACING_RIGHT) {
                m_hexCells[teleported->m_hex - 1].m_occupantSide = COMBAT_SIDE_NONE;
                m_hexCells[teleported->m_hex - 1].m_occupantIndex = COMBAT_ARMY_INDEX_NONE;
            }
            teleported->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            WaitSample(sample);
            sprintf(gText, "telein.82m");
            sample = LoadPlaySample(gText);
            if (teleported->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                destination = targetHex;
                if (teleported->m_facing == ARMY_FACING_RIGHT) {
                    destination = teleported->GetAdjacentCellIndex(destination, COMBAT_DIRECTION_EAST);
                    if (destination == ARMY_HEX_INVALID
                        || (m_hexCells[destination].m_occupantSide != COMBAT_SIDE_NONE
                            && (m_hexCells[destination].m_occupantSide != armySide
                                || m_hexCells[destination].m_occupantIndex != occupantIndex))
                        || m_hexCells[destination].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                        targetHex--;
                }
                if (teleported->m_facing == ARMY_FACING_LEFT) {
                    destination = teleported->GetAdjacentCellIndex(destination, COMBAT_DIRECTION_WEST);
                    if (destination == ARMY_HEX_INVALID
                        || (m_hexCells[destination].m_occupantSide != COMBAT_SIDE_NONE
                            && (m_hexCells[destination].m_occupantSide != armySide
                                || m_hexCells[destination].m_occupantIndex != occupantIndex))
                        || m_hexCells[destination].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                        targetHex++;
                }
                teleported->m_hex = targetHex;
                switch (teleported->m_facing) {
                    case ARMY_FACING_RIGHT:
                        m_hexCells[teleported->m_hex].m_occupantSide = armySide;
                        m_hexCells[teleported->m_hex].m_occupantIndex = occupantIndex;
                        m_hexCells[teleported->m_hex].m_occupantFrame = ARMY_FACING_LEFT;
                        m_hexCells[teleported->m_hex + 1].m_occupantSide = armySide;
                        m_hexCells[teleported->m_hex + 1].m_occupantIndex = occupantIndex;
                        m_hexCells[teleported->m_hex + 1].m_occupantFrame = ARMY_FACING_RIGHT;
                        break;
                    case ARMY_FACING_LEFT:
                        m_hexCells[teleported->m_hex].m_occupantSide = armySide;
                        m_hexCells[teleported->m_hex].m_occupantIndex = occupantIndex;
                        m_hexCells[teleported->m_hex].m_occupantFrame = ARMY_FACING_RIGHT;
                        m_hexCells[teleported->m_hex - 1].m_occupantSide = armySide;
                        m_hexCells[teleported->m_hex - 1].m_occupantIndex = occupantIndex;
                        m_hexCells[teleported->m_hex - 1].m_occupantFrame = ARMY_FACING_LEFT;
                        break;
                }
                teleported->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            } else {
                teleported->m_hex = targetHex;
                m_hexCells[teleported->m_hex].m_occupantSide = armySide;
                m_hexCells[teleported->m_hex].m_occupantIndex = occupantIndex;
                m_hexCells[teleported->m_hex].m_occupantFrame = HEXCELL_OCCUPANT_FRAME_NONE;
                teleported->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            }
            teleported->Stand(1);
            break;
        case SPELL_LIGHTNING_BOLT:
            sprintf(
                gText,
                localization::Tr("spell.lightning.damage"),
                m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25,
                CREATURE_DISPLAY_NAME(targetCreature->m_creatureType, targetCreature->m_quantity)
            );
            CombatMessage(gText, 1);
            targetCreature->SpellEffect(COMBAT_EFFECT_LIGHTNING_BOLT, 0);
            targetCreature->Damage(
                m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25
            );
            targetCreature->PowEffect(COMBAT_POW_RED_FIRE);
            if (!(targetCreature->m_stats.attributes & MONSTER_FLAGS_DEAD))
                targetCreature->Stand(1);
            break;
        case SPELL_CURE:
            CastMassSpell(m_currentSide, 1);
            break;
        case SPELL_RESURRECT:
            targetCreature->SpellEffect(COMBAT_EFFECT_RESURRECT, 0);
            targetCreature->SpellEffect(COMBAT_EFFECT_RESURRECT, 0);
            quantity = targetCreature->m_quantity;
            targetCreature->m_quantity +=
                m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50
                / targetCreature->m_stats.hitPoints;
            if (targetCreature->m_quantity > targetCreature->m_initialQuantity)
                targetCreature->m_quantity = targetCreature->m_initialQuantity;
            if (targetCreature->m_quantity - quantity > 1)
                sprintf(
                    gText,
                    localization::Tr("spell.resurrect.plural"),
                    targetCreature->m_quantity - quantity,
                    gArmyNamesPlural[targetCreature->m_creatureType]
                );
            else
                sprintf(
                    gText,
                    localization::Tr("spell.resurrect.singular"),
                    targetCreature->m_quantity - quantity,
                    gArmyNames[targetCreature->m_creatureType]
                );
            CombatMessage(gText, 1);
            targetCreature->Stand(1);
            break;
        case SPELL_SLOW:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            targetCreature->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            targetCreature->m_stats.speed = CREATURE_SPEED_SLOW;
            if (targetCreature->m_stats.attributes & MONSTER_FLAGS_FLYING)
                targetCreature->m_stats.attributes -= MONSTER_FLAGS_FLYING;
            targetCreature->m_spellEffect = SPELL_SLOW;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            targetCreature->Stand(1);
            break;
        case SPELL_HASTE:
            gpCombatManager->m_currentSpeed = CREATURE_SPEED_BLAZING;
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            targetCreature->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            targetCreature->m_stats.speed = CREATURE_SPEED_BLAZING;
            targetCreature->m_spellEffect = SPELL_HASTE;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            targetCreature->Stand(1);
            break;
        case SPELL_BLESS:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_BLESS, 0);
            targetCreature->SpellEffect(COMBAT_EFFECT_BLESS, 0);
            targetCreature->m_damageMode = ARMY_DAMAGE_MAXIMUM;
            targetCreature->m_spellEffect = SPELL_BLESS;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            targetCreature->Stand(1);
            break;
        case SPELL_PROTECTION:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_PROTECTION, 0);
            targetCreature->SpellEffect(COMBAT_EFFECT_PROTECTION, 0);
            targetCreature->m_spellEffect = SPELL_PROTECTION;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            targetCreature->m_stats.defense += ARMY_PROTECTION_DEFENSE_BONUS;
            targetCreature->Stand(1);
            break;
        case SPELL_CURSE:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_CURSE, 0);
            targetCreature->m_animationFrame = 2;
            targetCreature->SpellEffect(COMBAT_EFFECT_CURSE, 0);
            targetCreature->m_damageMode = ARMY_DAMAGE_MINIMUM;
            targetCreature->m_spellEffect = SPELL_CURSE;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            targetCreature->Stand(1);
            break;
        case SPELL_BERZERKER:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_BERZERKER, 0);
            targetCreature->m_animationFrame = 2;
            targetCreature->SpellEffect(COMBAT_EFFECT_BERZERKER, 0);
            targetCreature->m_spellEffect = SPELL_BERZERKER;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_AFTER_ATTACK;
            targetCreature->Stand(1);
            break;
        case SPELL_PARALYZE:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_PARALYZE, 0);
            targetCreature->m_animationFrame = 2;
            targetCreature->SpellEffect(COMBAT_EFFECT_PARALYZE, 0);
            targetCreature->m_damageMode = ARMY_DAMAGE_MINIMUM;
            targetCreature->m_spellEffect = SPELL_PARALYZE;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_AFTER_DAMAGE;
            targetCreature->Stand(1);
            break;
        case SPELL_BLIND:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            targetCreature->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            targetCreature->m_stats.speed = CREATURE_SPEED_NONE;
            targetCreature->m_damageMode = ARMY_DAMAGE_MINIMUM;
            targetCreature->m_spellEffect = SPELL_BLIND;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_AFTER_DAMAGE;
            targetCreature->Stand(1);
            break;
        case SPELL_TURN_UNDEAD:
            targetCreature->m_animationFrame = 2;
            targetCreature->SpellEffect(COMBAT_EFFECT_TURN_UNDEAD, 0);
            targetCreature->m_powFrames = ARMY_POW_FRAMES_KILLED;
            targetCreature->PowEffect(COMBAT_POW_CLOUD);
            targetCreature->m_quantity = 0;
            break;
        case SPELL_DISPEL_MAGIC:
            CastMassSpell(COMBAT_SIDE_ANY, 0);
            break;
        case SPELL_ANTI_MAGIC:
            targetCreature->CancelSpell();
            targetCreature->SpellEffect(COMBAT_EFFECT_ANTI_MAGIC, 0);
            targetCreature->m_spellEffect = SPELL_ANTI_MAGIC;
            targetCreature->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            targetCreature->Stand(1);
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
    if (targetCreature && targetCreature->m_spellEndCondition >= 0) {
        if (castByCreature)
            targetCreature->m_spellRounds = 3;
        else
            targetCreature->m_spellRounds =
                m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER];
    }
    WaitSample(sample);
done:
    CheckChangeSelector();
}

// Buka SPELLS.cpp DefaultSpell; HoMM1 plays the effect in two frames.
VA(0x0045b2c6, 0xaf)
void combatManager::DefaultSpell(i8 targetHex) {
    army* target;

    if (!ValidHex(targetHex) || m_hexCells[targetHex].m_occupantSide < 0)
        return;
    target = &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
    target->m_animationFrame = 1;
    target->SpellEffect(m_selectedSpell, 0);
    target->m_animationFrame = 2;
    target->SpellEffect(m_selectedSpell, 0);
    target->Stand(1);
}

// HoMM1 Cure and Dispel Magic: one glow over every affected stack, then the
// spells are cancelled side by side.
VA(0x0045b375, 0x383)
void combatManager::CastMassSpell(i8 castSide, i8 cureOnly) {
    i32 curLast;
    i32 spare;
    i16 side;
    i16 armyIndex;
    i16 fileIdNo;
    i32 team;

    m_computeExtent = m_redrawExtent = 0;
    fileIdNo = MAKEFILEID(gCombatFxNames[COMBAT_EFFECT_DISPEL_MAGIC]);
    if (gCurLoadedSpellFileId != fileIdNo) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(fileIdNo);
        gCurLoadedSpellFileId = fileIdNo;
    }
    if (castSide == COMBAT_SIDE_ANY) {
        team = COMBAT_DEFENDER_SIDE;
        curLast = COMBAT_ATTACKER_SIDE;
    } else {
        team = m_currentSide;
        curLast = m_currentSide;
    }
    for (side = team; side <= curLast; side++) {
        for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
            if (m_armies[side][armyIndex].m_spellEffect != SPELL_ANTI_MAGIC
                && m_armies[side][armyIndex].m_spellEffect != SPELL_DISPEL_MAGIC
                && m_armies[side][armyIndex].m_creatureType != CREATURE_DRAGON) {
                if (!cureOnly) {
                    if (m_armies[side][armyIndex].m_spellEffect != SPELL_NONE)
                        m_armies[side][armyIndex].m_animationSequence = ARMY_ANIMATION_EFFECT;
                } else if (cureOnly == 1) {
                    if (m_armies[side][armyIndex].m_spellEffect == SPELL_SLOW
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_BLIND
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_CURSE
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_BERZERKER
                        || m_armies[side][armyIndex].m_spellEffect == SPELL_PARALYZE)
                        m_armies[side][armyIndex].m_animationSequence = ARMY_ANIMATION_EFFECT;
                }
            }
        }
    }
    for (armyIndex = 0; armyIndex < 10; armyIndex++) {
        m_gridUpdateRow = 0;
        gSpellEffectFrame = armyIndex;
        DrawFrame(1);
    }
    for (armyIndex = 0; armyIndex < 10; armyIndex++) {
        m_gridUpdateRow = 0;
        gSpellEffectFrame = armyIndex;
        DrawFrame(1);
    }
    if (castSide == COMBAT_SIDE_ANY) {
        CancelSideSpells(COMBAT_DEFENDER_SIDE, cureOnly);
        CancelSideSpells(COMBAT_ATTACKER_SIDE, cureOnly);
    } else
        CancelSideSpells(castSide, cureOnly);
    DrawFrame(1);
}

// HoMM1: lifts every stack of one side out of the glow and cancels its
// spell (only the harmful ones for Cure).
VA(0x0045b6f8, 0xf0)
void combatManager::CancelSideSpells(i8 side, i8 cureOnly) {
    army* curArmy;
    i16 ii;

    for (ii = 0; ii < m_numArmies[side]; ii++) {
        curArmy = &m_armies[side][ii];
        curArmy->m_animationSequence = ARMY_ANIMATION_STAND;
        curArmy->m_animationFrame = 1;
        if (curArmy->m_spellEffect != SPELL_ANTI_MAGIC
            && curArmy->m_spellEffect != SPELL_DISPEL_MAGIC
            && curArmy->m_creatureType != CREATURE_DRAGON) {
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
VA(0x0045b7e8, 0x403)
void combatManager::Fireball(i8 targetHex) {
    i32 prevDmg;
    icon* fireballIcon;
    i16 xPos;
    army* curArmyPtr;
    i16 y;
    i16 i;
    i16 adjHexes[COMBAT_DIRECTION_ADJACENT_COUNT + 1];
    i8 hit;

    if (!ValidHex(targetHex))
        return;
    fireballIcon = gpResourceManager->GetIcon("fireball.icn");
    xPos = m_hexCells[targetHex].m_x;
    y = m_hexCells[targetHex].m_y - 30;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        m_gridUpdateRow = 0;
        ClippedIconToBitmap(fireballIcon, gpWindowManager->m_screen, xPos, y, i, 0);
        FlipClippedIconToBitmap(fireballIcon, gpWindowManager->m_screen, xPos, y, i, 0);
        UpdateCombatArea();
        DrawFrame(0);
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
    }
    gpResourceManager->Dispose(fireballIcon);
    curArmyPtr = &m_armies[m_currentSide][m_currentArmyIndex];
    adjHexes[0] = targetHex;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT; i++)
        adjHexes[i + 1] = curArmyPtr->GetAdjacentCellIndex(targetHex, i);
    prevDmg = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 10;
    ClearEffects();
    hit = 0;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (adjHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[adjHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            curArmyPtr = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide]
                                  [m_hexCells[adjHexes[i]].m_occupantIndex];
            if (curArmyPtr->m_creatureType != CREATURE_DRAGON
                && curArmyPtr->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmyPtr->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide]
                                 [m_hexCells[adjHexes[i]].m_occupantIndex]) {
                gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide]
                             [m_hexCells[adjHexes[i]].m_occupantIndex] = 1;
                if (curArmyPtr->m_powFrames == ARMY_POW_NONE) {
                    curArmyPtr->Damage(prevDmg);
                    hit = 1;
                }
            }
        }
    }
    if (hit) {
        sprintf(gText, localization::Tr("spell.fireball.damage"), prevDmg);
        CombatMessage(gText, 1);
    }
    curArmyPtr->PowEffect(COMBAT_POW_RED_FIRE);
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (adjHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[adjHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            curArmyPtr = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide]
                                  [m_hexCells[adjHexes[i]].m_occupantIndex];
            if (!(curArmyPtr->m_stats.attributes & MONSTER_FLAGS_DEAD))
                curArmyPtr->Stand(1);
        }
    }
    DrawFrame(1);
}

// Buka SPELLS.cpp MeteorShower; HoMM1 drops a meteor on each of the seven
// hexes in turn.
VA(0x0045bbeb, 0x402)
void combatManager::MeteorShower(i8 targetHex) {
    i16 i;
    i32 prevDmg;
    icon* rockIcon;
    army* curArmyPtr;
    i16 adjHexes[COMBAT_DIRECTION_ADJACENT_COUNT + 1];
    i16 j;
    i8 hit;

    if (!ValidHex(targetHex))
        return;
    rockIcon = gpResourceManager->GetIcon("meteor.icn");
    curArmyPtr = &m_armies[m_currentSide][m_currentArmyIndex];
    adjHexes[0] = targetHex;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT; i++)
        adjHexes[i + 1] = curArmyPtr->GetAdjacentCellIndex(targetHex, i);
    for (j = 0; j < 10; j++) {
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 112.5;
        m_gridUpdateRow = 0;
        DrawFrame(0);
        for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
            if (adjHexes[i] != ARMY_HEX_INVALID)
                rockIcon->DrawToBuffer(
                    m_hexCells[adjHexes[i]].m_x,
                    m_hexCells[adjHexes[i]].m_y,
                    j,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        UpdateCombatArea();
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
    }
    gpResourceManager->Dispose(rockIcon);
    prevDmg = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25;
    ClearEffects();
    hit = 0;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (adjHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[adjHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            curArmyPtr = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide]
                                  [m_hexCells[adjHexes[i]].m_occupantIndex];
            if (curArmyPtr->m_creatureType != CREATURE_DRAGON
                && curArmyPtr->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmyPtr->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide]
                                 [m_hexCells[adjHexes[i]].m_occupantIndex]) {
                gArmyEffected[m_hexCells[adjHexes[i]].m_occupantSide]
                             [m_hexCells[adjHexes[i]].m_occupantIndex] = 1;
                if (curArmyPtr->m_powFrames == ARMY_POW_NONE) {
                    curArmyPtr->Damage(prevDmg);
                    hit = 1;
                }
            }
        }
    }
    if (hit) {
        sprintf(gText, localization::Tr("spell.meteor.damage"), prevDmg);
        CombatMessage(gText, 1);
    }
    curArmyPtr->PowEffect(COMBAT_POW_PHYSICAL);
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (adjHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[adjHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            curArmyPtr = &m_armies[m_hexCells[adjHexes[i]].m_occupantSide]
                                  [m_hexCells[adjHexes[i]].m_occupantIndex];
            if (!(curArmyPtr->m_stats.attributes & MONSTER_FLAGS_DEAD))
                curArmyPtr->Stand(1);
        }
    }
    DrawFrame(1);
}

// Buka SPELLS.cpp ElementalStorm over HoMM1's 10x7 grid of 64-pixel tiles.
VA(0x0045bfed, 0x2d0)
void combatManager::ElementalStorm(void) {
    i32 prevDamage;
    i16 indexNum;
    i16 x;
    army* curArmyPtr;
    i16 oldCycle;
    i16 frm;
    i16 y;
    icon* storm;
    i16 sideIdxNo;
    i8 hit;

    storm = gpResourceManager->GetIcon("storm.icn");
    for (oldCycle = 0; oldCycle < 5; oldCycle++) {
        for (frm = 0; frm < 10; frm++) {
            glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
            m_gridUpdateRow = 0;
            DrawFrame(0);
            for (y = 0; y < 7; y++) {
                for (x = 0; x < 10; x++)
                    storm->DrawToBuffer(
                        x * 64,
                        y * 64,
                        frm,
                        ICON_DRAW_NORMAL,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
            UpdateCombatArea();
            DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
        }
    }
    gpResourceManager->Dispose(storm);
    hit = 0;
    prevDamage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25;
    for (sideIdxNo = 0; sideIdxNo < COMBAT_SIDE_COUNT; sideIdxNo++) {
        for (indexNum = 0; indexNum < m_numArmies[sideIdxNo]; indexNum++) {
            curArmyPtr = &m_armies[sideIdxNo][indexNum];
            if (curArmyPtr->m_creatureType != CREATURE_DRAGON
                && curArmyPtr->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmyPtr->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(curArmyPtr->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                curArmyPtr->Damage(prevDamage);
                hit = 1;
            }
        }
    }
    if (hit) {
        sprintf(gText, localization::Tr("spell.storm.damage"), prevDamage);
        CombatMessage(gText, 1);
    }
    curArmyPtr->PowEffect(COMBAT_POW_ELECTRIC);
    for (sideIdxNo = 0; sideIdxNo < COMBAT_SIDE_COUNT; sideIdxNo++) {
        for (indexNum = 0; indexNum < m_numArmies[sideIdxNo]; indexNum++) {
            curArmyPtr = &m_armies[sideIdxNo][indexNum];
            if (!(curArmyPtr->m_stats.attributes & MONSTER_FLAGS_DEAD))
                curArmyPtr->Stand(0);
        }
    }
    DrawFrame(1);
}

// Buka SPELLS.cpp Armageddon; HoMM1 fades a copy of kb.pal to red instead of
// shaking the screen.
VA(0x0045c2bd, 0x3b6)
void combatManager::Armageddon(void) {
    i16 sideIdxNo;
    i32 dmg;
    i16 index;
    i8* palDataEntry;
    i16 fadeStep;
    army* curArmy;
    palette* curPal;
    i16 jx;
    palette* workPalItem;
    i8 hit;

    dmg = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50;
    hit = 0;
    for (sideIdxNo = 0; sideIdxNo < COMBAT_SIDE_COUNT; sideIdxNo++) {
        for (index = 0; index < m_numArmies[sideIdxNo]; index++) {
            curArmy = &m_armies[sideIdxNo][index];
            if (curArmy->m_creatureType != CREATURE_DRAGON
                && curArmy->m_spellEffect != SPELL_ANTI_MAGIC
                && (curArmy->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(curArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                curArmy->Damage(dmg);
                hit = 1;
            }
        }
    }
    if (hit) {
        sprintf(gText, localization::Tr("combat.armageddon.damage"), dmg);
        CombatMessage(gText, 1);
    }
    gpWindowManager->m_updateFlags = 0;
    curPal = gpResourceManager->GetPalette("kb.pal");
    workPalItem = new palette;
    if (!workPalItem)
        MemError();
    memcpy(workPalItem->Data(), curPal->Data(), 0x300);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    palDataEntry = workPalItem->Data();
    for (fadeStep = 0; fadeStep < 32; fadeStep++) {
        for (jx = 0; jx < PALETTE_COLOR_COUNT; jx++) {
            if (palDataEntry[jx * 3 + 1])
                palDataEntry[jx * 3 + 1]--;
            if (palDataEntry[jx * 3 + 2])
                palDataEntry[jx * 3 + 2]--;
        }
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
        SetPalette(palDataEntry, 1);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    }
    curArmy->PowEffect(COMBAT_POW_RED_FIRE);
    for (sideIdxNo = 0; sideIdxNo < COMBAT_SIDE_COUNT; sideIdxNo++) {
        for (index = 0; index < m_numArmies[sideIdxNo]; index++) {
            curArmy = &m_armies[sideIdxNo][index];
            if (!(curArmy->m_stats.attributes & MONSTER_FLAGS_DEAD))
                curArmy->Stand(0);
        }
    }
    DrawFrame(1);
    SetPalette(curPal->Data(), 1);
    gpWindowManager->m_updateFlags = 1;
    gpResourceManager->Dispose(curPal);
    delete workPalItem;
}

// SPELLS owns retail .data 0x00490690-0x0048f4d3. HandleCastSpell's
// indexToCastOn (0x004906b4) is its local static: /Gi emits it at the head
// of that function's literals.
DATA(0x004cccb8)
i8 gInTeleportGetDest = 0;
