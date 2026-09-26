#ifndef HOMM1_SOURCE_TOWN_H
#define HOMM1_SOURCE_TOWN_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>
#include <SOURCE/armyGroup.h>

// forward declarations:
class hero;

#pragma pack(push, 1)
class town {
public:
    // Retail constructor and HasGarrison establish this packed prefix.
    signed char m_id;
    signed char m_owner;
    signed char m_threat;
    signed char m_type;
    unsigned char m_x;
    unsigned char m_y;
    armyGroup m_army;
    signed char m_occupyingHeroId;
    short m_buildings;
    signed char m_buildState;
    char m_unknown19;
    short m_garrison[6];
    // --- constructors ---
    town(void);
    // --- methods ---
    signed char HasGarrison(void);
    void GiveSpells(class hero *);
    void XformToCastle(void);
    void View(void);
    void Deallocate(void);
    void BuildBuilding(int);
    int CanBuildDock(void);
    void CalcNumLevelArchers(int *, int *);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_TOWN_H
