#ifndef HOMM1_SOURCE_GAME_H
#define HOMM1_SOURCE_GAME_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 114 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
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

// clang-format off
H1_ENUM_CONST_BEGIN(GameStorageConstant)
    GAME_PLAYER_COUNT = 4,
    // A player id slot with no player (combatManager::m_playerId for a
    // neutral side; tested before gbHumanPlayer[] lookups).
    GAME_PLAYER_NONE = -1,
    GAME_TOWN_COUNT = 36,
    GAME_HERO_COUNT = 36,
    GAME_MINE_COUNT = 36,
    GAME_BOAT_COUNT = 32
H1_ENUM_CONST_END(GameStorageConstant)

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
    MINE_SLOT_LIGHTHOUSE = 1
H1_ENUM_END(GameMineSlot)

// spellwin.bin widget ids shared by ViewSpellsHandler, ViewSpecialHandler
// and CombatSpecialHandler (cSpellHelp rows 0..3 describe 2..5); entries
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
// clang-format on

// ComputeDailyGold strides mines by seven bytes from game+0x14341 with the
// owner at +1 and the type at +2, as in HoMM2's mineRecord.
#pragma pack(push, 1)
struct mineRecord {
    signed char id;
    signed char owner;
    signed char type;
    signed char guardianType;
    unsigned char guardianCount;
    // GetMineId sign-extends both coordinates.
    signed char x;
    signed char y;
};
#pragma pack(pop)

// CreateBoat fills eight-byte records from game+0x14486 in HoMM2's
// boatRecord order (direction 2, owner at +7).
#pragma pack(push, 1)
struct boatRecord {
    signed char id;
    signed char x;
    signed char y;
    signed char direction;
    unsigned char savedTriggerType;
    unsigned char savedEventData;
    signed char heroId;
    signed char owner;
};
#pragma pack(pop)

// SetupTowns and RandomizeTown read a town's map-extra record: custom flag, owner, buildings, mage-guild
// level and garrison.
#pragma pack(push, 1)
struct mapTownExtra {
    signed char customized;
    signed char owner;
    short buildings;
    signed char buildState;
    signed char troopTypes[5];
    short troopCounts[5];
};
#pragma pack(pop)

// ProcessOnMapHeroes reads a placed hero's map-extra record: owner,
// garrison, hero id, four artifacts and starting experience.
#pragma pack(push, 1)
struct mapHeroExtra {
    signed char owner;
    signed char troopTypes[5];
    short troopCounts[5];
    signed char heroId;
    signed char artifacts[4];
    int experience;
};
#pragma pack(pop)

// Player records (0x105 bytes at 0x20c), the embedded 72x72 world map at
// 0x620, towns (0x37 bytes at 0x121a1) and heroes (0xb6 bytes at 0x12985)
// are fixed by retail address arithmetic; unrecovered spans stay opaque.
#pragma pack(push, 1)
class game {
public:
    // ShowCongrats scales the base score by this percentage.
    short m_difficultyRating;
    char m_unknown0002;
    // ControlPanel's scenario-info choice shows the campaign when positive.
    int m_campaignType;
    int m_campaignScenario;
    // Incremented per campaign victory; names the SCENWN%02d save and picks
    // the campaign-info frame.
    int m_campaignScenariosWon;
    // InitEntireCampaign starts it at 1; InitCampaignMap derives the
    // calendar from it.
    int m_campaignDay;
    // NewGame copies the chosen map's size, difficulty, title and
    // description; ShowCongrats files the title with the high score.
    signed char m_mapSize;
    signed char m_mapDifficulty;
    char m_mapName[0x11];
    char m_mapDescription[0x79];
    // SaveGame/LoadGame and the save requester's default name.
    char m_saveName[0x15f];
    // InitEntireCampaign stores 3 here.
    signed char m_difficulty;
    signed char m_playerCount;
    char m_unknown200;
    signed char m_deadPlayerCount;
    signed char m_playerDead[GAME_PLAYER_COUNT];
    unsigned short m_day;
    unsigned short m_week;
    unsigned short m_month;
    class playerData m_players[GAME_PLAYER_COUNT];
    class mapCell m_map[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    char m_unknownd0a0[0x5100];
    signed char m_obeliskCount;
    class town m_castleRecs[GAME_TOWN_COUNT];
    // ClaimTown mirrors each town owner into this byte array.
    signed char m_townOwners[GAME_TOWN_COUNT];
    unsigned char m_townBuiltToday[4];
    class hero m_heroRecs[GAME_HERO_COUNT];
    signed char m_availableHeroes[GAME_HERO_COUNT];
    mineRecord m_mines[GAME_MINE_COUNT];
    // ClaimMine mirrors each mine owner into this byte array.
    signed char m_mineOwners[GAME_MINE_COUNT];
    // GetRandomArtifactId scans artifacts 4..36 for a free (-1) entry.
    signed char m_randomArtifacts[0x25];
    boatRecord m_boats[GAME_BOAT_COUNT];
    signed char m_boatSlots[GAME_BOAT_COUNT];
    // Obelisk events test and set the visiting player bit, one byte per obelisk.
    signed char m_obeliskVisitors[0x30];
    // InsertSound reads the environment sound id per [x][y] cell.
    signed char m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    // GetCloudLookup tests the watching player bit per [x][y] cell.
    unsigned char m_mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    signed char m_ultimateArtifactX;
    signed char m_ultimateArtifactY;
    signed char m_ultimateArtifactId;
    // NewGame's newgame.bin window.
    class heroWindow* m_newGameWindow;
    char m_unknown16e5d;
    // ViewArmy's open army window; ViewArmyHandler animates it.
    class heroWindow* m_viewArmyWindow;
    // TavernHandler advances this word as its animation counter (Buka name).
    short m_viewArmyResult;
    // InitMainClasses allocates 0x16e7a bytes for the game object.
    // ViewSpells' window state (Buka m_viewSpells*): the hero's spell slots
    // run from m_spellFirst to m_spellLast, four per page from m_viewSpellsTop.
    class heroWindow* m_viewSpellsWindow;
    class hero* m_viewSpellsHero;
    short m_spellFirst;
    short m_spellLast;
    short m_viewSpell;
    short m_viewSpellsTop;
    short (*m_viewSpellsCallback)(struct tag_message&);
    signed char m_viewSpellsReadOnly;
    // LoadGame sets it; ProcessMapExtra clears it for a 0xc7 (map hero)
    // trigger cell. While set, every player starts with a town hero;
    // otherwise the map's heroes are processed.
    signed char m_noMapHeroes;
    hero* GetHero(signed char id) {
        return &m_heroRecs[id];
    }
    // TownEvent passes the unsigned cell metadata through a signed byte.
    town* GetTown(signed char id) {
        return &m_castleRecs[id];
    }
    // --- methods ---
    void SetupDynamicStuff(int, int, int);
    void SetupNewOverviewType(int, int);
    void SetupResources(void);
    void Overview(void);
    void DoKnob(void);
    int ProcessIconSelect(int, int);
    signed char SetupCampaignGame(void);
    signed char SetupBaud(void);
    signed char SetupComPort(void);
    signed char SetupHotSeatGame(void);
    signed char SetupNetworkGame(void);
    int SetupNetworkGame2(void);
    signed char SetupModemGame(void);
    signed char SetupMultiPlayerGame(void);
    signed char SetupGame(signed char);
    signed char PickLoadGame(void);
    int HandleCampaignWin(void);
    void PlayPreScenarioSmacker(int, int);
    void ShowCampaignInfo(int, int, int);
    void CampaignInfoUpdate(int);
    void InitEntireCampaign(int);
    // Retail InitCampaignMap takes the scenario and an unused int (ret 8).
    void InitCampaignMap(int, int);
    int MineTypesOwned(int, int);
    int SetupPuzzlePieces(int, int);
    signed char IsMobile(signed char);
    class mapCell (*GetWorldMapData(void))[MAP_CELL_GRID_SIZE];
    // Inline world-map file I/O (LoadMap, SaveGame, LoadGame): each
    // expansion leaves its jmp $+0 after the read or write call.
    void ReadWorldMap(int);
    void WriteWorldMap(int);
    signed char CreateBoat(signed char, signed char);
    signed char Scan(signed char*, signed char, signed char);
    signed char RandomScan(signed char*, signed char, signed char, int);
    signed char GetNewHeroId(signed char);
    signed char GetTownId(signed char, signed char);
    signed char GetMineId(signed char, signed char);
    short SaveGame(char*, signed char);
    void SetupOrigData(void);
    // HoMM1 retail returns 1 in AX (ret 0xc).
    short LoadGame(char*, int, int);
    void GiveTroopsToNeutralTown(int);
    void GiveTroopsToNeutralTowns(void);
    void NewMap(char*);
    void RandomizeEvents(void);
    void InitializePasswords(void);
    void RandomizeBarrier(class mapCell*);
    void RandomizePassword(class mapCell*);
    // HoMM1 retail returns 0 in AX.
    short LoadMap(char*);
    void ClaimTown(signed char, signed char);
    void ClaimMine(signed char, signed char);
    // HoMM1 retail: byte spell type and read-only flag, spell in AL (ret 0x10).
    signed char ViewSpells(class hero*, signed char, short (*)(struct tag_message&), signed char);
    // HoMM1: limits the spell page to the combat or adventure slots.
    void SetupSpellRange(short);
    void UpdateSpellWidgets(void);
    // HoMM1 retail: word x/y, byte creature/flags, word count, eleven
    // arguments (ret 0x2c); combatManager::ViewArmy pushes its word locals
    // unextended and the body hands them to heroWindow(short, short, char*).
    void ViewArmy(
        short,
        short,
        signed char,
        short,
        class town*,
        signed char,
        signed char,
        signed char,
        class hero*,
        class army*,
        class armyGroup*
    );
    // HoMM1 retail: byte creature, count returned in AL.
    signed char GetRandomNumTroops(signed char);
    void TurnOnAIMusic(void);
    void TurnOffAIMusic(void);
    void NextPlayer(void);
    int ComputeDailyGold(int);
    void PerDay(void);
    void PerWeek(void);
    void WeeklyRecruitSite(class mapCell*);
    void WeeklyGenericSite(class mapCell*);
    void PerMonth(void);
    void ConvertObject(int, int, int, int, int, int, int, int, int, int, int);
    // HoMM1 retail: byte x, y and castle flag (ret 0xc).
    void RandomizeTown(signed char, signed char, signed char);
    // HoMM1 retail: byte x and y (ret 8).
    void RandomizeMine(signed char, signed char);
    // HoMM1 retail 0x00442fb4 (ret 8): default dwellings and mage-guild spells.
    void SetupTown(signed char, signed char);
    void InitRandomArtifacts(void);
    signed char GetRandomArtifactId(void);
    void RandomizeHeroPool(void);
    void SetRandomHeroArmies(short, int);
    // HoMM1 retail: towns-only pass flag (ret 4).
    void ProcessRandomObjects(int);
    void SetVisibility(short, short, short, short);
    void MakeAllWaterVisible(int);
    void GiveArmy(class armyGroup*, int, int, int);
    int ExperienceValueOfStack(class armyGroup*, class hero*);
    // HoMM1 retail: hero and army only (ret 8).
    int GetLuck(class hero*, class army*);
    int GetPlayerCrest(int player) {
        return m_players[player].m_color;
    }
    void SetupAdjacentMons(void);
    void CancelComputerScreen(void);
    void ShowComputerScreen(void);
    void ShowHeroesLogo(void);
    void WaitForPlayer(char*, int);
    int HasLateOverlay(int, int);
    void ConvertFlagToLateOverlay(int, int);
    int HasObjectTilesetIndex(int, int, int, int);
    void ConvertAllToLateOverlay(int, int);
    // HoMM1 retail 0x0043d4c3 (ret 8): once a cell's object frame is gone,
    // pulls its overlay frame down into the object layer.
    void SettleOverlay(int, int);
    // HoMM1: NewMap rerolls each cell's terrain tile variant after LoadMap.
    void RandomizeTerrainTiles(void);
    void ProcessMapExtra(void);
    // Retail returns whether no town took an owner from its map extra (AL).
    signed char SetupTowns(void);
    void ProcessOnMapHeroes(void);
    void CheckHeroConsistency(void);
    int TransmitSaveGame(int, int);
    int ReceiveSaveGame(int, int);
    void DoNewTurn(void);
    int GetBoatsBuilt(void);
    int GetNumThievesGuilds(int);
    int CalcDifficultyRating(void);
    void RestoreCell(int, int, int, int, class mapCell*, int);
    void SetMapSize(int, int);
    int HeroIDToHeroPos(class playerData*, int);
    int TownIDToTownPos(class playerData*, int);
    void SetupNewRumour(void);
    void CheckForTimeEvent(void);
    int CountShrines(int);
    void ShowMoraleInfo(class hero*, int);
    void ShowLuckInfo(class hero*, int);
    // Retail GetMap never reads ecx; its caller passes no this.
    static void GetMap(void);
    void ProcessNewMap(struct SMapHeader*);
    void InitNewGame(struct SMapHeader*);
    void SetupNetPlayerNames(void);
    // Retail returns the started flag in AL.
    signed char NewGame(void);
    void CleanUpNewGameWindow(void);
    void InitNewGameWindow(void);
    void UpdateNewGameWindow(void);
    int ProcessNGKeyPress(struct tag_message&);
    void NGKPSetupDisplayString(char*, unsigned short int);
    void DrawNGKPDisplayString(int);
    void ShowScenInfo(void);
    // HoMM1: NewMap gives every opponent a distinct crest.
    void RandomizePlayerCrests(void);
    void GetLossConditionText(char*);
    void GetVictoryConditionText(char*);
    int GetSideDesc(char*, int, int);
    // DoEvent's obelisk branch (byte player, ret 4).
    void VisitObelisk(signed char);
};
#pragma pack(pop)

// Recomputes a player's ultimate-artifact hint (cdecl, int player).
void ComputeUALoc(int);
// GAME's dialog handlers and the standard-game day score ShowCongrats files.
short ViewSpellsHandler(struct tag_message&);
short ViewSpecialHandler(struct tag_message&);
short ViewArmyHandler(struct tag_message&);
int GetBaseScore(int);
#endif // HOMM1_SOURCE_GAME_H
