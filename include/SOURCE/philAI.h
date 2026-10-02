#ifndef HOMM1_SOURCE_PHILAI_H
#define HOMM1_SOURCE_PHILAI_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 75 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/resourceTypes.h>

extern signed char gDwellingType[4][6];

extern float gafAITurnCostResource[static_cast<int>(RESOURCE_COUNT)];

H1_ENUM_CONST_BEGIN(AIPlayerConstant)
AI_PLAYER_COUNT = 4, AI_PLAYER_BEGIN = 0,
                     AI_PLAYER_END = AI_PLAYER_COUNT H1_ENUM_CONST_END(AIPlayerConstant)

                         extern signed char giBuildShipyard[AI_PLAYER_COUNT];
extern signed char giBuildBoat[AI_PLAYER_COUNT];
extern signed char giBuildBoatStuffTurn[AI_PLAYER_COUNT];
void ShowStatus();
void CheckDoMain(int, int);
int GetBuildingBaseResourceValue(int, int, int);
extern int iDummy;
extern int gArtifactBaseRV[];
extern int gResourceBaseValue[];
extern float gfStatValue[];
extern int bHeroBuiltThisTurn;
extern int iCurHourGlassPhase;

// forward declarations:
class armyGroup;
class font;
class hero;
class mapCell;
class town;
// Buka 2.1 purchase record: town, kind, building/dwelling and count.
struct BHC {
    town* pTown;
    int type;
    int what;
    int num;
};

class philAI {
public:
    // Retail four-byte allocation; adjacent debug-text routine draws through this font.
    font* m_debugFont;
    // --- constructors ---
    philAI(void);
    // --- methods ---
    void ShowDebugText(char*);
    void DoAllHeroInteractions(void);
    void CheckForCreatureUpgrades(void);
    void CheckBuyStuff(void);
    int GoodAdjacent(class hero*, int*);
    void CheckReload(class hero*);
    void CheckBerserk(class hero*);
    void DimensionDoorTo(int, int);
    int DoAnywhereDDoorTownGate(int);
    signed char DoDimensionDoor(class hero*);
    void SetupRelativeHeroStrengths(void);
    void DoAI(int);
    void GetGameAIVars(void);
    void GetTurnAIVars(int);
    void GetBestBHC(int, struct BHC&);
    class hero* DetermineHeroToMove(int);
    void DetermineTargetPosition(class hero*, signed char&, signed char&, short);
    void ProbableOutcomeOfBattle(
        class armyGroup*,
        class hero*,
        class armyGroup*,
        class hero*,
        class armyGroup*,
        signed char,
        signed char,
        int,
        float&,
        int&,
        int&,
        int&,
        int&,
        int&
    );
    float GetOddsOfWinning(int);
    void ValueOfBuyingBuilding(class town*, int, int&, float&);
    void GetBestBuilding(class town*, struct BHC&, float&);
    void ValueOfBuyingCreature(class town*, int, int&, int, float&);
    void GetBestCreature(class town*, struct BHC&, float&);
    int CreaturesToBuy(class town*, int);
    int CreaturesToBuy(int, int);
    int MaxBuyableCreatures(int);
    void ValueOfBuyingHero(class town*, class hero*, int&, float&);
    void GetBestHero(class town*, struct BHC&, float&);
    void
    LikelihoodOfEnemyAttacking(class town*, class hero*, float&, float&, int&, int&, int&, float&);
    int MeanRVOfUnexploredTerritory(int);
    void GetGameAttentionValue(int);
    void GetTurnAttentionValue(int);
    int RVConversion(int* const);
    float TurnsToBuy(int* const);
    int RVOfPosition(class hero*, short, short, signed char, short, short, signed char, short, short, int);
    int StrategicValueOfPosition(class hero*, short, short, signed char, int*);
    int ValueOfTown(class town*);
    void TurnCostResource(int);
    float TurnValueOfObelisk(int);
    float FutureDeflator(int* const);
    int FightValueOfStack(class armyGroup*, class hero*, int, signed char, signed char);
    void EvaluateOneTimeCreaturePurchase(class hero*, int, int, int, int&, int&, int&);
    int QuickCombat(
        class armyGroup*,
        class hero*,
        class armyGroup*,
        class hero*,
        signed char,
        signed char,
        float&,
        float&
    );
    void HeroInteractionAtHero(class hero*, class hero*, int, int*);
    void HeroInteractionAtTown(class hero*, class town*, int, int*);
    void RedistributeTroops(class armyGroup*, class armyGroup*, int, int, int, int, int);
    int ChooseGoldOrExperience(class hero*, int, int);
    void ChooseEvaluateBattle(
        class armyGroup*,
        class hero*,
        class armyGroup*,
        class hero*,
        int,
        int,
        int,
        int&,
        int&
    );
    int ChooseToFightForArtifact(int, int, int);
    int ChooseToBuyArtifact(class hero*, int, int);
    int NetValueOfArtifact(int, int, int, int);
    int ChooseToPayRansomOnHero(class hero*, int);
    void BuildBuilding(class town*, short);
    void BuildHero(class town*, short);
    void BuildCreature(class town*, int, int);
    int CanBuyBHC(struct BHC&);
    signed char CombatMonsterEvent(class hero*, signed char, int*, class mapCell*);
    void FightEvent(class hero*, class mapCell*);
    int DamageGroup(class armyGroup*, class hero*, class hero*, float);
    float StatChangeValue(int, int);
    void IncrementHourGlass(void);
    void TownEvent(class mapCell*, class hero*, int, int);
    int ComputeUpgradeValue(int, int);
    int ComputeValueOfSS(class hero*, int, int);
    int ComputeValueOfFreeSS(class hero*, int);
    int ManaRefreshValue(class hero*, int);
    int ValueOfEventAtPosition(class hero*, short, short, int, int*);
    int EvaluateGenericSite(class mapCell*);
    int EvaluateBarrier(class mapCell*);
    int EvaluatePassword(class mapCell*);
    int EvaluateRecruitSite(class mapCell*);
    int EvaluateJail(class mapCell*);
    int EvaluateArtifactEvent(int, int);
    int EvaluateMineEvent(int, int, int, int*);
    int EvaluateMonsterEvent(int, int, int*);
    int EvaluateHeroEvent(int, int, int, int, int*);
    int EvaluateTownEvent(int, int, int, int, int*);
};
extern philAI* gpPhilAI;
extern armyGroup* gpMonGroup;
extern int costTemp[];
extern int iLastFrameRateTimer;
extern signed char gbDrawSavedCursor;
extern int bSpecialHideCursor;
extern int gbHumanPlayer[];
extern int giHumanTownConquered;
extern int gbBerserk;
extern float fBerserkFactor;
// CheckReload's troop-reload verdict and its reduction factor.
extern int gbTroopReload;
// GetBestBHC's per-player hero ceiling (GetTurnAIVars sets it).
extern int giMaxHeroesForThisPlayer;
// GetBestBHC lets young towns buy during a network game only with this set;
// TransmitSaveGame tests the same dword for its serial compression path.
extern int gbSerialCompression;
// GetTurnAIVars' per-cell enemy-hero turn distance for mines.
extern signed char gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern float gfHeroInteractionBonus[];
extern float gfAttackHumanBonus;
extern float gfAttackComputerBonus;
extern signed char gbIAmGreatest;
// ValueOfEventAtPosition: cells an enemy hero can reach this turn, the event
// cache, per-resource mine income and the ultimate artifact's average value.
extern signed char gaiEnemyHeroReachable[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern short gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern int giMineIncome[];
extern int gUltArtifactAvgValue;
// DoAI: the single player the AI may run for, and the places each hero has
// already started from this turn.
extern signed char giLimitPlayer;
extern int iPlacesVisited[30][2];
extern int iCurPlaceToVisit;
void ResetHeroRVs(int, int, int);
// DetermineTargetPosition's shipyard search state.
extern signed char giBestShipyardId;
extern signed char gbPossibleShipyardFound;
extern signed char gbActualShipyardFound;
extern signed char gbActualBoatFound;
extern signed char giBestShipyardDist;
// StrategicValueOfPosition's per-cell cache, hero live chances and the
// shared search it borrows unless a nested evaluation already holds it.
extern short gaiHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern short gaiLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern short gaiHeroLiveChance[];
extern signed char bSVSearchArrayInUse;
extern class searchArray SVSearchArray;
// FightValueOfStack's primary-stat power curve, per-spell AI flags and
// values, spell-power duration scale and per-charge cast weights.
extern float gfStatPower[];
extern signed char gcSpellAIFlags[];
extern short giSpellAIValue[];
extern float gfSpellPowerMod[];
extern float gfSpellCastNumMod[];
extern float fReduceFactor;
// ValueOfBuyingHero: the hero class native to each town type.
extern signed char gTownHeroClass[];
// GoodAdjacent skips cells whose adjacency byte carries the monster bit.
extern unsigned char mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];

#endif // HOMM1_SOURCE_PHILAI_H
