#ifndef HOMM1_SOURCE_PHILAI_H
#define HOMM1_SOURCE_PHILAI_H

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/game.h>
#include <SOURCE/resourceTypes.h>

#define gAITurnCostResource gafAITurnCostResource // spelling fixes .bss order
extern H1_ENUM_ARRAY(float, gAITurnCostResource, ResourceType, RESOURCE_COUNT);

#define gBuildShipyard giBuildShipyard // spelling fixes .bss order
extern i8 gBuildShipyard[GAME_PLAYER_COUNT];
#define gBuildBoat giBuildBoat // spelling fixes .bss order
extern i8 gBuildBoat[GAME_PLAYER_COUNT];
#define gBuildBoatStuffTurn giBuildBoatStuffTurn // spelling fixes .bss order
extern i8 gBuildBoatStuffTurn[GAME_PLAYER_COUNT];
void ShowStatus();
void CheckDoMain(i32, b32 doMain);
#define gDummy iDummy // spelling fixes .bss order
extern i32 gDummy;
#define gHeroBuiltThisTurn bHeroBuiltThisTurn // spelling fixes .bss order
extern i32 gHeroBuiltThisTurn;

// forward declarations:
class armyGroup;
class font;
class hero;
class mapCell;
class town;
// Purchase record: town, kind, building/dwelling and count.
// DoAI's boat plan holds back the shipyard's price (gNeutralBuildingCosts
// row BUILDING_SLOT_SHIPYARD: 2000 gold, 20 wood) while it buys other things,
// as it holds back TOWN_BOAT_GOLD_COST/WOOD_COST for the boat.
H1_ENUM_CONST_BEGIN(AIBoatPlanConstant)
    AI_SHIPYARD_GOLD_RESERVE = 2000,
    AI_SHIPYARD_WOOD_RESERVE = 20
H1_ENUM_CONST_END(AIBoatPlanConstant)

// BHC::type: what GetBestBHC chose to buy:
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
    town* townPointer;
    H1_ENUM_STORAGE(AIPurchaseType, i32) type;
    // The building slot, available-hero index or dwelling index of `type`.
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
    void ShowDebugText(char* text);
    void DoAllHeroInteractions(void);
    void CheckBuyStuff(void);
    i32 GoodAdjacent(class hero* aiHero, H1_ENUM_PARAM(MapDirection, i32) * direction);
    void CheckReload(class hero* aiHero);
    void CheckBerserk(class hero* aiHero);
    i8 DoDimensionDoor(class hero* aiHero);
    void DoAI(i32 player);
    void GetGameAIVars(void);
    void GetTurnAIVars(i32 player);
    void GetBestBHC(i32 player, struct BHC& best);
    class hero* DetermineHeroToMove(i32 player);
    void DetermineTargetPosition(class hero* aiHero, i8& targetX, i8& targetY, i16 mobility);
    void ProbableOutcomeOfBattle(
        class armyGroup* attacker,
        class hero* attackerHero,
        class armyGroup* defender,
        class hero* defenderHero,
        class armyGroup* townArmy,
        i8 useTown,
        i8 townId,
        i32 enemyPlayer,
        float& winChance,
        i32& attackerLoss,
        i32& defenderLoss,
        i32& expectedAttackerLoss,
        i32& expectedDefenderLoss,
        i32& outcomeValue
    );
    float GetOddsOfWinning(i32);
    void ValueOfBuyingBuilding(
        class town* townPointer,
        H1_ENUM_PARAM(BuildingSlotType, i32) building,
        i32& resourceValue,
        float& benefitCost
    );
    void GetBestBuilding(class town* townPointer, struct BHC& purchase, float& benefitCost);
    void ValueOfBuyingCreature(
        class town* townPointer,
        H1_ENUM_PARAM(CreatureType, i32) creature,
        i32& resourceValue,
        i32 purchaseCount,
        float& benefitCost
    );
    void GetBestCreature(class town* townPointer, struct BHC& best, float& bestValue);
    i32 CreaturesToBuy(class town* townPointer, i32 level);
    i32 CreaturesToBuy(H1_ENUM_PARAM(CreatureType, i32) creatureType, i32 availableCount);
    i32 MaxBuyableCreatures(H1_ENUM_PARAM(CreatureType, i32) creatureType);
    void ValueOfBuyingHero(
        class town* townPointer,
        class hero* heroPointer,
        i32& resourceValue,
        float& benefitCost
    );
    void GetBestHero(class town* townPointer, struct BHC& best, float& bestValue);
    void LikelihoodOfEnemyAttacking(
        class town* townPointer,
        class hero* heroPointer,
        float& attackChance,
        float& lossRisk,
        i32& attackStrength,
        i32& weightedAttack,
        i32& attackWeeks,
        float& dangerRating
    );
    i32 MeanRVOfUnexploredTerritory(i32 player);
    void GetGameAttentionValue(i32 player);
    void GetTurnAttentionValue(i32 player);
    i32 RVConversion(i32* const resources);
    float TurnsToBuy(i32* const resources);
    i32 RVOfPosition(
        class hero* aiHero,
        i16 x,
        i16 y,
        i8 hasEvent,
        i16 eventX,
        i16 eventY,
        i8 hasStrategicEvent,
        i16 strategicX,
        i16 strategicY,
        i32 eventMode
    );
    i32 StrategicValueOfPosition(
        class hero* aiHero,
        i16 targetX,
        i16 targetY,
        b8 immediate,
        i32* liveChance
    );
    i32 ValueOfTown(class town* townPointer);
    void TurnCostResource(i32 player);
    float TurnValueOfObelisk(i32 player);
    float FutureDeflator(i32* const resources);
    i32 FightValueOfStack(
        class armyGroup* group,
        class hero* heroPointer,
        b32 useAdjustedFightValue,
        i8 useTown = 0,
        i8 townId = 0
    );
    void EvaluateOneTimeCreaturePurchase(
        class hero* aiHero,
        H1_ENUM_PARAM(CreatureType, i32) creature,
        i32 availableCount,
        b32 useAvailableCount,
        i32& purchaseCount,
        i32& purchaseValue,
        i32& replacementSlot
    );
    i32 QuickCombat(
        class armyGroup* attacker,
        class hero* attackerHero,
        class armyGroup* defender,
        class hero* defenderHero,
        i8 townBattle,
        i8 townId,
        float& attackerCasualtyFraction,
        float& defenderCasualtyFraction
    );
    void HeroInteractionAtTown(
        class hero* heroPointer,
        class town* townPointer,
        b32 evaluateOnly,
        i32* value
    );
    i32 ChooseGoldOrExperience(class hero* heroPointer, i32 gold, i32 experience);
    void ChooseEvaluateBattle(
        class armyGroup* attackerArmy,
        class hero* attackerHero,
        class armyGroup* defenderArmy,
        class hero* defenderHero,
        i32 isCastle,
        i32 castleId,
        i32 rewardValue,
        i32& canWin,
        i32& rating
    );
    i32 ChooseToBuyArtifact(
        class hero* heroPointer,
        H1_ENUM_PARAM(ArtifactType, i32) artifact,
        i32 goldCost
    );
    i32 ChooseToPayRansomOnHero(class hero* heroPointer, i32 goldCost);
    void BuildBuilding(class town* townPointer, H1_ENUM_PARAM(BuildingSlotType, i16) building);
    void BuildHero(class town* townPointer, i16 availableHeroIndex);
    void BuildCreature(class town* townPointer, i32 dwelling, i32 purchaseCount);
    i32 CanBuyBHC(struct BHC& purchase);
    i8 CombatMonsterEvent(
        class hero* heroPointer,
        H1_ENUM_PARAM(CreatureType, i8) monsterType,
        i32* monsterCount,
        class mapCell* cell
    );
    void FightEvent(class hero* heroPointer, class mapCell* cell);
    i32 DamageGroup(
        class armyGroup* group,
        class hero* loser,
        class hero* winner,
        float casualtyFraction
    );
    float StatChangeValue(i32 oldValue, i32 newValue);
    void IncrementHourGlass(void);
    void TownEvent(class mapCell* cell, class hero* heroPointer, i32 x, i32 y);
    i32 ValueOfEventAtPosition(class hero* aiHero, i16 x, i16 y, i32 immediate, i32* liveChance);
};

#define gCreatureCost costTemp // spelling fixes .bss order
extern H1_ENUM_ARRAY(i32, gCreatureCost, ResourceType, RESOURCE_COUNT);
#define gLastFrameRateTimer iLastFrameRateTimer // spelling fixes .bss order
extern i32 gLastFrameRateTimer;
extern i32 gHumanTownConquered;
#define gBerserk gbBerserk // spelling fixes .bss order
extern i32 gBerserk;
#define gBerserkFactor fBerserkFactor // spelling fixes .bss order
extern float gBerserkFactor;
// CheckReload's troop-reload verdict and its reduction factor.
#define gTroopReload gbTroopReload // spelling fixes .bss order
extern i32 gTroopReload;
// GetBestBHC's per-player hero ceiling (GetTurnAIVars sets it).
#define gMaxHeroesForThisPlayer giMaxHeroesForThisPlayer // spelling fixes .bss order
extern i32 gMaxHeroesForThisPlayer;
// GetTurnAIVars' per-cell enemy-hero turn distance for mines.
extern i8 gTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
#define gHeroInteractionBonus gfHeroInteractionBonus // spelling fixes .bss order
extern float gHeroInteractionBonus[];
extern float gAttackHumanBonus;
extern float gAttackComputerBonus;
// ValueOfEventAtPosition's event cache, per-resource mine income and the
// ultimate artifact's average value.
extern i16 gHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
// DoAI's per-turn visit log: up to 30 positions (x, y) a hero has moved
// from; a target already in it ends the hero's turn.
H1_ENUM_CONST_BEGIN(AIPlaceVisitConstant)
    AI_PLACE_VISIT_COUNT = 30,
    AI_PLACE_COORDINATE_COUNT = 2
H1_ENUM_CONST_END(AIPlaceVisitConstant)
#define gPlacesVisited iPlacesVisited // spelling fixes .bss order
extern i32 gPlacesVisited[AI_PLACE_VISIT_COUNT][AI_PLACE_COORDINATE_COUNT];
#define gCurPlaceToVisit iCurPlaceToVisit // spelling fixes .bss order
extern i32 gCurPlaceToVisit;
void ResetHeroRVs(b32 resetAll, i32 x, i32 y);
// DetermineTargetPosition's shipyard search state.
#define gBestShipyardId giBestShipyardId // spelling fixes .bss order
extern i8 gBestShipyardId;
#define gPossibleShipyardFound gbPossibleShipyardFound // spelling fixes .bss order
extern i8 gPossibleShipyardFound;
#define gActualShipyardFound gbActualShipyardFound // spelling fixes .bss order
extern i8 gActualShipyardFound;
#define gActualBoatFound gbActualBoatFound // spelling fixes .bss order
extern i8 gActualBoatFound;
#define gBestShipyardDist giBestShipyardDist // spelling fixes .bss order
extern i8 gBestShipyardDist;
// StrategicValueOfPosition's per-cell cache, hero live chances and the
// shared search it borrows unless a nested evaluation already holds it.
#define gHeroStrategicRVOfPos gaiHeroStrategicRVOfPos // spelling fixes .bss order
extern i16 gHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
#define gLiveChanceOfPos gaiLiveChanceOfPos // spelling fixes .bss order
extern i16 gLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
#define gHeroLiveChance gaiHeroLiveChance // spelling fixes .bss order
extern i16 gHeroLiveChance[];
#define gStrategicSearchArray SVSearchArray // spelling fixes .bss order
extern class searchArray gStrategicSearchArray;
#define gReduceFactor fReduceFactor // spelling fixes .bss order
extern float gReduceFactor;
// The per-cell/per-hero resource-value caches (gHeroStrategicRVOfPos,
// gHeroEventStratRVOfPos, gHeroLiveChance) hold RV_UNSET until
// evaluated; ResetHeroRVs writes it back.
H1_ENUM_CONST_BEGIN(AIResourceValue)
    RV_UNSET = -32001
H1_ENUM_CONST_END(AIResourceValue)
// mapExtra bit 7: game::SetupAdjacentMons sets it where FindAdjacentMonster
// finds a guard next to the cell and clears it (mask 0x7f) elsewhere. A flag
// of the cell's mapExtra bit set (tested with &, set with |=), not a value.
H1_ENUM_FLAGS_BEGIN(MapExtraFlag, u8)
    MAP_EXTRA_MONSTER_ADJACENT = 0x80
H1_ENUM_FLAGS_END(MapExtraFlag)

// Shared with GAME and EVENTS: the per-cell bitmask of the players whose
// heroes have stood there and the current/watch players' high bits (all in
// PHILAI's .bss band), ViewArmy's dismiss flag and the creatures a creature
// month may feature.
extern i8 gMapVisitFlags[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
#define gCurPlayerHighBit gCurPlayerTopBit // spelling fixes .bss order
extern u8 gCurPlayerHighBit;
#define gCurWatchPlayerHighBit gCurWatchPlayerHighFlag // spelling fixes .bss order
extern u8 gCurWatchPlayerHighBit;
void AiPrint(char* text);
void AbsAiPrint(char* text);
extern b8 gShowComputerRoute;
#define gCurWatchPlayerBit giCurWatchPlayerBit // spelling fixes .bss order
extern u8 gCurWatchPlayerBit;
#define gCurPlayerBit giCurPlayerBit // spelling fixes .bss order
extern u8 gCurPlayerBit;
extern playerData* gCurPlayerData;
#define gCurPlayer giCurPlayer // spelling fixes .bss order
extern i8 gCurPlayer;
#define gCurTurn giCurTurn // spelling fixes .bss order
extern i32 gCurTurn;

// The AI's hourglass: phases 0..LAST, advanced faster with fewer heroes
// (PHASE_1/3/6 are the steps a two- or three-hero turn skips).
H1_ENUM_CONST_BEGIN(AIHourGlassConstant)
    AI_HOUR_GLASS_PHASE_1 = 1,
    AI_HOUR_GLASS_PHASE_3 = 3,
    AI_HOUR_GLASS_PHASE_6 = 6,
    AI_HOUR_GLASS_PHASE_LAST = 9,
    AI_HOUR_GLASS_TWO_HEROES = 2,
    AI_HOUR_GLASS_THREE_HEROES = 3
H1_ENUM_CONST_END(AIHourGlassConstant)

// ValueOfEventAtPosition's battle odds: CERTAIN is a 100% chance; debug
// level AI_DEBUG_LEVEL_EVENT (KB.h DebugLevel) turns into BATTLE tracing for
// column TRACE_COLUMN.
H1_ENUM_CONST_BEGIN(AIEventValueConstant)
    AI_CHANCE_CERTAIN = 100,
    AI_DEBUG_TRACE_COLUMN = 15
H1_ENUM_CONST_END(AIEventValueConstant)

#endif // HOMM1_SOURCE_PHILAI_H
