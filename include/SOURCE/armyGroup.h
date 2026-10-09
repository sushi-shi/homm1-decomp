#ifndef HOMM1_SOURCE_ARMYGROUP_H
#define HOMM1_SOURCE_ARMYGROUP_H

#include <SOURCE/creatureTypes.h>

class hero;
class town;

enum ArmyGroupConstant {
    ARMY_GROUP_EMPTY_SLOT = -1,
    ARMY_GROUP_SLOT_COUNT = 5,
    ARMY_GROUP_RACE_COUNT = 5,
    ARMY_GROUP_MORALE_MIN = -3,
    ARMY_GROUP_MORALE_MAX = 3
};

enum ArmyGroupAlignmentResult {
    ARMY_GROUP_ALIGNMENT_FIVE_OR_MORE = -3,
    ARMY_GROUP_ALIGNMENT_FOUR = -2,
    ARMY_GROUP_ALIGNMENT_THREE = -1,
    ARMY_GROUP_ALIGNMENT_NO_MODIFIER = 0,
    ARMY_GROUP_ALIGNMENT_NO_BONUS_LAST = 0,
    ARMY_GROUP_ALIGNMENT_SAME = 1
};

#define CLEAR_ARMY_GROUP(group)                                                                    \
    (memset(                                                                                       \
         (group).m_creatureTypes,                                                                  \
         CREATURE_NONE,                                              \
         sizeof((group).m_creatureTypes)                                                           \
     ),                                                                                            \
     memset((group).m_creatureCounts, 0, sizeof((group).m_creatureCounts)))

class armyGroup {
public:
    i8 m_creatureTypes[ARMY_GROUP_SLOT_COUNT];
    i16 m_creatureCounts[ARMY_GROUP_SLOT_COUNT];
    armyGroup(void);
    void View(i32);
    i16 GetMorale(class hero* armyHero, class town* occupiedTown);
    void Dismiss(i8 slot);
    b8 IsMember(i8 creatureType);
    i8 IsHomogeneous(i8 alignmentMode);
    b8 CanJoin(i8 creatureType);
    i16 GetNumArmies(void);
    i16 Add(i8 creatureType, i16 quantity, i8 slot);
    void Swap(i8 slot, class armyGroup* otherGroup, i8 otherSlot);
    void DamageGroup(float casualtyFraction);
};
enum ArmyGroupRaceCount {
    ARMY_GROUP_RACES_THREE = 3,
    ARMY_GROUP_RACES_FOUR = 4,
    ARMY_GROUP_RACES_FIVE = 5
};

#endif
