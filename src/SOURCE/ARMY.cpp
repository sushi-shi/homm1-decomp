// Combat stacks and the computer's combat moves. HoMM1 keeps Buka's
// SOURCE/ARMY methods after the SOURCE/AI combat helpers in one object
// (retail 0x00464520-0x0046ba8f; hero::hero starts the next one).

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

    float prob;
    int treasureValue;
    hero heroCopy;
    hero* sideHero;
    armyGroup* armyPtr;
    int armyIndex;
    int side;
    armyGroup bareGroup;
    int artifactTotals[2];
    float expBonus;
    int force[2];
    float retreatRatio;

    for (side = 0; side < 2; side++) {
        if (m_heroes[side]) {
            heroCopy = *m_heroes[side];
            sideHero = &heroCopy;
            armyPtr = &sideHero->m_army;
        } else {
            armyPtr = &bareGroup;
            sideHero = 0;
        }
        for (armyIndex = 0; armyIndex < 5; armyIndex++) {
            if (m_armies[side][armyIndex].IsAlive()) {
                armyPtr->m_creatureTypes[armyIndex] = m_armies[side][armyIndex].m_creatureType;
                if (m_armies[side][armyIndex].m_stats.attributes & 0x80)
                    armyPtr->m_creatureCounts[armyIndex] = m_armies[side][armyIndex].m_quantity;
                else
                    armyPtr->m_creatureCounts[armyIndex] = (short)(m_armies[side][armyIndex].m_quantity * 1.2);
            } else {
                armyPtr->m_creatureTypes[armyIndex] = -1;
                armyPtr->m_creatureCounts[armyIndex] = 0;
            }
        }
        force[side] = gpPhilAI->FightValueOfStack(armyPtr, sideHero, 1, 0, 0);
        if (m_combatTowns[side])
            force[side] = (int)(force[side] * 1.1);
        artifactTotals[side] = 0;
        if (sideHero) {
            for (armyIndex = 0; armyIndex < 14; armyIndex++) {
                if (sideHero->m_artifacts[armyIndex] >= 0 && sideHero->m_artifacts[armyIndex] < 37)
                    artifactTotals[side] += gArtifactBaseRV[sideHero->m_artifacts[armyIndex]];
            }
        }
    }
    force[1 - m_currentSide] = (int)(force[1 - m_currentSide] * 1.1);
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
    retreatRatio = (float)force[m_currentSide] / (force[0] + force[1]);
    if (retreatRatio < prob) {
        giNextAction = 4;
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
    if (currentArmy->m_stats.attributes & 4) {
        if (currentArmy->m_stats.shots > 0)
            plan = 1;
        else
            plan = 3;
    } else if (currentArmy->m_stats.attributes & 2) {
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
                    giNextAction = 2;
                    giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                    goto finish;
                }
                targetIndex = GetBestArmy(sideEnemy, flyerMasks[sideEnemy]);
                if (targetIndex != -1) {
                    giNextAction = 2;
                    giNextActionGridIndex = m_armies[sideEnemy][targetIndex].m_hex;
                    goto finish;
                }
                if (walkerMask[sideEnemy]) {
                    targetIndex = GetClosestArmy(currentArmy, sideEnemy, walkerMask[sideEnemy]);
                    if (targetIndex != -1) {
                        giNextAction = 2;
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
                if (m_currentSide == 1 && m_castleSide[0] && currentArmy->m_hex % 9 < 4) {
                    targetHex = currentArmy->m_hex / 9 * 9 + 4;
                    hexCell = &gpCombatManager->m_hexCells[targetHex];
                    if (ValidHex(targetHex) && hexCell->m_occupantSide == -1 && hexCell->m_obstacleIndex == -1) {
                        giNextAction = 2;
                        giNextActionGridIndex = targetHex;
                        goto finish;
                    }
                }
            }
            break;
    }
    giNextAction = 3;
finish:
    if (giNextAction == 2 && giNextActionGridIndex > 0 && giNextActionGridIndex <= 43
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
    short armyMask = 0;
    class army* army;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & 0x10) && (army->m_stats.attributes & 4)
            && army->m_stats.shots > 0)
            armyMask |= bitMask;
        bitMask <<= 1;
    }
    return armyMask;
}

VA(0x0046573b, 0xbb)
short combatManager::GetFlyerMask(signed char side) {
    short armyIndex = 0;
    short bitMask = 1;
    short armyMask = 0;
    class army* army;

    for (armyIndex = 0; armyIndex < m_numArmies[side]; armyIndex++) {
        army = &m_armies[side][armyIndex];
        if (army && !(army->m_stats.attributes & 0x10) && (army->m_stats.attributes & 2))
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
        if (army && !(army->m_stats.attributes & 0x10) && !(army->m_stats.attributes & 2)
            && (!(army->m_stats.attributes & 4) || army->m_stats.shots <= 0))
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

    for (armyIndex = 0; armyIndex < 5; armyIndex++) {
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

    for (armyIndex = 0; armyIndex < 5; armyIndex++) {
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
    short armyIndex = 0;
    army* target;
    short bitFlag = 1;
    int closestDist = 640;
    short bestArmy = -1;
    int val;

    for (armyIndex = 0; armyIndex < 5; armyIndex++) {
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
            if (army && !(army->m_stats.attributes & 0x10))
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
        if (currentArmy->m_creatureType == 26)
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
            giNextAction = 2;
            giNextActionGridIndex = targetHex;
            return 1;
        }
        if (m_armies[side][targetArmy].m_stats.attributes & 1) {
            if (m_armies[side][targetArmy].m_facing == 1)
                targetHex--;
            else
                targetHex++;
            currentArmy->m_moveTargetHex = targetHex;
            if (currentArmy->ValidPath(targetHex, 0)) {
                giNextAction = 2;
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
    for (dir = 0; dir < 8; dir++) {
        if (openMask & oneBit) {
            hex = currentArmy->GetAdjacentCellIndex(currentArmy->m_hex, dir);
            if (ValidHex(hex) && (currentArmy->m_stats.attributes & 1)
                    && m_hexCells[hex].m_occupantSide != 1 - m_currentSide
                || m_hexCells[hex].m_occupantIndex == m_currentArmyIndex
                    && m_hexCells[hex].m_occupantSide == m_currentSide) {
                if (currentArmy->m_facing == 0)
                    otherHex = currentArmy->m_hex + 1;
                else
                    otherHex = currentArmy->m_hex - 1;
                if (hex % 9 != 0 && hex % 9 != 8)
                    hex = currentArmy->GetAdjacentCellIndex(otherHex, dir);
                if (m_hexCells[hex].m_occupantSide != 1 - m_currentSide)
                    hex = -1;
            }
            if (hex >= 0)
                enemyMask |= 1 << m_hexCells[hex].m_occupantIndex;
        }
        oneBit <<= 1;
    }
    if (currentArmy->m_creatureType == 26)
        target = GetWorstArmy(1 - m_currentSide, enemyMask);
    else
        target = GetBestArmy(1 - m_currentSide, enemyMask);
    if (target != -1) {
        giNextAction = 2;
        giNextActionGridIndex = m_armies[1 - m_currentSide][target].m_hex;
        return 1;
    } else {
        return 0;
    }
}

VA(0x00466050, 0x20f)
signed char combatManager::WalkTowardArmyFront(class army* currentArmy, signed char side, short mask) {
    int armyIndex;
    int frontDelta;
    int canReach;
    short frontHex;
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
    if (m_armies[side][armyIndex].m_stats.attributes & 1)
        frontDelta = 2;
    if (currentArmy->m_facing == 0)
        frontHex = frontHex + frontDelta;
    else
        frontHex = frontHex + -frontDelta;
    if (frontHex % 9 == 8 || frontHex % 9 == 0)
        return WalkTowardArmy(currentArmy, side, mask);
    oldSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = 127;
    canReach = gpSearchArray->FindCombatPath(currentArmy->m_hex, frontHex, currentArmy, 1);
    currentArmy->m_stats.speed = oldSpeed;
    if (gpSearchArray->m_pathLength > 0) {
        giNextAction = 2;
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
        giNextAction = 3;
        return 1;
    }
    savedSpeed = currentArmy->m_stats.speed;
    currentArmy->m_stats.speed = 127;
    routeGot = gpSearchArray->FindCombatPath(currentArmy->m_hex, goalHex, currentArmy, -1);
    if (!routeGot && (targetPtr->m_stats.attributes & 1)) {
        switch (targetPtr->m_facing) {
            case 1:
                goalHex = goalHex - 1;
                break;
            case 0:
                goalHex = goalHex + 1;
                break;
        }
        if (goalHex != -1)
            routeGot = gpSearchArray->FindCombatPath(currentArmy->m_hex, goalHex, currentArmy, -1);
    }
    currentArmy->m_stats.speed = savedSpeed;
    if (gpSearchArray->m_pathLength > 1) {
        giNextAction = 2;
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

VA(0x00466490, 0xc9)
army::army(void) {
    int i;

    m_unknown08 = 0;
    m_unknown09 = 0;
    m_attackIcon = 0;
    m_walkIcon = 0;
    m_standIcon = 0;
    m_hex = 0;
    for (i = 0; i < 4; i++)
        m_samples[i] = 0;
    m_unknown2f = -1;
    m_unknown33 = 1;
    gCurLoadedSpellIcon = 0;
    gCurLoadedSpellFileId = 0;
    giSpellEffectFrame = 0;
    m_targetSide = -1;
    m_targetIndex = -1;
    m_attackDirection = -1;
    m_unknown04 = 0;
    m_moveTargetHex = 0;
}

// The Windows build waits on no sample channel.
VA(0x00466559, 0x18)
void army::WaitSample(int) {
    return;
}

VA(0x00466571, 0x71)
void army::InitClean(void) {
    int i;

    for (i = 0; i < 4; i++)
        m_samples[i] = 0;
    m_unknown2f = -1;
    m_unknown33 = 1;
    m_attackIcon = 0;
    m_walkIcon = 0;
    m_standIcon = 0;
}

// The commanding hero's attack and defense raise the copied creature stats.
VA(0x004665e2, 0x122)
void army::Init(signed char type, short quantity, signed char side, signed char index) {
    hero* commander;

    InitClean();
    m_creatureType = type;
    memcpy(&m_stats, &gMonsterDatabase[type].stats, sizeof(tag_monsterStats));
    m_unknown29 = 6;
    m_spellEffect = -1;
    m_unknown52 = -1;
    commander = gpCombatManager->m_heroes[side];
    if (commander) {
        m_stats.attack = commander->m_primaryStats[0] + m_stats.attack;
        m_stats.defense = commander->m_primaryStats[1] + m_stats.defense;
    }
    m_facing = side ^ 1;
    m_unknown0b = 0;
    m_unknown08 = 0;
    m_unknown09 = 1;
    m_baseSpeed = m_stats.speed;
    m_quantity = quantity;
    m_initialQuantity = m_quantity;
    m_hitPointsLost = 0;
    m_damageMode = 0;
    m_powFrames = -1;
    m_side = side;
    m_index = index;
}

VA(0x00466704, 0x237)
void army::LoadResources(void) {
    char sprite[16];
    int i;
    char buf[16];

    if (m_creatureType != 3)
        strcpy(sprite, gArmyNames[m_creatureType]);
    else
        strcpy(sprite, "swrdsman");
    sprintf(gText, "%s.std", sprite);
    giMonoIconSkip = 0;
    m_standIcon = gpResourceManager->GetIcon(gText);
    giMonoIconSkip = -1;
    sprintf(gText, "%s.wlk", sprite);
    m_walkIcon = gpResourceManager->GetIcon(gText);
    sprintf(gText, "move%02d.82M", m_creatureType);
    m_samples[0] = gpResourceManager->GetSample(gText);
    sprintf(gText, "atksnd%02d.82M", m_creatureType);
    m_samples[1] = gpResourceManager->GetSample(gText);
    sprintf(gText, "wince%02d.82M", m_creatureType);
    m_samples[2] = gpResourceManager->GetSample(gText);
    if (m_stats.attributes & 4) {
        sprintf(gText, "%s.atk", sprite);
        m_attackIcon = gpResourceManager->GetIcon(gText);
        sprintf(gText, "shoot%02d.82M", m_creatureType);
        m_samples[3] = gpResourceManager->GetSample(gText);
    } else {
        m_attackIcon = 0;
        m_samples[3] = 0;
    }
    for (i = 0; i < 4; i++) {
        if (m_samples[i]) {
            m_samples[i]->m_playbackData.volume = 0x40;
            m_samples[i]->m_playbackData.channelType = 3;
            m_samples[i]->m_playbackData.loopCount = 1;
        }
    }
}

// donor PoL RVA 0x0004b36e; preferred Buka symbol ?FreeResources@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463954;margin=0.140795;shape=0.318;size=0.842;calls=0.750;alternate=pol20:void army::FreeResources(void)@0x0004b36e
VA(0x0046693b, 0xf1)
void army::FreeResources(void) {
    int i;

    if (m_standIcon) {
        gpResourceManager->Dispose(m_standIcon);
        m_standIcon = 0;
    }
    if (m_walkIcon) {
        gpResourceManager->Dispose(m_walkIcon);
        m_walkIcon = 0;
    }
    if ((m_stats.attributes & 4) && m_attackIcon) {
        gpResourceManager->Dispose(m_attackIcon);
        m_attackIcon = 0;
    }
    for (i = 0; i < 4; i++) {
        if (m_samples[i]) {
            gpResourceManager->Dispose(m_samples[i]);
            m_samples[i] = 0;
        }
    }
}

// m_unknown08 selects the stand, walk, attack or spell-effect pose and
// m_unknown09 its frame; m_unknown33 adds the shadow frames.
VA(0x00466a2c, 0x855)
void army::DrawToBuffer(short x, short y) {
    short effectX;
    signed char flip;
    signed char outlined;
    short outlineColor;
    char countText[12];
    short iconX;
    short qtyX;

    flip = 0;
    outlined = 0;
    if ((m_unknown09 == 2 || m_unknown09 >= 3)
        && ((m_stats.attributes & 1) && (m_hex % 9 <= 2 || m_hex % 9 >= 6)
            || !(m_stats.attributes & 1) && (m_hex % 9 <= 1 || m_hex % 9 >= 7)))
        gbUseClippedIconRenderer = 1;
    if (m_unknown0b) {
        y += m_unknown09 * m_unknown0b;
        if (m_unknown09 > 0 && m_unknown09 <= 5)
            flip = 1;
    }
    switch (m_unknown08) {
        case 0:
            switch (m_creatureType) {
                case 22:
                    if (m_unknown33)
                        m_standIcon->DimToBuffer(x, y, m_unknown09 + 8, m_facing, flip);
                    break;
                case 11:
                case 17:
                case 23:
                    if (m_unknown33)
                        m_standIcon->DimToBuffer(x, y, m_unknown09 + 15, m_facing, flip);
                    break;
                default:
                    if (m_unknown33)
                        m_standIcon->DimToBuffer(x, y, m_unknown09 + 9, m_facing, flip);
                    break;
            }
            if (m_unknown09 > 4 && m_creatureType != 22)
                m_standIcon->DrawToBuffer(x, y, 5, m_facing, flip);
            m_standIcon->DrawToBuffer(x, y, m_unknown09, m_facing, flip);
            if (m_hex == gpCombatManager->m_limitCreatureHex && gpCombatManager->m_limitCreature == 1) {
                m_standIcon->FillToBuffer(x, y, 0, 0xe4, m_facing, flip);
                outlined = 1;
            }
            if (m_spellEffect != -1) {
                switch (m_spellEffect) {
                    case 5:
                    case 8:
                    case 9:
                    case 12:
                        outlineColor = 0xf7;
                        break;
                    default:
                        outlineColor = 0xe0;
                        break;
                }
                if (!outlined && m_unknown09 == 1)
                    m_standIcon->FillToBuffer(x, y, 0, outlineColor, m_facing, flip);
                iconX = x;
                if (m_stats.attributes & 1) {
                    if (m_facing == 0)
                        iconX += 75;
                    else
                        iconX -= 95;
                } else if (m_facing == 1) {
                    iconX -= 39;
                }
                gpCombatManager->m_combatIcons[8]->DrawToBuffer(iconX, y - 40, m_spellEffect, 0, 0);
            }
            if (m_unknown09 == 1 && gpCombatManager->m_unknown6c0) {
                if (m_stats.attributes & 1) {
                    if (m_facing == 0)
                        qtyX = x + 75;
                    else
                        qtyX = x - 95;
                } else {
                    if (m_facing == 0)
                        qtyX = x + 8;
                    else
                        qtyX = x - 39;
                }
                gpCombatManager->m_combatIcons[1]->DrawToBuffer(qtyX, y - 11, 5, 0, 0);
                sprintf(countText, "%d", m_quantity);
                gpCombatManager->m_smallFont->DrawBoundedString(countText, qtyX, y - 12, 20, 12, 1, 1);
            }
            break;
        case 1:
            if (m_unknown33)
                m_walkIcon->DimToBuffer(x, y, m_unknown09 + 6, m_facing, flip);
            m_walkIcon->DrawToBuffer(x, y, m_unknown09, m_facing, flip);
            break;
        case 2:
            if (m_unknown09 < 5) {
                if (m_unknown33)
                    m_attackIcon->DimToBuffer(x, y, m_unknown09 + 9, m_facing, flip);
                m_attackIcon->DrawToBuffer(x, y, 0, m_facing, flip);
            }
            m_attackIcon->DrawToBuffer(x, y, m_unknown09, m_facing, flip);
            break;
        case 3:
            if (!(m_stats.attributes & 0x10)) {
                switch (m_creatureType) {
                    case 22:
                        if (m_unknown33)
                            m_standIcon->DimToBuffer(x, y, m_unknown09 + 8, m_facing, flip);
                        break;
                    case 11:
                    case 17:
                    case 23:
                        if (m_unknown33)
                            m_standIcon->DimToBuffer(x, y, m_unknown09 + 15, m_facing, flip);
                        break;
                    default:
                        if (m_unknown33)
                            m_standIcon->DimToBuffer(x, y, m_unknown09 + 9, m_facing, flip);
                        break;
                }
                m_standIcon->DrawToBuffer(x, y, m_unknown09, m_facing, flip);
            }
            effectX = x;
            if (m_stats.attributes & 1) {
                if (m_facing == 0) {
                    effectX += 39;
                    x += 75;
                } else {
                    effectX -= 39;
                    x -= 95;
                }
            } else if (m_facing == 1) {
                x -= 39;
            }
            if (m_spellEffect != -1)
                gpCombatManager->m_combatIcons[8]->DrawToBuffer(x, y - 40, m_spellEffect, 0, 0);
            gCurLoadedSpellIcon->DrawToBuffer(effectX, y, giSpellEffectFrame, m_facing, flip);
            break;
    }
    gbUseClippedIconRenderer = 0;
}

VA(0x00467281, 0x63)
void army::Stand(signed char redraw) {
    m_unknown08 = 0;
    m_unknown09 = 1;
    m_unknown0b = 0;
    gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    if (redraw)
        gpCombatManager->DrawFrame(1);
}

VA(0x004672e4, 0x61)
void army::Wince(void) {
    m_unknown08 = 0;
    m_unknown09 = 2;
    m_unknown0b = 0;
    gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    gpCombatManager->SetGridMode(m_facing != 0);
}

// One hex of walking: six frames redrawn inside the union of the old and
// new extents; a stack turned away from the step moves before animating.
VA(0x00467345, 0x852)
void army::Walk(short direction, signed char standAfter, signed char continued) {
    int rectMaxX;
    int rectMaxY;
    short moveDist;
    short startFrame;
    short step;
    short reverse;
    short i;
    short baseHex;
    int rectMinX;
    short flag;
    int partnerHex;
    short targetHex;
    int rectMinY;
    int nextTail;

    if (!continued) {
        giMinExtentX = giMinExtentY = 640;
        giMaxExtentX = giMaxExtentY = 0;
        gbComputeExtent = 1;
        gbSaveBiggestExtent = 1;
        DrawToBuffer(gpCombatManager->m_hexCells[m_hex].m_x, gpCombatManager->m_hexCells[m_hex].m_y);
        gbSaveBiggestExtent = 0;
        gbComputeExtent = 0;
    }
    if (giMinExtentX < 0)
        giMinExtentX = 0;
    if (giMinExtentY < 0)
        giMinExtentY = 0;
    if (giMaxExtentX > 639)
        giMaxExtentX = 639;
    if (giMaxExtentY > 459)
        giMaxExtentY = 459;
    rectMinX = giMinExtentX - 5;
    rectMinY = giMinExtentY - 5;
    rectMaxX = giMaxExtentX + 5;
    rectMaxY = giMaxExtentY + 5;
    moveDist = 16;
    reverse = 0;
    m_unknown0b = 0;
    if (direction < 3) {
        if (m_facing == 0) {
            startFrame = 0;
            step = 1;
        } else {
            startFrame = 5;
            step = -1;
            reverse = 1;
        }
    } else if (m_facing == 1) {
        startFrame = 0;
        step = 1;
    } else {
        startFrame = 5;
        step = -1;
        reverse = 1;
    }
    if (direction == 5 || direction == 0)
        m_unknown0b = -16;
    if (direction == 3 || direction == 2)
        m_unknown0b = 16;
    baseHex = m_hex;
    hexcell tempCell;
    hexcell tailCell;
    if (reverse) {
        targetHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(targetHex)) {
            tempCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
            if (m_stats.attributes & 1)
                tailCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex + (m_facing ? -1 : 1)]);
            gpCombatManager->m_hexCells[targetHex].TakeOccupant(&tempCell);
            partnerHex = targetHex + (m_facing ? -1 : 1);
            if (ValidHex(partnerHex) && (m_stats.attributes & 1))
                gpCombatManager->m_hexCells[partnerHex].TakeOccupant(&tailCell);
            m_hex = targetHex;
            if (m_unknown0b)
                m_unknown0b = -m_unknown0b;
        }
    } else {
        flag = 0;
        if (m_facing == 0 && direction == 2)
            flag = 1;
        else if (m_facing == 1 && direction == 5)
            flag = 1;
        gpCombatManager->SetGridMode(flag);
    }
    m_unknown08 = 1;
    m_unknown09 = startFrame;
    gpSoundManager->MemorySample(m_samples[0]);
    if (!continued) {
        if (ValidHex(m_hex))
            gpCombatManager->m_hexCells[m_hex].m_occupantSide = -1;
        gpCombatManager->DrawFrame(0);
        if (ValidHex(m_hex))
            gpCombatManager->m_hexCells[m_hex].m_occupantSide = gpCombatManager->m_currentSide;
        gpWindowManager->m_screen->CopyTo(gpCombatManager->m_backgroundBuffer, 0, 0, 0, 0, 640, 460);
        gpCombatManager->m_backgroundDrawn = 0;
    }
    for (i = 0; i < 6; i++) {
        if (continued || i) {
            gpCombatManager->m_backgroundBuffer->CopyTo(gpWindowManager->m_screen, giMinExtentX, giMinExtentY,
                                                        giMinExtentX, giMinExtentY,
                                                        giMaxExtentX - giMinExtentX + 1,
                                                        giMaxExtentY - giMinExtentY + 1);
            rectMinX = giMinExtentX;
            rectMinY = giMinExtentY;
            rectMaxX = giMaxExtentX;
            rectMaxY = giMaxExtentY;
        }
        giMinExtentX = giMinExtentY = 640;
        giMaxExtentX = giMaxExtentY = 0;
        gbComputeExtent = 1;
        gbSaveBiggestExtent = 1;
        DrawToBuffer(gpCombatManager->m_hexCells[m_hex].m_x, gpCombatManager->m_hexCells[m_hex].m_y);
        gbComputeExtent = 0;
        gbSaveBiggestExtent = 0;
        if (giMinExtentX < 0)
            giMinExtentX = 0;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentX > 639)
            giMaxExtentX = 639;
        if (giMaxExtentY > 459)
            giMaxExtentY = 459;
        gbCurrArmyDrawn = 0;
        gbComputeExtent = 1;
        gbLimitToExtent = 1;
        m_unknown33 = 0;
        gpCombatManager->DrawFrame(0);
        m_unknown33 = 1;
        gbLimitToExtent = 0;
        gbComputeExtent = 0;
        gbCurrArmyDrawn = 1;
        if (giMinExtentX < rectMinX)
            rectMinX = giMinExtentX;
        if (giMinExtentY < rectMinY)
            rectMinY = giMinExtentY;
        if (giMaxExtentX > rectMaxX)
            rectMaxX = giMaxExtentX;
        if (giMaxExtentY > rectMaxY)
            rectMaxY = giMaxExtentY;
        DelayTil(glTimers);
        glTimers[0] = KBTickCount() + 75;
        gpWindowManager->UpdateScreenRegion(rectMinX, rectMinY, rectMaxX - rectMinX + 1, rectMaxY - rectMinY + 1);
        m_unknown09 += step;
    }
    if (!reverse) {
        targetHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(targetHex)) {
            tempCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
            nextTail = m_hex + (m_facing ? -1 : 1);
            if ((m_stats.attributes & 1) && ValidHex(nextTail))
                tailCell.TakeOccupant(&gpCombatManager->m_hexCells[nextTail]);
            gpCombatManager->m_hexCells[targetHex].TakeOccupant(&tempCell);
            nextTail = targetHex + (m_facing ? -1 : 1);
            if ((m_stats.attributes & 1) && ValidHex(nextTail))
                gpCombatManager->m_hexCells[nextTail].TakeOccupant(&tailCell);
            m_hex = targetHex;
        }
    }
    if (standAfter == 1)
        Stand(1);
}

// A ranged attack: turn toward the target, animate the missile hex by hex
// over a saved screen patch, apply wall and luck modifiers, report the
// damage; creature 14 shoots twice.
VA(0x00467b97, 0xcca)
void army::SpecialAttack(void) {
    int targetHexCol;
    int dmg;
    int xEnd;
    signed char arrowFrame;
    int firstX;
    int destY;
    int killCount;
    army* target;
    int startY;
    int destX;
    int startX;
    int maxY;
    bitmap* saved;
    int iMaxX;
    int j;
    signed char faceLeft;
    int facing;
    int dy;
    int i;
    int dx;
    int yStep;
    int posY;
    int steps;
    int yOffset[5];
    int xStep;
    int y2;
    int posX;
    signed char myRow;
    int y0;
    int prevY;
    signed char targetRow;
    signed char srcCol;
    signed char pitchSign;
    int offY;
    int minY;
    int prevX;
    signed char tgtCol;
    int offX;
    int minX;
    signed char inCastle;

    facing = m_facing;
    m_unknown0b = 0;
    if (m_targetSide < 0 || m_targetIndex < 0)
        return;
    target = &gpCombatManager->m_armies[m_targetSide][m_targetIndex];
    tgtCol = target->m_hex % 9;
    targetRow = target->m_hex / 9;
    srcCol = m_hex % 9;
    myRow = m_hex / 9;
    facing = m_facing;
    if (tgtCol > srcCol || !(myRow & 1) && tgtCol == srcCol)
        m_facing = 0;
    else
        m_facing = 1;
    gpCombatManager->SetGridMode(m_facing == 0);
    CheckLuck();
    m_unknown08 = 2;
    gpSoundManager->MemorySample(m_samples[3]);
    for (i = 0; i < 4; i++) {
        m_unknown09 = i + 1;
        gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
        gpCombatManager->DrawFrame(1);
    }
    targetHexCol = tgtCol;
    if (target->m_stats.attributes & 1) {
        if (target->m_facing == 1)
            targetHexCol--;
        else
            targetHexCol++;
    }
    dx = targetHexCol - srcCol;
    faceLeft = 0;
    if (dx < 0) {
        faceLeft = 1;
        dx = -dx;
    }
    dy = targetRow - myRow;
    if (dy < 0)
        dy = -dy;
    steps = dy > dx ? dy : dx;
    arrowFrame = 7;
    pitchSign = 0;
    if (targetRow < myRow)
        pitchSign = -1;
    else if (targetRow > myRow)
        pitchSign = 1;
    if (dy > 1)
        arrowFrame += pitchSign;
    if (dx <= 3) {
        if (dy > 2)
            arrowFrame += pitchSign;
        if (dy == 1)
            arrowFrame += pitchSign;
    }
    yOffset[0] = -20;
    yOffset[1] = -15;
    yOffset[2] = 0;
    yOffset[3] = 15;
    yOffset[4] = 20;
    startX = gpCombatManager->m_hexCells[m_hex].m_x + (m_facing == 0 ? 80 : -80);
    startY = gpCombatManager->m_hexCells[m_hex].m_y - 90 + yOffset[arrowFrame - 5];
    destX = gpCombatManager->m_hexCells[targetRow * 9 + targetHexCol].m_x;
    destY = gpCombatManager->m_hexCells[targetRow * 9].m_y - 90;
    if (dx == 0)
        xStep = 0;
    else
        xStep = (destX - startX) / (steps * 2);
    if (dy == 0)
        yStep = 0;
    else
        yStep = (destY - startY) / (steps * 2);
    firstX = xStep + startX;
    xEnd = destX - xStep * steps * 2;
    offX = (firstX + xEnd) / 2 - firstX;
    y0 = yStep + startY;
    y2 = destY - steps * yStep * 2;
    offY = (y0 + y2) / 2 - y0;
    posX = offX + startX;
    posY = offY + startY;
    iMaxX = 0;
    minX = 639;
    maxY = 0;
    minY = 479;
    saved = new bitmap(33, 70, 60);
    saved->GrabBitmap(gpWindowManager->m_screen, posX - 35, posY - 30);
    prevX = posX;
    prevY = posY;
    for (j = 0; j < steps * 2; j++) {
        saved->DrawToBuffer(prevX - 35, prevY - 30);
        if (prevX - 35 < minX)
            minX = prevX - 35;
        if (prevX + 35 > iMaxX)
            iMaxX = prevX + 35;
        if (prevY - 30 < minY)
            minY = prevY - 30;
        if (prevY + 30 > maxY)
            maxY = prevY + 30;
        saved->GrabBitmap(gpWindowManager->m_screen, posX - 35, posY - 30);
        m_attackIcon->DrawToBuffer(posX, posY, arrowFrame, faceLeft, 0);
        if (posX - 35 < minX)
            minX = posX - 35;
        if (posX + 35 > iMaxX)
            iMaxX = posX + 35;
        if (posY - 30 < minY)
            minY = posY - 30;
        if (posY + 30 > maxY)
            maxY = posY + 30;
        DelayTil(glTimers);
        gpWindowManager->UpdateScreenRegion(minX, minY, iMaxX - minX + 1, maxY - minY + 1);
        glTimers[0] = KBTickCount() + 15;
        prevX = posX;
        prevY = posY;
        posX += xStep;
        posY += yStep;
    }
    saved->DrawToBuffer(prevX - 35, prevY - 30);
    gpWindowManager->UpdateScreenRegion(prevX - 35, prevY - 30, 70, 60);
    delete saved;
    m_stats.shots--;
    inCastle = 0;
    if (gpCombatManager->m_castleSide[0] && m_hex % 9 <= 4 && target->m_hex % 9 >= 6) {
        int gateHex;
        int hitRow;
        int colDist;
        int myR;
        int sCol;
        int targetR;
        int tgtC;
        int wallDist;

        sCol = m_hex % 9;
        myR = m_hex / 9;
        colDist = sCol - 5;
        tgtC = target->m_hex % 9;
        targetR = target->m_hex / 9;
        wallDist = 5 - sCol;
        hitRow = targetR;
        if (abs(targetR - myR) >= 2)
            hitRow -= -(-((targetR - myR) / 2));
        if (abs(targetR - myR) % 2 == 1) {
            if (colDist < wallDist || colDist == wallDist && (myR == 1 || myR == 3)) {
                if (myR < targetR)
                    hitRow--;
                else
                    hitRow++;
            }
        }
        if (hitRow > 4)
            hitRow = 4;
        if (hitRow < 0)
            hitRow = 0;
        if (gpCombatManager->m_hexCells[hitRow * 9 + 5].m_obstacleIndex == 10
            || gpCombatManager->m_hexCells[hitRow * 9 + 5].m_obstacleIndex == 8)
            inCastle = 1;
        else
            inCastle = 0;
    }
    DamageEnemy(target, &dmg, &killCount, 1, inCastle ? 4 : 0);
    if (killCount > 0)
        sprintf(gText, "%s %s %d %s.  %d %s %s.",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "do" : "does", dmg, "Damage", killCount,
                killCount > 1 ? gArmyNamesPlural[target->m_creatureType] : gArmyNames[target->m_creatureType],
                killCount > 1 ? "perish" : "perishes");
    else
        sprintf(gText, "%s %s %d %s.",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "do" : "does", dmg, "Damage");
    gText[0] -= 32;
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.unknown07);
    if (!(target->m_stats.attributes & 0x10))
        target->Stand(0);
    if (m_unknown52 == 1)
        CancelSpell();
    WaitSample(3);
    m_facing = facing;
    Stand(1);
    if (target->m_quantity > 0)
        target->Stand(1);
    if (!gbSecondShot && m_creatureType == 14 && target->m_quantity > 0) {
        gbSecondShot = 1;
        SpecialAttack();
        gbSecondShot = 0;
    }
}

// Attacks every enemy next to the stack (the hydra), then turns them back.
VA(0x00468861, 0x765)
void army::DoHydraAttack(void) {
    int killedNow;
    int damage;
    short armyIndex;
    short i;
    short dir;
    short attackMask;
    short occSide;
    army* pTarget;
    short targetHex;
    int totalLost;
    int totDmg;
    army* eachArmy;

    m_unknown0b = 0;
    CheckLuck();
    gpSoundManager->MemorySample(m_samples[1]);
    m_unknown08 = 0;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    for (i = 0; i < 5; i++) {
        gpCombatManager->m_unknown727 = 1;
        m_unknown09 = i + 3;
        gpCombatManager->DrawFrame(1);
    }
    if (m_spellEffect == 14)
        attackMask = GetAttackMask(m_hex, 2, -1);
    else
        attackMask = GetAttackMask(m_hex, 1, -1);
    gpCombatManager->ResetHitByCreature();
    totalLost = 0;
    totDmg = totalLost;
    for (dir = 0; dir < 8; dir++) {
        if (!(attackMask & (1 << dir))) {
            targetHex = m_hex;
            if ((m_stats.attributes & 1)
                && (m_facing == 1 && dir > 2 || m_facing == 0 && (dir < 3 || dir > 5))) {
                if (m_facing == 1)
                    targetHex = m_hex - 1;
                else
                    targetHex = m_hex + 1;
            }
            targetHex = GetAdjacentCellIndex(targetHex, dir);
            if (ValidHex(targetHex)) {
                occSide = gpCombatManager->m_hexCells[targetHex].m_occupantSide;
                armyIndex = gpCombatManager->m_hexCells[targetHex].m_occupantIndex;
                if (occSide >= 0 && armyIndex >= 0) {
                    gpCombatManager->m_limitCreatureCount[occSide][armyIndex]++;
                    pTarget = &gpCombatManager->m_armies[occSide][armyIndex];
                    if (!pTarget->m_hitByCreature) {
                        pTarget->m_hitByCreature = 1;
                        DamageEnemy(pTarget, &damage, &killedNow, 0, 0);
                        totDmg += damage;
                        totalLost += killedNow;
                    }
                }
            }
        }
    }
    if (totalLost > 0)
        sprintf(gText, "%s %s %d %s.  %d %s %s.",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "do" : "does", totDmg, "Damage", totalLost,
                totalLost > 1 ? "creatures" : "creature", totalLost > 1 ? "perish" : "perishes");
    else
        sprintf(gText, "%s %s %d %s.",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "do" : "does", totDmg, "Damage");
    gText[0] -= 32;
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.unknown07);
    WaitSample(1);
    m_unknown08 = 0;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    for (i = 4; i >= 0; i--) {
        gpCombatManager->m_unknown727 = 1;
        m_unknown09 = i + 3;
        gpCombatManager->DrawFrame(1);
    }
    if (m_spellEffect == 14)
        attackMask = GetAttackMask(m_hex, 2, -1);
    else
        attackMask = GetAttackMask(m_hex, 1, -1);
    for (dir = 0; dir < 8; dir++) {
        if (!(attackMask & (1 << dir))) {
            targetHex = m_hex;
            if ((m_stats.attributes & 1)
                && (m_facing == 1 && dir > 2 || m_facing == 0 && (dir < 3 || dir > 5))) {
                if (m_facing == 1)
                    targetHex = m_hex - 1;
                else
                    targetHex = m_hex + 1;
            }
            targetHex = GetAdjacentCellIndex(targetHex, dir);
            if (ValidHex(targetHex)) {
                occSide = gpCombatManager->m_hexCells[targetHex].m_occupantSide;
                armyIndex = gpCombatManager->m_hexCells[targetHex].m_occupantIndex;
                if (occSide >= 0 && armyIndex >= 0) {
                    gpCombatManager->m_limitCreatureCount[occSide][armyIndex]++;
                    pTarget = &gpCombatManager->m_armies[occSide][armyIndex];
                    if (!(pTarget->m_stats.attributes & 0x10))
                        pTarget->Stand(0);
                }
            }
        }
    }
    m_targetSide = targetHex = -1;
    gpCombatManager->m_unknown727 = 0;
    for (occSide = 0; occSide < 2; occSide++) {
        for (armyIndex = 0; armyIndex < gpCombatManager->m_numArmies[occSide]; armyIndex++) {
            eachArmy = &gpCombatManager->m_armies[occSide][armyIndex];
            if (!(eachArmy->m_stats.attributes & 0x10) || eachArmy->m_powFrames == -1)
                eachArmy->Stand(0);
        }
    }
    gpCombatManager->DrawFrame(1);
}

VA(0x00468fc6, 0x2d)
void army::DirDoAttack(short direction) {
    m_attackDirection = direction;
    DoAttack(0);
}

// A melee strike in m_attackDirection: breath attackers (attribute 8) also
// hit the hex behind, some creatures cast on the target, the target
// retaliates once, and creatures 5 and 8 strike twice.
VA(0x00468ff3, 0x108d)
void army::DoAttack(int retaliation) {
    int unused;
    int oldMode;
    short frameBase;
    army* target2;
    int curDir;
    short facing;
    int attackDir;
    int didCast;
    army* target;
    int dmg;
    short newHex;
    int kills;

    oldMode = 0;
    dmg = 0;
    kills = 0;
    didCast = 0;
    if (retaliation)
        gpCombatManager->m_currentSide = 1 - gpCombatManager->m_currentSide;
    if (m_creatureType == 22) {
        DoHydraAttack();
        if (m_unknown52 == 1 && !retaliation)
            CancelSpell();
        goto secondStrike;
    }
    attackDir = m_attackDirection;
    facing = m_facing;
    m_unknown0b = 0;
    if (m_attackDirection <= 2)
        m_facing = 0;
    else if (m_attackDirection <= 5)
        m_facing = 1;
    if (m_attackDirection == 5 || m_attackDirection == 0 || m_attackDirection == 6)
        frameBase = 6;
    else if (m_attackDirection == 3 || m_attackDirection == 2 || m_attackDirection == 7)
        frameBase = 8;
    else
        frameBase = 7;
    gpCombatManager->SetGridMode(m_facing == 0);
    CheckLuck();
    newHex = m_hex;
    if ((m_stats.attributes & 1)
        && (facing == 1 && m_attackDirection >= 3
            || facing == 0 && (m_attackDirection <= 2 || m_attackDirection >= 6))) {
        if (facing == 1)
            newHex = m_hex - 1;
        else
            newHex = m_hex + 1;
    }
    newHex = GetAdjacentCellIndex(newHex, m_attackDirection);
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    if (m_stats.attributes & 8) {
        short behindHex;

        if (ValidHex(newHex) && gpCombatManager->m_hexCells[newHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[newHex].m_occupantIndex >= 0)
            gpCombatManager->m_limitCreatureCount[gpCombatManager->m_hexCells[newHex].m_occupantSide]
                                                  [gpCombatManager->m_hexCells[newHex].m_occupantIndex]++;
        behindHex = GetAdjacentCellIndex(newHex, m_attackDirection);
        if (ValidHex(behindHex) && gpCombatManager->m_hexCells[behindHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[behindHex].m_occupantIndex >= 0) {
            gpCombatManager->m_limitCreatureCount[gpCombatManager->m_hexCells[behindHex].m_occupantSide]
                                                  [gpCombatManager->m_hexCells[behindHex].m_occupantIndex]++;
            if (m_attackDirection == 2 || m_attackDirection == 3)
                gpCombatManager->m_unknown260 = 1;
        }
    }
    oldMode = gpCombatManager->m_unknown260;
    m_unknown08 = 0;
    m_unknown09 = 3;
    gpCombatManager->DrawFrame(1);
    glTimers[0] = KBTickCount() + 105;
    gpSoundManager->MemorySample(m_samples[1]);
    m_unknown09 = 4;
    gpCombatManager->m_unknown727 = 1;
    gpCombatManager->DrawFrame(1);
    glTimers[0] = KBTickCount() + 105;
    m_unknown09 = frameBase;
    gpCombatManager->m_unknown727 = 1;
    gpCombatManager->DrawFrame(1);
    glTimers[0] = KBTickCount() + 105;
    if (m_stats.attributes & 8) {
        m_unknown09 = frameBase + 3;
        gpCombatManager->m_unknown727 = 1;
        gpCombatManager->DrawFrame(1);
        glTimers[0] = KBTickCount() + 105;
    }
    target2 = 0;
    target = 0;
    if (ValidHex(newHex)) {
        int savedKilled;
        short nextHex;

        if (gpCombatManager->m_hexCells[newHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[newHex].m_occupantIndex >= 0) {
            target = &gpCombatManager->m_armies[gpCombatManager->m_hexCells[newHex].m_occupantSide]
                                               [gpCombatManager->m_hexCells[newHex].m_occupantIndex];
            gpCombatManager->m_limitCreatureCount[target->m_side][target->m_index]++;
            gpCombatManager->m_unknown727 = 1;
            DamageEnemy(target, &dmg, &kills, 0, 0);
        }
        savedKilled = kills;
        nextHex = GetAdjacentCellIndex(newHex, m_attackDirection);
        if ((m_stats.attributes & 8) && m_attackDirection < 6 && ValidHex(nextHex)
            && gpCombatManager->m_hexCells[nextHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[nextHex].m_occupantIndex >= 0
            && gpCombatManager->m_hexCells[newHex].m_occupantIndex
                   != gpCombatManager->m_hexCells[nextHex].m_occupantIndex) {
            gpCombatManager->m_limitCreatureCount[gpCombatManager->m_hexCells[nextHex].m_occupantSide]
                                                  [gpCombatManager->m_hexCells[nextHex].m_occupantIndex]++;
            newHex = nextHex;
            if (ValidHex(newHex) && gpCombatManager->m_hexCells[newHex].m_occupantSide != -1
                && gpCombatManager->m_hexCells[newHex].m_occupantIndex != -1) {
                m_unknown09 = frameBase + 6;
                gpCombatManager->m_unknown727 = 1;
                gpCombatManager->DrawFrame(1);
                target2 = &gpCombatManager->m_armies[gpCombatManager->m_hexCells[newHex].m_occupantSide]
                                                    [gpCombatManager->m_hexCells[newHex].m_occupantIndex];
                DamageEnemy(target2, &dmg, &kills, 0, 0);
                if (target2->m_quantity > 0)
                    target2->Stand(1);
            }
        }
        kills = savedKilled;
    }
    if (gbGenieHalf)
        sprintf(gText, "%s %s half the enemy troops!",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "destroy" : "destroys");
    else if (kills > 0)
        sprintf(gText, "%s %s %d %s.  %d %s %s.",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "do" : "does", dmg, "Damage", kills,
                kills > 1 ? gArmyNamesPlural[target->m_creatureType] : gArmyNames[target->m_creatureType],
                kills > 1 ? "perish" : "perishes");
    else
        sprintf(gText, "%s %s %d %s.",
                m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType],
                m_quantity > 1 ? "do" : "does", dmg, "Damage");
    gText[0] -= 32;
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.unknown07);
    gpCombatManager->m_unknown260 = oldMode;
    switch (m_creatureType) {
        case 11:
            if (SRandom(1, 5) == 3) {
                if (target && target->m_spellEffect != 12 && target->m_creatureType != 23
                    && (target->m_creatureType != 13 || SRandom(0, 4) != 1)
                    && !(target->m_stats.attributes & 0x10)) {
                    gpCombatManager->CastSpell(18, target->m_hex, 1, -1);
                    didCast = 1;
                }
            } else if (SRandom(1, 5) == 3 && target2 && target2->m_spellEffect != 12
                       && target2->m_creatureType != 23
                       && (target2->m_creatureType != 13 || SRandom(0, 4) != 1)
                       && !(target2->m_stats.attributes & 0x10)) {
                gpCombatManager->CastSpell(18, target2->m_hex, 1, -1);
                didCast = 1;
            }
            break;
        case 16:
            if (SRandom(1, 5) == 3 && target && target->m_spellEffect != 12 && target->m_creatureType != 23
                && (target->m_creatureType != 13 || SRandom(0, 127) % 4 != 1)
                && !(target->m_stats.attributes & 0x10)) {
                gpCombatManager->CastSpell(7, target->m_hex, 1, -1);
                didCast = 1;
            }
            break;
        case 26:
            gpCombatManager->m_unknown6e9[gpCombatManager->m_hexCells[m_hex].m_occupantSide] = kills;
            break;
        default:
            break;
    }
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_unknown260 = oldMode;
    gpCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    if (target) {
        gpCombatManager->m_limitCreatureCount[target->m_side][target->m_index] = 1;
        if (!(target->m_stats.attributes & 0x10))
            target->Stand(0);
    }
    if (target2) {
        gpCombatManager->m_limitCreatureCount[target2->m_side][target2->m_index] = 1;
        if (!(target2->m_stats.attributes & 0x10))
            target2->Stand(0);
    }
    WaitSample(1);
    if (m_stats.attributes & 8) {
        if (target2) {
            m_unknown09 = frameBase + 6;
            gpCombatManager->m_unknown727 = 1;
            gpCombatManager->DrawFrame(1);
        }
        m_unknown09 = frameBase + 3;
        gpCombatManager->m_unknown727 = 1;
        gpCombatManager->DrawFrame(1);
        m_unknown09 = frameBase;
        gpCombatManager->m_unknown727 = 1;
        gpCombatManager->DrawFrame(1);
    }
    m_unknown09 = 4;
    gpCombatManager->m_unknown727 = 1;
    gpCombatManager->DrawFrame(1);
    m_unknown09 = 3;
    gpCombatManager->m_unknown727 = 1;
    gpCombatManager->DrawFrame(1);
    gpCombatManager->m_unknown727 = 1;
    Stand(1);
    m_facing = facing;
    gpCombatManager->m_unknown727 = 1;
    if (m_unknown52 == 1 && !retaliation)
        CancelSpell();
    gpCombatManager->m_unknown727 = 1;
    Stand(1);
    if (m_creatureType == 26)
        m_quantity += gpCombatManager->m_unknown6e9[gpCombatManager->m_hexCells[m_hex].m_occupantSide];
    if (target && target->m_quantity > 0) {
        gpCombatManager->m_unknown727 = 1;
        target->Stand(1);
        if (target->m_spellEffect == 18
            || target->m_creatureType != 20 && (target->m_stats.attributes & 0x40) || m_creatureType == 24
            || m_creatureType == 12 || didCast || retaliation) {
            goto secondStrike;
        } else {
            target->m_attackDirection = OppositeDirection(m_attackDirection);
            if (target->m_stats.attributes & 1) {
                short checkHex;

                checkHex = GetAdjacentCellIndex(target->m_hex, target->m_facing ? 5 : 0);
                if (m_hex == checkHex)
                    target->m_attackDirection = 6;
                checkHex = GetAdjacentCellIndex(target->m_hex, (signed char)(target->m_facing ? 3 : 2));
                if (m_hex == checkHex)
                    target->m_attackDirection = 7;
            }
            target->DoAttack(1);
            target->m_stats.attributes |= 0x40;
            if (target->m_creatureType == 26)
                target->m_quantity +=
                    gpCombatManager->m_unknown6e9[gpCombatManager->m_hexCells[target->m_hex].m_occupantSide];
        }
    }
secondStrike:
    if ((m_creatureType == 8 || m_creatureType == 5) && target && target->m_quantity > 0 && !retaliation
        && m_spellEffect != 18 && m_quantity > 0) {
        curDir = m_attackDirection;
        m_attackDirection = attackDir;
        DoAttack(1);
        m_attackDirection = curDir;
    }
    m_targetSide = newHex = -1;
    if (retaliation)
        gpCombatManager->m_currentSide = 1 - gpCombatManager->m_currentSide;
}

VA(0x0046a080, 0x49)
void army::ResetPath(void) {
    short i;

    for (i = 0; i < COMBAT_HEX_COUNT; i++)
        gpCombatManager->m_hexCells[i].m_pathFlag = 0;
}

VA(0x0046a0c9, 0x27)
short army::WalkTo(void) {
    return WalkTo(m_moveTargetHex);
}

// Walks the found path one hex at a time, at most the stack's speed.
VA(0x0046a0f0, 0xfc)
short army::WalkTo(short destHex) {
    signed char step;
    int moved;

    m_targetSide = m_targetIndex = -1;
    if (!FindPath(m_hex, destHex, m_stats.speed, 1, 0))
        return 3;
    moved = 0;
    for (step = gpSearchArray->m_pathLength - 1; step >= 0; step--) {
        Walk(gpSearchArray->m_directions[step], 0, gpSearchArray->m_pathLength - 1 != step);
        moved++;
        if (moved >= m_stats.speed)
            step = -1;
    }
    if (!m_unknown52)
        CancelSpell();
    Stand(1);
    return 0;
}

VA(0x0046a1ec, 0x27)
short army::AttackTo(void) {
    return AttackTo(m_moveTargetHex);
}

// Flyers jump next to the target; walkers stop short when out of moves.
VA(0x0046a213, 0x1c9)
short army::AttackTo(short destHex) {
    signed char step;
    int moved;

    if (m_stats.attributes & 2) {
        if (m_hex != destHex)
            FlyTo(destHex);
        DoAttack(0);
        return 0;
    }
    if ((m_stats.attributes & 8) && m_hex == m_moveTargetHex) {
        DoAttack(0);
        return 0;
    }
    if (FindPath(m_hex, destHex, m_stats.speed, 1, 0)) {
        if (gpSearchArray->m_pathLength == 1) {
            m_attackDirection = gpSearchArray->m_directions[0];
            DoAttack(0);
        } else {
            step = 0;
            moved = 0;
            for (step = gpSearchArray->m_pathLength - 1; step; step--) {
                Walk(gpSearchArray->m_directions[step], 0, gpSearchArray->m_pathLength - 1 != step);
                moved++;
                if (moved >= m_stats.speed && step != 1) {
                    Stand(1);
                    return 3;
                }
            }
            if (!m_unknown52)
                CancelSpell();
            m_attackDirection = gpSearchArray->m_directions[0];
            DoAttack(0);
        }
        return 0;
    }
    return 3;
}

// donor PoL RVA 0x0004f93e; preferred Buka symbol ?CheckLuck@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.723107;margin=0.234495;shape=0.473;size=0.925;calls=0.875;strings=badluck.82m|goodluck.82m;alternate=pol20:void army::CheckLuck(void)@0x0004f93e
VA(0x0046a3dc, 0x249)
void army::CheckLuck(void) {
    int luck;

    if (!gpCombatManager->m_heroes[m_side])
        return;
    m_luck = 0;
    luck = gpGame->GetLuck(gpCombatManager->m_heroes[m_side], this);
    if (luck > 0 && SRandom(1, 12) <= luck)
        m_luck = 1;
    if (luck < 0 && SRandom(1, 12) < -luck)
        m_luck = -1;
    if (m_luck) {
        SAMPLE2 sample = NULL_SAMPLE2;
        if (m_luck < 0)
            sprintf(gText, "badluck.82m");
        else
            sprintf(gText, "goodluck.82m");
        sample = LoadPlaySample(gText);
        if (m_luck < 0) {
            sprintf(gText, "Bad luck descends on the %s",
                    m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType]);
            gpCombatManager->CombatMessage(gText, 1);
            Wince();
            SpellEffect(23, 180);
        } else {
            sprintf(gText, "Good luck shines on the %s",
                    m_quantity > 1 ? gArmyNamesPlural[m_creatureType] : gArmyNames[m_creatureType]);
            gpCombatManager->CombatMessage(gText, 1);
            Stand(1);
            SpellEffect(22, 180);
        }
        Stand(1);
        WaitEndSample(sample, -1);
    }
}

// donor PoL RVA 0x0004fbc0; preferred Buka symbol ?DamageEnemy@army@@QAEXPAV1@PAH1HH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.387284;margin=0.228483;shape=0.296;size=0.628;calls=0.714;alternate=pol20:void army::DamageEnemy(class army *, int *, int *, int, int)@0x0004fbc0
VA(0x0046a625, 0x2ae)
void army::DamageEnemy(class army* target, int* damageResult, int* killedResult, int rangedAttack,
                       int defenseModifier) {
    float total;
    short delta;
    int damage;
    short defenseBonus;
    short index;
    short attBonus;
    int halfDamage;

    if (!target)
        return;
    total = 0;
    gbGenieHalf = 0;
    for (index = 0; index < m_quantity; index++) {
        switch (m_damageMode) {
            case 3:
                total += m_stats.damageMax;
                break;
            case 1:
                total += m_stats.damageMin;
                break;
            default:
                total += SRandom(m_stats.damageMin, m_stats.damageMax);
                break;
        }
    }
    attBonus = 0;
    defenseBonus = 0;
    delta = m_stats.attack + attBonus - (target->m_stats.defense + defenseBonus + defenseModifier);
    if (delta > 20)
        delta = 20;
    if (delta < -20)
        delta = -20;
    total *= gfBattleStat[delta + 20];
    if (m_luck > 0)
        total *= 2;
    if (m_luck < 0)
        total /= 2;
    m_luck = 0;
    if ((m_stats.attributes & 4) && !rangedAttack)
        total /= 2;
    if (m_damageMode == 2)
        total /= 2;
    damage = (int)(total + 0.5);
    if (m_creatureType == 27 && SRandom(1, 5) == 2) {
        halfDamage = target->m_stats.hitPoints * ((target->m_quantity + 1) / 2);
        if (damage < halfDamage) {
            gbGenieHalf = 1;
            damage = halfDamage;
        }
    }
    if (damage > 32000)
        damage = 32000;
    if (damage <= 0)
        damage = 1;
    *damageResult = damage;
    *killedResult = target->Damage(damage);
}

// donor PoL RVA 0x0005012e; preferred Buka symbol ?Damage@army@@QAEHJH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.419408;margin=1.157007;shape=0.216;size=0.693;calls=1.000;alternate=pol20:int army::Damage(long int, int)@0x0005012e
// A stack whose spell (2) breaks on damage loses it.
VA(0x0046a8d3, 0x176)
int army::Damage(long int damage) {
    signed char facing;
    int minKilled;
    int kills;

    damage += m_hitPointsLost;
    kills = damage / m_stats.hitPoints;
    m_hitPointsLost = damage % m_stats.hitPoints;
    minKilled = m_quantity / 5;
    if (minKilled == 0)
        minKilled = 1;
    if (kills > 0)
        m_powFrames = 4;
    else
        m_powFrames = -1;
    if (m_quantity < kills)
        kills = m_quantity;
    m_quantity = m_quantity - kills;
    if (m_quantity <= 0)
        m_powFrames = 5;
    facing = m_facing;
    m_facing = gpCombatManager->m_armies[gpCombatManager->m_currentSide][gpCombatManager->m_currentArmyIndex].m_facing ^ 1;
    Wince();
    m_facing = facing;
    gpCombatManager->DrawFrame(1);
    if (m_unknown52 == 2) {
        m_stats.attributes |= 0x80;
        if (m_spellEffect != 7)
            m_stats.attributes |= 0x40;
        CancelSpell();
    }
    return kills;
}

// Plays the impact effect on every stack hit this attack (m_powFrames),
// fading the killed ones out, then restores the grid.
VA(0x0046aa49, 0x8a9)
void army::PowEffect(signed char effect) {
    short frames;
    short stackIndex;
    short armyNum;
    short step;
    short longest;
    short side;
    int cellHex;
    army* curArmy;

    longest = 0;
    for (armyNum = 0; armyNum < gpCombatManager->m_numArmies[1]; armyNum++)
        if (gpCombatManager->m_armies[1][armyNum].m_powFrames > longest)
            longest = gpCombatManager->m_armies[1][armyNum].m_powFrames;
    for (armyNum = 0; armyNum < gpCombatManager->m_numArmies[0]; armyNum++)
        if (gpCombatManager->m_armies[0][armyNum].m_powFrames > longest)
            longest = gpCombatManager->m_armies[0][armyNum].m_powFrames;
    if (longest >= 5)
        frames = 10;
    else
        frames = longest;
    if (effect != gCurLoadedSpellFileId) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(gPowEffectNames[effect]);
        gCurLoadedSpellFileId = effect;
    }
    for (side = 0; side < 2; side++)
        for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++)
            if (gpCombatManager->m_armies[side][stackIndex].m_powFrames > 0)
                gpSoundManager->MemorySample(gpCombatManager->m_armies[side][stackIndex].m_samples[2]);
    step = 0;
    gpCombatManager->ResetLimitCreature();
    while (step < frames && step < 5) {
        gpCombatManager->m_unknown727 = 1;
        glTimers[1] = KBTickCount() + 30;
        for (side = 0; side < 2; side++) {
            for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++) {
                if (gpCombatManager->m_armies[side][stackIndex].m_powFrames >= step) {
                    gpCombatManager->m_armies[side][stackIndex].m_unknown08 = 3;
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex])
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                } else if (gpCombatManager->m_armies[side][stackIndex].m_unknown08 != 2) {
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex]
                        && gpCombatManager->m_armies[side][stackIndex].m_unknown08)
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                    gpCombatManager->m_armies[side][stackIndex].m_unknown08 = 0;
                }
            }
        }
        giSpellEffectFrame = step;
        gpCombatManager->DrawFrame(1);
        step++;
    }
    while (step < frames && step < 10) {
        gpCombatManager->m_unknown727 = 1;
        glTimers[1] = KBTickCount() + 30;
        for (side = 0; side < 2; side++) {
            for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++) {
                if (gpCombatManager->m_armies[side][stackIndex].m_powFrames >= 5) {
                    gpCombatManager->m_armies[side][stackIndex].m_unknown08 = 3;
                    gpCombatManager->m_armies[side][stackIndex].m_stats.attributes |= 0x10;
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex])
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                } else if (gpCombatManager->m_armies[side][stackIndex].m_unknown08 != 2) {
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex]
                        && gpCombatManager->m_armies[side][stackIndex].m_unknown08)
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                    gpCombatManager->m_armies[side][stackIndex].m_unknown08 = 0;
                }
            }
        }
        giSpellEffectFrame = step;
        gpCombatManager->DrawFrame(1);
        step++;
    }
    while (++step < 10)
        DelayMilli(15);
    for (side = 0; side < 2; side++) {
        for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++) {
            curArmy = &gpCombatManager->m_armies[side][stackIndex];
            if ((curArmy->m_stats.attributes & 0x10) && curArmy->m_powFrames != -1) {
                cellHex = curArmy->m_hex;
                if (ValidHex(cellHex))
                    gpCombatManager->m_hexCells[cellHex].m_occupantSide = -1;
                if (curArmy->m_stats.attributes & 1) {
                    cellHex = curArmy->m_hex + (curArmy->m_facing ? -1 : 1);
                    gpCombatManager->m_hexCells[cellHex].m_occupantSide = -1;
                }
            } else if (curArmy->m_unknown08 != 2) {
                curArmy->m_unknown08 = 0;
            }
            curArmy->m_powFrames = -1;
            gpCombatManager->UpdateGrid(curArmy->m_hex, curArmy->m_stats.attributes);
        }
    }
    gpCombatManager->DrawFrame(1);
    for (side = 0; side < 2; side++)
        for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++)
            gpCombatManager->m_armies[side][stackIndex].WaitSample(2);
}

VA(0x0046b2f2, 0x34)
unsigned long int army::Strength(void) {
    return gMonsterDatabase[m_creatureType].fightValue * m_quantity;
}

// Plays a combat effect animation over this stack.
VA(0x0046b326, 0x131)
void army::SpellEffect(short effect, int frameDelay) {
    short frame;
    short effectFileId;
    short frameCount;

    m_unknown2f = effect;
    effectFileId = MAKEFILEID(gCombatFxNames[effect]);
    if (gCurLoadedSpellFileId != effectFileId) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(effectFileId);
        gCurLoadedSpellFileId = effectFileId;
    }
    m_unknown08 = 3;
    frameCount = 10;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    for (frame = 0; frame < frameCount; frame++) {
        gpCombatManager->m_unknown727 = 1;
        glTimers[1] = KBTickCount() + frameDelay;
        giSpellEffectFrame = frame;
        gpCombatManager->DrawFrame(1);
        DelayTil(glTimers + 1);
    }
    m_unknown2f = -1;
}

// Slow (and the other speed spells) restore the base speed and flight;
// effect 9 gave three defense.
VA(0x0046b457, 0xb2)
void army::CancelSpell(void) {
    switch (m_spellEffect) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 10:
            m_damageMode = 0;
            m_stats.speed = m_baseSpeed;
            m_stats.attributes |= gMonsterDatabase[m_creatureType].stats.attributes & 2;
            break;
        case 9:
            m_stats.defense -= 3;
            break;
    }
    m_spellEffect = -1;
    m_unknown52 = -1;
}

// A berserk stack attacks a random neighbour, or flies or steps at random.
VA(0x0046b509, 0x1de)
void army::GoBerserk(void) {
    signed char found;
    short tryCount;
    short dir;
    short attackMask;
    short targetHex;
    short target;

    found = 0;
    dir = 0;
    tryCount = 0;
    while (!found) {
        attackMask = GetAttackMask(m_hex, 2, -1);
        if (attackMask != 0xff) {
            while (!found) {
                dir = Random(0, 7);
                if (!(attackMask & (1 << dir))) {
                    giNextAction = 2;
                    ValidAttack(m_hex, dir, 2, -1, &targetHex);
                    giNextActionGridIndex = targetHex;
                    found = 1;
                }
            }
        } else if (m_stats.attributes & 2) {
            target = Random(1, 43);
            if (gpCombatManager->m_hexCells[target].m_occupantSide != -1) {
                m_targetSide = gpCombatManager->m_hexCells[target].m_occupantSide;
                m_targetIndex = gpCombatManager->m_hexCells[target].m_occupantIndex;
                if (ValidFlight(target, 0)) {
                    giNextAction = 2;
                    giNextActionGridIndex = target;
                    found++;
                }
            } else {
                giNextAction = 2;
                giNextActionGridIndex = target;
            }
        } else {
            dir = Random(0, 5);
            if (ValidMove(dir)) {
                giNextAction = 2;
                giNextActionGridIndex = m_hex;
                giNextActionGridIndex = GetAdjacentCellIndex(giNextActionGridIndex, dir);
            }
            found++;
        }
        tryCount++;
    }
}

// Attacks the stack on the hex (flying, shooting or picking the adjacent
// direction) or moves there; a second argument forbids attacking.
VA(0x0046b6e7, 0x3a3)
void army::MoveAttack(int hex, int moveOnly) {
    hexcell* pCell;
    int baseHex;
    short meleeMask;
    short atkMask;
    int adjHex;
    int dirIndex;

    gpCombatManager->m_limitCreature = 0;
    m_targetSide = -1;
    m_targetIndex = -1;
    if (!ValidHex(hex))
        return;
    if (gpCombatManager->m_hexCells[hex].m_occupantSide != -1
        && (gpCombatManager->m_hexCells[hex].m_occupantSide != gpCombatManager->m_currentSide
            || gpCombatManager->m_hexCells[hex].m_occupantIndex != gpCombatManager->m_currentArmyIndex)) {
        if (moveOnly)
            return;
        m_targetSide = gpCombatManager->m_hexCells[hex].m_occupantSide;
        m_targetIndex = gpCombatManager->m_hexCells[hex].m_occupantIndex;
        m_moveTargetHex = hex;
        meleeMask = GetAttackMask(m_hex, 0, -1);
        if ((m_stats.attributes & 2) && meleeMask == 0xff && m_moveTargetHex != m_hex
            && !ValidFlight(m_moveTargetHex, 0))
            return;
        if (m_spellEffect == 14)
            atkMask = GetAttackMask(m_hex, 2, -1);
        else
            atkMask = GetAttackMask(m_hex, 1, -1);
        if (atkMask == 0xff && m_stats.shots > 0) {
            SpecialAttack();
        } else if (meleeMask == 0xff) {
            AttackTo();
        } else {
            for (dirIndex = 0; dirIndex < 8; dirIndex++) {
                if (dirIndex < 6 || (m_stats.attributes & 1)) {
                    baseHex = m_hex;
                    if ((m_stats.attributes & 1) && m_facing == 0 && dirIndex >= 0 && dirIndex <= 2)
                        baseHex++;
                    if ((m_stats.attributes & 1) && m_facing == 1 && dirIndex >= 3 && dirIndex <= 5)
                        baseHex--;
                    if (dirIndex >= 6) {
                        if (m_facing == 0)
                            baseHex++;
                        else
                            baseHex--;
                    }
                    adjHex = GetAdjacentCellIndex(baseHex, dirIndex);
                    if (ValidHex(adjHex)) {
                        pCell = &gpCombatManager->m_hexCells[adjHex];
                        if (pCell->m_occupantSide == m_targetSide && pCell->m_occupantIndex == m_targetIndex)
                            m_attackDirection = dirIndex;
                    }
                }
            }
            DoAttack(0);
        }
    } else if (m_stats.attributes & 2) {
        m_moveTargetHex = hex;
        if (!ValidFlight(m_moveTargetHex, 0))
            return;
        FlyTo(m_moveTargetHex);
    } else {
        WalkTo(hex);
    }
    gpCombatManager->m_limitCreature = 1;
}

