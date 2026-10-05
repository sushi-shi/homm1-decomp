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

// m_mageGuildSpells slots by guild level: three first-level spells, two each
// of the second and third, then the fourth and fifth levels.
H1_ENUM_CONST_BEGIN(MageGuildSpellSlot)
    MAGE_GUILD_SLOT_LEVEL_1_FIRST = 0,
    MAGE_GUILD_SLOT_LEVEL_1_SECOND = 1,
    MAGE_GUILD_SLOT_LEVEL_1_THIRD = 2,
    MAGE_GUILD_SLOT_LEVEL_2_FIRST = 3,
    MAGE_GUILD_SLOT_LEVEL_2_SECOND = 4,
    MAGE_GUILD_SLOT_LEVEL_3_FIRST = 5,
    MAGE_GUILD_SLOT_LEVEL_3_SECOND = 6
H1_ENUM_CONST_END(MageGuildSpellSlot)

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
    i8 m_id;
    i8 m_owner;
    i8 m_threat;
    i8 m_type;
    // XformToCastle sign-extends the map coordinates.
    i8 m_x;
    i8 m_y;
    armyGroup m_army;
    i8 m_occupyingHeroId;
    i16 m_buildings;
    i8 m_buildState;
    i8 m_unknown19;
    i16 m_garrison[6];
    // ProcessMapExtra files the cell's map-extra index here; SetupTowns
    // marks towns whose extra record carries a custom setup.
    u8 m_extraIndex;
    i8 m_customized;
    char m_unknown28[4];
    i8 m_mageGuildSpells[TOWN_MAGE_GUILD_SPELL_COUNT];
    // ClaimTown sets two turns for a town taken from no owner, else zero.
    // GetBestBHC logs and compares it zero-extended.
    u16 m_turnsOwned;
    // --- constructors ---
    town(void);
    // --- methods ---
    i8 HasGarrison(void);
    // Win95 1.2 uses this inline accessor; Buka Open reads the member directly.
    i8 OccupyingHero(void) {
        return m_occupyingHeroId;
    }
    // HoMM1 retail 0x00463fd0 takes no argument (plain ret).
    void GiveSpells(void);
    void XformToCastle(void);
    void View(void);
    void Deallocate(void);
    void BuildBuilding(i32 building);
    i32 CanBuildDock(void);
    void CalcNumLevelArchers(i32* numArchers, i32* mageGuildLevel);
};
#pragma pack(pop)

// Town building ids: the order of retail gBuildingNames (0x004931c0), then
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

// Building slot is built in town t, the mage guild only at its last level
// (Buka 2.1 town.h): mask first, then the guild level.
#define TOWN_BUILDING_COMPLETE(t, slot)                                                            \
    (((t).m_buildings & (1 << (slot)))                                                             \
     && ((slot) != BUILDING_SLOT_MAGE_GUILD || (t).m_buildState == MAGE_GUILD_STATE_LEVEL_4))

#endif // HOMM1_SOURCE_TOWN_H
