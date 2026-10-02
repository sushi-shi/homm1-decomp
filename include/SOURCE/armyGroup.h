#ifndef HOMM1_SOURCE_ARMYGROUP_H
#define HOMM1_SOURCE_ARMYGROUP_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 13 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/creatureTypes.h>

// forward declarations:
class hero;
class town;

// clang-format off
H1_ENUM_CONST_BEGIN(ArmyGroupConstant)
    ARMY_GROUP_SLOT_COUNT = 5,
    // armyGroup::Add slot argument: merge into a matching stack or the first
    // empty slot.
    ARMY_GROUP_ANY_SLOT = -1
H1_ENUM_CONST_END(ArmyGroupConstant)
// clang-format on

#pragma pack(push, 1)
class armyGroup {
public:
    // Retail constructor clears five signed type bytes, then five short counts.
    H1_ENUM_STORAGE(CreatureType, signed char) m_creatureTypes[5];
    short m_creatureCounts[5];
    // --- constructors ---
    armyGroup(void);
    // --- methods ---
    void View(int);
    int HasAllUndead(void);
    int HasSomeUndead(void);
    // HoMM1 retail: hero and town only (ret 8), morale in AX.
    short GetMorale(class hero*, class town*);
    void Dismiss(signed char);
    signed char IsMember(signed char);
    signed char IsHomogeneous(signed char);
    signed char CanJoin(signed char);
    // HoMM1 returns the count in AX (callers sign-extend).
    short GetNumArmies(void);
    // HoMM1 retail: byte creature/slot, word count, word result (ret 0xc).
    short Add(signed char, short, signed char);
    void Swap(signed char, class armyGroup*, signed char);
    void DamageGroup(float);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_ARMYGROUP_H
