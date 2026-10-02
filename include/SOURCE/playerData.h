#ifndef HOMM1_SOURCE_PLAYERDATA_H
#define HOMM1_SOURCE_PLAYERDATA_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 6 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>

// clang-format off
H1_ENUM_CONST_BEGIN(PlayerDataConstant)
    PLAYER_HERO_CAPACITY = 8,
    PLAYER_TOWN_CAPACITY = 36,
    PLAYER_RESOURCE_COUNT = 7
H1_ENUM_CONST_END(PlayerDataConstant)

// A computer seat's playerData::m_difficulty, named by gPlayerTypeNames
// ("None", "Dumb", "Average", "Smart", "Genius"); NONE marks an empty seat.
// Human seats reuse the field as a gHandicapNames index.
H1_ENUM_BEGIN(ComputerPlayerType)
    PLAYER_TYPE_NONE = 0,
    PLAYER_TYPE_DUMB = 1,
    PLAYER_TYPE_AVERAGE = 2,
    PLAYER_TYPE_SMART = 3,
    PLAYER_TYPE_GENIUS = 4,
    PLAYER_TYPE_COUNT = 5,
    // game::PerDay gives types above these daily wood and ore, and above the
    // second one also the resource of the weekday.
    PLAYER_TYPE_NO_WOOD_ORE_BONUS_LAST = 2,
    PLAYER_TYPE_NO_WEEKDAY_BONUS_LAST = 3
H1_ENUM_END(ComputerPlayerType)
    // clang-format on

    // TurnCostResource's &players[p]+0xa1 base and +0x34 income rows place
    // HoMM2's per-player AI block (without its last float) inside playerData.
    struct playerAttentionWeights {
    float gameWeightA;
    float gameRemainder;
    float gameWeightB;
    float buildingValue;
    float upgradeBase;
    float heroValue;
};

class playerAIData {
public:
    playerAttentionWeights m_attentionWeights;
    char m_unknown18[0x1c];
    int m_income[PLAYER_RESOURCE_COUNT];
    int m_obeliskValue;
    // GetTurnAIVars stores MeanRVOfUnexploredTerritory here (+0xf5).
    int m_unexploredValue;
    // EvaluateOneTimeCreaturePurchase weights fight value by the float at +0xf9;
    // FightEvent adds the artifact float at +0xfd.
    float m_upgradeValueWeight;
    float m_artifactValue;
    // GetTurnAIVars' float share 1/(players + dead players) per player.
    float m_artifactPoolShare;
};

// Retail strides players by 0x105 bytes from game+0x20c (four records end at
// the 0x620 world map); fields follow HoMM2's order after a HoMM1 prefix.
#pragma pack(push, 1)
class playerData {
public:
    char m_unknown00[0x11];
    // Buka m_color (Color()); SetupThievesGuild adds it to the town-window
    // flag frame base.
    signed char m_color;
    // Computer-player difficulty: GetTurnAIVars scales the attack bonuses by
    // it and hero::CalcMobility grants computer heroes +3 from level 3.
    signed char m_difficulty;
    signed char m_heroCount;
    signed char m_currentHero;
    signed char m_heroLocatorPage;
    signed char m_heroIds[PLAYER_HERO_CAPACITY];
    signed char m_availableHeroIds[2];
    char m_unknown20[0x32];
    // Saved one byte at a time between the hero and town blocks.
    signed char m_ultimateArtifactHintChance;
    signed char m_ultimateArtifactHintX;
    signed char m_ultimateArtifactHintY;
    // Buka m_daysLeft: 7 when the last town falls, -1 when a town is held;
    // the turn counts it down.
    signed char m_daysLeft;
    signed char m_townCount;
    signed char m_currentTown;
    signed char m_townLocatorPage;
    signed char m_townIds[PLAYER_TOWN_CAPACITY];
    int m_resources[PLAYER_RESOURCE_COUNT];
    char m_unknown99[2];
    unsigned char m_obelisksVisited[6];
    playerAIData m_aiData;
    // --- methods ---
    void Write(int);
    void Read(int);
    signed char NextHero(int);
    signed char HasMobileHero(void);
    int BuildingsOwned(int, int, int);
    int NumOfGivenArtifact(int);
    signed char CountVisitedObelisks(void);
    signed char CurrentHero(void) {
        return m_currentHero;
    }
    signed char CurrentTown(void) {
        return m_currentTown;
    }
    // Buka Color(); RecruitHero's crest index inlines this byte read.
    signed char Color(void) {
        return m_color;
    }
};
#pragma pack(pop)

extern playerData* gpCurPlayer;
extern signed char giCurPlayer;
extern int giCurTurn;
#endif // HOMM1_SOURCE_PLAYERDATA_H
