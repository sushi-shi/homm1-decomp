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
DATA(0x0048f060)
i32 gSeed = 1;

VA(0x00418b30, 0x18b)
combatManager::combatManager(void) {
    m_gridMode = 0;
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
    m_heroType[COMBAT_DEFENDER_SIDE] = m_heroType[COMBAT_ATTACKER_SIDE] =
        m_catapultFrame[COMBAT_DEFENDER_SIDE] = m_catapultFrame[COMBAT_ATTACKER_SIDE] =
            m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
    m_unknown6d9 = m_unknown6db = 0;
    m_castleSide[COMBAT_DEFENDER_SIDE] = m_castleSide[COMBAT_ATTACKER_SIDE] = 0;
    m_combatWindowOpen = 0;
}

VA(0x00418cbb, 0x118)
void combatManager::CombineGroups(armyGroup* from, armyGroup* to) {
    i16 i;
    i16 j;

    if (!from || !to)
        return;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (to->IsMember(from->m_creatureTypes[i])) {
            to->Add(from->m_creatureTypes[i], from->m_creatureCounts[i], ARMY_GROUP_EMPTY_SLOT);
            from->Dismiss(i);
        }
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (from->m_creatureTypes[i] != CREATURE_NONE) {
            for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                if (to->m_creatureTypes[j] == CREATURE_NONE) {
                    to->Add(from->m_creatureTypes[i], from->m_creatureCounts[i], j);
                    from->Dismiss(i);
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

    gSeed = randomSeed;
    SRand(combatX * 100 + combatY);
    m_combatX = combatX;
    m_combatY = combatY;
    if (mapX >= 0 && mapY >= 0)
        m_battlefieldCell = gpAdvManager->GetCell(mapX, mapY);
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
            m_humanSide[i] = gbHumanPlayer[m_playerId[i]];
        else
            m_humanSide[i] = 0;
        m_heroes[i] = i == COMBAT_ATTACKER_SIDE ? attackerHero : defenderHero;
        m_heroType[i] = m_heroes[i] ? m_heroes[i]->m_heroClass : COMBAT_HERO_TYPE_NONE;
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
    giNextAction = ACTION_NONE;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    m_sideRetreated[COMBAT_DEFENDER_SIDE] = 0;
    m_sideRetreated[COMBAT_ATTACKER_SIDE] = 0;
    m_combatResult = COMBAT_RESULT_PENDING;
    gbIconClipOn = 0;
    m_computeExtent = 0;
    m_redrawExtent = 0;
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
    gpMouseManager->SetPointer("cmbtmous.mse", COMBAT_POINTER_DEFAULT);
    m_combatWindow = new heroWindow(0, 0, "cmbtwin.bin");
    if (!m_combatWindow)
        MemError();
    gpWindowManager->AddWindow(m_combatWindow, WINDOW_Z_ORDER_APPEND, 1);
    m_smallFont = gpResourceManager->GetFont("smalfont.fnt");
    LoadIcons();
    LoadArmies();
    m_selectedHex = ARMY_HEX_INVALID;
    m_limitCreatureHex = ARMY_HEX_INVALID;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    GenerateMap();
    gbRetreatWin = 0;
    gbCombatSurrender = 0;
    m_sideDefeated[COMBAT_DEFENDER_SIDE] = 0;
    m_sideDefeated[COMBAT_ATTACKER_SIDE] = 0;
    m_limitCreature = 1;
    SetGridMode(0);
    m_gridUpdateRow = 0;
    m_combatWindowOpen = 1;
    DrawFrame(1);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
    m_combatPalette = gpResourceManager->GetPalette("kb.pal");
    KBChangeMenu(hmnuCmbt);
    CombatMessage("", 1);
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, m_combatPalette);
    gLimitedCombatUpdatePalette = 1;
    gpMouseManager->NewUpdate(1);
    gpMouseManager->WarpPointer(
        m_hexCells[m_limitCreatureHex].m_x,
        m_hexCells[m_limitCreatureHex].m_y - 50
    );
    gpMouseManager->ReallyShowPointer();
    m_gridSelectionDisabled = 0;
    WaitSample(sample);
    musicList[0] = MUSIC_TRACK_BATTLE_2;
    musicList[1] = MUSIC_TRACK_BATTLE_3;
    musicList[2] = MUSIC_TRACK_BATTLE_1;
    musicList[3] = MUSIC_TRACK_BATTLE_4;
    song = musicList[SRandom(0, 3)];
    PlayMusic(song);
    gpInputManager->Flush();
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
VA(0x00419519, 0x211)
void combatManager::Close(void) {
    i32 ii;
    i32 survivor;

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
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    delete m_backgroundBuffer;
    for (ii = 0; ii < COMBAT_SIDE_COUNT; ii++)
        UpdateArmyGroup(ii);
    if (m_battlefieldCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
        survivor = m_playerId[COMBAT_DEFENDER_SIDE] != GAME_PLAYER_NONE ? static_cast<i8>(1)
                                                                        : static_cast<i8>(0);
        m_battlefieldCell->m_objectMetadata = 0;
        for (ii = 0; ii < ARMY_GROUP_SLOT_COUNT; ii++) {
            if (m_armyGroups[survivor]->m_creatureTypes[ii] != CREATURE_NONE)
                m_battlefieldCell->m_objectMetadata += m_armyGroups[survivor]->m_creatureCounts[ii];
        }
    }
    gpWindowManager->RemoveWindow(m_combatWindow);
    FreeArmies();
    FreeIcons();
    gpResourceManager->Dispose(m_smallFont);
    gpResourceManager->Dispose(m_combatPalette);
    delete m_combatWindow;
    if (m_savedBorder)
        free(m_savedBorder);
    m_active = 0;
    m_combatWindowOpen = 0;
}

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
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_occupantFrame = HEXCELL_OCCUPANT_FRAME_NONE;
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
            m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_obstacleType =
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
                    .m_occupantFrame = ARMY_FACING_LEFT;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1]
                    .m_occupantFrame = ARMY_FACING_RIGHT;
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
                    .m_occupantFrame = ARMY_FACING_LEFT;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN]
                    .m_occupantFrame = ARMY_FACING_RIGHT;
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
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleType = COMBAT_ICON_OBSTACLES;
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
    SRand(gSeed);
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
            && (gpGame->GetHero(m_battlefieldCell->m_objectMetadata)->m_locationType
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
VA(0x0041a123, 0x1d5)
i8 combatManager::MoreTreesNear(void) {
    i32 yPos;
    i32 xPos;
    i16 pass;
    i16 posX;
    i8 lastTypeTable[3][MAP_DIRECTION_COUNT];
    i16 curNumMountains;
    i16 numTrees;
    mapCell* tile;
    i16 homeY;
    u8 nearbyTileset;
    i16 n;

    memset(lastTypeTable, -1, sizeof(lastTypeTable));
    posX = m_combatX;
    homeY = m_combatY;
    for (pass = 0; pass < 3; pass++) {
        for (n = 0; n < MAP_DIRECTION_COUNT; n++) {
            xPos = posX + normalDirTable[n].x * pass;
            yPos = homeY + normalDirTable[n].y * pass;
            if (xPos >= 0 && xPos < MAP_CELL_GRID_SIZE && yPos >= 0 && yPos < MAP_CELL_GRID_SIZE) {
                tile = gpAdvManager->GetCell(xPos, yPos);
                nearbyTileset = tile->m_objectTileset & MAP_CELL_TILESET_MASK;
                if (nearbyTileset == TILESET_MTN32)
                    lastTypeTable[pass][n] = 0;
                else if (nearbyTileset == TILESET_TREE32)
                    lastTypeTable[pass][n] = 1;
            }
        }
    }
    numTrees = 0;
    curNumMountains = 0;
    for (pass = 0; pass < 3; pass++) {
        for (n = 0; n < MAP_DIRECTION_COUNT; n++) {
            if (lastTypeTable[pass][n] == 0)
                curNumMountains++;
            if (lastTypeTable[pass][n] == 1)
                numTrees++;
        }
    }
    if (numTrees > curNumMountains)
        return 1;
    return 0;
}

VA(0x0041a2f8, 0x1c4)
void combatManager::LoadIcons(void) {
    i32 i;

    for (i = 0; i < COMBAT_ICON_COUNT; i++)
        m_combatIcons[i] = NULL;
    m_combatIcons[COMBAT_ICON_SPELLS] = gpResourceManager->GetIcon("spells.icn");
    m_backgroundBitmap = gpResourceManager->GetBitmap(GetBackgroundName());
    m_combatIcons[COMBAT_ICON_GROUND] =
        gpResourceManager->GetIcon(gCombatGroundNames[m_terrainType]);
    m_combatIcons[COMBAT_ICON_OBSTACLES] =
        gpResourceManager->GetIcon(gCombatObstacleNames[m_terrainType]);
    m_combatIcons[COMBAT_ICON_TEXTBAR] = gpResourceManager->GetIcon("textbar.icn");
    m_combatIcons[COMBAT_ICON_TENT] = gpResourceManager->GetIcon("tent.icn");
    m_combatIcons[COMBAT_ICON_CLOUD] = gpResourceManager->GetIcon("cloud.icn");
    if (m_castleSide[COMBAT_ATTACKER_SIDE] || m_castleSide[COMBAT_DEFENDER_SIDE]) {
        m_combatIcons[COMBAT_ICON_CATAPULT] = gpResourceManager->GetIcon("catapult.icn");
        sprintf(
            gText,
            "castle%02d.icn",
            m_combatTowns
                [m_castleSide[COMBAT_ATTACKER_SIDE] == 1 ? static_cast<i8>(COMBAT_ATTACKER_SIDE)
                                                         : static_cast<i8>(COMBAT_DEFENDER_SIDE)]
                    ->m_type
        );
        m_combatIcons[COMBAT_ICON_CASTLE] = gpResourceManager->GetIcon(gText);
        sprintf(gText, "keep%02d.icn", m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type);
        m_combatIcons[COMBAT_ICON_KEEP] = gpResourceManager->GetIcon(gText);
    }
}

VA(0x0041a4bc, 0x6c)
void combatManager::FreeIcons(void) {
    i16 i;

    for (i = 0; i < COMBAT_ICON_COUNT; i++) {
        if (m_combatIcons[i])
            gpResourceManager->Dispose(m_combatIcons[i]);
    }
    gpResourceManager->Dispose(m_backgroundBitmap);
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
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
}

// No callers and an empty body.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0041a842, 0xd)
void combatManager::NoShowCombatLog(char*) {}

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
VA(0x0041a8f6, 0x1a5)
void combatManager::CheckApplyGoodMorale(i32 side, i32 index) {
    armyGroup* theGroup;
    army* activeArmyRef;
    class sample* sample;
    i32 moraleVal;

    if (side < 0 || index < 0)
        return;
    if (gInHighMoraleBonus) {
        gInHighMoraleBonus = 0;
        return;
    }
    gInHighMoraleBonus = 0;
    theGroup = m_armyGroups[side];
    activeArmyRef = &m_armies[side][index];
    if (!activeArmyRef->m_quantity)
        return;
    moraleVal = theGroup->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (moraleVal <= 0 || SRandom(1, 24) > moraleVal)
        return;
    gInHighMoraleBonus = 1;
    sprintf(gText, "goodmrle.82M");
    sample = LoadPlaySample(gText);
    if (activeArmyRef->m_quantity <= 1)
        sprintf(
            gText,
            localization::Tr("combat.morale.good"),
            gArmyNames[activeArmyRef->m_creatureType]
        );
    else
        sprintf(
            gText,
            localization::Tr("combat.morale.good"),
            gArmyNamesPlural[activeArmyRef->m_creatureType]
        );
    CombatMessage(gText, 1);
    activeArmyRef->SpellEffect(COMBAT_EFFECT_GOOD_MORALE, 180);
    activeArmyRef->Stand(1);
    if (activeArmyRef->m_stats.attributes & MONSTER_FLAGS_TURN_SPENT)
        activeArmyRef->m_stats.attributes -= MONSTER_FLAGS_TURN_SPENT;
    activeArmyRef->m_stats.attributes |= MONSTER_FLAGS_HIGH_MORALE;
    WaitSample(sample);
}

// A computer side skips one bad-morale roll in four.
VA(0x0041aa9b, 0x173)
i32 combatManager::CheckApplyBadMorale(i32 side, i32 index) {
    armyGroup* theGroup;
    army* activeArmyRef;
    class sample* sample;
    i32 moraleVal;

    if (side < 0 || index < 0)
        return 0;
    theGroup = m_armyGroups[side];
    activeArmyRef = &m_armies[side][index];
    moraleVal = theGroup->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (moraleVal >= 0 || SRandom(1, 12) > -moraleVal)
        return 0;
    if (!m_humanSide[side] && SRandom(1, 4) == 1)
        return 0;
    sample = LoadPlaySample("BADMRLE.82M");
    if (activeArmyRef->m_quantity <= 1)
        sprintf(
            gText,
            localization::Tr("combat.morale.bad"),
            gArmyNames[activeArmyRef->m_creatureType]
        );
    else
        sprintf(
            gText,
            localization::Tr("combat.morale.bad"),
            gArmyNamesPlural[activeArmyRef->m_creatureType]
        );
    CombatMessage(gText, 1);
    activeArmyRef->m_animationFrame = 2;
    activeArmyRef->SpellEffect(COMBAT_EFFECT_BAD_MORALE, 180);
    activeArmyRef->Stand(1);
    activeArmyRef->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
    WaitSample(sample);
    return 1;
}

// GetNextArmy: the fastest unspent stack, alternating sides, high-morale
// stacks first.
VA(0x0041ac0e, 0x1d9)
i8 combatManager::GetNextArmy(i32 checkMorale) {
    army* theArmy;
    i8 oldSpeed;
    i32 sideIter;
    i16 temp;
    i8 counterValue;
    i8 stackSide;
    i32 bSkip;

    stackSide = m_currentSide;
    for (oldSpeed = 0; oldSpeed < 5; oldSpeed++) {
        for (sideIter = 0; sideIter < COMBAT_SIDE_COUNT; sideIter++) {
            stackSide ^= 1;
            for (counterValue = 0; counterValue < m_numArmies[stackSide]; counterValue++) {
                bSkip = 0;
                theArmy = &m_armies[stackSide][counterValue];
                if ((theArmy->m_stats.attributes & (MONSTER_FLAGS_DEAD | MONSTER_FLAGS_TURN_SPENT))
                    || theArmy->m_spellEffect == SPELL_PARALYZE
                    || theArmy->m_spellEffect == SPELL_BLIND
                    || (theArmy->m_stats.speed != m_currentSpeed
                        && !(theArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE)))
                    bSkip = 1;
                if (!bSkip && !oldSpeed
                    && !(theArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE))
                    bSkip = 1;
                if (!bSkip && checkMorale && CheckApplyBadMorale(stackSide, counterValue))
                    bSkip = 1;
                if (!bSkip)
                    break;
            }
            if (counterValue != m_numArmies[stackSide]) {
                m_currentSide = stackSide;
                m_currentArmyIndex = counterValue;
                GetControl();
                return 1;
            }
        }
        if (oldSpeed) {
            m_currentSpeed--;
            if (!m_currentSpeed)
                m_currentSpeed = CREATURE_SPEED_BLAZING;
        }
    }
    GetControl();
    return 0;
}

// IsWinner: the other side surrendered, retreated or has no live stack left.
VA(0x0041ade7, 0xb5)
i8 combatManager::IsWinner(i8 side) {
    i8 isWinner;
    i16 i;

    if (m_sideDefeated[1 - side])
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
VA(0x0041ae9c, 0xcf2)
void combatManager::CatAttack(i8 side) {
    i16 dxVal;
    icon* boulderRef;
    i16 summitXValue;
    i16 xPos;
    i8 nextCol;
    i16 i;
    i16 prevFrm;
    i16 localDy;
    i16 ourY;
    i16 posY;
    i16 curTgtX;
    i16 activeForce;
    i16 startX;
    class sample* sampleInfo;
    i8 theLeft;
    i16 startY;
    i16 newY;

    if (!m_castleSide[COMBAT_DEFENDER_SIDE])
        return;
    sampleInfo = NULL;
    nextCol = side == COMBAT_ATTACKER_SIDE ? COMBAT_CASTLE_WALL_COLUMN
                                           : COMBAT_GRID_LAST_COLUMN - COMBAT_CASTLE_WALL_COLUMN;
    theLeft = 0;
    for (i = 0; i < COMBAT_GRID_ROWS; i++) {
        if (m_hexCells[i * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
            theLeft = 1;
    }
    if (!theLeft)
        return;
    gpMouseManager->ReallyHidePointer();
    boulderRef = gpResourceManager->GetIcon("boulder.icn");
    sprintf(gText, "catsnd%02d.82M", 0);
    sampleInfo = LoadPlaySample(gText);
    giMinExtentX = 0;
    giMaxExtentX = 200;
    giMinExtentY = 190;
    giMaxExtentY = 420;
    m_catapultFrame[side] = 0;
    while (m_catapultFrame[side] < 8) {
        m_redrawExtent = 1;
        DrawFrame(1);
        m_catapultFrame[side]++;
    }
    if ((m_hexCells[nextCol + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_WALL_DAMAGED
         || m_hexCells[nextCol + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
        && (m_hexCells[nextCol + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_WALL_DAMAGED
            || m_hexCells[nextCol + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
                   == COMBAT_OBSTACLE_NONE)) {
        m_catapultTarget = SRandom(0, 4);
        while (m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex
               == COMBAT_OBSTACLE_NONE)
            m_catapultTarget = SRandom(0, 4);
    } else if (m_hexCells[nextCol + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
        m_catapultTarget = COMBAT_LOWER_WALL_ROW;
    } else if (m_hexCells[nextCol + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
               == COMBAT_OBSTACLE_NONE) {
        m_catapultTarget = COMBAT_UPPER_WALL_ROW;
    } else if (m_hexCells[nextCol + COMBAT_GRID_COLUMNS].m_obstacleIndex != COMBAT_WALL_INTACT) {
        m_catapultTarget = COMBAT_LOWER_WALL_ROW;
    } else if (m_hexCells[nextCol + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex
               != COMBAT_WALL_INTACT) {
        m_catapultTarget = COMBAT_UPPER_WALL_ROW;
    } else {
        m_catapultTarget = SRandom(0, 1);
        m_catapultTarget = m_catapultTarget ? COMBAT_LOWER_WALL_ROW : COMBAT_UPPER_WALL_ROW;
    }
    startX = 0x75;
    startY = 0x104;
    curTgtX = m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_x;
    posY = m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_y
           - COMBAT_HEX_HEIGHT;
    prevFrm = 0;
    xPos = startX;
    ourY = startY;
    if (!m_catapultTarget) {
        dxVal = (curTgtX - startX) / 12;
        localDy = (posY - startY) / 12;
        i = 0;
        while (i < 12) {
            m_redrawExtent = 1;
            if (i) {
                giMinExtentX = xPos - dxVal - 20;
                giMaxExtentX = xPos + 75;
                giMinExtentY = ourY - 75;
                giMaxExtentY = ourY + 75;
                if (giMinExtentX < 0)
                    giMinExtentX = 0;
                if (giMinExtentY < 0)
                    giMinExtentY = 0;
                if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                    giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
                if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                    giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            }
            DrawFrame(0);
            boulderRef->DrawToBuffer(xPos, ourY, prevFrm, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            UPDATE_INCLUSIVE_REGION(giMinExtentX, giMinExtentY, giMaxExtentX, giMaxExtentY);
            xPos += dxVal;
            ourY += localDy;
            prevFrm++;
            prevFrm %= 3;
            if (i < 2)
                m_catapultFrame[side]++;
            i++;
        }
    } else {
        summitXValue = (startX + curTgtX) / 2;
        switch (m_catapultTarget) {
            case COMBAT_UPPER_WALL_ROW:
                newY = 25;
                break;
            default:
                newY = m_catapultTarget * 20 + 25;
                break;
        }
        dxVal = (summitXValue - startX) / 12;
        localDy = (newY - startY) / 78;
        for (i = 0; i < 12; i++) {
            m_redrawExtent = 1;
            if (i) {
                giMinExtentX = xPos - dxVal - 20;
                giMaxExtentX = xPos + 75;
                giMinExtentY = ourY - 75;
                giMaxExtentY = ourY + 75;
                if (giMinExtentX < 0)
                    giMinExtentX = 0;
                if (giMinExtentY < 0)
                    giMinExtentY = 0;
                if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                    giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
                if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                    giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            }
            DrawFrame(0);
            boulderRef->DrawToBuffer(xPos, ourY, prevFrm, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            UPDATE_INCLUSIVE_REGION(giMinExtentX, giMinExtentY, giMaxExtentX, giMaxExtentY);
            xPos += dxVal;
            ourY += (12 - i) * localDy;
            prevFrm++;
            prevFrm %= 3;
            if (i < 2)
                m_catapultFrame[side]++;
        }
        dxVal = (curTgtX - xPos) / 8;
        localDy = (posY - ourY) / 36;
        for (i = 1; i <= 8; i++) {
            m_redrawExtent = 1;
            giMinExtentX = xPos - dxVal - 20;
            giMaxExtentX = xPos + 75;
            giMinExtentY = ourY - 75;
            giMaxExtentY = ourY + 75;
            if (giMinExtentX < 0)
                giMinExtentX = 0;
            if (giMinExtentY < 0)
                giMinExtentY = 0;
            if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
            if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            DrawFrame(0);
            boulderRef->DrawToBuffer(xPos, ourY, prevFrm, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            UPDATE_INCLUSIVE_REGION(giMinExtentX, giMinExtentY, giMaxExtentX, giMaxExtentY);
            xPos += dxVal;
            ourY += i * localDy;
            prevFrm++;
            prevFrm %= 3;
        }
    }
    WaitSample(sampleInfo);
    sprintf(gText, "catsnd%02d.82M", 2);
    sampleInfo = LoadPlaySample(gText);
    if (m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex
        == COMBAT_WALL_DAMAGED)
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex =
            COMBAT_WALL_DAMAGED_HIT;
    else
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex =
            COMBAT_WALL_INTACT_HIT;
    activeForce = SRandom(0, 150);
    if (!gbHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        activeForce -= 15;
    if (activeForce < 30
        || m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex
               == COMBAT_WALL_DAMAGED_HIT) {
        m_wallSurvives = 0;
        m_wallFrame = 0;
        giMinExtentX = 300;
        giMaxExtentX = 490;
        giMinExtentY = m_catapultTarget * COMBAT_HEX_HEIGHT - 30;
        giMaxExtentY = (m_catapultTarget + 2) * COMBAT_HEX_HEIGHT + 30;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        while (m_wallFrame < 10) {
            m_wallDamage = m_wallFrame;
            if (m_wallFrame == COMBAT_WALL_COLLAPSE_FRAME)
                m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex =
                    COMBAT_WALL_COLLAPSING;
            m_redrawExtent = 1;
            m_gridUpdateRow = m_catapultTarget - 2;
            if (m_gridUpdateRow < 0)
                m_gridUpdateRow = 0;
            DrawFrame(1);
            m_wallFrame++;
        }
        m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex =
            COMBAT_OBSTACLE_NONE;
    } else {
        m_wallSurvives = 1;
        m_wallFrame = 0;
        giMinExtentX = 300;
        giMaxExtentX = 490;
        giMinExtentY = m_catapultTarget * COMBAT_HEX_HEIGHT - 30;
        giMaxExtentY = (m_catapultTarget + 2) * COMBAT_HEX_HEIGHT + 30;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        while (m_wallFrame < 10) {
            if (m_wallFrame == COMBAT_WALL_COLLAPSE_FRAME)
                m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex =
                    COMBAT_WALL_DAMAGED_HIT;
            m_redrawExtent = 1;
            DrawFrame(1);
            m_wallFrame++;
        }
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + nextCol].m_obstacleIndex =
            COMBAT_WALL_DAMAGED;
        m_wallFrame = COMBAT_WALL_FRAME_NONE;
    }
    m_redrawExtent = 1;
    DrawFrame(1);
    giMinExtentX = 0;
    giMaxExtentX = 200;
    giMinExtentY = 220;
    giMaxExtentY = 420;
    while (m_catapultFrame[side] < 14) {
        m_redrawExtent = 1;
        DrawFrame(1);
        m_catapultFrame[side]++;
    }
    m_catapultFrame[side] = 0;
    m_redrawExtent = 1;
    DrawFrame(1);
    gpResourceManager->Dispose(boulderRef);
    gpMouseManager->ReallyShowPointer();
    WaitSample(sampleInfo);
}

// Unreferenced; reloads the armies and rebuilds the field before a full
// redraw.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0041bb8e, 0x43)
void combatManager::RegenerateField(void) {
    FreeArmies();
    LoadArmies();
    GenerateMap();
    SetGridMode(0);
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
    gpMouseManager->ReallyHidePointer();
    hisStack = &gpCombatManager->m_armies[COMBAT_ATTACKER_SIDE][pickIndex];
    hisCol = hisStack->m_hex % COMBAT_GRID_COLUMNS;
    hisRow = hisStack->m_hex / COMBAT_GRID_COLUMNS;
    originalColumn = COMBAT_GRID_LAST_COLUMN;
    originalRow = 0;
    gpCombatManager->SetGridMode(0);
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
    landX = gpCombatManager->m_hexCells[hisRow * COMBAT_GRID_COLUMNS + aimColumn].m_x;
    landY = gpCombatManager->m_hexCells[hisRow * COMBAT_GRID_COLUMNS + aimColumn].m_y - 75;
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
    backing->GrabBitmap(gpWindowManager->m_screen, inFlightX, inFlightY);
    lastX = inFlightX;
    lastY = inFlightY;
    for (k = 0; k < flightSteps * 3; k++) {
        clipLeft = inFlightX;
        clipTop = lastY;
        maxX = lastX + projectileSizeX;
        maxY = projectileSizeY + inFlightY;
        backing->DrawToBuffer(lastX, lastY);
        backing->GrabBitmap(gpWindowManager->m_screen, inFlightX, inFlightY);
        m_combatIcons[COMBAT_ICON_KEEP]->DrawToBuffer(
            inFlightX,
            inFlightY,
            arrowFrame + 1,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
        UPDATE_INCLUSIVE_REGION(clipLeft, clipTop, maxX, maxY);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 10;
        lastX = inFlightX;
        lastY = inFlightY;
        inFlightX += gainX;
        inFlightY += gainY;
    }
    backing->DrawToBuffer(lastX, lastY);
    gpWindowManager->UpdateScreenRegion(lastX, lastY, projectileSizeX, projectileSizeY);
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
    arrowDamage = static_cast<i32>(arrowDamage * gBattleStat[mod + 20]);
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
    gpCombatManager->CombatMessage(gText, 1);
    hisStack->PowEffect(hisStack->m_stats.powEffect);
    if (!(hisStack->m_stats.attributes & MONSTER_FLAGS_DEAD))
        hisStack->Stand(0);
    WaitSample(sample);
    if (hisStack->m_quantity > 0)
        hisStack->Stand(1);
    gpMouseManager->ReallyShowPointer();
}

// ExperienceValueOfStack: fight value of the side's losses, plus 500 for a
// defeated hero.
VA(0x0041c6b4, 0xec)
i32 combatManager::ExperienceValueOfStack(i8 side) {
    i32 i;
    i32 num;

    num = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[side][i].m_creatureType != CREATURE_NONE)
            num += (m_armies[side][i].m_initialQuantity - m_armies[side][i].m_quantity)
                   * gMonsterDatabase[m_armies[side][i].m_creatureType].hitPoints;
    }
    if (m_heroes[side])
        num += 500;
    return num;
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
        gpWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT,
        LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT)
    );
}

// HoMM1 DrawCombatBorder: put the saved rows back.
VA(0x0041c87d, 0x42)
void combatManager::DrawCombatBorder(void) {
    if (!m_savedBorder)
        return;
    memcpy(
        gpWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT,
        m_savedBorder,
        LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT)
    );
}

VA_COMPGEN(0x0041c900, 0x27, "??_H@YGXPAXIHP6EX0@Z@Z", 0x00418b30)
