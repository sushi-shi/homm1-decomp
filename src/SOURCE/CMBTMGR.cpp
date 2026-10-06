#include <H1/Ints.h>

#include <BASE/audio.h>
#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/soundmgr.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/PATH.h>
#include <SOURCE/philAI.h>
#include <SOURCE/town.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

b32 gInHighMoraleBonus = false;
#define gSavedSeed gSeed
i32 gSavedSeed = 1;

combatManager::combatManager(void) {
    m_drawRightToLeft = 0;
    m_unused6f9 = -1;
    m_currentSide = COMBAT_DEFENDER_SIDE;
    m_limitCreatureHex = 0;
    m_limitCreature = false;
    m_showArmyQuantities = true;
    m_gridUpdateRow = 0;
    m_currentCommand = COMBAT_MESSAGE_COMMAND_DEFAULT;
    m_unused6e8 = 0;
    m_currentSpeed = CREATURE_SPEED_BLAZING;
    m_savedBorder = NULL;
    m_heroClass[COMBAT_DEFENDER_SIDE] = m_heroClass[COMBAT_ATTACKER_SIDE] =
        m_catapultFrame[COMBAT_DEFENDER_SIDE] = m_catapultFrame[COMBAT_ATTACKER_SIDE] =
            m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
    m_unused6d9 = m_unused6db = 0;
    m_castleSide[COMBAT_DEFENDER_SIDE] = m_castleSide[COMBAT_ATTACKER_SIDE] = 0;
    m_combatWindowOpen = false;
}

void combatManager::CombineGroups(armyGroup* sourceGroup, armyGroup* targetGroup) {
    i16 i;
    i16 j;

    if (!sourceGroup || !targetGroup)
        return;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (targetGroup->IsMember(sourceGroup->m_creatureTypes[i])) {
            targetGroup->Add(
                sourceGroup->m_creatureTypes[i],
                sourceGroup->m_creatureCounts[i],
                ARMY_GROUP_EMPTY_SLOT
            );
            sourceGroup->Dismiss(i);
        }
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (sourceGroup->m_creatureTypes[i] != CREATURE_NONE) {
            for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                if (targetGroup->m_creatureTypes[j] == CREATURE_NONE) {
                    targetGroup
                        ->Add(sourceGroup->m_creatureTypes[i], sourceGroup->m_creatureCounts[i], j);
                    sourceGroup->Dismiss(i);
                }
            }
        }
    }
}

void combatManager::SetupCombat(
    i32 mapX,
    i32 mapY,
    hero* attackerHero,
    armyGroup* attackerGroup,
    town* defenderTown,
    hero* defenderHero,
    armyGroup* defenderGroup,
    i32 combatX,
    i32 combatY,
    i32 randomSeed
) {
    i32 i;

    gSavedSeed = randomSeed;
    SRand(combatX * 100 + combatY);
    m_combatX = combatX;
    m_combatY = combatY;
    if (mapX >= 0 && mapY >= 0)
        m_battlefieldCell = gAdvManager->GetCell(mapX, mapY);
    else
        m_battlefieldCell = NULL;
    m_terrainType = CELL_TERRAIN(m_battlefieldCell);
    if (attackerHero) {
        m_playerId[COMBAT_ATTACKER_SIDE] = attackerHero->m_owner;
        attackerGroup = &attackerHero->m_army;
    } else {
        m_playerId[COMBAT_ATTACKER_SIDE] = GAME_PLAYER_NONE;
    }
    if (defenderHero) {
        m_playerId[COMBAT_DEFENDER_SIDE] = defenderHero->m_owner;
        defenderGroup = &defenderHero->m_army;
    } else if (defenderTown) {
        m_playerId[COMBAT_DEFENDER_SIDE] = defenderTown->m_owner;
        defenderGroup = &defenderTown->m_army;
    } else {
        m_playerId[COMBAT_DEFENDER_SIDE] = GAME_PLAYER_NONE;
    }
    for (i = COMBAT_SIDE_FIRST; i < COMBAT_SIDE_COUNT; i++) {
        if (m_playerId[i] >= 0)
            m_humanPlayerSide[i] = gHumanPlayer[m_playerId[i]];
        else
            m_humanPlayerSide[i] = 0;
        m_heroes[i] = i == COMBAT_ATTACKER_SIDE ? attackerHero : defenderHero;
        m_heroClass[i] = m_heroes[i] ? m_heroes[i]->m_heroClass : COMBAT_HERO_CLASS_NONE;
        m_armyGroups[i] = i == COMBAT_ATTACKER_SIDE ? attackerGroup : defenderGroup;
        m_catapultAttackCount[i] = m_catapultAttacksRemaining[i] = 1;
        if (m_heroes[i] && m_heroes[i]->HasArtifact(ARTIFACT_BALLISTA))
            m_catapultAttackCount[i] = m_catapultAttacksRemaining[i] = 2;
        m_keepAttacksRemaining[i] = 1;
        m_visitingHeroPresent[i] = false;
        m_heroCastSpell[i] = 0;
    }
    m_castleSide[COMBAT_ATTACKER_SIDE] = 0;
    if (defenderTown) {
        if (defenderTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
            m_armyGroups[COMBAT_DEFENDER_SIDE] = &m_heroes[COMBAT_DEFENDER_SIDE]->m_army;
            CombineGroups(&defenderTown->m_army, &m_heroes[COMBAT_DEFENDER_SIDE]->m_army);
            m_visitingHeroPresent[COMBAT_DEFENDER_SIDE] = true;
        } else {
            m_visitingHeroPresent[COMBAT_DEFENDER_SIDE] = false;
        }
        m_castleSide[COMBAT_DEFENDER_SIDE] =
            (defenderTown->m_buildings & (1 << BUILDING_SLOT_CASTLE)) ? 1
                                                                                              : 0;
        m_combatTowns[COMBAT_DEFENDER_SIDE] = defenderTown;
        m_originalCombatTown = defenderTown;
    } else {
        m_castleSide[COMBAT_DEFENDER_SIDE] = 0;
        m_combatTowns[COMBAT_DEFENDER_SIDE] = NULL;
    }
    m_combatTowns[COMBAT_ATTACKER_SIDE] = NULL;
}

i16 combatManager::Open(i16 priority) {
    i32 song;
    class sample* sample;
    i32 musicList[4];

    m_messageTypeMask = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                        | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                        | MESSAGE_WIDGET;
    m_combatWindowOpen = false;
    m_savedBorder = NULL;
    m_restoreMusicSuspension = MusicSuspended();
    m_restoreSampleSuspension = SamplesSuspended();
    if (m_restoreSampleSuspension)
        ResumeSamples();
    if (m_restoreMusicSuspension) {
        ResumeMusic();
        m_savedMusicTrack = GetCurrentTrack();
    } else {
        m_savedMusicTrack = MUSIC_TRACK_NONE;
    }
    StopMusic();
    m_backgroundBuffer = new bitmap(BITMAP_TYPE_NONE, LOGICAL_SCREEN_WIDTH, COMBAT_VIEW_HEIGHT);
    m_backgroundDrawn = false;
    sample = LoadPlaySample("PREBATTL.82M");
    gNextAction = ACTION_NONE;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    m_sideRetreated[COMBAT_DEFENDER_SIDE] = 0;
    m_sideRetreated[COMBAT_ATTACKER_SIDE] = 0;
    m_combatResult = COMBAT_RESULT_PENDING;
    gIconClipOn = false;
    m_computeExtent = false;
    m_redrawExtent = false;
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
    gMouseManager->SetPointer("cmbtmous.mse", COMBAT_POINTER_DEFAULT);
    m_combatWindow = new heroWindow(0, 0, "cmbtwin.bin");
    if (!m_combatWindow)
        MemError();
    gWindowManager->AddWindow(m_combatWindow, WINDOW_Z_ORDER_APPEND, 1);
    m_smallFont = gResourceManager->GetFont("smalfont.fnt");
    LoadIcons();
    LoadArmies();
    m_selectedHex = ARMY_HEX_INVALID;
    m_limitCreatureHex = ARMY_HEX_INVALID;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    GenerateMap();
    gRetreatWin = false;
    gCombatSurrender = false;
    m_sideSurrendered[COMBAT_DEFENDER_SIDE] = 0;
    m_sideSurrendered[COMBAT_ATTACKER_SIDE] = 0;
    m_limitCreature = true;
    SetDrawRightToLeft(0);
    m_gridUpdateRow = 0;
    m_combatWindowOpen = true;
    DrawFrame(true);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    m_combatPalette = gResourceManager->GetPalette("kb.pal");
    KBChangeMenu(gCombatMenu);
    CombatMessage("", true);
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, m_combatPalette);
    gLimitedCombatUpdatePalette = true;
    gMouseManager->NewUpdate(true);
    gMouseManager->WarpPointer(
        m_hexCells[m_limitCreatureHex].m_x,
        m_hexCells[m_limitCreatureHex].m_y - 50
    );
    gMouseManager->ReallyShowPointer();
    m_gridSelectionDisabled = false;
    WaitSample(sample);
    musicList[0] = MUSIC_TRACK_BATTLE_2;
    musicList[1] = MUSIC_TRACK_BATTLE_3;
    musicList[2] = MUSIC_TRACK_BATTLE_1;
    musicList[3] = MUSIC_TRACK_BATTLE_4;
    song = musicList[SRandom(0, 3)];
    PlayMusic(song);
    gInputManager->Flush();
    ResetMouse();
    m_messageMask = BASE_MANAGER_ACCEPT_WIDGET;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "combatManager");
    return BASE_MANAGER_SUCCESS;
}

void combatManager::Close(void) {
    i32 i;
    i32 monsterSide;
    i32 survivors;

    StopMusic();
    if (m_restoreSampleSuspension) {
        m_restoreSampleSuspension = false;
        SuspendSamples();
    }
    if (m_restoreMusicSuspension) {
        m_restoreMusicSuspension = false;
        if (m_savedMusicTrack >= MUSIC_TRACK_FIRST)
            PlayMusic(m_savedMusicTrack);
        SuspendMusic();
        m_savedMusicTrack = MUSIC_TRACK_NONE;
    }
    DrawCombatBorder();
    gLimitedCombatUpdatePalette = false;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    delete m_backgroundBuffer;
    for (i = COMBAT_SIDE_FIRST; i < COMBAT_SIDE_COUNT; i++)
        UpdateArmyGroup(i);
    if (m_battlefieldCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER)) {
        monsterSide = m_playerId[COMBAT_DEFENDER_SIDE] != GAME_PLAYER_NONE
                          ? static_cast<i8>(COMBAT_ATTACKER_SIDE)
                          : static_cast<i8>(COMBAT_DEFENDER_SIDE);
        survivors = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_armyGroups[monsterSide]->m_creatureTypes[i] != CREATURE_NONE)
                survivors += m_armyGroups[monsterSide]->m_creatureCounts[i];
        }
        if (survivors > COMBAT_MAP_MONSTER_COUNT_MAX)
            survivors = COMBAT_MAP_MONSTER_COUNT_MAX;
        m_battlefieldCell->m_objectMetadata = survivors;
    }
    gWindowManager->RemoveWindow(m_combatWindow);
    FreeArmies();
    FreeIcons();
    gResourceManager->Dispose(m_smallFont);
    gResourceManager->Dispose(m_combatPalette);
    delete m_combatWindow;
    if (m_savedBorder)
        free(m_savedBorder);
    m_active = 0;
    m_combatWindowOpen = false;
}

void combatManager::UpdateArmyGroup(i8 side) {
    i16 i;
    i16 j;

    // Combat stacks were created from the occupied slots in slot order, so
    // walk both together; two stacks of one creature keep their own slots.
    i = 0;
    for (j = 0; j < ARMY_GROUP_SLOT_COUNT && i < m_numArmies[side]; j++) {
        if (m_armyGroups[side]->m_creatureTypes[j] == CREATURE_NONE)
            continue;
        if (m_armies[side][i].m_stats.attributes & MONSTER_FLAGS_DEAD) {
            m_armyGroups[side]->m_creatureTypes[j] = CREATURE_NONE;
            m_armyGroups[side]->m_creatureCounts[j] = 0;
        } else {
            m_armyGroups[side]->m_creatureCounts[j] = m_armies[side][i].m_quantity;
        }
        i++;
    }
}

void combatManager::GenerateMap(void) {
    i16 x;
    i16 i;
    i16 y;
    i16 count;
    i16 armyCount;

    m_catapultFrame[COMBAT_ATTACKER_SIDE] = m_castleSide[COMBAT_DEFENDER_SIDE] == 1
                                                ? COMBAT_CATAPULT_FRAME_FIRST
                                                : COMBAT_CATAPULT_FRAME_NONE;
    m_catapultFrame[COMBAT_DEFENDER_SIDE] = m_castleSide[COMBAT_ATTACKER_SIDE] == 1
                                                ? COMBAT_CATAPULT_FRAME_FIRST
                                                : COMBAT_CATAPULT_FRAME_NONE;
    for (y = 0; y < COMBAT_GRID_ROWS; y++) {
        for (x = 0; x < COMBAT_GRID_COLUMNS; x++) {
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_y =
                y * COMBAT_HEX_HEIGHT + COMBAT_HEX_ORIGIN_Y;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_x =
                x * COMBAT_HEX_WIDTH + static_cast<i16>((y & 1) ? 27 : -12);
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundIcon = COMBAT_ICON_GROUND;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame =
                static_cast<i8>(SRandom(0, 3)) + 4;
            if (x == 0) {
                if (m_castleSide[COMBAT_ATTACKER_SIDE] == 1)
                    m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundIcon = COMBAT_ICON_CASTLE;
                if (y & 1)
                    m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame = 0;
                else
                    m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame = 1;
            } else if (x == COMBAT_GRID_LAST_COLUMN) {
                if (m_castleSide[COMBAT_DEFENDER_SIDE] == 1)
                    m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundIcon = COMBAT_ICON_CASTLE;
                if (y & 1)
                    m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame = 3;
                else
                    m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame = 2;
            }
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_occupantSide = COMBAT_SIDE_NONE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_occupantIndex = COMBAT_ARMY_INDEX_NONE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_occupantFootprintHalf = ARMY_FACING_NONE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex = COMBAT_OBSTACLE_NONE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_pathFlag = false;
        }
    }
    count = SRandom(8, 15);
    for (i = 0; i < count; i++) {
        m_hexCells[SRandom(0, 4) * COMBAT_GRID_COLUMNS + SRandom(1, 7)].m_groundFrame =
            static_cast<i8>(SRandom(0, 2)) + 8;
    }
    if (m_castleSide[COMBAT_DEFENDER_SIDE]) {
        for (x = 6; x < 8; x++) {
            for (y = 0; y < COMBAT_GRID_ROWS; y++) {
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundIcon = COMBAT_ICON_CASTLE;
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame = 4;
            }
        }
        for (y = 0; y < COMBAT_GRID_ROWS; y++) {
            m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_obstacleIcon =
                COMBAT_ICON_CASTLE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_obstacleIndex =
                COMBAT_WALL_INTACT;
        }
    }
    armyCount = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armyGroups[COMBAT_ATTACKER_SIDE]->m_creatureTypes[i] != CREATURE_NONE) {
            m_armies[COMBAT_ATTACKER_SIDE][armyCount].m_hex =
                i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN;
            m_armies[COMBAT_ATTACKER_SIDE][armyCount].m_stats.attributes &=
                MONSTER_FLAGS_BATTLE_START_MASK;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN].m_occupantSide =
                COMBAT_ATTACKER_SIDE;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN].m_occupantIndex =
                armyCount;
            if (m_armies[COMBAT_ATTACKER_SIDE][armyCount].m_stats.attributes & MONSTER_FLAGS_WIDE) {
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1]
                    .m_occupantSide = COMBAT_ATTACKER_SIDE;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1]
                    .m_occupantIndex = armyCount;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN]
                    .m_occupantFootprintHalf = ARMY_FACING_LEFT;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1]
                    .m_occupantFootprintHalf = ARMY_FACING_RIGHT;
            }
            armyCount++;
        }
    }
    armyCount = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armyGroups[COMBAT_DEFENDER_SIDE]->m_creatureTypes[i] != CREATURE_NONE) {
            m_armies[COMBAT_DEFENDER_SIDE][armyCount].m_hex =
                i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN;
            m_armies[COMBAT_DEFENDER_SIDE][armyCount].m_stats.attributes &=
                MONSTER_FLAGS_BATTLE_START_MASK;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN].m_occupantSide =
                COMBAT_DEFENDER_SIDE;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN].m_occupantIndex =
                armyCount;
            if (m_armies[COMBAT_DEFENDER_SIDE][armyCount].m_stats.attributes & MONSTER_FLAGS_WIDE) {
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN - 1]
                    .m_occupantSide = COMBAT_DEFENDER_SIDE;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN - 1]
                    .m_occupantIndex = armyCount;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN - 1]
                    .m_occupantFootprintHalf = ARMY_FACING_LEFT;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN]
                    .m_occupantFootprintHalf = ARMY_FACING_RIGHT;
            }
            armyCount++;
        }
    }
    count = 0;
    if (!m_castleSide[COMBAT_ATTACKER_SIDE] && !m_castleSide[COMBAT_DEFENDER_SIDE]) {
        count = SRandom(0, 3);
        for (i = 0; i < count; i++) {
            x = SRandom(3, 5);
            y = SRandom(0, 4);
            while (m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_occupantSide != COMBAT_SIDE_NONE) {
                x = SRandom(3, 5);
                y = SRandom(0, 4);
            }
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIcon = COMBAT_ICON_OBSTACLES;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex =
                (SRandom(0, 2));
            if ((m_terrainType == TERRAIN_WATER || m_terrainType == TERRAIN_LAVA)
                && m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex
                       == COMBAT_OBSTACLE_LAND_ONLY_FRAME)
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex =
                    COMBAT_OBSTACLE_FIRST_FRAME;
        }
    }
    m_currentSide = COMBAT_DEFENDER_SIDE;
    m_currentSpeed = CREATURE_SPEED_BLAZING;
    GetNextArmy(false);
    m_gridUpdateRow = 0;
    SRand(gSavedSeed);
}

char* combatManager::GetBackgroundName(void) {
    static char* gCombatBkgNames[COMBAT_BACKGROUND_COUNT] = {
        "frstwgrs.bkg",
        "mtnwgrsf.bkg",
        "snowfrst.bkg",
        "snowmtnf.bkg",
        "swamp.bkg",
        "lava.bkg",
        "desert.bkg",
        "frstwdrt.bkg",
        "mtnwdrtf.bkg",
        "boat.bkg",
        "gravyard.bkg",
    };
    if (MAP_TRIGGER_OBJECT(m_battlefieldCell->m_triggerType) == MAP_OBJECT_GRAVEYARD
        || (MAP_TRIGGER_OBJECT(m_battlefieldCell->m_triggerType) == MAP_OBJECT_HERO
            && MAP_TRIGGER_OBJECT(
                   gGame->GetHero(m_battlefieldCell->m_objectMetadata)->m_locationType
               ) == MAP_OBJECT_GRAVEYARD)) {
        m_terrainType = TERRAIN_DIRT;
        return gCombatBkgNames[COMBAT_BACKGROUND_GRAVEYARD];
    }
    switch (m_terrainType) {
        case TERRAIN_WATER:
            return gCombatBkgNames[COMBAT_BACKGROUND_BOAT];
        case TERRAIN_SWAMP:
            return gCombatBkgNames[COMBAT_BACKGROUND_SWAMP];
        case TERRAIN_LAVA:
            return gCombatBkgNames[COMBAT_BACKGROUND_LAVA];
        case TERRAIN_DESERT:
            return gCombatBkgNames[COMBAT_BACKGROUND_DESERT];
        case TERRAIN_GRASS:
            if (MoreTreesNear())
                return gCombatBkgNames[COMBAT_BACKGROUND_GRASS_FOREST];
            else
                return gCombatBkgNames[COMBAT_BACKGROUND_GRASS_MOUNTAIN];
        case TERRAIN_SNOW:
            if (MoreTreesNear())
                return gCombatBkgNames[COMBAT_BACKGROUND_SNOW_FOREST];
            else
                return gCombatBkgNames[COMBAT_BACKGROUND_SNOW_MOUNTAIN];
        case TERRAIN_DIRT:
            if (MoreTreesNear())
                return gCombatBkgNames[COMBAT_BACKGROUND_DIRT_FOREST];
            else
                return gCombatBkgNames[COMBAT_BACKGROUND_DIRT_MOUNTAIN];
    }
    return gCombatBkgNames[COMBAT_BACKGROUND_GRASS_FOREST];
}

i8 combatManager::MoreTreesNear(void) {
    i32 yPos;
    i32 xPos;
    i16 originY;
    i16 treeCount;
    u8 nearbyTileset;
    i16 mountainCount;
    i16 originX;
    mapCell* cell;
    i16 radius;
    i8 nearbyTypeGrid[3][MAP_DIRECTION_COUNT];
    i16 dir;

    memset(nearbyTypeGrid, -1, sizeof(nearbyTypeGrid));
    originX = m_combatX;
    originY = m_combatY;
    for (radius = 0; radius < 3; radius++) {
        for (dir = MAP_DIRECTION_FIRST; dir < MAP_DIRECTION_COUNT; dir++) {
            xPos = originX + gNormalDirTable[dir].x * radius;
            yPos = originY + gNormalDirTable[dir].y * radius;
            if (MAP_CELL_IN_BOUNDS(xPos, yPos)) {
                cell = gAdvManager->GetCell(xPos, yPos);
                nearbyTileset =
                    (cell->m_objectTileset & MAP_CELL_TILESET_MASK);
                if (nearbyTileset == TILESET_MTN32)
                    nearbyTypeGrid[radius][dir] = COMBAT_NEARBY_MOUNTAIN;
                else if (nearbyTileset == TILESET_TREE32)
                    nearbyTypeGrid[radius][dir] = COMBAT_NEARBY_TREE;
            }
        }
    }
    treeCount = 0;
    mountainCount = 0;
    for (radius = 0; radius < 3; radius++) {
        for (dir = MAP_DIRECTION_FIRST; dir < MAP_DIRECTION_COUNT; dir++) {
            if (nearbyTypeGrid[radius][dir] == COMBAT_NEARBY_MOUNTAIN)
                mountainCount++;
            if (nearbyTypeGrid[radius][dir] == COMBAT_NEARBY_TREE)
                treeCount++;
        }
    }
    if (treeCount > mountainCount)
        return 1;
    return 0;
}

// The battle for the Dragon City that ends the campaign.
i32 combatManager::IsCampaignFinalBattle(void) {
    return gGame->m_campaignType > 0 && gGame->m_campaignScenario == COMBAT_FINAL_CAMPAIGN_SCENARIO
           && m_heroes[COMBAT_ATTACKER_SIDE]
           && m_heroes[COMBAT_ATTACKER_SIDE]->m_x == COMBAT_FINAL_DRAGON_CITY_X
           && m_heroes[COMBAT_ATTACKER_SIDE]->m_y == COMBAT_FINAL_DRAGON_CITY_Y;
}

void combatManager::LoadIcons(void) {
    i32 i;

    for (i = COMBAT_ICON_GROUND; i < COMBAT_ICON_COUNT; i++)
        m_combatIcons[i] = NULL;
    m_combatIcons[COMBAT_ICON_SPELLS] = gResourceManager->GetIcon("spells.icn");
    m_backgroundBitmap = gResourceManager->GetBitmap(GetBackgroundName());
    if (IsCampaignFinalBattle())
        m_combatIcons[COMBAT_ICON_GROUND] = gResourceManager->GetIcon("dirt.xtl");
    else
        m_combatIcons[COMBAT_ICON_GROUND] =
            gResourceManager->GetIcon(gCombatGroundNames[m_terrainType]);
    m_combatIcons[COMBAT_ICON_OBSTACLES] =
        gResourceManager->GetIcon(gCombatObstacleNames[m_terrainType]);
    m_combatIcons[COMBAT_ICON_TEXTBAR] = gResourceManager->GetIcon("textbar.icn");
    m_combatIcons[COMBAT_ICON_TENT] = gResourceManager->GetIcon("tent.icn");
    m_combatIcons[COMBAT_ICON_CLOUD] = gResourceManager->GetIcon("cloud.icn");
    if (m_castleSide[COMBAT_ATTACKER_SIDE] || m_castleSide[COMBAT_DEFENDER_SIDE]) {
        m_combatIcons[COMBAT_ICON_CATAPULT] = gResourceManager->GetIcon("catapult.icn");
        sprintf(
            gText,
            "castle%02d.icn",
            (m_combatTowns [m_castleSide[COMBAT_ATTACKER_SIDE] == 1 ? static_cast<i8>(COMBAT_ATTACKER_SIDE) : static_cast<i8>(COMBAT_DEFENDER_SIDE)] ->m_type)
        );
        m_combatIcons[COMBAT_ICON_CASTLE] = gResourceManager->GetIcon(gText);
        sprintf(
            gText,
            "keep%02d.icn",
            m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type
        );
        m_combatIcons[COMBAT_ICON_KEEP] = gResourceManager->GetIcon(gText);
    }
}

void combatManager::FreeIcons(void) {
    i16 i;

    for (i = COMBAT_ICON_GROUND; i < COMBAT_ICON_COUNT; i++) {
        if (m_combatIcons[i])
            gResourceManager->Dispose(m_combatIcons[i]);
    }
    gResourceManager->Dispose(m_backgroundBitmap);
}

void combatManager::LoadArmies(void) {
    i16 j;
    i16 i;

    m_numArmies[COMBAT_ATTACKER_SIDE] = m_numArmies[COMBAT_DEFENDER_SIDE] = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        for (j = COMBAT_SIDE_FIRST; j < COMBAT_SIDE_COUNT; j++) {
            m_armies[j][i].m_quantity = 0;
            m_armies[j][i].m_creatureType = CREATURE_NONE;
        }
    }
    for (j = COMBAT_SIDE_FIRST; j < COMBAT_SIDE_COUNT; j++) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            m_armies[j][i].InitClean();
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armyGroups[COMBAT_ATTACKER_SIDE]->m_creatureTypes[i] != CREATURE_NONE) {
            m_armies[COMBAT_ATTACKER_SIDE][m_numArmies[COMBAT_ATTACKER_SIDE]].Init(
                m_armyGroups[COMBAT_ATTACKER_SIDE]->m_creatureTypes[i],
                m_armyGroups[COMBAT_ATTACKER_SIDE]->m_creatureCounts[i],
                COMBAT_ATTACKER_SIDE,
                m_numArmies[COMBAT_ATTACKER_SIDE]
            );
            m_armies[COMBAT_ATTACKER_SIDE][m_numArmies[COMBAT_ATTACKER_SIDE]].LoadResources();
            m_numArmies[COMBAT_ATTACKER_SIDE]++;
        }
        if (m_armyGroups[COMBAT_DEFENDER_SIDE]->m_creatureTypes[i] != CREATURE_NONE) {
            m_armies[COMBAT_DEFENDER_SIDE][m_numArmies[COMBAT_DEFENDER_SIDE]].Init(
                m_armyGroups[COMBAT_DEFENDER_SIDE]->m_creatureTypes[i],
                m_armyGroups[COMBAT_DEFENDER_SIDE]->m_creatureCounts[i],
                COMBAT_DEFENDER_SIDE,
                m_numArmies[COMBAT_DEFENDER_SIDE]
            );
            m_armies[COMBAT_DEFENDER_SIDE][m_numArmies[COMBAT_DEFENDER_SIDE]].LoadResources();
            m_numArmies[COMBAT_DEFENDER_SIDE]++;
        }
    }
}

void combatManager::FreeArmies(void) {
    i16 i;

    StopAllAudio();
    for (i = 0; i < m_numArmies[COMBAT_ATTACKER_SIDE]; i++)
        m_armies[COMBAT_ATTACKER_SIDE][i].FreeResources();
    for (i = 0; i < m_numArmies[COMBAT_DEFENDER_SIDE]; i++)
        m_armies[COMBAT_DEFENDER_SIDE][i].FreeResources();
    if (gCurLoadedSpellIcon)
        gResourceManager->Dispose(gCurLoadedSpellIcon);
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
}

void combatManager::NoShowCombatLog(char* message) {}

i16 combatManager::GetGridIndex(i16 x, i16 y) {
    y -= COMBAT_FIELD_TOP;
    y /= COMBAT_HEX_HEIGHT;
    if (y & 1) {
        if (x < 66) {
            x = -1;
        } else {
            x -= 66;
            x /= COMBAT_HEX_WIDTH;
        }
    } else {
        x -= 27;
        x /= COMBAT_HEX_WIDTH;
    }
    x++;
    if (y == COMBAT_GRID_ROWS)
        return ARMY_HEX_INVALID;
    else
        return y * COMBAT_GRID_COLUMNS + x;
}

void combatManager::CheckApplyGoodMorale(i32 side, i32 index) {
    armyGroup* group;
    army* currentArmy;
    class sample* moraleSound;
    i32 moraleLevel;

    if (side < COMBAT_SIDE_FIRST || index < 0)
        return;
    if (gInHighMoraleBonus) {
        gInHighMoraleBonus = false;
        return;
    }
    gInHighMoraleBonus = false;
    group = m_armyGroups[side];
    currentArmy = &m_armies[side][index];
    if (!currentArmy->m_quantity)
        return;
    moraleLevel = group->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (moraleLevel <= 0 || SRandom(1, 24) > moraleLevel)
        return;
    gInHighMoraleBonus = true;
    sprintf(gText, "goodmrle.82M");
    moraleSound = LoadPlaySample(gText);
    if (currentArmy->m_quantity <= 1)
        sprintf(
            gText,
            localization::Tr("te.combat.morale.good_single"),
            gArmyNames[currentArmy->m_creatureType]
        );
    else
        sprintf(
            gText,
            localization::Tr("combat.morale.good"),
            gArmyNamesPlural[currentArmy->m_creatureType]
        );
    CombatMessage(gText, true);
    currentArmy->SpellEffect(COMBAT_EFFECT_GOOD_MORALE, 180);
    currentArmy->Stand(true);
    if (currentArmy->m_stats.attributes & MONSTER_FLAGS_TURN_SPENT)
        currentArmy->m_stats.attributes -= MONSTER_FLAGS_TURN_SPENT;
    currentArmy->m_stats.attributes |= MONSTER_FLAGS_HIGH_MORALE;
    WaitSample(moraleSound);
}

i32 combatManager::CheckApplyBadMorale(i32 side, i32 index) {
    armyGroup* group;
    army* currentArmy;
    class sample* moraleSound;
    i32 moraleLevel;

    if (side < COMBAT_SIDE_FIRST || index < 0)
        return 0;
    group = m_armyGroups[side];
    currentArmy = &m_armies[side][index];
    moraleLevel = group->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (moraleLevel >= 0 || SRandom(1, 12) > -moraleLevel)
        return 0;
    if (!m_humanPlayerSide[side] && SRandom(1, 4) == 1)
        return 0;
    moraleSound = LoadPlaySample("BADMRLE.82M");
    if (currentArmy->m_quantity <= 1)
        sprintf(
            gText,
            localization::Tr("te.combat.morale.bad_single"),
            gArmyNames[currentArmy->m_creatureType]
        );
    else
        sprintf(
            gText,
            localization::Tr("combat.morale.bad"),
            gArmyNamesPlural[currentArmy->m_creatureType]
        );
    CombatMessage(gText, true);
    currentArmy->m_animationFrame = 2;
    currentArmy->SpellEffect(COMBAT_EFFECT_BAD_MORALE, 180);
    currentArmy->Stand(true);
    currentArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
    WaitSample(moraleSound);
    return 1;
}

i8 combatManager::GetNextArmy(b32 checkMorale) {
    army* checkArmy;
    i8 speedLevelIndex;
    i32 sideIter;
    i16 unused;
    i8 armyCounter;
    i8 stackSide;
    b32 skip;

    stackSide = m_currentSide;
    for (speedLevelIndex = 0; speedLevelIndex < COMBAT_SPEED_PASS_COUNT; speedLevelIndex++) {
        for (sideIter = COMBAT_SIDE_FIRST; sideIter < COMBAT_SIDE_COUNT; sideIter++) {
            COMBAT_SWITCH_SIDE(stackSide);
            for (armyCounter = 0; armyCounter < m_numArmies[stackSide]; armyCounter++) {
                skip = false;
                checkArmy = &m_armies[stackSide][armyCounter];
                if ((checkArmy->m_stats.attributes
                     & (MONSTER_FLAGS_DEAD | MONSTER_FLAGS_TURN_SPENT))
                    || checkArmy->m_spellEffect == SPELL_PARALYZE
                    || checkArmy->m_spellEffect == SPELL_BLIND
                    || (checkArmy->m_stats.speed != m_currentSpeed
                        && !(checkArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE)))
                    skip = true;
                if (!skip && !speedLevelIndex
                    && !(checkArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE))
                    skip = true;
                if (!skip && checkMorale && CheckApplyBadMorale(stackSide, armyCounter))
                    skip = true;
                if (!skip)
                    break;
            }
            if (armyCounter != m_numArmies[stackSide]) {
                m_currentSide = stackSide;
                m_currentArmyIndex = armyCounter;
                GetControl();
                return 1;
            }
        }
        if (speedLevelIndex) {
            m_currentSpeed--;
            if (!m_currentSpeed)
                m_currentSpeed = CREATURE_SPEED_BLAZING;
        }
    }
    GetControl();
    return 0;
}

i8 combatManager::IsWinner(i8 side) {
    b8 isWinner;
    i16 i;

    if (m_sideSurrendered[COMBAT_OPPOSING_SIDE(side)])
        return 1;
    if (m_sideRetreated[COMBAT_OPPOSING_SIDE(side)])
        return 1;
    COMBAT_SWITCH_SIDE(side);
    isWinner = true;
    for (i = 0; i < m_numArmies[side]; i++) {
        if (!(m_armies[side][i].m_stats.attributes & MONSTER_FLAGS_DEAD))
            isWinner = false;
    }
    return isWinner;
}

// The catapult animation repaints the whole screen each frame; partial
// extents left fragments of the boulder and the wall behind.
static void SetFullScreenExtent(void) {
    gMinExtentX = 0;
    gMinExtentY = 0;
    gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
    gMaxExtentY = LOGICAL_SCREEN_HEIGHT - 1;
}

void combatManager::CatAttack(i8 side) {
    i16 yPos;
    i16 topPosX;
    icon* boulderIcon;
    i16 xPos;
    i16 i;
    i8 castleColumn;
    i16 frameIndex;
    i16 endY;
    i16 collapseChance;
    i16 yDelta;
    i16 endX;
    i16 xDelta;
    i16 startX;
    class sample* catapultSound;
    b8 anyStanding;
    i16 startY;
    i16 topPosY;

    if (!m_castleSide[COMBAT_DEFENDER_SIDE])
        return;
    catapultSound = NULL;
    castleColumn = side == COMBAT_ATTACKER_SIDE
                       ? COMBAT_CASTLE_WALL_COLUMN
                       : COMBAT_GRID_LAST_COLUMN - COMBAT_CASTLE_WALL_COLUMN;
    anyStanding = false;
    for (i = 0; i < COMBAT_GRID_ROWS; i++) {
        if (m_hexCells[i * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex
            != COMBAT_OBSTACLE_NONE)
            anyStanding = true;
    }
    if (!anyStanding)
        return;
    gMouseManager->ReallyHidePointer();
    boulderIcon = gResourceManager->GetIcon("boulder.icn");
    sprintf(gText, "catsnd%02d.82M", 0);
    catapultSound = LoadPlaySample(gText);
    SetFullScreenExtent();
    m_catapultFrame[side] = COMBAT_CATAPULT_FRAME_FIRST;
    while (m_catapultFrame[side] < 8) {
        m_redrawExtent = true;
        DrawFrame(true);
        m_catapultFrame[side]++;
    }
    if ((m_hexCells[castleColumn + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_WALL_DAMAGED
         || m_hexCells[castleColumn + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
        && (m_hexCells[castleColumn + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
                == COMBAT_WALL_DAMAGED
            || m_hexCells[castleColumn + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
                   == COMBAT_OBSTACLE_NONE)) {
        m_catapultTargetRow = SRandom(0, 4);
        while (m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex
               == COMBAT_OBSTACLE_NONE)
            m_catapultTargetRow = SRandom(0, 4);
    } else if (m_hexCells[castleColumn + COMBAT_GRID_COLUMNS].m_obstacleIndex
               == COMBAT_OBSTACLE_NONE) {
        m_catapultTargetRow = COMBAT_LOWER_WALL_ROW;
    } else if (m_hexCells[castleColumn + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
               == COMBAT_OBSTACLE_NONE) {
        m_catapultTargetRow = COMBAT_UPPER_WALL_ROW;
    } else if (m_hexCells[castleColumn + COMBAT_GRID_COLUMNS].m_obstacleIndex
               != COMBAT_WALL_INTACT) {
        m_catapultTargetRow = COMBAT_LOWER_WALL_ROW;
    } else if (m_hexCells[castleColumn + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
               != COMBAT_WALL_INTACT) {
        m_catapultTargetRow = COMBAT_UPPER_WALL_ROW;
    } else {
        m_catapultTargetRow = SRandom(0, 1);
        m_catapultTargetRow = m_catapultTargetRow ? COMBAT_LOWER_WALL_ROW : COMBAT_UPPER_WALL_ROW;
    }
    startX = 0x75;
    startY = 0x104;
    endX = m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_x;
    endY = m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_y
           - COMBAT_HEX_HEIGHT;
    frameIndex = 0;
    xPos = startX;
    yPos = startY;
    if (!m_catapultTargetRow) {
        xDelta = (endX - startX) / 12;
        yDelta = (endY - startY) / 12;
        i = 0;
        while (i < 12) {
            m_redrawExtent = true;
            SetFullScreenExtent();
            DrawFrame(false);
            boulderIcon
                ->DrawToBuffer(xPos, yPos, frameIndex, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            UPDATE_INCLUSIVE_REGION(gMinExtentX, gMinExtentY, gMaxExtentX, gMaxExtentY);
            xPos += xDelta;
            yPos += yDelta;
            frameIndex++;
            frameIndex %= 3;
            if (i < 2)
                m_catapultFrame[side]++;
            i++;
        }
    } else {
        topPosX = (startX + endX) / 2;
        switch (m_catapultTargetRow) {
            case COMBAT_UPPER_WALL_ROW:
                topPosY = 25;
                break;
            default:
                topPosY = m_catapultTargetRow * 20 + 25;
                break;
        }
        xDelta = (topPosX - startX) / 12;
        yDelta = (topPosY - startY) / 78;
        for (i = 0; i < 12; i++) {
            m_redrawExtent = true;
            SetFullScreenExtent();
            DrawFrame(false);
            boulderIcon
                ->DrawToBuffer(xPos, yPos, frameIndex, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            UPDATE_INCLUSIVE_REGION(gMinExtentX, gMinExtentY, gMaxExtentX, gMaxExtentY);
            xPos += xDelta;
            yPos += (12 - i) * yDelta;
            frameIndex++;
            frameIndex %= 3;
            if (i < 2)
                m_catapultFrame[side]++;
        }
        xDelta = (endX - xPos) / 8;
        yDelta = (endY - yPos) / 36;
        for (i = 1; i <= 8; i++) {
            m_redrawExtent = true;
            SetFullScreenExtent();
            DrawFrame(false);
            boulderIcon
                ->DrawToBuffer(xPos, yPos, frameIndex, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            UPDATE_INCLUSIVE_REGION(gMinExtentX, gMinExtentY, gMaxExtentX, gMaxExtentY);
            xPos += xDelta;
            yPos += i * yDelta;
            frameIndex++;
            frameIndex %= 3;
        }
    }
    WaitSample(catapultSound);
    sprintf(gText, "catsnd%02d.82M", 2);
    catapultSound = LoadPlaySample(gText);
    if (m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex
        == COMBAT_WALL_DAMAGED)
        m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex =
            COMBAT_WALL_DAMAGED_HIT;
    else
        m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex =
            COMBAT_WALL_INTACT_HIT;
    collapseChance = SRandom(0, 150);
    if (!gHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        collapseChance -= 15;
    if (collapseChance < 30
        || m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex
               == COMBAT_WALL_DAMAGED_HIT) {
        m_wallSurvives = false;
        m_wallFrame = 0;
        SetFullScreenExtent();
        while (m_wallFrame < 10) {
            m_wallDamage = m_wallFrame;
            if (m_wallFrame == COMBAT_WALL_COLLAPSE_FRAME)
                m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn]
                    .m_obstacleIndex = COMBAT_WALL_COLLAPSING;
            m_redrawExtent = true;
            m_gridUpdateRow = m_catapultTargetRow - 2;
            if (m_gridUpdateRow < 0)
                m_gridUpdateRow = 0;
            DrawFrame(true);
            m_wallFrame++;
        }
        m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
        m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex =
            COMBAT_OBSTACLE_NONE;
    } else {
        m_wallSurvives = true;
        m_wallFrame = 0;
        SetFullScreenExtent();
        while (m_wallFrame < 10) {
            if (m_wallFrame == COMBAT_WALL_COLLAPSE_FRAME)
                m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn]
                    .m_obstacleIndex = COMBAT_WALL_DAMAGED_HIT;
            m_redrawExtent = true;
            DrawFrame(true);
            m_wallFrame++;
        }
        m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex =
            COMBAT_WALL_DAMAGED;
        m_wallFrame = COMBAT_WALL_FRAME_NONE;
    }
    m_redrawExtent = true;
    DrawFrame(true);
    SetFullScreenExtent();
    while (m_catapultFrame[side] < 14) {
        m_redrawExtent = true;
        DrawFrame(true);
        m_catapultFrame[side]++;
    }
    m_catapultFrame[side] = COMBAT_CATAPULT_FRAME_FIRST;
    m_redrawExtent = true;
    DrawFrame(true);
    gResourceManager->Dispose(boulderIcon);
    gMouseManager->ReallyShowPointer();
    WaitSample(catapultSound);
    // Repaint the whole field so no boulder or catapult frame is left behind.
    DrawFrame(1);
}

void combatManager::RegenerateField(void) {
    FreeArmies();
    LoadArmies();
    GenerateMap();
    SetDrawRightToLeft(0);
    m_gridUpdateRow = 0;
    DrawFrame(true);
}

void combatManager::KeepAttack(void) {
    i8 arrowFrame;
    i16 lastX;
    i32 numRolls;
    class sample* sample;
    i16 lastY;
    i8 originalColumn;
    i8 hisCol;
    i8 originalRow;
    float gainX;
    float inFlightX;
    float gainY;
    float inFlightY;
    i16 landX;
    i8 shotTable[COMBAT_HEX_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                                      1, 2, 0, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 1,
                                      1, 1, 1, 2, 2, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0};
    i8 hisRow;
    i16 projectileSizeY;
    i32 mod;
    i16 landY;
    i16 projectileSizeX;
    i32 arrowDamage;
    i16 clipTop;
    i32 stackKilled;
    i32 k;
    i32 priority;
    i16 clipLeft;
    bitmap* backing;
    i32 pickIndex;
    i16 aimColumn;
    army* hisStack;
    i16 fullXLen;
    i16 flightSteps;
    i16 startX;
    i16 maxX;
    i16 fullYLen;
    i16 startY;
    i16 maxY;
    i32 bestClass;
    i32 power;
    i32 bestStrength;

    bestClass = -1;
    bestStrength = 0;
    pickIndex = COMBAT_ARMY_INDEX_NONE;
    for (k = 0; k < ARMY_GROUP_SLOT_COUNT; k++) {
        if (m_armies[COMBAT_ATTACKER_SIDE][k].IsAlive()) {
            hisStack = &m_armies[COMBAT_ATTACKER_SIDE][k];
            if (hisStack->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                priority = 2;
            else if (hisStack->m_stats.attributes & MONSTER_FLAGS_FLYING)
                priority = 1;
            else
                priority = 0;
            power = hisStack->m_quantity * gMonsterDatabase[hisStack->m_creatureType].fightValue;
            if (priority > bestClass || (priority == bestClass && power > bestStrength)) {
                bestStrength = power;
                bestClass = priority;
                pickIndex = k;
            }
        }
    }
    if (pickIndex == COMBAT_ARMY_INDEX_NONE)
        return;
    gMouseManager->ReallyHidePointer();
    hisStack = &gCombatManager->m_armies[COMBAT_ATTACKER_SIDE][pickIndex];
    hisCol = hisStack->m_hex % COMBAT_GRID_COLUMNS;
    hisRow = hisStack->m_hex / COMBAT_GRID_COLUMNS;
    originalColumn = COMBAT_GRID_LAST_COLUMN;
    originalRow = 0;
    gCombatManager->SetDrawRightToLeft(0);
    if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type == TOWN_TYPE_WARLOCK
        || m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type == TOWN_TYPE_SORCERESS)
        sprintf(gText, "shoot15.82M");
    else
        sprintf(gText, "shoot01.82M");
    sample = LoadPlaySample(gText);
    aimColumn = hisCol;
    if (hisStack->m_stats.attributes & MONSTER_FLAGS_WIDE)
        aimColumn += hisStack->m_facing == ARMY_FACING_LEFT ? -1 : 1;
    fullXLen = abs(aimColumn - originalColumn);
    fullYLen = abs(hisRow - originalRow);
    flightSteps = __max(fullXLen, fullYLen);
    arrowFrame = shotTable[hisStack->m_hex];
    startX = 0x24d;
    startY = 0x19;
    landX = gCombatManager->m_hexCells[hisRow * COMBAT_GRID_COLUMNS + aimColumn].m_x;
    landY = gCombatManager->m_hexCells[hisRow * COMBAT_GRID_COLUMNS + aimColumn].m_y - 75;
    gainX = static_cast<float>(landX - startX) / static_cast<float>(flightSteps * 3);
    gainY = static_cast<float>(landY - startY) / static_cast<float>(flightSteps * 3);
    inFlightX = startX;
    inFlightY = startY;
    maxX = 0;
    clipLeft = LOGICAL_SCREEN_WIDTH - 1;
    maxY = 0;
    clipTop = LOGICAL_SCREEN_HEIGHT - 1;
    if (arrowFrame == 0) {
        projectileSizeX = 0x43;
        projectileSizeY = 0x12;
    } else if (arrowFrame == 1) {
        projectileSizeX = 0x37;
        projectileSizeY = 0x2b;
    } else {
        projectileSizeX = 0x12;
        projectileSizeY = 0x43;
    }
    backing = new bitmap(BITMAP_TYPE_MEMORY, projectileSizeX, projectileSizeY);
    backing->GrabBitmap(gWindowManager->m_screen, inFlightX, inFlightY);
    lastX = inFlightX;
    lastY = inFlightY;
    for (k = 0; k < flightSteps * 3; k++) {
        clipLeft = inFlightX;
        clipTop = lastY;
        maxX = lastX + projectileSizeX;
        maxY = projectileSizeY + inFlightY;
        backing->DrawToBuffer(lastX, lastY);
        backing->GrabBitmap(gWindowManager->m_screen, inFlightX, inFlightY);
        m_combatIcons[COMBAT_ICON_KEEP]->DrawToBuffer(
            inFlightX,
            inFlightY,
            arrowFrame + 1,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        UPDATE_INCLUSIVE_REGION(clipLeft, clipTop, maxX, maxY);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 10;
        lastX = inFlightX;
        lastY = inFlightY;
        inFlightX += gainX;
        inFlightY += gainY;
    }
    backing->DrawToBuffer(lastX, lastY);
    gWindowManager->UpdateScreenRegion(lastX, lastY, projectileSizeX, projectileSizeY);
    delete backing;
    mod = 2;
    if (m_heroes[COMBAT_DEFENDER_SIDE])
        mod += m_heroes[COMBAT_DEFENDER_SIDE]->m_primaryStats[HERO_PRIMARY_ATTACK];
    if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings
        & (1 << BUILDING_SLOT_MAGE_GUILD))
        mod += m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildState + 1;
    mod -= hisStack->m_stats.defense;
    if (mod > STAT_CURVE_OFFSET)
        mod = STAT_CURVE_OFFSET;
    if (mod < -STAT_CURVE_OFFSET)
        mod = -STAT_CURVE_OFFSET;
    numRolls = 5;
    for (k = BUILDING_SLOT_DWELLING_FIRST;
         k <= BUILDING_SLOT_DWELLING_LAST;
         k++) {
        if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings
            & (1 << k))
            numRolls += 4;
    }
    for (k = BUILDING_SLOT_MAGE_GUILD;
         k <= BUILDING_SLOT_GENERIC_LAST;
         k++) {
        if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings
            & (1 << k))
            numRolls++;
    }
    arrowDamage = 0;
    for (k = 0; k < numRolls; k++)
        arrowDamage += SRandom(2, 3);
    arrowDamage = arrowDamage * gBattleStat[mod + STAT_CURVE_OFFSET];
    if (arrowDamage <= 0)
        arrowDamage = 1;
    stackKilled = hisStack->Damage(arrowDamage);
    if (stackKilled > 0)
        sprintf(
            gText,
            "%s %d %s. %d %s %s.",
            localization::Tr("combat.tower.garrison.damage.prefix"),
            arrowDamage,
            localization::Tr("combat.fragment.damage_points"),
            stackKilled,
            CREATURE_DISPLAY_NAME(hisStack->m_creatureType, stackKilled),
            stackKilled <= 1 ? localization::Tr("combat.fragment.dies")
                             : localization::Tr("combat.fragment.killed")
        );
    else
        sprintf(
            gText,
            "%s %d %s.",
            localization::Tr("combat.tower.garrison.damage.prefix"),
            arrowDamage,
            localization::Tr("combat.fragment.damage_points")
        );
    gCombatManager->CombatMessage(gText, true);
    hisStack->PowEffect(hisStack->m_stats.powEffect);
    if (!(hisStack->m_stats.attributes & MONSTER_FLAGS_DEAD))
        hisStack->Stand(false);
    WaitSample(sample);
    if (hisStack->m_quantity > 0)
        hisStack->Stand(true);
    gMouseManager->ReallyShowPointer();
}

i32 combatManager::ExperienceValueOfStack(i8 side) {
    i32 i;
    i32 sideExperience;
    i32 lost;

    sideExperience = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[side][i].m_creatureType != CREATURE_NONE) {
            lost = (m_armies[side][i].m_initialQuantity - m_armies[side][i].m_quantity)
                   * gMonsterDatabase[m_armies[side][i].m_creatureType].hitPoints;
            if (lost > 0)
                sideExperience += lost;
        }
    }
    if (m_heroes[side])
        sideExperience += 500;
    return sideExperience;
}

void combatManager::ResetHitByCreature(void) {
    i32 j;
    i32 i;

    for (i = COMBAT_SIDE_FIRST; i < COMBAT_SIDE_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++)
            m_armies[i][j].m_hitByCreature = false;
    }
}

i32 ValidHex(i32 hex) {
    return hex >= 0 && hex <= COMBAT_HEX_COUNT - 1;
}

void combatManager::SaveCombatBorder(void) {
    if (!m_savedBorder)
        m_savedBorder = static_cast<char*>(
            malloc(LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT))
        );
    memcpy(
        m_savedBorder,
        gWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT,
        LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT)
    );
}

void combatManager::DrawCombatBorder(void) {
    if (!m_savedBorder)
        return;
    memcpy(
        gWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT,
        m_savedBorder,
        LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT)
    );
}
