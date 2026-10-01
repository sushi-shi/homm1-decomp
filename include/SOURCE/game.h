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
H1_ENUM_BEGIN(GameStorageConstant)
    GAME_PLAYER_COUNT = 4,
    GAME_TOWN_COUNT = 36,
    GAME_HERO_COUNT = 36,
    GAME_MINE_COUNT = 36,
    GAME_BOAT_COUNT = 32
H1_ENUM_END(GameStorageConstant)
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

// Player records (0x105 bytes at 0x20c), the embedded 72x72 world map at
// 0x620, towns (0x37 bytes at 0x121a1) and heroes (0xb6 bytes at 0x12985)
// are fixed by retail address arithmetic; unrecovered spans stay opaque.
#pragma pack(push, 1)
        class game {
public:
    char m_unknown0000[3];
    // ControlPanel's scenario-info choice shows the campaign when positive.
    int m_campaignType;
    int m_campaignScenario;
    char m_unknown000b[0x1f4];
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
    char m_unknown1295d[0x24];
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
    char m_unknown145a6[0x1470];
    // GetCloudLookup tests the watching player bit per [x][y] cell.
    unsigned char m_mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    signed char m_ultimateArtifactX;
    signed char m_ultimateArtifactY;
    signed char m_ultimateArtifactId;
    char m_unknown16e59[9];
    // TavernHandler advances this word as its animation counter (Buka name).
    short m_viewArmyResult;
    // InitMainClasses allocates 0x16e7a bytes for the game object.
    char m_unknown16e64[0x16];
    hero* GetHero(int id) {
        return &m_heroRecs[id];
    }
    town* GetTown(int id) {
        return &m_castleRecs[id];
    }
    // --- methods ---
    void SetupDynamicStuff(int, int, int);
    void SetupNewOverviewType(int, int);
    void SetupResources(void);
    void Overview(void);
    void DoKnob(void);
    int ProcessIconSelect(int, int);
    int SetupCampaignGame(void);
    signed char SetupBaud(void);
    signed char SetupComPort(void);
    signed char SetupHotSeatGame(void);
    signed char SetupNetworkGame(void);
    int SetupNetworkGame2(void);
    signed char SetupModemGame(void);
    signed char SetupMultiPlayerGame(void);
    int SetupGame(void);
    int PickLoadGame(void);
    int HandleCampaignWin(void);
    void PlayPreScenarioSmacker(int, int);
    void ShowCampaignInfo(int, int);
    void CampaignInfoUpdate(int);
    void InitEntireCampaign(int);
    void InitCampaignMap(void);
    int MineTypesOwned(int, int);
    int SetupPuzzlePieces(int, int);
    signed char IsMobile(signed char);
    class mapCell (*GetWorldMapData(void))[MAP_CELL_GRID_SIZE];
    signed char CreateBoat(signed char, signed char);
    signed char Scan(signed char*, signed char, signed char);
    signed char RandomScan(signed char*, signed char, signed char, int);
    signed char GetNewHeroId(signed char);
    signed char GetTownId(signed char, signed char);
    signed char GetMineId(signed char, signed char);
    short SaveGame(char *, signed char);
    void SetupOrigData(void);
    void LoadGame(char*, int, int);
    void GiveTroopsToNeutralTown(int);
    void GiveTroopsToNeutralTowns(void);
    void NewMap(char*);
    void RandomizeEvents(void);
    void InitializePasswords(void);
    void RandomizeBarrier(class mapCell*);
    void RandomizePassword(class mapCell*);
    int LoadMap(char*);
    void ClaimTown(int, int, int);
    void ClaimMine(signed char, signed char);
    int ViewSpells(class hero*, int, short (*)(struct tag_message&), int);
    void UpdateSpellWidgets(void);
    // HoMM1 retail: byte creature/flags, word count, eleven arguments (ret 0x2c).
    void ViewArmy(
        int,
        int,
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
    int GetRandomNumTroops(int);
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
    void RandomizeTown(int, int, int);
    void RandomizeMine(int, int);
    void InitRandomArtifacts(void);
    signed char GetRandomArtifactId(void);
    void RandomizeHeroPool(void);
    void SetRandomHeroArmies(int, int);
    void ProcessRandomObjects(void);
    void SetVisibility(short, short, short, short);
    void MakeAllWaterVisible(int);
    void GiveArmy(class armyGroup*, int, int, int);
    int ExperienceValueOfStack(class armyGroup*, class hero*);
    int GetLuck(class hero*, class army*, class town*);
    void SetupAdjacentMons(void);
    void CancelComputerScreen(void);
    void ShowComputerScreen(void);
    void ShowHeroesLogo(void);
    void WaitForPlayer(char*, int);
    int HasLateOverlay(int, int);
    void ConvertFlagToLateOverlay(int, int);
    int HasObjectTilesetIndex(int, int, int, int);
    void ConvertAllToLateOverlay(int, int);
    void ProcessMapExtra(void);
    void SetupTowns(void);
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
    void GetMap(void);
    void ProcessNewMap(struct SMapHeader*);
    void InitNewGame(struct SMapHeader*);
    void SetupNetPlayerNames(void);
    int NewGame(void);
    void CleanUpNewGameWindow(void);
    void InitNewGameWindow(void);
    void UpdateNewGameWindow(void);
    int ProcessNGKeyPress(struct tag_message&);
    void NGKPSetupDisplayString(char*, unsigned short int);
    void DrawNGKPDisplayString(int);
    void ShowScenInfo(void);
    void GetLossConditionText(char*);
    void GetVictoryConditionText(char*);
    int GetSideDesc(char*, int, int);
};
#pragma pack(pop)

extern game* gpGame;
#endif // HOMM1_SOURCE_GAME_H
