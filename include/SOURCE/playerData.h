#ifndef HOMM1_SOURCE_PLAYERDATA_H
#define HOMM1_SOURCE_PLAYERDATA_H

#include <SOURCE/artifactTypes.h>
#include <SOURCE/gameTypes.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/town.h>

enum PlayerDataConstant {
    PLAYER_HERO_CAPACITY = 8,
    PLAYER_TAVERN_HERO_COUNT = 2,
    PLAYER_PUZZLE_PIECE_COUNT = 48,
    PLAYER_PUZZLE_PIECE_STORAGE_SIZE = (PLAYER_PUZZLE_PIECE_COUNT + 7) / 8,
    PLAYER_ULTIMATE_HINT_NONE = -1,
    PLAYER_SAVE_PAD_SIZE = 50
};

enum ComputerPlayerType {
    PLAYER_TYPE_NONE = 0,
    PLAYER_TYPE_DUMB = 1,
    PLAYER_TYPE_AVERAGE = 2,
    PLAYER_TYPE_SMART = 3,
    PLAYER_TYPE_GENIUS = 4,
    PLAYER_TYPE_COUNT = 5,
    PLAYER_TYPE_NO_WOOD_ORE_BONUS_LAST = 2,
    PLAYER_TYPE_NO_WEEKDAY_BONUS_LAST = 3,
    PLAYER_TYPE_MOBILITY_BONUS_FIRST = 3
};

enum HumanHandicap {
    HUMAN_HANDICAP_NONE = 0,
    HUMAN_HANDICAP_EASY = 1,
    HUMAN_HANDICAP_NORMAL = 2,
    HUMAN_HANDICAP_HARD = 3,
    HUMAN_HANDICAP_EXPERT = 4
};

enum PlayerColor {
    PLAYER_COLOR_NONE = -1,
    PLAYER_COLOR_BLUE = 0,
    PLAYER_COLOR_GREEN = 1,
    PLAYER_COLOR_RED = 2,
    PLAYER_COLOR_YELLOW = 3,
    PLAYER_COLOR_COUNT = 4,
    PLAYER_COLOR_NEUTRAL = 4
};

struct playerAttentionWeights {
    float gameBuildingAttention;
    float gameCreatureAttention;
    float gameHeroAttention;
    float turnBuildingAttention;
    float turnCreatureAttention;
    float turnHeroAttention;
};

class playerAIData {
public:
    playerAttentionWeights m_attentionWeights;
    char m_unused18[0x1c];
    i32 m_income[RESOURCE_COUNT];
    i32 m_obeliskValue;
    i32 m_unexploredValue;
    float m_fightValueResourceWeight;
    float m_meanArtifactValue;
    float m_playerShare;
};

#pragma pack(push, 1)
class playerData {
public:
    char m_unused00[0x11];
    i8 m_color;
    i8 m_difficulty;
    i8 m_heroCount;
    i8 m_currentHero;
    i8 m_heroLocatorPage;
    i8 m_heroIds[PLAYER_HERO_CAPACITY];
    i8 m_availableHeroIds[PLAYER_TAVERN_HERO_COUNT];
    char m_unusedSaveData[0x32];
    i8 m_ultimateArtifactHintChance;
    i8 m_ultimateArtifactHintX;
    i8 m_ultimateArtifactHintY;
    i8 m_daysLeft;
    i8 m_townCount;
    i8 m_currentTown;
    i8 m_townLocatorPage;
    i8 m_townIds[GAME_TOWN_COUNT];
    i32 m_resources[RESOURCE_COUNT];
    i8 m_unused99;
    i8 m_unused9a;
    u8 m_puzzlePiecesRemoved[PLAYER_PUZZLE_PIECE_STORAGE_SIZE];
    playerAIData m_aiData;
    void Write(i32 file);
    void Read(i32 file);
    i8 NextHero(i32);
    b8 HasMobileHero(void);
    i32 BuildingsOwned(
        i32 townType,
        i32 buildingIndex,
        i32 mageGuildLevel
    );
    i32 NumOfGivenArtifact(i32 artifact);
    i8 CountPuzzlePiecesRemoved(void);
    i8 CurrentHero(void) {
        return m_currentHero;
    }
    i8 CurrentTown(void) {
        return m_currentTown;
    }
    i16 Color(void) {
        return m_color;
    }
    i8 HeroCount(void) {
        return m_heroCount;
    }
    i8 TownCount(void) {
        return m_townCount;
    }
    i8 HeroId(i32 index) {
        return m_heroIds[index];
    }
    i8 TownId(i32 index) {
        return m_townIds[index];
    }
    i8 AvailableHeroId(i32 index) {
        return m_availableHeroIds[index];
    }
};
#pragma pack(pop)

#endif
