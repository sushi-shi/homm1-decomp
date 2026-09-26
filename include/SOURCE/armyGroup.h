#ifndef HOMM1_SOURCE_ARMYGROUP_H
#define HOMM1_SOURCE_ARMYGROUP_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 13 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

// forward declarations:
class hero;
class town;

#pragma pack(push, 1)
class armyGroup {
public:
    // Retail constructor clears five signed type bytes, then five short counts.
    signed char m_creatureTypes[5];
    short m_creatureCounts[5];
    // --- constructors ---
    armyGroup(void);
    // --- methods ---
    void View(int);
    int HasAllUndead(void);
    int HasSomeUndead(void);
    int GetMorale(class hero *, class town *, class armyGroup *);
    void Dismiss(int);
    signed char IsMember(signed char);
    int IsHomogeneous(int);
    signed char CanJoin(signed char);
    int GetNumArmies(void);
    int Add(int, int, int);
    void Swap(int, class armyGroup *, int);
    void DamageGroup(float);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_ARMYGROUP_H
