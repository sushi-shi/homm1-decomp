#ifndef HOMM1_SOURCE_PLAYERDATA_H
#define HOMM1_SOURCE_PLAYERDATA_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 6 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/gameTypes.h>
#include <SOURCE/resourceTypes.h>

// playerData::m_heroIds: a player keeps at most eight heroes (TOWNMGR's
// recruit and swap tests).
// The obelisk puzzle has 48 pieces kept as bits in m_obelisksVisited (Buka
// PUZZLE_PIECE_COUNT / PUZZLE_PIECE_STORAGE_SIZE). The ultimate-artifact
// hint coordinates are HINT_NONE until ComputeUALoc places them. Write/Read
// save 50 zero bytes between the hero and hint blocks.
H1_ENUM_CONST_BEGIN(PlayerDataConstant)
    PLAYER_HERO_CAPACITY = 8,
    // m_availableHeroIds: the two heroes a player's taverns offer.
    PLAYER_TAVERN_HERO_COUNT = 2,
    PLAYER_PUZZLE_PIECE_COUNT = 48,
    PLAYER_PUZZLE_PIECE_STORAGE_SIZE = (PLAYER_PUZZLE_PIECE_COUNT + 7) / 8,
    PLAYER_ULTIMATE_HINT_NONE = -1,
    PLAYER_SAVE_PAD_SIZE = 50
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

// A human seat's playerData::m_difficulty indexes gHandicapNames ("Human-",
// "Human-Easy", "Human-Normal", "Human-Hard", "Human-Expert"); the scenario
// info prints gHandicapNames[game difficulty + 1] for the game itself.
// NewGameHandler never leaves a human seat at NONE (it wraps to EASY) and
// InitCampaignMap gives the campaign's human EXPERT.
H1_ENUM_BEGIN(HumanHandicap)
    HUMAN_HANDICAP_NONE = 0,
    HUMAN_HANDICAP_EASY = 1,
    HUMAN_HANDICAP_NORMAL = 2,
    HUMAN_HANDICAP_HARD = 3,
    HUMAN_HANDICAP_EXPERT = 4
H1_ENUM_END(HumanHandicap)

// Player colours: playerData::m_color indexes the constructor's flag ICNs
// (b-/g-/r-/y-flag32.icn and -bflg32.icn) and KB's gColorNames
// {"blue", "green", "red", "yellow"} (Buka PlayerColor, first four values);
// UpdateRadar uses slot 4 of gRadarOwnerColor for unowned towns and mines.
H1_ENUM_BEGIN(PlayerColor)
    // A seat whose crest is not chosen yet (the new-game and scenario
    // windows skip the crest frame).
    PLAYER_COLOR_NONE = -1,
    PLAYER_COLOR_BLUE = 0,
    PLAYER_COLOR_GREEN = 1,
    PLAYER_COLOR_RED = 2,
    PLAYER_COLOR_YELLOW = 3,
    PLAYER_COLOR_COUNT = 4,
    PLAYER_COLOR_NEUTRAL = 4
H1_ENUM_END(PlayerColor)

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
    char m_unused18[0x1c];
    i32 m_income[RESOURCE_COUNT];
    i32 m_obeliskValue;
    // GetTurnAIVars stores MeanRVOfUnexploredTerritory here (+0xf5).
    i32 m_unexploredValue;
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
    H1_ENUM_STORAGE(PlayerColor, i8) m_color;
    // Computer-player difficulty: GetTurnAIVars scales the attack bonuses by
    // it and hero::CalcMobility grants computer heroes +3 from level 3.
    i8 m_difficulty;
    i8 m_heroCount;
    i8 m_currentHero;
    i8 m_heroLocatorPage;
    i8 m_heroIds[PLAYER_HERO_CAPACITY];
    i8 m_availableHeroIds[PLAYER_TAVERN_HERO_COUNT];
    // Buka m_unusedSaveData: Write stores zeros and Read skips this span.
    char m_unusedSaveData[0x32];
    // Saved one byte at a time between the hero and town blocks.
    i8 m_ultimateArtifactHintChance;
    i8 m_ultimateArtifactHintX;
    i8 m_ultimateArtifactHintY;
    // Buka m_daysLeft: 7 when the last town falls, -1 when a town is held;
    // the turn counts it down.
    i8 m_daysLeft;
    i8 m_townCount;
    i8 m_currentTown;
    i8 m_townLocatorPage;
    i8 m_townIds[GAME_TOWN_COUNT];
    i32 m_resources[RESOURCE_COUNT];
    char m_unknown99[2];
    u8 m_obelisksVisited[PLAYER_PUZZLE_PIECE_STORAGE_SIZE];
    playerAIData m_aiData;
    // --- methods ---
    void Write(i32 file);
    void Read(i32 file);
    i8 NextHero(i32);
    i8 HasMobileHero(void);
    i32 BuildingsOwned(i32 townType, i32 buildingIndex, i32 buildState);
    i32 NumOfGivenArtifact(i32 artifact);
    i8 CountVisitedObelisks(void);
    i8 CurrentHero(void) {
        return m_currentHero;
    }
    i8 CurrentTown(void) {
        return m_currentTown;
    }
    // Buka crest reads widen the stored signed byte to a signed short.
    H1_ENUM_RETURN(PlayerColor, i16) Color(void) {
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

#endif // HOMM1_SOURCE_PLAYERDATA_H
