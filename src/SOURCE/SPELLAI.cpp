// HoMM1 SPELLAI: combat spell selection for computer-controlled heroes.
// Retail int3 padding opens this object at 0x00437010; Buka 2.1
// SOURCE/SPELLAI.cpp supplies the family (DoSpellAI, DetermineEffectOfSpell,
// RawEffectSpellInfluence, ClearEffects, NextPos, FirstArmy, EffectSpell*).

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

// Per-spell weights of a stack's fight value (Buka keeps the same named
// float constants for its larger spell list).
DATA(0x0048d500)
static const float SPELL_AI_SLOW_MODIFIER = -0.11f;
DATA(0x0048d504)
static const float SPELL_AI_BLIND_MODIFIER = -0.6f;
DATA(0x0048d508)
static const float SPELL_AI_CURSE_MODIFIER = -0.18f;
DATA(0x0048d50c)
static const float SPELL_AI_PARALYZE_MODIFIER = -0.6f;
DATA(0x0048d510)
static const float SPELL_AI_BERSERK_MODIFIER = -0.7f;
DATA(0x0048d514)
static const float SPELL_AI_HASTE_MODIFIER = 0.33f;
DATA(0x0048d518)
static const float SPELL_AI_BLESS_MODIFIER = 0.18f;
DATA(0x0048d51c)
static const float SPELL_AI_STONESKIN_MODIFIER = 0.24f;
DATA(0x0048d520)
static const float SPELL_AI_SHIELD_MODIFIER = 0.15f;

// The weaker side's hero halves (or quarters) a spell's raw effect.
DATA(0x004cccb4)
i32 gSpellAIEffectShift;
// Side of the stack standing on the hex DetermineEffectOfSpell evaluates.
DATA(0x004cb17c)
i32 gSpellAITargetSide;

// Buka SPELLAI.cpp:69-139; HoMM1 heroes memorize spells with charges.
VA(0x00458de0, 0x196)
i32 combatManager::DoSpellAI(i8 side) {
    i32 selectedSpell;
    i32 bestEffect;
    i32 spellEffect;
    i32 bestHexWork;
    i32 slotIndex;
    i32 candHex;

    bestEffect = 0;
    selectedSpell = SPELL_NONE;
    bestHexWork = -1;
    if (m_heroes[side] == NULL)
        return 0;
    if (m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 1)
        gSpellAIEffectShift = 2;
    else if (m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 2)
        gSpellAIEffectShift = 1;
    else
        gSpellAIEffectShift = 0;
    for (slotIndex = 0; slotIndex < HERO_SPELL_SLOT_COUNT; slotIndex++) {
        if (m_heroes[side]->m_spells[slotIndex] >= 0
            && (gSpellAIFlags[m_heroes[side]->m_spells[slotIndex]] & SPELL_AI_FLAG_COMBAT)
            && m_heroes[side]->m_spellCharges[slotIndex] > 0) {
            DetermineEffectOfSpell(m_heroes[side]->m_spells[slotIndex], &spellEffect, &candHex);
            if (spellEffect > bestEffect) {
                bestEffect = spellEffect;
                selectedSpell = m_heroes[side]->m_spells[slotIndex];
                bestHexWork = candHex;
            }
        }
    }
    if (bestEffect > 0) {
        giNextAction = ACTION_CAST_SPELL;
        giNextActionExtra = selectedSpell;
        giNextActionGridIndex = bestHexWork;
        return 1;
    }
    return 0;
}

// DetermineEffectOfSpell's target walk (Buka 2.1 SPELLAI.cpp
// CombatSpellAITargetMode; HoMM1 numbers its four modes in this order): one
// global evaluation, every area position, or each friendly / enemy stack.
H1_ENUM_BEGIN(CombatSpellAITargetMode)
    SPELL_AI_GLOBAL = 0,
    SPELL_AI_AREA = 1,
    SPELL_AI_FRIENDLY = 2,
    SPELL_AI_ENEMY = 3
H1_ENUM_END(CombatSpellAITargetMode)

// Buka SPELLAI.cpp:141-733 reduced to HoMM1's nineteen combat spells: each
// spell is scored once, across the area grid, or over one side's stacks.
VA(0x00458f76, 0x42d)
void combatManager::DetermineEffectOfSpell(i32 spell, i32* bestEffect, i32* bestHex) {
    i32 spellEffect;
    i32 durMax;
    i32 bDone;
    i32 side;
    army* target;
    i32 spellMode;
    i32 curHex;

    bDone = 0;
    side = COMBAT_DEFENDER_SIDE;
    curHex = COMBAT_SPELL_AI_HEX_FIRST;
    spellEffect = 0;
    target = NULL;
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
            side = 1 - m_currentSide;
            break;
        default:
            *bestEffect = 0;
            return;
    }
    if (spellMode == SPELL_AI_FRIENDLY || spellMode == SPELL_AI_ENEMY)
        bDone = FirstArmy(COMBAT_SPELL_AI_HEX_FIRST, side, &curHex);
    while (!bDone) {
        if (m_hexCells[curHex].m_occupantIndex >= 0) {
            target =
                &m_armies[m_hexCells[curHex].m_occupantSide][m_hexCells[curHex].m_occupantIndex];
            gSpellAITargetSide = m_hexCells[curHex].m_occupantSide;
        }
        switch (spell) {
            case SPELL_CURE:
                EffectSpellCure(&spellEffect, m_currentSide, 1);
                break;
            case SPELL_DISPEL_MAGIC:
                EffectSpellCure(&spellEffect, COMBAT_SIDE_ANY, 0);
                break;
            case SPELL_RESURRECT:
                EffectSpellResurrect(&spellEffect, curHex);
                break;
            case SPELL_ARMAGEDDON:
                EffectSpellDamage(&spellEffect, spell, 50, curHex);
                break;
            case SPELL_STORM:
                EffectSpellDamage(&spellEffect, spell, 25, curHex);
                break;
            case SPELL_FIREBALL:
                EffectSpellDamage(&spellEffect, spell, 10, curHex);
                break;
            case SPELL_METEOR_SHOWER:
                EffectSpellDamage(&spellEffect, spell, 25, curHex);
                break;
            case SPELL_LIGHTNING_BOLT:
                EffectSpellDamage(&spellEffect, spell, 25, curHex);
                break;
            case SPELL_HASTE:
            case SPELL_BLESS:
            case SPELL_PROTECTION:
            case SPELL_ANTI_MAGIC:
                if (spell == SPELL_ANTI_MAGIC && m_heroes[1 - m_currentSide] == NULL)
                    spellEffect = 0;
                else
                    spellEffect = RawEffectSpellInfluence(target, spell) >> gSpellAIEffectShift;
                if (target->m_spellEffect >= 0)
                    spellEffect -= RawEffectSpellInfluence(target, target->m_spellEffect);
                break;
            case SPELL_SLOW:
            case SPELL_BLIND:
            case SPELL_CURSE:
            case SPELL_BERZERKER:
            case SPELL_PARALYZE:
                spellEffect = -(RawEffectSpellInfluence(target, spell) >> gSpellAIEffectShift);
                if (target->m_spellEffect >= 0)
                    spellEffect += RawEffectSpellInfluence(target, target->m_spellEffect);
                break;
            case SPELL_TELEPORT:
                spellEffect = 0;
                break;
            case SPELL_TURN_UNDEAD:
                if (target->m_creatureType == CREATURE_GHOST)
                    spellEffect =
                        gMonsterDatabase[target->m_creatureType].fightValue * target->m_quantity;
                else
                    spellEffect = 0;
                break;
            default:
                *bestEffect = 0;
                return;
        }
        if (*bestEffect < spellEffect) {
            *bestEffect = spellEffect;
            *bestHex = curHex;
        }
        switch (spellMode) {
            case SPELL_AI_GLOBAL:
                bDone = 1;
                break;
            case SPELL_AI_FRIENDLY:
            case SPELL_AI_ENEMY:
                bDone = FirstArmy(curHex + 1, side, &curHex);
                break;
            case SPELL_AI_AREA:
                NextPos(&curHex);
                if (curHex > COMBAT_SPELL_AI_HEX_LAST)
                    bDone = 1;
                break;
        }
    }
}

// Buka SPELLAI.cpp:802-960: a spell's value as a share of the stack's
// fight value.
VA(0x004593a3, 0x246)
i32 combatManager::RawEffectSpellInfluence(army* target, i32 spell) {
    i32 stackValue;
    i32 effect;

    effect = 0;
    stackValue = gMonsterDatabase[target->m_creatureType].fightValue * target->m_quantity;
    switch (spell) {
        case SPELL_SLOW:
            if (target->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                effect = 0;
            else if (target->m_stats.attributes & MONSTER_FLAGS_FLYING)
                effect = stackValue * SPELL_AI_SLOW_MODIFIER * 3.0f;
            else
                effect = (target->m_stats.speed - 1) * static_cast<float>(stackValue)
                         * SPELL_AI_SLOW_MODIFIER;
            break;
        case SPELL_BLIND:
            effect = stackValue * SPELL_AI_BLIND_MODIFIER;
            break;
        case SPELL_CURSE:
            effect = stackValue * SPELL_AI_CURSE_MODIFIER;
            break;
        case SPELL_BERZERKER:
            effect = stackValue * SPELL_AI_PARALYZE_MODIFIER;
            break;
        case SPELL_PARALYZE:
            effect = stackValue * SPELL_AI_BERSERK_MODIFIER;
            break;
        case SPELL_HASTE:
            if (target->m_stats.attributes & MONSTER_FLAGS_FLYING)
                effect = 0;
            else if (target->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                effect = 0;
            else if (target->m_stats.speed < CREATURE_SPEED_SLOW_END)
                effect = stackValue * SPELL_AI_HASTE_MODIFIER;
            else if (target->m_stats.speed < CREATURE_SPEED_MEDIUM_END)
                effect = stackValue * SPELL_AI_HASTE_MODIFIER / 2.0f;
            else
                effect = 0;
            break;
        case SPELL_BLESS:
            effect = stackValue * SPELL_AI_BLESS_MODIFIER;
            break;
        case SPELL_PROTECTION:
            effect = stackValue * SPELL_AI_STONESKIN_MODIFIER;
            break;
        case SPELL_ANTI_MAGIC:
            effect = stackValue * SPELL_AI_SHIELD_MODIFIER;
            break;
        default:
            effect = 0;
            break;
    }
    if (target->m_creatureType == CREATURE_DRAGON || target->m_spellEffect == SPELL_ANTI_MAGIC)
        effect = 0;
    else if (target->m_creatureType == CREATURE_DWARF && effect < 0)
        effect = effect * 0.75;
    return effect;
}

// Buka SPELLAI.cpp:962-973.
VA(0x004595e9, 0x52)
void combatManager::ClearEffects(void) {
    i32 side;
    i32 index;
    for (side = 0; side < COMBAT_SIDE_COUNT; ++side) {
        for (index = 0; index < ARMY_GROUP_SLOT_COUNT; ++index)
            gArmyEffected[side][index] = 0;
    }
}

// Buka 2.1 NextPos with HoMM1's retail-backed nine-hex row width.
VA(0x0045963b, 0x3d)
void combatManager::NextPos(i32* hex) {
    if ((*hex + COMBAT_SPELL_AI_ROW_END_OFFSET) % COMBAT_GRID_COLUMNS == 0)
        *hex += COMBAT_SPELL_AI_ROW_SKIP;
    else
        (*hex)++;
}

// Buka SPELLAI.cpp:983-995: the next hex at or after startHex holding a
// stack of the side (2: either side).
VA(0x00459678, 0x66)
i32 combatManager::FirstArmy(i32 startHex, i32 side, i32* hex) {
    while (startHex <= COMBAT_SPELL_AI_HEX_LAST) {
        if (m_hexCells[startHex].m_occupantSide == side
            || (side == COMBAT_SIDE_ANY && m_hexCells[startHex].m_occupantSide >= 0)) {
            *hex = startHex;
            return 0;
        }
        NextPos(&startHex);
    }
    *hex = ARMY_HEX_INVALID;
    return 1;
}

// Buka SPELLAI.cpp:1022-1136: the value of cancelling a side's (2: both
// sides') spell effects; HoMM1 stacks carry a single effect.
VA(0x004596de, 0x20c)
void combatManager::EffectSpellCure(i32* effect, i32 targetSide, i8 cure) {
    i32 curSide;
    i32 negEffect;
    i32 index;
    army* armyPtr;
    i32 posEffect;
    i32 finished;
    i32 fightValue;

    *effect = 0;
    finished = 0;
    if (targetSide == COMBAT_SIDE_ANY)
        curSide = m_currentSide;
    else
        curSide = targetSide;
    while (!finished) {
        negEffect = 0;
        posEffect = 0;
        for (index = 0; index < ARMY_GROUP_SLOT_COUNT; index++) {
            if (m_armies[curSide][index].IsAlive()) {
                armyPtr = &m_armies[curSide][index];
                fightValue =
                    gMonsterDatabase[armyPtr->m_creatureType].fightValue * armyPtr->m_quantity;
                switch (armyPtr->m_spellEffect) {
                    case SPELL_SLOW:
                    case SPELL_BLIND:
                    case SPELL_CURSE:
                    case SPELL_BERZERKER:
                    case SPELL_PARALYZE:
                        negEffect += -RawEffectSpellInfluence(armyPtr, armyPtr->m_spellEffect);
                        break;
                    case SPELL_HASTE:
                    case SPELL_BLESS:
                    case SPELL_PROTECTION:
                    case SPELL_ANTI_MAGIC:
                        posEffect += RawEffectSpellInfluence(armyPtr, armyPtr->m_spellEffect);
                        break;
                }
            }
        }
        if (cure == 1)
            posEffect = 0;
        if (targetSide == COMBAT_SIDE_ANY) {
            if (m_currentSide == curSide)
                *effect += negEffect - posEffect;
            else
                *effect += posEffect - negEffect;
        } else
            *effect += negEffect;
        if (targetSide == COMBAT_SIDE_ANY && m_currentSide == curSide)
            curSide = 1 - m_currentSide;
        else
            finished = 1;
    }
}

// Buka SPELLAI.cpp:1147-1166: the fight value Resurrect would restore to
// the stack on hex.
VA(0x004598ea, 0xd4)
void combatManager::EffectSpellResurrect(i32* effect, i32 hex) {
    army* targetArmy;
    i32 resurrectPower;
    i32 num;

    targetArmy = &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
    if (targetArmy->m_creatureType == CREATURE_DRAGON
        || targetArmy->m_spellEffect == SPELL_ANTI_MAGIC) {
        *effect = 0;
        return;
    }
    num = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * 50
          / targetArmy->m_stats.hitPoints;
    if (targetArmy->m_quantity + num > targetArmy->m_initialQuantity)
        num = targetArmy->m_initialQuantity - targetArmy->m_quantity;
    *effect = gMonsterDatabase[targetArmy->m_creatureType].fightValue * num;
}

// Buka SPELLAI.cpp:1183-1525: the net fight value a damage spell destroys,
// or a decisive value when it wipes out a side.
VA(0x004599be, 0x491)
void combatManager::EffectSpellDamage(i32* effect, i32 spell, i32 damagePerPower, i32 targetHex) {
    i32 partValue[COMBAT_SIDE_COUNT];
    i32 killed;
    i32 stacksKilled[COMBAT_SIDE_COUNT];
    i32 extra;
    i32 finished;
    army* targetCreature;
    i32 combatValue[COMBAT_SIDE_COUNT];
    i32 side;
    i32 hitDamage;
    i32 cell;
    i32 power;
    i32 dir;

    power = m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] * damagePerPower;
    cell = 0;
    dir = COMBAT_DIRECTION_NORTHEAST;
    finished = 0;
    if (m_hexCells[targetHex].m_occupantIndex >= 0)
        targetCreature =
            &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        stacksKilled[side] = 0;
        partValue[side] = 0;
        combatValue[side] = 0;
    }
    ClearEffects();
    while (!finished) {
        switch (spell) {
            case SPELL_ARMAGEDDON:
            case SPELL_STORM:
                NextPos(&cell);
                finished = cell > COMBAT_SPELL_AI_HEX_LAST;
                break;
            case SPELL_FIREBALL:
            case SPELL_METEOR_SHOWER:
                if (dir < COMBAT_DIRECTION_ADJACENT_COUNT) {
                    cell = GetAdjacentCellIndexNoArmy(targetHex, dir);
                    dir++;
                } else
                    finished = 1;
                break;
            case SPELL_LIGHTNING_BOLT:
                if (cell == targetHex)
                    finished = 1;
                else
                    cell = targetHex;
                break;
        }
        if (!finished && m_hexCells[cell].m_occupantIndex >= 0
            && m_hexCells[cell].m_occupantSide >= 0) {
            targetCreature =
                &m_armies[m_hexCells[cell].m_occupantSide][m_hexCells[cell].m_occupantIndex];
            if (targetCreature->m_stats.hitPoints > 0
                && !gArmyEffected[m_hexCells[cell].m_occupantSide]
                                 [m_hexCells[cell].m_occupantIndex]) {
                gArmyEffected[m_hexCells[cell].m_occupantSide][m_hexCells[cell].m_occupantIndex] =
                    1;
                if (targetCreature->m_creatureType != CREATURE_DRAGON
                    && targetCreature->m_spellEffect != SPELL_ANTI_MAGIC) {
                    if (targetCreature->m_creatureType == CREATURE_DWARF)
                        hitDamage = power * 0.75;
                    else
                        hitDamage = power;
                    killed = hitDamage / targetCreature->m_stats.hitPoints;
                    extra = hitDamage % targetCreature->m_stats.hitPoints;
                    if (extra + targetCreature->m_hitPointsLost
                        >= targetCreature->m_stats.hitPoints) {
                        killed++;
                        extra -=
                            targetCreature->m_stats.hitPoints - targetCreature->m_hitPointsLost;
                    }
                    if (targetCreature->m_quantity <= killed) {
                        killed = targetCreature->m_quantity;
                        extra = 0;
                        stacksKilled[m_hexCells[cell].m_occupantSide]++;
                    }
                    partValue[m_hexCells[cell].m_occupantSide] +=
                        (killed * targetCreature->m_stats.hitPoints + extra * 0.75)
                        * gMonsterDatabase[targetCreature->m_creatureType].fightValue
                        / targetCreature->m_stats.hitPoints;
                    combatValue[m_hexCells[cell].m_occupantSide] +=
                        gMonsterDatabase[targetCreature->m_creatureType].fightValue
                        * targetCreature->m_stats.hitPoints * killed
                        / targetCreature->m_stats.hitPoints;
                }
            }
        }
    }
    if (stacksKilled[COMBAT_DEFENDER_SIDE] >= m_numArmies[COMBAT_DEFENDER_SIDE]
        || stacksKilled[COMBAT_ATTACKER_SIDE] >= m_numArmies[COMBAT_ATTACKER_SIDE]) {
        if (combatValue[m_currentSide] <= 0)
            *effect = 100000000 - gSpellAIValue[spell];
        else
            *effect = combatValue[1 - m_currentSide] - combatValue[m_currentSide];
    } else
        *effect = partValue[1 - m_currentSide] - partValue[m_currentSide];
}
