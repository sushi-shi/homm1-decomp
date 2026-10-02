// HoMM1 SPELLAI: combat spell selection for computer-controlled heroes.
// Retail int3 padding opens this object at 0x00437010; Buka 2.1
// SOURCE/SPELLAI.cpp supplies the family (DoSpellAI, DetermineEffectOfSpell,
// RawEffectSpellInfluence, ClearEffects, NextPos, FirstArmy, EffectSpell*).

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/combatTypes.h>

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
int giSpellAIEffectShift;
// Side of the stack standing on the hex DetermineEffectOfSpell evaluates.
int giSpellAITargetSide;

// Buka SPELLAI.cpp:69-139; HoMM1 heroes memorize spells with charges.
VA(0x00437010, 0x1bd)
int combatManager::DoSpellAI(signed char side)
{
    int bestEffect;
    int selectedSpell;
    int bestHexWork;
    int slotIndex;
    int spellEffect;
    int candHex;

    bestEffect = 0;
    selectedSpell = -1;
    bestHexWork = -1;
    if (m_heroes[side] == NULL)
        return 0;
    if (m_heroes[m_currentSide]->m_primaryStats[2] == 1)
        giSpellAIEffectShift = 2;
    else if (m_heroes[m_currentSide]->m_primaryStats[2] == 2)
        giSpellAIEffectShift = 1;
    else
        giSpellAIEffectShift = 0;
    for (slotIndex = 0; slotIndex < HERO_SPELL_SLOT_COUNT; slotIndex++) {
        if (m_heroes[side]->m_spells[slotIndex] >= 0 && (gcSpellAIFlags[m_heroes[side]->m_spells[slotIndex]] & 2)
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
        giNextAction = 1;
        giNextActionExtra = selectedSpell;
        giNextActionGridIndex = bestHexWork;
        return 1;
    }
    return 0;
}

// Buka SPELLAI.cpp:141-733 reduced to HoMM1's nineteen combat spells: each
// spell is scored once, across the area grid, or over one side's stacks.
VA(0x004371cd, 0x4b4)
void combatManager::DetermineEffectOfSpell(int spell, int* bestEffect, int* bestHex)
{
    int spellEffect;
    int durMax;
    int bDone;
    int side;
    army* target;
    int spellMode;
    int curHex;

    bDone = 0;
    side = 0;
    curHex = 1;
    spellEffect = 0;
    target = NULL;
    *bestEffect = 0;
    switch (spell) {
    case 3:
    case 13:
    case 15:
    case 16:
        spellMode = 0;
        break;
    case 0:
    case 17:
        spellMode = 1;
        break;
    case 2:
    case 4:
    case 5:
    case 8:
    case 9:
    case 12:
        spellMode = 2;
        side = m_currentSide;
        break;
    case 1:
    case 6:
    case 7:
    case 10:
    case 11:
    case 14:
    case 18:
        spellMode = 3;
        side = 1 - m_currentSide;
        break;
    default:
        *bestEffect = 0;
        return;
    }
    if (spellMode == 2 || spellMode == 3)
        bDone = FirstArmy(1, side, &curHex);
    while (!bDone) {
        if (m_hexCells[curHex].m_occupantIndex >= 0) {
            target = &m_armies[m_hexCells[curHex].m_occupantSide][m_hexCells[curHex].m_occupantIndex];
            giSpellAITargetSide = m_hexCells[curHex].m_occupantSide;
        }
        switch (spell) {
        case 3:
            EffectSpellCure(&spellEffect, m_currentSide, 1);
            break;
        case 13:
            EffectSpellCure(&spellEffect, 2, 0);
            break;
        case 4:
            EffectSpellResurrect(&spellEffect, curHex);
            break;
        case 15:
            EffectSpellDamage(&spellEffect, spell, 50, curHex);
            break;
        case 16:
            EffectSpellDamage(&spellEffect, spell, 25, curHex);
            break;
        case 0:
            EffectSpellDamage(&spellEffect, spell, 10, curHex);
            break;
        case 17:
            EffectSpellDamage(&spellEffect, spell, 25, curHex);
            break;
        case 1:
            EffectSpellDamage(&spellEffect, spell, 25, curHex);
            break;
        case 5:
        case 8:
        case 9:
        case 12:
            if (spell == 12 && m_heroes[1 - m_currentSide] == NULL)
                spellEffect = 0;
            else
                spellEffect = RawEffectSpellInfluence(target, spell) >> giSpellAIEffectShift;
            if (target->m_spellEffect >= 0)
                spellEffect -= RawEffectSpellInfluence(target, target->m_spellEffect);
            break;
        case 6:
        case 7:
        case 10:
        case 14:
        case 18:
            spellEffect = -(RawEffectSpellInfluence(target, spell) >> giSpellAIEffectShift);
            if (target->m_spellEffect >= 0)
                spellEffect += RawEffectSpellInfluence(target, target->m_spellEffect);
            break;
        case 2:
            spellEffect = 0;
            break;
        case 11:
            if (target->m_creatureType == 0x1a)
                spellEffect = gMonsterDatabase[target->m_creatureType].fightValue * target->m_quantity;
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
        case 0:
            bDone = 1;
            break;
        case 2:
        case 3:
            bDone = FirstArmy(curHex + 1, side, &curHex);
            break;
        case 1:
            NextPos(&curHex);
            if (curHex > 0x2b)
                bDone = 1;
            break;
        }
    }
}

// Buka SPELLAI.cpp:802-960: a spell's value as a share of the stack's
// fight value.
VA(0x00437681, 0x2f6)
int combatManager::RawEffectSpellInfluence(army* target, int spell)
{
    int stackValue;
    int effect;

    effect = 0;
    stackValue = gMonsterDatabase[target->m_creatureType].fightValue * target->m_quantity;
    switch (spell) {
    case 6:
        if (target->m_stats.attributes & 4)
            effect = 0;
        else if (target->m_stats.attributes & 2)
            effect = stackValue * SPELL_AI_SLOW_MODIFIER * 3.0f;
        else
            effect = (target->m_stats.speed - 1) * (float)stackValue * SPELL_AI_SLOW_MODIFIER;
        break;
    case 7:
        effect = stackValue * SPELL_AI_BLIND_MODIFIER;
        break;
    case 10:
        effect = stackValue * SPELL_AI_CURSE_MODIFIER;
        break;
    case 14:
        effect = stackValue * SPELL_AI_PARALYZE_MODIFIER;
        break;
    case 18:
        effect = stackValue * SPELL_AI_BERSERK_MODIFIER;
        break;
    case 5:
        if (target->m_stats.attributes & 2)
            effect = 0;
        else if (target->m_stats.attributes & 4)
            effect = 0;
        else if (target->m_stats.speed < 2)
            effect = stackValue * SPELL_AI_HASTE_MODIFIER;
        else if (target->m_stats.speed < 3)
            effect = stackValue * SPELL_AI_HASTE_MODIFIER / 2.0f;
        else
            effect = 0;
        break;
    case 8:
        effect = stackValue * SPELL_AI_BLESS_MODIFIER;
        break;
    case 9:
        effect = stackValue * SPELL_AI_STONESKIN_MODIFIER;
        break;
    case 12:
        effect = stackValue * SPELL_AI_SHIELD_MODIFIER;
        break;
    default:
        effect = 0;
        break;
    }
    if (target->m_creatureType == 0x17 || target->m_spellEffect == 12)
        effect = 0;
    else if (target->m_creatureType == 0xd && effect < 0)
        effect = effect * 0.75;
    return effect;
}

// Buka SPELLAI.cpp:962-973.
VA(0x00437977, 0x63)
void combatManager::ClearEffects(void) {
    int side;
    int index;
    for (side = 0; side < COMBAT_EFFECT_SIDE_COUNT; ++side) {
        for (index = 0; index < COMBAT_EFFECT_SLOT_COUNT; ++index)
            gArmyEffected[side][index] = 0;
    }
}

// Buka 2.1 NextPos with HoMM1's retail-backed nine-hex row width.
VA(0x004379da, 0x40)
void combatManager::NextPos(int* hex) {
    if ((*hex + COMBAT_SPELL_AI_ROW_END_OFFSET) % COMBAT_SPELL_AI_ROW_LENGTH == 0)
        *hex += COMBAT_SPELL_AI_ROW_SKIP;
    else
        (*hex)++;
}

// Buka SPELLAI.cpp:983-995: the next hex at or after startHex holding a
// stack of the side (2: either side).
VA(0x00437a1a, 0x87)
int combatManager::FirstArmy(int startHex, int side, int* hex)
{
    while (startHex <= 0x2b) {
        if (m_hexCells[startHex].m_occupantSide == side
            || (side == 2 && m_hexCells[startHex].m_occupantSide >= 0)) {
            *hex = startHex;
            return 0;
        }
        NextPos(&startHex);
    }
    *hex = -1;
    return 1;
}

// Buka SPELLAI.cpp:1022-1136: the value of cancelling a side's (2: both
// sides') spell effects; HoMM1 stacks carry a single effect.
VA(0x00437aa1, 0x273)
void combatManager::EffectSpellCure(int* effect, int targetSide, signed char cure)
{
    int curSide;
    int index;
    army* armyPtr;
    int posEffect;
    int negEffect;
    int finished;
    int fightValue;

    *effect = 0;
    finished = 0;
    if (targetSide == 2)
        curSide = m_currentSide;
    else
        curSide = targetSide;
    while (!finished) {
        negEffect = 0;
        posEffect = 0;
        for (index = 0; index < 5; index++) {
            if (m_armies[curSide][index].IsAlive()) {
                armyPtr = &m_armies[curSide][index];
                fightValue = gMonsterDatabase[armyPtr->m_creatureType].fightValue * armyPtr->m_quantity;
                switch (armyPtr->m_spellEffect) {
                case 6:
                case 7:
                case 10:
                case 14:
                case 18:
                    negEffect += -RawEffectSpellInfluence(armyPtr, armyPtr->m_spellEffect);
                    break;
                case 5:
                case 8:
                case 9:
                case 12:
                    posEffect += RawEffectSpellInfluence(armyPtr, armyPtr->m_spellEffect);
                    break;
                }
            }
        }
        if (cure == 1)
            posEffect = 0;
        if (targetSide == 2) {
            if (m_currentSide == curSide)
                *effect += negEffect - posEffect;
            else
                *effect += posEffect - negEffect;
        } else
            *effect += negEffect;
        if (targetSide == 2 && m_currentSide == curSide)
            curSide = 1 - m_currentSide;
        else
            finished = 1;
    }
}

// Buka SPELLAI.cpp:1147-1166: the fight value Resurrect would restore to
// the stack on hex.
VA(0x00437d14, 0xf9)
void combatManager::EffectSpellResurrect(int* effect, int hex)
{
    army* targetArmy;
    int resurrectPower;
    int num;

    targetArmy = &m_armies[m_hexCells[hex].m_occupantSide][m_hexCells[hex].m_occupantIndex];
    if (targetArmy->m_creatureType == 0x17 || targetArmy->m_spellEffect == 12) {
        *effect = 0;
        return;
    }
    num = m_heroes[m_currentSide]->m_primaryStats[2] * 50 / targetArmy->m_stats.hitPoints;
    if (targetArmy->m_quantity + num > targetArmy->m_initialQuantity)
        num = targetArmy->m_initialQuantity - targetArmy->m_quantity;
    *effect = gMonsterDatabase[targetArmy->m_creatureType].fightValue * num;
}

// Buka SPELLAI.cpp:1183-1525: the net fight value a damage spell destroys,
// or a decisive value when it wipes out a side.
VA(0x00437e0d, 0x501)
void combatManager::EffectSpellDamage(int* effect, int spell, int damagePerPower, int targetHex)
{
    int partValue[2];
    int killed;
    int stacksKilled[2];
    int extra;
    int finished;
    army* targetCreature;
    int combatValue[2];
    int side;
    int hitDamage;
    int cell;
    int power;
    int dir;

    power = m_heroes[m_currentSide]->m_primaryStats[2] * damagePerPower;
    cell = 0;
    dir = 0;
    finished = 0;
    if (m_hexCells[targetHex].m_occupantIndex >= 0)
        targetCreature = &m_armies[m_hexCells[targetHex].m_occupantSide][m_hexCells[targetHex].m_occupantIndex];
    for (side = 0; side < 2; side++) {
        stacksKilled[side] = 0;
        partValue[side] = 0;
        combatValue[side] = 0;
    }
    ClearEffects();
    while (!finished) {
        switch (spell) {
        case 15:
        case 16:
            NextPos(&cell);
            finished = cell > 0x2b;
            break;
        case 0:
        case 17:
            if (dir < 6) {
                cell = GetAdjacentCellIndexNoArmy(targetHex, dir);
                dir++;
            } else
                finished = 1;
            break;
        case 1:
            if (cell == targetHex)
                finished = 1;
            else
                cell = targetHex;
            break;
        }
        if (!finished && m_hexCells[cell].m_occupantIndex >= 0 && m_hexCells[cell].m_occupantSide >= 0) {
            targetCreature = &m_armies[m_hexCells[cell].m_occupantSide][m_hexCells[cell].m_occupantIndex];
            if (targetCreature->m_stats.hitPoints > 0
                && !gArmyEffected[m_hexCells[cell].m_occupantSide][m_hexCells[cell].m_occupantIndex]) {
                gArmyEffected[m_hexCells[cell].m_occupantSide][m_hexCells[cell].m_occupantIndex] = 1;
                if (targetCreature->m_creatureType != 0x17 && targetCreature->m_spellEffect != 12) {
                    if (targetCreature->m_creatureType == 0xd)
                        hitDamage = power * 0.75;
                    else
                        hitDamage = power;
                    killed = hitDamage / targetCreature->m_stats.hitPoints;
                    extra = hitDamage % targetCreature->m_stats.hitPoints;
                    if (extra + targetCreature->m_hitPointsLost >= targetCreature->m_stats.hitPoints) {
                        killed++;
                        extra -= targetCreature->m_stats.hitPoints - targetCreature->m_hitPointsLost;
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
    if (stacksKilled[0] >= m_numArmies[0] || stacksKilled[1] >= m_numArmies[1]) {
        if (combatValue[m_currentSide] <= 0)
            *effect = 100000000 - giSpellAIValue[spell];
        else
            *effect = combatValue[1 - m_currentSide] - combatValue[m_currentSide];
    } else
        *effect = partValue[1 - m_currentSide] - partValue[m_currentSide];
}
