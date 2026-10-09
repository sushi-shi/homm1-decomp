#ifndef HOMM1_SOURCE_PHILAI_H
#define HOMM1_SOURCE_PHILAI_H

#include <SOURCE/game.h>
#include <SOURCE/resourceTypes.h>

extern float gAITurnCostResource[RESOURCE_COUNT];

extern i8 gBuildShipyard[GAME_PLAYER_COUNT];
extern i8 gBuildBoat[GAME_PLAYER_COUNT];
extern i8 gBuildBoatStuffTurn[GAME_PLAYER_COUNT];
void ShowStatus();
void CheckDoMain(i32, b32 doMain);
extern i32 gDummy;
extern b32 gHeroBuiltThisTurn;

class armyGroup;
class font;
class hero;
class mapCell;
class town;
enum AIBoatPlanConstant {
    AI_SHIPYARD_GOLD_RESERVE = 2000,
    AI_SHIPYARD_WOOD_RESERVE = 20
};

enum AIPurchaseType {
    PURCHASE_NONE = -1,
    PURCHASE_FIRST = 0,
    PURCHASE_BUILDING = 0,
    PURCHASE_HERO = 1,
    PURCHASE_CREATURE = 2
};

struct BHC {
    town* townPointer;
    i32 type;
    i32 what;
    i32 num;
};

class philAI {
public:
    font* m_debugFont;
    philAI(void);
    void ShowDebugText(char* text);
    void DoAllHeroInteractions(void);
    void CheckBuyStuff(void);
    b32 GoodAdjacent(class hero* aiHero, i32 * direction);
    void CheckReload(class hero* aiHero);
    void CheckBerserk(class hero* aiHero);
    b8 DoDimensionDoor(class hero* aiHero);
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
        i32 building,
        i32& resourceValue,
        float& benefitCost
    );
    void GetBestBuilding(class town* townPointer, struct BHC& purchase, float& benefitCost);
    void ValueOfBuyingCreature(
        class town* townPointer,
        i32 creature,
        i32& resourceValue,
        i32 purchaseCount,
        float& benefitCost
    );
    void GetBestCreature(class town* townPointer, struct BHC& best, float& bestValue);
    i32 CreaturesToBuy(class town* townPointer, i32 level);
    i32 CreaturesToBuy(i32 creatureType, i32 availableCount);
    i32 MaxBuyableCreatures(i32 creatureType);
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
        i8 hasAdjacentMonster,
        i16 adjacentMonsterX,
        i16 adjacentMonsterY,
        i8 beyondTurnMobility,
        i16 turnEndX,
        i16 turnEndY,
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
        i32 creature,
        i32 availableCount,
        b32 isFree,
        i32& purchaseCount,
        i32& purchaseValue,
        i32& replacementSlot
    );
    b32 QuickCombat(
        class armyGroup* attacker,
        class hero* attackerHero,
        class armyGroup* defender,
        class hero* defenderHero,
        b8 townBattle,
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
    b32 ChooseGoldOrExperience(class hero* heroPointer, i32 gold, i32 experience);
    void ChooseEvaluateBattle(
        class armyGroup* attackerArmy,
        class hero* attackerHero,
        class armyGroup* defenderArmy,
        class hero* defenderHero,
        i32 isCastle,
        i32 castleId,
        i32 rewardValue,
        i32& worthFighting,
        i32& rating
    );
    b32 ChooseToBuyArtifact(
        class hero* heroPointer,
        i32 artifact,
        i32 goldCost
    );
    b32 ChooseToPayRansomOnHero(class hero* heroPointer, i32 goldCost);
    void BuildBuilding(class town* townPointer, i16 building);
    void BuildHero(class town* townPointer, i16 availableHeroIndex);
    void BuildCreature(class town* townPointer, i32 dwelling, i32 purchaseCount);
    b32 CanBuyBHC(struct BHC& purchase);
    b8 CombatMonsterEvent(
        class hero* heroPointer,
        i8 monsterType,
        i32* monsterCount,
        class mapCell* cell
    );
    void FightEvent(class hero* heroPointer, class mapCell* cell);
    b32 DamageGroup(
        class armyGroup* group,
        class hero* loser,
        class hero* winner,
        float casualtyFraction
    );
    float StatChangeValue(i32 oldValue, i32 newValue);
    void IncrementHourGlass(void);
    void TownEvent(class mapCell* cell, class hero* heroPointer, i32 x, i32 y);
    i32 ValueOfEventAtPosition(class hero* aiHero, i16 x, i16 y, i32 eventMode, i32* liveChance);
};

extern i32 gCreatureCost[RESOURCE_COUNT];
extern i32 gLastFrameRateTimer;
extern i32 gHumanTownConquered;
extern b32 gBerserk;
extern float gBerserkFactor;
extern b32 gTroopReload;
extern i32 gMaxHeroesForThisPlayer;
extern i8 gTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern float gHeroInteractionBonus[];
extern float gAttackHumanBonus;
extern float gAttackComputerBonus;
extern i16 gHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
enum AIPlaceVisitConstant {
    AI_PLACE_VISIT_COUNT = 30,
    AI_PLACE_X = 0,
    AI_PLACE_Y = 1,
    AI_PLACE_COORDINATE_COUNT = 2
};
extern i32 gPlacesVisited[AI_PLACE_VISIT_COUNT][AI_PLACE_COORDINATE_COUNT];
extern i32 gCurPlaceToVisit;
void ResetHeroRVs(b32 resetAll, i32 x, i32 y);
extern i8 gBestShipyardId;
extern b8 gPossibleShipyardFound;
extern b8 gActualShipyardFound;
extern b8 gActualBoatFound;
extern i8 gBestShipyardDist;
extern i16 gHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i16 gLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i16 gHeroLiveChance[];
extern class searchArray gStrategicSearchArray;
extern float gReduceFactor;
enum AIResourceValue {
    RV_UNSET = -32001
};
enum MapExtraFlag {
    MAP_EXTRA_MONSTER_ADJACENT = 0x80
};

extern i8 gMapVisitFlags[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern u8 gCurPlayerHighBit;
extern u8 gCurWatchPlayerHighBit;
void AiPrint(char* text);
void AbsAiPrint(char* text);
extern b8 gShowComputerRoute;
extern u8 gCurWatchPlayerBit;
extern u8 gCurPlayerBit;
extern playerData* gCurPlayerData;
extern i8 gCurPlayer;
extern i32 gCurTurn;

enum AIHourGlassConstant {
    AI_HOUR_GLASS_PHASE_FIRST = 0,
    AI_HOUR_GLASS_PHASE_1 = 1,
    AI_HOUR_GLASS_PHASE_3 = 3,
    AI_HOUR_GLASS_PHASE_6 = 6,
    AI_HOUR_GLASS_PHASE_LAST = 9,
    AI_HOUR_GLASS_ONE_HERO = 1,
    AI_HOUR_GLASS_TWO_HEROES = 2,
    AI_HOUR_GLASS_THREE_HEROES = 3
};

enum AIEventValueConstant {
    AI_CHANCE_CERTAIN = 100,
    AI_DEBUG_TRACE_COLUMN = 15,
    AI_QUICK_COMBAT_TOWN_EXPERIENCE = 500,
    // SlightlyHarderAI: worthless or visited objects and battles it is not
    // sure to win are scored below anything else.
    AI_HARDER_WORTHLESS_EVENT_VALUE = -1000,
    AI_HARDER_LOSING_BATTLE_VALUE = -32000,
    AI_TARGET_SEARCH_MARGIN = 0x2a,
    AI_HARDER_TARGET_SEARCH_MARGIN = 0x7e
};

enum AIEventEvaluation {
    AI_EVENT_STRATEGIC = 0,
    AI_EVENT_IMMEDIATE = 1,
    AI_EVENT_TARGET = 2
};

enum AIBattleSideConstant {
    AI_BATTLE_ATTACKER = 0,
    AI_BATTLE_DEFENDER = 1,
    AI_BATTLE_SIDE_COUNT = 2
};

enum AIFightValueConstant {
    AI_FIGHT_VALUE_MIN = 100,
    AI_BERSERK_FIGHT_VALUE_MIN = 30000
};

#endif
