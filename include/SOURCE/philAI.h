#ifndef HOMM1_SOURCE_PHILAI_H
#define HOMM1_SOURCE_PHILAI_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 75 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/game.h>
#include <SOURCE/resourceTypes.h>

extern signed char gDwellingType[4][6];

extern float gafAITurnCostResource[static_cast<int>(RESOURCE_COUNT)];

extern signed char giBuildShipyard[GAME_PLAYER_COUNT];
extern signed char giBuildBoat[GAME_PLAYER_COUNT];
extern signed char giBuildBoatStuffTurn[GAME_PLAYER_COUNT];
void ShowStatus();
void CheckDoMain(int, int);
int GetBuildingBaseResourceValue(int, int, int);
extern int iDummy;
extern int gArtifactBaseRV[];
extern int gResourceBaseValue[];
extern int bHeroBuiltThisTurn;
extern int iCurHourGlassPhase;

// forward declarations:
class armyGroup;
class font;
class hero;
class mapCell;
class town;
// Buka 2.1 purchase record: town, kind, building/dwelling and count.
// DoAI's boat plan holds back the shipyard's price (gNeutralBuildingCosts
// row BUILDING_SLOT_SHIPYARD: 2000 gold, 20 wood) while it buys other things,
// as it holds back TOWN_BOAT_GOLD_COST/WOOD_COST for the boat.
H1_ENUM_CONST_BEGIN(AIBoatPlanConstant)
    AI_SHIPYARD_GOLD_RESERVE = 2000,
    AI_SHIPYARD_WOOD_RESERVE = 20
H1_ENUM_CONST_END(AIBoatPlanConstant)

// BHC::type: what GetBestBHC chose to buy (Buka 2.1 PHILAI.h AIPurchaseType):
// GetBestBuilding/GetBestHero/GetBestCreature fill BUILDING/HERO/CREATURE,
// DoAI dispatches BuildBuilding/BuildHero/BuildCreature and CanBuyBHC checks
// each; NONE when nothing is worth buying (DoAI buys when type >= FIRST).
H1_ENUM_BEGIN(AIPurchaseType)
    PURCHASE_NONE = -1,
    PURCHASE_FIRST = 0,
    PURCHASE_BUILDING = 0,
    PURCHASE_HERO = 1,
    PURCHASE_CREATURE = 2
H1_ENUM_END(AIPurchaseType)

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
    void DimensionDoorTo(int x, int y);
    int DoAnywhereDDoorTownGate(int targetValue);
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
    int RVOfPosition(
        class hero*,
        short,
        short,
        signed char,
        short,
        short,
        signed char,
        short,
        short,
        int
    );
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
    void HeroInteractionAtHero(class hero* firstHero, class hero* secondHero, int evaluateOnly, int* value);
    void HeroInteractionAtTown(class hero*, class town*, int, int*);
    void RedistributeTroops(class armyGroup* sourceArmy, class armyGroup* destinationArmy, int preserveOne, int preferFast, int sourceStrength, int destinationStrength, int transferBudget);
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
    int ChooseToFightForArtifact(int artifact, int monster, int quantity);
    int ChooseToBuyArtifact(class hero*, int, int);
    int NetValueOfArtifact(int artifact, int goldCost, int resourceType, int resourceCost);
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
    int ComputeUpgradeValue(int baseCreatureType, int upgradedCreatureType);
    int ComputeValueOfSS(class hero* heroPointer, int skill, int level);
    int ComputeValueOfFreeSS(class hero* heroPointer, int skill);
    int ManaRefreshValue(class hero* heroPointer, int level);
    int ValueOfEventAtPosition(class hero*, short, short, int, int*);
    int EvaluateGenericSite(class mapCell* cell);
    int EvaluateBarrier(class mapCell* cell);
    int EvaluatePassword(class mapCell* cell);
    int EvaluateRecruitSite(class mapCell* cell);
    int EvaluateJail(class mapCell*);
    int EvaluateArtifactEvent(int artifact, int eventData);
    int EvaluateMineEvent(int mineIndex, int x, int y, int* liveChance);
    int EvaluateMonsterEvent(int monsterType, int eventData, int* liveChance);
    int EvaluateHeroEvent(int heroId, int x, int y, int mode, int* liveChance);
    int EvaluateTownEvent(int townId, int x, int y, int mode, int* liveChance);
};
extern philAI* gpPhilAI;
extern armyGroup* gpMonGroup;
extern int costTemp[];
extern int iLastFrameRateTimer;
extern signed char gbDrawSavedCursor;
extern int bSpecialHideCursor;
extern int giHumanTownConquered;
extern int gbBerserk;
extern float fBerserkFactor;
// CheckReload's troop-reload verdict and its reduction factor.
extern int gbTroopReload;
// GetBestBHC's per-player hero ceiling (GetTurnAIVars sets it).
extern int giMaxHeroesForThisPlayer;
// GetTurnAIVars' per-cell enemy-hero turn distance for mines.
extern signed char gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern float gfHeroInteractionBonus[];
extern float gfAttackHumanBonus;
extern float gfAttackComputerBonus;
// ValueOfEventAtPosition's event cache, per-resource mine income and the
// ultimate artifact's average value.
extern short gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern int giMineIncome[];
extern int gUltArtifactAvgValue;
// DoAI: the single player the AI may run for, and the places each hero has
// already started from this turn.
extern signed char giLimitPlayer;
// DoAI's per-turn visit log: up to 30 positions (x, y) a hero has moved
// from; a target already in it ends the hero's turn (Buka 2.1 ADVMGR.h
// names).
H1_ENUM_CONST_BEGIN(AIPlaceVisitConstant)
    ADVMGR_PLACE_VISIT_COUNT = 30,
    ADVMGR_PLACE_COORDINATE_COUNT = 2
H1_ENUM_CONST_END(AIPlaceVisitConstant)
extern int iPlacesVisited[ADVMGR_PLACE_VISIT_COUNT][ADVMGR_PLACE_COORDINATE_COUNT];
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
extern class searchArray SVSearchArray;
// FightValueOfStack's primary-stat power curve, per-spell AI flags and
// values, spell-power duration scale and per-charge cast weights.
extern float gfStatPower[];
extern signed char gcSpellAIFlags[];
extern short giSpellAIValue[];
extern float gfSpellCastNumMod[];
extern float fReduceFactor;
// ValueOfBuyingHero: the hero class native to each town type.
extern signed char gTownHeroClass[];
// The per-cell/per-hero resource-value caches (gaiHeroStrategicRVOfPos,
// gaiHeroEventStratRVOfPos, gaiHeroLiveChance) hold RV_UNSET until
// evaluated; ResetHeroRVs writes it back (Buka's name).
H1_ENUM_CONST_BEGIN(AIResourceValue)
    RV_UNSET = -32001
H1_ENUM_CONST_END(AIResourceValue)
// GoodAdjacent skips cells whose adjacency byte carries the monster bit.
extern unsigned char mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
// mapExtra bit 7: game::SetupAdjacentMons sets it where FindAdjacentMonster
// finds a guard next to the cell and clears it (mask 0x7f) elsewhere.
H1_ENUM_BEGIN(MapExtraFlag)
    MAP_EXTRA_MONSTER_ADJACENT = 0x80
H1_ENUM_END(MapExtraFlag)

// Shared with GAME and EVENTS: the per-cell bitmask of the players whose
// heroes have stood there and the current/watch players' high bits (all in
// PHILAI's .bss band), ViewArmy's dismiss flag and the creatures a creature
// month may feature (Buka PHILAI.h).
extern signed char mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern unsigned char giCurPlayerHighBit;
extern unsigned char giCurWatchPlayerHighBit;
extern signed char gbDismissArmy;

#endif // HOMM1_SOURCE_PHILAI_H
