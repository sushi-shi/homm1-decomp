// HoMM1's CURSOR object (0x00405950-0x00407d8f): Buka SOURCE/CURSOR
// advManager movement routines, aligned apart from wingraph and TOWNMGR.

#include <match.h>

#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/X_GLOBAL.h>

// clang-format off
// Hero-cursor drawing and movement constants (Buka CURSOR.h CursorConstant
// and CURSOR.cpp CursorPrivateConstant names, HoMM1 values).
H1_ENUM_CONST_BEGIN(CursorConstant)
    CURSOR_DRAW_X = 0xe0,
    CURSOR_DRAW_Y = 0xff,
    CURSOR_BOAT_DRAW_Y_ADJUST = 10,
    CURSOR_SHADOW_FLIP_X_ADJUST = 0x20,
    CURSOR_FLAG_FRAME_BASE = 0x38,
    CURSOR_FLAG_FRAME_CYCLE_MASK = 3,
    CURSOR_LAST_FRAME_COUNT = 8,
    CURSOR_TURN_FRAME_COUNT = 16,
    CURSOR_SLOW_TURN_MULTIPLIER = 3,
    CURSOR_MAP_DRAW_OFFSET = 7,
    CURSOR_MOVE_HALF_TILE_PIXELS = 16,
    CURSOR_NORTH_DIRECTION_MASK = 0x83,
    CURSOR_SOUTH_DIRECTION_MASK = 0x38,
    CURSOR_DIAGONAL_DIRECTION_BIT = 1,
    CURSOR_TURN_TIMER_SLOT = 1,
    BOAT_OCCUPIED_FLAG = 0x80,
    SLOW_CURSOR_CYCLE_START = 2,
    SKIPPED_ANIMATION_FRAME = 4,
    FOOTSTEP_ANIMATION_FRAME = 3,
    DIRECTION_HALF_COUNT = 4,
    TURN_FRAME_MULTIPLIER = 2,
    MOVE_TILE_HALF_COUNT = 2
H1_ENUM_CONST_END(CursorConstant)

   // clang-format on

   // Buka CURSOR.cpp:50 StartCursor; HoMM1 keys the cycle off the global
   // walk speed and indexes the map directly.
   VA(0x00405950, 0x169)
void advManager::StartCursor(signed char direction) {
    short directionX;
    short newX;
    short directionY;
    short newY;

    m_cursorDirection = direction;
    m_cursorFrame = GetCursorBaseFrame(direction) + 1;
    if (gConfig.walkSpeed > WALK_SPEED_FIRST)
        m_cursorCycle = 1;
    else
        m_cursorCycle = SLOW_CURSOR_CYCLE_START;
    directionX = normalDirTable[direction].x;
    directionY = normalDirTable[direction].y;
    m_previousCursorMapX = m_cursorMapX;
    m_previousCursorMapY = m_cursorMapY;
    m_cursorMapX += directionX;
    m_cursorMapY += directionY;
    newX = m_mapOriginX + m_cursorMapX;
    newY = m_mapOriginY + m_cursorMapY;
    m_mapData[newX][newY].m_flags |= MAP_CELL_HERO_CURSOR;
}

// Buka CURSOR.cpp:78 StopCursor; HoMM1 also forgets the footstep samples.
VA(0x00405ab9, 0x150)
void advManager::StopCursor(signed char stopSound) {
    if (stopSound) {
        bMoveSoundMade = 1;
        m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
        m_cursorFrameCount = 0;
        EveryOther = 0;
        hPrevMoveSound = NULL;
        hLastMoveSound = NULL;
    }
    m_cursorCycle = 0;
    if (m_previousCursorMapX != CURSOR_CELL_NONE) {
        m_mapData[m_mapOriginX + m_previousCursorMapX][m_mapOriginY + m_previousCursorMapY]
            .m_flags &= ~MAP_CELL_HERO_CURSOR;
        m_previousCursorMapX = m_previousCursorMapY = CURSOR_CELL_NONE;
    }
    m_cursorTurning = 0;
}

// Buka CURSOR.cpp:99 DrawCursor; HoMM1 draws the hero shadow first and
// counts flag frames with m_updateMaxY.
VA(0x00405c09, 0x5e4)
void advManager::DrawCursor(void) {
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
    drawX = m_updateMinX + CURSOR_DRAW_X;
    screenY = m_updateMinY + CURSOR_DRAW_Y;
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
        screenY -= CURSOR_BOAT_DRAW_Y_ADJUST;
    if (m_cursorFrame & HERO_FRAME_MIRROR_FLAG) {
        drawX += CURSOR_SHADOW_FLIP_X_ADJUST;
        drawFrame = (m_cursorFrame & HERO_FRAME_INDEX_MASK) + m_cursorFrameCount;
        if (m_drawHeroShadows && m_cursorType != ADVMGR_HERO_ICON_BOAT)
            FlipDimIconToBitmap(
                m_boatShadowIcon,
                gpWindowManager->m_screen,
                drawX,
                screenY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        FlipIconToBitmap(
            m_heroIcons[m_cursorType],
            gpWindowManager->m_screen,
            drawX,
            screenY,
            drawFrame,
            ICON_DRAW_OFFSET_FULL
        );
        if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame & HERO_FRAME_INDEX_MASK;
            FlipIconToBitmap(
                m_boatFlagIcons[gpCurPlayer->m_color],
                gpWindowManager->m_screen,
                drawX,
                screenY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_updateMaxY & CURSOR_FLAG_FRAME_CYCLE_MASK)
                            + (m_cursorFrame & HERO_FRAME_INDEX_MASK) + CURSOR_FLAG_FRAME_BASE;
            FlipIconToBitmap(
                m_flagIcons[gpCurPlayer->m_color],
                gpWindowManager->m_screen,
                drawX,
                screenY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
            m_updateMaxY++;
        }
    } else {
        drawFrame = m_cursorFrame + m_cursorFrameCount;
        if (m_drawHeroShadows && m_cursorType != ADVMGR_HERO_ICON_BOAT)
            DimIconToBitmap(
                m_boatShadowIcon,
                gpWindowManager->m_screen,
                drawX,
                screenY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        IconToBitmap(
            m_heroIcons[m_cursorType],
            gpWindowManager->m_screen,
            drawX,
            screenY,
            drawFrame,
            ICON_DRAW_OFFSET_FULL
        );
        if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame;
            IconToBitmap(
                m_boatFlagIcons[gpCurPlayer->m_color],
                gpWindowManager->m_screen,
                drawX,
                screenY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_updateMaxY & CURSOR_FLAG_FRAME_CYCLE_MASK) + m_cursorFrame
                            + CURSOR_FLAG_FRAME_BASE;
            IconToBitmap(
                m_flagIcons[gpCurPlayer->m_color],
                gpWindowManager->m_screen,
                drawX,
                screenY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
            m_updateMaxY++;
        }
    }
    if (m_cursorCycle && gConfig.walkSpeed != WALK_SPEED_JUMP) {
        m_cursorFrameCount++;
        if (gConfig.walkSpeed == WALK_SPEED_GALLOP
            && (m_cursorFrameCount == SKIPPED_ANIMATION_FRAME || m_cursorFrameCount == 1))
            m_cursorFrameCount++;
        if (gConfig.walkSpeed == WALK_SPEED_WALK) {
            EveryOther = 1 - EveryOther;
            if (EveryOther)
                m_cursorFrameCount--;
        }
    }
    if (m_cursorFrameCount >= CURSOR_LAST_FRAME_COUNT)
        m_cursorFrameCount = 0;
    if (!m_cursorTurning) {
        if (m_cursorFrameCount == 0)
            hPrevMoveSound = hLastMoveSound;
        if (m_cursorFrameCount == FOOTSTEP_ANIMATION_FRAME
            || (gConfig.walkSpeed == WALK_SPEED_JUMP && !bMoveSoundMade)) {
            bMoveSoundMade = 1;
            if (!EveryOther)
                hLastMoveSound = gpSoundManager->MemorySample(
                    m_cursorSamples[giGroundToTerrain[GetCell(
                                                          m_mapOriginX + CURSOR_MAP_DRAW_OFFSET,
                                                          m_mapOriginY + CURSOR_MAP_DRAW_OFFSET
                    )
                                                          ->m_tileIndex]]
                );
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
short advManager::GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, short) direction) {
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
void advManager::TurnTo(signed char direction) {
    short frameStep = 1;
    short curFrame;
    short directionDifference = direction - m_cursorDirection;
    int delayTime;

    if (directionDifference == 0)
        return;
    if ((directionDifference < 0 && directionDifference >= -DIRECTION_HALF_COUNT)
        || (directionDifference > 0 && directionDifference > DIRECTION_HALF_COUNT))
        frameStep = -1;
    m_cursorTurning = 1;
    curFrame = m_cursorDirection * TURN_FRAME_MULTIPLIER;
    delayTime = giStepDelay[gConfig.walkSpeed];
    if (gConfig.walkSpeed == WALK_SPEED_WALK)
        delayTime *= CURSOR_SLOW_TURN_MULTIPLIER;
    if (gConfig.walkSpeed == WALK_SPEED_TROT)
        delayTime = delayTime * 1.5;
    do {
        m_cursorCycle = 1;
        if (m_cursorType >= ADVMGR_HERO_ICON_CLASS_END)
            m_cursorFrame = boatFrameFlip[curFrame];
        else
            m_cursorFrame = horseFrameFlip[curFrame];
        m_cursorFrameCount = 0;
        glTimers[CURSOR_TURN_TIMER_SLOT] = KBTickCount() + delayTime;
        if (gConfig.walkSpeed != WALK_SPEED_JUMP) {
            if (ComboDraw(m_mapOriginX, m_mapOriginY, 0))
                UpdateScreen(0, 0);
            if (bShowIt)
                DelayTil(&glTimers[CURSOR_TURN_TIMER_SLOT]);
        }
        curFrame += frameStep;
        if (curFrame < 0)
            curFrame = CURSOR_TURN_FRAME_COUNT - 1;
        curFrame %= CURSOR_TURN_FRAME_COUNT;
    } while (curFrame != direction * TURN_FRAME_MULTIPLIER);
    m_cursorDirection = direction;
    StopCursor(1);
    if (bShowIt)
        DelayTil(&glTimers[CURSOR_TURN_TIMER_SLOT]);
    if (ComboDraw(m_mapOriginX, m_mapOriginY, 0))
        UpdateScreen(0, 0);
}

// Buka CURSOR.cpp:429 GetMoveShowIt; HoMM1 reads the current hero itself
// and tests the watch player's high bit (0x004be7cc) directly in the
// map-extra grid.
VA(0x004064d6, 0x136)
int advManager::GetMoveShowIt(signed char direction) {
    short dy;
    hero* movingHero;
    short dirX;

    if (gpCurPlayer->CurrentHero() == INVALID_HERO)
        return 0;
    movingHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    dirX = normalDirTable[direction].x;
    dy = normalDirTable[direction].y;
    if ((gbThisNetHumanPlayer[giCurPlayer] || (!gConfig.blackoutComputer && !gbRemoteOn))
        && ((gpGame->m_mapExtra[movingHero->m_x][movingHero->m_y] & giCurWatchPlayerHighBit)
            || (gpGame->m_mapExtra[movingHero->m_x + dirX][movingHero->m_y + dy]
                & giCurWatchPlayerHighBit)))
        return 1;
    else
        return 0;
}

// Buka CURSOR.cpp MoveHero; HoMM1 recomputes the step cost from the hero
// type, parks the boat on a coast step and has no deferred object draw.

VA(0x0040660c, 0xe1e)
mapCell* advManager::MoveHero(
    signed char direction,
    signed char stopAfterMove,
    int* eventX,
    int* eventY,
    int* outOfMobility,
    signed char processEvent,
    signed char* adjacentMonster
) {
    mapCell* pCursorCell;
    int step;
    int origX;
    mapCell* nextCell;
    hero* movingHero;
    int origY;
    signed char terrain;
    mapCell* retCell;
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
    retCell = NULL;
    movingHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    origX = movingHero->m_x;
    origY = movingHero->m_y;
    xInc = normalDirTable[direction].x;
    yInc = normalDirTable[direction].y;
    bShowIt = GetMoveShowIt(direction);
    terrain = giGroundToTerrain[GetCell(movingHero->m_x, movingHero->m_y)->m_tileIndex];
    nextCell = GetCell(movingHero->m_x + xInc, movingHero->m_y + yInc);
    if (CalcTerrainCost(
            terrain,
            direction & CURSOR_DIAGONAL_DIRECTION_BIT,
            movingHero->m_remainingMobility,
            movingHero->m_heroClass
        )
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
    if ((movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
        && nextCell->m_triggerType == MAP_OBJECT_COAST) {
        boatRecord* boat;
        mapCell* boatCell;

        for (step = 0; step < GAME_BOAT_COUNT; step++) {
            if (gpGame->m_boats[step].heroId == movingHero->m_id)
                break;
        }
        boat = &gpGame->m_boats[step];
        boatCell = GetCell(movingHero->m_x, movingHero->m_y);
        boat->savedTriggerType = boatCell->m_triggerType;
        boat->savedEventData = boatCell->m_objectMetadata;
        boat->direction = m_cursorDirection;
        boat->heroId |= BOAT_OCCUPIED_FLAG;
        boatCell->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP);
        boatCell->m_objectMetadata = step;
        boat->x = movingHero->m_x;
        boat->y = movingHero->m_y;
        StopCursor(1);
        CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
        UpdateScreen(0, 0);
        m_cursorActive = 0;
    }
    if (nextCell->m_triggerType & MAP_TRIGGER_EVENT) {
        switch (nextCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
            case MAP_OBJECT_SHIP:
                if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    goto movementDone;
                StopCursor(1);
                m_cursorActive = 0;
                gpWindowManager->SaveFizzleSource(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT
                );
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager->FizzleForward(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT,
                    FIZZLE_USE_DEFAULT_DELAY
                );
                break;
            case MAP_OBJECT_BUOY:
                if (!(movingHero->m_eventFlags & HERO_EVENT_EMBARKED))
                    goto movementDone;
                else
                    goto stoppingEvent;
            case MAP_OBJECT_HERO:
                if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                    if (gpGame->GetHero(nextCell->m_objectMetadata)->m_eventFlags
                        & HERO_EVENT_EMBARKED)
                        goto stoppingEvent;
                    else
                        goto movementDone;
                }
            case MAP_OBJECT_SIGNPOST:
            case MAP_OBJECT_SKELETON:
            case MAP_OBJECT_TREASURE_CHEST:
            case MAP_OBJECT_CAMPFIRE:
            case MAP_OBJECT_FOUNTAIN:
            case MAP_OBJECT_ANCIENT_LAMP:
            case MAP_OBJECT_MONSTER:
            case MAP_OBJECT_OBELISK:
            case MAP_OBJECT_OASIS:
            case MAP_OBJECT_RESOURCE:
            case MAP_OBJECT_STATUE:
            case MAP_OBJECT_WELL:
            case MAP_OBJECT_ARTIFACT:
                if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    goto movementDone;
            stoppingEvent:
                StopCursor(1);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                UpdateScreen(0, 0);
                movingHero->m_remainingMobility -= CalcTerrainCost(
                    terrain,
                    direction & CURSOR_DIAGONAL_DIRECTION_BIT,
                    movingHero->m_remainingMobility,
                    movingHero->m_heroClass
                );
                if (CalcTerrainCost(
                        giGroundToTerrain[nextCell->m_tileIndex],
                        0,
                        movingHero->m_remainingMobility,
                        movingHero->m_heroClass
                    )
                    > movingHero->m_remainingMobility) {
                    movingHero->m_remainingMobility = 0;
                    stopAfterMove = 1;
                }
                retCell = nextCell;
                goto movementDone;
            case MAP_OBJECT_TOWN:
                if (gpGame->GetTown(nextCell->m_objectMetadata)->m_owner != giCurPlayer
                    && gpGame->GetTown(nextCell->m_objectMetadata)->HasGarrison()) {
                    StopCursor(1);
                    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                    UpdateScreen(0, 0);
                    movingHero->m_remainingMobility -= CalcTerrainCost(
                        terrain,
                        direction & CURSOR_DIAGONAL_DIRECTION_BIT,
                        movingHero->m_remainingMobility,
                        movingHero->m_heroClass
                    );
                    if (CalcTerrainCost(
                            giGroundToTerrain[nextCell->m_tileIndex],
                            0,
                            movingHero->m_remainingMobility,
                            movingHero->m_heroClass
                        )
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
    if (movingHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
        town* occupiedTown;

        occupiedTown = gpGame->GetTown(movingHero->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (m_routeShown)
        *(m_visibilityMap + (movingHero->m_x + xInc)
          + (movingHero->m_y + yInc) * MAP_CELL_GRID_SIZE) = 0;
    m_updateMinX = m_updateMinY = 0;
    gpGame->SetVisibility(
        m_mapOriginX + xInc + CURSOR_MAP_DRAW_OFFSET,
        m_mapOriginY + yInc + CURSOR_MAP_DRAW_OFFSET,
        giCurPlayer,
        gHeroScoutRadius[movingHero->m_heroClass]
    );
    m_forceCompleteDraw = 1;
    pixelsPerStep = giPixelsPerStep[gConfig.walkSpeed];
    msDelay = giStepDelay[gConfig.walkSpeed];
    StartCursor(direction);
    if (gConfig.walkSpeed == WALK_SPEED_JUMP) {
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
        numSteps = CURSOR_MOVE_HALF_TILE_PIXELS / pixelsPerStep;
        for (step = 0; step < numSteps * MOVE_TILE_HALF_COUNT; step++) {
            long tick;

            if (step == numSteps) {
                MoveOrigin(xInc, yInc);
                movingHero->m_x += xInc;
                movingHero->m_y += yInc;
                m_updateMinX = startVals[xInc + 1];
                m_updateMinY = startVals[yInc + 1];
            }
            tick = KBTickCount();
            if (step + 1 == numSteps * MOVE_TILE_HALF_COUNT) {
                m_updateMinX = 0;
                m_updateMinY = 0;
            } else {
                m_updateMinX += xInc * pixelsPerStep;
                m_updateMinY += yInc * pixelsPerStep;
            }
            if (ComboDraw(0)) {
                giLimitUpdMinX = UPDATE_NONE;
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
        terrain,
        direction & CURSOR_DIAGONAL_DIRECTION_BIT,
        movingHero->m_remainingMobility,
        movingHero->m_heroClass
    );
    if (CalcTerrainCost(
            giGroundToTerrain[nextCell->m_tileIndex],
            0,
            movingHero->m_remainingMobility,
            movingHero->m_heroClass
        )
        > movingHero->m_remainingMobility) {
        movingHero->m_remainingMobility = 0;
        stopAfterMove = 1;
    }
    StopCursor(stopAfterMove);
    if (processEvent && stopAfterMove && ComboDraw(0))
        UpdateScreen(0, 0);
    SetEnvironmentOrigin(
        m_mapOriginX + CURSOR_MAP_DRAW_OFFSET,
        m_mapOriginY + CURSOR_MAP_DRAW_OFFSET,
        0
    );
    step = GetCell(m_mapOriginX + CURSOR_MAP_DRAW_OFFSET, m_mapOriginY + CURSOR_MAP_DRAW_OFFSET)
               ->m_tileIndex;
    if (giGroundToTerrain[step] != m_currentTerrain && step % MAP_CELL_TILES_PER_TERRAIN < 4) {
        m_currentTerrain = giGroundToTerrain[step];
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    m_updateMinX = m_updateMinY = 0;
    pCursorCell = GetCell(m_cursorMapX + m_mapOriginX, m_cursorMapY + m_mapOriginY);
    *eventX = m_cursorMapX + m_mapOriginX;
    *eventY = m_cursorMapY + m_mapOriginY;
    if ((pCursorCell->m_triggerType & MAP_TRIGGER_EVENT)
        || ((movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
            && pCursorCell->m_triggerType == MAP_OBJECT_COAST)) {
        retCell = pCursorCell;
        switch (pCursorCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
            case MAP_OBJECT_ROSEBUSH:
            case MAP_OBJECT_TREE_STUMP:
            case MAP_OBJECT_OAK_TREE:
            case MAP_OBJECT_NOTHING_HERE:
            case 50:
            case 51:
            case MAP_OBJECT_MOUNTAINS:
            case MAP_OBJECT_MOUNTAINS_2:
            case MAP_OBJECT_MOUNTAINS_3:
            case MAP_OBJECT_MOUNTAINS_4:
            case MAP_OBJECT_TREES:
            case MAP_OBJECT_TREES_2:
            case MAP_OBJECT_TREES_3:
            case MAP_OBJECT_TREES_4:
            case MAP_OBJECT_TREES_5:
                retCell = NULL;
                break;
        }
        goto movementDone;
    } else
        goto movementDone;
movementDone:
    UpdateRadar(1, 1);
    gbHeroMoving = 0;
    if (movingHero->m_x != origX || movingHero->m_y != origY) {
        if (mapExtra[movingHero->m_x][movingHero->m_y] & MAP_EXTRA_MONSTER_ADJACENT) {
            if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED)
                goto adjacentDone;
            if (retCell
                && static_cast<char>(retCell->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       == MAP_OBJECT_SHIP)
                goto adjacentDone;
            CheckAdjacentMon(adjacentMonster);
            if (movingHero->m_owner == GAME_PLAYER_NONE)
                retCell = NULL;
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
void advManager::CheckAdjacentMon(signed char* adjacentMonster) {
    int monX;
    int monY;
    hero* theHero;
    signed char dead;
    mapCell* heroCell;
    mapCell* monsterCell;

    theHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    dead = 0;
    if (FindAdjacentMonster(
            theHero->m_x,
            theHero->m_y,
            &monX,
            &monY,
            SEARCH_INVALID_COORDINATE,
            SEARCH_INVALID_COORDINATE
        )) {
        StopCursor(1);
        CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
        UpdateScreen(0, 0);
        monsterCell = GetCell(monX, monY);
        heroCell = GetCell(theHero->m_x, theHero->m_y);
        if (gbThisNetHumanPlayer[giCurPlayer])
            PlayerMonsterInteract(
                monsterCell,
                heroCell,
                theHero,
                &dead,
                theHero->m_x,
                theHero->m_y,
                1,
                monX,
                monY
            );
        else
            ComputerMonsterInteract(monsterCell, theHero, &dead);
        if (dead) {
            EraseObj(monsterCell, monX, monY);
            if (gbThisNetHumanPlayer[giCurPlayer])
                FizzleCenter(EVENT_FIZZLE_HERO_LOSS);
        }
        *adjacentMonster = 1;
    }
}

// Buka CURSOR.cpp:962 ValidMoveWithEvent; HoMM1 lets a boat meet another
// boat, forbids landing a boat on most objects and defers the rest to
// ValidMove.
VA(0x004075ab, 0x20a)
short advManager::ValidMoveWithEvent(hero* movingHero, short direction) {
    short deltaY;
    short newY;
    short deltaX;
    short newX;
    mapCell* cell;

    deltaX = normalDirTable[direction].x;
    deltaY = normalDirTable[direction].y;
    newX = movingHero->m_x + deltaX;
    newY = movingHero->m_y + deltaY;
    if (newX < 0 || newX > MAP_CELL_GRID_SIZE - 1 || newY < 0 || newY > MAP_CELL_GRID_SIZE - 1)
        return 0;
    cell = &m_mapData[newX][newY];
    switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
        case MAP_OBJECT_BUOY:
            if (!(movingHero->m_eventFlags & HERO_EVENT_EMBARKED))
                return 1;
            else
                return 0;
        case MAP_OBJECT_HERO:
            if (movingHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                if (gpGame->GetHero(cell->m_objectMetadata)->m_eventFlags & HERO_EVENT_EMBARKED)
                    return 1;
                else
                    return 0;
            }
        case MAP_OBJECT_SIGNPOST:
        case MAP_OBJECT_SKELETON:
        case MAP_OBJECT_TREASURE_CHEST:
        case MAP_OBJECT_CAMPFIRE:
        case MAP_OBJECT_FOUNTAIN:
        case MAP_OBJECT_ANCIENT_LAMP:
        case MAP_OBJECT_MONSTER:
        case MAP_OBJECT_OBELISK:
        case MAP_OBJECT_OASIS:
        case MAP_OBJECT_RESOURCE:
        case MAP_OBJECT_STATUE:
        case MAP_OBJECT_WELL:
        case MAP_OBJECT_ARTIFACT:
            if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
                return 0;
            else
                return 1;
    }
    return ValidMove(direction);
}

// Buka CURSOR.cpp:1006 ValidMove; HoMM1 indexes from the cursor's map
// position and tests the north/south object masks directly.
VA(0x004077b5, 0x2a3)
short advManager::ValidMove(short direction) {
    short directionX;
    short downMask;
    mapCell* hereCell;
    mapCell* destCell;
    short directionY;
    short north;
    short newX;
    short newY;

    directionX = normalDirTable[direction].x;
    directionY = normalDirTable[direction].y;
    newX = m_mapOriginX + directionX;
    newY = m_mapOriginY + directionY;
    if (newX < -CURSOR_MAP_DRAW_OFFSET || newX > MAP_CELL_GRID_SIZE - CURSOR_MAP_DRAW_OFFSET - 1)
        return 0;
    if (newY < -CURSOR_MAP_DRAW_OFFSET || newY > MAP_CELL_GRID_SIZE - CURSOR_MAP_DRAW_OFFSET - 1)
        return 0;
    destCell = &m_mapData[m_cursorMapX + newX][m_cursorMapY + newY];
    if (destCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)
        return 0;
    if (giGroundToTerrain[destCell->m_tileIndex] == TERRAIN_WATER) {
        if (m_cursorType != ADVMGR_HERO_ICON_BOAT
            && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
            && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK))
            return 0;
    } else {
        if (m_cursorType == ADVMGR_HERO_ICON_BOAT && destCell->m_triggerType != MAP_OBJECT_COAST
            && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_WHIRLPOOL))
            return 0;
    }
    hereCell = &m_mapData[m_cursorMapX + m_mapOriginX][m_cursorMapY + m_mapOriginY];
    north = (1 << direction) & CURSOR_NORTH_DIRECTION_MASK;
    downMask = (1 << direction) & CURSOR_SOUTH_DIRECTION_MASK;
    if (north && hereCell->m_objectIndex != MAP_CELL_NO_FRAME
        && !(hereCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
        && hereCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_WHIRLPOOL))
        return 0;
    if (downMask && destCell->m_objectIndex != MAP_CELL_NO_FRAME
        && !(destCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
        && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_WHIRLPOOL))
        return 0;
    return 1;
}

// Buka CURSOR.cpp:1099 MoveOrigin; HoMM1 indexes the map directly.
VA(0x00407a58, 0x329)
void advManager::MoveOrigin(short directionX, short directionY) {
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
        m_mapData[m_cursorMapX + oldOriginX][m_cursorMapY + oldOriginY].m_flags &=
            ~MAP_CELL_HERO_CURSOR;
        m_cursorMapX += directionX;
        m_cursorMapY += directionY;
        cellX = m_cursorMapX + m_mapOriginX;
        cellY = m_cursorMapY + m_mapOriginY;
        m_mapData[cellX][cellY].m_flags |= MAP_CELL_HERO_CURSOR;
        if (m_previousCursorMapX != CURSOR_CELL_NONE) {
            m_mapData[m_previousCursorMapX + oldOriginX][m_previousCursorMapY + oldOriginY]
                .m_flags &= ~MAP_CELL_HERO_CURSOR;
            m_previousCursorMapX += directionX;
            m_previousCursorMapY += directionY;
            cellX = m_previousCursorMapX + m_mapOriginX;
            cellY = m_previousCursorMapY + m_mapOriginY;
            m_mapData[cellX][cellY].m_flags |= MAP_CELL_HERO_CURSOR;
        }
    }
    m_forceCompleteDraw = 1;
}

// CURSOR owns retail .data 0x0048eb18-0x0048eb4f (initialized, before the
// TOWNMGR band) and .bss 0x004a4b80-0x004a4b97. Initializers are retail bytes.
DATA(0x0048eb18)
signed char bMoveSoundMade = 1;
DATA(0x0048eb20)
short giPixelsPerStep[5] = {1, 4, 6, 8, 16};
DATA(0x0048eb30)
short giStepDelay[5] = {30, 45, 30, 15, 15};
DATA(0x0048eb3c)
struct _SAMPLE* hPrevMoveSound = NULL;
DATA(0x0048eb40)
struct _SAMPLE* hLastMoveSound = NULL;
DATA(0x0048eb44)
signed char EveryOther = 0;
DATA(0x0048eb48)
short startVals[3] = {16, 0, -16};
DATA(0x004a4b80)
short S1cursorCycle;
DATA(0x004a4b84)
short S1cursorFrameCount;
DATA(0x004a4b88)
short S1cursorTurning;
DATA(0x004a4b8c)
short S1cursorBaseFrame;
DATA(0x004a4b90)
signed char S1cursorDirection;
