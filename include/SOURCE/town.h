#ifndef HOMM1_SOURCE_TOWN_H
#define HOMM1_SOURCE_TOWN_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

// forward declarations:
class hero;

class town {
public:
    // --- constructors ---
    town(void);
    // --- methods ---
    int HasGarrison(void);
    void GiveSpells(class hero *);
    void XformToCastle(void);
    void View(int);
    void Deallocate(void);
    void BuildBuilding(int);
    int CanBuildDock(void);
    void CalcNumLevelArchers(int *, int *);
};
#endif // HOMM1_SOURCE_TOWN_H
