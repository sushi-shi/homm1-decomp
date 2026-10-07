// Regression tests for the retail out-of-bounds defects fixed in the shared
// game units (docs/port/divergences.md). The test links the native game
// without its entry point and starts it headless (SDL's dummy drivers) on
// the game data, like editor_maps_test. Most cases matter most under the
// sanitizers (ctest in a -DHOMM1_SANITIZERS=ON build), where the original
// code fails at once; each also checks the corrected result.
//
// Needs $HOMM1_DATA; without it the test is skipped (exit code 77).

#include <H1/Ints.h>

#include <BASE/Misc.h>
#include <BASE/bitmap.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <PLATFORM/File.h>
#include <PLATFORM/Records.h>
#include <SOURCE/KB.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/townManager.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen);

namespace {

int gFailures = 0;

void Expect(bool condition, const char* what) {
    std::printf("%s %s\n", condition ? "ok" : "FAIL", what);
    if (!condition)
        gFailures++;
}

void StartGame() {
    if (gExec->InitSystem())
        std::_Exit(1);
    gPalette = gResourceManager->GetPalette("kb.pal");
    PostprocessPalette(gPalette->m_data);
    SetPalette(gPalette->m_data, 1);
    gPhilAI->m_debugFont = gResourceManager->GetFont("smalfont.fnt");
    LoadSystemwideIcons();
    memset(gThisNetHumanPlayer, 0, GAME_PLAYER_COUNT);
}

// A new single-player game on a shipped map, as the new game window starts it.
void NewGame(const char* map) {
    gNumHumanPlayers = 1;
    gHumanPlayer[0] = 1;
    gGame->LoadGame(const_cast<char*>("origdata.bin"), 1, 0);
    gGame->m_playerCount = GAME_PLAYER_COUNT;
    for (int i = 1; i < GAME_PLAYER_COUNT; i++)
        gGame->m_players[i].m_difficulty = PLAYER_TYPE_AVERAGE;
    gGame->NewMap(const_cast<char*>(map));
}

// GiveArtifact recorded the spell book's owner one past the random artifact
// table, over the first boat's id.
void SpellBookKeepsBoatTable() {
    hero* bearer = &gGame->m_heroRecs[3];
    memset(bearer->m_artifacts, ARTIFACT_NONE, sizeof(bearer->m_artifacts));
    gGame->m_boats[0].id = 0x55;
    gAdvManager->GiveArtifact(bearer, ARTIFACT_MAGIC_BOOK);
    Expect(gGame->m_boats[0].id == 0x55, "the spell book does not overwrite the boat table");
    Expect(bearer->HasArtifact(ARTIFACT_MAGIC_BOOK) != 0, "the hero has the spell book");
}

// The ultimate artifact's value was read at index -1 once it was dug up.
void ObeliskValueWithoutUltimate() {
    i8 saved = gGame->m_ultimateArtifactId;
    gGame->m_ultimateArtifactId = ARTIFACT_NONE;
    Expect(gPhilAI->TurnValueOfObelisk(1) == 0.0f, "no obelisk value once the artifact is gone");
    gGame->m_ultimateArtifactId = saved;
    // A map whose header counts no obelisks divided the value by zero.
    i8 savedCount = gGame->m_obeliskCount;
    gGame->m_obeliskCount = 0;
    if (gGame->m_ultimateArtifactId == ARTIFACT_NONE)
        gGame->m_ultimateArtifactId = ARTIFACT_ULTIMATE_BOOK;
    Expect(gPhilAI->TurnValueOfObelisk(1) == 0.0f, "no obelisk value without obelisks");
    gGame->m_obeliskCount = savedCount;
    gGame->m_ultimateArtifactId = saved;
}

// A site on the top rows put its owner's flag outside the grid.
void MineFlagOnTopRow() {
    u8 before[sizeof(gGame->m_map)];
    memcpy(before, gGame->m_map, sizeof(before));
    i8 slot = GAME_MINE_COUNT - 1;
    mineRecord saved = gGame->m_mines[slot];
    gGame->m_mines[slot].x = 40;
    gGame->m_mines[slot].y = 1;
    gGame->m_mines[slot].type = RESOURCE_MERCURY;
    gGame->ClaimMine(slot, 0);
    Expect(gGame->m_mineOwners[slot] == 0, "a mine on row 1 is claimed");
    Expect(memcmp(before, gGame->m_map, sizeof(before)) == 0, "its flag is not written outside the map");
    gGame->m_mines[slot] = saved;
    gGame->m_mineOwners[slot] = GAME_PLAYER_NONE;
}

// BuildPath followed a stale route out of the grid and kept its steps.
void RouteLeavingTheGrid() {
    gSearchArray->Clear();
    searchNode& node = gSearchArray->m_cells[0][5];
    node.x = 0;
    node.y = 5;
    node.visited = 1;
    node.distance = 1;
    node.direction = MAP_DIRECTION_EAST;  // traced back: west, out of the map
    i32 length = gSearchArray->BuildPath(10, 10, 0, 5, 100);
    Expect(length == 0 && gSearchArray->m_pathLength == 0, "a route traced off the map fails empty");
}

// The map contents are checked when a game starts on them.
void MapChecks() {
    Expect(gGame->MapDataValid() != 0, "the started map is valid");
    mapCell saved = gGame->m_map[10][10];
    gGame->m_map[10][10].m_triggerType = MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER;
    gGame->m_map[10][10].m_objectIndex = CREATURE_COUNT + 3;
    Expect(gGame->MapDataValid() == 0, "a monster of no creature type is refused");
    gGame->m_map[10][10] = saved;
    gGame->m_map[10][10].m_tileIndex = MAP_CELL_GROUND_TILE_COUNT;
    Expect(gGame->MapDataValid() == 0, "a ground tile past the table is refused");
    gGame->m_map[10][10] = saved;
    gGame->m_map[10][10].m_triggerType = MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN;
    Expect(gGame->MapDataValid() == 0, "a town cell without a town record is refused");
    gGame->m_map[10][10] = saved;
    Expect(gGame->MapDataValid() != 0, "the map is valid again");
}

// Week turns with the empty tavern slots of players not in the game.
void WeekWithAbsentPlayers() {
    gGame->m_players[3].m_availableHeroIds[0] = HERO_ID_NONE;
    gGame->m_players[3].m_availableHeroIds[1] = HERO_ID_NONE;
    gGame->PerWeek();
    Expect(gGame->m_players[3].m_availableHeroIds[0] >= 0, "an absent player's tavern is drawn");
}

// Four-player campaign scenarios read a fourth crest from a three-entry table.
void CampaignCrests() {
    gNumHumanPlayers = 1;
    gHumanPlayer[0] = 1;
    gGame->InitEntireCampaign(1);
    bool distinct = true;
    for (int i = 0; i < gGame->m_playerCount; i++)
        for (int j = i + 1; j < gGame->m_playerCount; j++)
            distinct = distinct && gGame->m_players[i].m_color != gGame->m_players[j].m_color;
    Expect(distinct, "campaign crests are distinct");
}

// A word wider than a multi-line field: LineLength counted lines without
// end (the editor hung while typing a long word into the map description),
// and both it and DrawBoundedString read before the text on the first line.
void LongWords() {
    font* small = gResourceManager->GetFont(const_cast<char*>("smalfont.fnt"));
    char word[64];
    memset(word, 'W', sizeof(word) - 1);
    word[sizeof(word) - 1] = 0;
    i32 lines = small->LineLength(word, 40);
    Expect(lines > 1 && lines < 64, "a long word is counted over several lines");
    char sentence[] = "short WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW end";
    Expect(small->LineLength(sentence, 40) >= 3, "a long word inside a sentence is broken");
    small->DrawBoundedString(word, 10, 10, 40, 60, 0, 0);
    Expect(true, "a long word is drawn in a bounded box");
    gResourceManager->Dispose(small);
}


// A commander's Attack and Defense were added into the stack's 8-bit
// attack and defense, which wrapped negative above 127.
void CommanderStatsHoldInTheirByte() {
    hero* commander = &gGame->m_heroRecs[5];
    i8 savedAttack = commander->m_primaryStats[HERO_PRIMARY_ATTACK];
    i8 savedDefense = commander->m_primaryStats[HERO_PRIMARY_DEFENSE];
    hero* savedHero = gCombatManager->m_heroes[0];
    commander->m_primaryStats[HERO_PRIMARY_ATTACK] = 120;
    commander->m_primaryStats[HERO_PRIMARY_DEFENSE] = 120;
    gCombatManager->m_heroes[0] = commander;
    army* stack = new army;
    stack->Init(CREATURE_DRAGON, 1, 0, 0);
    Expect(stack->m_stats.attack == ARMY_STAT_MAX && stack->m_stats.defense == ARMY_STAT_MAX,
           "a commander's skill of 120 holds the stack's attack and defense at 127");
    delete stack;
    gCombatManager->m_heroes[0] = savedHero;
    commander->m_primaryStats[HERO_PRIMARY_ATTACK] = savedAttack;
    commander->m_primaryStats[HERO_PRIMARY_DEFENSE] = savedDefense;
}

// A berserk stack with nothing to attack: a flier drew hexes without end,
// a walker that drew a blocked direction was left without an action.
void BerserkWithNothingToAttack() {
    hexcell savedCells[COMBAT_HEX_COUNT];
    memcpy(savedCells, gCombatManager->m_hexCells, sizeof(savedCells));
    for (int i = 0; i < COMBAT_HEX_COUNT; i++) {
        gCombatManager->m_hexCells[i].m_occupantSide = COMBAT_SIDE_NONE;
        gCombatManager->m_hexCells[i].m_obstacleIndex = COMBAT_OBSTACLE_NONE;
    }
    // Hex 0 is a corner the flier's draws (1-43) never pick.
    gCombatManager->m_hexCells[0].m_occupantSide = 0;
    gCombatManager->m_hexCells[0].m_occupantIndex = 0;
    hero* savedHero = gCombatManager->m_heroes[0];
    gCombatManager->m_heroes[0] = nullptr;
    army* stack = new army;
    stack->Init(CREATURE_GRIFFIN, 5, 0, 0);
    stack->m_hex = 0;
    gNextAction = ACTION_NONE;
    stack->GoBerserk();
    Expect(gNextAction == ACTION_SKIP_TURN, "a berserk flier with nothing to reach waits");
    // A walker whose neighbours are all obstacles.
    stack->Init(CREATURE_PEASANT, 5, 0, 0);
    stack->m_hex = 0;
    for (int direction = COMBAT_DIRECTION_ADJACENT_FIRST; direction <= COMBAT_DIRECTION_ADJACENT_LAST;
         direction++) {
        i16 next = GetAdjacentCellIndexNoArmy(0, direction);
        if (ValidHex(next))
            gCombatManager->m_hexCells[next].m_obstacleIndex = 0;
    }
    gNextAction = ACTION_NONE;
    stack->GoBerserk();
    Expect(gNextAction == ACTION_SKIP_TURN, "a berserk walker that cannot move waits");
    delete stack;
    gCombatManager->m_heroes[0] = savedHero;
    memcpy(gCombatManager->m_hexCells, savedCells, sizeof(savedCells));
    gNextAction = ACTION_NONE;
}


// MaxBuyableCreatures kept only the last resource's (gold's) count.
void AffordableCreaturesNeedEveryResource() {
    i32 cost[RESOURCE_COUNT];
    i32 saved[RESOURCE_COUNT];
    int creature = CREATURE_DRAGON;
    GetMonsterCost(creature, cost);
    int scarce = RESOURCE_FIRST;
    while (scarce < RESOURCE_GOLD && cost[scarce] == 0)
        scarce++;
    gCurPlayerData = &gGame->m_players[0];
    memcpy(saved, gCurPlayerData->m_resources, sizeof(saved));
    for (int i = RESOURCE_FIRST; i < RESOURCE_COUNT; i++)
        gCurPlayerData->m_resources[i] = 1000;
    gCurPlayerData->m_resources[RESOURCE_GOLD] = 1000000;
    gCurPlayerData->m_resources[scarce] = 0;
    Expect(scarce < RESOURCE_GOLD && gPhilAI->MaxBuyableCreatures(creature) == 0,
           "a creature whose rare resource is missing is not affordable");
    memcpy(gCurPlayerData->m_resources, saved, sizeof(saved));
}

// The value of the stack a purchase replaces was read from the creature
// numbered like its slot.
void ReplacedStackIsTheWeakest() {
    hero* buyer = &gGame->m_heroRecs[6];
    armyGroup saved = buyer->m_army;
    const int types[ARMY_GROUP_SLOT_COUNT] = {CREATURE_DRAGON, CREATURE_GRIFFIN, CREATURE_UNICORN,
                                              CREATURE_CENTAUR, CREATURE_PEASANT};
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        buyer->m_army.m_creatureTypes[i] = types[i];
        buyer->m_army.m_creatureCounts[i] = 1;
    }
    i32 count = 0;
    i32 value = 0;
    i32 slot = -1;
    gPhilAI->EvaluateOneTimeCreaturePurchase(buyer, CREATURE_GOBLIN, 1, true, count, value, slot);
    Expect(slot == ARMY_GROUP_SLOT_COUNT - 1, "a purchase replaces the weakest stack (the peasant)");
    buyer->m_army = saved;
}

// ResetHeroRVs measured the y distance to a hero with the hero's x.
void HeroDistanceUsesY() {
    hero* far = &gGame->m_heroRecs[7];
    i16 savedX = far->m_x;
    i16 savedY = far->m_y;
    far->m_x = 10;
    far->m_y = 40;
    gHeroLiveChance[7] = 50;
    ResetHeroRVs(true, 10, 10);
    Expect(gHeroLiveChance[7] == 50, "a hero 30 cells away is not reset as near");
    far->m_x = savedX;
    far->m_y = savedY;
}

// BuildPath accepted a node from another cell when one coordinate matched.
void RouteThroughAStaleNode() {
    gSearchArray->Clear();
    searchNode& node = gSearchArray->m_cells[20][20];
    node.x = 20;
    node.y = 21;
    node.visited = 1;
    node.distance = 1;
    node.direction = MAP_DIRECTION_EAST;
    Expect(gSearchArray->BuildPath(19, 20, 20, 20, 100) == 0, "a node left from another cell ends the route");
}

// A site's random artifact for a hero with no free slot was lost.
void RandomArtifactWithoutAFreeSlot() {
    hero* bearer = &gGame->m_heroRecs[8];
    i8 savedArtifacts[HERO_ARTIFACT_SLOT_COUNT];
    memcpy(savedArtifacts, bearer->m_artifacts, sizeof(savedArtifacts));
    i8 savedOwner = bearer->m_owner;
    bearer->m_owner = 0;
    memset(bearer->m_artifacts, ARTIFACT_FIRST, sizeof(bearer->m_artifacts));
    i32 gold = gGame->m_players[0].m_resources[RESOURCE_GOLD];
    i32 artifact = gAdvManager->GiveRandomArtifact(&gAdvManager->m_mapData[30][30], bearer);
    Expect(artifact == ARTIFACT_NONE && gGame->m_players[0].m_resources[RESOURCE_GOLD] == gold + 1000,
           "a hero with every slot full is paid 1000 gold for a site's artifact");
    gGame->m_players[0].m_resources[RESOURCE_GOLD] = gold;
    memcpy(bearer->m_artifacts, savedArtifacts, sizeof(savedArtifacts));
    bearer->m_owner = savedOwner;
}


// The computer's campfire cleared the ambient sound at the view's centre
// instead of its own cell; a computer hero with every slot full lost the
// map artifacts it walked onto.
void ComputerCampfireAndFullHero() {
    hero* visitor = &gGame->m_heroRecs[9];
    i8 savedOwner = visitor->m_owner;
    i8 savedArtifacts[HERO_ARTIFACT_SLOT_COUNT];
    memcpy(savedArtifacts, visitor->m_artifacts, sizeof(savedArtifacts));
    visitor->m_owner = 1;
    i32 centreX = gAdvManager->m_mapOriginX + ADVMGR_VIEW_CENTER;
    i32 centreY = gAdvManager->m_mapOriginY + ADVMGR_VIEW_CENTER;
    int x = centreX < 36 ? 50 : 20;
    int y = 30;
    mapCell saved = gGame->m_map[x][y];
    i8 savedCentreSound = gGame->m_mapSounds[centreX][centreY];
    gGame->m_mapSounds[centreX][centreY] = 3;
    gGame->m_mapSounds[x][y] = 5;
    gGame->m_map[x][y].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_CAMPFIRE);
    gGame->m_map[x][y].m_objectMetadata = (2 << CAMPFIRE_AMOUNT_SHIFT) | RESOURCE_WOOD;
    gAdvManager->DoAIEvent(&gGame->m_map[x][y], visitor, x, y);
    Expect(gGame->m_mapSounds[centreX][centreY] == 3 && gGame->m_mapSounds[x][y] == MAP_SOUND_NONE,
           "a computer's campfire silences its own cell, not the view's centre");
    gGame->m_map[x][y] = saved;
    gGame->m_map[x][y].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT);
    gGame->m_map[x][y].m_objectIndex = ARTIFACT_FIRST + 1;
    gGame->m_map[x][y].m_objectMetadata = ARTIFACT_EVENT_MODE_PICKUP;
    memset(visitor->m_artifacts, ARTIFACT_FIRST, sizeof(visitor->m_artifacts));
    gAdvManager->DoAIEvent(&gGame->m_map[x][y], visitor, x, y);
    Expect(gGame->m_map[x][y].m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT),
           "a computer hero with every slot full leaves the artifact on the map");
    gGame->m_map[x][y] = saved;
    gGame->m_mapSounds[centreX][centreY] = savedCentreSound;
    memcpy(visitor->m_artifacts, savedArtifacts, sizeof(savedArtifacts));
    visitor->m_owner = savedOwner;
}

// Beside the map's east edge Summon Boat tried the cells past it, which
// GetCell answers with cell (0,0): with water there, the boat went to x 72.
void SummonBoatOnTheEdge() {
    const int x = MAP_CELL_GRID_SIZE - 1;
    const int y = 20;
    mapCell* map = &gGame->m_map[0][0];
    mapCell savedMap[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    memcpy(savedMap, gGame->m_map, sizeof(savedMap));
    boatRecord savedBoat = gGame->m_boats[0];
    i8 savedSlot = gGame->m_boatSlots[0];
    i16 savedOriginX = gAdvManager->m_mapOriginX;
    i16 savedOriginY = gAdvManager->m_mapOriginY;
    i8 savedHero = gGame->m_players[0].m_currentHero;
    for (int dx = -1; dx <= 0; dx++)
        for (int dy = -1; dy <= 1; dy++) {
            gGame->m_map[x + dx][y + dy].m_tileIndex = 2 * MAP_CELL_TILES_PER_TERRAIN;
            gGame->m_map[x + dx][y + dy].m_objectIndex = MAP_CELL_NO_FRAME;
        }
    gGame->m_map[x - 1][y].m_tileIndex = 0;
    map->m_tileIndex = 0;
    map->m_objectIndex = MAP_CELL_NO_FRAME;
    gGame->m_boatSlots[0] = 0;
    gGame->m_boats[0].heroId = static_cast<i8>(BOAT_OCCUPIED_FLAG);
    gGame->m_boats[0].owner = 0;
    gGame->m_boats[0].x = 60;
    gGame->m_boats[0].y = 60;
    gCurPlayer = 0;
    gCurPlayerData = &gGame->m_players[0];
    gCurPlayerData->m_currentHero = gCurPlayerData->m_heroIds[0];
    gAdvManager->m_mapOriginX = x - ADVMGR_VIEW_CENTER;
    gAdvManager->m_mapOriginY = y - ADVMGR_VIEW_CENTER;
    b32 savedShowIt = gShowIt;
    gShowIt = false;
    gAdvManager->SummonBoat();
    gShowIt = savedShowIt;
    Expect(gGame->m_boats[0].x == x - 1 && gGame->m_boats[0].y == y,
           "Summon Boat on the map's edge puts the boat on the water beside the hero");
    memcpy(gGame->m_map, savedMap, sizeof(savedMap));
    gGame->m_boats[0] = savedBoat;
    gGame->m_boatSlots[0] = savedSlot;
    gAdvManager->m_mapOriginX = savedOriginX;
    gAdvManager->m_mapOriginY = savedOriginY;
    gGame->m_players[0].m_currentHero = savedHero;
}

// PNM31234 keeps towns without records under two monsters; once a monster
// was gone, its cell showed and selected town 0. The triggers stay as the
// map has them; such a cell has no record, and its lookups are left out.
void StrayTownsUnderMonsters() {
    NewGame("PNM31234.MAP");
    const int cells[][2] = {{7, 45}, {33, 61}};
    bool kept = true;
    bool recordless = true;
    for (const auto& c : cells) {
        mapCell* cell = &gGame->m_map[c[0]][c[1]];
        gAdvManager->EraseObj(cell, c[0], c[1]);
        kept = kept && MAP_TRIGGER_OBJECT(cell->m_triggerType) == MAP_OBJECT_TOWN;
        recordless = recordless && !gGame->CellHasRecord(c[0], c[1]);
    }
    Expect(kept, "a defeated monster on PNM31234 uncovers the town trigger the map keeps");
    Expect(recordless, "the uncovered town cells have no town record");
    bool towns = true;
    for (int i = 0; i < GAME_TOWN_COUNT; i++) {
        const town& t = gGame->m_castleRecs[i];
        if (MAP_CELL_IN_BOUNDS(t.m_x, t.m_y)
            && MAP_TRIGGER_OBJECT(gGame->m_map[t.m_x][t.m_y].m_triggerType) == MAP_OBJECT_TOWN)
            towns = towns && gGame->CellHasRecord(t.m_x, t.m_y)
                    && gGame->CellHasRecord(t.m_x - 1, t.m_y - 1);
    }
    bool heroes = true;
    for (int i = 0; i < GAME_HERO_COUNT; i++) {
        const hero& h = gGame->m_heroRecs[i];
        if (h.m_owner >= 0 && MAP_CELL_IN_BOUNDS(h.m_x, h.m_y)
            && MAP_TRIGGER_OBJECT(gGame->m_map[h.m_x][h.m_y].m_triggerType) == MAP_OBJECT_HERO)
            heroes = heroes && gGame->CellHasRecord(h.m_x, h.m_y);
    }
    Expect(towns && heroes, "real town and hero cells keep their records");
    NewGame("AES31000.MAP");
}


// The campaign's crests were read one entry late: the enemy lord of
// scenarios 5-8 never got the crest the scenario names.
void CampaignLordCrest() {
    i32 savedType = gGame->m_campaignType;
    i32 savedScenario = gGame->m_campaignScenario;
    i8 savedCount = gGame->m_playerCount;
    i16 savedColors[GAME_PLAYER_COUNT];
    for (int i = 0; i < GAME_PLAYER_COUNT; i++)
        savedColors[i] = gGame->m_players[i].m_color;
    bool named = true;
    for (int scenario = CAMPAIGN_SCENARIO_5; scenario <= CAMPAIGN_SCENARIO_8; scenario++) {
        gGame->m_campaignType = CAMPAIGN_IRONFIST;
        gGame->m_campaignScenario = scenario;
        gGame->m_playerCount = GAME_PLAYER_COUNT;
        i16 lord = gCampaignScenarios[scenario].playerCrests[0];
        gGame->m_players[0].m_color = lord == PLAYER_COLOR_BLUE ? PLAYER_COLOR_GREEN : PLAYER_COLOR_BLUE;
        gGame->RandomizePlayerCrests();
        named = named && gGame->m_players[1].m_color == lord;
    }
    Expect(named, "the enemy lord of campaign scenarios 5-8 has the scenario's crest");
    gGame->m_campaignType = savedType;
    gGame->m_campaignScenario = savedScenario;
    gGame->m_playerCount = savedCount;
    for (int i = 0; i < GAME_PLAYER_COUNT; i++)
        gGame->m_players[i].m_color = savedColors[i];
}


// ComputeUALoc tested player > 0: the first player never had the hint a
// computer player digs at.
void FirstPlayerUltimateHint() {
    playerData& first = gGame->m_players[0];
    playerData saved = first;
    i8 savedArtifact = gGame->m_ultimateArtifactId;
    if (gGame->m_ultimateArtifactId == ARTIFACT_NONE)
        gGame->m_ultimateArtifactId = ARTIFACT_ULTIMATE_BOOK;
    memset(first.m_puzzlePiecesRemoved, 0xFF, sizeof(first.m_puzzlePiecesRemoved));
    first.m_ultimateArtifactHintChance = 0;
    ComputeUALoc(0);
    Expect(first.m_ultimateArtifactHintChance > 0
               && first.m_ultimateArtifactHintX != PLAYER_ULTIMATE_HINT_NONE,
           "the first player gets the ultimate artifact hint");
    first = saved;
    gGame->m_ultimateArtifactId = savedArtifact;
}

// The "built today" flags held 32 towns: a build in towns 32-35 set bits
// of the first hero's id, which were never cleared.
void TownFlagsForEveryTown() {
    town* last = &gGame->m_castleRecs[GAME_TOWN_COUNT - 1];
    town saved = *last;
    i32 resources[RESOURCE_COUNT];
    memcpy(resources, gCurPlayerData->m_resources, sizeof(resources));
    i8 heroId = gGame->m_heroRecs[0].m_id;
    last->m_id = GAME_TOWN_COUNT - 1;
    memset(gGame->m_townBuiltToday, 0, sizeof(gGame->m_townBuiltToday));
    gPhilAI->BuildBuilding(last, BUILDING_SLOT_WELL);
    Expect(gGame->m_heroRecs[0].m_id == heroId, "a build in town 35 leaves the first hero's id");
    Expect(!CanBuild(last, BUILDING_SLOT_TAVERN), "town 35 has built today");
    memset(gGame->m_townBuiltToday, 0, sizeof(gGame->m_townBuiltToday));
    *last = saved;
    memcpy(gCurPlayerData->m_resources, resources, sizeof(resources));
}

// CopyTo's full-width branch added the rows without multiplying them by the
// row length, copying the wrong rows onto themselves.
void FullWidthCopy() {
    const int rows = 8;
    bitmap source(0, LOGICAL_SCREEN_WIDTH, rows);
    bitmap destination(0, LOGICAL_SCREEN_WIDTH, rows);
    for (int i = 0; i < LOGICAL_SCREEN_WIDTH * rows; i++) {
        source.m_pixels[i] = static_cast<u8>(i / LOGICAL_SCREEN_WIDTH + 1);
        destination.m_pixels[i] = 0xEE;
    }
    source.CopyTo(&destination, 0, 3, 0, 3, LOGICAL_SCREEN_WIDTH, 2);
    bool copied = true;
    for (int row = 0; row < rows; row++)
        for (int x = 0; x < LOGICAL_SCREEN_WIDTH; x++)
            copied = copied
                     && destination.m_pixels[row * LOGICAL_SCREEN_WIDTH + x]
                            == (row == 3 || row == 4 ? row + 1 : 0xEE);
    Expect(copied, "a full-width copy from row 3 writes rows 3 and 4");
}

}  // namespace

int main() {
    const char* data = std::getenv("HOMM1_DATA");
    if (data == nullptr || data[0] == '\0') {
        std::printf("skipped: needs HOMM1_DATA\n");
        return 77;
    }
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    // SIGTERM ends the program at once (timeout, ctest); SDL would turn it into
    // a quit event that a hung loop never reads.
    setenv("SDL_NO_SIGNAL_HANDLERS", "1", 0);
    setenv("SDL_AUDIO_DRIVER", "dummy", 0);
    setenv("HOMM1_NO_DIALOGS", "1", 1);
    const char* temporary = std::getenv("TMPDIR");
    std::string config = std::string(temporary != nullptr && temporary[0] != '\0' ? temporary : "/tmp")
                         + "/homm1-game-regressions-XXXXXX";
    if (mkdtemp(config.data()) == nullptr)
        return 1;
    setenv("XDG_CONFIG_HOME", config.c_str(), 1);
    if (!KBStartHost(data, "/I0", 0))
        return 1;
    StartGame();
    NewGame("AES31000.MAP");
    SpellBookKeepsBoatTable();
    ObeliskValueWithoutUltimate();
    MineFlagOnTopRow();
    RouteLeavingTheGrid();
    MapChecks();
    WeekWithAbsentPlayers();
    CampaignCrests();
    LongWords();
    AffordableCreaturesNeedEveryResource();
    ReplacedStackIsTheWeakest();
    HeroDistanceUsesY();
    RouteThroughAStaleNode();
    RandomArtifactWithoutAFreeSlot();
    CommanderStatsHoldInTheirByte();
    BerserkWithNothingToAttack();
    ComputerCampfireAndFullHero();
    SummonBoatOnTheEdge();
    StrayTownsUnderMonsters();
    CampaignLordCrest();
    FirstPlayerUltimateHint();
    TownFlagsForEveryTown();
    FullWidthCopy();
    std::string cleanup = "rm -r '" + config + "'";
    if (std::system(cleanup.c_str()) != 0)
        std::fprintf(stderr, "could not remove %s\n", config.c_str());
    std::fflush(stdout);
    std::_Exit(gFailures != 0 ? 1 : 0);
}
