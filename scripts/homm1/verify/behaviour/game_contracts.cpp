// Game-behaviour contracts: links the reconstructed game objects into a
// console program and prints what the game's own tables and functions yield
// on fixed fixtures. test_game_contracts.py compares the output with
// game_contracts.expected, a snapshot taken from the retail-exact build.
//
//   game_contracts.exe                    tables, formulas, AI, pathing and
//                                         synthetic save/map records
//   game_contracts.exe --installed DIR    an imported game copy (read only):
//                                         its saves, ORIGDATA.BIN and maps
#include <match.h>

#include <BASE/audio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>
#include <SOURCE/KB.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/fileRequester.h>

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// FNV-1a over a byte range: one word that changes with any byte of a record.
static u32 Digest(const void* data, u32 size, u32 hash = 2166136261u) {
    const u8* bytes = static_cast<const u8*>(data);
    for (u32 i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

static void Line(const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    vprintf(format, arguments);
    va_end(arguments);
    printf("\n");
}

// "[wood,mercury,ore,sulfur,crystal,gems,gold]"
static const char* Resources(const i32* amounts) {
    static char text[128];
    sprintf(
        text,
        "[%d,%d,%d,%d,%d,%d,%d]",
        amounts[0],
        amounts[1],
        amounts[2],
        amounts[3],
        amounts[4],
        amounts[5],
        amounts[6]
    );
    return text;
}

static void Floats(const char* label, const float* values, i32 count) {
    printf("%s:", label);
    for (i32 i = 0; i < count; i++)
        printf(" %.3f", values[i]);
    printf("\n");
}

// A deterministic byte stream for record fixtures.
static u32 gFixtureSeed;
static u8 FixtureByte(void) {
    gFixtureSeed = gFixtureSeed * 1103515245u + 12345u;
    return static_cast<u8>(gFixtureSeed >> 16);
}
static void FixtureBytes(void* data, u32 size) {
    u8* bytes = static_cast<u8*>(data);
    for (u32 i = 0; i < size; i++)
        bytes[i] = FixtureByte();
}

// The managers InitMainClasses would create, without a window or resources:
// the combat screen stays closed, so combat code skips its drawing.
static void SetUpManagers(void) {
    gGame = new game;
    memset(gGame, 0, sizeof(game));
    gAdvManager = new advManager;
    gAdvManager->m_mapData = gGame->m_map;
    gAdvManager->m_heroContextLocked = false;
    gCombatManager = new combatManager;
    gSearchArray = new searchArray;
    gPhilAI = new philAI;
    gMonGroup = new armyGroup;
    InitVars();
    gCurPlayer = 0;
    gCurPlayerData = &gGame->m_players[0];
    gCurPlayerBit = 1;
    for (i32 player = 0; player < GAME_PLAYER_COUNT; player++)
        gHumanPlayer[player] = false;
}

static void ClearGroup(armyGroup& group) {
    CLEAR_ARMY_GROUP(group);
}

static void SetUpHero(hero& h, i8 heroClass, i8 attack, i8 defense, i8 power, i8 knowledge) {
    memset(&h, 0, sizeof(hero));
    h.m_heroClass = heroClass;
    h.m_owner = 0;
    h.m_primaryStats[HERO_PRIMARY_ATTACK] = attack;
    h.m_primaryStats[HERO_PRIMARY_DEFENSE] = defense;
    h.m_primaryStats[HERO_PRIMARY_SPELL_POWER] = power;
    h.m_primaryStats[HERO_PRIMARY_KNOWLEDGE] = knowledge;
    memset(h.m_spells, SPELL_NONE, sizeof(h.m_spells));
    memset(h.m_artifacts, ARTIFACT_NONE, sizeof(h.m_artifacts));
    ClearGroup(h.m_army);
}

// --- tables -----------------------------------------------------------------

static void CreatureTable(void) {
    i32 cost[RESOURCE_COUNT];
    for (i32 creature = CREATURE_FIRST; creature < CREATURE_COUNT; creature++) {
        const tag_monsterInfo& info = gMonsterDatabase[creature];
        const tag_monsterStats& stats = info.stats;
        GetMonsterCost(creature, cost);
        Line(
            "creature %2d: cost %s fight %d growth %d hp %d speed %d att %d def %d "
            "dmg %d-%d shots %d missile %d pow %d attr 0x%02x",
            creature,
            Resources(cost),
            info.fightValue,
            info.growth,
            stats.hitPoints,
            stats.speed,
            stats.attack,
            stats.defense,
            stats.damageMin,
            stats.damageMax,
            stats.shots,
            stats.missileType,
            stats.powEffect,
            stats.attributes
        );
    }
    Line("creature table digest %08x", Digest(gMonsterDatabase, sizeof(gMonsterDatabase)));
}

static void TownTables(void) {
    i32 cost[RESOURCE_COUNT];
    for (i32 level = 0; level < 4; level++) {
        GetBuildingCost(TOWN_TYPE_KNIGHT, BUILDING_SLOT_MAGE_GUILD, cost, level);
        Line(
            "mage guild level %d: cost %s value %d spells %d",
            level + 1,
            Resources(cost),
            GetBuildingBaseResourceValue(TOWN_TYPE_KNIGHT, BUILDING_SLOT_MAGE_GUILD, level),
            gMageGuildSpellCount[level]
        );
    }
    for (i32 building = BUILDING_SLOT_THIEVES_GUILD; building < BUILDING_SLOT_DWELLING_FIRST;
         building++) {
        GetBuildingCost(TOWN_TYPE_KNIGHT, building, cost, 0);
        Line(
            "building %d: cost %s value %d",
            building,
            Resources(cost),
            GetBuildingBaseResourceValue(TOWN_TYPE_KNIGHT, building, 0)
        );
    }
    for (i32 type = TOWN_TYPE_KNIGHT; type < TOWN_TYPE_COUNT; type++) {
        for (i32 dwelling = 0; dwelling < BUILDING_SLOT_DWELLING_COUNT; dwelling++) {
            i32 building = BUILDING_SLOT_DWELLING_FIRST + dwelling;
            GetBuildingCost(type, building, cost, 0);
            Line(
                "town %d dwelling %d: creature %d requires 0x%04x cost %s value %d",
                type,
                dwelling,
                gDwellingType[type][dwelling],
                gDwellingRequirements[type * BUILDING_SLOT_DWELLING_COUNT + dwelling],
                Resources(cost),
                GetBuildingBaseResourceValue(type, building, 0)
            );
        }
    }
    for (i32 race = TOWN_TYPE_KNIGHT; race < TOWN_TYPE_COUNT; race++)
        Line(
            "town %d: hero class %d theme %d",
            race,
            gTownHeroClass[race],
            gTownTheme[race]
        );
    for (i32 color = 0; color < PLAYER_COLOR_COUNT; color++)
        Line(
            "crest %d: town %d hero class %d",
            color,
            gCrestTownTypes[color],
            gCrestHeroClass[color]
        );
    for (i32 difficulty = 0; difficulty < DIFFICULTY_COUNT; difficulty++)
        Line(
            "starting resources difficulty %d: %s",
            difficulty,
            Resources(gStartingResources[difficulty])
        );
    Line("mine income: %s", Resources(gMineIncome));
    Line("resource base value: %s", Resources(gResourceBaseValue));
    Line("hero gold cost %d, town vision %d", gHeroGoldCost, gVisRangeTown);
}

static void HeroTables(void) {
    for (i32 heroClass = 0; heroClass < HERO_CLASS_COUNT; heroClass++) {
        printf("class %d: min exp", heroClass);
        for (i32 level = 0; level < HERO_EXPERIENCE_LEVEL_TABLE_COUNT; level++)
            printf(" %d", gMinExpForLevel[heroClass][level]);
        printf("; skill rows");
        for (i32 row = 0; row < 9; row++) {
            printf(" ");
            for (i32 stat = 0; stat < HERO_PRIMARY_STAT_COUNT; stat++)
                printf("%s%d", stat ? "/" : "", gHeroSkillBonus[heroClass][row][stat]);
        }
        printf("\n");
    }
    printf("class navigation:");
    for (i32 navigationClass = 0; navigationClass < 8; navigationClass++)
        printf(" %.2f", gClassNavigationMod[navigationClass]);
    printf("; scout radius:");
    for (i32 scoutClass = 0; scoutClass < 8; scoutClass++)
        printf(" %d", gHeroScoutRadius[scoutClass]);
    printf("\n");
}

static void SpellAndArtifactTables(void) {
    printf("spell AI value/flags:");
    for (i32 spell = SPELL_FIRST; spell < SPELL_COUNT; spell++)
        printf(" %d/%d", gSpellAIValue[spell], gSpellAIFlags[spell]);
    printf("\n");
    for (i32 level = 0; level < 4; level++) {
        printf("mage guild pool level %d:", level + 1);
        for (i32 i = 0; i < 8; i++)
            printf(" %d", gMageGuildSpellPool[level][i]);
        printf("\n");
    }
    Floats("spell cast count weight", gSpellCastNumMod, SPELL_CAST_COUNT_LAST + 1);
    printf("artifact base value:");
    for (i32 artifact = ARTIFACT_FIRST; artifact < ARTIFACT_REGULAR_END; artifact++)
        printf(" %d", gArtifactBaseRV[artifact]);
    printf("; ultimate average %d\n", gUltArtifactAvgValue);
    Floats("battle stat curve", gBattleStat, STAT_CURVE_LAST + 1);
    Floats("fight stat power curve", gStatPower, STAT_CURVE_LAST + 1);
}

// --- formulas ---------------------------------------------------------------

static void TerrainCosts(void) {
    for (i32 terrain = TERRAIN_WATER; terrain < TERRAIN_COUNT; terrain++) {
        printf(
            "terrain %d: step %d/%d table %d/%d;",
            terrain,
            TerrainStepCost(terrain, FINDPATH_STEP_STRAIGHT),
            TerrainStepCost(terrain, FINDPATH_STEP_DIAGONAL),
            gTerrainCost[terrain][FINDPATH_STEP_STRAIGHT],
            gTerrainCost[terrain][FINDPATH_STEP_DIAGONAL]
        );
        // The step a hero with 0..12 movement left pays, straight and
        // diagonal, for a knight and a barbarian.
        for (i32 heroClass = 0; heroClass < 2; heroClass++) {
            printf(" class %d", heroClass);
            for (i32 mobility = 0; mobility <= 12; mobility += 2)
                printf(
                    " %d/%d",
                    CalcTerrainCost(terrain, FINDPATH_STEP_STRAIGHT, mobility, heroClass),
                    CalcTerrainCost(terrain, FINDPATH_STEP_DIAGONAL, mobility, heroClass)
                );
        }
        printf("\n");
    }
}

static void ExperienceAndMobility(void) {
    hero h;
    for (i32 heroClass = 0; heroClass < HERO_CLASS_COUNT; heroClass++) {
        SetUpHero(h, heroClass, 0, 0, 0, 0);
        printf("class %d experience by level:", heroClass);
        for (i32 level = 1; level <= 20; level++)
            printf(" %d", h.GetExperience(level));
        printf("; level of");
        static const i32 samples[] = {0, 999, 1000, 5000, 20000, 100000, 1000000};
        for (i32 i = 0; i < 7; i++)
            printf(" %d=%d", samples[i], h.GetLevel(samples[i]));
        printf("\n");
    }
    // Slowest creature, boots, compass, sea travel with and without the
    // lighthouse and astrolabe, and the computer seat's bonus.
    struct MobilityCase {
        const char* label;
        i8 heroClass;
        i8 creature;
        i8 artifact;
        i8 embarked;
        i8 lighthouse;
        i8 computerType;
    };
    static const MobilityCase cases[] = {
        {"peasants", 0, CREATURE_PEASANT, ARTIFACT_NONE, 0, 0, -1},
        {"swordsmen", 0, CREATURE_SWORDSMAN, ARTIFACT_NONE, 0, 0, -1},
        {"cavalry", 0, CREATURE_CAVALRY, ARTIFACT_NONE, 0, 0, -1},
        {"phoenix", 0, CREATURE_PHOENIX, ARTIFACT_NONE, 0, 0, -1},
        {"nomad boots", 0, CREATURE_PEASANT, ARTIFACT_NOMAD_BOOTS, 0, 0, -1},
        {"traveler boots", 0, CREATURE_PEASANT, ARTIFACT_TRAVELER_BOOTS, 0, 0, -1},
        {"true compass", 0, CREATURE_PEASANT, ARTIFACT_TRUE_COMPASS, 0, 0, -1},
        {"knight at sea", 0, CREATURE_PEASANT, ARTIFACT_NONE, 1, 0, -1},
        {"sorceress at sea", 2, CREATURE_PEASANT, ARTIFACT_NONE, 1, 0, -1},
        {"sea with lighthouse", 0, CREATURE_PEASANT, ARTIFACT_NONE, 1, 1, -1},
        {"sea with astrolabe", 2, CREATURE_PEASANT, ARTIFACT_SAILORS_ASTROLABE, 1, 1, -1},
        {"average computer", 0, CREATURE_PEASANT, ARTIFACT_NONE, 0, 0, PLAYER_TYPE_AVERAGE},
        {"smart computer", 0, CREATURE_PEASANT, ARTIFACT_NONE, 0, 0, PLAYER_TYPE_SMART},
    };
    printf("mobility:");
    for (i32 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const MobilityCase& c = cases[i];
        SetUpHero(h, c.heroClass, 0, 0, 0, 0);
        h.m_army.m_creatureTypes[0] = c.creature;
        h.m_army.m_creatureCounts[0] = 1;
        h.m_army.m_creatureTypes[1] = CREATURE_PALADIN;
        h.m_army.m_creatureCounts[1] = 1;
        h.m_artifacts[0] = c.artifact;
        if (c.embarked)
            h.m_eventFlags |= HERO_EVENT_EMBARKED;
        gGame->m_mines[MINE_SLOT_LIGHTHOUSE].owner = c.lighthouse ? 0 : GAME_PLAYER_NONE;
        gHumanPlayer[0] = c.computerType < 0;
        gGame->m_players[0].m_difficulty = c.computerType < 0 ? 0 : c.computerType;
        printf(" %s %d%s", c.label, h.CalcMobility(), i + 1 < sizeof(cases) / sizeof(cases[0]) ? "," : "");
    }
    printf("\n");
    gHumanPlayer[0] = false;
    gGame->m_players[0].m_difficulty = 0;
    gGame->m_mines[MINE_SLOT_LIGHTHOUSE].owner = GAME_PLAYER_NONE;
}

static void SeededRolls(const char* label, i32 seed, i32 low, i32 high, i32 count) {
    gLastSeed = seed;
    printf("%s:", label);
    for (i32 roll = 0; roll < count; roll++)
        printf(" %d", SRandom(low, high));
    printf("\n");
}

static void LuckAndMorale(void) {
    hero h;
    // GetLuck: the four luck artifacts stack on the hero's own luck,
    // clamped to -3..3.
    printf("luck:");
    static const i8 luckArtifacts[] = {
        ARTIFACT_LUCKY_RABBITS_FOOT,
        ARTIFACT_GOLDEN_HORSESHOE,
        ARTIFACT_GAMBLERS_LUCKY_COIN,
        ARTIFACT_FOUR_LEAF_CLOVER,
    };
    for (i32 own = -4; own <= 2; own += 2) {
        for (i32 count = 0; count <= 4; count++) {
            SetUpHero(h, 0, 0, 0, 0, 0);
            h.m_luck = own;
            for (i32 i = 0; i < count; i++)
                h.m_artifacts[i] = luckArtifacts[i];
            printf(" %d+%d=%d", own, count, gGame->GetLuck(&h, NULL));
        }
    }
    printf("; no hero %d\n", gGame->GetLuck(NULL, NULL));
    // GetMorale: the races in the group, the knight's bonus, the hero's own
    // morale, the medals and Fizbin, and a tavern.
    struct MoraleCase {
        const char* label;
        i8 creatures[ARMY_GROUP_SLOT_COUNT];
        i8 heroClass;
        i8 heroMorale;
        i8 artifacts[2];
        i8 tavern;
    };
    static const MoraleCase cases[] = {
        {"one race no hero", {0, 1, 2, -1, -1}, -1, 0, {-1, -1}, 0},
        {"two races", {0, 6, -1, -1, -1}, 2, 0, {-1, -1}, 0},
        {"three races", {0, 6, 12, -1, -1}, 2, 0, {-1, -1}, 0},
        {"four races", {0, 6, 12, 18, -1}, 2, 0, {-1, -1}, 0},
        {"five races", {0, 6, 12, 18, 24}, 2, 0, {-1, -1}, 0},
        {"knight one race", {0, 1, -1, -1, -1}, 0, 0, {-1, -1}, 0},
        {"medals", {0, 6, -1, -1, -1}, 2, 0, {ARTIFACT_MEDAL_OF_VALOR, ARTIFACT_MEDAL_OF_HONOR}, 0},
        {"fizbin", {0, 6, -1, -1, -1}, 2, 0, {ARTIFACT_FIZBIN_OF_MISFORTUNE, -1}, 0},
        {"hero morale -2 tavern", {0, 6, 12, -1, -1}, 2, -2, {-1, -1}, 1},
        {"clamped high", {0, 1, -1, -1, -1}, 0, 3, {ARTIFACT_MEDAL_OF_VALOR, -1}, 1},
        {"clamped low", {0, 6, 12, 18, 24}, 2, -2, {ARTIFACT_FIZBIN_OF_MISFORTUNE, -1}, 0},
    };
    printf("morale:");
    town t;
    for (i32 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const MoraleCase& c = cases[i];
        armyGroup group;
        ClearGroup(group);
        for (i32 slot = 0; slot < ARMY_GROUP_SLOT_COUNT; slot++) {
            group.m_creatureTypes[slot] = c.creatures[slot];
            group.m_creatureCounts[slot] = c.creatures[slot] < 0 ? 0 : 10;
        }
        SetUpHero(h, c.heroClass < 0 ? 0 : c.heroClass, 0, 0, 0, 0);
        h.m_morale = c.heroMorale;
        h.m_artifacts[0] = c.artifacts[0];
        h.m_artifacts[1] = c.artifacts[1];
        t.m_buildings = c.tavern ? H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TAVERN) : 0;
        printf(
            " %s %d/%d%s",
            c.label,
            group.IsHomogeneous(ARMY_GROUP_EMPTY_SLOT),
            group.GetMorale(c.heroClass < 0 ? NULL : &h, &t),
            i + 1 < sizeof(cases) / sizeof(cases[0]) ? "," : ""
        );
    }
    printf("\n");
    // The rolls CheckLuck (SRandom(1, 12)) and the morale checks
    // (SRandom(1, 24) good, SRandom(1, 12) bad) draw from one seed; good luck
    // fires on a roll <= luck, good morale on a roll <= morale.
    SeededRolls("luck rolls from seed 1", 1, 1, 12, 24);
    SeededRolls("morale rolls from seed 1", 1, 1, 24, 24);
    SeededRolls("seeded rolls from seed 4242", 4242, 0, 99, 16);
    printf("seed after them %d\n", gLastSeed);
    srand(1);
    printf("Random(0, 99) after srand(1):");
    for (i32 roll = 0; roll < 16; roll++)
        printf(" %d", Random(0, 99));
    printf("\n");
}

// One attack through army::DamageEnemy and army::Damage on stacks set up by
// army::Init, with the combat screen closed.
static void Attack(
    const char* label,
    i8 attackerType,
    i16 attackerCount,
    i8 defenderType,
    i16 defenderCount,
    hero* attackerHero,
    i8 damageMode,
    i8 luck,
    b32 ranged,
    i32 defenseModifier,
    i32 seed,
    b32 quiet = false
) {
    combatManager* combat = gCombatManager;
    combat->m_heroes[COMBAT_DEFENDER_SIDE] = NULL;
    combat->m_heroes[COMBAT_ATTACKER_SIDE] = attackerHero;
    army& attacker = combat->m_armies[COMBAT_ATTACKER_SIDE][0];
    army& defender = combat->m_armies[COMBAT_DEFENDER_SIDE][0];
    attacker.Init(attackerType, attackerCount, COMBAT_ATTACKER_SIDE, 0);
    defender.Init(defenderType, defenderCount, COMBAT_DEFENDER_SIDE, 0);
    attacker.m_hex = 12;
    defender.m_hex = 14;
    attacker.m_damageMode = damageMode;
    attacker.m_luck = luck;
    combat->m_currentSide = COMBAT_ATTACKER_SIDE;
    combat->m_currentArmyIndex = 0;
    combat->m_combatWindowOpen = false;
    gLastSeed = seed;
    i32 damage = 0;
    i32 killed = 0;
    attacker.DamageEnemy(&defender, &damage, &killed, ranged, defenseModifier);
    if (quiet)
        return;
    Line(
        "attack %s: damage %d killed %d left %d hp lost %d luck after %d",
        label,
        damage,
        killed,
        defender.m_quantity,
        defender.m_hitPointsLost,
        attacker.m_luck
    );
}

static void Damage(void) {
    hero strong;
    SetUpHero(strong, 0, 5, 2, 1, 1);
    Attack("10 swordsmen on 20 orcs", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, 0, 0, false, 0, 7);
    Attack("same, seed 99", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, 0, 0, false, 0, 99);
    Attack("minimum roll", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, ARMY_DAMAGE_MINIMUM, 0, false, 0, 7);
    Attack("maximum roll", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, ARMY_DAMAGE_MAXIMUM, 0, false, 0, 7);
    Attack("halved", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, ARMY_DAMAGE_HALF, 0, false, 0, 7);
    Attack("good luck", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, ARMY_DAMAGE_MAXIMUM, ARMY_LUCK_GOOD, false, 0, 7);
    Attack("bad luck", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, ARMY_DAMAGE_MAXIMUM, ARMY_LUCK_BAD, false, 0, 7);
    Attack("hero attack 5", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, &strong, ARMY_DAMAGE_MAXIMUM, 0, false, 0, 7);
    Attack("castle wall defense", CREATURE_SWORDSMAN, 10, CREATURE_ORC, 20, NULL, ARMY_DAMAGE_MAXIMUM, 0, false, ARMY_CASTLE_WALL_DEFENSE_BONUS, 7);
    Attack("archers shooting", CREATURE_ARCHER, 12, CREATURE_PEASANT, 100, NULL, ARMY_DAMAGE_MAXIMUM, 0, true, 0, 7);
    Attack("archers in melee", CREATURE_ARCHER, 12, CREATURE_PEASANT, 100, NULL, ARMY_DAMAGE_MAXIMUM, 0, false, 0, 7);
    Attack("peasants on a dragon", CREATURE_PEASANT, 50, CREATURE_DRAGON, 1, NULL, 0, 0, false, 0, 7);
    Attack("dragons on peasants", CREATURE_DRAGON, 3, CREATURE_PEASANT, 200, NULL, 0, 0, false, 0, 7);
    Attack("one peasant on a dragon", CREATURE_PEASANT, 1, CREATURE_DRAGON, 1, NULL, ARMY_DAMAGE_MINIMUM, ARMY_LUCK_BAD, false, 0, 7);
    // The genie's special (SRandom(1, 5) == 2) deals half the target stack
    // when that beats the rolled damage.
    i32 halved = 0;
    i32 firstHalved = 0;
    for (i32 seed = 1; seed <= 100; seed++) {
        Attack("", CREATURE_GENIE, 2, CREATURE_CAVALRY, 30, NULL, 0, 0, false, 0, seed, true);
        if (gGenieHalf) {
            halved++;
            if (!firstHalved)
                firstHalved = seed;
        }
    }
    Line("genie halves the cavalry on %d of seeds 1..100, first %d", halved, firstHalved);
    Attack("genie on cavalry, no special", CREATURE_GENIE, 2, CREATURE_CAVALRY, 30, NULL, 0, 0, false, 0, 1);
    Attack("genie on cavalry, special", CREATURE_GENIE, 2, CREATURE_CAVALRY, 30, NULL, 0, 0, false, 0, firstHalved);
}

// --- AI ---------------------------------------------------------------------

static void SetGroup(armyGroup& group, i8 type0, i16 count0, i8 type1 = -1, i16 count1 = 0, i8 type2 = -1, i16 count2 = 0) {
    ClearGroup(group);
    group.m_creatureTypes[0] = type0;
    group.m_creatureCounts[0] = count0;
    group.m_creatureTypes[1] = type1;
    group.m_creatureCounts[1] = count1;
    group.m_creatureTypes[2] = type2;
    group.m_creatureCounts[2] = count2;
}

static void ArtificialIntelligence(void) {
    armyGroup knights;
    armyGroup goblins;
    armyGroup dragons;
    armyGroup garrison;
    SetGroup(knights, CREATURE_SWORDSMAN, 20, CREATURE_ARCHER, 15, CREATURE_CAVALRY, 6);
    SetGroup(goblins, CREATURE_GOBLIN, 120, CREATURE_ORC, 40, CREATURE_WOLF, 9);
    SetGroup(dragons, CREATURE_DRAGON, 2, CREATURE_SPRITE, 30);
    SetGroup(garrison, CREATURE_PIKEMAN, 25);
    hero warrior;
    SetUpHero(warrior, 1, 4, 3, 1, 1);
    warrior.m_morale = 1;
    warrior.m_luck = 1;
    warrior.m_spells[0] = SPELL_FIREBALL;
    warrior.m_spellCharges[0] = 2;
    warrior.m_spells[1] = SPELL_LIGHTNING_BOLT;
    warrior.m_spellCharges[1] = 1;
    warrior.m_artifacts[0] = ARTIFACT_THUNDER_MACE;
    warrior.m_aiFightValue = 1.0f;
    hero mage;
    SetUpHero(mage, 2, 1, 1, 4, 3);
    mage.m_owner = 1;
    mage.m_spells[0] = SPELL_FIREBALL;
    mage.m_spellCharges[0] = 4;
    mage.m_aiFightValue = 1.0f;
    town& castle = gGame->m_castleRecs[3];
    castle.m_id = 3;
    castle.m_type = TOWN_TYPE_SORCERESS;
    castle.m_buildings = H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE)
                         | H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD)
                         | H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_DWELLING_FIRST)
                         | H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_DWELLING_FIRST + 1);

    Line(
        "fight value knights: raw %d adjusted %d with warrior %d",
        gPhilAI->FightValueOfStack(&knights, NULL, false),
        gPhilAI->FightValueOfStack(&knights, NULL, true),
        gPhilAI->FightValueOfStack(&knights, &warrior, true)
    );
    Line(
        "fight value goblins: raw %d adjusted %d with mage %d",
        gPhilAI->FightValueOfStack(&goblins, NULL, false),
        gPhilAI->FightValueOfStack(&goblins, NULL, true),
        gPhilAI->FightValueOfStack(&goblins, &mage, true)
    );
    Line(
        "fight value dragons: raw %d adjusted %d; garrison in castle 3: %d",
        gPhilAI->FightValueOfStack(&dragons, NULL, false),
        gPhilAI->FightValueOfStack(&dragons, NULL, true),
        gPhilAI->FightValueOfStack(&garrison, NULL, true, true, 3)
    );
    Line(
        "experience value: knights %d, knights with hero %d, goblins %d; stack strength %u",
        gGame->ExperienceValueOfStack(&knights, NULL),
        gGame->ExperienceValueOfStack(&knights, &warrior),
        gGame->ExperienceValueOfStack(&goblins, NULL),
        gCombatManager->m_armies[COMBAT_ATTACKER_SIDE][0].Strength()
    );

    struct OutcomeCase {
        const char* label;
        armyGroup* attacker;
        hero* attackerHero;
        armyGroup* defender;
        hero* defenderHero;
        i8 useTown;
        i32 enemyPlayer;
        b32 enemyHuman;
        i8 difficulty;
    };
    OutcomeCase cases[] = {
        {"knights vs neutral goblins", &knights, NULL, &goblins, NULL, 0, GAME_PLAYER_NONE, false, PLAYER_TYPE_AVERAGE},
        {"warrior vs neutral goblins", &knights, &warrior, &goblins, NULL, 0, GAME_PLAYER_NONE, false, PLAYER_TYPE_AVERAGE},
        {"genius warrior vs neutral goblins", &knights, &warrior, &goblins, NULL, 0, GAME_PLAYER_NONE, false, PLAYER_TYPE_GENIUS},
        {"warrior vs human mage", &knights, &warrior, &goblins, &mage, 0, 1, true, PLAYER_TYPE_AVERAGE},
        {"dumb warrior vs human mage", &knights, &warrior, &goblins, &mage, 0, 1, true, PLAYER_TYPE_DUMB},
        {"warrior vs computer mage", &knights, &warrior, &goblins, &mage, 0, 1, false, PLAYER_TYPE_SMART},
        {"warrior vs mage in castle", &knights, &warrior, &garrison, &mage, 1, 1, false, PLAYER_TYPE_SMART},
        {"goblins vs dragons", &goblins, NULL, &dragons, NULL, 0, GAME_PLAYER_NONE, false, PLAYER_TYPE_AVERAGE},
    };
    playerData& player = gGame->m_players[0];
    player.m_aiData.m_attentionWeights.turnCreatureAttention = 0.5f;
    player.m_aiData.m_fightValueResourceWeight = 1.0f;
    for (i32 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const OutcomeCase& c = cases[i];
        float winChance = 0.0f;
        i32 attackerLoss = 0;
        i32 defenderLoss = 0;
        i32 expectedAttackerLoss = 0;
        i32 expectedDefenderLoss = 0;
        i32 outcome = 0;
        player.m_difficulty = c.difficulty;
        gHumanPlayer[1] = c.enemyHuman;
        gPhilAI->ProbableOutcomeOfBattle(
            c.attacker,
            c.attackerHero,
            c.defender,
            c.defenderHero,
            NULL,
            c.useTown,
            3,
            c.enemyPlayer,
            winChance,
            attackerLoss,
            defenderLoss,
            expectedAttackerLoss,
            expectedDefenderLoss,
            outcome
        );
        Line(
            "outcome %s: win %.4f loss %d/%d expected %d/%d value %d",
            c.label,
            winChance,
            attackerLoss,
            defenderLoss,
            expectedAttackerLoss,
            expectedDefenderLoss,
            outcome
        );
    }
    gHumanPlayer[1] = false;
    player.m_difficulty = 0;
}

// --- pathing ----------------------------------------------------------------

static void SetCell(i32 x, i32 y, i32 terrain) {
    mapCell& cell = gGame->m_map[x][y];
    memset(&cell, 0, sizeof(mapCell));
    cell.m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN + (x + y) % 4;
    cell.m_objectIndex = MAP_CELL_NO_FRAME;
    cell.m_overlayIndex = MAP_CELL_NO_FRAME;
}

static void FillTerrain(i32 x0, i32 x1, i32 y0, i32 y1, i32 terrain) {
    for (i32 x = x0; x <= x1; x++)
        for (i32 y = y0; y <= y1; y++)
            SetCell(x, y, terrain);
}

static void Pathing(void) {
    // A grass field with a swamp patch, a desert strip, a lake and a wall
    // of blocked cells open at one gap; the target is a town trigger.
    FillTerrain(0, MAP_CELL_GRID_SIZE - 1, 0, MAP_CELL_GRID_SIZE - 1, TERRAIN_GRASS);
    FillTerrain(12, 18, 8, 16, TERRAIN_SWAMP);
    FillTerrain(9, 9, 0, 30, TERRAIN_DESERT);
    FillTerrain(24, 27, 4, 14, TERRAIN_WATER);
    for (i32 wall = 0; wall <= 30; wall++)
        if (wall != 22)
            gGame->m_map[21][wall].m_secondaryTrigger = MAP_CELL_SECONDARY_BLOCKED;
    // A rock: a non-shadow object blocks steps into it from the north.
    gGame->m_map[30][15].m_objectIndex = 3;
    gGame->m_map[30][15].m_objectTileset = 1;
    gGame->m_map[32][12].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN);
    gGame->m_map[5][60].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_MINE);
    gHumanPlayer[gCurPlayer] = false;

    struct PathCase {
        const char* label;
        i16 x;
        i16 y;
        i16 maximumCost;
        u8 trigger;
    };
    static const PathCase cases[] = {
        {"town from 4,12", 4, 12, SEARCH_NO_COST_LIMIT, MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)},
        {"town from 40,40", 40, 40, SEARCH_NO_COST_LIMIT, MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)},
        {"town from 4,12 within 60", 4, 12, 60, MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)},
        {"mine from 4,12", 4, 12, SEARCH_NO_COST_LIMIT, MAP_EVENT_TRIGGER(MAP_OBJECT_MINE)},
        {"town from the lake shore 23,9", 23, 9, SEARCH_NO_COST_LIMIT, MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)},
    };
    for (i32 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const PathCase& c = cases[i];
        i16 length = gSearchArray->FindNearestObject(c.x, c.y, MAP_DIRECTION_NORTH, c.maximumCost, c.trigger);
        char steps[SEARCH_PATH_CAPACITY + 1];
        // The path buffer runs from the target back to the start.
        for (i32 step = 0; step < length; step++)
            steps[step] = static_cast<char>('0' + gSearchArray->m_directions[length - 1 - step]);
        steps[length] = 0;
        i32 cost = -1;
        if (length > 0)
            cost = gSearchArray->m_cells[gSearchArray->m_specialTargetX][gSearchArray->m_specialTargetY].distance;
        Line(
            "path %s: length %d cost %d target %d,%d steps %s",
            c.label,
            length,
            cost,
            length > 0 ? gSearchArray->m_specialTargetX : -1,
            length > 0 ? gSearchArray->m_specialTargetY : -1,
            steps
        );
    }
    Line(
        "quick distance: %d %d %d",
        gSearchArray->QuickDistance(0, 0, 10, 0),
        gSearchArray->QuickDistance(0, 0, 10, 10),
        gSearchArray->QuickDistance(3, 50, 20, 41)
    );
}

static void Buildings(void) {
    // CanBuild over a fresh town of each type: what a castle allows first,
    // and what the first two dwellings unlock.
    town& t = gGame->m_castleRecs[5];
    for (i32 type = TOWN_TYPE_KNIGHT; type < TOWN_TYPE_COUNT; type++) {
        t.m_id = 5;
        t.m_type = type;
        t.m_x = 40;
        t.m_y = 40;
        t.m_buildState = 0;
        printf("town %d can build:", type);
        static const i16 sets[3] = {
            0,
            1 << BUILDING_SLOT_CASTLE,
            (1 << BUILDING_SLOT_CASTLE) | (1 << BUILDING_SLOT_DWELLING_FIRST)
                | (1 << (BUILDING_SLOT_DWELLING_FIRST + 1)) | (1 << BUILDING_SLOT_TAVERN)
        };
        for (i32 set = 0; set < 3; set++) {
            t.m_buildings = sets[set];
            printf(" ");
            for (i32 building = BUILDING_SLOT_FIRST; building < BUILDING_SLOT_COUNT; building++)
                printf("%d", CanBuild(&t, building) ? 1 : 0);
        }
        printf("\n");
    }
    t.m_buildings = 1 << BUILDING_SLOT_CASTLE;
    gGame->m_townBuiltToday[0] = 1 << 5;
    Line("town built today can build a tavern: %d", CanBuild(&t, BUILDING_SLOT_TAVERN) ? 1 : 0);
    gGame->m_townBuiltToday[0] = 0;
}

// --- records ----------------------------------------------------------------

// Bytes [skipFrom, skipTo) may differ.
static b32 SameFile(const char* a, const char* b, i32* size, i32 skipFrom = 0, i32 skipTo = 0) {
    FILE* fa = fopen(a, "rb");
    FILE* fb = fopen(b, "rb");
    b32 same = fa != NULL && fb != NULL;
    *size = 0;
    while (same) {
        i32 ca = fgetc(fa);
        i32 cb = fgetc(fb);
        if (ca != cb && (*size < skipFrom || *size >= skipTo))
            same = false;
        if (ca == EOF || cb == EOF)
            break;
        (*size)++;
    }
    if (fa)
        fclose(fa);
    if (fb)
        fclose(fb);
    return same;
}

static u32 FileDigest(const char* path) {
    FILE* f = fopen(path, "rb");
    u32 hash = 2166136261u;
    if (!f)
        return 0;
    i32 c;
    while ((c = fgetc(f)) != EOF) {
        u8 byte = static_cast<u8>(c);
        hash = Digest(&byte, 1, hash);
    }
    fclose(f);
    return hash;
}

static void FillSavedState(void) {
    FixtureBytes(gGame->m_mapName, sizeof(gGame->m_mapName));
    FixtureBytes(gGame->m_mapDescription, sizeof(gGame->m_mapDescription));
    FixtureBytes(gGame->m_players, sizeof(gGame->m_players));
    for (i32 x = 0; x < MAP_CELL_GRID_SIZE; x++)
        for (i32 y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            mapCell& cell = gGame->m_map[x][y];
            FixtureBytes(&cell, sizeof(mapCell));
            cell.m_tileIndex %= MAP_CELL_GROUND_TILE_COUNT;
            cell.m_triggerType = 0;
            cell.m_secondaryTrigger = 0;
        }
    FixtureBytes(gGame->m_castleRecs, sizeof(gGame->m_castleRecs));
    FixtureBytes(gGame->m_townOwners, sizeof(gGame->m_townOwners));
    FixtureBytes(gGame->m_heroRecs, sizeof(gGame->m_heroRecs));
    FixtureBytes(gGame->m_heroOwners, sizeof(gGame->m_heroOwners));
    FixtureBytes(gGame->m_mines, sizeof(gGame->m_mines));
    FixtureBytes(gGame->m_mineOwners, sizeof(gGame->m_mineOwners));
    FixtureBytes(gGame->m_randomArtifacts, sizeof(gGame->m_randomArtifacts));
    FixtureBytes(gGame->m_boats, sizeof(gGame->m_boats));
    FixtureBytes(gGame->m_boatSlots, sizeof(gGame->m_boatSlots));
    FixtureBytes(gGame->m_obeliskVisitors, sizeof(gGame->m_obeliskVisitors));
    FixtureBytes(gGame->m_mapSounds, sizeof(gGame->m_mapSounds));
    FixtureBytes(gGame->m_mapExtra, sizeof(gGame->m_mapExtra));
    FixtureBytes(gMapVisitFlags, sizeof(gMapVisitFlags));
    // SaveGame first demobilizes the current player's hero; none is moving.
    for (i32 player = 0; player < GAME_PLAYER_COUNT; player++)
        gGame->m_players[player].m_currentHero = HERO_ID_NONE;
    gGame->m_campaignType = 0;
    gGame->m_campaignScenario = 2;
    gGame->m_campaignDay = 17;
    gGame->m_campaignScenariosWon = 1;
    gGame->m_mapSize = 1;
    gGame->m_mapDifficulty = 2;
    gGame->m_difficulty = 1;
    gGame->m_playerCount = 3;
    gGame->m_deadPlayerCount = 1;
    gGame->m_playerDead[2] = true;
    gGame->m_day = 4;
    gGame->m_week = 2;
    gGame->m_month = 3;
    gGame->m_obeliskCount = 5;
    gGame->m_ultimateArtifactX = 33;
    gGame->m_ultimateArtifactY = 44;
    gGame->m_ultimateArtifactId = ARTIFACT_ULTIMATE_SWORD;
    gIAmGreatest = 0;
    gMonthType = 1;
    gMonthTypeExtra = 4;
    gWeekType = 2;
    gWeekTypeExtra = 7;
    gCurPlayer = 1;
    gHumanPlayer[0] = true;
    gHumanPlayer[1] = true;
    gHumanPlayer[2] = true;
    gNumHumanPlayers = 3;
    strcpy(gGame->m_saveName, "FIXTURE");
}

static void SaveRoundTrip(void) {
    _mkdir("save-a");
    _mkdir("save-b");
    gFixtureSeed = 1996;
    FillSavedState();
    gCurPlayerData = &gGame->m_players[gCurPlayer];
    gCurPlayerData->m_currentHero = HERO_ID_NONE;
    strcpy(gGamePath, "save-a\\");
    gGame->SaveGame("FIXTURE", true);
    // Overwrite everything the save holds, read it back and save again.
    gFixtureSeed = 2003;
    FillSavedState();
    gCurPlayer = 0;
    // Two living human players name the save .GM2.
    gGame->LoadGame("FIXTURE.GM2", false, false);
    i32 loadedPlayer = gCurPlayer;
    i32 loadedWatch = gCurWatchPlayer;
    strcpy(gGamePath, "save-b\\");
    gCurPlayerData->m_currentHero = HERO_ID_NONE;
    // Saved under the same base name: the record keeps the name it was given.
    gGame->SaveGame("FIXTURE", true);
    i32 size = 0;
    b32 same = SameFile("save-a\\FIXTURE.GM2", "save-b\\FIXTURE.GM2", &size);
    Line(
        "save record FIXTURE.GM2: %d bytes digest %08x, current player %d watch %d, reload and resave %s",
        size,
        FileDigest("save-a\\FIXTURE.GM2"),
        loadedPlayer,
        loadedWatch,
        same ? "identical" : "DIFFERENT"
    );
    gCurPlayer = 0;
    gCurPlayerData = &gGame->m_players[0];
    for (i32 player = 0; player < GAME_PLAYER_COUNT; player++)
        gHumanPlayer[player] = false;
}

static u32 MapStateDigest(void) {
    u32 hash = Digest(gGame->m_map, sizeof(gGame->m_map));
    hash = Digest(gGame->m_castleRecs, sizeof(gGame->m_castleRecs), hash);
    hash = Digest(gGame->m_mines, sizeof(gGame->m_mines), hash);
    hash = Digest(gGame->m_randomArtifacts, sizeof(gGame->m_randomArtifacts), hash);
    hash = Digest(&gGame->m_obeliskCount, sizeof(gGame->m_obeliskCount), hash);
    hash = Digest(gGame->m_mapSounds, sizeof(gGame->m_mapSounds), hash);
    for (i32 i = 1; i < gMaxMapExtra; i++)
        hash = Digest(gMapExtraBlocks[i], gMapExtraSizes[i], hash);
    return hash;
}

static void ResetMapState(void) {
    memset(gGame->m_map, 0, sizeof(gGame->m_map));
    memset(gGame->m_castleRecs, 0, sizeof(gGame->m_castleRecs));
    memset(gGame->m_mines, 0, sizeof(gGame->m_mines));
}

static void SummarizeMap(const char* label) {
    i32 towns = 0;
    i32 castles = 0;
    i32 mines = 0;
    for (i32 i = 0; i < GAME_TOWN_COUNT; i++) {
        if (gGame->m_castleRecs[i].m_buildings) {
            towns++;
            if (gGame->m_castleRecs[i].m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
                castles++;
        }
    }
    for (i32 mine = 0; mine < GAME_MINE_COUNT; mine++)
        if (gGame->m_mines[mine].x || gGame->m_mines[mine].y)
            mines++;
    Line(
        "map %s: towns %d castles %d mines %d obelisks %d extras %d state %08x",
        label,
        towns,
        castles,
        mines,
        gGame->m_obeliskCount,
        gMaxMapExtra - 1,
        MapStateDigest()
    );
}

// A version-1112 map written field by field in the order game::LoadMap reads.
static void MapRecord(void) {
    _mkdir("maps");
    gFixtureSeed = 1112;
    i32 file = open("maps\\FIXTURE.MAP", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    i16 version = MAP_EXTRA_VERSION;
    i16 width = MAP_CELL_GRID_SIZE;
    i16 height = MAP_CELL_GRID_SIZE;
    write(file, &version, sizeof(version));
    write(file, &width, sizeof(width));
    write(file, &height, sizeof(height));
    static mapCell cells[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    FixtureBytes(cells, sizeof(cells));
    write(file, cells, sizeof(cells));
    for (i32 i = 0; i < GAME_TOWN_COUNT; i++) {
        // Six towns: types 0..3 with bit 7 marking a castle; the rest unused.
        i8 record[3] = {-1, -1, 0};
        if (i < 6) {
            record[0] = static_cast<i8>(10 + i * 5);
            record[1] = static_cast<i8>(20 + i);
            record[2] = static_cast<i8>((i % 4) | (i & 1 ? 0x80 : 0));
        }
        write(file, record, sizeof(record));
    }
    for (i32 mine = 0; mine < GAME_MINE_COUNT; mine++) {
        i8 record[3] = {-1, -1, 0};
        if (mine < 4) {
            record[0] = static_cast<i8>(50 + mine);
            record[1] = static_cast<i8>(5 + mine * 3);
            record[2] = static_cast<i8>(mine);
        }
        write(file, record, sizeof(record));
    }
    i8 artifacts[ARTIFACT_REGULAR_END];
    FixtureBytes(artifacts, sizeof(artifacts));
    write(file, artifacts, sizeof(artifacts));
    i8 obelisks = 4;
    write(file, &obelisks, sizeof(obelisks));
    static i8 sounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    FixtureBytes(sounds, sizeof(sounds));
    write(file, sounds, sizeof(sounds));
    i32 extras = 3;
    write(file, &extras, sizeof(extras));
    for (i32 extra = 1; extra < extras; extra++) {
        char record[24];
        i32 size = 8 + extra * 8;
        FixtureBytes(record, size);
        write(file, &size, sizeof(size));
        write(file, record, size);
    }
    close(file);
    ResetMapState();
    strcpy(gMapPath, "maps\\");
    gGame->LoadMap("FIXTURE.MAP");
    i32 cellsSame = !memcmp(gGame->m_map, cells, sizeof(cells));
    i32 soundsSame = !memcmp(gGame->m_mapSounds, sounds, sizeof(sounds));
    i32 artifactsSame = !memcmp(gGame->m_randomArtifacts, artifacts, sizeof(artifacts));
    printf("map record: cells %s sounds %s artifacts %s; towns",
           cellsSame ? "read" : "DIFFER",
           soundsSame ? "read" : "DIFFER",
           artifactsSame ? "read" : "DIFFER");
    for (i32 slot = 0; slot < 6; slot++) {
        const town& t = gGame->m_castleRecs[slot];
        printf(" %d,%d type %d buildings 0x%04x", t.m_x, t.m_y, t.m_type, static_cast<u16>(t.m_buildings));
    }
    printf("; mines");
    for (i32 listed = 0; listed < 4; listed++)
        printf(" %d,%d type %d", gGame->m_mines[listed].x, gGame->m_mines[listed].y, gGame->m_mines[listed].type);
    printf("; extras %d sizes %d %d\n", gMaxMapExtra, gMapExtraSizes[1], gMapExtraSizes[2]);
    SummarizeMap("fixture");
    ClearMapExtra();
}

// --- the installed game's shipped files ---------------------------------------

// A save record's name field follows the header, campaign state, map
// description, size, difficulty and map name.
static const i32 SAVE_NAME_OFFSET = 1 + 2 + 4 + 16 + 0x2c + 0x79 + 2 + 0x11;

// Every save present (the network save in Data and the saves in Games) must
// read back and save again byte for byte; the saves themselves change as the
// game is played, so only that property is checked.
static void InstalledSaves(const char* root, const char* folder, const char* pattern) {
    char search[MAX_PATH];
    WIN32_FIND_DATAA found;
    sprintf(search, "%s\\%s\\%s", root, folder, pattern);
    HANDLE handle = FindFirstFileA(search, &found);
    if (handle == INVALID_HANDLE_VALUE)
        return;
    _mkdir("installed");
    do {
        char source[MAX_PATH];
        char copy[MAX_PATH];
        sprintf(source, "%s\\%s\\", root, folder);
        gHumanPlayer[0] = gHumanPlayer[1] = gHumanPlayer[2] = gHumanPlayer[3] = false;
        gNumHumanPlayers = GAME_PLAYER_COUNT;
        gRemoteOn = false;
        // REMOTE.GAM resolves against the data folder, other saves against
        // the game folder: point both at the folder that holds the file.
        strcpy(gDataPath, source);
        strcpy(gGamePath, source);
        gGame->LoadGame(found.cFileName, false, false);
        strcpy(gDataPath, "installed\\");
        strcpy(gGamePath, "installed\\");
        gAdvManager->m_heroContextLocked = false;
        gGame->SaveGame(found.cFileName, false);
        sprintf(source, "%s\\%s\\%s", root, folder, found.cFileName);
        sprintf(copy, "installed\\%s", found.cFileName);
        // SaveGame files the save's own name where the record kept the name
        // it was last saved under; every other byte must survive.
        i32 size = 0;
        b32 same = SameFile(source, copy, &size, SAVE_NAME_OFFSET, SAVE_NAME_OFFSET + 0x11);
        Line("save %s\\%s: %d bytes, reload and resave %s", folder, found.cFileName, size,
             same ? "identical outside the name" : "DIFFERENT");
    } while (FindNextFileA(handle, &found));
    FindClose(handle);
}

// ORIGDATA.BIN, the new-game template LoadGame reads with origData set.
static void InstalledOriginalData(const char* root) {
    sprintf(gDataPath, "%s\\Data\\", root);
    gHumanPlayer[0] = gHumanPlayer[1] = gHumanPlayer[2] = gHumanPlayer[3] = false;
    gNumHumanPlayers = 1;
    memset(gGame, 0, sizeof(game));
    gGame->LoadGame("ORIGDATA.BIN", true, false);
    u32 hash = Digest(gGame->m_players, sizeof(gGame->m_players));
    hash = Digest(gGame->m_map, sizeof(gGame->m_map), hash);
    hash = Digest(gGame->m_castleRecs, sizeof(gGame->m_castleRecs), hash);
    hash = Digest(gGame->m_heroRecs, sizeof(gGame->m_heroRecs), hash);
    hash = Digest(gGame->m_mines, sizeof(gGame->m_mines), hash);
    Line(
        "original data: players %d day %d week %d month %d current %d names %08x state %08x",
        gGame->m_playerCount,
        gGame->m_day,
        gGame->m_week,
        gGame->m_month,
        gCurPlayer,
        Digest(gGame->m_mapName, sizeof(gGame->m_mapName)),
        hash
    );
}

static void InstalledMaps(const char* root) {
    char search[MAX_PATH];
    WIN32_FIND_DATAA found;
    sprintf(search, "%s\\Maps\\*.*", root);
    HANDLE handle = FindFirstFileA(search, &found);
    if (handle == INVALID_HANDLE_VALUE)
        return;
    sprintf(gMapPath, "%s\\Maps\\", root);
    char names[128][MAX_PATH];
    i32 count = 0;
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && count < 128)
            strcpy(names[count++], found.cFileName);
    } while (FindNextFileA(handle, &found));
    FindClose(handle);
    // FindNextFile has no defined order; list the maps by name.
    for (i32 i = 0; i < count; i++)
        for (i32 j = i + 1; j < count; j++)
            if (stricmp(names[j], names[i]) < 0) {
                char swap[MAX_PATH];
                strcpy(swap, names[i]);
                strcpy(names[i], names[j]);
                strcpy(names[j], swap);
            }
    for (i32 map = 0; map < count; map++) {
        ResetMapState();
        gGame->LoadMap(names[map]);
        SummarizeMap(names[map]);
        ClearMapExtra();
    }
}

// An unhandled exception ends the program with a line and exit code 3,
// never a dialog.
static LONG WINAPI ReportCrash(EXCEPTION_POINTERS* exception) {
    printf(
        "unhandled exception %08lx at %p\n",
        exception->ExceptionRecord->ExceptionCode,
        exception->ExceptionRecord->ExceptionAddress
    );
    ExitProcess(3);
    return EXCEPTION_EXECUTE_HANDLER;
}

i32 __cdecl main(i32 argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(ReportCrash);
    // Unbuffered, so a crash report follows the last line that completed.
    setvbuf(stdout, NULL, _IONBF, 0);
    SetUpManagers();
    if (argc == 3 && !strcmp(argv[1], "--installed")) {
        InstalledSaves(argv[2], "Data", "REMOTE.GAM");
        InstalledSaves(argv[2], "Games", "*.GM?");
        InstalledOriginalData(argv[2]);
        InstalledMaps(argv[2]);
        return 0;
    }
    CreatureTable();
    TownTables();
    HeroTables();
    SpellAndArtifactTables();
    TerrainCosts();
    ExperienceAndMobility();
    LuckAndMorale();
    Damage();
    ArtificialIntelligence();
    Buildings();
    Pathing();
    SaveRoundTrip();
    MapRecord();
    return 0;
}
