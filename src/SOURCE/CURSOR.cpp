// HoMM1's CURSOR object (0x00405950-0x00407d8f): Buka SOURCE/CURSOR
// advManager movement routines, aligned apart from wingraph and TOWNMGR.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/X_GLOBAL.h>

// Buka CURSOR.cpp:50 StartCursor; HoMM1 keys the cycle off the global
// walk speed and indexes the map directly.
VA(0x00405950, 0x169)
void advManager::StartCursor(signed char direction)
{
    short directionX;
    short newX;
    short directionY;
    short newY;

    m_cursorDirection = direction;
    m_cursorFrame = GetCursorBaseFrame(direction) + 1;
    if (gConfig.walkSpeed > 0)
        m_cursorCycle = 1;
    else
        m_cursorCycle = 2;
    directionX = normalDirTable[direction].x;
    directionY = normalDirTable[direction].y;
    m_previousCursorMapX = m_cursorMapX;
    m_previousCursorMapY = m_cursorMapY;
    m_cursorMapX += directionX;
    m_cursorMapY += directionY;
    newX = m_mapOriginX + m_cursorMapX;
    newY = m_mapOriginY + m_cursorMapY;
    m_mapData[newX][newY].m_flags |= 0x40;
}

// Buka CURSOR.cpp:78 StopCursor; HoMM1 also forgets the footstep samples.
VA(0x00405ab9, 0x150)
void advManager::StopCursor(signed char stopSound)
{
    if (stopSound) {
        bMoveSoundMade = 1;
        m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
        m_cursorFrameCount = 0;
        EveryOther = 0;
        hPrevMoveSound = 0;
        hLastMoveSound = 0;
    }
    m_cursorCycle = 0;
    if (m_previousCursorMapX != -1) {
        m_mapData[m_mapOriginX + m_previousCursorMapX][m_mapOriginY + m_previousCursorMapY].m_flags &=
            ~0x40;
        m_previousCursorMapX = m_previousCursorMapY = -1;
    }
    m_cursorTurning = 0;
}

// Buka CURSOR.cpp:99 DrawCursor; HoMM1 draws the hero shadow first and
// counts flag frames with m_updateMaxY.
VA(0x00405c09, 0x5e4)
void advManager::DrawCursor(void)
{
    short drawX;
    short screenY;
    short drawFrame;

    if (bShowIt == 0 || bSpecialHideCursor)
        return;
    if (gbDrawSavedCursor) {
        m_cursorDirection = S1cursorDirection;
        m_cursorFrame = S1cursorBaseFrame;
        m_cursorFrameCount = S1cursorFrameCount;
        m_cursorCycle = S1cursorCycle;
        m_cursorTurning = S1cursorTurning;
    }
    drawX = m_updateMinX + 0xe0;
    screenY = m_updateMinY + 0xff;
    if (m_cursorType == 4)
        screenY -= 10;
    if (m_cursorFrame & 0x80) {
        drawX += 0x20;
        drawFrame = (m_cursorFrame & 0x7f) + m_cursorFrameCount;
        if (m_drawHeroShadows && m_cursorType != 4)
            FlipDimIconToBitmap(m_boatShadowIcon, gpWindowManager->m_screen, drawX, screenY, drawFrame, 0);
        FlipIconToBitmap(m_heroIcons[m_cursorType], gpWindowManager->m_screen, drawX, screenY,
                         drawFrame, 0);
        if (m_cursorType == 4) {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame & 0x7f;
            FlipIconToBitmap(m_boatFlagIcons[gpCurPlayer->m_color], gpWindowManager->m_screen,
                             drawX, screenY, drawFrame, 0);
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_updateMaxY & 3) + (m_cursorFrame & 0x7f) + 0x38;
            FlipIconToBitmap(m_flagIcons[gpCurPlayer->m_color], gpWindowManager->m_screen, drawX,
                             screenY, drawFrame, 0);
            m_updateMaxY++;
        }
    } else {
        drawFrame = m_cursorFrame + m_cursorFrameCount;
        if (m_drawHeroShadows && m_cursorType != 4)
            DimIconToBitmap(m_boatShadowIcon, gpWindowManager->m_screen, drawX, screenY, drawFrame, 0);
        IconToBitmap(m_heroIcons[m_cursorType], gpWindowManager->m_screen, drawX, screenY, drawFrame,
                     0);
        if (m_cursorType == 4) {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame;
            IconToBitmap(m_boatFlagIcons[gpCurPlayer->m_color], gpWindowManager->m_screen, drawX,
                         screenY, drawFrame, 0);
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_updateMaxY & 3) + m_cursorFrame + 0x38;
            IconToBitmap(m_flagIcons[gpCurPlayer->m_color], gpWindowManager->m_screen, drawX,
                         screenY, drawFrame, 0);
            m_updateMaxY++;
        }
    }
    if (m_cursorCycle && gConfig.walkSpeed != 4) {
        m_cursorFrameCount++;
        if (gConfig.walkSpeed == 3 && (m_cursorFrameCount == 4 || m_cursorFrameCount == 1))
            m_cursorFrameCount++;
        if (gConfig.walkSpeed == 0) {
            EveryOther = 1 - EveryOther;
            if (EveryOther)
                m_cursorFrameCount--;
        }
    }
    if (m_cursorFrameCount >= 8)
        m_cursorFrameCount = 0;
    if (!m_cursorTurning) {
        if (m_cursorFrameCount == 0)
            hPrevMoveSound = hLastMoveSound;
        if (m_cursorFrameCount == 3 || (gConfig.walkSpeed == 4 && !bMoveSoundMade)) {
            bMoveSoundMade = 1;
            if (!EveryOther)
                hLastMoveSound = gpSoundManager->MemorySample(
                    m_cursorSamples[giGroundToTerrain[GetCell(m_mapOriginX + 7, m_mapOriginY + 7)->m_tileIndex]]);
        }
    }
    if (!gbDrawSavedCursor) {
        S1cursorDirection = m_cursorDirection;
        S1cursorBaseFrame = m_cursorFrame;
        S1cursorFrameCount = m_cursorFrameCount;
        S1cursorCycle = m_cursorCycle;
        S1cursorTurning = m_cursorTurning;
    }
}

// donor PoL RVA 0x0000e198; preferred Buka symbol ?GetCursorBaseFrame@advManager@@QAEHH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.375377;margin=0.466673;shape=0.186;size=0.574;calls=1.000;alternate=pol20:int advManager::GetCursorBaseFrame(int)@0x0000e198
VA(0x004061ed, 0x88)
short advManager::GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, short) direction)
{
    if (static_cast<int>(direction) > static_cast<int>(MAP_DIRECTION_SOUTH)) {
        switch (direction) {
            case MAP_DIRECTION_SOUTH_WEST:
                return static_cast<short>(CURSOR_BOAT_BASE_FRAME_5);
            case MAP_DIRECTION_WEST:
                return static_cast<short>(CURSOR_BOAT_BASE_FRAME_6);
            case MAP_DIRECTION_NORTH_WEST:
                return static_cast<short>(CURSOR_BOAT_BASE_FRAME_7);
            default:
                return 0;
        }
    } else {
        return static_cast<int>(direction) * static_cast<int>(CURSOR_FRAMES_PER_DIRECTION);
    }
}

// Buka CURSOR.cpp:379 TurnTo; HoMM1 keeps sixteen half-step frames and
// word-sized step delays.
VA(0x00406275, 0x261)
void advManager::TurnTo(signed char direction)
{
    short frameStep = 1;
    short curFrame;
    short directionDifference = direction - m_cursorDirection;
    int delayTime;

    if (directionDifference == 0)
        return;
    if ((directionDifference < 0 && directionDifference >= -4)
        || (directionDifference > 0 && directionDifference > 4))
        frameStep = -1;
    m_cursorTurning = 1;
    curFrame = m_cursorDirection * 2;
    delayTime = giStepDelay[gConfig.walkSpeed];
    if (gConfig.walkSpeed == 0)
        delayTime *= 3;
    if (gConfig.walkSpeed == 1)
        delayTime = delayTime * 1.5;
    do {
        m_cursorCycle = 1;
        if (m_cursorType >= 4)
            m_cursorFrame = boatFrameFlip[curFrame];
        else
            m_cursorFrame = horseFrameFlip[curFrame];
        m_cursorFrameCount = 0;
        glTimers[1] = KBTickCount() + delayTime;
        if (gConfig.walkSpeed != 4) {
            if (ComboDraw(m_mapOriginX, m_mapOriginY, 0))
                UpdateScreen(0, 0);
            if (bShowIt)
                DelayTil(&glTimers[1]);
        }
        curFrame += frameStep;
        if (curFrame < 0)
            curFrame = 15;
        curFrame %= 16;
    } while (curFrame != direction * 2);
    m_cursorDirection = direction;
    StopCursor(1);
    if (bShowIt)
        DelayTil(&glTimers[1]);
    if (ComboDraw(m_mapOriginX, m_mapOriginY, 0))
        UpdateScreen(0, 0);
}

// Alias: retail reaches KB's gbRemoteOn (0x00494164); rename at the use.
extern int gbHideComputerMoves;

// Buka CURSOR.cpp:429 GetMoveShowIt; HoMM1 reads the current hero itself
// and tests the watch bit directly in the map-extra grid.
VA(0x004064d6, 0x136)
int advManager::GetMoveShowIt(signed char direction)
{
    hero *movingHero;
    short dirX;
    short dy;

    if (gpCurPlayer->CurrentHero() == -1)
        return 0;
    movingHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    dirX = normalDirTable[direction].x;
    dy = normalDirTable[direction].y;
    if ((gbThisNetHumanPlayer[giCurPlayer] || (!gConfig.blackoutComputer && !gbHideComputerMoves))
        && ((gpGame->m_mapExtra[movingHero->m_x][movingHero->m_y]
             & giCurWatchPlayerBit)
            || (gpGame->m_mapExtra[movingHero->m_x + dirX][movingHero->m_y + dy]
                & giCurWatchPlayerBit)))
        return 1;
    else
        return 0;
}

// Buka CURSOR.cpp MoveHero; HoMM1 recomputes the step cost from the hero
// type, parks the boat on a coast step and has no deferred object draw.

VA(0x0040660c, 0xe1e)
mapCell *advManager::MoveHero(signed char direction, signed char stopAfterMove, int *eventX, int *eventY,
                              int *outOfMobility, signed char processEvent, signed char *adjacentMonster)
{
    mapCell *pCursorCell;
    int step;
    int origX;
    mapCell *nextCell;
    hero *movingHero;
    int origY;
    signed char terrain;
    mapCell *retCell;
    int msDelay;
    short xInc;
    short yInc;
    short pixelsPerStep;
    short numSteps;

    if (gbThisNetHumanPlayer[giCurPlayer])
        SetNoDialogMenus(0);
    *adjacentMonster = 0;
    *outOfMobility = 0;
    gbHeroMoving = 1;
    retCell = 0;
    movingHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    origX = movingHero->m_x;
    origY = movingHero->m_y;
    xInc = normalDirTable[direction].x;
    yInc = normalDirTable[direction].y;
    bShowIt = GetMoveShowIt(direction);
    terrain = giGroundToTerrain[GetCell(movingHero->m_x, movingHero->m_y)->m_tileIndex];
    nextCell = GetCell(movingHero->m_x + xInc, movingHero->m_y + yInc);
    if (CalcTerrainCost(terrain, direction & 1, movingHero->m_remainingMobility, movingHero->m_heroClass)
        > movingHero->m_remainingMobility) {
        *outOfMobility = 1;
        StopCursor(1);
        goto movementDone;
    }
    MobilizeCurrHero(0);
    *eventX = movingHero->m_x + xInc;
    *eventY = movingHero->m_y + yInc;
    if (m_cursorDirection != direction)
        TurnTo(direction);
    movingHero->m_direction = direction;
    if ((movingHero->m_eventFlags & HERO_EVENT_EMBARKED) && nextCell->m_triggerType == 0x1f) {
        boatRecord *boat;
        mapCell *boatCell;

        for (step = 0; step < 32; step++) {
            if (gpGame->m_boats[step].heroId == movingHero->m_id)
                break;
        }
        boat = &gpGame->m_boats[step];
        boatCell = GetCell(movingHero->m_x, movingHero->m_y);
        boat->savedTriggerType = boatCell->m_triggerType;
        boat->savedEventData = boatCell->m_objectMetadata;
        boat->direction = m_cursorDirection;
        boat->heroId |= 0x80;
        boatCell->m_triggerType = 0xbe;
        boatCell->m_objectMetadata = step;
        boat->x = movingHero->m_x;
        boat->y = movingHero->m_y;
        StopCursor(1);
        CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
        UpdateScreen(0, 0);
        m_cursorActive = 0;
    }
    if (nextCell->m_triggerType & 0x80) {
        switch (nextCell->m_triggerType & 0x7f) {
            case 62:
                if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    goto movementDone;
                StopCursor(1);
                m_cursorActive = 0;
                gpWindowManager->SaveFizzleSource(0xc0, 0xc0, 0x60, 0x60);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager->FizzleForward(0xc0, 0xc0, 0x60, 0x60, -1);
                break;
            case 3:
                if (!(movingHero->m_eventFlags & HERO_EVENT_EMBARKED))
                    goto movementDone;
                else
                    goto stoppingEvent;
            case 61:
                if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                    if (gpGame->GetHero(nextCell->m_objectMetadata)->m_eventFlags & HERO_EVENT_EMBARKED)
                        goto stoppingEvent;
                    else
                        goto movementDone;
                }
            case 2:
            case 4:
            case 6:
            case 8:
            case 9:
            case 11:
            case 26:
            case 27:
            case 28:
            case 29:
            case 36:
            case 43:
            case 48:
                if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    goto movementDone;
            stoppingEvent:
                StopCursor(1);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                UpdateScreen(0, 0);
                movingHero->m_remainingMobility -= CalcTerrainCost(
                    terrain, direction & 1, movingHero->m_remainingMobility, movingHero->m_heroClass);
                if (CalcTerrainCost(giGroundToTerrain[nextCell->m_tileIndex], 0, movingHero->m_remainingMobility,
                                    movingHero->m_heroClass)
                    > movingHero->m_remainingMobility) {
                    movingHero->m_remainingMobility = 0;
                    stopAfterMove = 1;
                }
                retCell = nextCell;
                goto movementDone;
            case 40:
                if (gpGame->GetTown(nextCell->m_objectMetadata)->m_owner != giCurPlayer
                    && gpGame->GetTown(nextCell->m_objectMetadata)->HasGarrison()) {
                    StopCursor(1);
                    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                    UpdateScreen(0, 0);
                    movingHero->m_remainingMobility -= CalcTerrainCost(
                        terrain, direction & 1, movingHero->m_remainingMobility, movingHero->m_heroClass);
                    if (CalcTerrainCost(giGroundToTerrain[nextCell->m_tileIndex], 0,
                                        movingHero->m_remainingMobility, movingHero->m_heroClass)
                        > movingHero->m_remainingMobility) {
                        movingHero->m_remainingMobility = 0;
                        stopAfterMove = 1;
                    }
                    retCell = nextCell;
                    goto movementDone;
                }
                break;
            default:
                break;
        }
    }
    if (!ValidMove(direction))
        goto movementDone;
    if (movingHero->m_locationType == 0xa8) {
        town *occupiedTown;

        occupiedTown = gpGame->GetTown(movingHero->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = -1;
    }
    if (m_routeShown)
        *(m_visibilityMap + (movingHero->m_x + xInc) + (movingHero->m_y + yInc) * MAP_CELL_GRID_SIZE) = 0;
    m_updateMinX = m_updateMinY = 0;
    gpGame->SetVisibility(m_mapOriginX + xInc + 7, m_mapOriginY + yInc + 7, giCurPlayer,
                          gHeroScoutRadius[movingHero->m_heroClass]);
    m_forceCompleteDraw = 1;
    pixelsPerStep = giPixelsPerStep[gConfig.walkSpeed];
    msDelay = giStepDelay[gConfig.walkSpeed];
    StartCursor(direction);
    if (gConfig.walkSpeed == 4) {
        if (EveryOther)
            m_cursorFrame--;
        bMoveSoundMade = 0;
        MoveOrigin(xInc, yInc);
        movingHero->m_x += xInc;
        movingHero->m_y += yInc;
        if (ComboDraw(0))
            UpdateScreen(0, 0);
        EveryOther = 1 - EveryOther;
    } else {
        gbEnlargeScreenBlit = 0;
        gbNoBorder = 1;
        numSteps = 16 / pixelsPerStep;
        for (step = 0; step < numSteps * 2; step++) {
            long tick;

            if (step == numSteps) {
                MoveOrigin(xInc, yInc);
                movingHero->m_x += xInc;
                movingHero->m_y += yInc;
                m_updateMinX = startVals[xInc + 1];
                m_updateMinY = startVals[yInc + 1];
            }
            tick = KBTickCount();
            if (step + 1 == numSteps * 2) {
                m_updateMinX = 0;
                m_updateMinY = 0;
            } else {
                m_updateMinX += xInc * pixelsPerStep;
                m_updateMinY += yInc * pixelsPerStep;
            }
            if (ComboDraw(0)) {
                giLimitUpdMinX = -1;
                UpdateScreen(0, 0);
            }
            if (bShowIt)
                DelayTilMilli(msDelay + tick);
        }
        gbNoBorder = 0;
        DrawAdventureBorder();
        gbEnlargeScreenBlit = 1;
    }
    movingHero->m_remainingMobility -= CalcTerrainCost(
        terrain, direction & 1, movingHero->m_remainingMobility, movingHero->m_heroClass);
    if (CalcTerrainCost(giGroundToTerrain[nextCell->m_tileIndex], 0, movingHero->m_remainingMobility,
                        movingHero->m_heroClass)
        > movingHero->m_remainingMobility) {
        movingHero->m_remainingMobility = 0;
        stopAfterMove = 1;
    }
    StopCursor(stopAfterMove);
    if (processEvent && stopAfterMove && ComboDraw(0))
        UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 0);
    step = GetCell(m_mapOriginX + 7, m_mapOriginY + 7)->m_tileIndex;
    if (giGroundToTerrain[step] != m_currentTerrain && step % 20 < 4) {
        m_currentTerrain = giGroundToTerrain[step];
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    m_updateMinX = m_updateMinY = 0;
    pCursorCell = GetCell(m_cursorMapX + m_mapOriginX, m_cursorMapY + m_mapOriginY);
    *eventX = m_cursorMapX + m_mapOriginX;
    *eventY = m_cursorMapY + m_mapOriginY;
    if ((pCursorCell->m_triggerType & 0x80)
        || ((movingHero->m_eventFlags & HERO_EVENT_EMBARKED) && pCursorCell->m_triggerType == 0x1f)) {
        retCell = pCursorCell;
        switch (pCursorCell->m_triggerType & 0x7f) {
            case 30:
            case 37:
            case 46:
            case 49:
            case 50:
            case 51:
            case 52:
            case 53:
            case 54:
            case 55:
            case 56:
            case 57:
            case 58:
            case 59:
            case 60:
                retCell = 0;
                break;
        }
        goto movementDone;
    } else
        goto movementDone;
movementDone:
    UpdateRadar(1, 1);
    gbHeroMoving = 0;
    if (movingHero->m_x != origX || movingHero->m_y != origY) {
        if (mapExtra[movingHero->m_x][movingHero->m_y] & 0x80) {
            if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
                goto adjacentDone;
            if (retCell && static_cast<char>(retCell->m_triggerType & 0x7f) == 0x3e)
                goto adjacentDone;
            CheckAdjacentMon(adjacentMonster);
            if (movingHero->m_owner == -1)
                retCell = 0;
        }
    }
adjacentDone:
    if (gbThisNetHumanPlayer[giCurPlayer])
        SetNoDialogMenus(1);
    return retCell;
}

// donor PoL RVA 0x0000f753; preferred Buka symbol ?CheckAdjacentMon@advManager@@QAEXPAH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.530646;margin=0.751795;shape=0.360;size=0.888;calls=1.000;alternate=pol20:void advManager::CheckAdjacentMon(int *)@0x0000f753
// Buka CURSOR.cpp:907; HoMM1 keeps byte flags and redraws through the
// three-argument CompleteDraw.
VA(0x0040742a, 0x181)
void advManager::CheckAdjacentMon(signed char *adjacentMonster)
{
    int monX;
    int monY;
    hero *theHero;
    signed char dead;
    mapCell *heroCell;
    mapCell *monsterCell;

    theHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    dead = 0;
    if (FindAdjacentMonster(theHero->m_x, theHero->m_y, &monX, &monY, -1, -1)) {
        StopCursor(1);
        CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
        UpdateScreen(0, 0);
        monsterCell = GetCell(monX, monY);
        heroCell = GetCell(theHero->m_x, theHero->m_y);
        if (gbThisNetHumanPlayer[giCurPlayer])
            PlayerMonsterInteract(monsterCell, heroCell, theHero, &dead, theHero->m_x, theHero->m_y,
                                  1, monX, monY);
        else
            ComputerMonsterInteract(monsterCell, theHero, &dead);
        if (dead) {
            EraseObj(monsterCell, monX, monY);
            if (gbThisNetHumanPlayer[giCurPlayer])
                FizzleCenter(0);
        }
        *adjacentMonster = 1;
    }
}

// Buka CURSOR.cpp:962 ValidMoveWithEvent; HoMM1 lets a boat meet another
// boat, forbids landing a boat on most objects and defers the rest to
// ValidMove.
VA(0x004075ab, 0x20a)
short advManager::ValidMoveWithEvent(hero *movingHero, short direction)
{
    short deltaY;
    short newY;
    short deltaX;
    short newX;
    mapCell *cell;

    deltaX = normalDirTable[direction].x;
    deltaY = normalDirTable[direction].y;
    newX = movingHero->m_x + deltaX;
    newY = movingHero->m_y + deltaY;
    if (newX < 0 || newX > MAP_CELL_GRID_SIZE - 1 || newY < 0 || newY > MAP_CELL_GRID_SIZE - 1)
        return 0;
    cell = &m_mapData[newX][newY];
    switch (cell->m_triggerType & 0x7f) {
        case 3:
            if (!(movingHero->m_eventFlags & 0x80))
                return 1;
            else
                return 0;
        case 61:
            if (movingHero->m_eventFlags & 0x80) {
                if (gpGame->GetHero(cell->m_objectMetadata)->m_eventFlags & 0x80)
                    return 1;
                else
                    return 0;
            }
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
        case 11:
        case 26:
        case 27:
        case 28:
        case 29:
        case 36:
        case 43:
        case 48:
            if (m_cursorType == 4)
                return 0;
            else
                return 1;
    }
    return ValidMove(direction);
}

// Buka CURSOR.cpp:1006 ValidMove; HoMM1 indexes from the cursor's map
// position and tests the north/south object masks directly.
VA(0x004077b5, 0x2a3)
short advManager::ValidMove(short direction)
{
    short downMask;
    short directionX;
    short newX;
    short directionY;
    short newY;
    mapCell *destCell;
    mapCell *hereCell;
    short north;

    directionX = normalDirTable[direction].x;
    directionY = normalDirTable[direction].y;
    newX = m_mapOriginX + directionX;
    newY = m_mapOriginY + directionY;
    if (newX < -7 || newX > MAP_CELL_GRID_SIZE - 7 - 1)
        return 0;
    if (newY < -7 || newY > MAP_CELL_GRID_SIZE - 7 - 1)
        return 0;
    destCell = &m_mapData[m_cursorMapX + newX][m_cursorMapY + newY];
    if (destCell->m_secondaryTrigger & 0x80)
        return 0;
    if (giGroundToTerrain[destCell->m_tileIndex] == 0) {
        if (m_cursorType != 4 && destCell->m_triggerType != 0xbe && destCell->m_triggerType != 0xa3)
            return 0;
    } else {
        if (m_cursorType == 4 && destCell->m_triggerType != 0x1f && destCell->m_triggerType != 0xac)
            return 0;
    }
    hereCell = &m_mapData[m_cursorMapX + m_mapOriginX][m_cursorMapY + m_mapOriginY];
    north = (1 << direction) & 0x83;
    downMask = (1 << direction) & 0x38;
    if (north && hereCell->m_objectIndex != 0xff && !(hereCell->m_flags & 0x80)
        && hereCell->m_triggerType != 0xac)
        return 0;
    if (downMask && destCell->m_objectIndex != 0xff && !(destCell->m_flags & 0x80)
        && destCell->m_triggerType != 0xac)
        return 0;
    return 1;
}

// Buka CURSOR.cpp:1099 MoveOrigin; HoMM1 indexes the map directly.
VA(0x00407a58, 0x329)
void advManager::MoveOrigin(short directionX, short directionY)
{
    short oldOriginX;
    short oldOriginY;
    short cellX;
    short cellY;

    oldOriginX = m_mapOriginX;
    oldOriginY = m_mapOriginY;
    m_mapOriginX += directionX;
    m_mapOriginY += directionY;
    directionX = oldOriginX - m_mapOriginX;
    directionY = oldOriginY - m_mapOriginY;
    if (directionX != 0 || directionY != 0) {
        m_mapData[m_cursorMapX + oldOriginX][m_cursorMapY + oldOriginY].m_flags &= ~0x40;
        m_cursorMapX += directionX;
        m_cursorMapY += directionY;
        cellX = m_cursorMapX + m_mapOriginX;
        cellY = m_cursorMapY + m_mapOriginY;
        m_mapData[cellX][cellY].m_flags |= 0x40;
        if (m_previousCursorMapX != -1) {
            m_mapData[m_previousCursorMapX + oldOriginX][m_previousCursorMapY + oldOriginY].m_flags &=
                ~0x40;
            m_previousCursorMapX += directionX;
            m_previousCursorMapY += directionY;
            cellX = m_previousCursorMapX + m_mapOriginX;
            cellY = m_previousCursorMapY + m_mapOriginY;
            m_mapData[cellX][cellY].m_flags |= 0x40;
        }
    }
    m_forceCompleteDraw = 1;
}
