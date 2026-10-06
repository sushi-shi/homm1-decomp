#ifndef HOMM1_SOURCE_TOWN_H
#define HOMM1_SOURCE_TOWN_H

#include <SOURCE/armyGroup.h>
#include <SOURCE/spellTypes.h>

class hero;

enum TownConstant {
    TOWN_MAGE_GUILD_SPELL_COUNT = 9,
    BUILDING_SLOT_DWELLING_COUNT = 6,
    TOWN_OCCUPYING_HERO_NONE = -1
};

enum MageGuildSpellSlot {
    MAGE_GUILD_SLOT_LEVEL_1_FIRST = 0,
    MAGE_GUILD_SLOT_LEVEL_1_SECOND = 1,
    MAGE_GUILD_SLOT_LEVEL_1_THIRD = 2,
    MAGE_GUILD_SLOT_LEVEL_2_FIRST = 3,
    MAGE_GUILD_SLOT_LEVEL_2_SECOND = 4,
    MAGE_GUILD_SLOT_LEVEL_3_FIRST = 5,
    MAGE_GUILD_SLOT_LEVEL_3_SECOND = 6
};

enum TownType {
    TOWN_TYPE_NONE = -1,
    TOWN_TYPE_KNIGHT = 0,
    TOWN_TYPE_SORCERESS = 1,
    TOWN_TYPE_BARBARIAN = 2,
    TOWN_TYPE_WARLOCK = 3,
    TOWN_TYPE_COUNT = 4
};

enum TownMageGuildConstant {
    MAGE_GUILD_STATE_LEVEL_1 = 0,
    MAGE_GUILD_STATE_LEVEL_2 = 1,
    MAGE_GUILD_STATE_LEVEL_3 = 2,
    MAGE_GUILD_STATE_LEVEL_4 = 3,
    MAGE_GUILD_LEVEL_1_LAST_SLOT = 2,
    MAGE_GUILD_LEVEL_2_LAST_SLOT = 4,
    MAGE_GUILD_LEVEL_3_LAST_SLOT = 6,
    MAGE_GUILD_SPELL_FRAME_SHOWN = 0,
    MAGE_GUILD_SPELL_FRAME_LOCKED = 1
};

enum TownFootprintConstant {
    TOWN_FOOTPRINT_LEFT = 2,
    TOWN_FOOTPRINT_TOP = 2,
    TOWN_FOOTPRINT_WIDTH = 4,
    TOWN_FOOTPRINT_HEIGHT = 3,
    TOWN_RACE_FRAME_STRIDE = 24,
    TOWN_CASTLE_FRAME_OFFSET = 12,
    TOWN_RANDOM_AGE = 10
};

enum TownViewConstant {
    TOWN_VIEW_HIGH_MEMORY_LIMIT = 200
};

#pragma pack(push, 1)
class town {
public:
    i8 m_id;
    i8 m_owner;

    i8 m_nameIndex;
    i8 m_type;
    i8 m_x;
    i8 m_y;
    armyGroup m_army;
    i8 m_occupyingHeroId;
    i16 m_buildings;
    i8 m_buildState;
    i8 m_unused19;
    i16 m_dwellingAvailable[6];
    u8 m_extraIndex;
    b8 m_customized;
    char m_unused28[4];
    i8 m_mageGuildSpells[TOWN_MAGE_GUILD_SPELL_COUNT];
    u16 m_turnsOwned;
    town(void);
    i8 HasGarrison(void);
    i8 OccupyingHero(void) {
        return m_occupyingHeroId;
    }
    void GiveSpells(void);
    void XformToCastle(void);
    void View(void);
    void Deallocate(void);
};
#pragma pack(pop)

enum BuildingSlotType {
    BUILDING_SLOT_NONE = -1,
    BUILDING_SLOT_FIRST = 0,
    BUILDING_SLOT_MAGE_GUILD = 0,
    BUILDING_SLOT_THIEVES_GUILD = 1,
    BUILDING_SLOT_TAVERN = 2,
    BUILDING_SLOT_SHIPYARD = 3,
    BUILDING_SLOT_WELL = 4,
    BUILDING_SLOT_GENERIC_LAST = 4,
    BUILDING_SLOT_RACE_FIRST = 5,
    BUILDING_SLOT_TENT = 5,
    BUILDING_SLOT_CASTLE = 6,
    BUILDING_SLOT_STRUCTURE_LAST = 6,
    BUILDING_SLOT_NEUTRAL_COUNT = 7,
    BUILDING_SLOT_DWELLING_FIRST = 7,
    BUILDING_SLOT_DWELLING_1 = 7,
    BUILDING_SLOT_DWELLING_2 = 8,
    BUILDING_SLOT_DWELLING_3 = 9,
    BUILDING_SLOT_DWELLING_4 = 10,
    BUILDING_SLOT_DWELLING_5 = 11,
    BUILDING_SLOT_DWELLING_6 = 12,
    BUILDING_SLOT_DWELLING_LAST = 12,
    BUILDING_SLOT_REQUIREMENT_END = 12,
    BUILDING_SLOT_COUNT = 13,
    BUILDING_SLOT_SPECIAL = 13,
    BUILDING_SLOT_CAPACITY = 16
};

#define TOWN_BUILDING_COMPLETE(t, slot)                                                            \
    (((t).m_buildings & (1 << slot))                                       \
     && ((slot) != BUILDING_SLOT_MAGE_GUILD || (t).m_buildState == MAGE_GUILD_STATE_LEVEL_4))

#endif
