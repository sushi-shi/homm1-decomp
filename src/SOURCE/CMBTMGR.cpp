// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/soundManager.h>
#include <BASE/soundmgr.h>
#include <BASE/WINMGR_TYPES.h>
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
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CheckApplyGoodMorale grants one extra turn at a time.
DATA(0x00490d50)
int bInHighMoraleBonus = 0;
// SetupCombat saves the adventure random seed here; GenerateMap restores it.
DATA(0x00490d54)
int giSeed = 1;

// Buka CMBTMGR.cpp combatManager(); HoMM1 keeps no message buffers.
VA(0x0044b440, 0x1b8)
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

// donor PoL RVA 0x0008ff0a; preferred Buka symbol ?CombineGroups@combatManager@@QAEXPAVarmyGroup@@0@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.491936;margin=0.502339;shape=0.296;size=0.801;calls=1.000;alternate=pol20:void combatManager::CombineGroups(class armyGroup *, class armyGroup *)@0x0008ff0a
VA(0x0044b5f8, 0x138)
void combatManager::CombineGroups(armyGroup* from, armyGroup* to) {
    short i;
    short j;

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

// Buka CMBTMGR.cpp SetupCombat; HoMM1's attacker is side 1.
VA(0x0044b730, 0x3db)
void combatManager::SetupCombat(
    int mapX,
    int mapY,
    hero* attackerHero,
    armyGroup* attackerGroup,
    town* defenderTown,
    hero* defenderHero,
    armyGroup* defenderGroup,
    int combatX,
    int combatY,
    int randomSeed
) {
    int i;

    giSeed = randomSeed;
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
        if (i == COMBAT_ATTACKER_SIDE)
            m_heroes[i] = attackerHero;
        else
            m_heroes[i] = defenderHero;
        if (m_heroes[i])
            m_heroType[i] = m_heroes[i]->m_heroClass;
        else
            m_heroType[i] = COMBAT_HERO_TYPE_NONE;
        if (i == COMBAT_ATTACKER_SIDE)
            m_armyGroups[i] = attackerGroup;
        else
            m_armyGroups[i] = defenderGroup;
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
        if (defenderTown->m_buildings & (1 << BUILDING_SLOT_CASTLE))
            m_castleSide[COMBAT_DEFENDER_SIDE] = 1;
        else
            m_castleSide[COMBAT_DEFENDER_SIDE] = 0;
        m_combatTowns[COMBAT_DEFENDER_SIDE] = defenderTown;
        m_originalCombatTown = m_combatTowns[COMBAT_DEFENDER_SIDE];
    } else {
        m_castleSide[COMBAT_DEFENDER_SIDE] = 0;
        m_combatTowns[COMBAT_DEFENDER_SIDE] = NULL;
    }
    m_combatTowns[COMBAT_ATTACKER_SIDE] = NULL;
}

// Buka CMBTMGR.cpp Open: screen buffer, combat window, icons, armies and
// field, then the fade-in and a random combat theme.
VA(0x0044bb0b, 0x40e)
short combatManager::Open(short priority) {
    int song;
    SAMPLE2 sample;
    int musicList[4];

    m_messageTypeMask = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                        | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                        | MESSAGE_WIDGET;
    m_combatWindowOpen = 0;
    m_savedBorder = NULL;
    gpSoundManager->PlayAmbientMusic(MUSIC_TRACK_NONE, 0, SOUND_VOLUME_FROM_CONFIG);
    m_backgroundBuffer = new bitmap(BITMAP_TYPE_NONE, LOGICAL_SCREEN_WIDTH, COMBAT_VIEW_HEIGHT);
    m_backgroundDrawn = 0;
    sample = NULL_SAMPLE2;
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
    gbLimitedCombatUpdatePalette = 1;
    gpMouseManager->NewUpdate(1);
    gpMouseManager->WarpPointer(
        m_hexCells[m_limitCreatureHex].m_x,
        m_hexCells[m_limitCreatureHex].m_y - 50
    );
    gpMouseManager->ReallyShowPointer();
    ResetMouse();
    m_gridSelectionDisabled = 0;
    WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
    musicList[0] = MUSIC_TRACK_BATTLE_2;
    musicList[1] = MUSIC_TRACK_BATTLE_3;
    musicList[2] = MUSIC_TRACK_BATTLE_1;
    musicList[3] = MUSIC_TRACK_BATTLE_4;
    song = musicList[SRandom(0, 3)];
    gpSoundManager->SwitchAmbientMusic(song);
    m_messageMask = MESSAGE_WIDGET;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "combatManager");
    return BASE_MANAGER_SUCCESS;
}

// cCombatBkgNames rows: GetBackgroundName picks one per terrain (forest or
// mountain variant by MoreTreesNear), the boat for water and the graveyard.
H1_ENUM_BEGIN(CombatBackground)
    COMBAT_BACKGROUND_GRASS_FOREST = 0,
    COMBAT_BACKGROUND_GRASS_MOUNTAIN = 1,
    COMBAT_BACKGROUND_SNOW_FOREST = 2,
    COMBAT_BACKGROUND_SNOW_MOUNTAIN = 3,
    COMBAT_BACKGROUND_SWAMP = 4,
    COMBAT_BACKGROUND_LAVA = 5,
    COMBAT_BACKGROUND_DESERT = 6,
    COMBAT_BACKGROUND_DIRT_FOREST = 7,
    COMBAT_BACKGROUND_DIRT_MOUNTAIN = 8,
    COMBAT_BACKGROUND_BOAT = 9,
    COMBAT_BACKGROUND_GRAVEYARD = 10,
    COMBAT_BACKGROUND_COUNT = 11
H1_ENUM_END(CombatBackground)

// CMBTMGR owns retail .data 0x00490d50-0x00491057. Retail emits the backdrop
// table after Open's literals, followed by its own literals.
DATA(0x00490db0)
char* cCombatBkgNames[COMBAT_BACKGROUND_COUNT] = {
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

// Buka CMBTMGR.cpp Close; a wandering-monster cell keeps the surviving
// count of the side that held it.
VA(0x0044bf19, 0x1ea)
void combatManager::Close(void) {
    int i;
    int survivor;

    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NONE);
    DrawCombatBorder();
    gbLimitedCombatUpdatePalette = 0;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    delete m_backgroundBuffer;
    for (i = 0; i < COMBAT_SIDE_COUNT; i++)
        UpdateArmyGroup(i);
    if (m_battlefieldCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
        survivor = static_cast<signed char>(m_playerId[COMBAT_DEFENDER_SIDE] != GAME_PLAYER_NONE);
        m_battlefieldCell->m_objectMetadata = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_armyGroups[survivor]->m_creatureTypes[i] != CREATURE_NONE)
                m_battlefieldCell->m_objectMetadata += m_armyGroups[survivor]->m_creatureCounts[i];
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

// Buka CMBTMGR.cpp UpdateArmyGroup: copy surviving counts back into the
// side's army group; a dead stack empties its slot.
VA(0x0044c103, 0x161)
void combatManager::UpdateArmyGroup(signed char side) {
    short i;
    short j;

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

// Buka CMBTMGR.cpp GenerateMap; HoMM1 also places both armies, scatters
// ground patches and, outside a siege, up to two obstacles.
VA(0x0044c264, 0x7be)
void combatManager::GenerateMap(void) {
    short x;
    short i;
    short y;
    short count;
    int randomRow;
    int randomCol;
    short armyCount;

    if (m_castleSide[COMBAT_DEFENDER_SIDE] == 1)
        m_catapultFrame[COMBAT_ATTACKER_SIDE] = 0;
    else
        m_catapultFrame[COMBAT_ATTACKER_SIDE] = COMBAT_CATAPULT_FRAME_NONE;
    if (m_castleSide[COMBAT_ATTACKER_SIDE] == 1)
        m_catapultFrame[COMBAT_DEFENDER_SIDE] = 0;
    else
        m_catapultFrame[COMBAT_DEFENDER_SIDE] = COMBAT_CATAPULT_FRAME_NONE;
    for (y = 0; y < COMBAT_GRID_ROWS; y++) {
        for (x = 0; x < COMBAT_GRID_COLUMNS; x++) {
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_y = y * COMBAT_HEX_HEIGHT + COMBAT_HEX_ORIGIN_Y;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_x = ((y & 1) ? 27 : -12) + x * COMBAT_HEX_WIDTH;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundIcon = COMBAT_ICON_GROUND;
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame =
                static_cast<signed char>(SRandom(0, 3)) + 4;
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
        randomRow = SRandom(0, 4);
        randomCol = SRandom(1, 7);
        m_hexCells[randomRow * COMBAT_GRID_COLUMNS + randomCol].m_groundFrame =
            static_cast<signed char>(SRandom(0, 2)) + 8;
    }
    if (m_castleSide[COMBAT_DEFENDER_SIDE]) {
        for (x = 6; x < 8; x++) {
            for (y = 0; y < COMBAT_GRID_ROWS; y++) {
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundIcon = COMBAT_ICON_CASTLE;
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_groundFrame = 4;
            }
        }
        for (y = 0; y < COMBAT_GRID_ROWS; y++) {
            m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_obstacleType = COMBAT_ICON_CASTLE;
            m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_obstacleIndex = COMBAT_WALL_INTACT;
        }
    }
    armyCount = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armyGroups[COMBAT_ATTACKER_SIDE]->m_creatureTypes[i] != CREATURE_NONE) {
            m_armies[COMBAT_ATTACKER_SIDE][armyCount].m_hex = i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN;
            m_armies[COMBAT_ATTACKER_SIDE][armyCount].m_stats.attributes &=
                MONSTER_FLAGS_BATTLE_START_MASK;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN].m_occupantSide = COMBAT_ATTACKER_SIDE;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN].m_occupantIndex = armyCount;
            if (m_armies[COMBAT_ATTACKER_SIDE][armyCount].m_stats.attributes & MONSTER_FLAGS_WIDE) {
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1].m_occupantSide = COMBAT_ATTACKER_SIDE;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1].m_occupantIndex = armyCount;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN].m_occupantFrame = ARMY_FACING_LEFT;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_FIRST_INNER_COLUMN + 1].m_occupantFrame = ARMY_FACING_RIGHT;
            }
            armyCount++;
        }
    }
    armyCount = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armyGroups[COMBAT_DEFENDER_SIDE]->m_creatureTypes[i] != CREATURE_NONE) {
            m_armies[COMBAT_DEFENDER_SIDE][armyCount].m_hex = i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN;
            m_armies[COMBAT_DEFENDER_SIDE][armyCount].m_stats.attributes &=
                MONSTER_FLAGS_BATTLE_START_MASK;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN].m_occupantSide = COMBAT_DEFENDER_SIDE;
            m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN].m_occupantIndex = armyCount;
            if (m_armies[COMBAT_DEFENDER_SIDE][armyCount].m_stats.attributes & MONSTER_FLAGS_WIDE) {
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN - 1].m_occupantSide = COMBAT_DEFENDER_SIDE;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN - 1].m_occupantIndex = armyCount;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN - 1].m_occupantFrame = ARMY_FACING_LEFT;
                m_hexCells[i * COMBAT_GRID_COLUMNS + COMBAT_GRID_LAST_INNER_COLUMN].m_occupantFrame = ARMY_FACING_RIGHT;
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
                && m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex == 2)
                m_hexCells[y * COMBAT_GRID_COLUMNS + x].m_obstacleIndex = 0;
        }
    }
    m_currentSide = COMBAT_DEFENDER_SIDE;
    m_currentSpeed = CREATURE_SPEED_BLAZING;
    GetNextArmy(0);
    m_gridUpdateRow = 0;
    SRand(giSeed);
}

// Buka CMBTMGR.cpp GetBackgroundName; a graveyard (or a hero standing on
// one) forces the graveyard field.
VA(0x0044ca22, 0x18e)
char* combatManager::GetBackgroundName(void) {
    if ((m_battlefieldCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_GRAVEYARD
        || ((m_battlefieldCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_HERO
            && (gpGame->GetHero(m_battlefieldCell->m_objectMetadata)->m_locationType
                & MAP_TRIGGER_TYPE_MASK)
                   == MAP_OBJECT_GRAVEYARD)) {
        m_terrainType = TERRAIN_DIRT;
        return cCombatBkgNames[COMBAT_BACKGROUND_GRAVEYARD];
    }
    switch (m_terrainType) {
        case TERRAIN_WATER:
            return cCombatBkgNames[COMBAT_BACKGROUND_BOAT];
        case TERRAIN_SWAMP:
            return cCombatBkgNames[COMBAT_BACKGROUND_SWAMP];
        case TERRAIN_LAVA:
            return cCombatBkgNames[COMBAT_BACKGROUND_LAVA];
        case TERRAIN_DESERT:
            return cCombatBkgNames[COMBAT_BACKGROUND_DESERT];
        case TERRAIN_GRASS:
            if (MoreTreesNear())
                return cCombatBkgNames[COMBAT_BACKGROUND_GRASS_FOREST];
            else
                return cCombatBkgNames[COMBAT_BACKGROUND_GRASS_MOUNTAIN];
        case TERRAIN_SNOW:
            if (MoreTreesNear())
                return cCombatBkgNames[COMBAT_BACKGROUND_SNOW_FOREST];
            else
                return cCombatBkgNames[COMBAT_BACKGROUND_SNOW_MOUNTAIN];
        case TERRAIN_DIRT:
            if (MoreTreesNear())
                return cCombatBkgNames[COMBAT_BACKGROUND_DIRT_FOREST];
            else
                return cCombatBkgNames[COMBAT_BACKGROUND_DIRT_MOUNTAIN];
    }
    return cCombatBkgNames[COMBAT_BACKGROUND_GRASS_FOREST];
}

// Buka CMBTMGR.cpp MoreTreesNear: tree (9) against mountain (8) objects
// within two cells of the battle.
VA(0x0044cbb0, 0x1e7)
signed char combatManager::MoreTreesNear(void) {
    int yPos;
    int xPos;
    short step;
    short homeX;
    signed char typeTable[3][MAP_DIRECTION_COUNT];
    short numMountains;
    short numTrees;
    mapCell* nearCell;
    short homeY;
    unsigned char nearbyTileset;
    short k;

    memset(typeTable, -1, sizeof(typeTable));
    homeX = m_combatX;
    homeY = m_combatY;
    for (step = 0; step < 3; step++) {
        for (k = 0; k < MAP_DIRECTION_COUNT; k++) {
            xPos = normalDirTable[k].x * step + homeX;
            yPos = normalDirTable[k].y * step + homeY;
            if (xPos >= 0 && xPos < MAP_CELL_GRID_SIZE && yPos >= 0 && yPos < MAP_CELL_GRID_SIZE) {
                nearCell = gpAdvManager->GetCell(xPos, yPos);
                nearbyTileset = nearCell->m_objectTileset & MAP_CELL_TILESET_MASK;
                if (nearbyTileset == TILESET_MTN32)
                    typeTable[step][k] = 0;
                else if (nearbyTileset == TILESET_TREE32)
                    typeTable[step][k] = 1;
            }
        }
    }
    numTrees = 0;
    numMountains = 0;
    for (step = 0; step < 3; step++) {
        for (k = 0; k < MAP_DIRECTION_COUNT; k++) {
            if (typeTable[step][k] == 0)
                numMountains++;
            if (typeTable[step][k] == 1)
                numTrees++;
        }
    }
    if (numTrees > numMountains)
        return 1;
    return 0;
}

// Buka CMBTMGR.cpp LoadIcons.
VA(0x0044cd97, 0x1d7)
void combatManager::LoadIcons(void) {
    int i;

    for (i = 0; i < COMBAT_ICON_COUNT; i++)
        m_combatIcons[i] = NULL;
    m_combatIcons[COMBAT_ICON_SPELLS] = gpResourceManager->GetIcon("spells.icn");
    m_backgroundBitmap = gpResourceManager->GetBitmap(GetBackgroundName());
    m_combatIcons[COMBAT_ICON_GROUND] = gpResourceManager->GetIcon(cCombatGroundNames[m_terrainType]);
    m_combatIcons[COMBAT_ICON_OBSTACLES] = gpResourceManager->GetIcon(cCombatObstacleNames[m_terrainType]);
    m_combatIcons[COMBAT_ICON_TEXTBAR] = gpResourceManager->GetIcon("textbar.icn");
    m_combatIcons[COMBAT_ICON_TENT] = gpResourceManager->GetIcon("tent.icn");
    m_combatIcons[COMBAT_ICON_CLOUD] = gpResourceManager->GetIcon("cloud.icn");
    if (m_castleSide[COMBAT_ATTACKER_SIDE] || m_castleSide[COMBAT_DEFENDER_SIDE]) {
        m_combatIcons[COMBAT_ICON_CATAPULT] = gpResourceManager->GetIcon("catapult.icn");
        sprintf(
            gText,
            "castle%02d.icn",
            m_combatTowns[static_cast<signed char>(m_castleSide[COMBAT_ATTACKER_SIDE] == 1)]->m_type
        );
        m_combatIcons[COMBAT_ICON_CASTLE] = gpResourceManager->GetIcon(gText);
        sprintf(gText, "keep%02d.icn", m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type);
        m_combatIcons[COMBAT_ICON_KEEP] = gpResourceManager->GetIcon(gText);
    }
}

// Buka CMBTMGR.cpp FreeIcons.
VA(0x0044cf6e, 0x7b)
void combatManager::FreeIcons(void) {
    short i;

    for (i = 0; i < COMBAT_ICON_COUNT; i++) {
        if (m_combatIcons[i])
            gpResourceManager->Dispose(m_combatIcons[i]);
    }
    gpResourceManager->Dispose(m_backgroundBitmap);
}

// Buka CMBTMGR.cpp LoadArmies; HoMM1 places stacks itself after Init.
VA(0x0044cfe9, 0x287)
void combatManager::LoadArmies(void) {
    short j;
    short i;

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

// Buka CMBTMGR.cpp FreeArmies; HoMM1 frees the defenders first.
VA(0x0044d270, 0xdc)
void combatManager::FreeArmies(void) {
    short i;

    gpSoundManager->StopAllSamples();
    for (i = 0; i < m_numArmies[COMBAT_ATTACKER_SIDE]; i++)
        m_armies[COMBAT_ATTACKER_SIDE][i].FreeResources();
    for (i = 0; i < m_numArmies[COMBAT_DEFENDER_SIDE]; i++)
        m_armies[COMBAT_DEFENDER_SIDE][i].FreeResources();
    if (gCurLoadedSpellIcon)
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
}

// HoMM1 retail 0x0044d34c: no callers and an empty body; HoMM2's combat log
// for unshown battles is the nearest one-argument fit.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0044d34c, 0x18)
void combatManager::NoShowCombatLog(char*) {}

// Buka CMBTMGR.cpp GetGridIndex over HoMM1's 9x5 grid: rows 80 pixels high
// from y 60, odd rows indented by 66 and even rows by 27, hexes 78 wide.
VA(0x0044d364, 0xbe)
short combatManager::GetGridIndex(short x, short y) {
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

// Buka CMBTMGR.cpp CheckApplyGoodMorale; HoMM1 rolls the group's morale.
VA(0x0044d422, 0x1d9)
void combatManager::CheckApplyGoodMorale(int side, int index) {
    armyGroup* theGroup;
    army* activeArmy;
    SAMPLE2 sample;
    int morale;

    if (side < 0 || index < 0)
        return;
    if (bInHighMoraleBonus) {
        bInHighMoraleBonus = 0;
        return;
    }
    bInHighMoraleBonus = 0;
    theGroup = m_armyGroups[side];
    activeArmy = &m_armies[side][index];
    if (!activeArmy->m_quantity)
        return;
    morale = theGroup->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (morale <= 0 || SRandom(1, 24) > morale)
        return;
    bInHighMoraleBonus = 1;
    sprintf(gText, "goodmrle.82M");
    sample = LoadPlaySample(gText);
    if (activeArmy->m_quantity <= 1)
        sprintf(
            gText,
            "High morale enables the %s to attack again.",
            gArmyNames[activeArmy->m_creatureType]
        );
    else
        sprintf(
            gText,
            "High morale enables the %s to attack again.",
            gArmyNamesPlural[activeArmy->m_creatureType]
        );
    CombatMessage(gText, 1);
    activeArmy->SpellEffect(COMBAT_EFFECT_GOOD_MORALE, 180);
    activeArmy->Stand(1);
    if (activeArmy->m_stats.attributes & MONSTER_FLAGS_TURN_SPENT)
        activeArmy->m_stats.attributes -= MONSTER_FLAGS_TURN_SPENT;
    activeArmy->m_stats.attributes |= MONSTER_FLAGS_HIGH_MORALE;
    WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
}

// Buka CMBTMGR.cpp CheckApplyBadMorale; a computer side skips one roll
// in four.
VA(0x0044d5fb, 0x1c6)
int combatManager::CheckApplyBadMorale(int side, int index) {
    armyGroup* theGroup;
    army* activeArmy;
    SAMPLE2 sample;
    int morale;

    if (side < 0 || index < 0)
        return 0;
    theGroup = m_armyGroups[side];
    activeArmy = &m_armies[side][index];
    morale = theGroup->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (morale >= 0 || SRandom(1, 12) > -morale)
        return 0;
    if (!m_humanSide[side] && SRandom(1, 4) == 1)
        return 0;
    sample = NULL_SAMPLE2;
    sample = LoadPlaySample("BADMRLE.82M");
    if (activeArmy->m_quantity <= 1)
        sprintf(
            gText,
            "Low morale causes the %s to freeze in panic.",
            gArmyNames[activeArmy->m_creatureType]
        );
    else
        sprintf(
            gText,
            "Low morale causes the %s to freeze in panic.",
            gArmyNamesPlural[activeArmy->m_creatureType]
        );
    CombatMessage(gText, 1);
    activeArmy->m_animationFrame = 2;
    activeArmy->SpellEffect(COMBAT_EFFECT_BAD_MORALE, 180);
    activeArmy->Stand(1);
    activeArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
    WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
    return 1;
}

// Buka CMBTMGR.cpp GetNextArmy: the fastest unspent stack, alternating
// sides, high-morale stacks first.
VA(0x0044d7c1, 0x209)
signed char combatManager::GetNextArmy(int checkMorale) {
    army* pArmy;
    signed char iSpeed;
    int sideIter;
    short temp;
    signed char stackCounter;
    signed char stackSide;
    int bSkip;

    stackSide = m_currentSide;
    for (iSpeed = 0; iSpeed < 5; iSpeed++) {
        for (sideIter = 0; sideIter < COMBAT_SIDE_COUNT; sideIter++) {
            stackSide ^= 1;
            for (stackCounter = 0; stackCounter < m_numArmies[stackSide]; stackCounter++) {
                bSkip = 0;
                pArmy = &m_armies[stackSide][stackCounter];
                if ((pArmy->m_stats.attributes & (MONSTER_FLAGS_DEAD | MONSTER_FLAGS_TURN_SPENT))
                    || pArmy->m_spellEffect == SPELL_PARALYZE || pArmy->m_spellEffect == SPELL_BLIND
                    || (pArmy->m_stats.speed != m_currentSpeed
                        && !(pArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE)))
                    bSkip = 1;
                if (!bSkip && !iSpeed && !(pArmy->m_stats.attributes & MONSTER_FLAGS_HIGH_MORALE))
                    bSkip = 1;
                if (!bSkip && checkMorale && CheckApplyBadMorale(stackSide, stackCounter))
                    bSkip = 1;
                if (!bSkip)
                    break;
            }
            if (m_numArmies[stackSide] != stackCounter) {
                m_currentSide = stackSide;
                m_currentArmyIndex = stackCounter;
                GetControl();
                return 1;
            }
        }
        if (iSpeed) {
            m_currentSpeed--;
            if (!m_currentSpeed)
                m_currentSpeed = CREATURE_SPEED_BLAZING;
        }
    }
    GetControl();
    return 0;
}

// Buka CMBTMGR.cpp IsWinner: the other side surrendered, retreated or has
// no live stack left.
VA(0x0044d9ca, 0xd3)
signed char combatManager::IsWinner(signed char side) {
    signed char isWinner;
    short i;

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
VA(0x0044da9d, 0xd55)
void combatManager::CatAttack(signed char side) {
    short dx;
    icon* boulder;
    short summitX;
    short x;
    signed char col;
    short i;
    short frm;
    short dy;
    short y;
    short tgtY;
    short tgtX;
    short force;
    short startX;
    SAMPLE2 catSample;
    signed char wallsLeft;
    short startY;
    short summitY;

    if (!m_castleSide[COMBAT_DEFENDER_SIDE])
        return;
    catSample = NULL_SAMPLE2;
    if (side == COMBAT_ATTACKER_SIDE)
        col = COMBAT_CASTLE_WALL_COLUMN;
    else
        col = COMBAT_GRID_LAST_COLUMN - COMBAT_CASTLE_WALL_COLUMN;
    wallsLeft = 0;
    for (i = 0; i < COMBAT_GRID_ROWS; i++) {
        if (m_hexCells[i * COMBAT_GRID_COLUMNS + col].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
            wallsLeft = 1;
    }
    if (!wallsLeft)
        return;
    gpMouseManager->ReallyHidePointer();
    boulder = gpResourceManager->GetIcon("boulder.icn");
    sprintf(gText, "catsnd%02d.82M", 0);
    catSample = LoadPlaySample(gText);
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
    if ((m_hexCells[col + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_WALL_DAMAGED
         || m_hexCells[col + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
        && (m_hexCells[col + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_WALL_DAMAGED
            || m_hexCells[col + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE)) {
        m_catapultTarget = SRandom(0, 4);
        while (m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex
               == COMBAT_OBSTACLE_NONE)
            m_catapultTarget = SRandom(0, 4);
    } else if (m_hexCells[col + COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
        m_catapultTarget = 3;
    } else if (m_hexCells[col + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
        m_catapultTarget = 1;
    } else if (m_hexCells[col + COMBAT_GRID_COLUMNS].m_obstacleIndex != COMBAT_WALL_INTACT) {
        m_catapultTarget = 3;
    } else if (m_hexCells[col + 3 * COMBAT_GRID_COLUMNS].m_obstacleIndex != COMBAT_WALL_INTACT) {
        m_catapultTarget = 1;
    } else {
        m_catapultTarget = SRandom(0, 1);
        if (!m_catapultTarget)
            m_catapultTarget = 1;
        else
            m_catapultTarget = 3;
    }
    startX = 0x75;
    startY = 0x104;
    tgtX = m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_x;
    tgtY = m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_y - COMBAT_HEX_HEIGHT;
    frm = 0;
    x = startX;
    y = startY;
    if (!m_catapultTarget) {
        dx = (tgtX - startX) / 12;
        dy = (tgtY - startY) / 12;
        i = 0;
        while (i < 12) {
            m_redrawExtent = 1;
            if (i) {
                giMinExtentX = x - dx - 20;
                giMaxExtentX = x + 75;
                giMinExtentY = y - 75;
                giMaxExtentY = y + 75;
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
            boulder->DrawToBuffer(x, y, frm, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            gpWindowManager->UpdateScreenRegion(
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            x = dx + x;
            y = y + dy;
            frm++;
            frm %= 3;
            if (i < 2)
                m_catapultFrame[side]++;
            i++;
        }
    } else {
        summitX = (startX + tgtX) / 2;
        switch (m_catapultTarget) {
            case 1:
                summitY = 25;
                break;
            default:
                summitY = m_catapultTarget * 20 + 25;
                break;
        }
        dx = (summitX - startX) / 12;
        dy = (summitY - startY) / 78;
        for (i = 0; i < 12; i++) {
            m_redrawExtent = 1;
            if (i) {
                giMinExtentX = x - dx - 20;
                giMaxExtentX = x + 75;
                giMinExtentY = y - 75;
                giMaxExtentY = y + 75;
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
            boulder->DrawToBuffer(x, y, frm, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            gpWindowManager->UpdateScreenRegion(
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            x = dx + x;
            y = (12 - i) * dy + y;
            frm++;
            frm %= 3;
            if (i < 2)
                m_catapultFrame[side]++;
        }
        dx = (tgtX - x) / 8;
        dy = (tgtY - y) / 36;
        for (i = 1; i <= 8; i++) {
            m_redrawExtent = 1;
            giMinExtentX = x - dx - 20;
            giMaxExtentX = x + 75;
            giMinExtentY = y - 75;
            giMaxExtentY = y + 75;
            if (giMinExtentX < 0)
                giMinExtentX = 0;
            if (giMinExtentY < 0)
                giMinExtentY = 0;
            if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
                giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
            if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
                giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
            DrawFrame(0);
            boulder->DrawToBuffer(x, y, frm, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            gpWindowManager->UpdateScreenRegion(
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            x = dx + x;
            y = dy * i + y;
            frm++;
            frm %= 3;
        }
    }
    WaitEndSample(catSample, SAMPLE_WAIT_DEFAULT);
    sprintf(gText, "catsnd%02d.82M", 2);
    catSample = LoadPlaySample(gText);
    if (m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex
        == COMBAT_WALL_DAMAGED)
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex =
            COMBAT_WALL_DAMAGED_HIT;
    else
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex =
            COMBAT_WALL_INTACT_HIT;
    force = SRandom(0, 150);
    if (!gbHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        force -= 15;
    if (force < 30
        || m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex
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
            if (m_wallFrame == 5)
                m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex =
                    COMBAT_WALL_COLLAPSING;
            m_redrawExtent = 1;
            m_gridUpdateRow = m_catapultTarget - 2;
            if (m_gridUpdateRow < 0)
                m_gridUpdateRow = 0;
            DrawFrame(1);
            m_wallFrame++;
        }
        m_wallFrame = m_wallDamage = COMBAT_WALL_DAMAGE_NONE;
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex =
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
            if (m_wallFrame == 5)
                m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex =
                    COMBAT_WALL_DAMAGED_HIT;
            m_redrawExtent = 1;
            DrawFrame(1);
            m_wallFrame++;
        }
        m_hexCells[m_catapultTarget * COMBAT_GRID_COLUMNS + col].m_obstacleIndex =
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
    gpResourceManager->Dispose(boulder);
    gpMouseManager->ReallyShowPointer();
    WaitEndSample(catSample, SAMPLE_WAIT_DEFAULT);
}

// HoMM1 retail 0x0044e7f2: unreferenced; reloads the armies and rebuilds
// the field before a full redraw.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0044e7f2, 0x4e)
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
VA(0x0044e840, 0xb8b)
void combatManager::KeepAttack(void) {
    int mod;
    short minX;
    short minY;
    short lastX;
    short gapX;
    signed char hexCol;
    signed char keepY;
    float yAdvance;
    short lastY;
    signed char targetRow;
    signed char shotShape[45] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                                 1, 2, 0, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 1,
                                 1, 1, 1, 2, 2, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0};
    int bestRank;
    signed char srcCol;
    float yRun;
    short distance;
    float xAdvance;
    int targetIndex;
    SAMPLE2 sample;
    short updRight;
    short gapY;
    short w;
    short height;
    bitmap* behind;
    int i;
    int bestWorth;
    int power;
    short startX;
    float xRun;
    short maxY;
    short startY;
    short destX;
    int numLost;
    army* target;
    int priority;
    short frontCol;
    int hurt;
    signed char arrowFrame;
    int dice;
    short targetY;

    bestRank = -1;
    bestWorth = 0;
    targetIndex = COMBAT_ARMY_INDEX_NONE;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[COMBAT_ATTACKER_SIDE][i].IsAlive()) {
            target = &m_armies[COMBAT_ATTACKER_SIDE][i];
            if (target->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                priority = 2;
            else if (target->m_stats.attributes & MONSTER_FLAGS_FLYING)
                priority = 1;
            else
                priority = 0;
            power = gMonsterDatabase[target->m_creatureType].fightValue * target->m_quantity;
            if (priority > bestRank || (priority == bestRank && power > bestWorth)) {
                bestWorth = power;
                bestRank = priority;
                targetIndex = i;
            }
        }
    }
    if (targetIndex == COMBAT_ARMY_INDEX_NONE)
        return;
    gpMouseManager->ReallyHidePointer();
    target = &gpCombatManager->m_armies[COMBAT_ATTACKER_SIDE][targetIndex];
    hexCol = target->m_hex % COMBAT_GRID_COLUMNS;
    targetRow = target->m_hex / COMBAT_GRID_COLUMNS;
    srcCol = COMBAT_GRID_LAST_COLUMN;
    keepY = 0;
    gpCombatManager->SetGridMode(0);
    if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type == TOWN_TYPE_WARLOCK
        || m_combatTowns[COMBAT_DEFENDER_SIDE]->m_type == TOWN_TYPE_SORCERESS)
        sprintf(gText, "shoot15.82M");
    else
        sprintf(gText, "shoot01.82M");
    sample = NULL_SAMPLE2;
    sample = LoadPlaySample(gText);
    frontCol = hexCol;
    if (target->m_stats.attributes & MONSTER_FLAGS_WIDE) {
        if (target->m_facing == ARMY_FACING_LEFT)
            frontCol = frontCol - 1;
        else
            frontCol = frontCol + 1;
    }
    gapX = abs(frontCol - srcCol);
    gapY = abs(targetRow - keepY);
    distance = __max(gapX, gapY);
    arrowFrame = shotShape[target->m_hex];
    startX = 0x24d;
    startY = 0x19;
    destX = gpCombatManager->m_hexCells[targetRow * COMBAT_GRID_COLUMNS + frontCol].m_x;
    targetY = gpCombatManager->m_hexCells[targetRow * COMBAT_GRID_COLUMNS + frontCol].m_y - 75;
    xAdvance = static_cast<float>(destX - startX) / static_cast<float>(distance * 3);
    yAdvance = static_cast<float>(targetY - startY) / static_cast<float>(distance * 3);
    xRun = startX;
    yRun = startY;
    updRight = 0;
    minX = LOGICAL_SCREEN_WIDTH - 1;
    maxY = 0;
    minY = LOGICAL_SCREEN_HEIGHT - 1;
    if (arrowFrame == 0) {
        w = 0x43;
        height = 0x12;
    } else if (arrowFrame == 1) {
        w = 0x37;
        height = 0x2b;
    } else {
        w = 0x12;
        height = 0x43;
    }
    behind = new bitmap(BITMAP_TYPE_MEMORY, w, height);
    behind->GrabBitmap(gpWindowManager->m_screen, xRun, yRun);
    lastX = xRun;
    lastY = yRun;
    for (i = 0; i < distance * 3; i++) {
        minX = xRun;
        minY = lastY;
        updRight = lastX + w;
        maxY = height + yRun;
        behind->DrawToBuffer(lastX, lastY);
        behind->GrabBitmap(gpWindowManager->m_screen, xRun, yRun);
        m_combatIcons[COMBAT_ICON_KEEP]->DrawToBuffer(xRun, yRun, arrowFrame + 1, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
        DelayTil(glTimers);
        UPDATE_INCLUSIVE_REGION(minX, minY, updRight, maxY);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 10;
        lastX = xRun;
        lastY = yRun;
        xRun = xAdvance + xRun;
        yRun = yAdvance + yRun;
    }
    behind->DrawToBuffer(lastX, lastY);
    gpWindowManager->UpdateScreenRegion(lastX, lastY, w, height);
    delete behind;
    mod = 2;
    if (m_heroes[COMBAT_DEFENDER_SIDE])
        mod += m_heroes[COMBAT_DEFENDER_SIDE]->m_primaryStats[HERO_PRIMARY_ATTACK];
    if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings & 1)
        mod += m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildState + 1;
    mod -= -(-target->m_stats.defense);
    if (mod > 20)
        mod = 20;
    if (mod < -20)
        mod = -20;
    dice = 5;
    for (i = 7; i <= 12; i++) {
        if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings & (1 << i))
            dice += 4;
    }
    for (i = 0; i <= 4; i++) {
        if (m_combatTowns[COMBAT_DEFENDER_SIDE]->m_buildings & (1 << i))
            dice++;
    }
    hurt = 0;
    for (i = 0; i < dice; i++)
        hurt += SRandom(2, 3);
    hurt = static_cast<int>(hurt * gfBattleStat[mod + 20]);
    if (hurt <= 0)
        hurt = 1;
    numLost = target->Damage(hurt);
    if (numLost > 0)
        sprintf(
            gText,
            "%s %d %s. %d %s %s.",
            "Garrison does",
            hurt,
            "Damage",
            numLost,
            CREATURE_DISPLAY_NAME(target->m_creatureType, numLost),
            numLost > 1 ? "perish" : "perishes"
        );
    else
        sprintf(gText, "%s %d %s.", "Garrison does", hurt, "Damage");
    gpCombatManager->CombatMessage(gText, 1);
    target->PowEffect(target->m_stats.powEffect);
    if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
        target->Stand(0);
    WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
    if (target->m_quantity > 0)
        target->Stand(1);
    gpMouseManager->ReallyShowPointer();
}

// Buka CMBTMGR.cpp ExperienceValueOfStack: fight value of the side's
// losses, plus 500 for a defeated hero.
VA(0x0044f3cb, 0x114)
int combatManager::ExperienceValueOfStack(signed char side) {
    int i;
    int value;

    value = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[side][i].m_creatureType != CREATURE_NONE)
            value += (m_armies[side][i].m_initialQuantity - m_armies[side][i].m_quantity)
                     * gMonsterDatabase[m_armies[side][i].m_creatureType].hitPoints;
    }
    if (m_heroes[side])
        value += 500;
    return value;
}

// Buka CMBTMGR.cpp ResetHitByCreature.
VA(0x0044f4df, 0x78)
void combatManager::ResetHitByCreature(void) {
    int j;
    int i;

    for (i = 0; i < COMBAT_SIDE_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++)
            m_armies[i][j].m_hitByCreature = 0;
    }
}

// HoMM1's combat grid is nine columns by five rows.
VA(0x0044f557, 0x30)
int ValidHex(int hex) {
    return hex >= 0 && hex <= COMBAT_HEX_COUNT - 1;
}

// HoMM1 SaveCombatBorder: keep the twenty screen rows under the field.
VA(0x0044f587, 0x64)
void combatManager::SaveCombatBorder(void) {
    if (!m_savedBorder)
        m_savedBorder = static_cast<char*>(malloc(LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT)));
    memcpy(m_savedBorder, gpWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT,
           LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT));
}

// HoMM1 DrawCombatBorder: put the saved rows back.
VA(0x0044f5eb, 0x53)
void combatManager::DrawCombatBorder(void) {
    if (!m_savedBorder)
        return;
    memcpy(gpWindowManager->m_screen->m_pixels + LOGICAL_SCREEN_WIDTH * COMBAT_VIEW_HEIGHT, m_savedBorder,
           LOGICAL_SCREEN_WIDTH * (LOGICAL_SCREEN_HEIGHT - COMBAT_VIEW_HEIGHT));
}
