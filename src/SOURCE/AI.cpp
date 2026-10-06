#include <H1/Ints.h>

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

i32 combatManager::AICheckRetreat(void) {
    if (m_combatTowns[m_currentSide])
        return 0;
    if (!m_heroes[m_currentSide])
        return 0;
    if (!gpGame->m_players[m_heroes[m_currentSide]->m_owner].m_townCount)
        return 0;

    hero heroCopy;
    armyGroup* armyPtr;
    hero* sideHero;
    float retreatRatio;
    float prob;
    i32 treasureValue;
    i32 armyIndex;
    i32 side;
    armyGroup bareGroup;
    i32 artifactTotals[COMBAT_SIDE_COUNT];
    float expBonus;
    i32 force[COMBAT_SIDE_COUNT];

    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        if (m_heroes[side]) {
            heroCopy = *m_heroes[side];
            sideHero = &heroCopy;
            armyPtr = &sideHero->m_army;
        } else {
            armyPtr = &bareGroup;
            sideHero = NULL;
        }
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
            if (m_armies[side][armyIndex].IsAlive()) {
                armyPtr->m_creatureTypes[armyIndex] = m_armies[side][armyIndex].m_creatureType;
                if (m_armies[side][armyIndex].m_stats.attributes & MONSTER_FLAGS_TURN_SPENT)
                    armyPtr->m_creatureCounts[armyIndex] = m_armies[side][armyIndex].m_quantity;
                else
                    armyPtr->m_creatureCounts[armyIndex] =
                        static_cast<i16>(m_armies[side][armyIndex].m_quantity * 1.2);
            } else {
                armyPtr->m_creatureTypes[armyIndex] = CREATURE_NONE;
                armyPtr->m_creatureCounts[armyIndex] = 0;
            }
        }
        force[side] = gpPhilAI->FightValueOfStack(armyPtr, sideHero, 1, 0, 0);
        if (m_combatTowns[side])
            force[side] = static_cast<i32>(force[side] * 1.1);
        artifactTotals[side] = 0;
        if (sideHero) {
            for (armyIndex = 0; armyIndex < HERO_ARTIFACT_SLOT_COUNT; armyIndex++) {
                if (sideHero->m_artifacts[armyIndex] >= 0
                    && sideHero->m_artifacts[armyIndex] < ARTIFACT_REGULAR_END)
                    artifactTotals[side] += gArtifactBaseRV[sideHero->m_artifacts[armyIndex]];
            }
        }
    }
    force[1 - m_currentSide] = static_cast<i32>(force[1 - m_currentSide] * 1.1);
    treasureValue = artifactTotals[m_currentSide];
    if (artifactTotals[m_currentSide] < COMBAT_AI_MIN_ARTIFACT_VALUE)
        return 0;
    prob = 0.16f;
    if (treasureValue > COMBAT_AI_HIGH_ARTIFACT_VALUE)
        prob = prob + 0.06;
    else if (treasureValue > COMBAT_AI_MEDIUM_ARTIFACT_VALUE)
        prob = prob + 0.05;
    else if (treasureValue > 0)
        prob = prob + 0.04;
    if (force[m_currentSide] > COMBAT_AI_RETREAT_SCALED_PENALTY_THRESHOLD)
        prob -= force[m_currentSide] / COMBAT_AI_RETREAT_STRENGTH_DIVISOR;
    else if (force[m_currentSide] > COMBAT_AI_RETREAT_TIER_4_THRESHOLD)
        prob = prob - 0.08;
    else if (force[m_currentSide] > COMBAT_AI_RETREAT_TIER_3_THRESHOLD)
        prob = prob - 0.06;
    else if (force[m_currentSide] > COMBAT_AI_RETREAT_TIER_2_THRESHOLD)
        prob = prob - 0.04;
    else if (force[m_currentSide] > COMBAT_AI_RETREAT_TIER_1_THRESHOLD)
        prob = prob - 0.02;
    expBonus = m_heroes[m_currentSide]->m_experience / COMBAT_AI_EXPERIENCE_DIVISOR;
    if (expBonus > 0.03)
        expBonus = 0.03f;
    prob += expBonus;
    if (m_currentSide == COMBAT_ATTACKER_SIDE)
        prob = prob - 0.06;
    prob -= (COMBAT_AI_MAX_DIFFICULTY
             - gpGame->m_players[m_heroes[m_currentSide]->m_owner].m_difficulty)
            * 0.03;
    retreatRatio = static_cast<float>(force[m_currentSide])
                   / (force[COMBAT_DEFENDER_SIDE] + force[COMBAT_ATTACKER_SIDE]);
    if (retreatRatio < prob) {
        giNextAction = ACTION_RETREAT;
        return 1;
    }
    return 0;
}

void combatManager::DoCompAI(i8) {
    i8 stronger;
    i16 ranged[COMBAT_SIDE_COUNT];
    i32 shootStrengths[COMBAT_SIDE_COUNT];
    i32 total;
    i16 walkerMask[COMBAT_SIDE_COUNT];
    i16 plan;
    i32 dirIndex;
    army* currentArmy;
    i16 sideEnemy;
    i32 foeShooters;
    i16 flyerMasks[COMBAT_SIDE_COUNT];
    i32 minShootPower;
    i8 targetIndex;
    i32 dummy;
    i32 myShootPower;
    i8 canOutshoot;
    hexcell* hexCell;
    i32 wallStrength;
    town* castleTown;
    i32 numArchers;
    i32 targetHex;
    i32 adj;

    m_limitCreature = 0;
    gpMouseManager->ReallyHidePointer();
    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    plan = COMBAT_AI_ATTACK_NONE;
    sideEnemy = 1 - m_currentSide;
    ranged[m_currentSide] = GetShooterMask(m_currentSide);
    ranged[sideEnemy] = GetShooterMask(sideEnemy);
    flyerMasks[m_currentSide] = GetFlyerMask(m_currentSide);
    flyerMasks[sideEnemy] = GetFlyerMask(sideEnemy);
    walkerMask[m_currentSide] = GetWalkerMask(m_currentSide);
    walkerMask[sideEnemy] = GetWalkerMask(sideEnemy);
    shootStrengths[m_currentSide] = GetStrength(m_currentSide, ranged[m_currentSide]);
    shootStrengths[sideEnemy] = GetStrength(sideEnemy, ranged[sideEnemy]);
    total = GetStrength(
        m_currentSide,
        ranged[m_currentSide] | flyerMasks[m_currentSide] | walkerMask[m_currentSide]
    );
    minShootPower = (total + COMBAT_AI_STRENGTH_ROUNDING) / COMBAT_AI_STRENGTH_FRACTION;
    canOutshoot = 0;
    stronger = 0;
    myShootPower = GetStrength(m_currentSide, ranged[m_currentSide]);
    foeShooters = GetStrength(sideEnemy, ranged[sideEnemy]);
    if (m_castleSide[COMBAT_DEFENDER_SIDE]) {
        numArchers = COMBAT_AI_CASTLE_BASE_ARCHERS;
        castleTown = m_combatTowns[COMBAT_DEFENDER_SIDE];
        for (dirIndex = BUILDING_SLOT_DWELLING_FIRST; dirIndex <= BUILDING_SLOT_DWELLING_LAST;
             dirIndex++)
            if (castleTown->m_buildings & (1 << dirIndex))
                numArchers += COMBAT_AI_CASTLE_ARCHERS_PER_DWELLING;
        for (dirIndex = BUILDING_SLOT_MAGE_GUILD; dirIndex <= BUILDING_SLOT_RACE_FIRST - 1;
             dirIndex++)
            if (castleTown->m_buildings & (1 << dirIndex))
                numArchers++;
        wallStrength = numArchers * COMBAT_AI_CASTLE_ARCHER_STRENGTH;
        if (m_currentSide == COMBAT_DEFENDER_SIDE)
            myShootPower += wallStrength;
        else
            foeShooters += wallStrength;
    }
    if ((total + COMBAT_AI_STRENGTH_ROUNDING) / COMBAT_AI_STRENGTH_FRACTION < myShootPower)
        canOutshoot = 1;
    if (foeShooters > myShootPower)
        stronger = 1;
    if (currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER) {
        if (currentArmy->m_stats.shots > 0)
            plan = COMBAT_AI_ATTACK_SHOOT;
        else
            plan = COMBAT_AI_ATTACK_WALK;
    } else if (currentArmy->m_stats.attributes & MONSTER_FLAGS_FLYING) {
        plan = COMBAT_AI_ATTACK_FLY;
    } else {
        plan = COMBAT_AI_ATTACK_WALK;
    }
    switch (plan) {
        case COMBAT_AI_ATTACK_SHOOT:
            if (AttemptAdjacentAttack(currentArmy)) {
                goto finish;
            } else {
                targetIndex = GetBestArmy(sideEnemy, ranged[sideEnemy]);
                if (targetIndex != COMBAT_ARMY_INDEX_NONE) {
                    giNextAction = ACTION_MOVE;
                    giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                    goto finish;
                }
                targetIndex = GetBestArmy(sideEnemy, flyerMasks[sideEnemy]);
                if (targetIndex != COMBAT_ARMY_INDEX_NONE) {
                    giNextAction = ACTION_MOVE;
                    giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                    goto finish;
                }
                if (walkerMask[sideEnemy]) {
                    targetIndex = GetClosestArmy(currentArmy, sideEnemy, walkerMask[sideEnemy]);
                    if (targetIndex != COMBAT_ARMY_INDEX_NONE) {
                        giNextAction = ACTION_MOVE;
                        giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                        goto finish;
                    }
                }
            }
            break;
        case COMBAT_AI_ATTACK_FLY:
            if (canOutshoot && !stronger) {
                if (AttemptAttack(currentArmy, sideEnemy, ranged[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(currentArmy, sideEnemy, flyerMasks[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(currentArmy, sideEnemy, walkerMask[sideEnemy]))
                    goto finish;
            } else {
                if (AttemptAttack(currentArmy, sideEnemy, ranged[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(currentArmy, sideEnemy, flyerMasks[sideEnemy]))
                    goto finish;
                else if (AttemptAttack(currentArmy, sideEnemy, walkerMask[sideEnemy]))
                    goto finish;
            }
            break;
        case COMBAT_AI_ATTACK_WALK:
            if (AttemptAdjacentAttack(currentArmy)) {
                goto finish;
            } else {
                if (canOutshoot && !stronger) {
                    if (WalkTowardArmyFront(currentArmy, m_currentSide, ranged[m_currentSide]))
                        goto finish;
                } else {
                    if (AttemptAttack(currentArmy, sideEnemy, ranged[sideEnemy]))
                        goto finish;
                    else if (AttemptAttack(currentArmy, sideEnemy, walkerMask[sideEnemy]))
                        goto finish;
                    else if (AttemptAttack(currentArmy, sideEnemy, flyerMasks[sideEnemy]))
                        goto finish;
                }
                if (WalkTowardArmy(currentArmy, sideEnemy, ranged[sideEnemy]))
                    goto finish;
                else if (WalkTowardArmy(currentArmy, sideEnemy, walkerMask[sideEnemy]))
                    goto finish;
                else if (WalkTowardArmy(currentArmy, sideEnemy, flyerMasks[sideEnemy]))
                    goto finish;
                if (m_currentSide == COMBAT_ATTACKER_SIDE && m_castleSide[COMBAT_DEFENDER_SIDE]
                    && currentArmy->m_hex % COMBAT_GRID_COLUMNS < COMBAT_CASTLE_WALL_COLUMN - 1) {
                    targetHex = currentArmy->m_hex / COMBAT_GRID_COLUMNS * COMBAT_GRID_COLUMNS
                                + (COMBAT_CASTLE_WALL_COLUMN - 1);
                    hexCell = &gpCombatManager->m_hexCells[targetHex];
                    if (ValidHex(targetHex) && hexCell->m_occupantSide == COMBAT_SIDE_NONE
                        && hexCell->m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
                        giNextAction = ACTION_MOVE;
                        giNextActionGridIndex = targetHex;
                        goto finish;
                    }
                }
            }
            break;
    }
    giNextAction = ACTION_SKIP_TURN;
finish:
    if (giNextAction == ACTION_MOVE && giNextActionGridIndex > 0 && giNextActionGridIndex <= 43
        && gpCombatManager->m_hexCells[giNextActionGridIndex].m_occupantSide == COMBAT_SIDE_NONE) {
        for (dirIndex = 0; dirIndex < COMBAT_DIRECTION_ADJACENT_COUNT; dirIndex++) {
            adj = currentArmy->GetAdjacentCellIndex(giNextActionGridIndex, dirIndex);
            if (adj > 0 && adj <= 43
                && gpCombatManager->m_hexCells[adj].m_occupantSide == 1 - m_currentSide) {
                giNextActionGridIndex = adj;
                return;
            }
        }
    }
}

i16 combatManager::GetShooterMask(i8 side) {
    i16 armyIndex = 0;
    i16 bitMask = 1;
    class army* army;
    i16 armyMask = 0;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD)
            && (army->m_stats.attributes & MONSTER_FLAGS_SHOOTER) && army->m_stats.shots > 0)
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

i16 combatManager::GetFlyerMask(i8 side) {
    i16 armyIndex = 0;
    i16 armyMask;
    i16 bitMask = 1;
    class army* army;

    armyMask = 0;
    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD)
            && (army->m_stats.attributes & MONSTER_FLAGS_FLYING))
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

i16 combatManager::GetWalkerMask(i8 side) {
    i16 armyIndex = 0;
    i16 bitMask = 1;
    i16 armyMask = 0;
    class army* army;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD)
            && !(army->m_stats.attributes & MONSTER_FLAGS_FLYING)
            && (!(army->m_stats.attributes & MONSTER_FLAGS_SHOOTER) || army->m_stats.shots <= 0))
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

i16 combatManager::GetBestArmy(i8 side, i16 mask) {
    i16 armyIndex = 0;
    i16 bitFlag = 1;
    u32 strength;
    u32 bestStrength = 0;
    i16 best = COMBAT_ARMY_INDEX_NONE;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bitFlag) {
            strength = m_armies[side][armyIndex].Strength();
            if (strength > bestStrength) {
                best = armyIndex;
                bestStrength = strength;
            }
        }
        bitFlag <<= 1;
    }
    return best;
}

i16 combatManager::GetWorstArmy(i8 side, i16 mask) {
    i16 armyIndex = 0;
    i16 bitFlag = 1;
    u32 strength;
    u32 worstStrength = COMBAT_AI_WORST_STRENGTH_LIMIT;
    i16 worst = COMBAT_ARMY_INDEX_NONE;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bitFlag) {
            strength = m_armies[side][armyIndex].Strength();
            if (strength < worstStrength) {
                worst = armyIndex;
                worstStrength = strength;
            }
        }
        bitFlag <<= 1;
    }
    return worst;
}

i16 combatManager::GetClosestArmy(class army* currentArmy, i8 side, i16 mask) {
    i32 val;
    i16 armyIndex = 0;
    army* target;
    i16 bitFlag = 1;
    i32 closestDist = FINDPATH_INITIAL_BEST_DISTANCE;
    i16 bestArmy = COMBAT_ARMY_INDEX_NONE;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bitFlag) {
            target = &m_armies[side][armyIndex];
            val = gpSearchArray->QuickDistance(
                m_hexCells[currentArmy->m_hex].m_x,
                m_hexCells[currentArmy->m_hex].m_y,
                m_hexCells[target->m_hex].m_x,
                m_hexCells[target->m_hex].m_y
            );
            if (val < closestDist) {
                bestArmy = armyIndex;
                closestDist = val;
            }
        }
        bitFlag <<= 1;
    }
    return bestArmy;
}

u32 combatManager::GetStrength(i8 side, i16 mask) {
    i16 index = 0;
    i16 bitMask = 1;
    u32 total = 0;
    class army* army;

    for (index = 0; index < m_numArmies[side]; index++) {
        if (mask & bitMask) {
            army = &m_armies[side][index];
            if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD))
                total += army->Strength();
        }
        bitMask <<= 1;
    }
    return total;
}

i8 combatManager::AttemptAttack(class army* currentArmy, i8 side, i16 mask) {
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
            giNextAction = ACTION_MOVE;
            giNextActionGridIndex = targetHex;
            return 1;
        }
        if (m_armies[side][targetArmy].m_stats.attributes & MONSTER_FLAGS_WIDE) {
            if (m_armies[side][targetArmy].m_facing == ARMY_FACING_LEFT)
                targetHex--;
            else
                targetHex++;
            currentArmy->m_moveTargetHex = targetHex;
            if (currentArmy->ValidPath(targetHex, ARMY_PATH_ANY_TARGET_HEX)) {
                giNextAction = ACTION_MOVE;
                giNextActionGridIndex = targetHex;
                return 1;
            }
        }
        mask &= ~(1 << targetArmy);
    }
    return 0;
}

i8 combatManager::AttemptAdjacentAttack(class army* currentArmy) {
    i16 otherHex;
    i16 hex;
    i16 oneBit;
    i16 dir;
    i16 enemyMask;
    i16 openMask;
    i16 target;

    openMask =
        ~currentArmy->GetAttackMask(currentArmy->m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
    if (!openMask)
        return 0;
    oneBit = 1;
    enemyMask = 0;
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (openMask & oneBit) {
            hex = currentArmy->GetAdjacentCellIndex(currentArmy->m_hex, dir);
            if (ValidHex(hex) && (currentArmy->m_stats.attributes & MONSTER_FLAGS_WIDE)
                    && m_hexCells[hex].m_occupantSide != 1 - m_currentSide
                || m_hexCells[hex].m_occupantIndex == m_currentArmyIndex
                       && m_hexCells[hex].m_occupantSide == m_currentSide) {
                if (currentArmy->m_facing == ARMY_FACING_RIGHT)
                    otherHex = currentArmy->m_hex + 1;
                else
                    otherHex = currentArmy->m_hex - 1;
                if (hex % COMBAT_GRID_COLUMNS != 0
                    && hex % COMBAT_GRID_COLUMNS != COMBAT_GRID_LAST_COLUMN)
                    hex = currentArmy->GetAdjacentCellIndex(otherHex, dir);
                if (m_hexCells[hex].m_occupantSide != 1 - m_currentSide)
                    hex = ARMY_HEX_INVALID;
            }
            if (hex >= 0)
                enemyMask |= 1 << m_hexCells[hex].m_occupantIndex;
        }
        oneBit <<= 1;
    }
    if (currentArmy->m_creatureType == CREATURE_GHOST)
        target = GetWorstArmy(1 - m_currentSide, enemyMask);
    else
        target = GetBestArmy(1 - m_currentSide, enemyMask);
    if (target != COMBAT_ARMY_INDEX_NONE) {
        giNextAction = ACTION_MOVE;
        giNextActionGridIndex = m_armies[1 - m_currentSide][target].m_hex;
        return 1;
    } else {
        return 0;
    }
}

i8 combatManager::WalkTowardArmyFront(class army* currentArmy, i8 side, i16 mask) {
    i16 frontHex;
    i32 armyIndex;
    i32 frontDelta;
    i32 canReach;
    i8 oldSpeed;
    i16 pathNdx;
    i16 left;

    CLEAR_ARMY_TARGET(currentArmy);
    armyIndex = GetClosestArmy(currentArmy, side, mask);
    if (armyIndex == COMBAT_ARMY_INDEX_NONE)
        return 0;
    frontDelta = 1;
    frontHex = m_armies[side][armyIndex].m_hex;
    if (m_armies[side][armyIndex].m_stats.attributes & MONSTER_FLAGS_WIDE)
        frontDelta = 2;
    if (currentArmy->m_facing == ARMY_FACING_RIGHT)
        frontHex = frontHex + frontDelta;
    else
        frontHex = frontHex + -frontDelta;
    if (frontHex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN
        || frontHex % COMBAT_GRID_COLUMNS == 0)
        return WalkTowardArmy(currentArmy, side, mask);
    oldSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = COMBAT_AI_UNLIMITED_PATH_SPEED;
    canReach =
        gpSearchArray
            ->FindCombatPath(currentArmy->m_hex, frontHex, currentArmy, ARMY_PATH_EXACT_TARGET_HEX);
    currentArmy->m_stats.speed = oldSpeed;
    if (gpSearchArray->m_pathLength > 0) {
        giNextAction = ACTION_MOVE;
        left = currentArmy->m_stats.speed;
        pathNdx = gpSearchArray->m_pathLength - 1;
        giNextActionGridIndex = currentArmy->m_hex;
        while (pathNdx >= 0 && left) {
            giNextActionGridIndex = currentArmy->GetAdjacentCellIndex(
                giNextActionGridIndex,
                gpSearchArray->m_directions[pathNdx]
            );
            pathNdx--;
            left--;
        }
        return 1;
    }
    return WalkTowardArmy(currentArmy, side, mask);
}

i8 combatManager::WalkTowardArmy(class army* currentArmy, i8 side, i16 mask) {
    i32 armyIndex;
    i8 savedSpeed;
    i32 routeGot;
    i16 attackMask;
    i16 pathNdx;
    i16 left;
    army* targetPtr;
    i16 goalHex;
    i32 dest;

    armyIndex = GetClosestArmy(currentArmy, side, mask);
    if (armyIndex == COMBAT_ARMY_INDEX_NONE)
        return 0;
    targetPtr = &m_armies[side][armyIndex];
    goalHex = targetPtr->m_hex;
    currentArmy->m_targetSide = side;
    currentArmy->m_targetIndex = armyIndex;
    attackMask = currentArmy->GetAttackMask(
        currentArmy->m_hex,
        ARMY_ATTACK_TARGET_ASSIGNED,
        ARMY_HEX_INVALID
    );
    if (attackMask != COMBAT_ALL_DIRECTIONS_BLOCKED) {
        giNextAction = ACTION_SKIP_TURN;
        return 1;
    }
    savedSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = COMBAT_AI_UNLIMITED_PATH_SPEED;
    routeGot = gpSearchArray->FindCombatPath(
        currentArmy->m_hex,
        goalHex,
        currentArmy,
        ARMY_PATH_ASSIGNED_TARGET_HEX
    );
    if (!routeGot && (targetPtr->m_stats.attributes & MONSTER_FLAGS_WIDE)) {
        switch (targetPtr->m_facing) {
            case ARMY_FACING_LEFT:
                goalHex = goalHex - 1;
                break;
            case ARMY_FACING_RIGHT:
                goalHex = goalHex + 1;
                break;
        }
        if (goalHex != ARMY_HEX_INVALID)
            routeGot = gpSearchArray->FindCombatPath(
                currentArmy->m_hex,
                goalHex,
                currentArmy,
                ARMY_PATH_ASSIGNED_TARGET_HEX
            );
    }
    currentArmy->m_stats.speed = savedSpeed;
    if (gpSearchArray->m_pathLength > 1) {
        giNextAction = ACTION_MOVE;
        left = currentArmy->m_stats.speed;
        pathNdx = gpSearchArray->m_pathLength - 1;
        giNextActionGridIndex = currentArmy->m_hex;
        while (pathNdx >= 1 && left) {
            giNextActionGridIndex = currentArmy->GetAdjacentCellIndex(
                giNextActionGridIndex,
                gpSearchArray->m_directions[pathNdx]
            );
            pathNdx--;
            left--;
        }
        return 1;
    }
    return 0;
}
