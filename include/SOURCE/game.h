#ifndef HOMM1_SOURCE_GAME_H
#define HOMM1_SOURCE_GAME_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 114 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
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
// gDifficultyNames ("Easy", "Normal", "Hard", "Expert"); Buka GameDifficulty.
H1_ENUM_BEGIN(GameDifficulty)
    DIFFICULTY_EASY = 0,
    DIFFICULTY_NORMAL = 1,
    DIFFICULTY_HARD = 2,
    DIFFICULTY_EXPERT = 3,
    DIFFICULTY_COUNT = 4
H1_ENUM_END(GameDifficulty)

// giWeekType / giMonthType (Buka CalendarPeriodType): a named week or month
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
H1_ENUM_BEGIN(GameMineSlot)
    MINE_SLOT_DRAGON_CITY = 0,
    MINE_SLOT_LIGHTHOUSE = 1,
    // The ordinary mines follow the two unique sites (PerDay, Overview,
    // ComputeDailyGold loop from here).
    MINE_SLOT_STANDARD_FIRST = 2
H1_ENUM_END(GameMineSlot)

// A mine object covers 2x2 cells from its record's (x, y - 1) to (x + 1, y)
// (RandomizeEvents, RandomizeMine).
H1_ENUM_CONST_BEGIN(MineFootprintConstant)
    MINE_FOOTPRINT_WIDTH = 2,
    MINE_FOOTPRINT_HEIGHT = 2
H1_ENUM_CONST_END(MineFootprintConstant)

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

// Weekly growth (PerWeek/PerMonth, Buka GameWeeklyConstant): a well adds two
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
H1_ENUM_BEGIN(SpellBookControl)
    SPELL_BOOK_PREVIOUS_PAGE = 2,
    SPELL_BOOK_NEXT_PAGE = 3,
    SPELL_BOOK_ADVENTURE_SPELLS = 4,
    SPELL_BOOK_COMBAT_SPELLS = 5,
    SPELL_BOOK_ENTRY_FIRST = 6,
    SPELL_BOOK_ENTRY_LAST = 9,
    SPELL_BOOK_LABEL_FIRST = 10
H1_ENUM_END(SpellBookControl)

// campaign.bin widget ids (Buka CampaignControlId spells RESTART 0x385); the
// progress icon shows scenarios won + PROGRESS_FRAME_BASE. game::ShowCampaignInfo
// fills them; KB's EventWindowHandler restarts the scenario on RESTART.
H1_ENUM_BEGIN(CampaignInfoControl)
    CAMPAIGN_INFO_NAME = 1,
    CAMPAIGN_INFO_TEXT = 2,
    CAMPAIGN_INFO_PROGRESS = 3,
    CAMPAIGN_INFO_PROGRESS_FRAME_BASE = 4,
    CAMPAIGN_INFO_RESTART = 0x385
H1_ENUM_END(CampaignInfoControl)

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
// obelisks, four percent per further obelisk; a missed roll scatters the
// hint up to three cells (3 - three 0..2 rolls) for at most 200 tries.
H1_ENUM_CONST_BEGIN(UltimateHintConstant)
    ULTIMATE_HINT_OBELISK_MIN = 11,
    ULTIMATE_HINT_PERCENT_PER_OBELISK = 4,
    ULTIMATE_HINT_SCATTER = 3,
    ULTIMATE_HINT_PLACE_TRIES = 200,
    // VisitObelisk's fallback piece search.
    OBELISK_PIECE_PICK_TRIES = 100
H1_ENUM_CONST_END(UltimateHintConstant)

// RandomizeHeroPool / SetRandomHeroArmies (Buka GameRandomHeroConstant):
// starting experience 40 + 0..50, the strong-army flag (PHILAI's hires), the
// chance of the second and third table stacks (50/25 percent, +30/+40 for a
// strong army) and the counts drawn in tenths (min * 10 .. max * 10 + 9).
// armyTable rows: per hero class three (creature, min, max) options, of
// which the first two are drawn; unused slots get count -1.
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
// owner at +1 and the type at +2, as in HoMM2's mineRecord.
#pragma pack(push, 1)
struct mineRecord {
    i8 id;
    i8 owner;
    i8 type;
    i8 guardianType;
    u8 guardianCount;
    // GetMineId sign-extends both coordinates.
    i8 x;
    i8 y;
};
#pragma pack(pop)

// CreateBoat fills eight-byte records from game+0x14486 in HoMM2's
// boatRecord order (direction 2, owner at +7).
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

// The map file's town records (LoadMap): a type byte whose low seven bits
// are the TownType and whose sign bit marks a castle. A customized
// mapTownExtra's owner is UNSET (-2) when the map leaves it open; SetupTowns
// copies only the buildings in EXTRA_BUILDING_MASK (every slot but the tent
// and castle bits, which the record's castle flag decides).
H1_ENUM_CONST_BEGIN(MapTownRecordConstant)
    MAP_TOWN_TYPE_MASK = 0x7f,
    MAP_TOWN_OWNER_UNSET = -2,
    MAP_TOWN_EXTRA_BUILDING_MASK = 0x1f9f,
    // mapHeroExtra::artifacts: a placed hero's four starting artifacts.
    MAP_HERO_EXTRA_ARTIFACT_COUNT = 4
H1_ENUM_CONST_END(MapTownRecordConstant)

// SetupTowns and RandomizeTown read a town's map-extra record: custom flag,
// owner, buildings, mage-guild level and garrison.
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

// ProcessOnMapHeroes reads a placed hero's map-extra record: owner,
// garrison, hero id, four artifacts and starting experience.
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

// Player records (0x105 bytes at 0x20c), the embedded 72x72 world map at
// 0x620, towns (0x37 bytes at 0x121a1) and heroes (0xb6 bytes at 0x12985)
// are fixed by retail address arithmetic; unrecovered spans stay opaque.
#pragma pack(push, 1)
class game {
public:
    // ShowCongrats scales the base score by this percentage.
    i16 m_difficultyRating;
    i8 m_unknown0002;
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
    i8 m_mapSize;
    i8 m_mapDifficulty;
    char m_mapName[0x11];
    char m_mapDescription[0x79];
    // SaveGame/LoadGame and the save requester's default name.
    char m_saveName[0x15f];
    // InitEntireCampaign stores 3 here.
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
    // ClaimTown mirrors each town owner into this byte array.
    i8 m_townOwners[GAME_TOWN_COUNT];
    u8 m_townBuiltToday[4];
    class hero m_heroRecs[GAME_HERO_COUNT];
    i8 m_availableHeroes[GAME_HERO_COUNT];
    mineRecord m_mines[GAME_MINE_COUNT];
    // ClaimMine mirrors each mine owner into this byte array.
    i8 m_mineOwners[GAME_MINE_COUNT];
    // GetRandomArtifactId scans artifacts 4..36 for a free (-1) entry.
    i8 m_randomArtifacts[0x25];
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
    i8 m_ultimateArtifactId;
    // NewGame's newgame.bin window.
    class heroWindow* m_newGameWindow;
    i8 m_unknown16e5d;
    // ViewArmy's open army window; ViewArmyHandler animates it.
    class heroWindow* m_viewArmyWindow;
    // TavernHandler advances this word as its animation counter (Buka name).
    i16 m_viewArmyResult;
    // InitMainClasses allocates 0x16e7a bytes for the game object.
    // ViewSpells' window state (Buka m_viewSpells*): the hero's spell slots
    // run from m_spellFirst to m_spellLast, four per page from m_viewSpellsTop.
    class heroWindow* m_viewSpellsWindow;
    class hero* m_viewSpellsHero;
    i16 m_spellFirst;
    i16 m_spellLast;
    i16 m_viewSpell;
    i16 m_viewSpellsTop;
    i16 (*m_viewSpellsCallback)(struct tag_message&);
    i8 m_viewSpellsReadOnly;
    // LoadGame sets it; ProcessMapExtra clears it for a 0xc7 (map hero)
    // trigger cell. While set, every player starts with a town hero;
    // otherwise the map's heroes are processed.
    i8 m_noMapHeroes;
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
    // Retail InitCampaignMap takes the scenario and an unused int (ret 8).
    void InitCampaignMap(i32 scenario, i32);
    i32 MineTypesOwned(i32 owner, i32 mineType);
    i32 SetupPuzzlePieces(i32 player, i32 justCount);
    i8 IsMobile(i8 heroId);
    class mapCell (*GetWorldMapData(void))[MAP_CELL_GRID_SIZE];
    // Inline world-map file I/O (LoadMap, SaveGame, LoadGame): each
    // expansion leaves its jmp $+0 after the read or write call.
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
    // HoMM1 retail returns 1 in AX (ret 0xc).
    i16 LoadGame(char* filename, i32 origData, i32);
    void GiveTroopsToNeutralTown(i32 townId);
    void GiveTroopsToNeutralTowns(void);
    void NewMap(char* mapName);
    void RandomizeEvents(void);
    void InitializePasswords(void);
    void RandomizeBarrier(class mapCell* cell);
    void RandomizePassword(class mapCell* cell);
    // HoMM1 retail returns 0 in AX.
    i16 LoadMap(char* filename);
    void ClaimTown(i8 townId, i8 player);
    void ClaimMine(i8 mineId, i8 player);
    // HoMM1 retail: byte spell type and read-only flag, spell in AL (ret 0x10).
    i8 ViewSpells(
        class hero* spellHero,
        H1_ENUM_PARAM(HeroSpellType, i8) spellType,
        i16 (*callback)(struct tag_message&),
        i8 readOnly
    );
    // HoMM1: limits the spell page to the combat or adventure slots.
    void SetupSpellRange(H1_ENUM_PARAM(HeroSpellType, i16) spellType);
    void UpdateSpellWidgets(void);
    // HoMM1 retail: word x/y, byte creature/flags, word count, eleven
    // arguments (ret 0x2c); combatManager::ViewArmy pushes its word locals
    // unextended and the body hands them to heroWindow(short, short, char*).
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
    // HoMM1 retail: byte creature, count returned in AL.
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
    // HoMM1 retail: byte x, y and castle flag (ret 0xc).
    void RandomizeTown(i8 x, i8 y, i8 isCastle);
    // HoMM1 retail: byte x and y (ret 8).
    void RandomizeMine(i8 x, i8 y);
    // HoMM1 retail 0x00442fb4 (ret 8): default dwellings and mage-guild spells.
    void SetupTown(i8 townId, i8 aiOwned);
    void InitRandomArtifacts(void);
    i8 GetRandomArtifactId(void);
    void RandomizeHeroPool(void);
    void SetRandomHeroArmies(i16 heroId, i32 strongArmy);
    // HoMM1 retail: towns-only pass flag (ret 4).
    void ProcessRandomObjects(i32 castlesOnly);
    void SetVisibility(i16 x, i16 y, i16 player, i16 radius);
    void MakeAllWaterVisible(i32 player);
    void GiveArmy(class armyGroup* group, i32 type, i32 count, i32 slot);
    i32 ExperienceValueOfStack(class armyGroup* group, class hero* h);
    // HoMM1 retail: hero and army only (ret 8).
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
    // HoMM1 retail 0x0043d4c3 (ret 8): once a cell's object frame is gone,
    // pulls its overlay frame down into the object layer.
    void SettleOverlay(i32 x, i32 y);
    // HoMM1: NewMap rerolls each cell's terrain tile variant after LoadMap.
    void RandomizeTerrainTiles(void);
    void ProcessMapExtra(void);
    // Retail returns whether no town took an owner from its map extra (AL).
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
    // Retail GetMap never reads ecx; its caller passes no this.
    static void GetMap(void);
    void ProcessNewMap(struct SMapHeader* header);
    void InitNewGame(struct SMapHeader* header);
    void SetupNetPlayerNames(void);
    // Retail returns the started flag in AL.
    i8 NewGame(void);
    void CleanUpNewGameWindow(void);
    void InitNewGameWindow(void);
    void UpdateNewGameWindow(void);
    i32 ProcessNGKeyPress(struct tag_message& message);
    void NGKPSetupDisplayString(char* text, u16 cursor);
    void DrawNGKPDisplayString(i32 updateScreen);
    void ShowScenInfo(void);
    // HoMM1: NewMap gives every opponent a distinct crest.
    void RandomizePlayerCrests(void);
    void GetLossConditionText(char* text);
    void GetVictoryConditionText(char* text);
    i32 GetSideDesc(char* text, i32 firstPlayer, i32 lastPlayer);
    // DoEvent's obelisk branch (byte player, ret 4).
    void VisitObelisk(i8 player);
};
#pragma pack(pop)

// Recomputes a player's ultimate-artifact hint (cdecl, int player).
void ComputeUALoc(i32 player);
// GAME's dialog handlers and the standard-game day score ShowCongrats files.
i16 ViewSpellsHandler(struct tag_message& message);
i16 ViewSpecialHandler(struct tag_message& message);
i16 ViewArmyHandler(struct tag_message& message);
i32 GetBaseScore(i32 days);
extern i32 gGameOver;
extern i32 gEndSequence;
// SaveGame files the current player through this byte.
extern i8 gSaveCurPlayer;
// NewGame remembers the last new-game settings for the next setup screen.
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
// GetMap raises gShowMapInfo around its .MAP requester and owns the
// reqextra.bin side window the requester fills.
extern i8 gShowMapInfo;
extern heroWindow* gReqExtraWindow;
extern char gCurMapName[];
extern i8 gbDismissArmy;

#endif // HOMM1_SOURCE_GAME_H
