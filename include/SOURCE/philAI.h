#ifndef HOMM1_SOURCE_PHILAI_H
#define HOMM1_SOURCE_PHILAI_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 75 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/game.h>
#include <SOURCE/resourceTypes.h>

extern float gafAITurnCostResource[static_cast<i32>(RESOURCE_COUNT)];

extern i8 giBuildShipyard[GAME_PLAYER_COUNT];
extern i8 giBuildBoat[GAME_PLAYER_COUNT];
extern i8 giBuildBoatStuffTurn[GAME_PLAYER_COUNT];
void ShowStatus();
void CheckDoMain(i32, i32);
extern i32 iDummy;
extern i32 bHeroBuiltThisTurn;

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
    i32 type;
    i32 what;
    i32 num;
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
    i32 GoodAdjacent(class hero*, i32*);
    void CheckReload(class hero*);
    void CheckBerserk(class hero*);
    void DimensionDoorTo(i32 x, i32 y);
    i32 DoAnywhereDDoorTownGate(i32 targetValue);
    i8 DoDimensionDoor(class hero*);
    void SetupRelativeHeroStrengths(void);
    void DoAI(i32);
    void GetGameAIVars(void);
    void GetTurnAIVars(i32);
    void GetBestBHC(i32, struct BHC&);
    class hero* DetermineHeroToMove(i32);
    void DetermineTargetPosition(class hero*, i8&, i8&, i16);
    void ProbableOutcomeOfBattle(
        class armyGroup*,
        class hero*,
        class armyGroup*,
        class hero*,
        class armyGroup*,
        i8,
        i8,
        i32,
        float&,
        i32&,
        i32&,
        i32&,
        i32&,
        i32&
    );
    float GetOddsOfWinning(i32);
    void ValueOfBuyingBuilding(class town*, i32, i32&, float&);
    void GetBestBuilding(class town*, struct BHC&, float&);
    void ValueOfBuyingCreature(class town*, i32, i32&, i32, float&);
    void GetBestCreature(class town*, struct BHC&, float&);
    i32 CreaturesToBuy(class town*, i32);
    i32 CreaturesToBuy(i32, i32);
    i32 MaxBuyableCreatures(i32);
    void ValueOfBuyingHero(class town*, class hero*, i32&, float&);
    void GetBestHero(class town*, struct BHC&, float&);
    void
    LikelihoodOfEnemyAttacking(class town*, class hero*, float&, float&, i32&, i32&, i32&, float&);
    i32 MeanRVOfUnexploredTerritory(i32);
    void GetGameAttentionValue(i32);
    void GetTurnAttentionValue(i32);
    i32 RVConversion(i32* const);
    float TurnsToBuy(i32* const);
    i32 RVOfPosition(
        class hero*,
        i16,
        i16,
        i8,
        i16,
        i16,
        i8,
        i16,
        i16,
        i32
    );
    i32 StrategicValueOfPosition(class hero*, i16, i16, i8, i32*);
    i32 ValueOfTown(class town*);
    void TurnCostResource(i32);
    float TurnValueOfObelisk(i32);
    float FutureDeflator(i32* const);
    i32 FightValueOfStack(class armyGroup*, class hero*, i32, i8, i8);
    void EvaluateOneTimeCreaturePurchase(class hero*, i32, i32, i32, i32&, i32&, i32&);
    i32 QuickCombat(
        class armyGroup*,
        class hero*,
        class armyGroup*,
        class hero*,
        i8,
        i8,
        float&,
        float&
    );
    void HeroInteractionAtHero(
        class hero* firstHero,
        class hero* secondHero,
        i32 evaluateOnly,
        i32* value
    );
    void HeroInteractionAtTown(class hero*, class town*, i32, i32*);
    void RedistributeTroops(
        class armyGroup* sourceArmy,
        class armyGroup* destinationArmy,
        i32 preserveOne,
        i32 preferFast,
        i32 sourceStrength,
        i32 destinationStrength,
        i32 transferBudget
    );
    i32 ChooseGoldOrExperience(class hero*, i32, i32);
    void ChooseEvaluateBattle(
        class armyGroup*,
        class hero*,
        class armyGroup*,
        class hero*,
        i32,
        i32,
        i32,
        i32&,
        i32&
    );
    i32 ChooseToFightForArtifact(i32 artifact, i32 monster, i32 quantity);
    i32 ChooseToBuyArtifact(class hero*, i32, i32);
    i32 NetValueOfArtifact(i32 artifact, i32 goldCost, i32 resourceType, i32 resourceCost);
    i32 ChooseToPayRansomOnHero(class hero*, i32);
    void BuildBuilding(class town*, i16);
    void BuildHero(class town*, i16);
    void BuildCreature(class town*, i32, i32);
    i32 CanBuyBHC(struct BHC&);
    i8 CombatMonsterEvent(class hero*, i8, i32*, class mapCell*);
    void FightEvent(class hero*, class mapCell*);
    i32 DamageGroup(class armyGroup*, class hero*, class hero*, float);
    float StatChangeValue(i32, i32);
    void IncrementHourGlass(void);
    void TownEvent(class mapCell*, class hero*, i32, i32);
    i32 ComputeUpgradeValue(i32 baseCreatureType, i32 upgradedCreatureType);
    i32 ComputeValueOfSS(class hero* heroPointer, i32 skill, i32 level);
    i32 ComputeValueOfFreeSS(class hero* heroPointer, i32 skill);
    i32 ManaRefreshValue(class hero* heroPointer, i32 level);
    i32 ValueOfEventAtPosition(class hero*, i16, i16, i32, i32*);
    i32 EvaluateGenericSite(class mapCell* cell);
    i32 EvaluateBarrier(class mapCell* cell);
    i32 EvaluatePassword(class mapCell* cell);
    i32 EvaluateRecruitSite(class mapCell* cell);
    i32 EvaluateJail(class mapCell*);
    i32 EvaluateArtifactEvent(i32 artifact, i32 eventData);
    i32 EvaluateMineEvent(i32 mineIndex, i32 x, i32 y, i32* liveChance);
    i32 EvaluateMonsterEvent(i32 monsterType, i32 eventData, i32* liveChance);
    i32 EvaluateHeroEvent(i32 heroId, i32 x, i32 y, i32 mode, i32* liveChance);
    i32 EvaluateTownEvent(i32 townId, i32 x, i32 y, i32 mode, i32* liveChance);
};
extern i32 costTemp[];
extern i32 iLastFrameRateTimer;
extern i32 giHumanTownConquered;
extern i32 gbBerserk;
extern float fBerserkFactor;
// CheckReload's troop-reload verdict and its reduction factor.
extern i32 gbTroopReload;
// GetBestBHC's per-player hero ceiling (GetTurnAIVars sets it).
extern i32 giMaxHeroesForThisPlayer;
// GetTurnAIVars' per-cell enemy-hero turn distance for mines.
extern i8 gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern float gfHeroInteractionBonus[];
extern float gAttackHumanBonus;
extern float gAttackComputerBonus;
// ValueOfEventAtPosition's event cache, per-resource mine income and the
// ultimate artifact's average value.
extern i16 gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
// DoAI's per-turn visit log: up to 30 positions (x, y) a hero has moved
// from; a target already in it ends the hero's turn (Buka 2.1 ADVMGR.h
// names).
H1_ENUM_CONST_BEGIN(AIPlaceVisitConstant)
    ADVMGR_PLACE_VISIT_COUNT = 30,
    ADVMGR_PLACE_COORDINATE_COUNT = 2
H1_ENUM_CONST_END(AIPlaceVisitConstant)
extern i32 iPlacesVisited[ADVMGR_PLACE_VISIT_COUNT][ADVMGR_PLACE_COORDINATE_COUNT];
extern i32 iCurPlaceToVisit;
void ResetHeroRVs(i32, i32, i32);
// DetermineTargetPosition's shipyard search state.
extern i8 giBestShipyardId;
extern i8 gbPossibleShipyardFound;
extern i8 gbActualShipyardFound;
extern i8 gbActualBoatFound;
extern i8 giBestShipyardDist;
// StrategicValueOfPosition's per-cell cache, hero live chances and the
// shared search it borrows unless a nested evaluation already holds it.
extern i16 gaiHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i16 gaiLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i16 gaiHeroLiveChance[];
extern class searchArray SVSearchArray;
extern float fReduceFactor;
// The per-cell/per-hero resource-value caches (gaiHeroStrategicRVOfPos,
// gaiHeroEventStratRVOfPos, gaiHeroLiveChance) hold RV_UNSET until
// evaluated; ResetHeroRVs writes it back (Buka's name).
H1_ENUM_CONST_BEGIN(AIResourceValue)
    RV_UNSET = -32001
H1_ENUM_CONST_END(AIResourceValue)
// mapExtra bit 7: game::SetupAdjacentMons sets it where FindAdjacentMonster
// finds a guard next to the cell and clears it (mask 0x7f) elsewhere.
H1_ENUM_BEGIN(MapExtraFlag)
    MAP_EXTRA_MONSTER_ADJACENT = 0x80
H1_ENUM_END(MapExtraFlag)

// Shared with GAME and EVENTS: the per-cell bitmask of the players whose
// heroes have stood there and the current/watch players' high bits (all in
// PHILAI's .bss band), ViewArmy's dismiss flag and the creatures a creature
// month may feature (Buka PHILAI.h).
extern i8 mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern u8 giCurPlayerHighBit;
extern u8 gCurWatchPlayerHighBit;
void AiPrint(char*);
void AbsAiPrint(char*);
extern i8 gShowComputerRoute;
extern u8 giCurWatchPlayerBit;
extern u8 giCurPlayerBit;
extern playerData* gpCurPlayer;
extern i8 giCurPlayer;
extern i32 giCurTurn;

#endif // HOMM1_SOURCE_PHILAI_H
