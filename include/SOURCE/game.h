#ifndef HOMM1_SOURCE_GAME_H
#define HOMM1_SOURCE_GAME_H

#include <BASE/message.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/gameTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/playerData.h>
#include <SOURCE/town.h>

// forward declarations:
class army;
class armyGroup;
class fullMap;
class hero;
class mapCell;
class playerData;
class town;
struct SMapHeader;
struct tag_message;

// game::m_difficulty: the four new-game difficulty buttons and
// gDifficultyNames ("Easy", "Normal", "Hard", "Expert").
H1_ENUM_BEGIN(GameDifficulty)
    DIFFICULTY_EASY = 0,
    DIFFICULTY_NORMAL = 1,
    DIFFICULTY_HARD = 2,
    DIFFICULTY_EXPERT = 3,
    DIFFICULTY_COUNT = 4
H1_ENUM_END(GameDifficulty)

// gWeekType / gMonthType: a named week or month
// (gWeekNames / gMonthNames[special]), a creature week or month
// (gArmyNames[special] grows), or the month of the plague. NONE suppresses
// the new-week announcement.
H1_ENUM_BEGIN(CalendarPeriodType)
    CALENDAR_PERIOD_NONE = -1,
    CALENDAR_PERIOD_NORMAL = 0,
    CALENDAR_PERIOD_CREATURE = 1,
    CALENDAR_PERIOD_PLAGUE = 2
H1_ENUM_END(CalendarPeriodType)

// The first two game::m_mines records are the unique sites: Dragon City
// (pays 1000 gold a day) and the Lighthouse (ship movement).
H1_ENUM_CONST_BEGIN(GameMineSlot)
    MINE_SLOT_DRAGON_CITY = 0,
    MINE_SLOT_LIGHTHOUSE = 1,
    // The ordinary mines follow the two unique sites (PerDay, Overview,
    // ComputeDailyGold loop from here).
    MINE_SLOT_STANDARD_FIRST = 2
H1_ENUM_CONST_END(GameMineSlot)

// A mine object covers 2x2 cells from its record's (x, y - 1) to (x + 1, y)
// (RandomizeEvents, RandomizeMine).
H1_ENUM_CONST_BEGIN(MineFootprintConstant)
    MINE_FOOTPRINT_WIDTH = 2,
    MINE_FOOTPRINT_HEIGHT = 2
H1_ENUM_CONST_END(MineFootprintConstant)

// ProcessRandomObjects rerolls a random monster until its fight value lies
// above its strength's LOW and below its HIGH; RandomizeMine rerolls the mine
// type up to MINE_TYPE_ROLLS times looking for a resource no mine has yet.
H1_ENUM_CONST_BEGIN(GameRandomObjectConstant)
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
H1_ENUM_CONST_END(GameRandomObjectConstant)

// Daily income (ComputeDailyGold, PerDay): Dragon City and a gold mine pay
// 1000 gold, a town 250 and a castle 1000; an ore or wood mine yields two
// units a day, the other non-gold mines one.
H1_ENUM_CONST_BEGIN(DailyIncomeConstant)
    DAILY_GOLD_DRAGON_CITY = 1000,
    DAILY_GOLD_MINE = 1000,
    DAILY_GOLD_TOWN = 250,
    DAILY_GOLD_CASTLE = 1000,
    DAILY_GOLD_ENDLESS_SACK = 1000,
    DAILY_GOLD_ENDLESS_BAG = 750,
    DAILY_GOLD_ENDLESS_PURSE = 500,
    DAILY_MINE_YIELD_WOOD_ORE = 2,
    DAILY_MINE_YIELD_OTHER = 1
H1_ENUM_CONST_END(DailyIncomeConstant)

// Weekly growth (PerWeek/PerMonth): a well adds two
// creatures a dwelling, the week's creature five; renewable sites stop
// restocking at 100 and an emptied water wheel (0xff) refills to 2.
H1_ENUM_CONST_BEGIN(GameWeeklyConstant)
    WEEKLY_WELL_GROWTH_BONUS = 2,
    WEEKLY_CREATURE_GROWTH_BONUS = 5,
    WEEKLY_SITE_STOCK_LIMIT = 100,
    WEEKLY_WATER_WHEEL_EMPTY = 0xff,
    WEEKLY_WATER_WHEEL_GOLD = 2,
    MONTHLY_CREATURE_GROWTH_FACTOR = 2
H1_ENUM_CONST_END(GameWeeklyConstant)

// spellwin.bin widget ids shared by ViewSpellsHandler, ViewSpecialHandler
// and CombatSpecialHandler (gSpellHelp rows 0..3 describe 2..5); entries
// 6..9 are the visible spells and 10..13 their labels (UpdateSpellWidgets).
H1_ENUM_ID_BEGIN(SpellBookControl)
    SPELL_BOOK_PREVIOUS_PAGE = 2,
    SPELL_BOOK_NEXT_PAGE = 3,
    SPELL_BOOK_ADVENTURE_SPELLS = 4,
    SPELL_BOOK_COMBAT_SPELLS = 5,
    SPELL_BOOK_ENTRY_FIRST = 6,
    SPELL_BOOK_ENTRY_LAST = 9,
    SPELL_BOOK_LABEL_FIRST = 10
H1_ENUM_ID_END(SpellBookControl)

// campaign.bin widget ids; the progress icon shows scenarios won + PROGRESS_FRAME_BASE. game::ShowCampaignInfo
// fills them; KB's EventWindowHandler restarts the scenario on RESTART.
H1_ENUM_ID_BEGIN(CampaignInfoControl)
    CAMPAIGN_INFO_NAME = 1,
    CAMPAIGN_INFO_TEXT = 2,
    CAMPAIGN_INFO_PROGRESS = 3,
    CAMPAIGN_INFO_PROGRESS_FRAME_BASE = 4,
    CAMPAIGN_INFO_RESTART = 0x385
H1_ENUM_ID_END(CampaignInfoControl)

// game::m_campaignScenario: scenarios LORD_FIRST..LORD_LAST are the four
// rival-lord scenarios, one per CampaignChoice in order; KB's scenario
// advance skips the player's own lord. In them the human starts with one
// hero and no town (NewMap), and a placed town the human owns takes the
// crest's race (RandomizeTown).
H1_ENUM_CONST_BEGIN(CampaignScenarioConstant)
    CAMPAIGN_SCENARIO_LORD_FIRST = 4,
    CAMPAIGN_SCENARIO_LORD_LAST = 7
H1_ENUM_CONST_END(CampaignScenarioConstant)

// game::m_mapSounds entry of a cell without an environment sound; new and
// loaded games clear the table to it and EraseObj resets erased cells.
H1_ENUM_CONST_BEGIN(MapSoundConstant)
    MAP_SOUND_NONE = -1
H1_ENUM_CONST_END(MapSoundConstant)
// The spell book shows four spells a page (entries FIRST..LAST).
H1_ENUM_CONST_BEGIN(SpellBookConstant)
    SPELL_BOOK_PAGE_SIZE = 4
H1_ENUM_CONST_END(SpellBookConstant)

// ComputeUALoc: a player sees the ultimate artifact's hint only after eleven
// puzzle pieces are removed, four percent per further piece; a missed roll scatters the
// hint up to three cells (3 - three 0..2 rolls) for at most 200 tries.
H1_ENUM_CONST_BEGIN(UltimateHintConstant)
    ULTIMATE_HINT_PIECE_MIN = 11,
    ULTIMATE_HINT_PERCENT_PER_PIECE = 4,
    ULTIMATE_HINT_SCATTER = 3,
    ULTIMATE_HINT_PLACE_TRIES = 200,
    // VisitObelisk's fallback piece search.
    OBELISK_PIECE_PICK_TRIES = 100
H1_ENUM_CONST_END(UltimateHintConstant)

// RandomizeHeroPool / SetRandomHeroArmies:
// starting experience 40 + 0..50, the strong-army flag (PHILAI's hires), the
// chance of the second and third table stacks (50/25 percent, +30/+40 for a
// strong army) and the counts drawn in tenths (min * 10 .. max * 10 + 9).
// armyTable rows: per hero class three (creature, min, max) options, of
// which the first two are drawn; unused slots get count -1. The first
// option is always present, the second rolls FIRST_STACK_CHANCE and the
// third SECOND_STACK_CHANCE (the second is forced when the third fails).
H1_ENUM_CONST_BEGIN(GameRandomHeroConstant)
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
H1_ENUM_CONST_END(GameRandomHeroConstant)

// game::GetLuck clamps a hero's luck to -3..3.
H1_ENUM_CONST_BEGIN(GameLuckConstant)
    GAME_LUCK_MIN = -3,
    GAME_LUCK_MAX = 3
H1_ENUM_CONST_END(GameLuckConstant)

// Save files: GenerateStandardFileName keeps an 8.3 base name (stopping its
// scan by jumping the index to SCAN_STOP); SaveGame keeps the save name
// unless the file is the 8-character AUTOSAVE or PLYREXIT.
H1_ENUM_CONST_BEGIN(SaveFileConstant)
    SAVE_FILE_BASE_NAME_LENGTH = 8,
    SAVE_FILE_NAME_SCAN_STOP = 999
H1_ENUM_CONST_END(SaveFileConstant)

// ComputeDailyGold strides mines by seven bytes from game+0x14341 with the
// owner at +1 and the type at +2.
#pragma pack(push, 1)
struct mineRecord {
    i8 id;
    i8 owner;
    // The resource the mine produces. The two unique sites' records
    // (MINE_SLOT_DRAGON_CITY, MINE_SLOT_LIGHTHOUSE) hold their MapObjectType
    // in this byte instead (ClaimMine switches on both encodings).
    H1_ENUM_STORAGE(ResourceType, i8) type;
    i8 guardianType;
    u8 guardianCount;
    // GetMineId sign-extends both coordinates.
    i8 x;
    i8 y;
};
#pragma pack(pop)

// CreateBoat fills eight-byte records from game+0x14486 (direction 2, owner
// at +7).
#pragma pack(push, 1)
struct boatRecord {
    i8 id;
    i8 x;
    i8 y;
    H1_ENUM_STORAGE(MapDirection, i8) direction;
    u8 savedTriggerType;
    u8 savedEventData;
    i8 heroId;
    i8 owner;
};
#pragma pack(pop)

// boatRecord::heroId: when a hero lands, the cursor walk ORs VACATED_FLAG
// into the boat's hero id; SummonBoat looks for the current
// hero's vacated boat, then any vacated boat of the player.
H1_ENUM_CONST_BEGIN(BoatRecordConstant)
    BOAT_VACATED_FLAG = 0x80
H1_ENUM_CONST_END(BoatRecordConstant)

// The map file's town records (LoadMap): a type byte whose low seven bits
// are the TownType and whose sign bit marks a castle. A customized
// mapTownExtra's owner is UNSET (-2) when the map leaves it open; SetupTowns
// copies only the buildings in EXTRA_BUILDING_MASK (every slot but the tent
// and castle bits, which the record's castle flag decides).
H1_ENUM_CONST_BEGIN(MapTownRecordConstant)
    MAP_TOWN_TYPE_MASK = 0x7f,
    MAP_TOWN_CASTLE_FLAG = 0x80,
    MAP_TOWN_OWNER_UNSET = -2,
    MAP_TOWN_EXTRA_BUILDING_MASK = 0x1f9f,
    // mapHeroExtra::artifacts: a placed hero's four starting artifacts.
    MAP_HERO_EXTRA_ARTIFACT_COUNT = 4
H1_ENUM_CONST_END(MapTownRecordConstant)

// SetupTowns and RandomizeTown read a town's map-extra record: custom flag,
// owner, buildings, mage-guild level and garrison.
#pragma pack(push, 1)
struct mapTownExtra {
    b8 customized;
    i8 owner;
    i16 buildings;
    i8 buildState;
    H1_ENUM_STORAGE(CreatureType, i8) troopTypes[ARMY_GROUP_SLOT_COUNT];
    i16 troopCounts[ARMY_GROUP_SLOT_COUNT];
};
#pragma pack(pop)

// ProcessOnMapHeroes reads a placed hero's map-extra record: owner,
// garrison, hero id, four artifacts and starting experience.
#pragma pack(push, 1)
struct mapHeroExtra {
    i8 owner;
    H1_ENUM_STORAGE(CreatureType, i8) troopTypes[ARMY_GROUP_SLOT_COUNT];
    i16 troopCounts[ARMY_GROUP_SLOT_COUNT];
    i8 heroId;
    H1_ENUM_STORAGE(ArtifactType, i8) artifacts[MAP_HERO_EXTRA_ARTIFACT_COUNT];
    i32 experience;
};
#pragma pack(pop)

// Player records (0x105 bytes at 0x20c), the embedded 72x72 world map at
// 0x620, towns (0x37 bytes at 0x121a1) and heroes (0xb6 bytes at 0x12985)
// are fixed by retail address arithmetic; unrecovered spans stay opaque.
#pragma pack(push, 1)
class game {
public:
    // ShowCongrats scales the base score by this percentage.
    i16 m_difficultyRating;
    i8 m_unused0002;
    // ControlPanel's scenario-info choice shows the campaign when positive.
    i32 m_campaignType;
    i32 m_campaignScenario;
    // Incremented per campaign victory; names the SCENWN%02d save and picks
    // the campaign-info frame.
    i32 m_campaignScenariosWon;
    // InitEntireCampaign starts it at 1; InitCampaignMap derives the
    // calendar from it.
    i32 m_campaignDay;
    // NewGame copies the chosen map's size, difficulty, title and
    // description; ShowCongrats files the title with the high score.
    H1_ENUM_STORAGE(MapSize, i8) m_mapSize;
    H1_ENUM_STORAGE(MapDifficulty, i8) m_mapDifficulty;
    char m_mapName[0x11];
    char m_mapDescription[0x79];
    // SaveGame/LoadGame and the save requester's default name.
    char m_saveName[0x15f];
    // InitEntireCampaign stores DIFFICULTY_EXPERT here.
    H1_ENUM_STORAGE(GameDifficulty, i8) m_difficulty;
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
    // ClaimTown mirrors each town owner into this byte array.
    i8 m_townOwners[GAME_TOWN_COUNT];
    u8 m_townBuiltToday[4];
    class hero m_heroRecs[GAME_HERO_COUNT];
    i8 m_heroOwners[GAME_HERO_COUNT];
    mineRecord m_mines[GAME_MINE_COUNT];
    // ClaimMine mirrors each mine owner into this byte array.
    i8 m_mineOwners[GAME_MINE_COUNT];
    // GetRandomArtifactId scans artifacts 4..36 for a free (-1) entry.
    H1_ENUM_ARRAY(i8, m_artifactHolders, ArtifactType, ARTIFACT_REGULAR_END);
    boatRecord m_boats[GAME_BOAT_COUNT];
    i8 m_boatSlots[GAME_BOAT_COUNT];
    // Obelisk events test and set the visiting player bit, one byte per obelisk.
    i8 m_obeliskVisitors[0x30];
    // InsertSound reads the environment sound id per [x][y] cell (MAP_SOUND_NONE
    // when silent).
    i8 m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    // GetCloudLookup tests the watching player bit per [x][y] cell.
    u8 m_mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    i8 m_ultimateArtifactX;
    i8 m_ultimateArtifactY;
    H1_ENUM_STORAGE(ArtifactType, i8) m_ultimateArtifactId;
    // NewGame's newgame.bin window.
    class heroWindow* m_newGameWindow;
    i8 m_unused16e5d;
    // ViewArmy's open army window; ViewArmyHandler animates it.
    class heroWindow* m_viewArmyWindow;
    // TavernHandler advances this word as its animation counter.
    i16 m_dialogAnimationCounter;
    // InitMainClasses allocates 0x16e7a bytes for the game object.
    // ViewSpells' window state: the hero's spell slots
    // run from m_spellFirst to m_spellLast, four per page from m_viewSpellsTop.
    class heroWindow* m_viewSpellsWindow;
    class hero* m_viewSpellsHero;
    i16 m_spellFirst;
    i16 m_spellLast;
    H1_ENUM_STORAGE(SpellType, i16) m_viewSpell;
    i16 m_viewSpellsTop;
    H1_ENUM_RETURN(MessageDispatchResult, i16) (*m_viewSpellsCallback)(struct tag_message&);
    i8 m_viewSpellsReadOnly;
    // LoadGame sets it; ProcessMapExtra clears it for a 0xc7 (map hero)
    // trigger cell. While set, every player starts with a town hero;
    // otherwise the map's heroes are processed.
    b8 m_noMapHeroes;
    hero* GetHero(i8 id) {
        return &m_heroRecs[id];
    }
    // TownEvent passes the unsigned cell metadata through a signed byte.
    town* GetTown(i8 id) {
        return &m_castleRecs[id];
    }
    hero* GetPlayerHero(i32 player, i32 index) {
        return &m_heroRecs[m_players[player].m_heroIds[index]];
    }
    town* GetPlayerTown(i32 player, i32 index) {
        return &m_castleRecs[m_players[player].m_townIds[index]];
    }
    // --- methods ---
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
    // Retail InitCampaignMap takes the scenario and an unused int (ret 8).
    void InitCampaignMap(i32 scenario, i32);
    b8 IsMobile(i8 heroId);
    class mapCell (*GetWorldMapData(void))[MAP_CELL_GRID_SIZE];
    // Inline world-map file I/O (LoadMap, SaveGame, LoadGame).
    void ReadWorldMap(i32 fd);
    void WriteWorldMap(i32 fd);
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
    H1_ENUM_RETURN(SpellType, i8) ViewSpells(
        class hero* spellHero,
        H1_ENUM_PARAM(HeroSpellType, i8) spellType,
        H1_ENUM_RETURN(MessageDispatchResult, i16) (*callback)(struct tag_message&),
        i8 readOnly
    );
    // Limits the spell page to the combat or adventure slots.
    void SetupSpellRange(H1_ENUM_PARAM(HeroSpellType, i16) spellType);
    void UpdateSpellWidgets(void);
    void ViewArmy(
        i16 x,
        i16 y,
        H1_ENUM_PARAM(CreatureType, i8) monsterType,
        i16 numTroops,
        class town* castle,
        b8 disableDismiss,
        H1_ENUM_PARAM(ArmyFacing, i8) facing,
        i8 quickView,
        class hero* theHero,
        class army* theArmy,
        class armyGroup* theGroup
    );
    i8 GetRandomNumTroops(H1_ENUM_PARAM(CreatureType, i8) monsterType);
    void TurnOnAIMusic(void);
    void TurnOffAIMusic(void);
    void NextPlayer(void);
    i32 ComputeDailyGold(i32 player);
    void PerDay(void);
    void PerWeek(void);
    void PerMonth(void);
    void RandomizeTown(i8 x, i8 y, b8 isCastle);
    void RandomizeMine(i8 x, i8 y);
    // Default dwellings and mage-guild spells.
    void SetupTown(i8 townId, b8 aiOwned);
    H1_ENUM_RETURN(ArtifactType, i8) GetRandomArtifactId(void);
    void RandomizeHeroPool(void);
    void SetRandomHeroArmies(i16 heroId, i32 strongArmy);
    void ProcessRandomObjects(b32 castlesOnly);
    void SetVisibility(i16 x, i16 y, i16 player, i16 radius);
    void
    GiveArmy(class armyGroup* group, H1_ENUM_PARAM(CreatureType, i32) type, i32 count, i32 slot);
    i32 ExperienceValueOfStack(class armyGroup* group, class hero* heroPointer);
    i32 GetLuck(class hero* heroPointer, class army*);
    // Enemy-turn crest reads widen the stored color to a signed short.
    H1_ENUM_RETURN(PlayerColor, i16) GetPlayerColor(i32 player) {
        return m_players[player].m_color;
    }
    void SetupAdjacentMons(void);
    void CancelComputerScreen(void);
    void ShowComputerScreen(void);
    void ShowHeroesLogo(void);
    void WaitForPlayer(char* text, i32 player);
    // Once a cell's object frame is gone,
    // pulls its overlay frame down into the object layer.
    void SettleOverlay(i32 x, i32 y);
    // NewMap rerolls each cell's terrain tile variant after LoadMap.
    void RandomizeTerrainTiles(void);
    void ProcessMapExtra(void);
    // Retail returns whether no town took an owner from its map extra (AL).
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
    void ShowMoraleInfo(class hero* heroPointer, H1_ENUM_PARAM(NormalDialogType, i32) dialogType);
    void ShowLuckInfo(class hero* heroPointer, H1_ENUM_PARAM(NormalDialogType, i32) dialogType);
    // Retail GetMap never reads ecx; its caller passes no this.
    static void GetMap(void);
    // Retail returns the started flag in AL.
    i8 NewGame(void);
    void UpdateNewGameWindow(void);
    void ShowScenInfo(void);
    // NewMap gives every opponent a distinct crest.
    void RandomizePlayerCrests(void);
    // DoEvent's obelisk branch (byte player, ret 4).
    void VisitObelisk(i8 player);
};
#pragma pack(pop)

// The one-based day number of game g's calendar, day first; the u16 fields
// promote to int.
#define GAME_DAY_NUMBER(g)                                                                         \
    ((g).m_day + ((g).m_week - 1) * CALENDAR_DAYS_PER_WEEK                                         \
     + ((g).m_month - 1) * CALENDAR_DAYS_PER_MONTH)

// Recomputes a player's ultimate-artifact hint (cdecl, int player).
void ComputeUALoc(i32 player);
// GAME's dialog handlers and the standard-game day score ShowCongrats files.
H1_ENUM_RETURN(MessageDispatchResult, i16) ViewSpellsHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) ViewSpecialHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) ViewArmyHandler(struct tag_message& message);
i32 GetBaseScore(i32 days);
extern b32 gGameOver;
// SaveGame files the current player through this byte.
extern i8 gSavedCurPlayer;
// NewGame remembers the last new-game settings for the next setup screen.

#define gSavedDifficulty gOldGameDifficulty // spelling fixes .bss order
extern H1_ENUM_STORAGE(GameDifficulty, i8) gSavedDifficulty;
extern i8 gSavedDifficulties[];
extern i8 gSavedKingOfTheHill;

#define gSavedCrest gKeptColor // spelling fixes .bss order
extern H1_ENUM_STORAGE(PlayerColor, i8) gSavedCrest;
extern H1_ENUM_STORAGE(TownType, i8) gRandomTownTypes[GAME_PLAYER_COUNT];
#define gMineTypeCount gMineTypeNums // spelling fixes .bss order
extern H1_ENUM_ARRAY(i16, gMineTypeCount, ResourceType, RESOURCE_COUNT);
extern i32 gLastSeed;
i32 SGenRand(void);
i32 SRandom(i32 low, i32 high);
void SIncRandomize(i32 x, i32 y);
void SRand(i32 seed);
// GetMap raises gShowMapInfo around its .MAP requester and owns the
// reqextra.bin side window the requester fills.
extern b8 gShowMapInfo;
extern heroWindow* gReqExtraWindow;
extern char gCurMapName[];
#define gDismissArmy gbDismissArmy // spelling fixes .bss order
extern b8 gDismissArmy;

// Moved from GAME.cpp.
// newgame.bin widget ids. The opponent toggles are players 1..3 (id - 1);
// difficulty buttons are FIRST + game::m_difficulty. OK and CANCEL are role
// names on the reserved dialog slots (gNewGameHelp: 0x7802 accepts, 0x7801
// returns to the main menu).
H1_ENUM_ID_BEGIN(NewGameControl)
    NEW_GAME_OPPONENT_FIRST = 2,
    NEW_GAME_OPPONENT_LAST = 4,
    NEW_GAME_COLOR = 8,
    NEW_GAME_SCENARIO_SELECT = 0xc,
    NEW_GAME_DIFFICULTY_FIRST = 0xd,
    NEW_GAME_DIFFICULTY_LAST = 0x10,
    NEW_GAME_SCENARIO_NAME = 0x11,
    NEW_GAME_SCENARIO_PANEL = 0x12,
    NEW_GAME_KING_OF_THE_HILL = 0x13,
    NEW_GAME_RATING = 0x14,
    NEW_GAME_CANCEL = DIALOG_BUTTON_1,
    NEW_GAME_OK = DIALOG_BUTTON_2,
    // Player p's type toggle is p + TOGGLE_BASE (ids 2..4) and its type label
    // p + LABEL_BASE (ids 5..7).
    NEW_GAME_OPPONENT_TOGGLE_BASE = 1,
    NEW_GAME_OPPONENT_LABEL_BASE = 4
H1_ENUM_ID_END(NewGameControl)

// sceninfo.bin widget ids ShowScenInfo fills: the map's size, difficulty
// level and description (the roles reqextra.bin's FILE_REQUESTER_MAP_* ids
// hold in the file requester), the human's crest, the map name, the game
// difficulty, the opponents list, King of the Hill and the difficulty rating.
H1_ENUM_ID_BEGIN(ScenarioInfoControl)
    SCENARIO_INFO_MAP_SIZE = 100,
    SCENARIO_INFO_MAP_LEVEL = 101,
    SCENARIO_INFO_MAP_DESCRIPTION = 102,
    SCENARIO_INFO_CREST = 103,
    SCENARIO_INFO_MAP_NAME = 104,
    SCENARIO_INFO_DIFFICULTY = 105,
    SCENARIO_INFO_OPPONENTS = 106,
    SCENARIO_INFO_KING_OF_THE_HILL = 107,
    SCENARIO_INFO_RATING = 108
H1_ENUM_ID_END(ScenarioInfoControl)

// newgame.icn frames UpdateNewGameWindow selects: the human-opponent face,
// the computer-type faces (type + base), the crests (two per color) and
// the King of the Hill toggle (flag + base). sceninfo.bin's crest
// (SCENARIO_INFO_CREST) is a newgame.icn widget too, so ShowScenInfo picks
// the same crest frames.
H1_ENUM_CONST_BEGIN(NewGameFrame)
    NEW_GAME_FRAME_COMPUTER_TYPE_BASE = 5,
    NEW_GAME_FRAME_CREST_BASE = 11,
    NEW_GAME_FRAME_CREST_STRIDE = 2,
    NEW_GAME_FRAME_HUMAN_OPPONENT = 0x1a,
    NEW_GAME_FRAME_KING_OF_THE_HILL_BASE = 27
H1_ENUM_CONST_END(NewGameFrame)

// NewGameHandler's right-click help: the gNewGameHelp row shown.
H1_ENUM_BEGIN(NewGameHelp)
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
H1_ENUM_END(NewGameHelp)

// GiveTroopsToNeutralTowns: a 1..15 roll picks the tier, whose key plus the
// town type selects the recruit and whose range the count.
H1_ENUM_CONST_BEGIN(NeutralTownReinforcementConstant)
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
H1_ENUM_CONST_END(NeutralTownReinforcementConstant)

// armywin.bin widget ids; dismiss is DIALOG_BUTTON_3 and close
// DIALOG_BUTTON_0. The animation icon cycles
// VIEW_ARMY_ANIMATION_FRAMES frames every VIEW_ARMY_FRAME_DELAY ticks.
H1_ENUM_ID_BEGIN(ViewArmyControl)
    VIEW_ARMY_COUNT_FRAME = 1,
    VIEW_ARMY_COUNT_TEXT = 2,
    VIEW_ARMY_TITLE = 3,
    VIEW_ARMY_STATS = 4,
    VIEW_ARMY_ANIMATION = 5,
    VIEW_ARMY_DISMISS = DIALOG_BUTTON_3,
    VIEW_ARMY_CLOSE = DIALOG_BUTTON_0
H1_ENUM_ID_END(ViewArmyControl)

H1_ENUM_CONST_BEGIN(ViewArmyConstant)
    VIEW_ARMY_ANIMATION_FRAMES = 6,
    VIEW_ARMY_FRAME_DELAY = 90,
    VIEW_ARMY_STAT_TEXT_SIZE = 550
H1_ENUM_CONST_END(ViewArmyConstant)

// overwind.bin widget ids: resource r's count is RESOURCE_BASE + r.
H1_ENUM_ID_BEGIN(OverviewControl)
    OVERVIEW_RESOURCE_BASE = 1,
    OVERVIEW_DATE = 64,
    OVERVIEW_DAILY_GOLD = 65
H1_ENUM_ID_END(OverviewControl)

// Calendar draws: gWeekNames / gMonthNames sizes, the creature tables a
// creature week or month picks from, and gNewTurnText's announcement rows.
H1_ENUM_CONST_BEGIN(CalendarConstant)
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
H1_ENUM_CONST_END(CalendarConstant)

// Ground tiles come in groups of four interchangeable variants;
// RandomizeTerrainTiles rerolls the variant within its group.
H1_ENUM_CONST_BEGIN(TerrainTileConstant)
    TERRAIN_TILE_VARIANT_COUNT = 4
H1_ENUM_CONST_END(TerrainTileConstant)

// REMOTE.GAM transfer: the sender announces the
// size (BOX_REMOTE_SAVE, answered by REMOTE_COMMAND_SAVE_INIT_RESPONSE),
// streams SEGMENT_SIZE-byte segments (SAVE_DATA), asks for each
// BATCH_SIZE-segment block's acknowledgement map (SAVE_ACK_REQUEST /
// SAVE_ACK_RESPONSE) and closes with SAVE_FINISH (RemoteCommand).
H1_ENUM_CONST_BEGIN(RemoteSaveConstant)
    REMOTE_SAVE_SEGMENT_SIZE = 200,
    REMOTE_SAVE_BATCH_SIZE = 100,
    REMOTE_SAVE_HEADER_SIZE = 8,
    REMOTE_SAVE_ACK_MAP_SIZE = 200,
    REMOTE_SAVE_INDEX_SIZE = 2,
    REMOTE_SAVE_BUFFER_EXTRA = 500,
    REMOTE_SAVE_DECODE_BUFFER_SIZE = 0x130b0,
    REMOTE_SAVE_TRANSFER_SOUNDS = 8
H1_ENUM_CONST_END(RemoteSaveConstant)

// Human player counts the map filenames encode (digit 4..7 of "????1234.MAP")
// and opponent counts CalcDifficultyRating rates.
H1_ENUM_CONST_BEGIN(GamePlayerCount)
    GAME_PLAYERS_TWO = 2,
    GAME_PLAYERS_THREE = 3,
    GAME_PLAYERS_FOUR = 4
H1_ENUM_CONST_END(GamePlayerCount)

// PerMonth's month of a creature: each empty land cell spawns that creature
// when Random(0, ROLL_MAX) hits ROLL_HIT.
H1_ENUM_CONST_BEGIN(MonthCreatureSpawnConstant)
    MONTH_CREATURE_SPAWN_ROLL_MAX = 360,
    MONTH_CREATURE_SPAWN_ROLL_HIT = 10
H1_ENUM_CONST_END(MonthCreatureSpawnConstant)

#endif // HOMM1_SOURCE_GAME_H
