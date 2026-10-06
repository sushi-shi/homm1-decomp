#ifndef HOMM1_SOURCE_GAME_H
#define HOMM1_SOURCE_GAME_H

#include <SOURCE/gameTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/playerData.h>
#include <SOURCE/town.h>

class army;
class armyGroup;
class fullMap;
class hero;
class mapCell;
class playerData;
class town;
struct SMapHeader;
struct tag_message;

enum GameDifficulty {
    DIFFICULTY_EASY = 0,
    DIFFICULTY_NORMAL = 1,
    DIFFICULTY_HARD = 2,
    DIFFICULTY_EXPERT = 3,
    DIFFICULTY_COUNT = 4
};

enum CalendarPeriodType {
    CALENDAR_PERIOD_NONE = -1,
    CALENDAR_PERIOD_NORMAL = 0,
    CALENDAR_PERIOD_CREATURE = 1,
    CALENDAR_PERIOD_PLAGUE = 2
};

enum GameMineSlot {
    MINE_SLOT_DRAGON_CITY = 0,
    MINE_SLOT_LIGHTHOUSE = 1,
    MINE_SLOT_STANDARD_FIRST = 2
};

enum MineFootprintConstant {
    MINE_FOOTPRINT_WIDTH = 2,
    MINE_FOOTPRINT_HEIGHT = 2
};

enum DailyIncomeConstant {
    DAILY_GOLD_DRAGON_CITY = 1000,
    DAILY_GOLD_MINE = 1000,
    DAILY_GOLD_TOWN = 250,
    DAILY_GOLD_CASTLE = 1000,
    DAILY_GOLD_ENDLESS_SACK = 1000,
    DAILY_GOLD_ENDLESS_BAG = 750,
    DAILY_GOLD_ENDLESS_PURSE = 500,
    DAILY_MINE_YIELD_WOOD_ORE = 2,
    DAILY_MINE_YIELD_OTHER = 1
};

enum GameWeeklyConstant {
    WEEKLY_WELL_GROWTH_BONUS = 2,
    WEEKLY_CREATURE_GROWTH_BONUS = 5,
    WEEKLY_SITE_STOCK_LIMIT = 100,
    WEEKLY_WATER_WHEEL_EMPTY = 0xff,
    WEEKLY_WATER_WHEEL_GOLD = 2,
    MONTHLY_CREATURE_GROWTH_FACTOR = 2
};

enum SpellBookControl {
    SPELL_BOOK_PREVIOUS_PAGE = 2,
    SPELL_BOOK_NEXT_PAGE = 3,
    SPELL_BOOK_ADVENTURE_SPELLS = 4,
    SPELL_BOOK_COMBAT_SPELLS = 5,
    SPELL_BOOK_ENTRY_FIRST = 6,
    SPELL_BOOK_ENTRY_LAST = 9,
    SPELL_BOOK_LABEL_FIRST = 10
};

enum CampaignInfoControl {
    CAMPAIGN_INFO_NAME = 1,
    CAMPAIGN_INFO_TEXT = 2,
    CAMPAIGN_INFO_PROGRESS = 3,
    CAMPAIGN_INFO_PROGRESS_FRAME_BASE = 4,
    CAMPAIGN_INFO_RESTART = 0x385
};

enum CampaignScenarioConstant {
    CAMPAIGN_SCENARIO_LORD_FIRST = 4,
    CAMPAIGN_SCENARIO_LORD_LAST = 7
};

enum MapSoundConstant {
    MAP_SOUND_NONE = -1
};
enum SpellBookConstant {
    SPELL_BOOK_PAGE_SIZE = 4
};

enum UltimateHintConstant {
    ULTIMATE_HINT_OBELISK_MIN = 11,
    ULTIMATE_HINT_PERCENT_PER_OBELISK = 4,
    ULTIMATE_HINT_SCATTER = 3,
    ULTIMATE_HINT_PLACE_TRIES = 200,
    OBELISK_PIECE_PICK_TRIES = 100
};

enum GameRandomHeroConstant {
    RANDOM_HERO_NORMAL_ARMY = 0,
    RANDOM_HERO_STRONG_ARMY = 1,
    RANDOM_HERO_EXPERIENCE_BASE = 40,
    RANDOM_HERO_FIRST_STACK_CHANCE = 50,
    RANDOM_HERO_FIRST_STACK_BONUS_CHANCE = 30,
    RANDOM_HERO_SECOND_STACK_CHANCE = 25,
    RANDOM_HERO_SECOND_STACK_BONUS_CHANCE = 40,
    RANDOM_HERO_ARMY_SELECTION_COUNT = 2,
    RANDOM_HERO_ARMY_OPTION_COUNT = 3,
    RANDOM_HERO_ARMY_FIELD_COUNT = 3,
    RANDOM_HERO_ARMY_FIELD_CREATURE = 0,
    RANDOM_HERO_ARMY_FIELD_MIN = 1,
    RANDOM_HERO_ARMY_FIELD_MAX = 2,
    RANDOM_HERO_COUNT_SCALE = 10,
    RANDOM_HERO_COUNT_ROUNDING = 9,
    RANDOM_HERO_EMPTY_COUNT = -1
};

enum GameLuckConstant {
    GAME_LUCK_MIN = -3,
    GAME_LUCK_MAX = 3
};

enum SaveFileConstant {
    SAVE_FILE_BASE_NAME_LENGTH = 8,
    SAVE_FILE_NAME_SCAN_STOP = 999
};

#pragma pack(push, 1)
struct mineRecord {
    i8 id;
    i8 owner;
    i8 type;
    i8 guardianType;
    u8 guardianCount;
    i8 x;
    i8 y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct boatRecord {
    i8 id;
    i8 x;
    i8 y;
    i8 direction;
    u8 savedTriggerType;
    u8 savedEventData;
    i8 heroId;
    i8 owner;
};
#pragma pack(pop)

enum MapTownRecordConstant {
    MAP_TOWN_TYPE_MASK = 0x7f,
    MAP_TOWN_OWNER_UNSET = -2,
    MAP_TOWN_EXTRA_BUILDING_MASK = 0x1f9f,
    MAP_HERO_EXTRA_ARTIFACT_COUNT = 4
};

#pragma pack(push, 1)
struct mapTownExtra {
    i8 customized;
    i8 owner;
    i16 buildings;
    i8 buildState;
    i8 troopTypes[ARMY_GROUP_SLOT_COUNT];
    i16 troopCounts[ARMY_GROUP_SLOT_COUNT];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct mapHeroExtra {
    i8 owner;
    i8 troopTypes[ARMY_GROUP_SLOT_COUNT];
    i16 troopCounts[ARMY_GROUP_SLOT_COUNT];
    i8 heroId;
    i8 artifacts[MAP_HERO_EXTRA_ARTIFACT_COUNT];
    i32 experience;
};
#pragma pack(pop)

#pragma pack(push, 1)
class game {
public:
    i16 m_difficultyRating;
    i8 m_unknown0002;
    i32 m_campaignType;
    i32 m_campaignScenario;
    i32 m_campaignScenariosWon;
    i32 m_campaignDay;
    i8 m_mapSize;
    i8 m_mapDifficulty;
    char m_mapName[0x11];
    char m_mapDescription[0x79];
    char m_saveName[0x15f];
    i8 m_difficulty;
    i8 m_playerCount;
    i8 m_unknown200;
    i8 m_deadPlayerCount;
    i8 m_playerDead[GAME_PLAYER_COUNT];
    u16 m_day;
    u16 m_week;
    u16 m_month;
    class playerData m_players[GAME_PLAYER_COUNT];
    class mapCell m_map[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    char m_unknownd0a0[0x5100];
    i8 m_obeliskCount;
    class town m_castleRecs[GAME_TOWN_COUNT];
    i8 m_townOwners[GAME_TOWN_COUNT];
    u8 m_townBuiltToday[4];
    class hero m_heroRecs[GAME_HERO_COUNT];
    i8 m_availableHeroes[GAME_HERO_COUNT];
    mineRecord m_mines[GAME_MINE_COUNT];
    i8 m_mineOwners[GAME_MINE_COUNT];
    i8 m_randomArtifacts[0x25];
    boatRecord m_boats[GAME_BOAT_COUNT];
    i8 m_boatSlots[GAME_BOAT_COUNT];
    i8 m_obeliskVisitors[0x30];
    i8 m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    u8 m_mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    i8 m_ultimateArtifactX;
    i8 m_ultimateArtifactY;
    i8 m_ultimateArtifactId;
    class heroWindow* m_newGameWindow;
    i8 m_unknown16e5d;
    class heroWindow* m_viewArmyWindow;
    i16 m_viewArmyResult;
    class heroWindow* m_viewSpellsWindow;
    class hero* m_viewSpellsHero;
    i16 m_spellFirst;
    i16 m_spellLast;
    i16 m_viewSpell;
    i16 m_viewSpellsTop;
    i16 (*m_viewSpellsCallback)(struct tag_message&);
    i8 m_viewSpellsReadOnly;
    i8 m_noMapHeroes;
    hero* GetHero(i8 id) {
        return &m_heroRecs[id];
    }
    town* GetTown(i8 id) {
        return &m_castleRecs[id];
    }
    hero* GetPlayerHero(i32 player, i32 index) {
        return &m_heroRecs[m_players[player].m_heroIds[index]];
    }
    town* GetPlayerTown(i32 player, i32 index) {
        return &m_castleRecs[m_players[player].m_townIds[index]];
    }
    void SetupDynamicStuff(i32 redraw, i32 updateKnob, i32 forceUpdate);
    void SetupNewOverviewType(i32 overviewType, i32 redrawFrom);
    void SetupResources(void);
    void Overview(void);
    void DoKnob(void);
    i32 ProcessIconSelect(i32 widgetId, i32 quickView);
    i8 SetupCampaignGame(void);
    i8 SetupBaud(void);
    i8 SetupComPort(void);
    i8 SetupHotSeatGame(void);
    i8 SetupNetworkGame(void);
    i32 SetupNetworkGame2(void);
    i8 SetupModemGame(void);
    i8 SetupMultiPlayerGame(void);
    i8 SetupGame(i8 newGame);
    i8 PickLoadGame(void);
    i32 HandleCampaignWin(void);
    void PlayPreScenarioSmacker(i32 side, i32 map);
    void ShowCampaignInfo(i32 scenario, i32 fromMenu, i32);
    void CampaignInfoUpdate(i32 redraw);
    void InitEntireCampaign(i32 side);
    void InitCampaignMap(i32 scenario, i32);
    i32 MineTypesOwned(i32 owner, i32 mineType);
    i32 SetupPuzzlePieces(i32 player, i32 justCount);
    i8 IsMobile(i8 heroId);
    class mapCell (*GetWorldMapData(void))[MAP_CELL_GRID_SIZE];
    void ReadWorldMap(i32 fd);
    void WriteWorldMap(i32 fd);
    i8 CreateBoat(i8 x, i8 y);
    i8 Scan(i8* array, i8 start, i8 length);
    i8 RandomScan(i8* array, i8 start, i8 range, i32);
    i8 GetNewHeroId(i8 heroClass);
    i8 GetTownId(i8 x, i8 y);
    i8 GetMineId(i8 x, i8 y);
    i16 SaveGame(char* filename, i8 generateName);
    void SetupOrigData(void);
    i16 LoadGame(char* filename, i32 origData, i32);
    void GiveTroopsToNeutralTown(i32 townId);
    void GiveTroopsToNeutralTowns(void);
    void NewMap(char* mapName);
    void RandomizeEvents(void);
    void InitializePasswords(void);
    void RandomizeBarrier(class mapCell* cell);
    void RandomizePassword(class mapCell* cell);
    i16 LoadMap(char* filename);
    void ClaimTown(i8 townId, i8 player);
    void ClaimMine(i8 mineId, i8 player);
    i8 ViewSpells(
        class hero* spellHero,
        i8 spellType,
        i16 (*callback)(struct tag_message&),
        i8 readOnly
    );
    void SetupSpellRange(i16 spellType);
    void UpdateSpellWidgets(void);
    void ViewArmy(
        i16 x,
        i16 y,
        i8 monsterType,
        i16 numTroops,
        class town* castle,
        i8 disableDismiss,
        i8 facing,
        i8 quickView,
        class hero* theHero,
        class army* theArmy,
        class armyGroup* theGroup
    );
    i8 GetRandomNumTroops(i8 monsterType);
    void TurnOnAIMusic(void);
    void TurnOffAIMusic(void);
    void NextPlayer(void);
    i32 ComputeDailyGold(i32 player);
    void PerDay(void);
    void PerWeek(void);
    void WeeklyRecruitSite(class mapCell* cell);
    void WeeklyGenericSite(class mapCell* cell);
    void PerMonth(void);
    void ConvertObject(
        i32 left,
        i32 top,
        i32 right,
        i32 bottom,
        i32 oldTileset,
        i32 oldFirstIndex,
        i32 oldLastIndex,
        i32 newTileset,
        i32 newFirstIndex,
        i32 oldTrigger,
        i32 newTrigger
    );
    void RandomizeTown(i8 x, i8 y, i8 isCastle);
    void RandomizeMine(i8 x, i8 y);
    void SetupTown(i8 townId, i8 aiOwned);
    void InitRandomArtifacts(void);
    i8 GetRandomArtifactId(void);
    void RandomizeHeroPool(void);
    void SetRandomHeroArmies(i16 heroId, i32 strongArmy);
    void ProcessRandomObjects(i32 castlesOnly);
    void SetVisibility(i16 x, i16 y, i16 player, i16 radius);
    void MakeAllWaterVisible(i32 player);
    void GiveArmy(class armyGroup* group, i32 type, i32 count, i32 slot);
    i32 ExperienceValueOfStack(class armyGroup* group, class hero* h);
    i32 GetLuck(class hero* h, class army*);
    i32 GetPlayerCrest(i32 player) {
        return m_players[player].m_color;
    }
    void SetupAdjacentMons(void);
    void CancelComputerScreen(void);
    void ShowComputerScreen(void);
    void ShowHeroesLogo(void);
    void WaitForPlayer(char* text, i32 player);
    i32 HasLateOverlay(i32 column, i32 row);
    void ConvertFlagToLateOverlay(i32 column, i32 row);
    i32 HasObjectTilesetIndex(i32 column, i32 row, i32 tileset, i32 index);
    void ConvertAllToLateOverlay(i32 column, i32 row);
    void SettleOverlay(i32 x, i32 y);
    void RandomizeTerrainTiles(void);
    void ProcessMapExtra(void);
    i8 SetupTowns(void);
    void ProcessOnMapHeroes(void);
    void CheckHeroConsistency(void);
    i32 TransmitSaveGame(i32 remotePlayer, i32 playerExited);
    i32 ReceiveSaveGame(i32 dataSize, i32 remotePlayer);
    void DoNewTurn(void);
    i32 GetBoatsBuilt(void);
    i32 GetNumThievesGuilds(i32 color);
    i32 CalcDifficultyRating(void);
    void RestoreCell(i32 x, i32 y, i32 obj, i32 barrier, class mapCell* passedCell, i32);
    void SetMapSize(i32 width, i32 height);
    i32 HeroIDToHeroPos(class playerData* player, i32 heroId);
    i32 TownIDToTownPos(class playerData* player, i32 townId);
    void SetupNewRumour(void);
    void CheckForTimeEvent(void);
    i32 CountShrines(i32 player);
    void ShowMoraleInfo(class hero* h, i32 dialogType);
    void ShowLuckInfo(class hero* h, i32 dialogType);
    static void GetMap(void);
    void ProcessNewMap(struct SMapHeader* header);
    void InitNewGame(struct SMapHeader* header);
    void SetupNetPlayerNames(void);
    i8 NewGame(void);
    void CleanUpNewGameWindow(void);
    void InitNewGameWindow(void);
    void UpdateNewGameWindow(void);
    i32 ProcessNGKeyPress(struct tag_message& message);
    void NGKPSetupDisplayString(char* text, u16 cursor);
    void DrawNGKPDisplayString(i32 updateScreen);
    void ShowScenInfo(void);
    void RandomizePlayerCrests(void);
    void GetLossConditionText(char* text);
    void GetVictoryConditionText(char* text);
    i32 GetSideDesc(char* text, i32 firstPlayer, i32 lastPlayer);
    void VisitObelisk(i8 player);
};
#pragma pack(pop)

void ComputeUALoc(i32 player);
i16 ViewSpellsHandler(struct tag_message& message);
i16 ViewSpecialHandler(struct tag_message& message);
i16 ViewArmyHandler(struct tag_message& message);
i32 GetBaseScore(i32 days);
extern i32 gGameOver;
extern i32 gEndSequence;
extern i8 gSaveCurPlayer;
extern i8 gSavedDifficulty;
extern i8 gSavedPlayerTypes[];
extern i8 gSavedKingOfTheHill;
extern i8 gSavedCrest;
extern i8 gRandomTownTypes[4];
extern i16 gMineTypeCount[];
extern u32 gLastSeed;
i32 SGenRand(void);
i32 SRandom(i32 low, i32 high);
void SIncRandomize(i32 x, i32 y);
void SRand(i32 seed);
extern i8 gShowMapInfo;
extern heroWindow* gReqExtraWindow;
extern char gCurMapName[];
extern i8 gbDismissArmy;

#endif
