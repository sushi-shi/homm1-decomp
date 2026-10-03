// HoMM1 SPELLAI: combat spell selection for computer-controlled heroes.
// Retail int3 padding opens this object at 0x00437010; Buka 2.1
// SOURCE/SPELLAI.cpp supplies the family (DoSpellAI, DetermineEffectOfSpell,
// RawEffectSpellInfluence, ClearEffects, NextPos, FirstArmy, EffectSpell*).

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/PATH.h>

#include <stdlib.h>

// Per-spell weights of a stack's fight value (Buka keeps the same named
// float constants for its larger spell list).
static const float SPELL_AI_SLOW_MODIFIER = -0.11f;
static const float SPELL_AI_BLIND_MODIFIER = -0.6f;
static const float SPELL_AI_CURSE_MODIFIER = -0.18f;
static const float SPELL_AI_PARALYZE_MODIFIER = -0.6f;
static const float SPELL_AI_BERSERK_MODIFIER = -0.7f;
static const float SPELL_AI_HASTE_MODIFIER = 0.33f;
static const float SPELL_AI_BLESS_MODIFIER = 0.18f;
static const float SPELL_AI_STONESKIN_MODIFIER = 0.24f;
static const float SPELL_AI_SHIELD_MODIFIER = 0.15f;

// The weaker side's hero halves (or quarters) a spell's raw effect.
DATA(0x004c50c4)
int giSpellAIEffectShift;
// Side of the stack standing on the hex DetermineEffectOfSpell evaluates.
DATA(0x004c50c8)
int giSpellAITargetSide;

// Buka SPELLAI.cpp:69-139; HoMM1 heroes memorize spells with charges.
VA(0x00437010, 0x1bd)
int combatManager::DoSpellAI(signed char side) {
    int selectedSpell;
    int bestEffect;
    int spellEffect;
    int bestHexWork;
    int slotIndex;
    int candHex;

    bestEffect = 0;
    selectedSpell = SPELL_NONE;
    bestHexWork = -1;
    if (m_heroes[side] == NULL)
        return 0;
    if (m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 1)
        giSpellAIEffectShift = 2;
    else if (m_heroes[m_currentSide]->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 2)
        giSpellAIEffectShift = 1;
    else
        giSpellAIEffectShift = 0;
    for (slotIndex = 0; slotIndex < HERO_SPELL_SLOT_COUNT; slotIndex++) {
        if (m_heroes[side]->m_spells[slotIndex] >= 0
            && (gcSpellAIFlags[m_heroes[side]->m_spells[slotIndex]] & SPELL_AI_FLAG_COMBAT)
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

// clang-format off
// DetermineEffectOfSpell's target walk (Buka 2.1 SPELLAI.cpp
// CombatSpellAITargetMode; HoMM1 numbers its four modes in this order): one
// global evaluation, every area position, or each friendly / enemy stack.
H1_ENUM_BEGIN(CombatSpellAITargetMode)
    SPELL_AI_GLOBAL = 0,
    SPELL_AI_AREA = 1,
    SPELL_AI_FRIENDLY = 2,
    SPELL_AI_ENEMY = 3
H1_ENUM_END(CombatSpellAITargetMode)
// clang-format on

// Buka SPELLAI.cpp:141-733 reduced to HoMM1's nineteen combat spells: each
// spell is scored once, across the area grid, or over one side's stacks.
VA(0x004371cd, 0x4b4)
void combatManager::DetermineEffectOfSpell(int spell, int* bestEffect, int* bestHex) {
    int spellEffect;
    int durMax;
    int bDone;
    int side;
    army* target;
    int spellMode;
    int curHex;

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
            giSpellAITargetSide = m_hexCells[curHex].m_occupantSide;
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
                    spellEffect = RawEffectSpellInfluence(target, spell) >> giSpellAIEffectShift;
                if (target->m_spellEffect >= 0)
                    spellEffect -= RawEffectSpellInfluence(target, target->m_spellEffect);
                break;
            case SPELL_SLOW:
            case SPELL_BLIND:
            case SPELL_CURSE:
            case SPELL_BERZERKER:
            case SPELL_PARALYZE:
                spellEffect = -(RawEffectSpellInfluence(target, spell) >> giSpellAIEffectShift);
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
VA(0x00437681, 0x2f6)
int combatManager::RawEffectSpellInfluence(army* target, int spell) {
    int stackValue;
    int effect;

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
VA(0x00437977, 0x63)
void combatManager::ClearEffects(void) {
    int side;
    int index;
    for (side = 0; side < COMBAT_SIDE_COUNT; ++side) {
        for (index = 0; index < ARMY_GROUP_SLOT_COUNT; ++index)
            gArmyEffected[side][index] = 0;
    }
}

// Buka 2.1 NextPos with HoMM1's retail-backed nine-hex row width.
VA(0x004379da, 0x40)
void combatManager::NextPos(int* hex) {
    if ((*hex + COMBAT_SPELL_AI_ROW_END_OFFSET) % COMBAT_GRID_COLUMNS == 0)
        *hex += COMBAT_SPELL_AI_ROW_SKIP;
    else
        (*hex)++;
}

// Buka SPELLAI.cpp:983-995: the next hex at or after startHex holding a
// stack of the side (2: either side).
VA(0x00437a1a, 0x87)
int combatManager::FirstArmy(int startHex, int side, int* hex) {
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
VA(0x00437aa1, 0x273)
void combatManager::EffectSpellCure(int* effect, int targetSide, signed char cure) {
    int curSide;
    int negEffect;
    int index;
    army* armyPtr;
    int posEffect;
    int finished;
    int fightValue;

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
VA(0x00437d14, 0xf9)
void combatManager::EffectSpellResurrect(int* effect, int hex) {
    army* targetArmy;
    int resurrectPower;
    int num;

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
VA(0x00437e0d, 0x501)
void combatManager::EffectSpellDamage(int* effect, int spell, int damagePerPower, int targetHex) {
    int partValue[COMBAT_SIDE_COUNT];
    int killed;
    int stacksKilled[COMBAT_SIDE_COUNT];
    int extra;
    int finished;
    army* targetCreature;
    int combatValue[COMBAT_SIDE_COUNT];
    int side;
    int hitDamage;
    int cell;
    int power;
    int dir;

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
            *effect = 100000000 - giSpellAIValue[spell];
        else
            *effect = combatValue[1 - m_currentSide] - combatValue[m_currentSide];
    } else
        *effect = partValue[1 - m_currentSide] - partValue[m_currentSide];
}
