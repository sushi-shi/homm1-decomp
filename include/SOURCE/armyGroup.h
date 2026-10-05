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
// Results above NO_BONUS_LAST are the one-race bonus (KB's morale help).
H1_ENUM_BEGIN(ArmyGroupAlignmentResult)
    ARMY_GROUP_ALIGNMENT_FIVE_OR_MORE = -3,
    ARMY_GROUP_ALIGNMENT_FOUR = -2,
    ARMY_GROUP_ALIGNMENT_THREE = -1,
    ARMY_GROUP_ALIGNMENT_NO_MODIFIER = 0,
    ARMY_GROUP_ALIGNMENT_NO_BONUS_LAST = 0,
    ARMY_GROUP_ALIGNMENT_SAME = 1
H1_ENUM_END(ArmyGroupAlignmentResult)

// Empty every slot of an army group (Buka 2.1 armyGroup.h).
#define CLEAR_ARMY_GROUP(group)                                                                    \
    (memset((group).m_creatureTypes, CREATURE_NONE, sizeof((group).m_creatureTypes)),              \
     memset((group).m_creatureCounts, 0, sizeof((group).m_creatureCounts)))

#pragma pack(push, 1)
class armyGroup {
public:
    // Retail constructor clears five signed type bytes, then five short counts.
    H1_ENUM_STORAGE(CreatureType, i8) m_creatureTypes[ARMY_GROUP_SLOT_COUNT];
    i16 m_creatureCounts[ARMY_GROUP_SLOT_COUNT];
    // --- constructors ---
    armyGroup(void);
    // --- methods ---
    void View(i32);
    i32 HasAllUndead(void);
    i32 HasSomeUndead(void);
    // HoMM1 retail: hero and town only (ret 8), morale in AX.
    i16 GetMorale(class hero* h, class town* t);
    void Dismiss(i8 slot);
    i8 IsMember(i8 creatureType);
    H1_ENUM_RETURN(ArmyGroupAlignmentResult, i8) IsHomogeneous(i8 countRaces);
    i8 CanJoin(i8 creatureType);
    // HoMM1 returns the count in AX (callers sign-extend).
    i16 GetNumArmies(void);
    // HoMM1 retail: byte creature/slot, word count, word result (ret 0xc).
    i16 Add(i8 creatureType, i16 quantity, i8 slot);
    void Swap(i8 slot, class armyGroup* otherGroup, i8 otherSlot);
    void DamageGroup(float damagePercent);
};
#pragma pack(pop)
// GetAlignmentModifier's distinct-race counts with their own modifier.
H1_ENUM_CONST_BEGIN(ArmyGroupRaceCount)
    ARMY_GROUP_RACES_THREE = 3,
    ARMY_GROUP_RACES_FOUR = 4,
    ARMY_GROUP_RACES_FIVE = 5
H1_ENUM_CONST_END(ArmyGroupRaceCount)

#endif // HOMM1_SOURCE_ARMYGROUP_H
