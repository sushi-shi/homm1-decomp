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
// Buka ArmyGroupConstant: Add's "any slot" argument, the five troop slots,
// IsHomogeneous' race table (creature / CREATURE_FACTION_SIZE: four town
// races and the neutrals) and GetMorale's clamp.
H1_ENUM_CONST_BEGIN(ArmyGroupConstant)
    ARMY_GROUP_EMPTY_SLOT = -1,
    ARMY_GROUP_SLOT_COUNT = 5,
    ARMY_GROUP_RACE_COUNT = 5,
    ARMY_GROUP_MORALE_MIN = -3,
    ARMY_GROUP_MORALE_MAX = 3
H1_ENUM_CONST_END(ArmyGroupConstant)

// IsHomogeneous' morale modifier by the number of races in the group (Buka
// ArmyGroupAlignmentResult, same numbering); two races give no modifier.
H1_ENUM_BEGIN(ArmyGroupAlignmentResult)
    ARMY_GROUP_ALIGNMENT_FIVE_OR_MORE = -3,
    ARMY_GROUP_ALIGNMENT_FOUR = -2,
    ARMY_GROUP_ALIGNMENT_THREE = -1,
    ARMY_GROUP_ALIGNMENT_NO_MODIFIER = 0,
    ARMY_GROUP_ALIGNMENT_SAME = 1
H1_ENUM_END(ArmyGroupAlignmentResult)
// clang-format on

#pragma pack(push, 1)
class armyGroup {
public:
    // Retail constructor clears five signed type bytes, then five short counts.
    H1_ENUM_STORAGE(CreatureType, signed char) m_creatureTypes[ARMY_GROUP_SLOT_COUNT];
    short m_creatureCounts[ARMY_GROUP_SLOT_COUNT];
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
    H1_ENUM_RETURN(ArmyGroupAlignmentResult, signed char) IsHomogeneous(signed char);
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
