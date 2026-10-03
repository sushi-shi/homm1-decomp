#ifndef HOMM1_SOURCE_TOWN_H
#define HOMM1_SOURCE_TOWN_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/armyGroup.h>

// forward declarations:
class hero;

H1_ENUM_CONST_BEGIN(TownConstant)
    TOWN_MAGE_GUILD_SPELL_COUNT = 9,
    // town::m_occupyingHeroId when no hero stands in the town (Buka's name).
    TOWN_OCCUPYING_HERO_NONE = -1
H1_ENUM_CONST_END(TownConstant)

// town::m_type: the faction whose dwellings the town builds. Retail
// gDwellingType rows (0..5 knight, 12..17 sorceress, 6..11 barbarian, 18..23
// warlock creatures) and GiveTroopsToNeutralTowns' recruits fix the order.
H1_ENUM_BEGIN(TownType)
    // gRandomTownTypes' entry before RandomizeTown picks the race.
    TOWN_TYPE_NONE = -1,
    TOWN_TYPE_KNIGHT = 0,
    TOWN_TYPE_SORCERESS = 1,
    TOWN_TYPE_BARBARIAN = 2,
    TOWN_TYPE_WARLOCK = 3,
    TOWN_TYPE_COUNT = 4
H1_ENUM_END(TownType)

// town::m_buildState is the mage guild's level - 1 (STATE_LEVEL_1..4); its
// nine m_mageGuildSpells slots fill three, two, two and two per level
// (SetupTown's pools, SetupMageGuild's locks, MageGuildHandler's bounds).
H1_ENUM_CONST_BEGIN(TownMageGuildConstant)
    MAGE_GUILD_STATE_LEVEL_1 = 0,
    MAGE_GUILD_STATE_LEVEL_2 = 1,
    MAGE_GUILD_STATE_LEVEL_3 = 2,
    MAGE_GUILD_STATE_LEVEL_4 = 3,
    MAGE_GUILD_LEVEL_1_LAST_SLOT = 2,
    MAGE_GUILD_LEVEL_2_LAST_SLOT = 4,
    MAGE_GUILD_LEVEL_3_LAST_SLOT = 6,
    // SetupMageGuild's spell frame: 0 shows the spell, 1 the locked slot.
    MAGE_GUILD_SPELL_FRAME_SHOWN = 0,
    MAGE_GUILD_SPELL_FRAME_LOCKED = 1
H1_ENUM_CONST_END(TownMageGuildConstant)

// A town object covers 4x3 map cells from (x - 2, y - 2) to (x + 1, y)
// (RandomizeTown, NewMap). Its frames run per race in blocks of 24 before
// the random town's (block TOWN_TYPE_COUNT); a town without a castle uses
// the frames 12 before the castle's. RandomizeTown ages a placed town ten
// turns (Buka RANDOM_TOWN_AGE).
H1_ENUM_CONST_BEGIN(TownFootprintConstant)
    TOWN_FOOTPRINT_LEFT = 2,
    TOWN_FOOTPRINT_TOP = 2,
    TOWN_FOOTPRINT_WIDTH = 4,
    TOWN_FOOTPRINT_HEIGHT = 3,
    TOWN_RACE_FRAME_STRIDE = 24,
    TOWN_CASTLE_FRAME_OFFSET = 12,
    TOWN_RANDOM_AGE = 10
H1_ENUM_CONST_END(TownFootprintConstant)

// town::View lets the resource manager release all adventure art
// (ADV_DISPOSE_FULL) only above this much high memory, else part of it.
H1_ENUM_CONST_BEGIN(TownViewConstant)
    TOWN_VIEW_HIGH_MEMORY_LIMIT = 200
H1_ENUM_CONST_END(TownViewConstant)

#pragma pack(push, 1)
         class town {
public:
    // Retail constructor and HasGarrison establish this packed prefix.
    signed char m_id;
    signed char m_owner;
    signed char m_threat;
    signed char m_type;
    // XformToCastle sign-extends the map coordinates.
    signed char m_x;
    signed char m_y;
    armyGroup m_army;
    signed char m_occupyingHeroId;
    short m_buildings;
    signed char m_buildState;
    char m_unknown19;
    short m_garrison[6];
    // ProcessMapExtra files the cell's map-extra index here; SetupTowns
    // marks towns whose extra record carries a custom setup.
    unsigned char m_extraIndex;
    signed char m_customized;
    char m_unknown28[4];
    signed char m_mageGuildSpells[TOWN_MAGE_GUILD_SPELL_COUNT];
    // ClaimTown sets two turns for a town taken from no owner, else zero.
    // GetBestBHC logs and compares it zero-extended.
    unsigned short m_turnsOwned;
    // --- constructors ---
    town(void);
    // --- methods ---
    signed char HasGarrison(void);
    // Buka town::OccupyingHero inline; townManager::Open emits its jmp $+0.
    signed char OccupyingHero(void) {
        return m_occupyingHeroId;
    }
    // HoMM1 retail 0x00463fd0 takes no argument (plain ret).
    void GiveSpells(void);
    void XformToCastle(void);
    void View(void);
    void Deallocate(void);
    void BuildBuilding(int);
    int CanBuildDock(void);
    void CalcNumLevelArchers(int*, int*);
};
#pragma pack(pop)


// Town building ids: the order of retail gBuildingNames (0x004933a8), then
// six dwellings named per race by gDwellingNames. town::m_buildings holds
// bit 1 << id. CanBuild confirms the roles: 6 needs no castle, 3 needs water
// at the dock cell, 5 is never built and 0 has mage-guild levels.
H1_ENUM_BEGIN(BuildingSlotType)
    BUILDING_SLOT_MAGE_GUILD = 0,
    BUILDING_SLOT_THIEVES_GUILD = 1,
    BUILDING_SLOT_TAVERN = 2,
    BUILDING_SLOT_SHIPYARD = 3,
    BUILDING_SLOT_WELL = 4,
    // The generic structures every town type shares (philAI's castle arrow
    // count adds one per built slot up to here).
    BUILDING_SLOT_GENERIC_LAST = 4,
    // Slots RACE_FIRST.. use per-race build-window frames, the generic ones
    // before them frame building + 1 (TOWNMGR SetupBuildWindow).
    BUILDING_SLOT_RACE_FIRST = 5,
    BUILDING_SLOT_TENT = 5,
    BUILDING_SLOT_CASTLE = 6,
    // The non-dwelling structures end here (TOWNMGR building <= 6 tests).
    BUILDING_SLOT_STRUCTURE_LAST = 6,
    BUILDING_SLOT_DWELLING_FIRST = 7,
    BUILDING_SLOT_DWELLING_1 = 7,
    BUILDING_SLOT_DWELLING_2 = 8,
    BUILDING_SLOT_DWELLING_3 = 9,
    BUILDING_SLOT_DWELLING_4 = 10,
    BUILDING_SLOT_DWELLING_5 = 11,
    BUILDING_SLOT_DWELLING_6 = 12,
    BUILDING_SLOT_DWELLING_LAST = 12,
    // gDwellingRequirements masks name only slots before the sixth dwelling
    // (nothing requires it); BuyBuild lists the prerequisites below this.
    BUILDING_SLOT_REQUIREMENT_END = 12,
    // Dwellings per town: gDwellingNames/gDwellingRequirements rows are
    // m_type * DWELLING_COUNT + dwelling (TOWNMGR).
    BUILDING_SLOT_DWELLING_COUNT = 6,
    BUILDING_SLOT_COUNT = 13,
    // Past the buildable slots: the race special building's bit (bit 13, as
    // in Buka's TOWN_BUILDING_COLISEUM/FORTIFICATIONS 0x2000, whose tent and
    // castle bits 5 and 6 match HoMM1's). LoadMap, NewMap and RandomizeTown
    // give it to barbarian towns only; no HoMM1 reader tests it.
    BUILDING_SLOT_SPECIAL = 13
H1_ENUM_END(BuildingSlotType)

#endif // HOMM1_SOURCE_TOWN_H
