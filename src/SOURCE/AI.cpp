// HoMM1 Buka combat AI: game functions occupy RVAs 0x11660..0x132ee.
// VC6 locale startup follows at 0x132ee/0x13315; INT3 padding ends at 0x13330,
// where army::army begins.

#include <match.h>

#include <BASE/inputManager.h>
#include <BASE/mouseManager.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/hero.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/PATH.h>
#include <SOURCE/philAI.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compares the two sides' fight values, weighting the defender of a town
// and unspent stacks, against a chance raised by the hero's artifacts and
// experience.
VA(0x00411660, 0x728)
i32 combatManager::AICheckRetreat(void) {
    if (m_combatTowns[m_currentSide])
        return 0;
    if (!m_heroes[m_currentSide])
        return 0;
    if (!gGame->m_players[m_heroes[m_currentSide]->m_owner].m_townCount)
        return 0;

    hero heroRec;
    armyGroup* thatArmy;
    hero* curLeader;
    float retreatRatio;
    float prob;
    i32 realLoot;
    i32 armyIndex;
    H1_ENUM_LOCAL(CombatSide, i32) owner;
    armyGroup curGroup;
    H1_ENUM_ARRAY(i32, artifactTotals, CombatSide, COMBAT_SIDE_COUNT);
    float expBonus;
    H1_ENUM_ARRAY(i32, theForces, CombatSide, COMBAT_SIDE_COUNT);

    for (owner = COMBAT_SIDE_FIRST; owner < COMBAT_SIDE_COUNT; owner++) {
        if (m_heroes[owner]) {
            heroRec = *m_heroes[owner];
            curLeader = &heroRec;
            thatArmy = &curLeader->m_army;
        } else {
            thatArmy = &curGroup;
            curLeader = NULL;
        }
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
            if (m_armies[owner][armyIndex].IsAlive()) {
                thatArmy->m_creatureTypes[armyIndex] = m_armies[owner][armyIndex].m_creatureType;
                if (m_armies[owner][armyIndex].m_stats.attributes & MONSTER_FLAGS_TURN_SPENT)
                    thatArmy->m_creatureCounts[armyIndex] = m_armies[owner][armyIndex].m_quantity;
                else
                    thatArmy->m_creatureCounts[armyIndex] =
                        m_armies[owner][armyIndex].m_quantity * 1.2;
            } else {
                thatArmy->m_creatureTypes[armyIndex] = CREATURE_NONE;
                thatArmy->m_creatureCounts[armyIndex] = 0;
            }
        }
        theForces[owner] = gPhilAI->FightValueOfStack(thatArmy, curLeader, true);
        if (m_combatTowns[owner])
            theForces[owner] = theForces[owner] * 1.1;
        artifactTotals[owner] = 0;
        if (curLeader) {
            for (armyIndex = 0; armyIndex < HERO_ARTIFACT_SLOT_COUNT; armyIndex++) {
                if (ARTIFACT_HAS_BASE_VALUE(curLeader->m_artifacts[armyIndex]))
                    artifactTotals[owner] += gArtifactBaseRV[curLeader->m_artifacts[armyIndex]];
            }
        }
    }
    theForces[COMBAT_OPPOSING_SIDE(m_currentSide)] *= 1.1;
    realLoot = artifactTotals[m_currentSide];
    if (artifactTotals[m_currentSide] < COMBAT_AI_MIN_ARTIFACT_VALUE)
        return 0;
    prob = 0.16f;
    if (realLoot > COMBAT_AI_HIGH_ARTIFACT_VALUE)
        prob = prob + 0.06;
    else if (realLoot > COMBAT_AI_MEDIUM_ARTIFACT_VALUE)
        prob = prob + 0.05;
    else if (realLoot > 0)
        prob = prob + 0.04;
    if (theForces[m_currentSide] > COMBAT_AI_RETREAT_SCALED_PENALTY_THRESHOLD)
        prob -= theForces[m_currentSide] / COMBAT_AI_RETREAT_STRENGTH_DIVISOR;
    else if (theForces[m_currentSide] > COMBAT_AI_RETREAT_TIER_4_THRESHOLD)
        prob = prob - 0.08;
    else if (theForces[m_currentSide] > COMBAT_AI_RETREAT_TIER_3_THRESHOLD)
        prob = prob - 0.06;
    else if (theForces[m_currentSide] > COMBAT_AI_RETREAT_TIER_2_THRESHOLD)
        prob = prob - 0.04;
    else if (theForces[m_currentSide] > COMBAT_AI_RETREAT_TIER_1_THRESHOLD)
        prob = prob - 0.02;
    expBonus = m_heroes[m_currentSide]->m_experience / COMBAT_AI_EXPERIENCE_DIVISOR;
    if (expBonus > 0.03)
        expBonus = 0.03f;
    prob += expBonus;
    if (m_currentSide == COMBAT_ATTACKER_SIDE)
        prob = prob - 0.06;
    prob -=
        (COMBAT_AI_MAX_DIFFICULTY - gGame->m_players[m_heroes[m_currentSide]->m_owner].m_difficulty)
        * 0.03;
    retreatRatio =
        static_cast<float>(theForces[m_currentSide])
        / static_cast<double>(theForces[COMBAT_DEFENDER_SIDE] + theForces[COMBAT_ATTACKER_SIDE]);
    if (retreatRatio < prob) {
        gNextAction = ACTION_RETREAT;
        return 1;
    }
    return 0;
}

// Shooters shoot (adjacent enemies first), flyers and walkers attack by
// target class, walkers otherwise close in; a castle attacker steps toward
// the gate. The chosen move is nudged onto a free hex next to an enemy.
VA(0x00411d88, 0x872)
void combatManager::DoCompAI(H1_ENUM_PARAM(CombatSide, i8) side) {
    b8 theyOutshoot;
    H1_ENUM_ARRAY(i16, mainShooters, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_ARRAY(i32, newStrengths, CombatSide, COMBAT_SIDE_COUNT);
    i32 theSum;
    H1_ENUM_ARRAY(i16, walkerMask, CombatSide, COMBAT_SIDE_COUNT);
    H1_ENUM_LOCAL(CombatAIAttackPlan, i16) newPlan;
    // Counts the castle's built slots (building ids), then walks the
    // adjacent directions of the chosen move.
    H1_ENUM_SHARED(CombatHexDirection, i32) newDir;
    army* curArmy;
    H1_ENUM_LOCAL(CombatSide, i16) sideEnemy;
    i32 foeShootersNow;
    H1_ENUM_ARRAY(i16, flyerMask, CombatSide, COMBAT_SIDE_COUNT);
    i32 minShootPowerVal;
    i8 ndx;
    i32 localDummy;
    i32 myShootPower;
    b8 ourOutshoot;
    hexcell* tile;
    i32 wallStrength;
    town* castleCopy;
    i32 curNumArchers;
    i32 targetHexValue;
    i32 keptAdj;

    m_limitCreature = false;
    gMouseManager->ReallyHidePointer();
    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    newPlan = COMBAT_AI_ATTACK_NONE;
    sideEnemy = COMBAT_OPPOSING_SIDE(m_currentSide);
    mainShooters[m_currentSide] = GetShooterMask(m_currentSide);
    mainShooters[sideEnemy] = GetShooterMask(sideEnemy);
    flyerMask[m_currentSide] = GetFlyerMask(m_currentSide);
    flyerMask[sideEnemy] = GetFlyerMask(sideEnemy);
    walkerMask[m_currentSide] = GetWalkerMask(m_currentSide);
    walkerMask[sideEnemy] = GetWalkerMask(sideEnemy);
    newStrengths[m_currentSide] = GetStrength(m_currentSide, mainShooters[m_currentSide]);
    newStrengths[sideEnemy] = GetStrength(sideEnemy, mainShooters[sideEnemy]);
    theSum = GetStrength(
        m_currentSide,
        mainShooters[m_currentSide] | flyerMask[m_currentSide] | walkerMask[m_currentSide]
    );
    minShootPowerVal = (theSum + COMBAT_AI_STRENGTH_ROUNDING) / COMBAT_AI_STRENGTH_FRACTION;
    ourOutshoot = false;
    theyOutshoot = false;
    myShootPower = GetStrength(m_currentSide, mainShooters[m_currentSide]);
    foeShootersNow = GetStrength(sideEnemy, mainShooters[sideEnemy]);
    if (m_castleSide[COMBAT_DEFENDER_SIDE]) {
        curNumArchers = COMBAT_AI_CASTLE_BASE_ARCHERS;
        castleCopy = m_combatTowns[COMBAT_DEFENDER_SIDE];
        for (newDir = H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_FIRST);
             newDir <= H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_LAST);
             newDir++)
            if (castleCopy->m_buildings & (1 << newDir))
                curNumArchers += COMBAT_AI_CASTLE_ARCHERS_PER_DWELLING;
        for (newDir = H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD);
             newDir <= H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_RACE_FIRST - 1);
             newDir++)
            if (castleCopy->m_buildings & (1 << newDir))
                curNumArchers++;
        wallStrength = curNumArchers * COMBAT_AI_CASTLE_ARCHER_STRENGTH;
        if (m_currentSide == COMBAT_DEFENDER_SIDE)
            myShootPower += wallStrength;
        else
            foeShootersNow += wallStrength;
    }
    if ((theSum + COMBAT_AI_STRENGTH_ROUNDING) / COMBAT_AI_STRENGTH_FRACTION < myShootPower)
        ourOutshoot = true;
    if (foeShootersNow > myShootPower)
        theyOutshoot = true;
    if (curArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER) {
        if (curArmy->m_stats.shots > 0)
            newPlan = COMBAT_AI_ATTACK_SHOOT;
        else
            newPlan = COMBAT_AI_ATTACK_WALK;
    } else if (curArmy->m_stats.attributes & MONSTER_FLAGS_FLYING) {
        newPlan = COMBAT_AI_ATTACK_FLY;
    } else {
        newPlan = COMBAT_AI_ATTACK_WALK;
    }
    switch (newPlan) {
        case COMBAT_AI_ATTACK_SHOOT:
            if (AttemptAdjacentAttack(curArmy)) {
                goto finish;
            } else {
                ndx = GetBestArmy(sideEnemy, mainShooters[sideEnemy]);
                if (ndx != COMBAT_ARMY_INDEX_NONE) {
                    SET_NEXT_COMBAT_MOVE(m_armies[sideEnemy][ndx].m_hex);
                    goto finish;
                }
                ndx = GetBestArmy(sideEnemy, flyerMask[sideEnemy]);
                if (ndx != COMBAT_ARMY_INDEX_NONE) {
                    SET_NEXT_COMBAT_MOVE(m_armies[sideEnemy][ndx].m_hex);
                    goto finish;
                }
                if (walkerMask[sideEnemy]) {
                    ndx = GetClosestArmy(curArmy, sideEnemy, walkerMask[sideEnemy]);
                    if (ndx != COMBAT_ARMY_INDEX_NONE) {
                        SET_NEXT_COMBAT_MOVE(m_armies[sideEnemy][ndx].m_hex);
                        goto finish;
                    }
                }
            }
            break;
        case COMBAT_AI_ATTACK_FLY:
            if (ourOutshoot && !theyOutshoot) {
                if (AttemptAttack(curArmy, sideEnemy, mainShooters[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(curArmy, sideEnemy, flyerMask[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(curArmy, sideEnemy, walkerMask[sideEnemy]))
                    goto finish;
            } else {
                if (AttemptAttack(curArmy, sideEnemy, mainShooters[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(curArmy, sideEnemy, flyerMask[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(curArmy, sideEnemy, walkerMask[sideEnemy]))
                    goto finish;
            }
            break;
        case COMBAT_AI_ATTACK_WALK:
            if (AttemptAdjacentAttack(curArmy)) {
                goto finish;
            } else {
                if (ourOutshoot && !theyOutshoot) {
                    if (WalkTowardArmyFront(curArmy, m_currentSide, mainShooters[m_currentSide]))
                        goto finish;
                } else {
                    if (AttemptAttack(curArmy, sideEnemy, mainShooters[sideEnemy]))
                        goto finish;
                    else if (AttemptAttack(curArmy, sideEnemy, walkerMask[sideEnemy]))
                        goto finish;
                    else if (AttemptAttack(curArmy, sideEnemy, flyerMask[sideEnemy]))
                        goto finish;
                }
                if (WalkTowardArmy(curArmy, sideEnemy, mainShooters[sideEnemy]))
                    goto finish;
                else if (WalkTowardArmy(curArmy, sideEnemy, walkerMask[sideEnemy]))
                    goto finish;
                else if (WalkTowardArmy(curArmy, sideEnemy, flyerMask[sideEnemy]))
                    goto finish;
                if (m_currentSide == COMBAT_ATTACKER_SIDE && m_castleSide[COMBAT_DEFENDER_SIDE]
                    && curArmy->m_hex % COMBAT_GRID_COLUMNS < COMBAT_CASTLE_WALL_COLUMN - 1) {
                    targetHexValue = curArmy->m_hex / COMBAT_GRID_COLUMNS * COMBAT_GRID_COLUMNS
                                     + (COMBAT_CASTLE_WALL_COLUMN - 1);
                    tile = &gCombatManager->m_hexCells[targetHexValue];
                    if (ValidHex(targetHexValue) && tile->m_occupantSide == COMBAT_SIDE_NONE
                        && tile->m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
                        SET_NEXT_COMBAT_MOVE(targetHexValue);
                        goto finish;
                    }
                }
            }
            break;
    }
    gNextAction = ACTION_SKIP_TURN;
finish:
    if (gNextAction == ACTION_MOVE && gNextActionGridIndex > 0 && gNextActionGridIndex <= 43
        && gCombatManager->m_hexCells[gNextActionGridIndex].m_occupantSide == COMBAT_SIDE_NONE) {
        for (newDir = COMBAT_DIRECTION_ADJACENT_FIRST; newDir < COMBAT_DIRECTION_ADJACENT_COUNT;
             newDir++) {
            keptAdj = curArmy->GetAdjacentCellIndex(gNextActionGridIndex, newDir);
            if (keptAdj > 0 && keptAdj <= 43
                && gCombatManager->m_hexCells[keptAdj].m_occupantSide
                       == COMBAT_OPPOSING_SIDE(m_currentSide)) {
                gNextActionGridIndex = keptAdj;
                return;
            }
        }
    }
}

// Mask helpers: loop word indices over m_numArmies and build word masks
// (dead flag 0x10, shooter 4, flyer 2).
VA(0x004125fa, 0xb7)
i16 combatManager::GetShooterMask(H1_ENUM_PARAM(CombatSide, i8) side) {
    i16 armyIndex = 0;
    i16 armyBit = 1;
    class army* currentArmy;
    i16 bits = 0;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        currentArmy = &m_armies[side][armyIndex];
        if (currentArmy && !(currentArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)
            && (currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
            && currentArmy->m_stats.shots > 0)
            bits |= armyBit;
        armyBit <<= 1;
    }
    return bits;
}

VA(0x004126b1, 0xa9)
i16 combatManager::GetFlyerMask(H1_ENUM_PARAM(CombatSide, i8) side) {
    i16 armyIndex = 0;
    i16 bits;
    i16 armyBit = 1;
    class army* currentArmy;

    bits = 0;
    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        currentArmy = &m_armies[side][armyIndex];
        if (currentArmy && !(currentArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)
            && (currentArmy->m_stats.attributes & MONSTER_FLAGS_FLYING))
            bits |= armyBit;
        armyBit <<= 1;
    }
    return bits;
}

VA(0x0041275a, 0xc4)
i16 combatManager::GetWalkerMask(H1_ENUM_PARAM(CombatSide, i8) side) {
    i16 armyIndex = 0;
    i16 armyBit = 1;
    i16 bits = 0;
    class army* currentArmy;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        currentArmy = &m_armies[side][armyIndex];
        if (currentArmy && !(currentArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)
            && !(currentArmy->m_stats.attributes & MONSTER_FLAGS_FLYING)
            && (!(currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                || currentArmy->m_stats.shots <= 0))
            bits |= armyBit;
        armyBit <<= 1;
    }
    return bits;
}

VA(0x0041281e, 0x9f)
i16 combatManager::GetBestArmy(H1_ENUM_PARAM(CombatSide, i8) side, i16 mask) {
    i16 armyIndex = 0;
    i16 bitFlag = 1;
    u32 savedStrength;
    u32 theStrength = 0;
    i16 curBest = COMBAT_ARMY_INDEX_NONE;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bitFlag) {
            savedStrength = m_armies[side][armyIndex].Strength();
            if (savedStrength > theStrength) {
                curBest = armyIndex;
                theStrength = savedStrength;
            }
        }
        bitFlag <<= 1;
    }
    return curBest;
}

VA(0x004128bd, 0x9f)
i16 combatManager::GetWorstArmy(H1_ENUM_PARAM(CombatSide, i8) side, i16 mask) {
    i16 armyIndex = 0;
    i16 bit = 1;
    u32 force;
    u32 weakestStrength = COMBAT_AI_WORST_STRENGTH_LIMIT;
    i16 weakestArmy = COMBAT_ARMY_INDEX_NONE;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bit) {
            force = m_armies[side][armyIndex].Strength();
            if (force < weakestStrength) {
                weakestArmy = armyIndex;
                weakestStrength = force;
            }
        }
        bit <<= 1;
    }
    return weakestArmy;
}

VA(0x0041295c, 0x102)
i16 combatManager::GetClosestArmy(
    class army* currentArmy,
    H1_ENUM_PARAM(CombatSide, i8) side,
    i16 mask
) {
    i16 armyIndex = 0;
    army* target;
    i16 bitFlag = 1;
    i32 bestValue = FINDPATH_INITIAL_BEST_DISTANCE;
    i16 armyFound = COMBAT_ARMY_INDEX_NONE;
    i32 val;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bitFlag) {
            target = &m_armies[side][armyIndex];
            val = gSearchArray->QuickDistance(
                m_hexCells[currentArmy->m_hex].m_x,
                m_hexCells[currentArmy->m_hex].m_y,
                m_hexCells[target->m_hex].m_x,
                m_hexCells[target->m_hex].m_y
            );
            if (val < bestValue) {
                armyFound = armyIndex;
                bestValue = val;
            }
        }
        bitFlag <<= 1;
    }
    return armyFound;
}

VA(0x00412a5e, 0xb1)
u32 combatManager::GetStrength(H1_ENUM_PARAM(CombatSide, i8) side, i16 mask) {
    i16 idx = 0;
    i16 bitMask = 1;
    u32 totalStrength = 0;
    class army* currentArmy;

    for (idx = 0; idx < m_numArmies[side]; idx++) {
        if (mask & bitMask) {
            currentArmy = &m_armies[side][idx];
            if (currentArmy && !(currentArmy->m_stats.attributes & MONSTER_FLAGS_DEAD))
                totalStrength += currentArmy->Strength();
        }
        bitMask <<= 1;
    }
    return totalStrength;
}

// Ghosts (26) pick the weakest stack; a missed two-hex target is retried
// from its rear hex.
VA(0x00412b0f, 0x183)
i8 combatManager::AttemptAttack(
    class army* currentArmy,
    H1_ENUM_PARAM(CombatSide, i8) side,
    i16 mask
) {
    i16 targetArmy;
    i32 targetHex;

    while (mask) {
        if (currentArmy->m_creatureType == CREATURE_GHOST)
            targetArmy = GetWorstArmy(side, mask);
        else
            targetArmy = GetBestArmy(side, mask);
        if (targetArmy == COMBAT_ARMY_INDEX_NONE)
            return 0;
        currentArmy->m_targetSide = side;
        currentArmy->m_targetIndex = targetArmy;
        targetHex = m_armies[side][targetArmy].m_hex;
        currentArmy->m_moveTargetHex = targetHex;
        if (currentArmy->ValidPath(targetHex, ARMY_PATH_ANY_TARGET_HEX)) {
            SET_NEXT_COMBAT_MOVE(targetHex);
            return 1;
        }
        if (m_armies[side][targetArmy].m_stats.attributes & MONSTER_FLAGS_WIDE) {
            if (m_armies[side][targetArmy].m_facing == ARMY_FACING_LEFT)
                targetHex--;
            else
                targetHex++;
            currentArmy->m_moveTargetHex = targetHex;
            if (currentArmy->ValidPath(targetHex, ARMY_PATH_ANY_TARGET_HEX)) {
                SET_NEXT_COMBAT_MOVE(targetHex);
                return 1;
            }
        }
        mask &= ~(1 << targetArmy);
    }
    return 0;
}

VA(0x00412c92, 0x27c)
i8 combatManager::AttemptAdjacentAttack(class army* currentArmy) {
    i16 otherHex;
    i16 hex;
    i16 oneBit;
    H1_ENUM_LOCAL(CombatHexDirection, i16) direction;
    i16 enemyMask;
    i16 openMaskValue;
    i16 victim;

    openMaskValue =
        ~currentArmy->GetAttackMask(currentArmy->m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
    if (!openMaskValue)
        return 0;
    oneBit = 1;
    enemyMask = 0;
    for (direction = COMBAT_DIRECTION_NORTHEAST; direction < COMBAT_DIRECTION_COUNT; direction++) {
        if (openMaskValue & oneBit) {
            hex = currentArmy->GetAdjacentCellIndex(currentArmy->m_hex, direction);
            if (ValidHex(hex) && (currentArmy->m_stats.attributes & MONSTER_FLAGS_WIDE)
                    && m_hexCells[hex].m_occupantSide != COMBAT_OPPOSING_SIDE(m_currentSide)
                || m_hexCells[hex].m_occupantIndex == m_currentArmyIndex
                       && m_hexCells[hex].m_occupantSide == m_currentSide) {
                if (currentArmy->m_facing == ARMY_FACING_RIGHT)
                    otherHex = currentArmy->m_hex + 1;
                else
                    otherHex = currentArmy->m_hex - 1;
                if (hex % COMBAT_GRID_COLUMNS != 0
                    && hex % COMBAT_GRID_COLUMNS != COMBAT_GRID_LAST_COLUMN)
                    hex = currentArmy->GetAdjacentCellIndex(otherHex, direction);
                if (m_hexCells[hex].m_occupantSide != COMBAT_OPPOSING_SIDE(m_currentSide))
                    hex = ARMY_HEX_INVALID;
            }
            if (hex >= 0)
                enemyMask |= 1 << m_hexCells[hex].m_occupantIndex;
        }
        oneBit <<= 1;
    }
    if (currentArmy->m_creatureType == CREATURE_GHOST)
        victim = GetWorstArmy(COMBAT_OPPOSING_SIDE(m_currentSide), enemyMask);
    else
        victim = GetBestArmy(COMBAT_OPPOSING_SIDE(m_currentSide), enemyMask);
    if (victim != COMBAT_ARMY_INDEX_NONE) {
        SET_NEXT_COMBAT_MOVE(m_armies[COMBAT_OPPOSING_SIDE(m_currentSide)][victim].m_hex);
        return 1;
    } else {
        return 0;
    }
}

VA(0x00412f0e, 0x1ea)
i8 combatManager::WalkTowardArmyFront(
    class army* currentArmy,
    H1_ENUM_PARAM(CombatSide, i8) side,
    i16 mask
) {
    i16 frontHex;
    i32 armyIndex;
    i32 frontDelta;
    i32 canReachRequested;
    i8 oldSpeed;
    i16 pathNdxIndex;
    i16 left;

    CLEAR_ARMY_TARGET(currentArmy);
    armyIndex = GetClosestArmy(currentArmy, side, mask);
    if (armyIndex == COMBAT_ARMY_INDEX_NONE)
        return 0;
    frontDelta = 1;
    frontHex = m_armies[side][armyIndex].m_hex;
    if (m_armies[side][armyIndex].m_stats.attributes & MONSTER_FLAGS_WIDE)
        frontDelta = 2;
    frontHex += currentArmy->m_facing == ARMY_FACING_RIGHT ? frontDelta : -frontDelta;
    if (frontHex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN
        || frontHex % COMBAT_GRID_COLUMNS == 0)
        return WalkTowardArmy(currentArmy, side, mask);
    oldSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = COMBAT_AI_UNLIMITED_PATH_SPEED;
    canReachRequested =
        gSearchArray
            ->FindCombatPath(currentArmy->m_hex, frontHex, currentArmy, ARMY_PATH_EXACT_TARGET_HEX);
    currentArmy->m_stats.speed = oldSpeed;
    if (gSearchArray->m_pathLength > 0) {
        gNextAction = ACTION_MOVE;
        left = currentArmy->m_stats.speed;
        pathNdxIndex = gSearchArray->m_pathLength - 1;
        gNextActionGridIndex = currentArmy->m_hex;
        while (pathNdxIndex >= 0 && left) {
            gNextActionGridIndex = currentArmy->GetAdjacentCellIndex(
                gNextActionGridIndex,
                H1_ENUM_DECODE(CombatHexDirection, gSearchArray->m_directions[pathNdxIndex])
            );
            pathNdxIndex--;
            left--;
        }
        return 1;
    }
    return WalkTowardArmy(currentArmy, side, mask);
}

VA(0x004130f8, 0x1f6)
i8 combatManager::WalkTowardArmy(
    class army* currentArmy,
    H1_ENUM_PARAM(CombatSide, i8) side,
    i16 mask
) {
    i32 slot;
    i8 savedSpeedRequested;
    i32 routeGot;
    i16 attackMaskValue;
    i16 pathNdx;
    i16 left;
    army* targetPtr;
    i16 savedHex;
    i32 destVal;

    slot = GetClosestArmy(currentArmy, side, mask);
    if (slot == COMBAT_ARMY_INDEX_NONE)
        return 0;
    targetPtr = &m_armies[side][slot];
    savedHex = targetPtr->m_hex;
    currentArmy->m_targetSide = side;
    currentArmy->m_targetIndex = slot;
    attackMaskValue = currentArmy->GetAttackMask(
        currentArmy->m_hex,
        ARMY_ATTACK_TARGET_ASSIGNED,
        ARMY_HEX_INVALID
    );
    if (attackMaskValue != COMBAT_ALL_DIRECTIONS_BLOCKED) {
        gNextAction = ACTION_SKIP_TURN;
        return 1;
    }
    savedSpeedRequested = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = COMBAT_AI_UNLIMITED_PATH_SPEED;
    routeGot = gSearchArray->FindCombatPath(
        currentArmy->m_hex,
        savedHex,
        currentArmy,
        ARMY_PATH_ASSIGNED_TARGET_HEX
    );
    if (!routeGot && (targetPtr->m_stats.attributes & MONSTER_FLAGS_WIDE)) {
        switch (targetPtr->m_facing) {
            case ARMY_FACING_LEFT:
                --savedHex;
                break;
            case ARMY_FACING_RIGHT:
                ++savedHex;
                break;
        }
        if (savedHex != ARMY_HEX_INVALID)
            routeGot = gSearchArray->FindCombatPath(
                currentArmy->m_hex,
                savedHex,
                currentArmy,
                ARMY_PATH_ASSIGNED_TARGET_HEX
            );
    }
    currentArmy->m_stats.speed = savedSpeedRequested;
    if (gSearchArray->m_pathLength > 1) {
        gNextAction = ACTION_MOVE;
        left = currentArmy->m_stats.speed;
        pathNdx = gSearchArray->m_pathLength - 1;
        gNextActionGridIndex = currentArmy->m_hex;
        while (pathNdx >= 1 && left) {
            gNextActionGridIndex = currentArmy->GetAdjacentCellIndex(
                gNextActionGridIndex,
                H1_ENUM_DECODE(CombatHexDirection, gSearchArray->m_directions[pathNdx])
            );
            pathNdx--;
            left--;
        }
        return 1;
    }
    return 0;
}
