#ifndef HOMM1_SOURCE_TOWN_H
#define HOMM1_SOURCE_TOWN_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/armyGroup.h>

// forward declarations:
class hero;

// clang-format off
H1_ENUM_CONST_BEGIN(TownConstant)
    TOWN_MAGE_GUILD_SPELL_COUNT = 9
H1_ENUM_CONST_END(TownConstant)
// clang-format on

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

// Spells taught per mage-guild level (retail 0x492514).
extern signed char gMageGuildSpellCount[];
#endif // HOMM1_SOURCE_TOWN_H
