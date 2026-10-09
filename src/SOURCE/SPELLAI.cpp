// Combat spell selection for computer-controlled heroes.

#include <match.h>

#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/PATH.h>
#include <SOURCE/philAI.h>

#include <stdlib.h>

// Per-spell weights of a stack's fight value.
DATA(0x0048a6d0)
static const float SPELL_AI_SLOW_MODIFIER = -0.11f;
DATA(0x0048a6d4)
static const float SPELL_AI_BLIND_MODIFIER = -0.6f;
DATA(0x0048a6d8)
static const float SPELL_AI_CURSE_MODIFIER = -0.18f;
DATA(0x0048a6dc)
static const float SPELL_AI_BERSERK_MODIFIER = -0.6f;
DATA(0x0048a6e0)
static const float SPELL_AI_PARALYZE_MODIFIER = -0.7f;
DATA(0x0048a6e4)
static const float SPELL_AI_HASTE_MODIFIER = 0.33f;
DATA(0x0048a6e8)
static const float SPELL_AI_BLESS_MODIFIER = 0.18f;
DATA(0x0048a6ec)
static const float SPELL_AI_PROTECTION_MODIFIER = 0.24f;
DATA(0x0048a6f0)
static const float SPELL_AI_ANTI_MAGIC_MODIFIER = 0.15f;

// The weaker side's hero halves (or quarters) a spell's raw effect.
DATA(0x004cccb4)
i32 gSpellAIEffectShift;
// Side of the stack standing on the hex DetermineEffectOfSpell evaluates.
DATA(0x004cccb0)
H1_ENUM_STORAGE(CombatSide, i32) gSpellAITargetSide;

// Heroes memorize spells with charges.
#define bestHex bestHexWork // frame-slot spelling
VA(0x00458de0, 0x196)
b32 combatManager::DoSpellAI(H1_ENUM_PARAM(CombatSide, i8) side) {
    H1_ENUM_LOCAL(SpellType, i32) chosenSpell;
    i32 bestValue;
    i32 effect;
    i32 slot;
    i32 bestHex;
    i32 hex;

    bestValue = 0;
    chosenSpell = SPELL_NONE;
    bestHex = -1;
    if (m_heroes[side] == NULL)
        return false;
    if (m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == HERO_SPELL_POWER_ONE)
        gSpellAIEffectShift = 2;
    else if (m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER]
             == HERO_SPELL_POWER_TWO)
        gSpellAIEffectShift = 1;
    else
        gSpellAIEffectShift = 0;
    for (slot = 0; slot < HERO_SPELL_SLOT_COUNT; slot++) {
        if (m_heroes[side]->m_spells[slot] >= SPELL_FIRST
            && (gSpellAIFlags[m_heroes[side]->m_spells[slot]] & SPELL_AI_FLAG_COMBAT)
            && m_heroes[side]->m_spellCharges[slot] > 0) {
            DetermineEffectOfSpell(m_heroes[side]->m_spells[slot], &effect, &hex);
            if (effect > bestValue) {
                bestValue = effect;
                chosenSpell = m_heroes[side]->m_spells[slot];
                bestHex = hex;
            }
        }
    }
    if (bestValue > 0) {
        gNextAction = ACTION_CAST_SPELL;
        gNextActionExtra = H1_ENUM_ENCODE(SpellType, chosenSpell);
        gNextActionGridIndex = bestHex;
        return true;
    }
    return false;
}
#undef bestHex

// Each of the nineteen combat spells is scored once, across the area
// grid, or over one side's stacks.
#define done bDone // frame-slot spelling
VA(0x00458f76, 0x42d)
void combatManager::DetermineEffectOfSpell(
    H1_ENUM_PARAM(SpellType, i32) spell,
    i32* bestEffect,
    i32* bestHex
) {
    H1_ENUM_LOCAL(CombatSide, i32) side;
    i32 firstDurMax;
    b32 done;
    i32 effect;
    army* targetCreature;
    H1_ENUM_LOCAL(CombatSpellAITargetMode, i32) spellMode;
    i32 hex;

    done = false;
    side = COMBAT_DEFENDER_SIDE;
    hex = COMBAT_SPELL_AI_HEX_FIRST;
    effect = 0;
    targetCreature = NULL;
    *bestEffect = 0;
    switch (spell) {
        case SPELL_CURE:
        case SPELL_DISPEL_MAGIC:
        case SPELL_ARMAGEDDON:
        case SPELL_STORM:
            spellMode = SPELL_AI_GLOBAL;
            break;
        case SPELL_FIREBALL:
        case SPELL_METEOR_SHOWER:
            spellMode = SPELL_AI_AREA;
            break;
        case SPELL_TELEPORT:
        case SPELL_RESURRECT:
        case SPELL_HASTE:
        case SPELL_BLESS:
        case SPELL_PROTECTION:
        case SPELL_ANTI_MAGIC:
            spellMode = SPELL_AI_FRIENDLY;
            side = m_currentSide;
            break;
        case SPELL_LIGHTNING_BOLT:
        case SPELL_SLOW:
        case SPELL_BLIND:
        case SPELL_CURSE:
        case SPELL_TURN_UNDEAD:
        case SPELL_BERZERKER:
        case SPELL_PARALYZE:
            spellMode = SPELL_AI_ENEMY;
            side = COMBAT_OPPOSING_SIDE(m_currentSide);
            break;
        default:
            *bestEffect = 0;
            return;
    }
    if (spellMode == SPELL_AI_FRIENDLY || spellMode == SPELL_AI_ENEMY)
        done = FirstArmy(COMBAT_SPELL_AI_HEX_FIRST, side, &hex);
    while (!done) {
        if (m_hexCells[hex].m_occupantIndex >= 0) {
            targetCreature =
                &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
            gSpellAITargetSide = m_hexCells[hex].m_occupantSide;
        }
        switch (spell) {
            case SPELL_CURE:
                EffectSpellCure(&effect, m_currentSide, true);
                break;
            case SPELL_DISPEL_MAGIC:
                EffectSpellCure(&effect, COMBAT_SIDE_ANY, false);
                break;
            case SPELL_RESURRECT:
                EffectSpellResurrect(&effect, hex);
                break;
            case SPELL_ARMAGEDDON:
                EffectSpellDamage(&effect, spell, 50, hex);
                break;
            case SPELL_STORM:
                EffectSpellDamage(&effect, spell, 25, hex);
                break;
            case SPELL_FIREBALL:
                EffectSpellDamage(&effect, spell, 10, hex);
                break;
            case SPELL_METEOR_SHOWER:
                EffectSpellDamage(&effect, spell, 25, hex);
                break;
            case SPELL_LIGHTNING_BOLT:
                EffectSpellDamage(&effect, spell, 25, hex);
                break;
            case SPELL_HASTE:
            case SPELL_BLESS:
            case SPELL_PROTECTION:
            case SPELL_ANTI_MAGIC:
                if (spell == SPELL_ANTI_MAGIC
                    && m_heroes[COMBAT_OPPOSING_SIDE(m_currentSide)] == NULL)
                    effect = 0;
                else
                    effect = RawEffectSpellInfluence(targetCreature, spell) >> gSpellAIEffectShift;
                if (targetCreature->m_spellEffect >= SPELL_FIRST)
                    effect -=
                        RawEffectSpellInfluence(targetCreature, targetCreature->m_spellEffect);
                break;
            case SPELL_SLOW:
            case SPELL_BLIND:
            case SPELL_CURSE:
            case SPELL_BERZERKER:
            case SPELL_PARALYZE:
                effect = -(RawEffectSpellInfluence(targetCreature, spell) >> gSpellAIEffectShift);
                if (targetCreature->m_spellEffect >= SPELL_FIRST)
                    effect +=
                        RawEffectSpellInfluence(targetCreature, targetCreature->m_spellEffect);
                break;
            case SPELL_TELEPORT:
                effect = 0;
                break;
            case SPELL_TURN_UNDEAD:
                if (targetCreature->m_creatureType == CREATURE_GHOST)
                    effect = targetCreature->m_quantity
                             * gMonsterDatabase[targetCreature->m_creatureType].fightValue;
                else
                    effect = 0;
                break;
            default:
                *bestEffect = 0;
                return;
        }
        if (effect > *bestEffect) {
            *bestEffect = effect;
            *bestHex = hex;
        }
        switch (spellMode) {
            case SPELL_AI_GLOBAL:
                done = true;
                break;
            case SPELL_AI_FRIENDLY:
            case SPELL_AI_ENEMY:
                done = FirstArmy(hex + 1, side, &hex);
                break;
            case SPELL_AI_AREA:
                NextPos(&hex);
                if (hex > COMBAT_SPELL_AI_HEX_LAST)
                    done = true;
                break;
        }
    }
}
#undef done

// A spell's value as a share of the stack's fight value.
VA(0x004593a3, 0x246)
i32 combatManager::RawEffectSpellInfluence(army* target, H1_ENUM_PARAM(SpellType, i32) spell) {
    i32 worth;
    i32 effect;

    effect = 0;
    worth = target->m_quantity * gMonsterDatabase[target->m_creatureType].fightValue;
    switch (spell) {
        case SPELL_SLOW:
            if (target->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                effect = 0;
            else if (target->m_stats.attributes & MONSTER_FLAGS_FLYING)
                effect = worth * SPELL_AI_SLOW_MODIFIER * 3.0f;
            else
                effect = worth * SPELL_AI_SLOW_MODIFIER * (target->m_stats.speed - 1);
            break;
        case SPELL_BLIND:
            effect = worth * SPELL_AI_BLIND_MODIFIER;
            break;
        case SPELL_CURSE:
            effect = worth * SPELL_AI_CURSE_MODIFIER;
            break;
        case SPELL_BERZERKER:
            effect = worth * SPELL_AI_BERSERK_MODIFIER;
            break;
        case SPELL_PARALYZE:
            effect = worth * SPELL_AI_PARALYZE_MODIFIER;
            break;
        case SPELL_HASTE:
            if (target->m_stats.attributes & MONSTER_FLAGS_FLYING)
                effect = 0;
            else if (target->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                effect = 0;
            else if (H1_ENUM_DECODE(CreatureSpeed, target->m_stats.speed) < CREATURE_SPEED_SLOW_END)
                effect = worth * SPELL_AI_HASTE_MODIFIER;
            else if (H1_ENUM_DECODE(CreatureSpeed, target->m_stats.speed)
                     < CREATURE_SPEED_MEDIUM_END)
                effect = worth * SPELL_AI_HASTE_MODIFIER / 2.0f;
            else
                effect = 0;
            break;
        case SPELL_BLESS:
            effect = worth * SPELL_AI_BLESS_MODIFIER;
            break;
        case SPELL_PROTECTION:
            effect = worth * SPELL_AI_PROTECTION_MODIFIER;
            break;
        case SPELL_ANTI_MAGIC:
            effect = worth * SPELL_AI_ANTI_MAGIC_MODIFIER;
            break;
        default:
            effect = 0;
            break;
    }
    if (ARMY_IGNORES_SPELLS(target))
        effect = 0;
    else if (target->m_creatureType == CREATURE_DWARF && effect < 0)
        effect = effect * 0.75;
    return effect;
}

VA(0x004595e9, 0x52)
void combatManager::ClearEffects(void) {
    H1_ENUM_LOCAL(CombatSide, i32) side;
    i32 armyIndex;
    for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; ++side) {
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; ++armyIndex)
            gArmyEffected[side][armyIndex] = false;
    }
}

// Rows are nine hexes wide.
VA(0x0045963b, 0x3d)
void combatManager::NextPos(i32* hex) {
    if ((*hex + COMBAT_SPELL_AI_ROW_END_OFFSET) % COMBAT_GRID_COLUMNS == 0)
        *hex += COMBAT_SPELL_AI_ROW_SKIP;
    else
        (*hex)++;
}

// The next hex at or after startHex holding a stack of the side
// (COMBAT_SIDE_ANY: either side).
VA(0x00459678, 0x66)
b32 combatManager::FirstArmy(i32 startHex, H1_ENUM_PARAM(CombatSide, i32) side, i32* hex) {
    while (startHex <= COMBAT_SPELL_AI_HEX_LAST) {
        if (m_hexCells[startHex].m_occupantSide == side
            || (side == COMBAT_SIDE_ANY
                && m_hexCells[startHex].m_occupantSide >= COMBAT_SIDE_FIRST)) {
            *hex = startHex;
            return false;
        }
        NextPos(&startHex);
    }
    *hex = ARMY_HEX_INVALID;
    return true;
}

// The value of cancelling a side's (COMBAT_SIDE_ANY: both
// sides') spell effects; stacks
// carry a single effect.
VA(0x004596de, 0x20c)
void combatManager::EffectSpellCure(
    i32* effect,
    H1_ENUM_PARAM(CombatSide, i32) targetSide,
    b8 cureOnly
) {
    H1_ENUM_LOCAL(CombatSide, i32) sideIndex;
    i32 negTotal;
    i32 stackNum;
    army* creature;
    i32 posTotal;
    b32 done;
    i32 armyWorth;

    *effect = 0;
    done = false;
    if (targetSide == COMBAT_SIDE_ANY)
        sideIndex = m_currentSide;
    else
        sideIndex = targetSide;
    while (!done) {
        negTotal = 0;
        posTotal = 0;
        for (stackNum = 0; stackNum < ARMY_GROUP_SLOT_COUNT; stackNum++) {
            if (m_armies[sideIndex][stackNum].IsAlive()) {
                creature = &m_armies[sideIndex][stackNum];
                armyWorth =
                    creature->m_quantity * gMonsterDatabase[creature->m_creatureType].fightValue;
                switch (creature->m_spellEffect) {
                    case SPELL_SLOW:
                    case SPELL_BLIND:
                    case SPELL_CURSE:
                    case SPELL_BERZERKER:
                    case SPELL_PARALYZE:
                        negTotal += -RawEffectSpellInfluence(creature, creature->m_spellEffect);
                        break;
                    case SPELL_HASTE:
                    case SPELL_BLESS:
                    case SPELL_PROTECTION:
                    case SPELL_ANTI_MAGIC:
                        posTotal += RawEffectSpellInfluence(creature, creature->m_spellEffect);
                        break;
                }
            }
        }
        if (cureOnly == true)
            posTotal = 0;
        if (targetSide == COMBAT_SIDE_ANY) {
            if (sideIndex == m_currentSide)
                *effect += negTotal - posTotal;
            else
                *effect += posTotal - negTotal;
        } else
            *effect += negTotal;
        if (targetSide == COMBAT_SIDE_ANY && sideIndex == m_currentSide)
            sideIndex = COMBAT_OPPOSING_SIDE(m_currentSide);
        else
            done = true;
    }
}

// The fight value Resurrect would restore to the stack on hex.
VA(0x004598ea, 0xd4)
void combatManager::EffectSpellResurrect(i32* effect, i32 hex) {
    army* target;
    i32 resurrectPower;
    i32 count;

    target = &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
    if (ARMY_IGNORES_SPELLS(target)) {
        *effect = 0;
        return;
    }
    count = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50
            / target->m_stats.hitPoints;
    if (count + target->m_quantity > target->m_initialQuantity)
        count = target->m_initialQuantity - target->m_quantity;
    *effect = count * gMonsterDatabase[target->m_creatureType].fightValue;
}

// The net fight value a damage spell destroys, or a decisive value when it
// wipes out a side.
#define expectedDamage hitDamage // frame-slot spelling
VA(0x004599be, 0x491)
void combatManager::EffectSpellDamage(
    i32* effect,
    H1_ENUM_PARAM(SpellType, i32) spell,
    i32 damagePerPower,
    i32 targetHex
) {
    i32 baseDamage;
    i32 leftDamage;
    H1_ENUM_ARRAY(i32, stacksKilled, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i32, killedValue, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_LOCAL(CombatSide, i32) side;
    i32 expectedDamage;
    army* targetArmy;
    i32 hex;
    b32 done;
    H1_ENUM_ARRAY(i32, damagedValue, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_LOCAL(CombatHexDirection, i32) neighborIndex;
    i32 killedCount;

    baseDamage = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * damagePerPower;
    hex = 0;
    neighborIndex = COMBAT_DIRECTION_NORTHEAST;
    done = false;
    if (m_hexCells[targetHex].m_occupantIndex >= 0)
        targetArmy =
            &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
    for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; side++) {
        stacksKilled[side] = 0;
        damagedValue[side] = 0;
        killedValue[side] = 0;
    }
    ClearEffects();
    while (!done) {
        switch (spell) {
            case SPELL_ARMAGEDDON:
            case SPELL_STORM:
                NextPos(&hex);
                done = hex > COMBAT_SPELL_AI_HEX_LAST;
                break;
            case SPELL_FIREBALL:
            case SPELL_METEOR_SHOWER:
                if (neighborIndex < COMBAT_DIRECTION_ADJACENT_COUNT) {
                    hex = GetAdjacentCellIndexNoArmy(targetHex, neighborIndex);
                    neighborIndex++;
                } else
                    done = true;
                break;
            case SPELL_LIGHTNING_BOLT:
                if (hex == targetHex)
                    done = true;
                else
                    hex = targetHex;
                break;
        }
        if (!done && m_hexCells[hex].m_occupantIndex >= 0
            && m_hexCells[hex].m_occupantSide >= COMBAT_SIDE_FIRST) {
            targetArmy = &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
            if (targetArmy->m_stats.hitPoints > 0
                && !gArmyEffected[m_hexCells[hex].m_occupantSide]
                                 [m_hexCells[hex].m_occupantIndex]) {
                gArmyEffected[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex] =
                    true;
                if (!ARMY_IGNORES_SPELLS(targetArmy)) {
                    if (targetArmy->m_creatureType == CREATURE_DWARF)
                        expectedDamage = baseDamage * 0.75;
                    else
                        expectedDamage = baseDamage;
                    killedCount = expectedDamage / targetArmy->m_stats.hitPoints;
                    leftDamage = expectedDamage % targetArmy->m_stats.hitPoints;
                    if (leftDamage + targetArmy->m_hitPointsLost >= targetArmy->m_stats.hitPoints) {
                        killedCount++;
                        leftDamage -= targetArmy->m_stats.hitPoints - targetArmy->m_hitPointsLost;
                    }
                    if (killedCount >= targetArmy->m_quantity) {
                        killedCount = targetArmy->m_quantity;
                        leftDamage = 0;
                        stacksKilled[m_hexCells[hex].m_occupantSide]++;
                    }
                    damagedValue[m_hexCells[hex].m_occupantSide] +=
                        (killedCount * targetArmy->m_stats.hitPoints + leftDamage * 0.75)
                        * gMonsterDatabase[targetArmy->m_creatureType].fightValue
                        / targetArmy->m_stats.hitPoints;
                    killedValue[m_hexCells[hex].m_occupantSide] +=
                        killedCount * targetArmy->m_stats.hitPoints
                        * gMonsterDatabase[targetArmy->m_creatureType].fightValue
                        / targetArmy->m_stats.hitPoints;
                }
            }
        }
    }
    if (stacksKilled[COMBAT_DEFENDER_SIDE] >= m_numArmies[COMBAT_DEFENDER_SIDE]
        || stacksKilled[COMBAT_ATTACKER_SIDE] >= m_numArmies[COMBAT_ATTACKER_SIDE]) {
        if (killedValue[m_currentSide] <= 0)
            *effect = 100000000 - gSpellAIValue[spell];
        else
            *effect = killedValue[COMBAT_OPPOSING_SIDE(m_currentSide)] - killedValue[m_currentSide];
    } else
        *effect = damagedValue[COMBAT_OPPOSING_SIDE(m_currentSide)] - damagedValue[m_currentSide];
}
#undef expectedDamage
