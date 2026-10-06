// CURSOR object (0x00439ee0-0x00407d8f): advManager movement routines,
// aligned apart from wingraph and TOWNMGR.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/inputManager.h>
#include <BASE/mouseManager.h>
#include <SOURCE/advManager.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>

// Keys the cycle off the global walk speed and indexes the map directly.
VA(0x004215c0, 0x143)
void advManager::StartCursor(i8 direction) {
    i16 deltaX;
    i16 newX;
    i16 deltaY;
    i16 newY;

    m_cursorDirection = direction;
    m_cursorFrame = GetCursorBaseFrame(direction) + 1;
    m_cursorCycle = gConfig.walkSpeed > WALK_SPEED_FIRST ? 1 : SLOW_CURSOR_CYCLE_START;
    deltaX = gNormalDirTable[direction].x;
    deltaY = gNormalDirTable[direction].y;
    m_previousCursorMapX = m_cursorMapX;
    m_previousCursorMapY = m_cursorMapY;
    m_cursorMapX += deltaX;
    m_cursorMapY += deltaY;
    newX = m_mapOriginX + m_cursorMapX;
    newY = m_mapOriginY + m_cursorMapY;
    m_mapData[newX][newY].m_flags |= MAP_CELL_HERO_CURSOR;
}

// Also forgets the footstep samples.
VA(0x00421703, 0x11d)
void advManager::StopCursor(i8 stopSound) {
    if (stopSound) {
        gMoveSoundMade = 1;
        m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
        m_cursorFrameCount = 0;
        gEveryOther = 0;
    }
    m_cursorCycle = 0;
    if (m_previousCursorMapX != CURSOR_CELL_NONE) {
        m_mapData[m_mapOriginX + m_previousCursorMapX][m_mapOriginY + m_previousCursorMapY]
            .m_flags &= ~MAP_CELL_HERO_CURSOR;
        m_previousCursorMapX = m_previousCursorMapY = CURSOR_CELL_NONE;
    }
    m_cursorTurning = 0;
}

// Draws the hero shadow first and counts flag frames with m_flagFrameCounter.
VA(0x00421820, 0x5a4)
void advManager::DrawCursor(void) {
    i16 drawX;
    i16 posY;
    i16 drawFrame;

    if (gShowIt == 0 || gSpecialHideCursor)
        return;
    if (gDrawSavedCursor) {
        m_cursorDirection = gSavedCursorDirection;
        m_cursorFrame = gSavedCursorBaseFrame;
        m_cursorFrameCount = gSavedCursorFrameCount;
        m_cursorCycle = gSavedCursorCycle;
        m_cursorTurning = gSavedCursorTurning;
    }
    drawX = m_scrollOffsetX + CURSOR_DRAW_X;
    posY = m_scrollOffsetY + CURSOR_DRAW_Y;
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
        posY -= CURSOR_BOAT_DRAW_Y_ADJUST;
    if (m_cursorFrame & HERO_FRAME_MIRROR_FLAG) {
        drawX += CURSOR_SHADOW_FLIP_X_ADJUST;
        drawFrame = (m_cursorFrame & HERO_FRAME_INDEX_MASK) + m_cursorFrameCount;
        if (m_drawHeroShadows && m_cursorType != ADVMGR_HERO_ICON_BOAT)
            FlipDimIconToBitmap(
                m_shadowIcon,
                gWindowManager->m_screen,
                drawX,
                posY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        FlipIconToBitmap(
            m_heroIcons[m_cursorType],
            gWindowManager->m_screen,
            drawX,
            posY,
            drawFrame,
            ICON_DRAW_OFFSET_FULL
        );
        if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame & HERO_FRAME_INDEX_MASK;
            FlipIconToBitmap(
                m_boatFlagIcons[gCurPlayerData->m_color],
                gWindowManager->m_screen,
                drawX,
                posY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_cursorFrame & HERO_FRAME_INDEX_MASK)
                            + (m_flagFrameCounter & CURSOR_FLAG_FRAME_CYCLE_MASK)
                            + CURSOR_FLAG_FRAME_BASE;
            FlipIconToBitmap(
                m_flagIcons[gCurPlayerData->m_color],
                gWindowManager->m_screen,
                drawX,
                posY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
            m_flagFrameCounter++;
        }
    } else {
        drawFrame = m_cursorFrame + m_cursorFrameCount;
        if (m_drawHeroShadows && m_cursorType != ADVMGR_HERO_ICON_BOAT)
            DimIconToBitmap(
                m_shadowIcon,
                gWindowManager->m_screen,
                drawX,
                posY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        IconToBitmap(
            m_heroIcons[m_cursorType],
            gWindowManager->m_screen,
            drawX,
            posY,
            drawFrame,
            ICON_DRAW_OFFSET_FULL
        );
        if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame;
            IconToBitmap(
                m_boatFlagIcons[gCurPlayerData->m_color],
                gWindowManager->m_screen,
                drawX,
                posY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
        } else {
            if (m_cursorCycle == 0)
                drawFrame = m_cursorFrame + (m_flagFrameCounter & CURSOR_FLAG_FRAME_CYCLE_MASK)
                            + CURSOR_FLAG_FRAME_BASE;
            IconToBitmap(
                m_flagIcons[gCurPlayerData->m_color],
                gWindowManager->m_screen,
                drawX,
                posY,
                drawFrame,
                ICON_DRAW_OFFSET_FULL
            );
            m_flagFrameCounter++;
        }
    }
    if (m_cursorCycle && gConfig.walkSpeed != WALK_SPEED_JUMP) {
        m_cursorFrameCount++;
        if (gConfig.walkSpeed == WALK_SPEED_GALLOP
            && (m_cursorFrameCount == SKIPPED_ANIMATION_FRAME || m_cursorFrameCount == 1))
            m_cursorFrameCount++;
        if (gConfig.walkSpeed == WALK_SPEED_WALK) {
            gEveryOther = 1 - gEveryOther;
            if (gEveryOther)
                m_cursorFrameCount--;
        }
    }
    if (m_cursorFrameCount >= CURSOR_LAST_FRAME_COUNT)
        m_cursorFrameCount = 0;
    if (!m_cursorTurning) {
        if (m_cursorFrameCount == FOOTSTEP_ANIMATION_FRAME
            || (gConfig.walkSpeed == WALK_SPEED_JUMP && !gMoveSoundMade)) {
            gMoveSoundMade = 1;
            if (!gEveryOther)
                PlaySample(
                    m_cursorSamples[CELL_TERRAIN(GetCell(
                        m_mapOriginX + ADVMGR_VIEW_CENTER,
                        m_mapOriginY + ADVMGR_VIEW_CENTER
                    ))]
                );
        }
    }
    if (!gDrawSavedCursor) {
        gSavedCursorDirection = m_cursorDirection;
        gSavedCursorBaseFrame = m_cursorFrame;
        gSavedCursorFrameCount = m_cursorFrameCount;
        gSavedCursorCycle = m_cursorCycle;
        gSavedCursorTurning = m_cursorTurning;
    }
}

VA(0x00421dc4, 0x51)
i16 advManager::GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, i16) direction) {
    if (static_cast<i32>(direction) > static_cast<i32>(MAP_DIRECTION_SOUTH)) {
        switch (direction) {
            case MAP_DIRECTION_SOUTH_WEST:
                return CURSOR_BOAT_BASE_FRAME_5;
            case MAP_DIRECTION_WEST:
                return CURSOR_BOAT_BASE_FRAME_6;
            case MAP_DIRECTION_NORTH_WEST:
                return CURSOR_BOAT_BASE_FRAME_7;
            default:
                return 0;
        }
    } else {
        return static_cast<i32>(direction) * CURSOR_FRAMES_PER_DIRECTION;
    }
}

// Sixteen half-step frames and word-sized step delays.
VA(0x00421e15, 0x20e)
void advManager::TurnTo(i8 direction) {
    i16 inc = 1;
    i16 frameIndex;
    i16 directionDifference = direction - m_cursorDirection;
    i32 delayTime;

    if (directionDifference == 0)
        return;
    if ((directionDifference < 0 && directionDifference >= -DIRECTION_HALF_COUNT)
        || (directionDifference > 0 && directionDifference > DIRECTION_HALF_COUNT))
        inc = -1;
    m_cursorTurning = 1;
    frameIndex = m_cursorDirection * TURN_FRAME_MULTIPLIER;
    delayTime = gStepDelay[gConfig.walkSpeed];
    if (gConfig.walkSpeed == WALK_SPEED_WALK)
        delayTime *= CURSOR_SLOW_TURN_MULTIPLIER;
    if (gConfig.walkSpeed == WALK_SPEED_TROT)
        delayTime = delayTime * 1.5;
    do {
        m_cursorCycle = 1;
        m_cursorFrame = m_cursorType < ADVMGR_HERO_ICON_CLASS_END ? gHorseFrameFlip[frameIndex]
                                                                  : gBoatFrameFlip[frameIndex];
        m_cursorFrameCount = 0;
        gTimers[CURSOR_TURN_TIMER_SLOT] = KBTickCount() + delayTime;
        if (gConfig.walkSpeed != WALK_SPEED_JUMP) {
            if (ComboDraw(m_mapOriginX, m_mapOriginY, 0))
                UpdateScreen(0, 0);
            if (gShowIt)
                DelayTil(&gTimers[CURSOR_TURN_TIMER_SLOT]);
        }
        frameIndex += inc;
        if (frameIndex < 0)
            frameIndex = CURSOR_TURN_FRAME_COUNT - 1;
        frameIndex %= CURSOR_TURN_FRAME_COUNT;
    } while (frameIndex != direction * TURN_FRAME_MULTIPLIER);
    m_cursorDirection = direction;
    StopCursor(1);
    if (gShowIt)
        DelayTil(&gTimers[CURSOR_TURN_TIMER_SLOT]);
    if (ComboDraw(m_mapOriginX, m_mapOriginY, 0))
        UpdateScreen(0, 0);
}

// Reads the current hero itself and tests the watch player's high bit
// (0x004be7cc) directly in the map-extra grid.
VA(0x00422023, 0x104)
i32 advManager::GetMoveShowIt(i8 direction) {
    i16 dy;
    hero* movingHero;
    i16 dx;

    if (gCurPlayerData->CurrentHero() == HERO_ID_NONE)
        return 0;
    movingHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    dx = gNormalDirTable[direction].x;
    dy = gNormalDirTable[direction].y;
    if ((gThisNetHumanPlayer[gCurPlayer] || (!gConfig.blackoutComputer && !gRemoteOn))
        && ((gGame->m_mapExtra[movingHero->m_x][movingHero->m_y] & gCurWatchPlayerHighBit)
            || (gGame->m_mapExtra[movingHero->m_x + dx][movingHero->m_y + dy]
                & gCurWatchPlayerHighBit)))
        return 1;
    else
        return 0;
}

// Recomputes the step cost from the hero type and parks the boat on a
// coast step.

VA(0x00422127, 0xccd)
mapCell* advManager::MoveHero(
    i8 direction,
    i8 stopAfterMove,
    i32* eventX,
    i32* eventY,
    i32* outOfMobility,
    i8 processEvent,
    i8* adjacentMonster
) {
    mapCell* cellPtr;
    i32 inc;
    i32 posX;
    mapCell* nextCellItem;
    i8 theTerrain;
    mapCell* retCell;
    i16 xInc;
    i16 yInc;
    i16 pixelsPerStep;
    hero* champion;
    i32 startY;
    i32 delayNum;
    i16 numSteps;

    if (gThisNetHumanPlayer[gCurPlayer])
        SetNoDialogMenus(0);
    *adjacentMonster = 0;
    *outOfMobility = 0;
    gHeroMoving = 1;
    retCell = NULL;
    champion = gGame->GetHero(gCurPlayerData->m_currentHero);
    posX = champion->m_x;
    startY = champion->m_y;
    xInc = gNormalDirTable[direction].x;
    yInc = gNormalDirTable[direction].y;
    gShowIt = GetMoveShowIt(direction);
    theTerrain = CELL_TERRAIN(GetCell(champion->m_x, champion->m_y));
    nextCellItem = GetCell(champion->m_x + xInc, champion->m_y + yInc);
    if (champion->m_remainingMobility < CalcTerrainCost(
            theTerrain,
            direction & CURSOR_DIAGONAL_DIRECTION_BIT,
            champion->m_remainingMobility,
            champion->m_heroClass
        )) {
        *outOfMobility = 1;
        StopCursor(1);
        goto movementDone;
    }
    MobilizeCurrHero(0);
    *eventX = champion->m_x + xInc;
    *eventY = champion->m_y + yInc;
    if (m_cursorDirection != direction)
        TurnTo(direction);
    champion->m_direction = direction;
    if (champion->IsEmbarked() && nextCellItem->m_triggerType == MAP_OBJECT_COAST) {
        boatRecord* boat;
        mapCell* boatCell;

        for (inc = 0; inc < GAME_BOAT_COUNT; inc++) {
            if (gGame->m_boats[inc].heroId == champion->m_id)
                break;
        }
        boat = &gGame->m_boats[inc];
        boatCell = GetCell(champion->m_x, champion->m_y);
        boat->savedTriggerType = boatCell->m_triggerType;
        boat->savedEventData = boatCell->m_objectMetadata;
        boat->direction = m_cursorDirection;
        boat->heroId |= BOAT_OCCUPIED_FLAG;
        boatCell->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP);
        boatCell->m_objectMetadata = inc;
        boat->x = champion->m_x;
        boat->y = champion->m_y;
        StopCursor(1);
        CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
        UpdateScreen(0, 0);
        m_cursorActive = 0;
    }
    if (nextCellItem->m_triggerType & MAP_TRIGGER_EVENT) {
        switch (nextCellItem->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
            case MAP_OBJECT_SHIP:
                if (champion->IsEmbarked())
                    goto movementDone;
                StopCursor(1);
                m_cursorActive = 0;
                gWindowManager->SaveFizzleSource(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT
                );
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gWindowManager->FizzleForward(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT,
                    FIZZLE_USE_DEFAULT_DELAY
                );
                break;
            case MAP_OBJECT_BUOY:
                if (!champion->IsEmbarked())
                    goto movementDone;
                else
                    goto stoppingEvent;
            case MAP_OBJECT_HERO:
                if (champion->IsEmbarked()) {
                    if (gGame->GetHero(nextCellItem->m_objectMetadata)->IsEmbarked())
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
                if (champion->IsEmbarked())
                    goto movementDone;
            stoppingEvent:
                StopCursor(1);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                UpdateScreen(0, 0);
                champion->m_remainingMobility -= CalcTerrainCost(
                    theTerrain,
                    direction & CURSOR_DIAGONAL_DIRECTION_BIT,
                    champion->m_remainingMobility,
                    champion->m_heroClass
                );
                if (champion->m_remainingMobility < CalcTerrainCost(
                        CELL_TERRAIN(nextCellItem),
                        0,
                        champion->m_remainingMobility,
                        champion->m_heroClass
                    )) {
                    champion->m_remainingMobility = 0;
                    stopAfterMove = 1;
                }
                retCell = nextCellItem;
                goto movementDone;
            case MAP_OBJECT_TOWN:
                if (gGame->GetTown(nextCellItem->m_objectMetadata)->m_owner != gCurPlayer
                    && gGame->GetTown(nextCellItem->m_objectMetadata)->HasGarrison()) {
                    StopCursor(1);
                    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                    UpdateScreen(0, 0);
                    champion->m_remainingMobility -= CalcTerrainCost(
                        theTerrain,
                        direction & CURSOR_DIAGONAL_DIRECTION_BIT,
                        champion->m_remainingMobility,
                        champion->m_heroClass
                    );
                    if (champion->m_remainingMobility < CalcTerrainCost(
                            CELL_TERRAIN(nextCellItem),
                            0,
                            champion->m_remainingMobility,
                            champion->m_heroClass
                        )) {
                        champion->m_remainingMobility = 0;
                        stopAfterMove = 1;
                    }
                    retCell = nextCellItem;
                    goto movementDone;
                }
                break;
            default:
                break;
        }
    }
    if (!ValidMove(direction))
        goto movementDone;
    if (champion->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
        town* occupiedTown;

        occupiedTown = gGame->GetTown(champion->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (m_routeShown)
        *(m_routeMap + (champion->m_x + xInc) + (champion->m_y + yInc) * MAP_CELL_GRID_SIZE) = 0;
    m_scrollOffsetX = m_scrollOffsetY = 0;
    gGame->SetVisibility(
        m_mapOriginX + xInc + ADVMGR_VIEW_CENTER,
        m_mapOriginY + yInc + ADVMGR_VIEW_CENTER,
        gCurPlayer,
        gHeroScoutRadius[champion->m_heroClass]
    );
    m_forceCompleteDraw = 1;
    pixelsPerStep = gPixelsPerStep[gConfig.walkSpeed];
    delayNum = gStepDelay[gConfig.walkSpeed];
    StartCursor(direction);
    if (gConfig.walkSpeed == WALK_SPEED_JUMP) {
        if (gEveryOther)
            m_cursorFrame--;
        gMoveSoundMade = 0;
        MoveOrigin(xInc, yInc);
        champion->m_x += xInc;
        champion->m_y += yInc;
        if (ComboDraw(0))
            UpdateScreen(0, 0);
        gEveryOther = 1 - gEveryOther;
    } else {
        gEnlargeScreenBlit = 0;
        gNoBorder = 1;
        numSteps = CURSOR_MOVE_HALF_TILE_PIXELS / pixelsPerStep;
        for (inc = 0; inc < numSteps * MOVE_TILE_HALF_COUNT; inc++) {
            i32 tick;

            if (inc == numSteps) {
                MoveOrigin(xInc, yInc);
                champion->m_x += xInc;
                champion->m_y += yInc;
                m_scrollOffsetX = gStepScrollStart[xInc + 1];
                m_scrollOffsetY = gStepScrollStart[yInc + 1];
            }
            tick = KBTickCount();
            if (inc + 1 == numSteps * MOVE_TILE_HALF_COUNT) {
                m_scrollOffsetX = 0;
                m_scrollOffsetY = 0;
            } else {
                m_scrollOffsetX += xInc * pixelsPerStep;
                m_scrollOffsetY += yInc * pixelsPerStep;
            }
            if (ComboDraw(0)) {
                gLimitUpdMinX = UPDATE_NONE;
                UpdateScreen(0, 0);
            }
            if (gShowIt)
                DelayTilMilli(tick + delayNum);
        }
        gNoBorder = 0;
        DrawAdventureBorder();
        gEnlargeScreenBlit = 1;
    }
    champion->m_remainingMobility -= CalcTerrainCost(
        theTerrain,
        direction & CURSOR_DIAGONAL_DIRECTION_BIT,
        champion->m_remainingMobility,
        champion->m_heroClass
    );
    if (champion->m_remainingMobility < CalcTerrainCost(
            CELL_TERRAIN(nextCellItem),
            0,
            champion->m_remainingMobility,
            champion->m_heroClass
        )) {
        champion->m_remainingMobility = 0;
        stopAfterMove = 1;
    }
    StopCursor(stopAfterMove);
    if (processEvent && stopAfterMove && ComboDraw(0))
        UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 0);
    inc =
        GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER)->m_tileIndex;
    if (gGroundToTerrain[inc] != m_currentTerrain && inc % MAP_CELL_TILES_PER_TERRAIN < 4) {
        m_currentTerrain = gGroundToTerrain[inc];
        PlayMusic(m_currentTerrain);
    }
    m_scrollOffsetX = m_scrollOffsetY = 0;
    cellPtr = GetCell(m_mapOriginX + m_cursorMapX, m_mapOriginY + m_cursorMapY);
    *eventX = m_mapOriginX + m_cursorMapX;
    *eventY = m_mapOriginY + m_cursorMapY;
    if ((cellPtr->m_triggerType & MAP_TRIGGER_EVENT)
        || (champion->IsEmbarked() && cellPtr->m_triggerType == MAP_OBJECT_COAST)) {
        retCell = cellPtr;
        switch (cellPtr->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
            case MAP_OBJECT_ROSEBUSH:
            case MAP_OBJECT_TREE_STUMP:
            case MAP_OBJECT_OAK_TREE:
            case MAP_OBJECT_NOTHING_HERE:
            case MAP_OBJECT_SHADOW:
            case MAP_OBJECT_RESOURCE_SHADOW:
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
    gHeroMoving = 0;
    if (posX != champion->m_x || startY != champion->m_y) {
        if (gMapExtra[champion->m_x][champion->m_y] & MAP_EXTRA_MONSTER_ADJACENT) {
            if (champion->IsEmbarked())
                goto adjacentDone;
            if (retCell && (retCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_SHIP)
                goto adjacentDone;
            CheckAdjacentMon(adjacentMonster);
            if (champion->m_owner == GAME_PLAYER_NONE)
                retCell = NULL;
        }
    }
adjacentDone:
    if (gThisNetHumanPlayer[gCurPlayer])
        SetNoDialogMenus(1);
    return retCell;
}

// Redraws through the three-argument CompleteDraw.
VA(0x00422df4, 0x161)
void advManager::CheckAdjacentMon(i8* adjacentMonster) {
    i32 monsterX;
    i32 monsterY;
    hero* curHero;
    i8 clearMonster;
    mapCell* heroCell;
    mapCell* monCell;

    curHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    clearMonster = 0;
    if (FindAdjacentMonster(
            curHero->m_x,
            curHero->m_y,
            &monsterX,
            &monsterY,
            SEARCH_INVALID_COORDINATE,
            SEARCH_INVALID_COORDINATE
        )) {
        StopCursor(1);
        CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
        UpdateScreen(0, 0);
        monCell = GetCell(monsterX, monsterY);
        heroCell = GetCell(curHero->m_x, curHero->m_y);
        if (gThisNetHumanPlayer[gCurPlayer])
            PlayerMonsterInteract(
                monCell,
                heroCell,
                curHero,
                &clearMonster,
                curHero->m_x,
                curHero->m_y,
                1,
                monsterX,
                monsterY
            );
        else
            ComputerMonsterInteract(monCell, curHero, &clearMonster);
        if (clearMonster) {
            EraseObj(monCell, monsterX, monsterY);
            if (gThisNetHumanPlayer[gCurPlayer])
                FizzleCenter(EVENT_FIZZLE_HERO_LOSS);
        }
        *adjacentMonster = 1;
    }
}

// A boat may meet another boat but not land on most objects; the rest is
// left to ValidMove.
VA(0x00422f55, 0x1a3)
i16 advManager::ValidMoveWithEvent(hero* movingHero, i16 direction) {
    i16 deltaY;
    i16 newY;
    i16 deltaX;
    i16 newX;
    mapCell* cellPtr;

    deltaX = gNormalDirTable[direction].x;
    deltaY = gNormalDirTable[direction].y;
    newX = movingHero->m_x + deltaX;
    newY = movingHero->m_y + deltaY;
    if (newX < 0 || newX > MAP_CELL_GRID_SIZE - 1 || newY < 0 || newY > MAP_CELL_GRID_SIZE - 1)
        return 0;
    cellPtr = &m_mapData[newX][newY];
    switch (cellPtr->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
        case MAP_OBJECT_BUOY:
            if (!movingHero->IsEmbarked())
                return 1;
            else
                return 0;
        case MAP_OBJECT_HERO:
            if (movingHero->IsEmbarked()) {
                if (gGame->GetHero(cellPtr->m_objectMetadata)->IsEmbarked())
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

// Indexes from the cursor's map position and tests the north/south object
// masks directly.
VA(0x004230f8, 0x24c)
i16 advManager::ValidMove(i16 direction) {
    i16 deltaX;
    i16 southMask;
    mapCell* hereCellItem;
    mapCell* destCell;
    i16 deltaY;
    i16 north;
    i16 newX;
    i16 newY;

    deltaX = gNormalDirTable[direction].x;
    deltaY = gNormalDirTable[direction].y;
    newX = m_mapOriginX + deltaX;
    newY = m_mapOriginY + deltaY;
    if (newX < -ADVMGR_VIEW_CENTER || newX > MAP_CELL_GRID_SIZE - ADVMGR_VIEW_CENTER - 1)
        return 0;
    if (newY < -ADVMGR_VIEW_CENTER || newY > MAP_CELL_GRID_SIZE - ADVMGR_VIEW_CENTER - 1)
        return 0;
    destCell = &m_mapData[newX + m_cursorMapX][newY + m_cursorMapY];
    if (destCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)
        return 0;
    if (CELL_TERRAIN(destCell) == TERRAIN_WATER) {
        if (m_cursorType != ADVMGR_HERO_ICON_BOAT
            && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
            && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK))
            return 0;
    } else {
        if (m_cursorType == ADVMGR_HERO_ICON_BOAT && destCell->m_triggerType != MAP_OBJECT_COAST
            && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_WHIRLPOOL))
            return 0;
    }
    hereCellItem = &m_mapData[m_mapOriginX + m_cursorMapX][m_mapOriginY + m_cursorMapY];
    north = (1 << direction) & MAP_DIRECTION_NORTH_MASK;
    southMask = (1 << direction) & MAP_DIRECTION_SOUTH_MASK;
    if (north && CELL_HAS_NON_SHADOW_OBJECT(hereCellItem)
        && hereCellItem->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_WHIRLPOOL))
        return 0;
    if (southMask && CELL_HAS_NON_SHADOW_OBJECT(destCell)
        && destCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_WHIRLPOOL))
        return 0;
    return 1;
}

// Indexes the map directly.
VA(0x00423344, 0x2e8)
void advManager::MoveOrigin(i16 directionX, i16 directionY) {
    i16 cellY;
    i16 cellX;
    i16 oldOriginX;
    i16 oldOriginY;

    oldOriginX = m_mapOriginX;
    oldOriginY = m_mapOriginY;
    m_mapOriginX += directionX;
    m_mapOriginY += directionY;
    directionX = oldOriginX - m_mapOriginX;
    directionY = oldOriginY - m_mapOriginY;
    if (directionX != 0 || directionY != 0) {
        m_mapData[oldOriginX + m_cursorMapX][oldOriginY + m_cursorMapY].m_flags &=
            ~MAP_CELL_HERO_CURSOR;
        m_cursorMapX += directionX;
        m_cursorMapY += directionY;
        cellX = m_mapOriginX + m_cursorMapX;
        cellY = m_mapOriginY + m_cursorMapY;
        m_mapData[cellX][cellY].m_flags |= MAP_CELL_HERO_CURSOR;
        if (m_previousCursorMapX != CURSOR_CELL_NONE) {
            m_mapData[oldOriginX + m_previousCursorMapX][oldOriginY + m_previousCursorMapY]
                .m_flags &= ~MAP_CELL_HERO_CURSOR;
            m_previousCursorMapX += directionX;
            m_previousCursorMapY += directionY;
            cellX = m_mapOriginX + m_previousCursorMapX;
            cellY = m_mapOriginY + m_previousCursorMapY;
            m_mapData[cellX][cellY].m_flags |= MAP_CELL_HERO_CURSOR;
        }
    }
    m_forceCompleteDraw = 1;
}

// Movement tables and cursor state.
DATA(0x0048fa5c)
i8 gMoveSoundMade = 1;
DATA(0x0048fa60)
i16 gPixelsPerStep[5] = {1, 4, 6, 8, 16};
DATA(0x0048fa6c)
i16 gStepDelay[5] = {30, 45, 30, 15, 15};
DATA(0x004a6ac6)
i8 gEveryOther = 0;
DATA(0x0048fa78)
i16 gStepScrollStart[3] = {16, 0, -16};
DATA(0x004a6ac2)
i16 gSavedCursorCycle;
DATA(0x004a6abc)
i16 gSavedCursorFrameCount;
DATA(0x004a6ac4)
i16 gSavedCursorTurning;
DATA(0x004a6abe)
i16 gSavedCursorBaseFrame;
DATA(0x004a6ac0)
i8 gSavedCursorDirection;
