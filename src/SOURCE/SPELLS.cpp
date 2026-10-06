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

VA(0x00459e90, 0x11f)
i8 combatManager::ViewSpells(i32) {
    m_selectedSpell = gGame->ViewSpells(m_heroes[gCurGeneral], 0, CombatSpecialHandler, 0);
    if (m_selectedSpell != SPELL_NONE) {
        switch (m_selectedSpell) {
            case SPELL_CURE:
            case SPELL_DISPEL_MAGIC:
            case SPELL_ARMAGEDDON:
            case SPELL_STORM:
                gNextAction = ACTION_CAST_SPELL;
                gNextActionExtra = m_selectedSpell;
                break;
            default:
                gNextAction = ACTION_CAST_SPELL;
                gNextActionExtra = m_selectedSpell;
                gMouseManager->SetPointer("spelmous.mse", m_selectedSpell);
                gWindowManager->DoDialog(NULL, HandleCastSpell, 0);
                break;
        }
        gMouseManager->SetPointer("cmbtmous.mse", 0);
        if (m_selectedSpell != SPELL_NONE)
            return 1;
    }
    return 0;
}

// Spell-book hover help.
VA(0x00459faf, 0x101)
i16 CombatSpecialHandler(struct tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case SPELL_BOOK_PREVIOUS_PAGE:
                        gCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_PREVIOUS_PAGE], 1);
                        break;
                    case SPELL_BOOK_NEXT_PAGE:
                        gCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_NEXT_PAGE], 1);
                        break;
                    case DIALOG_BUTTON_0:
                        gCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_CLOSE], 1);
                        break;
                    case SPELL_BOOK_ENTRY_FIRST:
                    case SPELL_BOOK_ENTRY_FIRST + 1:
                    case SPELL_BOOK_ENTRY_FIRST + 2:
                    case SPELL_BOOK_ENTRY_LAST:
                        gCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_SELECT_SPELL], 1);
                        break;
                    default:
                        gCombatManager->CombatMessage(gSpellHelp[SPELL_HELP_VIEW_COMBAT_SPELLS], 1);
                        break;
                }
                return MESSAGE_DISPATCH_CONSUME;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// HandleCastSpell refreshes the coordinates from the mouse manager before
// re-entering for the teleport destination.
#define gSpellTargetHex indexToCastOn // spelling fixes .data order
VA(0x0045a0b0, 0x25d)
i16 HandleCastSpell(struct tag_message& message) {
    DATA(0x0049f97c)
    static i8 gSpellTargetHex = -1;
    i16 hex;

    switch (message.type) {
        case MESSAGE_MOUSE_MOVE:
            hex = gCombatManager->GetGridIndex(message.x, message.y);
            if (hex != gSpellTargetHex) {
                if (!gCombatManager->ValidSpellTarget(gCombatManager->m_selectedSpell, hex)) {
                    gSpellTargetHex = ARMY_HEX_INVALID;
                    gMouseManager->SetPointer(SPELL_POINTER_NO_TARGET);
                    if (gCombatManager->m_selectedSpell == SPELL_TELEPORT && gInTeleportGetDest)
                        gCombatManager->CombatMessage(
                            localization::Tr("spell.teleport.invalid"),
                            1
                        );
                    else
                        gCombatManager->CombatMessage(localization::Tr("spell.target.select"), 1);
                } else {
                    gSpellTargetHex = hex;
                    gMouseManager->SetPointer(gCombatManager->m_selectedSpell);
                    gCombatManager->SpellMessage(gCombatManager->m_selectedSpell, hex);
                }
            }
            break;
        case MESSAGE_LEFT_BUTTON_DOWN:
            if (gSpellTargetHex != ARMY_HEX_INVALID) {
                if (gInTeleportGetDest)
                    gNextActionGridIndex2 = gSpellTargetHex;
                else {
                    gNextActionGridIndex = gSpellTargetHex;
                    if (gCombatManager->m_selectedSpell == SPELL_TELEPORT) {
                        gInTeleportGetDest = 1;
                        gSpellTargetHex = ARMY_HEX_INVALID;
                        message.type = MESSAGE_MOUSE_MOVE;
                        gMouseManager->MouseCoords(message.x, message.y);
                        HandleCastSpell(message);
                        gCombatManager->CombatMessage(localization::Tr("spell.teleport.select"), 1);
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
            gCombatManager->m_selectedSpell = SPELL_NONE;
            gNextAction = ACTION_NONE;
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_DIALOG_SELECT;
            gInTeleportGetDest = 0;
            return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// There are no resurrection corpses; anti-magic, dispel and green dragons
// stop every spell but the area ones.
VA(0x0045a30d, 0x265)
i8 combatManager::ValidSpellTarget(i8 spell, i8 hex) {
    i32 unused;
    army* victim = NULL;
    i16 destHex;

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
                destHex = hex;
                if (destHex == gNextActionGridIndex
                    || !m_armies[gCombatManager->m_hexCells[gNextActionGridIndex].m_occupantSide]
                                [gCombatManager->m_hexCells[gNextActionGridIndex].m_occupantIndex]
                                    .CanFit(&destHex))
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

// A stack carries a single timed effect.
#define target targetCreature     // frame-slot spelling
#define targetSide armySide       // frame-slot spelling
#define targetIndex occupantIndex // frame-slot spelling
#define teleportArmy teleported   // frame-slot spelling
VA(0x0045a668, 0xc5e)
void combatManager::CastSpell(i8 spell, i8 targetHex, i8 castByCreature, i8 teleportDest) {
    army* target;
    i32 unusedStack;
    i32 targetIndex;
    i32 unusedSlot;
    i32 targetSide;
    i32 unusedValue;
    class sample* spellSound;
    i32 quantityBefore;
    class sample* immuneSample;
    i16 targetTailHex;
    army* teleportArmy;

    spellSound = NULL;
    if (m_limitCreature) {
        ResetLimitCreature();
        if (ValidHex(m_limitCreatureHex) && m_hexCells[m_limitCreatureHex].m_occupantSide >= 0)
            m_limitCreatureCount[m_hexCells[m_limitCreatureHex].m_occupantSide]
                                [m_hexCells[m_limitCreatureHex].m_occupantIndex]++;
        m_limitCreature = 0;
        m_limitCreatureHex = ARMY_HEX_INVALID;
        gCombatManager->DrawFrame(1);
    }
    gMouseManager->ReallyHidePointer();
    if (!castByCreature && m_heroes[m_currentSide])
        m_heroes[m_currentSide]->UseSpell(spell);
    target = NULL;
    if (spell == SPELL_FIREBALL || spell == SPELL_METEOR_SHOWER || spell == SPELL_STORM
        || spell == SPELL_ARMAGEDDON || spell == SPELL_CURE || spell == SPELL_DISPEL_MAGIC)
        target = NULL;
    else if (ValidHex(targetHex) && m_hexCells[targetHex].m_occupantSide >= 0) {
        target =
            &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
        targetSide = m_hexCells[targetHex].m_occupantSide;
        targetIndex = m_hexCells[targetHex].m_occupantIndex;
    } else
        target = NULL;
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
            if (target
                && (target->m_creatureType == CREATURE_DRAGON
                    || (target->m_creatureType == CREATURE_DWARF && SRandom(0, 4) == 1))) {
                immuneSample = LoadPlaySample("RSBRYFZL.82M");
                if (target->m_creatureType == CREATURE_DRAGON)
                    CombatMessage(localization::Tr("spell.dragon.immune"), 1);
                else
                    CombatMessage(localization::Tr("spell.dwarf.resisted"), 1);
                WaitSample(immuneSample);
                goto done;
            }
            break;
    }
    sprintf(gText, "spell%02d.82M", spell);
    spellSound = LoadPlaySample(gText);
    switch (spell) {
        case SPELL_TELEPORT:
            teleportArmy = target;
            targetHex = teleportDest;
            teleportArmy->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            CLEAR_HEX_OCCUPANT(m_hexCells[teleportArmy->m_hex]);
            if (m_hexCells[teleportArmy->m_hex].m_occupantFootprintHalf == ARMY_FACING_LEFT) {
                CLEAR_HEX_OCCUPANT(m_hexCells[teleportArmy->m_hex + 1]);
            } else if (m_hexCells[teleportArmy->m_hex].m_occupantFootprintHalf
                       == ARMY_FACING_RIGHT) {
                CLEAR_HEX_OCCUPANT(m_hexCells[teleportArmy->m_hex - 1]);
            }
            teleportArmy->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            WaitSample(spellSound);
            sprintf(gText, "telein.82m");
            spellSound = LoadPlaySample(gText);
            if (teleportArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                targetTailHex = targetHex;
                if (teleportArmy->m_facing == ARMY_FACING_RIGHT) {
                    targetTailHex =
                        teleportArmy->GetAdjacentCellIndex(targetTailHex, COMBAT_DIRECTION_EAST);
                    if (targetTailHex == ARMY_HEX_INVALID
                        || (m_hexCells[targetTailHex].m_occupantSide != COMBAT_SIDE_NONE
                            && (m_hexCells[targetTailHex].m_occupantSide != targetSide
                                || m_hexCells[targetTailHex].m_occupantIndex != targetIndex))
                        || m_hexCells[targetTailHex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                        targetHex--;
                }
                if (teleportArmy->m_facing == ARMY_FACING_LEFT) {
                    targetTailHex =
                        teleportArmy->GetAdjacentCellIndex(targetTailHex, COMBAT_DIRECTION_WEST);
                    if (targetTailHex == ARMY_HEX_INVALID
                        || (m_hexCells[targetTailHex].m_occupantSide != COMBAT_SIDE_NONE
                            && (m_hexCells[targetTailHex].m_occupantSide != targetSide
                                || m_hexCells[targetTailHex].m_occupantIndex != targetIndex))
                        || m_hexCells[targetTailHex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                        targetHex++;
                }
                teleportArmy->m_hex = targetHex;
                switch (teleportArmy->m_facing) {
                    case ARMY_FACING_RIGHT:
                        m_hexCells[teleportArmy->m_hex].m_occupantSide = targetSide;
                        m_hexCells[teleportArmy->m_hex].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex].m_occupantFootprintHalf = ARMY_FACING_LEFT;
                        m_hexCells[teleportArmy->m_hex + 1].m_occupantSide = targetSide;
                        m_hexCells[teleportArmy->m_hex + 1].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex + 1].m_occupantFootprintHalf =
                            ARMY_FACING_RIGHT;
                        break;
                    case ARMY_FACING_LEFT:
                        m_hexCells[teleportArmy->m_hex].m_occupantSide = targetSide;
                        m_hexCells[teleportArmy->m_hex].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex].m_occupantFootprintHalf = ARMY_FACING_RIGHT;
                        m_hexCells[teleportArmy->m_hex - 1].m_occupantSide = targetSide;
                        m_hexCells[teleportArmy->m_hex - 1].m_occupantIndex = targetIndex;
                        m_hexCells[teleportArmy->m_hex - 1].m_occupantFootprintHalf =
                            ARMY_FACING_LEFT;
                        break;
                }
                teleportArmy->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            } else {
                teleportArmy->m_hex = targetHex;
                m_hexCells[teleportArmy->m_hex].m_occupantSide = targetSide;
                m_hexCells[teleportArmy->m_hex].m_occupantIndex = targetIndex;
                m_hexCells[teleportArmy->m_hex].m_occupantFootprintHalf =
                    HEXCELL_FOOTPRINT_HALF_NONE;
                teleportArmy->SpellEffect(COMBAT_EFFECT_TELEPORT, 0);
            }
            teleportArmy->Stand(1);
            break;
        case SPELL_LIGHTNING_BOLT:
            sprintf(
                gText,
                localization::Tr("spell.lightning.damage"),
                m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25,
                CREATURE_DISPLAY_NAME(target->m_creatureType, target->m_quantity)
            );
            CombatMessage(gText, 1);
            target->SpellEffect(COMBAT_EFFECT_LIGHTNING_BOLT, 0);
            target->Damage(m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25);
            target->PowEffect(COMBAT_POW_RED_FIRE);
            if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
                target->Stand(1);
            break;
        case SPELL_CURE:
            CastMassSpell(m_currentSide, 1);
            break;
        case SPELL_RESURRECT:
            target->SpellEffect(COMBAT_EFFECT_RESURRECT, 0);
            target->SpellEffect(COMBAT_EFFECT_RESURRECT, 0);
            quantityBefore = target->m_quantity;
            target->m_quantity += m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER]
                                  * 50 / target->m_stats.hitPoints;
            if (target->m_quantity > target->m_initialQuantity)
                target->m_quantity = target->m_initialQuantity;
            if (target->m_quantity - quantityBefore > 1)
                sprintf(
                    gText,
                    localization::Tr("spell.resurrect.plural"),
                    target->m_quantity - quantityBefore,
                    gArmyNamesPlural[target->m_creatureType]
                );
            else
                sprintf(
                    gText,
                    localization::Tr("spell.resurrect.singular"),
                    target->m_quantity - quantityBefore,
                    gArmyNames[target->m_creatureType]
                );
            CombatMessage(gText, 1);
            target->Stand(1);
            break;
        case SPELL_SLOW:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            target->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            target->m_stats.speed = CREATURE_SPEED_SLOW;
            if (target->m_stats.attributes & MONSTER_FLAGS_FLYING)
                target->m_stats.attributes -= MONSTER_FLAGS_FLYING;
            target->m_spellEffect = SPELL_SLOW;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            target->Stand(1);
            break;
        case SPELL_HASTE:
            gCombatManager->m_currentSpeed = CREATURE_SPEED_BLAZING;
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            target->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            target->m_stats.speed = CREATURE_SPEED_BLAZING;
            target->m_spellEffect = SPELL_HASTE;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            target->Stand(1);
            break;
        case SPELL_BLESS:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_BLESS, 0);
            target->SpellEffect(COMBAT_EFFECT_BLESS, 0);
            target->m_damageMode = ARMY_DAMAGE_MAXIMUM;
            target->m_spellEffect = SPELL_BLESS;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            target->Stand(1);
            break;
        case SPELL_PROTECTION:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_PROTECTION, 0);
            target->SpellEffect(COMBAT_EFFECT_PROTECTION, 0);
            target->m_spellEffect = SPELL_PROTECTION;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            target->m_stats.defense += ARMY_PROTECTION_DEFENSE_BONUS;
            target->Stand(1);
            break;
        case SPELL_CURSE:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_CURSE, 0);
            target->m_animationFrame = 2;
            target->SpellEffect(COMBAT_EFFECT_CURSE, 0);
            target->m_damageMode = ARMY_DAMAGE_MINIMUM;
            target->m_spellEffect = SPELL_CURSE;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            target->Stand(1);
            break;
        case SPELL_BERZERKER:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_BERZERKER, 0);
            target->m_animationFrame = 2;
            target->SpellEffect(COMBAT_EFFECT_BERZERKER, 0);
            target->m_spellEffect = SPELL_BERZERKER;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_AFTER_ATTACK;
            target->Stand(1);
            break;
        case SPELL_PARALYZE:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_PARALYZE, 0);
            target->m_animationFrame = 2;
            target->SpellEffect(COMBAT_EFFECT_PARALYZE, 0);
            target->m_damageMode = ARMY_DAMAGE_MINIMUM;
            target->m_spellEffect = SPELL_PARALYZE;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_AFTER_DAMAGE;
            target->Stand(1);
            break;
        case SPELL_BLIND:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            target->SpellEffect(COMBAT_EFFECT_SLOW, 0);
            target->m_stats.speed = CREATURE_SPEED_NONE;
            target->m_damageMode = ARMY_DAMAGE_MINIMUM;
            target->m_spellEffect = SPELL_BLIND;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_AFTER_DAMAGE;
            target->Stand(1);
            break;
        case SPELL_TURN_UNDEAD:
            target->m_animationFrame = 2;
            target->SpellEffect(COMBAT_EFFECT_TURN_UNDEAD, 0);
            target->m_powFrames = ARMY_POW_FRAMES_KILLED;
            target->PowEffect(COMBAT_POW_CLOUD);
            target->m_quantity = 0;
            break;
        case SPELL_DISPEL_MAGIC:
            CastMassSpell(COMBAT_SIDE_ANY, 0);
            break;
        case SPELL_ANTI_MAGIC:
            target->CancelSpell();
            target->SpellEffect(COMBAT_EFFECT_ANTI_MAGIC, 0);
            target->m_spellEffect = SPELL_ANTI_MAGIC;
            target->m_spellEndCondition = ARMY_CANCEL_SPELLS_ROUNDS_ONLY;
            target->Stand(1);
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
    if (target && target->m_spellEndCondition >= 0) {
        if (castByCreature)
            target->m_spellRounds = 3;
        else
            target->m_spellRounds =
                m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER];
    }
    WaitSample(spellSound);
done:
    CheckChangeSelector();
}
#undef target
#undef targetSide
#undef targetIndex
#undef teleportArmy

// DefaultSpell plays the effect in two frames.
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

// Cure and Dispel Magic: one glow over every affected stack, then the
// spells are cancelled side by side.
VA(0x0045b375, 0x383)
void combatManager::CastMassSpell(i8 castSide, i8 cureOnly) {
    i16 effectFile;
    i32 spare;
    i32 lastSide;
    i16 armyIndex;
    i16 side;
    i32 firstSide;

    m_computeExtent = m_redrawExtent = 0;
    effectFile = MAKEFILEID(gCombatFxNames[COMBAT_EFFECT_DISPEL_MAGIC]);
    if (gCurLoadedSpellFileId != effectFile) {
        gResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gResourceManager->GetIcon(effectFile);
        gCurLoadedSpellFileId = effectFile;
    }
    if (castSide == COMBAT_SIDE_ANY) {
        firstSide = COMBAT_DEFENDER_SIDE;
        lastSide = COMBAT_ATTACKER_SIDE;
    } else {
        firstSide = m_currentSide;
        lastSide = m_currentSide;
    }
    for (side = firstSide; side <= lastSide; side++) {
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

// Lifts every stack of one side out of the glow and cancels its
// spell (only the harmful ones for Cure).
VA(0x0045b6f8, 0xf0)
void combatManager::CancelSideSpells(i8 side, i8 cureOnly) {
    army* target;
    i16 armyIndex;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        target = &m_armies[side][armyIndex];
        target->m_animationSequence = ARMY_ANIMATION_STAND;
        target->m_animationFrame = 1;
        if (target->m_spellEffect != SPELL_ANTI_MAGIC && target->m_spellEffect != SPELL_DISPEL_MAGIC
            && target->m_creatureType != CREATURE_DRAGON) {
            if (cureOnly == 1) {
                switch (target->m_spellEffect) {
                    case SPELL_SLOW:
                    case SPELL_BLIND:
                    case SPELL_CURSE:
                    case SPELL_BERZERKER:
                    case SPELL_PARALYZE:
                        target->CancelSpell();
                        break;
                    default:
                        break;
                }
            } else
                target->CancelSpell();
        }
    }
}

// Fireball draws the clipped ball and its mirror and always hits the target
// hex and its six neighbours.
#define x xPos          // frame-slot spelling
#define anyAffected hit // frame-slot spelling
VA(0x0045b7e8, 0x403)
void combatManager::Fireball(i8 targetHex) {
    i32 baseDamage;
    icon* fireballIcon;
    i16 x;
    army* target;
    i16 y;
    i16 i;
    i16 affectedHexes[COMBAT_DIRECTION_ADJACENT_COUNT + 1];
    i8 anyAffected;

    if (!ValidHex(targetHex))
        return;
    fireballIcon = gResourceManager->GetIcon("fireball.icn");
    x = m_hexCells[targetHex].m_x;
    y = m_hexCells[targetHex].m_y - 30;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        m_gridUpdateRow = 0;
        ClippedIconToBitmap(fireballIcon, gWindowManager->m_screen, x, y, i, 0);
        FlipClippedIconToBitmap(fireballIcon, gWindowManager->m_screen, x, y, i, 0);
        UpdateCombatArea();
        DrawFrame(0);
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
    }
    gResourceManager->Dispose(fireballIcon);
    target = &m_armies[m_currentSide][m_currentArmyIndex];
    affectedHexes[0] = targetHex;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT; i++)
        affectedHexes[i + 1] = target->GetAdjacentCellIndex(targetHex, i);
    baseDamage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 10;
    ClearEffects();
    anyAffected = 0;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (affectedHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[affectedHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            target = &m_armies[m_hexCells[affectedHexes[i]].m_occupantSide]
                              [m_hexCells[affectedHexes[i]].m_occupantIndex];
            if (!ARMY_IGNORES_SPELLS(target)
                && (target->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !gArmyEffected[m_hexCells[affectedHexes[i]].m_occupantSide]
                                 [m_hexCells[affectedHexes[i]].m_occupantIndex]) {
                gArmyEffected[m_hexCells[affectedHexes[i]].m_occupantSide]
                             [m_hexCells[affectedHexes[i]].m_occupantIndex] = 1;
                if (target->m_powFrames == ARMY_POW_NONE) {
                    target->Damage(baseDamage);
                    anyAffected = 1;
                }
            }
        }
    }
    if (anyAffected) {
        sprintf(gText, localization::Tr("spell.fireball.damage"), baseDamage);
        CombatMessage(gText, 1);
    }
    target->PowEffect(COMBAT_POW_RED_FIRE);
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (affectedHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[affectedHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            target = &m_armies[m_hexCells[affectedHexes[i]].m_occupantSide]
                              [m_hexCells[affectedHexes[i]].m_occupantIndex];
            if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
                target->Stand(1);
        }
    }
    DrawFrame(1);
}
#undef x
#undef anyAffected

// MeteorShower drops a meteor on each of the seven hexes in turn.
#define anyAffected hit     // frame-slot spelling
#define meteorIcon rockIcon // frame-slot spelling
VA(0x0045bbeb, 0x402)
void combatManager::MeteorShower(i8 targetHex) {
    i16 frame;
    i32 baseDamage;
    icon* meteorIcon;
    army* target;
    i16 affectedHexes[COMBAT_DIRECTION_ADJACENT_COUNT + 1];
    i16 i;
    i8 anyAffected;

    if (!ValidHex(targetHex))
        return;
    meteorIcon = gResourceManager->GetIcon("meteor.icn");
    target = &m_armies[m_currentSide][m_currentArmyIndex];
    affectedHexes[0] = targetHex;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT; i++)
        affectedHexes[i + 1] = target->GetAdjacentCellIndex(targetHex, i);
    for (frame = 0; frame < 10; frame++) {
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 112.5;
        m_gridUpdateRow = 0;
        DrawFrame(0);
        for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
            if (affectedHexes[i] != ARMY_HEX_INVALID)
                meteorIcon->DrawToBuffer(
                    m_hexCells[affectedHexes[i]].m_x,
                    m_hexCells[affectedHexes[i]].m_y,
                    frame,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        UpdateCombatArea();
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
    }
    gResourceManager->Dispose(meteorIcon);
    baseDamage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25;
    ClearEffects();
    anyAffected = 0;
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (affectedHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[affectedHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            target = &m_armies[m_hexCells[affectedHexes[i]].m_occupantSide]
                              [m_hexCells[affectedHexes[i]].m_occupantIndex];
            if (!ARMY_IGNORES_SPELLS(target)
                && (target->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !gArmyEffected[m_hexCells[affectedHexes[i]].m_occupantSide]
                                 [m_hexCells[affectedHexes[i]].m_occupantIndex]) {
                gArmyEffected[m_hexCells[affectedHexes[i]].m_occupantSide]
                             [m_hexCells[affectedHexes[i]].m_occupantIndex] = 1;
                if (target->m_powFrames == ARMY_POW_NONE) {
                    target->Damage(baseDamage);
                    anyAffected = 1;
                }
            }
        }
    }
    if (anyAffected) {
        sprintf(gText, localization::Tr("spell.meteor.damage"), baseDamage);
        CombatMessage(gText, 1);
    }
    target->PowEffect(COMBAT_POW_PHYSICAL);
    for (i = 0; i < COMBAT_DIRECTION_ADJACENT_COUNT + 1; i++) {
        if (affectedHexes[i] != ARMY_HEX_INVALID
            && m_hexCells[affectedHexes[i]].m_occupantSide != COMBAT_SIDE_NONE) {
            target = &m_armies[m_hexCells[affectedHexes[i]].m_occupantSide]
                              [m_hexCells[affectedHexes[i]].m_occupantIndex];
            if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
                target->Stand(1);
        }
    }
    DrawFrame(1);
}
#undef anyAffected
#undef meteorIcon

// ElementalStorm over the 10x7 grid of 64-pixel tiles.
VA(0x0045bfed, 0x2d0)
void combatManager::ElementalStorm(void) {
    i16 frame;
    i16 armyIdx;
    i16 x;
    i16 stormRound;
    army* target;
    i16 y;
    icon* storm;
    i32 damage;
    i16 side;
    i8 anyAffected;

    storm = gResourceManager->GetIcon("storm.icn");
    for (stormRound = 0; stormRound < 5; stormRound++) {
        for (frame = 0; frame < 10; frame++) {
            gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
            m_gridUpdateRow = 0;
            DrawFrame(0);
            for (y = 0; y < 7; y++) {
                for (x = 0; x < 10; x++)
                    storm->DrawToBuffer(
                        x * 64,
                        y * 64,
                        frame,
                        ICON_DRAW_NORMAL,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
            UpdateCombatArea();
            DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        }
    }
    gResourceManager->Dispose(storm);
    anyAffected = 0;
    damage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 25;
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (armyIdx = 0; armyIdx < m_numArmies[side]; armyIdx++) {
            target = &m_armies[side][armyIdx];
            if (!ARMY_IGNORES_SPELLS(target)
                && (target->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(target->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                target->Damage(damage);
                anyAffected = 1;
            }
        }
    }
    if (anyAffected) {
        sprintf(gText, localization::Tr("spell.storm.damage"), damage);
        CombatMessage(gText, 1);
    }
    target->PowEffect(COMBAT_POW_ELECTRIC);
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (armyIdx = 0; armyIdx < m_numArmies[side]; armyIdx++) {
            target = &m_armies[side][armyIdx];
            if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
                target->Stand(0);
        }
    }
    DrawFrame(1);
}

// Armageddon fades a copy of kb.pal to red.
#define damage dmg // frame-slot spelling
VA(0x0045c2bd, 0x3b6)
void combatManager::Armageddon(void) {
    i8* paletteBytes;
    i32 damage;
    palette* effectPalette;
    i16 combatSideIndex;
    i16 fadeStep;
    army* stack;
    palette* gamePal;
    i16 color;
    i16 armyIdx;
    i8 hit;

    damage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50;
    hit = 0;
    for (combatSideIndex = 0; combatSideIndex < COMBAT_SIDE_COUNT; combatSideIndex++) {
        for (armyIdx = 0; armyIdx < m_numArmies[combatSideIndex]; armyIdx++) {
            stack = &m_armies[combatSideIndex][armyIdx];
            if (!ARMY_IGNORES_SPELLS(stack)
                && (stack->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(stack->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                stack->Damage(damage);
                hit = 1;
            }
        }
    }
    if (hit) {
        sprintf(gText, localization::Tr("combat.armageddon.damage"), damage);
        CombatMessage(gText, 1);
    }
    gWindowManager->m_updateFlags = 0;
    gamePal = gResourceManager->GetPalette("kb.pal");
    effectPalette = new palette;
    if (!effectPalette)
        MemError();
    memcpy(effectPalette->Data(), gamePal->Data(), 0x300);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    paletteBytes = effectPalette->Data();
    for (fadeStep = 0; fadeStep < 32; fadeStep++) {
        for (color = 0; color < PALETTE_COLOR_COUNT; color++) {
            if (paletteBytes[color * 3 + 1])
                paletteBytes[color * 3 + 1]--;
            if (paletteBytes[color * 3 + 2])
                paletteBytes[color * 3 + 2]--;
        }
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        SetPalette(paletteBytes, 1);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    }
    stack->PowEffect(COMBAT_POW_RED_FIRE);
    for (combatSideIndex = 0; combatSideIndex < COMBAT_SIDE_COUNT; combatSideIndex++) {
        for (armyIdx = 0; armyIdx < m_numArmies[combatSideIndex]; armyIdx++) {
            stack = &m_armies[combatSideIndex][armyIdx];
            if (!(stack->m_stats.attributes & MONSTER_FLAGS_DEAD))
                stack->Stand(0);
        }
    }
    DrawFrame(1);
    SetPalette(gamePal->Data(), 1);
    gWindowManager->m_updateFlags = 1;
    gResourceManager->Dispose(gamePal);
    delete effectPalette;
}
#undef damage

// SPELLS owns retail .data 0x00490690-0x0048f4d3. HandleCastSpell's
// gSpellTargetHex (0x004906b4) is its local static: /Gi emits it at the head
// of that function's literals.
DATA(0x004cccb8)
i8 gInTeleportGetDest = 0;
