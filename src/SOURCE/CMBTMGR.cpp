#include <match.h>

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

// CheckApplyGoodMorale grants one extra turn at a time.
DATA(0x004a67dc)
i32 gInHighMoraleBonus = 0;
// SetupCombat saves the adventure random seed here; GenerateMap restores it.
#define gSavedSeed gSeed // spelling fixes .data order
DATA(0x0048f060)
i32 gSavedSeed = 1;

VA(0x00418b30, 0x18b)
combatManager::combatManager(void) {
    m_drawRightToLeft = 0;
    m_unknown6f9 = -1;
    m_currentSide = COMBAT_DEFENDER_SIDE;
    m_limitCreatureHex = 0;
    m_limitCreature = 0;
    m_showArmyQuantities = 1;
    m_gridUpdateRow = 0;
    m_currentCommand = COMBAT_MESSAGE_COMMAND_DEFAULT;
    m_unknown6e8 = 0;
    m_currentSpeed = CREATURE_SPEED_BLAZING;
    m_savedBorder = NULL;
    m_heroClass[COMBAT_DEFENDER_SIDE] = m_heroClass[COMBAT_ATTACKER_SIDE] =
        m_catapultFrame[COMBAT_DEFENDER_SIDE] = m_catapultFrame[COMBAT_ATTACKER_SIDE] =
            m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
    m_unknown6d9 = m_unknown6db = 0;
    m_castleSide[COMBAT_DEFENDER_SIDE] = m_castleSide[COMBAT_ATTACKER_SIDE] = 0;
    m_combatWindowOpen = 0;
}

VA(0x00418cbb, 0x118)
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

// The attacker is side 1.
VA(0x00418dd3, 0x345)
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
    for (i = 0; i < COMBAT_SIDE_COUNT; i++) {
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
        m_visitingHeroPresent[i] = 0;
        m_heroCastSpell[i] = 0;
    }
    m_castleSide[COMBAT_ATTACKER_SIDE] = 0;
    if (defenderTown) {
        if (defenderTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
            m_armyGroups[COMBAT_DEFENDER_SIDE] = &m_heroes[COMBAT_DEFENDER_SIDE]->m_army;
            CombineGroups(&defenderTown->m_army, &m_heroes[COMBAT_DEFENDER_SIDE]->m_army);
            m_visitingHeroPresent[COMBAT_DEFENDER_SIDE] = 1;
        } else {
            m_visitingHeroPresent[COMBAT_DEFENDER_SIDE] = 0;
        }
        m_castleSide[COMBAT_DEFENDER_SIDE] =
            (defenderTown->m_buildings & (1 << BUILDING_SLOT_CASTLE)) ? 1 : 0;
        m_combatTowns[COMBAT_DEFENDER_SIDE] = defenderTown;
        m_originalCombatTown = defenderTown;
    } else {
        m_castleSide[COMBAT_DEFENDER_SIDE] = 0;
        m_combatTowns[COMBAT_DEFENDER_SIDE] = NULL;
    }
    m_combatTowns[COMBAT_ATTACKER_SIDE] = NULL;
}

// Open: screen buffer, combat window, icons, armies and field, then the
// fade-in and a random combat theme.
VA(0x00419118, 0x401)
i16 combatManager::Open(i16 priority) {
    i32 song;
    class sample* sample;
    i32 musicList[4];

    m_messageTypeMask = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                        | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                        | MESSAGE_WIDGET;
    m_combatWindowOpen = 0;
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
    m_backgroundDrawn = 0;
    sample = LoadPlaySample("PREBATTL.82M");
    gNextAction = ACTION_NONE;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    m_sideRetreated[COMBAT_DEFENDER_SIDE] = 0;
    m_sideRetreated[COMBAT_ATTACKER_SIDE] = 0;
    m_combatResult = COMBAT_RESULT_PENDING;
    gIconClipOn = 0;
    m_computeExtent = 0;
    m_redrawExtent = 0;
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
    gRetreatWin = 0;
    gCombatSurrender = 0;
    m_sideSurrendered[COMBAT_DEFENDER_SIDE] = 0;
    m_sideSurrendered[COMBAT_ATTACKER_SIDE] = 0;
    m_limitCreature = 1;
    SetDrawRightToLeft(0);
    m_gridUpdateRow = 0;
    m_combatWindowOpen = 1;
    DrawFrame(1);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    m_combatPalette = gResourceManager->GetPalette("kb.pal");
    KBChangeMenu(gCombatMenu);
    CombatMessage("", 1);
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, m_combatPalette);
    gLimitedCombatUpdatePalette = 1;
    gMouseManager->NewUpdate(1);
    gMouseManager->WarpPointer(
        m_hexCells[m_limitCreatureHex].m_x,
        m_hexCells[m_limitCreatureHex].m_y - 50
    );
    gMouseManager->ReallyShowPointer();
    m_gridSelectionDisabled = 0;
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

// CMBTMGR owns retail .data 0x004a2878-0x00491057. The backdrop table is
// GetBackgroundName's local static: /Gi emits it after Open's literals,
// followed by its own.

// A wandering-monster cell keeps the surviving count of the side that held
// it.
#define i ii // frame-slot spelling
VA(0x00419519, 0x211)
void combatManager::Close(void) {
    i32 i;
    i32 monsterSide;

    StopMusic();
    if (m_restoreSampleSuspension) {
        m_restoreSampleSuspension = false;
        SuspendSamples();
    }
    if (m_restoreMusicSuspension) {
        m_restoreMusicSuspension = false;
        if (m_savedMusicTrack >= 0)
            PlayMusic(m_savedMusicTrack);
        SuspendMusic();
        m_savedMusicTrack = MUSIC_TRACK_NONE;
    }
    DrawCombatBorder();
    gLimitedCombatUpdatePalette = 0;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    delete m_backgroundBuffer;
    for (i = 0; i < COMBAT_SIDE_COUNT; i++)
        UpdateArmyGroup(i);
    if (m_battlefieldCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
        monsterSide = m_playerId[COMBAT_DEFENDER_SIDE] != GAME_PLAYER_NONE ? static_cast<i8>(1)
                                                                           : static_cast<i8>(0);
        m_battlefieldCell->m_objectMetadata = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_armyGroups[monsterSide]->m_creatureTypes[i] != CREATURE_NONE)
                m_battlefieldCell->m_objectMetadata +=
                    m_armyGroups[monsterSide]->m_creatureCounts[i];
        }
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
    m_combatWindowOpen = 0;
}
#undef i

// Copy surviving counts back into the side's army group; a dead stack
// empties its slot.
VA(0x0041972a, 0x13b)
void combatManager::UpdateArmyGroup(i8 side) {
    i16 i;
    i16 j;

    for (i = 0; i < m_numArmies[side]; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_armyGroups[side]->m_creatureTypes[j] == m_armies[side][i].m_creatureType)
                break;
        }
        if (j < ARMY_GROUP_SLOT_COUNT) {
            if (m_armies[side][i].m_stats.attributes & MONSTER_FLAGS_DEAD) {
                m_armyGroups[side]->m_creatureTypes[j] = CREATURE_NONE;
                m_armyGroups[side]->m_creatureCounts[j] = 0;
            } else {
                m_armyGroups[side]->m_creatureCounts[j] = m_armies[side][i].m_quantity;
            }
        }
    }
}

// GenerateMap also places both armies, scatters ground patches and, outside
// a siege, up to two obstacles.
VA(0x00419865, 0x797)
void combatManager::GenerateMap(void) {
    i16 x;
    i16 i;
    i16 y;
    i16 count;
    i16 armyCount;

    m_catapultFrame[COMBAT_ATTACKER_SIDE] =
        m_castleSide[COMBAT_DEFENDER_SIDE] == 1 ? 0 : COMBAT_CATAPULT_FRAME_NONE;
    m_catapultFrame[COMBAT_DEFENDER_SIDE] =
        m_castleSide[COMBAT_ATTACKER_SIDE] == 1 ? 0 : COMBAT_CATAPULT_FRAME_NONE;
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
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_occupantFootprintHalf =
                HEXCELL_FOOTPRINT_HALF_NONE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex = COMBAT_OBSTACLE_NONE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_pathFlag = 0;
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
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex = SRandom(0, 2);
            if ((m_terrainType == TERRAIN_WATER || m_terrainType == TERRAIN_LAVA)
                && m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex
                       == COMBAT_OBSTACLE_LAND_ONLY_FRAME)
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex = 0;
        }
    }
    m_currentSide = COMBAT_DEFENDER_SIDE;
    m_currentSpeed = CREATURE_SPEED_BLAZING;
    GetNextArmy(0);
    m_gridUpdateRow = 0;
    SRand(gSavedSeed);
}

// A graveyard (or a hero standing on one) forces the graveyard field.
VA(0x00419ffc, 0x127)
char* combatManager::GetBackgroundName(void) {
    DATA(0x0048f064)
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
    if ((m_battlefieldCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_GRAVEYARD
        || ((m_battlefieldCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_HERO
            && (gGame->GetHero(m_battlefieldCell->m_objectMetadata)->m_locationType
                & MAP_TRIGGER_TYPE_MASK)
                   == MAP_OBJECT_GRAVEYARD)) {
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

// MoreTreesNear: tree (9) against mountain (8) objects within two cells of
// the battle.
#define radius pass // frame-slot spelling
VA(0x0041a123, 0x1d5)
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
        for (dir = 0; dir < MAP_DIRECTION_COUNT; dir++) {
            xPos = originX + normalDirTable[dir].x * radius;
            yPos = originY + normalDirTable[dir].y * radius;
            if (MAP_CELL_IN_BOUNDS(xPos, yPos)) {
                cell = gAdvManager->GetCell(xPos, yPos);
                nearbyTileset = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
                if (nearbyTileset == TILESET_MTN32)
                    nearbyTypeGrid[radius][dir] = 0;
                else if (nearbyTileset == TILESET_TREE32)
                    nearbyTypeGrid[radius][dir] = 1;
            }
        }
    }
    treeCount = 0;
    mountainCount = 0;
    for (radius = 0; radius < 3; radius++) {
        for (dir = 0; dir < MAP_DIRECTION_COUNT; dir++) {
            if (nearbyTypeGrid[radius][dir] == 0)
                mountainCount++;
            if (nearbyTypeGrid[radius][dir] == 1)
                treeCount++;
        }
    }
    if (treeCount > mountainCount)
        return 1;
    return 0;
}
#undef radius

VA(0x0041a2f8, 0x1c4)
void combatManager::LoadIcons(void) {
    i32 i;

    for (i = 0; i < COMBAT_ICON_COUNT; i++)
        m_combatIcons[i] = NULL;
    m_combatIcons[COMBAT_ICON_SPELLS] = gResourceManager->GetIcon("spells.icn");
    m_backgroundBitmap = gResourceManager->GetBitmap(GetBackgroundName());
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
            m_combatTowns
                [m_castleSide[COMBAT_ATTACKER_SIDE] == 1 ? static_cast<i8>(COMBAT_ATTACKER_SIDE)
                                                         : static_cast<i8>(COMBAT_DEFENDER_SIDE)]
                    ->m_type
        );
        m_combatIcons[COMBAT_ICON_CASTLE] = gResourceManager->GetIcon(gText);
        sprintf(gText, "keep%02d.icn", m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type);
        m_combatIcons[COMBAT_ICON_KEEP] = gResourceManager->GetIcon(gText);
    }
}

VA(0x0041a4bc, 0x6c)
void combatManager::FreeIcons(void) {
    i16 i;

    for (i = 0; i < COMBAT_ICON_COUNT; i++) {
        if (m_combatIcons[i])
            gResourceManager->Dispose(m_combatIcons[i]);
    }
    gResourceManager->Dispose(m_backgroundBitmap);
}

// LoadArmies places stacks itself after Init.
VA(0x0041a528, 0x25f)
void combatManager::LoadArmies(void) {
    i16 j;
    i16 i;

    m_numArmies[COMBAT_ATTACKER_SIDE] = m_numArmies[COMBAT_DEFENDER_SIDE] = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        for (j = 0; j < COMBAT_SIDE_COUNT; j++) {
            m_armies[j][i].m_quantity = 0;
            m_armies[j][i].m_creatureType = CREATURE_NONE;
        }
    }
    for (j = 0; j < COMBAT_SIDE_COUNT; j++) {
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

// Buka stops both music and samples before freeing the armies.
VA(0x0041a787, 0xbb)
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

// No callers and an empty body.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0041a842, 0xd)
void combatManager::NoShowCombatLog(char* message) {}

// GetGridIndex over the 9x5 grid: rows 80 pixels high from y 60, odd rows
// indented by 66 and even rows by 27, hexes 78 wide.
VA(0x0041a84f, 0xa7)
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

// CheckApplyGoodMorale rolls the group's morale.
#define moraleSound sample // frame-slot spelling
VA(0x0041a8f6, 0x1a5)
void combatManager::CheckApplyGoodMorale(i32 side, i32 index) {
    armyGroup* group;
    army* currentArmy;
    class sample* moraleSound;
    i32 moraleLevel;

    if (side < 0 || index < 0)
        return;
    if (gInHighMoraleBonus) {
        gInHighMoraleBonus = 0;
        return;
    }
    gInHighMoraleBonus = 0;
    group = m_armyGroups[side];
    currentArmy = &m_armies[side][index];
    if (!currentArmy->m_quantity)
        return;
    moraleLevel = group->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (moraleLevel <= 0 || SRandom(1, 24) > moraleLevel)
        return;
    gInHighMoraleBonus = 1;
    sprintf(gText, "goodmrle.82M");
    moraleSound = LoadPlaySample(gText);
    if (currentArmy->m_quantity <= 1)
        sprintf(
            gText,
            localization::Tr("combat.morale.good"),
            gArmyNames[currentArmy->m_creatureType]
        );
    else
        sprintf(
            gText,
            localization::Tr("combat.morale.good"),
            gArmyNamesPlural[currentArmy->m_creatureType]
        );
    CombatMessage(gText, 1);
    currentArmy->SpellEffect(COMBAT_EFFECT_GOOD_MORALE, 180);
    currentArmy->Stand(1);
    if (currentArmy->m_stats.attributes & MONSTER_FLAGS_TURN_SPENT)
        currentArmy->m_stats.attributes -= MONSTER_FLAGS_TURN_SPENT;
    currentArmy->m_stats.attributes |= MONSTER_FLAGS_HIGH_MORALE;
    WaitSample(moraleSound);
}
#undef moraleSound

// A computer side skips one bad-morale roll in four.
#define moraleSound sample // frame-slot spelling
VA(0x0041aa9b, 0x173)
i32 combatManager::CheckApplyBadMorale(i32 side, i32 index) {
    armyGroup* group;
    army* currentArmy;
    class sample* moraleSound;
    i32 moraleLevel;

    if (side < 0 || index < 0)
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
            localization::Tr("combat.morale.bad"),
            gArmyNames[currentArmy->m_creatureType]
        );
    else
        sprintf(
            gText,
            localization::Tr("combat.morale.bad"),
            gArmyNamesPlural[currentArmy->m_creatureType]
        );
    CombatMessage(gText, 1);
    currentArmy->m_animationFrame = 2;
    currentArmy->SpellEffect(COMBAT_EFFECT_BAD_MORALE, 180);
    currentArmy->Stand(1);
    currentArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
    WaitSample(moraleSound);
    return 1;
}
#undef moraleSound

// GetNextArmy: the fastest unspent stack, alternating sides, high-morale
// stacks first.
#define unused temp // frame-slot spelling
VA(0x0041ac0e, 0x1d9)
i8 combatManager::GetNextArmy(i32 checkMorale) {
    army* checkArmy;
    i8 speedLevelIndex;
    i32 sideIter;
    i16 unused;
    i8 armyCounter;
    i8 stackSide;
    i32 skip;

    stackSide = m_currentSide;
    for (speedLevelIndex = 0; speedLevelIndex < 5; speedLevelIndex++) {
        for (sideIter = 0; sideIter < COMBAT_SIDE_COUNT; sideIter++) {
            stackSide ^= 1;
            for (armyCounter = 0; armyCounter < m_numArmies[stackSide]; armyCounter++) {
                skip = 0;
                checkArmy = &m_armies[stackSide][armyCounter];
                if ((checkArmy->m_stats.attributes
                     & (MONSTER_FLAGS_DEAD | MONSTER_FLAGS_TURN_SPENT))
                    || checkArmy->m_spellEffect == SPELL_PARALYZE
                    || checkArmy->m_spellEffect == SPELL_BLIND
                    || (checkArmy->m_stats.speed != m_currentSpeed
                        && !(checkArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE)))
                    skip = 1;
                if (!skip && !speedLevelIndex
                    && !(checkArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE))
                    skip = 1;
                if (!skip && checkMorale && CheckApplyBadMorale(stackSide, armyCounter))
                    skip = 1;
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
#undef unused

// IsWinner: the other side surrendered, retreated or has no live stack left.
VA(0x0041ade7, 0xb5)
i8 combatManager::IsWinner(i8 side) {
    i8 isWinner;
    i16 i;

    if (m_sideSurrendered[1 - side])
        return 1;
    if (m_sideRetreated[1 - side])
        return 1;
    side ^= 1;
    isWinner = 1;
    for (i = 0; i < m_numArmies[side]; i++) {
        if (!(m_armies[side][i].m_stats.attributes & MONSTER_FLAGS_DEAD))
            isWinner = 0;
    }
    return isWinner;
}

// HoMM1 catapult: a boulder arcs (or, for the top row, flies straight) at
// a random standing wall piece; a breach roll knocks it down, otherwise
// the piece is damaged.
#define catapultSound sampleInfo // frame-slot spelling
VA(0x0041ae9c, 0xcf2)
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
    i8 anyStanding;
    i16 startY;
    i16 topPosY;

    if (!m_castleSide[COMBAT_DEFENDER_SIDE])
        return;
    catapultSound = NULL;
    castleColumn = side == COMBAT_ATTACKER_SIDE
                       ? COMBAT_CASTLE_WALL_COLUMN
                       : COMBAT_GRID_LAST_COLUMN - COMBAT_CASTLE_WALL_COLUMN;
    anyStanding = 0;
    for (i = 0; i < COMBAT_GRID_ROWS; i++) {
        if (m_hexCells[i * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex
            != COMBAT_OBSTACLE_NONE)
            anyStanding = 1;
    }
    if (!anyStanding)
        return;
    gMouseManager->ReallyHidePointer();
    boulderIcon = gResourceManager->GetIcon("boulder.icn");
    sprintf(gText, "catsnd%02d.82M", 0);
    catapultSound = LoadPlaySample(gText);
    gMinExtentX = 0;
    gMaxExtentX = 200;
    gMinExtentY = 190;
    gMaxExtentY = 420;
    m_catapultFrame[side] = 0;
    while (m_catapultFrame[side] < 8) {
        m_redrawExtent = 1;
        DrawFrame(1);
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
            m_redrawExtent = 1;
            if (i) {
                gMinExtentX = xPos - xDelta - 20;
                gMaxExtentX = xPos + 75;
                gMinExtentY = yPos - 75;
                gMaxExtentY = yPos + 75;
                if (gMinExtentX < 0)
                    gMinExtentX = 0;
                if (gMinExtentY < 0)
                    gMinExtentY = 0;
                if (gMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                    gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
                if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                    gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            }
            DrawFrame(0);
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
            m_redrawExtent = 1;
            if (i) {
                gMinExtentX = xPos - xDelta - 20;
                gMaxExtentX = xPos + 75;
                gMinExtentY = yPos - 75;
                gMaxExtentY = yPos + 75;
                if (gMinExtentX < 0)
                    gMinExtentX = 0;
                if (gMinExtentY < 0)
                    gMinExtentY = 0;
                if (gMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                    gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
                if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                    gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            }
            DrawFrame(0);
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
            m_redrawExtent = 1;
            gMinExtentX = xPos - xDelta - 20;
            gMaxExtentX = xPos + 75;
            gMinExtentY = yPos - 75;
            gMaxExtentY = yPos + 75;
            if (gMinExtentX < 0)
                gMinExtentX = 0;
            if (gMinExtentY < 0)
                gMinExtentY = 0;
            if (gMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
            if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            DrawFrame(0);
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
        m_wallSurvives = 0;
        m_wallFrame = 0;
        gMinExtentX = 300;
        gMaxExtentX = 490;
        gMinExtentY = m_catapultTargetRow * COMBAT_HEX_HEIGHT - 30;
        gMaxExtentY = (m_catapultTargetRow + 2) * COMBAT_HEX_HEIGHT + 30;
        if (gMinExtentY < 0)
            gMinExtentY = 0;
        if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        while (m_wallFrame < 10) {
            m_wallDamage = m_wallFrame;
            if (m_wallFrame == COMBAT_WALL_COLLAPSE_FRAME)
                m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn]
                    .m_obstacleIndex = COMBAT_WALL_COLLAPSING;
            m_redrawExtent = 1;
            m_gridUpdateRow = m_catapultTargetRow - 2;
            if (m_gridUpdateRow < 0)
                m_gridUpdateRow = 0;
            DrawFrame(1);
            m_wallFrame++;
        }
        m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
        m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex =
            COMBAT_OBSTACLE_NONE;
    } else {
        m_wallSurvives = 1;
        m_wallFrame = 0;
        gMinExtentX = 300;
        gMaxExtentX = 490;
        gMinExtentY = m_catapultTargetRow * COMBAT_HEX_HEIGHT - 30;
        gMaxExtentY = (m_catapultTargetRow + 2) * COMBAT_HEX_HEIGHT + 30;
        if (gMinExtentY < 0)
            gMinExtentY = 0;
        if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        while (m_wallFrame < 10) {
            if (m_wallFrame == COMBAT_WALL_COLLAPSE_FRAME)
                m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn]
                    .m_obstacleIndex = COMBAT_WALL_DAMAGED_HIT;
            m_redrawExtent = 1;
            DrawFrame(1);
            m_wallFrame++;
        }
        m_hexCells[m_catapultTargetRow * COMBAT_GRID_COLUMNS + castleColumn].m_obstacleIndex =
            COMBAT_WALL_DAMAGED;
        m_wallFrame = COMBAT_WALL_FRAME_NONE;
    }
    m_redrawExtent = 1;
    DrawFrame(1);
    gMinExtentX = 0;
    gMaxExtentX = 200;
    gMinExtentY = 220;
    gMaxExtentY = 420;
    while (m_catapultFrame[side] < 14) {
        m_redrawExtent = 1;
        DrawFrame(1);
        m_catapultFrame[side]++;
    }
    m_catapultFrame[side] = 0;
    m_redrawExtent = 1;
    DrawFrame(1);
    gResourceManager->Dispose(boulderIcon);
    gMouseManager->ReallyShowPointer();
    WaitSample(catapultSound);
}
#undef catapultSound

// Unreferenced; reloads the armies and rebuilds the field before a full
// redraw.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0041bb8e, 0x43)
void combatManager::RegenerateField(void) {
    FreeArmies();
    LoadArmies();
    GenerateMap();
    SetDrawRightToLeft(0);
    m_gridUpdateRow = 0;
    DrawFrame(1);
}

// HoMM1 castle keep: shoots the attacker's most dangerous stack (shooters,
// then flyers, then fight value) with dice from the town's buildings.
VA(0x0041bbd1, 0xae3)
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
    i8 shotTable[45] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 0, 1, 1,
                        1, 1, 2, 0, 0, 0, 1, 1, 1, 1, 2, 2, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0};
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
    if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings & 1)
        mod += m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildState + 1;
    mod -= hisStack->m_stats.defense;
    if (mod > 20)
        mod = 20;
    if (mod < -20)
        mod = -20;
    numRolls = 5;
    for (k = 7; k <= 12; k++) {
        if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings & (1 << k))
            numRolls += 4;
    }
    for (k = 0; k <= 4; k++) {
        if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings & (1 << k))
            numRolls++;
    }
    arrowDamage = 0;
    for (k = 0; k < numRolls; k++)
        arrowDamage += SRandom(2, 3);
    arrowDamage = arrowDamage * gBattleStat[mod + 20];
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
    gCombatManager->CombatMessage(gText, 1);
    hisStack->PowEffect(hisStack->m_stats.powEffect);
    if (!(hisStack->m_stats.attributes & MONSTER_FLAGS_DEAD))
        hisStack->Stand(0);
    WaitSample(sample);
    if (hisStack->m_quantity > 0)
        hisStack->Stand(1);
    gMouseManager->ReallyShowPointer();
}

// ExperienceValueOfStack: fight value of the side's losses, plus 500 for a
// defeated hero.
VA(0x0041c6b4, 0xec)
i32 combatManager::ExperienceValueOfStack(i8 side) {
    i32 i;
    i32 sideExperience;

    sideExperience = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[side][i].m_creatureType != CREATURE_NONE)
            sideExperience += (m_armies[side][i].m_initialQuantity - m_armies[side][i].m_quantity)
                              * gMonsterDatabase[m_armies[side][i].m_creatureType].hitPoints;
    }
    if (m_heroes[side])
        sideExperience += 500;
    return sideExperience;
}

VA(0x0041c7a0, 0x5f)
void combatManager::ResetHitByCreature(void) {
    i32 j;
    i32 i;

    for (i = 0; i < COMBAT_SIDE_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++)
            m_armies[i][j].m_hitByCreature = 0;
    }
}

// HoMM1's combat grid is nine columns by five rows.
VA(0x0041c7ff, 0x27)
i32 ValidHex(i32 hex) {
    return hex >= 0 && hex <= COMBAT_HEX_COUNT - 1;
}

// HoMM1 SaveCombatBorder: keep the twenty screen rows under the field.
VA(0x0041c826, 0x57)
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

// HoMM1 DrawCombatBorder: put the saved rows back.
VA(0x0041c87d, 0x42)
void combatManager::DrawCombatBorder(void) {
    if (!m_savedBorder)
        return;
    memcpy(
        gWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT,
        m_savedBorder,
        LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT)
    );
}

VA_COMPGEN(0x0041c900, 0x27, "??_H@YGXPAXIHP6EX0@Z@Z", 0x00418b30)
