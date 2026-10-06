#ifndef HOMM1_SOURCE_GAME_H
#define HOMM1_SOURCE_GAME_H

#include <BASE/message.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/fileRequester.h>
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

enum GameRandomObjectConstant {
    RANDOM_MONSTER_ANY_LOW = 80,
    RANDOM_MONSTER_ANY_HIGH = 2000,
    RANDOM_MONSTER_WEAK_LOW = 0,
    RANDOM_MONSTER_WEAK_HIGH = 400,
    RANDOM_MONSTER_MEDIUM_LOW = 80,
    RANDOM_MONSTER_MEDIUM_HIGH = 1000,
    RANDOM_MONSTER_STRONG_LOW = 500,
    RANDOM_MONSTER_STRONG_HIGH = 2500,
    RANDOM_MONSTER_VERY_STRONG_LOW = 2000,
    RANDOM_MONSTER_VERY_STRONG_HIGH = 100000,
    RANDOM_MINE_TYPE_ROLLS = 30
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
    SPELL_BOOK_NEXT_PAGE = 3, SPELL_BOOK_ADVENTURE_SPELLS = 4, SPELL_BOOK_COMBAT_SPELLS = 5,
    SPELL_BOOK_ENTRY_FIRST = 6, SPELL_BOOK_ENTRY_LAST = 9,
    SPELL_BOOK_LABEL_FIRST = 10 };

    enum CampaignInfoControl { CAMPAIGN_INFO_NAME = 1,
    CAMPAIGN_INFO_TEXT = 2, CAMPAIGN_INFO_PROGRESS = 3, CAMPAIGN_INFO_PROGRESS_FRAME_BASE = 4,
    CAMPAIGN_INFO_RESTART = 0x385 };

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
    ULTIMATE_HINT_PIECE_MIN = 11,
    ULTIMATE_HINT_PERCENT_PER_PIECE = 4,
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
    RANDOM_HERO_ARMY_OPTION_SURE = 0,
    RANDOM_HERO_ARMY_OPTION_FIRST_ROLL = 1,
    RANDOM_HERO_ARMY_OPTION_SECOND_ROLL = 2,
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

enum BoatRecordConstant {
    BOAT_OCCUPIED_FLAG = 0x80
};

enum MapTownRecordConstant {
    MAP_TOWN_TYPE_MASK = 0x7f,
    MAP_TOWN_CASTLE_FLAG = 0x80,
    MAP_TOWN_OWNER_UNSET = -2,
    MAP_TOWN_EXTRA_BUILDING_MASK = 0x1f9f,
    MAP_HERO_EXTRA_ARTIFACT_COUNT = 4
};

#pragma pack(push, 1)
struct mapTownExtra {
    b8 customized;
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

class game {
public:
    i16 m_difficultyRating;
    i8 m_unused0002;
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
    i8 m_unused200;
    i8 m_deadPlayerCount;
    b8 m_playerDead[GAME_PLAYER_COUNT];
    u16 m_day;
    u16 m_week;
    u16 m_month;
    class playerData m_players[GAME_PLAYER_COUNT];
    class mapCell m_map[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    char m_unusedd0a0[0x5100];
    i8 m_obeliskCount;
    class town m_castleRecs[GAME_TOWN_COUNT];
    i8 m_townOwners[GAME_TOWN_COUNT];
    u8 m_townBuiltToday[4];
    class hero m_heroRecs[GAME_HERO_COUNT];
    i8 m_availableHeroes[GAME_HERO_COUNT];
    mineRecord m_mines[GAME_MINE_COUNT];
    i8 m_mineOwners[GAME_MINE_COUNT];
    i8 m_randomArtifacts[ARTIFACT_REGULAR_END];
    boatRecord m_boats[GAME_BOAT_COUNT];
    i8 m_boatSlots[GAME_BOAT_COUNT];
    i8 m_obeliskVisitors[0x30];
    i8 m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    u8 m_mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    i8 m_ultimateArtifactX;
    i8 m_ultimateArtifactY;
    i8 m_ultimateArtifactId;
    class heroWindow* m_newGameWindow;
    i8 m_unused16e5d;
    class heroWindow* m_viewArmyWindow;
    i16 m_dialogAnimationCounter;
    class heroWindow* m_viewSpellsWindow;
    class hero* m_viewSpellsHero;
    i16 m_spellFirst;
    i16 m_spellLast;
    i16 m_viewSpell;
    i16 m_viewSpellsTop;
    i16 (*m_viewSpellsCallback)(struct tag_message&);
    i8 m_viewSpellsReadOnly;
    b8 m_noMapHeroes;
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
    void Overview(void);
    i8 SetupCampaignGame(void);
    i8 SetupBaud(void);
    i8 SetupComPort(void);
    i8 SetupHotSeatGame(void);
    i8 SetupNetworkGame(void);
    i8 SetupModemGame(void);
    i8 SetupMultiPlayerGame(void);
    i8 SetupGame(b8 newGame);
    i8 PickLoadGame(void);
    void ShowCampaignInfo(i32 scenario, b32 viewOnly, i32);
    void InitEntireCampaign(i32 side);
    void InitCampaignMap(i32 scenario, i32);
    b8 IsMobile(i8 heroId);
    class mapCell (*GetWorldMapData(void))[MAP_CELL_GRID_SIZE];
    void ReadWorldMap(class RecordReader& in);
    void WriteWorldMap(class RecordWriter& out);
    i8 CreateBoat(i8 x, i8 y);
    i8 Scan(i8* array, i8 start, i8 length);
    i8 RandomScan(i8* array, i8 start, i8 range, i32);
    i8 GetNewHeroId(i8 heroClass);
    i8 GetTownId(i8 x, i8 y);
    i8 GetMineId(i8 x, i8 y);
    i16 SaveGame(char* filename, b8 generateName);
    i16 LoadGame(char* filename, b32 origData, b32 remoteGame);
    void GiveTroopsToNeutralTowns(void);
    void NewMap(char* mapName);
    void RandomizeEvents(void);
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
        b8 disableDismiss,
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
    void PerMonth(void);
    void RandomizeTown(i8 x, i8 y, b8 isCastle);
    void RandomizeMine(i8 x, i8 y);
    void SetupTown(i8 townId, b8 aiOwned);
    i8 GetRandomArtifactId(void);
    void RandomizeHeroPool(void);
    void SetRandomHeroArmies(i16 heroId, i32 strongArmy);
    void ProcessRandomObjects(b32 castlesOnly);
    void SetVisibility(i16 x, i16 y, i16 player, i16 radius);
    void
    GiveArmy(class armyGroup* group, i32 type, i32 count, i32 slot);
    i32 ExperienceValueOfStack(class armyGroup* group, class hero* heroPointer);
    i32 GetLuck(class hero* heroPointer, class army*);
    i16 GetPlayerColor(i32 player) {
        return m_players[player].m_color;
    }
    void SetupAdjacentMons(void);
    void CancelComputerScreen(void);
    void ShowComputerScreen(void);
    void ShowHeroesLogo(void);
    void WaitForPlayer(char* text, i32 player);
    void SettleOverlay(i32 x, i32 y);
    void RandomizeTerrainTiles(void);
    void ProcessMapExtra(void);
    i8 SetupTowns(void);
    void ProcessOnMapHeroes(void);
    void CheckHeroConsistency(void);
    i32 TransmitSaveGame(i32 remotePlayer, i32 playerExited);
    b32 ReceiveSaveGame(i32 dataSize, i32 remotePlayer);
    void DoNewTurn(void);
    i32 GetBoatsBuilt(void);
    i32 GetNumThievesGuilds(i32 player);
    i32 CalcDifficultyRating(void);
    void RestoreCell(i32 x, i32 y, i32 objectType, i32 barrier, class mapCell* passedCell, i32);
    void ShowMoraleInfo(class hero* heroPointer, i32 dialogType);
    void ShowLuckInfo(class hero* heroPointer, i32 dialogType);
    static void GetMap(void);
    i8 NewGame(void);
    void UpdateNewGameWindow(void);
    void ShowScenInfo(void);
    void RandomizePlayerCrests(void);
    void VisitObelisk(i8 player);
};

#define GAME_DAY_NUMBER(g)                                                                         \
    ((g).m_day + ((g).m_week - 1) * CALENDAR_DAYS_PER_WEEK                                         \
     + ((g).m_month - 1) * CALENDAR_DAYS_PER_MONTH)

void ComputeUALoc(i32 player);
i16 ViewSpellsHandler(struct tag_message& message);
i16 ViewSpecialHandler(struct tag_message& message);
i16 ViewArmyHandler(struct tag_message& message);
i32 GetBaseScore(i32 days);
extern b32 gGameOver;
extern i8 gSavedCurPlayer;

extern i8 gSavedDifficulty;
extern i8 gSavedDifficulties[];
extern i8 gSavedKingOfTheHill;

extern i8 gSavedCrest;
extern i8 gRandomTownTypes[4];
extern i16 gMineTypeCount[RESOURCE_COUNT];
extern i32 gLastSeed;
i32 SGenRand(void);
i32 SRandom(i32 low, i32 high);
void SIncRandomize(i32 x, i32 y);
void SRand(i32 seed);
extern b8 gShowMapInfo;
extern heroWindow* gReqExtraWindow;
extern char gCurMapName[];
extern b8 gDismissArmy;

enum NewGameControl {
NEW_GAME_OPPONENT_FIRST = 2,
    NEW_GAME_OPPONENT_LAST = 4, NEW_GAME_COLOR = 8, NEW_GAME_SCENARIO_SELECT = 0xc,
    NEW_GAME_DIFFICULTY_FIRST = 0xd, NEW_GAME_DIFFICULTY_LAST = 0x10, NEW_GAME_SCENARIO_NAME = 0x11,
    NEW_GAME_SCENARIO_PANEL = 0x12, NEW_GAME_KING_OF_THE_HILL = 0x13, NEW_GAME_RATING = 0x14,
    NEW_GAME_CANCEL = DIALOG_BUTTON_1, NEW_GAME_OK = DIALOG_BUTTON_2,
    NEW_GAME_OPPONENT_TOGGLE_BASE = 1,
    NEW_GAME_OPPONENT_LABEL_BASE = 4 };

    enum NewGameFrame {
    NEW_GAME_FRAME_COMPUTER_TYPE_BASE = 5,
    NEW_GAME_FRAME_CREST_BASE = 11,
    NEW_GAME_FRAME_CREST_STRIDE = 2,
    NEW_GAME_FRAME_HUMAN_OPPONENT = 0x1a,
    NEW_GAME_FRAME_KING_OF_THE_HILL_BASE = 27
};

enum NewGameHelp {
    NEW_GAME_HELP_NONE = -1,
    NEW_GAME_HELP_FIRST = 0,
    NEW_GAME_HELP_ACCEPT = 0,
    NEW_GAME_HELP_MAIN_MENU = 1,
    NEW_GAME_HELP_KING_OF_THE_HILL = 2,
    NEW_GAME_HELP_SCENARIO = 3,
    NEW_GAME_HELP_DIFFICULTY = 4,
    NEW_GAME_HELP_OPPONENT = 5,
    NEW_GAME_HELP_COLOR = 6,
    NEW_GAME_HELP_RATING = 7,
    NEW_GAME_HELP_HUMAN_OPPONENT = 8,
    NEW_GAME_HELP_COUNT = 9
};

enum NeutralTownReinforcementConstant {
    REINFORCEMENT_ROLL_MIN = 1,
    REINFORCEMENT_ROLL_MAX = 15,
    REINFORCEMENT_TIER_ONE_THRESHOLD = 5,
    REINFORCEMENT_TIER_TWO_THRESHOLD = 10,
    REINFORCEMENT_TIER_THREE_THRESHOLD = 13,
    REINFORCEMENT_TIER_ONE_KEY = 10,
    REINFORCEMENT_TIER_TWO_KEY = 20,
    REINFORCEMENT_TIER_THREE_KEY = 30,
    REINFORCEMENT_TIER_FOUR_KEY = 40,
    REINFORCEMENT_TIER_ONE_COUNT_MIN = 8,
    REINFORCEMENT_TIER_ONE_COUNT_MAX = 15,
    REINFORCEMENT_TIER_TWO_COUNT_MIN = 5,
    REINFORCEMENT_TIER_TWO_COUNT_MAX = 7,
    REINFORCEMENT_TIER_THREE_COUNT_MIN = 3,
    REINFORCEMENT_TIER_THREE_COUNT_MAX = 5,
    REINFORCEMENT_TIER_FOUR_COUNT_MIN = 1,
    REINFORCEMENT_TIER_FOUR_COUNT_MAX = 3
};

enum ViewArmyControl {
VIEW_ARMY_COUNT_FRAME = 1,
    VIEW_ARMY_COUNT_TEXT = 2, VIEW_ARMY_TITLE = 3, VIEW_ARMY_STATS = 4, VIEW_ARMY_ANIMATION = 5,
    VIEW_ARMY_DISMISS = DIALOG_BUTTON_3,
    VIEW_ARMY_CLOSE = DIALOG_BUTTON_0 };

        enum ViewArmyConstant {
    VIEW_ARMY_ANIMATION_FRAMES = 6,
    VIEW_ARMY_FRAME_DELAY = 90,
    VIEW_ARMY_STAT_TEXT_SIZE = 550
};

enum OverviewControl {
OVERVIEW_RESOURCE_BASE = 1,
    OVERVIEW_DATE = 64,
    OVERVIEW_DAILY_GOLD = 65 };

    enum CalendarConstant {
    CALENDAR_WEEK_NAME_COUNT = 15,
    CALENDAR_WEEK_CREATURE_COUNT = 24,
    CALENDAR_MONTH_NAME_COUNT = 10,
    CALENDAR_MONTH_CREATURE_COUNT = 12,
    NEW_TURN_TEXT_DAYS_LEFT = 0,
    NEW_TURN_TEXT_LAST_DAY = 1,
    NEW_TURN_TEXT_MONTH_NORMAL = 2,
    NEW_TURN_TEXT_MONTH_CREATURE = 3,
    NEW_TURN_TEXT_MONTH_PLAGUE = 4,
    NEW_TURN_TEXT_WEEK_NORMAL = 5,
    NEW_TURN_TEXT_WEEK_CREATURE = 6
};

enum TerrainTileConstant {
    TERRAIN_TILE_VARIANT_COUNT = 4
};

enum RemoteSaveConstant {
    REMOTE_SAVE_SEGMENT_SIZE = 200,
    REMOTE_SAVE_BATCH_SIZE = 100,
    REMOTE_SAVE_HEADER_SIZE = 8,
    REMOTE_SAVE_ACK_MAP_SIZE = 200,
    REMOTE_SAVE_INDEX_SIZE = 2,
    REMOTE_SAVE_BUFFER_EXTRA = 500,
    REMOTE_SAVE_DECODE_BUFFER_SIZE = 0x130b0,
    REMOTE_SAVE_TRANSFER_SOUNDS = 8,
    // The segments the transfer's acknowledgement tables hold.
    REMOTE_SAVE_SEGMENT_LIMIT = 500,
    // The compressed save's big-endian decoded size.
    REMOTE_SAVE_DECODED_SIZE_BYTES = 4
};

enum GamePlayerCount {
    GAME_PLAYERS_TWO = 2,
    GAME_PLAYERS_THREE = 3,
    GAME_PLAYERS_FOUR = 4
};

enum MonthCreatureSpawnConstant {
    MONTH_CREATURE_SPAWN_ROLL_MAX = 360,
    MONTH_CREATURE_SPAWN_ROLL_HIT = 10
};

#endif
