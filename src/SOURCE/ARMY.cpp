// Combat stacks and the computer's combat moves. HoMM1 keeps Buka's
// SOURCE/ARMY methods after the SOURCE/AI combat helpers in one object
// (retail 0x00464520-0x0046ba8f; hero::hero starts the next one).

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// donor PoL RVA 0x000c0790; preferred Buka symbol ?AICheckRetreat@combatManager@@QAEHXZ
// donor Buka TU SOURCE/AI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.550932;margin=0.199965;shape=0.407;size=0.920;calls=0.857;alternate=pol20:int combatManager::AICheckRetreat(void)@0x000c0790
VA(0x00464520, 0x783)
int combatManager::AICheckRetreat(void) { return 0; }

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
    m_unknown13 = 0;
    m_unknown2b = -1;
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

// donor PoL RVA 0x0004c7e5; preferred Buka symbol ?SpecialAttack@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.357822;margin=0.373223;shape=0.266;size=0.572;calls=0.585;alternate=pol20:void army::SpecialAttack(void)@0x0004c7e5
VA(0x00467b97, 0xcca)
void army::SpecialAttack(void) {}

VA(0x00468fc6, 0x2d)
void army::DirDoAttack(short direction) {
    m_attackDirection = direction;
    DoAttack(0);
}

// donor PoL RVA 0x0004e1a1; preferred Buka symbol ?DoAttack@army@@QAEXH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.427643;margin=0.977018;shape=0.217;size=0.817;calls=0.724;alternate=pol20:void army::DoAttack(int)@0x0004e1a1
VA(0x00468ff3, 0x108d)
void army::DoAttack(int) {}

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

VA(0x0046a1ec, 0x27)
short army::AttackTo(void) {
    return AttackTo(m_moveTargetHex);
}

// donor PoL RVA 0x0004f93e; preferred Buka symbol ?CheckLuck@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.723107;margin=0.234495;shape=0.473;size=0.925;calls=0.875;strings=badluck.82m|goodluck.82m;alternate=pol20:void army::CheckLuck(void)@0x0004f93e
VA(0x0046a3dc, 0x249)
void army::CheckLuck(void) {}

// donor PoL RVA 0x0004fbc0; preferred Buka symbol ?DamageEnemy@army@@QAEXPAV1@PAH1HH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.387284;margin=0.228483;shape=0.296;size=0.628;calls=0.714;alternate=pol20:void army::DamageEnemy(class army *, int *, int *, int, int)@0x0004fbc0
VA(0x0046a625, 0x2ae)
void army::DamageEnemy(class army *, int *, int *, int, int) {}

// donor PoL RVA 0x0005012e; preferred Buka symbol ?Damage@army@@QAEHJH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.419408;margin=1.157007;shape=0.216;size=0.693;calls=1.000;alternate=pol20:int army::Damage(long int, int)@0x0005012e
VA(0x0046a8d3, 0x176)
int army::Damage(long int, int) { return 0; }

VA(0x0046b2f2, 0x34)
unsigned long int army::Strength(void) {
    return gMonsterDatabase[m_creatureType].fightValue * m_quantity;
}

// donor PoL RVA 0x00052ad9; preferred Buka symbol ?MoveAttack@army@@QAEXHH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.522810;margin=0.525862;shape=0.325;size=0.919;calls=0.929;alternate=pol20:void army::MoveAttack(int, int)@0x00052ad9
VA(0x0046b6e7, 0x3a9)
void army::MoveAttack(int, int) {}

