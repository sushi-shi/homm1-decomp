// Buka SOURCE/AI combat helpers. Retail links them as their own object
// (0x00464520-0x00466487): eight int3 bytes pad WalkTowardArmy up to the
// 16-byte boundary where army::army starts the SOURCE/ARMY object.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/PATH.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Buka AI.cpp AICheckRetreat: compares the two sides' fight values,
// weighting the defender of a town and unspent stacks, against a chance
// raised by the hero's artifacts and experience.
VA(0x00464520, 0x783)
int combatManager::AICheckRetreat(void) {
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
    int treasureValue;
    int armyIndex;
    int side;
    armyGroup bareGroup;
    int artifactTotals[2];
    float expBonus;
    int force[2];

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
                    armyPtr->m_creatureCounts[armyIndex] = static_cast<short>(m_armies[side][armyIndex].m_quantity * 1.2);
            } else {
                armyPtr->m_creatureTypes[armyIndex] = CREATURE_NONE;
                armyPtr->m_creatureCounts[armyIndex] = 0;
            }
        }
        force[side] = gpPhilAI->FightValueOfStack(armyPtr, sideHero, 1, 0, 0);
        if (m_combatTowns[side])
            force[side] = static_cast<int>(force[side] * 1.1);
        artifactTotals[side] = 0;
        if (sideHero) {
            for (armyIndex = 0; armyIndex < 14; armyIndex++) {
                if (sideHero->m_artifacts[armyIndex] >= 0 && sideHero->m_artifacts[armyIndex] < 37)
                    artifactTotals[side] += gArtifactBaseRV[sideHero->m_artifacts[armyIndex]];
            }
        }
    }
    force[1 - m_currentSide] = static_cast<int>(force[1 - m_currentSide] * 1.1);
    treasureValue = artifactTotals[m_currentSide];
    if (artifactTotals[m_currentSide] < 1000)
        return 0;
    prob = 0.16f;
    if (treasureValue > 10000)
        prob = prob + 0.06;
    else if (treasureValue > 5000)
        prob = prob + 0.05;
    else if (treasureValue > 0)
        prob = prob + 0.04;
    if (force[m_currentSide] > 40000)
        prob -= force[m_currentSide] / 20000;
    else if (force[m_currentSide] > 30000)
        prob = prob - 0.08;
    else if (force[m_currentSide] > 15000)
        prob = prob - 0.06;
    else if (force[m_currentSide] > 5000)
        prob = prob - 0.04;
    else if (force[m_currentSide] > 2500)
        prob = prob - 0.02;
    expBonus = m_heroes[m_currentSide]->m_experience / 200000;
    if (expBonus > 0.03)
        expBonus = 0.03f;
    prob += expBonus;
    if (m_currentSide == 1)
        prob = prob - 0.06;
    prob -= (4 - gpGame->m_players[m_heroes[m_currentSide]->m_owner].m_difficulty) * 0.03;
    retreatRatio = static_cast<float>(force[m_currentSide]) / (force[0] + force[1]);
    if (retreatRatio < prob) {
        giNextAction = ACTION_RETREAT;
        return 1;
    }
    return 0;
}

// Buka AI.cpp DoCompAI: shooters shoot (adjacent enemies first), flyers and
// walkers attack by target class, walkers otherwise close in; a castle
// defender steps toward the gate. The chosen move is nudged onto a free hex
// next to an enemy.
VA(0x00464ca3, 0x9ce)
void combatManager::DoCompAI(signed char) {
    signed char stronger;
    short ranged[2];
    long shootStrengths[2];
    long total;
    short walkerMask[2];
    short plan;
    int dirIndex;
    army* currentArmy;
    short sideEnemy;
    long foeShooters;
    short flyerMasks[2];
    int minShootPower;
    signed char targetIndex;
    int dummy;
    long myShootPower;
    signed char canOutshoot;
    hexcell* hexCell;
    int wallStrength;
    town* castleTown;
    int numArchers;
    int targetHex;
    int adj;

    m_limitCreature = 0;
    gpMouseManager->ReallyHidePointer();
    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    plan = 0;
    sideEnemy = 1 - m_currentSide;
    ranged[m_currentSide] = GetShooterMask(m_currentSide);
    ranged[sideEnemy] = GetShooterMask(sideEnemy);
    flyerMasks[m_currentSide] = GetFlyerMask(m_currentSide);
    flyerMasks[sideEnemy] = GetFlyerMask(sideEnemy);
    walkerMask[m_currentSide] = GetWalkerMask(m_currentSide);
    walkerMask[sideEnemy] = GetWalkerMask(sideEnemy);
    shootStrengths[m_currentSide] = GetStrength(m_currentSide, ranged[m_currentSide]);
    shootStrengths[sideEnemy] = GetStrength(sideEnemy, ranged[sideEnemy]);
    total = GetStrength(m_currentSide, ranged[m_currentSide] | flyerMasks[m_currentSide] | walkerMask[m_currentSide]);
    minShootPower = (total + 4) / 5;
    canOutshoot = 0;
    stronger = 0;
    myShootPower = GetStrength(m_currentSide, ranged[m_currentSide]);
    foeShooters = GetStrength(sideEnemy, ranged[sideEnemy]);
    if (m_castleSide[0]) {
        numArchers = 5;
        castleTown = m_combatTowns[0];
        for (dirIndex = 7; dirIndex <= 12; dirIndex++)
            if (castleTown->m_buildings & (1 << dirIndex))
                numArchers += 4;
        for (dirIndex = 0; dirIndex <= 4; dirIndex++)
            if (castleTown->m_buildings & (1 << dirIndex))
                numArchers++;
        wallStrength = numArchers * 100;
        if (m_currentSide == 0)
            myShootPower += wallStrength;
        else
            foeShooters += wallStrength;
    }
    if ((total + 4) / 5 < myShootPower)
        canOutshoot = 1;
    if (foeShooters > myShootPower)
        stronger = 1;
    if (currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER) {
        if (currentArmy->m_stats.shots > 0)
            plan = 1;
        else
            plan = 3;
    } else if (currentArmy->m_stats.attributes & MONSTER_FLAGS_FLYING) {
        plan = 2;
    } else {
        plan = 3;
    }
    switch (plan) {
        case 1:
            if (AttemptAdjacentAttack(currentArmy)) {
                goto finish;
            } else {
                targetIndex = GetBestArmy(sideEnemy, ranged[sideEnemy]);
                if (targetIndex != -1) {
                    giNextAction = ACTION_MOVE;
                    giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                    goto finish;
                }
                targetIndex = GetBestArmy(sideEnemy, flyerMasks[sideEnemy]);
                if (targetIndex != -1) {
                    giNextAction = ACTION_MOVE;
                    giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                    goto finish;
                }
                if (walkerMask[sideEnemy]) {
                    targetIndex = GetClosestArmy(currentArmy, sideEnemy, walkerMask[sideEnemy]);
                    if (targetIndex != -1) {
                        giNextAction = ACTION_MOVE;
                        giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                        goto finish;
                    }
                }
            }
            break;
        case 2:
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
        case 3:
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
                if (m_currentSide == 1 && m_castleSide[0] && currentArmy->m_hex % COMBAT_GRID_COLUMNS < 4) {
                    targetHex = currentArmy->m_hex / COMBAT_GRID_COLUMNS * COMBAT_GRID_COLUMNS + 4;
                    hexCell = &gpCombatManager->m_hexCells[targetHex];
                    if (ValidHex(targetHex) && hexCell->m_occupantSide == -1 && hexCell->m_obstacleIndex == -1) {
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
        && gpCombatManager->m_hexCells[giNextActionGridIndex].m_occupantSide == -1) {
        for (dirIndex = 0; dirIndex < 6; dirIndex++) {
            adj = currentArmy->GetAdjacentCellIndex(giNextActionGridIndex, dirIndex);
            if (adj > 0 && adj <= 43 && gpCombatManager->m_hexCells[adj].m_occupantSide == 1 - m_currentSide) {
                giNextActionGridIndex = adj;
                return;
            }
        }
    }
}

// Buka AI.cpp mask helpers; HoMM1 loops word indices over m_numArmies and
// builds word masks (dead flag 0x10, shooter 4, flyer 2).
VA(0x00465671, 0xca)
short combatManager::GetShooterMask(signed char side) {
    short armyIndex = 0;
    short bitMask = 1;
    class army* army;
    short armyMask = 0;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD) && (army->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
            && army->m_stats.shots > 0)
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

VA(0x0046573b, 0xbb)
short combatManager::GetFlyerMask(signed char side) {
    short armyIndex = 0;
    short armyMask;
    short bitMask = 1;
    class army* army;

    armyMask = 0;
    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD) && (army->m_stats.attributes & MONSTER_FLAGS_FLYING))
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

VA(0x004657f6, 0xd7)
short combatManager::GetWalkerMask(signed char side) {
    short armyIndex = 0;
    short bitMask = 1;
    short armyMask = 0;
    class army* army;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & MONSTER_FLAGS_DEAD) && !(army->m_stats.attributes & MONSTER_FLAGS_FLYING)
            && (!(army->m_stats.attributes & MONSTER_FLAGS_SHOOTER) || army->m_stats.shots <= 0))
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

VA(0x004658cd, 0xb3)
short combatManager::GetBestArmy(signed char side, short mask) {
    short armyIndex = 0;
    short bitFlag = 1;
    unsigned long strength;
    unsigned long bestStrength = 0;
    short best = -1;

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

VA(0x00465980, 0xb3)
short combatManager::GetWorstArmy(signed char side, short mask) {
    short armyIndex = 0;
    short bitFlag = 1;
    unsigned long strength;
    unsigned long worstStrength = 999999999;
    short worst = -1;

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

VA(0x00465a33, 0x109)
short combatManager::GetClosestArmy(class army* currentArmy, signed char side, short mask) {
    int val;
    short armyIndex = 0;
    army* target;
    short bitFlag = 1;
    int closestDist = 640;
    short bestArmy = -1;

    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (mask & bitFlag) {
            target = &m_armies[side][armyIndex];
            val = gpSearchArray->QuickDistance(m_hexCells[currentArmy->m_hex].m_x, m_hexCells[currentArmy->m_hex].m_y,
                                                m_hexCells[target->m_hex].m_x, m_hexCells[target->m_hex].m_y);
            if (val < closestDist) {
                bestArmy = armyIndex;
                closestDist = val;
            }
        }
        bitFlag <<= 1;
    }
    return bestArmy;
}

VA(0x00465b3c, 0xbb)
unsigned long int combatManager::GetStrength(signed char side, short mask) {
    short index = 0;
    short bitMask = 1;
    unsigned long total = 0;
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

// Ghosts (26) pick the weakest stack; a missed two-hex target is retried
// from its rear hex.
VA(0x00465bf7, 0x1b0)
signed char combatManager::AttemptAttack(class army* currentArmy, signed char side, short mask) {
    short targetArmy;
    int targetHex;

    while (mask) {
        if (currentArmy->m_creatureType == CREATURE_GHOST)
            targetArmy = GetWorstArmy(side, mask);
        else
            targetArmy = GetBestArmy(side, mask);
        if (targetArmy == -1)
            return 0;
        currentArmy->m_targetSide = side;
        currentArmy->m_targetIndex = targetArmy;
        targetHex = m_armies[side][targetArmy].m_hex;
        currentArmy->m_moveTargetHex = targetHex;
        if (currentArmy->ValidPath(targetHex, 0)) {
            giNextAction = ACTION_MOVE;
            giNextActionGridIndex = targetHex;
            return 1;
        }
        if (m_armies[side][targetArmy].m_stats.attributes & MONSTER_FLAGS_WIDE) {
            if (m_armies[side][targetArmy].m_facing == ARMY_FACING_RIGHT)
                targetHex--;
            else
                targetHex++;
            currentArmy->m_moveTargetHex = targetHex;
            if (currentArmy->ValidPath(targetHex, 0)) {
                giNextAction = ACTION_MOVE;
                giNextActionGridIndex = targetHex;
                return 1;
            }
        }
        mask &= ~(1 << targetArmy);
    }
    return 0;
}

VA(0x00465da7, 0x2a9)
signed char combatManager::AttemptAdjacentAttack(class army* currentArmy) {
    short otherHex;
    short hex;
    short oneBit;
    short openMask;
    short dir;
    short enemyMask;
    short target;

    openMask = ~currentArmy->GetAttackMask(currentArmy->m_hex, 1, -1);
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
                if (currentArmy->m_facing == ARMY_FACING_LEFT)
                    otherHex = currentArmy->m_hex + 1;
                else
                    otherHex = currentArmy->m_hex - 1;
                if (hex % COMBAT_GRID_COLUMNS != 0 && hex % COMBAT_GRID_COLUMNS != COMBAT_GRID_LAST_COLUMN)
                    hex = currentArmy->GetAdjacentCellIndex(otherHex, dir);
                if (m_hexCells[hex].m_occupantSide != 1 - m_currentSide)
                    hex = -1;
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
    if (target != -1) {
        giNextAction = ACTION_MOVE;
        giNextActionGridIndex = m_armies[1 - m_currentSide][target].m_hex;
        return 1;
    } else {
        return 0;
    }
}

VA(0x00466050, 0x20f)
signed char combatManager::WalkTowardArmyFront(class army* currentArmy, signed char side, short mask) {
    short frontHex;
    int armyIndex;
    int frontDelta;
    int canReach;
    signed char oldSpeed;
    short pathNdx;
    short left;

    currentArmy->m_targetSide = -1;
    currentArmy->m_targetIndex = -1;
    armyIndex = GetClosestArmy(currentArmy, side, mask);
    if (armyIndex == -1)
        return 0;
    frontDelta = 1;
    frontHex = m_armies[side][armyIndex].m_hex;
    if (m_armies[side][armyIndex].m_stats.attributes & MONSTER_FLAGS_WIDE)
        frontDelta = 2;
    if (currentArmy->m_facing == ARMY_FACING_LEFT)
        frontHex = frontHex + frontDelta;
    else
        frontHex = frontHex + -frontDelta;
    if (frontHex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN || frontHex % COMBAT_GRID_COLUMNS == 0)
        return WalkTowardArmy(currentArmy, side, mask);
    oldSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = 127;
    canReach = gpSearchArray->FindCombatPath(currentArmy->m_hex, frontHex, currentArmy, 1);
    currentArmy->m_stats.speed = oldSpeed;
    if (gpSearchArray->m_pathLength > 0) {
        giNextAction = ACTION_MOVE;
        left = currentArmy->m_stats.speed;
        pathNdx = gpSearchArray->m_pathLength - 1;
        giNextActionGridIndex = currentArmy->m_hex;
        while (pathNdx >= 0 && left) {
            giNextActionGridIndex =
                currentArmy->GetAdjacentCellIndex(giNextActionGridIndex, gpSearchArray->m_directions[pathNdx]);
            pathNdx--;
            left--;
        }
        return 1;
    }
    return WalkTowardArmy(currentArmy, side, mask);
}

VA(0x0046625f, 0x229)
signed char combatManager::WalkTowardArmy(class army* currentArmy, signed char side, short mask) {
    int armyIndex;
    signed char savedSpeed;
    int routeGot;
    short attackMask;
    short pathNdx;
    short left;
    army* targetPtr;
    short goalHex;
    int dest;

    armyIndex = GetClosestArmy(currentArmy, side, mask);
    if (armyIndex == -1)
        return 0;
    targetPtr = &m_armies[side][armyIndex];
    goalHex = targetPtr->m_hex;
    currentArmy->m_targetSide = side;
    currentArmy->m_targetIndex = armyIndex;
    attackMask = currentArmy->GetAttackMask(currentArmy->m_hex, 0, -1);
    if (attackMask != 0xff) {
        giNextAction = ACTION_SKIP_TURN;
        return 1;
    }
    savedSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = 127;
    routeGot = gpSearchArray->FindCombatPath(currentArmy->m_hex, goalHex, currentArmy, -1);
    if (!routeGot && (targetPtr->m_stats.attributes & MONSTER_FLAGS_WIDE)) {
        switch (targetPtr->m_facing) {
            case ARMY_FACING_RIGHT:
                goalHex = goalHex - 1;
                break;
            case ARMY_FACING_LEFT:
                goalHex = goalHex + 1;
                break;
        }
        if (goalHex != -1)
            routeGot = gpSearchArray->FindCombatPath(currentArmy->m_hex, goalHex, currentArmy, -1);
    }
    currentArmy->m_stats.speed = savedSpeed;
    if (gpSearchArray->m_pathLength > 1) {
        giNextAction = ACTION_MOVE;
        left = currentArmy->m_stats.speed;
        pathNdx = gpSearchArray->m_pathLength - 1;
        giNextActionGridIndex = currentArmy->m_hex;
        while (pathNdx >= 1 && left) {
            giNextActionGridIndex =
                currentArmy->GetAdjacentCellIndex(giNextActionGridIndex, gpSearchArray->m_directions[pathNdx]);
            pathNdx--;
            left--;
        }
        return 1;
    }
    return 0;
}
