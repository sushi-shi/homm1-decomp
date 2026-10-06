#ifndef HOMM1_SOURCE_PHILAI_H
#define HOMM1_SOURCE_PHILAI_H

#include <SOURCE/game.h>
#include <SOURCE/resourceTypes.h>

extern float gafAITurnCostResource[static_cast<i32>(RESOURCE_COUNT)];

extern i8 giBuildShipyard[GAME_PLAYER_COUNT];
extern i8 giBuildBoat[GAME_PLAYER_COUNT];
extern i8 giBuildBoatStuffTurn[GAME_PLAYER_COUNT];
void ShowStatus();
void CheckDoMain(i32, i32 doMain);
extern i32 iDummy;
extern i32 bHeroBuiltThisTurn;

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
    town* pTown;
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
    void CheckForCreatureUpgrades(void);
    void CheckBuyStuff(void);
    i32 GoodAdjacent(class hero* pHero, i32* direction);
    void CheckReload(class hero* pHero);
    void CheckBerserk(class hero* pHero);
    void DimensionDoorTo(i32 x, i32 y);
    i32 DoAnywhereDDoorTownGate(i32 targetValue);
    i8 DoDimensionDoor(class hero* pHero);
    void SetupRelativeHeroStrengths(void);
    void DoAI(i32 player);
    void GetGameAIVars(void);
    void GetTurnAIVars(i32 player);
    void GetBestBHC(i32, struct BHC& best);
    class hero* DetermineHeroToMove(i32 player);
    void DetermineTargetPosition(class hero* pHero, i8& targetX, i8& targetY, i16 mobility);
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
        i32& attackerRemaining,
        i32& defenderRemaining,
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
        class town*,
        class hero*,
        float& chanceA,
        float& chanceB,
        i32& nAttack,
        i32& nValue,
        i32& nWeeks,
        float& fOut
    );
    i32 MeanRVOfUnexploredTerritory(i32);
    void GetGameAttentionValue(i32 player);
    void GetTurnAttentionValue(i32 player);
    i32 RVConversion(i32* const resources);
    float TurnsToBuy(i32* const resources);
    i32 RVOfPosition(
        class hero* pHero,
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
        class hero* pHero,
        i16 targetX,
        i16 targetY,
        i8 immediate,
        i32* liveChance
    );
    i32 ValueOfTown(class town* townPointer);
    void TurnCostResource(i32 player);
    float TurnValueOfObelisk(i32 player);
    float FutureDeflator(i32* const resources);
    i32 FightValueOfStack(
        class armyGroup* group,
        class hero* heroPointer,
        i32 useHero,
        i8 useTown,
        i8 townId
    );
    void EvaluateOneTimeCreaturePurchase(
        class hero* pHero,
        i32 creature,
        i32 availableCount,
        i32 useAvailableCount,
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
        float& attackerDamage,
        float& defenderDamage
    );
    void HeroInteractionAtHero(
        class hero* firstHero,
        class hero* secondHero,
        i32 evaluateOnly,
        i32* value
    );
    void HeroInteractionAtTown(
        class hero* heroPointer,
        class town* townPointer,
        i32 doInteraction,
        i32* value
    );
    void RedistributeTroops(
        class armyGroup* sourceArmy,
        class armyGroup* destinationArmy,
        i32 preserveOne,
        i32 preferFast,
        i32 sourceStrength,
        i32 destinationStrength,
        i32 transferBudget
    );
    i32 ChooseGoldOrExperience(class hero* thisHero, i32 gold, i32 experience);
    void ChooseEvaluateBattle(
        class armyGroup* attackerArmy,
        class hero* attackerHero,
        class armyGroup* defenderArmy,
        class hero* defenderHero,
        i32 isCastle,
        i32 castleId,
        i32 rewardValue,
        i32& outFlag,
        i32& outValue
    );
    i32 ChooseToFightForArtifact(i32 artifact, i32 monster, i32 quantity);
    i32 ChooseToBuyArtifact(class hero*, i32 artifact, i32 goldCost);
    i32 NetValueOfArtifact(i32 artifact, i32 goldCost, i32 resourceType, i32 resourceCost);
    i32 ChooseToPayRansomOnHero(class hero*, i32);
    void BuildBuilding(class town* townPointer, i16 building);
    void BuildHero(class town* townPointer, i16 availableHeroIndex);
    void BuildCreature(class town* townPointer, i32 dwelling, i32 purchaseCount);
    i32 CanBuyBHC(struct BHC& purchase);
    i8 CombatMonsterEvent(class hero* h, i8 monType, i32* pCount, class mapCell*);
    void FightEvent(class hero* heroPointer, class mapCell* cell);
    i32 DamageGroup(class armyGroup* ag, class hero* loser, class hero*, float dmg);
    float StatChangeValue(i32 oldValue, i32 newValue);
    void IncrementHourGlass(void);
    void TownEvent(class mapCell* cell, class hero* heroPointer, i32 x, i32 y);
    i32 ComputeUpgradeValue(i32 baseCreatureType, i32 upgradedCreatureType);
    i32 ComputeValueOfSS(class hero* heroPointer, i32 skill, i32 level);
    i32 ComputeValueOfFreeSS(class hero* heroPointer, i32 skill);
    i32 ManaRefreshValue(class hero* heroPointer, i32 level);
    i32 ValueOfEventAtPosition(class hero* pHero, i16 x, i16 y, i32 immediate, i32* liveChance);
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
extern i32 gbTroopReload;
extern i32 giMaxHeroesForThisPlayer;
extern i8 gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern float gfHeroInteractionBonus[];
extern float gAttackHumanBonus;
extern float gAttackComputerBonus;
extern i16 gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
enum AIPlaceVisitConstant {
    ADVMGR_PLACE_VISIT_COUNT = 30,
    ADVMGR_PLACE_COORDINATE_COUNT = 2
};
extern i32 iPlacesVisited[ADVMGR_PLACE_VISIT_COUNT][ADVMGR_PLACE_COORDINATE_COUNT];
extern i32 iCurPlaceToVisit;
void ResetHeroRVs(i32 resetAll, i32 x, i32 y);
extern i8 giBestShipyardId;
extern i8 gbPossibleShipyardFound;
extern i8 gbActualShipyardFound;
extern i8 gbActualBoatFound;
extern i8 giBestShipyardDist;
extern i16 gaiHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i16 gaiLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i16 gaiHeroLiveChance[];
extern class searchArray SVSearchArray;
extern float fReduceFactor;
enum AIResourceValue {
    RV_UNSET = -32001
};
enum MapExtraFlag {
    MAP_EXTRA_MONSTER_ADJACENT = 0x80
};

extern i8 mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern u8 giCurPlayerHighBit;
extern u8 gCurWatchPlayerHighBit;
void AiPrint(char* text);
void AbsAiPrint(char* text);
extern i8 gShowComputerRoute;
extern u8 giCurWatchPlayerBit;
extern u8 giCurPlayerBit;
extern playerData* gpCurPlayer;
extern i8 giCurPlayer;
extern i32 giCurTurn;

#endif
