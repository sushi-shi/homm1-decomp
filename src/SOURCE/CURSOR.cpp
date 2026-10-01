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

// CURSOR globals: Buka names; HoMM1 keeps byte flags and the last two
// footstep sample handles (0x0048eb3c/0x0048eb40).
extern signed char bMoveSoundMade;
extern signed char EveryOther;
extern struct _SAMPLE* hPrevMoveSound;
extern struct _SAMPLE* hLastMoveSound;

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

extern int bSpecialHideCursor;
extern signed char gbDrawSavedCursor;
extern signed char S1cursorDirection;
extern short S1cursorBaseFrame;
extern short S1cursorFrameCount;
extern short S1cursorCycle;
extern short S1cursorTurning;
extern signed char giGroundToTerrain[];

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
            FlipIconToBitmap(m_boatFlagIcons[gpCurPlayer->m_unknown11], gpWindowManager->m_screen,
                             drawX, screenY, drawFrame, 0);
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_updateMaxY & 3) + (m_cursorFrame & 0x7f) + 0x38;
            FlipIconToBitmap(m_flagIcons[gpCurPlayer->m_unknown11], gpWindowManager->m_screen, drawX,
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
            IconToBitmap(m_boatFlagIcons[gpCurPlayer->m_unknown11], gpWindowManager->m_screen, drawX,
                         screenY, drawFrame, 0);
        } else {
            if (m_cursorCycle == 0)
                drawFrame = (m_updateMaxY & 3) + m_cursorFrame + 0x38;
            IconToBitmap(m_flagIcons[gpCurPlayer->m_unknown11], gpWindowManager->m_screen, drawX,
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

extern short giStepDelay[];
extern short horseFrameFlip[];
extern short boatFrameFlip[];

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

extern unsigned char giCurWatchPlayerBit;
extern int gbHideComputerMoves;

// Buka CURSOR.cpp:429 GetMoveShowIt; HoMM1 reads the current hero itself
// and tests the watch bit directly in the map-extra grid.
VA(0x004064d6, 0x136)
int advManager::GetMoveShowIt(signed char direction)
{
    hero *movingHero;
    short dy;
    short dirX;

    if (gpCurPlayer->CurrentHero() == -1)
        return 0;
    movingHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    dirX = normalDirTable[direction].x;
    dy = normalDirTable[direction].y;
    if ((gbThisNetHumanPlayer[giCurPlayer] || (!gConfig.blackoutComputer && !gbHideComputerMoves))
        && ((gpGame->m_mapExtra[movingHero->m_x * MAP_CELL_GRID_SIZE + movingHero->m_y]
             & giCurWatchPlayerBit)
            || (gpGame->m_mapExtra[(movingHero->m_x + dirX) * MAP_CELL_GRID_SIZE + movingHero->m_y
                                   + dy]
                & giCurWatchPlayerBit)))
        return 1;
    else
        return 0;
}

// donor PoL RVA 0x0000e51f; preferred Buka symbol ?MoveHero@advManager@@QAEPAVmapCell@@HHPAH00H0H@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.461867;margin=0.353958;shape=0.281;size=0.831;calls=0.879;alternate=pol20:class mapCell * advManager::MoveHero(int, int, int *, int *, int *, int, int *, int)@0x0000e51f
VA(0x0040660c, 0xe1e)
class mapCell * advManager::MoveHero(int, int, int *, int *, int *, int, int *, int) { return 0; }

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
    short deltaX;
    short newX;
    short deltaY;
    short newY;
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
    if (destCell->m_unknown07 & 0x80)
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
