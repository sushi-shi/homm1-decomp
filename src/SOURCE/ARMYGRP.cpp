#include <H1/Ints.h>

#include <BASE/Misc.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/town.h>

#include <string.h>

armyGroup::armyGroup(void) {
    CLEAR_ARMY_GROUP(*this);
}

void armyGroup::View(i32) {}

i16 armyGroup::GetMorale(hero* armyHero, town* occupiedTown) {
    i32 morale;
    i32 alignment;

    morale = 0;
    alignment = IsHomogeneous(ARMY_GROUP_EMPTY_SLOT);
    morale += alignment;
    if (armyHero) {
        if (!armyHero->m_heroClass)
            morale++;
        morale += armyHero->m_morale;
        if (armyHero->HasArtifact(ARTIFACT_MEDAL_OF_VALOR))
            morale++;
        if (armyHero->HasArtifact(ARTIFACT_MEDAL_OF_COURAGE))
            morale++;
        if (armyHero->HasArtifact(ARTIFACT_MEDAL_OF_HONOR))
            morale++;
        if (armyHero->HasArtifact(ARTIFACT_MEDAL_OF_DISTINCTION))
            morale++;
        morale += armyHero->m_cowardice;
    }
    if (occupiedTown
        && (occupiedTown->m_buildings & (1 << BUILDING_SLOT_TAVERN)))
        morale++;
    if (morale < ARMY_GROUP_MORALE_MIN)
        morale = ARMY_GROUP_MORALE_MIN;
    else if (morale > ARMY_GROUP_MORALE_MAX)
        morale = ARMY_GROUP_MORALE_MAX;
    return morale;
}

void armyGroup::Dismiss(i8 slot) {
    m_creatureTypes[slot] = CREATURE_NONE;
    m_creatureCounts[slot] = 0;
}

i8 armyGroup::IsMember(i8 creatureType) {
    for (i16 slot = 0; slot < ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (m_creatureTypes[slot] == creatureType)
            return 1;
    }
    return 0;
}

i8 armyGroup::IsHomogeneous(i8 alignmentMode) {
    i32 numTypeRuns = 0;
    i8 raceUsed[ARMY_GROUP_RACE_COUNT];
    raceUsed[0] = raceUsed[1] = raceUsed[2] = raceUsed[3] = raceUsed[4] = 0;
    i32 prevType = CREATURE_NONE;
    i32 numRaces;
    i16 i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE) {
            if (alignmentMode == ARMY_GROUP_EMPTY_SLOT)
                ++raceUsed[CREATURE_FACTION(m_creatureTypes[i])];
            if (m_creatureTypes[i] != prevType) {
                ++numTypeRuns;
                prevType = m_creatureTypes[i];
            }
        }
    }

    if (numTypeRuns <= 1)
        return ARMY_GROUP_ALIGNMENT_NO_MODIFIER;

    numRaces = 0;
    for (i = 0; i < ARMY_GROUP_RACE_COUNT; ++i) {
        if (raceUsed[i])
            ++numRaces;
    }

    if (numRaces == 1)
        return ARMY_GROUP_ALIGNMENT_SAME;
    if (numRaces == ARMY_GROUP_RACES_THREE)
        return ARMY_GROUP_ALIGNMENT_THREE;
    if (numRaces == ARMY_GROUP_RACES_FOUR)
        return ARMY_GROUP_ALIGNMENT_FOUR;
    if (numRaces == ARMY_GROUP_RACES_FIVE)
        return ARMY_GROUP_ALIGNMENT_FIVE_OR_MORE;
    return ARMY_GROUP_ALIGNMENT_NO_MODIFIER;
}

i8 armyGroup::CanJoin(i8 creatureType) {
    if (IsMember(creatureType))
        return 1;
    if (IsMember(CREATURE_NONE))
        return 1;
    return 0;
}

i16 armyGroup::GetNumArmies(void) {
    i16 numArmies = 0;
    for (i16 i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE)
            ++numArmies;
    }
    return numArmies;
}

i16 armyGroup::Add(i8 creatureType, i16 quantity, i8 slot) {
    i16 searchSlot;
    if (slot == ARMY_GROUP_EMPTY_SLOT) {
        for (searchSlot = 0; searchSlot < ARMY_GROUP_SLOT_COUNT; ++searchSlot) {
            if (m_creatureTypes[searchSlot] == creatureType) {
                slot = searchSlot;
                break;
            }
        }
    }
    if (slot == ARMY_GROUP_EMPTY_SLOT) {
        for (searchSlot = 0; searchSlot < ARMY_GROUP_SLOT_COUNT; ++searchSlot) {
            if (m_creatureTypes[searchSlot] == CREATURE_NONE
                || m_creatureTypes[searchSlot] == creatureType) {
                slot = searchSlot;
                break;
            }
        }
    }
    if (slot >= ARMY_GROUP_SLOT_COUNT)
        return 0;

    m_creatureTypes[slot] = creatureType;
    if (m_creatureCounts[slot] < 0)
        m_creatureCounts[slot] = 0;
    m_creatureCounts[slot] += quantity;
    return 1;
}

void armyGroup::Swap(i8 slot, armyGroup* otherGroup, i8 otherSlot) {
    i32 temporary(m_creatureTypes[slot]);
    m_creatureTypes[slot] = otherGroup->m_creatureTypes[otherSlot];
    otherGroup->m_creatureTypes[otherSlot] = temporary;

    temporary = m_creatureCounts[slot];
    m_creatureCounts[slot] = otherGroup->m_creatureCounts[otherSlot];
    otherGroup->m_creatureCounts[otherSlot] = temporary;
}

void armyGroup::DamageGroup(float casualtyFraction) {
    i32 killed;
    i32 killChance = casualtyFraction * 100.0f;
    i32 i;
    b32 isFirstTroop = true;
    i32 j;

    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE) {
            killed = 0;
            for (j = 0; j < m_creatureCounts[i]; ++j) {
                if (SRandom(0, 99) < killChance)
                    ++killed;
            }
            if (isFirstTroop && killed > 0 && killed == m_creatureCounts[i])
                --killed;
            m_creatureCounts[i] -= killed;
            if (m_creatureCounts[i] <= 0 || casualtyFraction >= 1.0) {
                m_creatureCounts[i] = 0;
                m_creatureTypes[i] = CREATURE_NONE;
            }
            isFirstTroop = false;
        } else {
            m_creatureCounts[i] = 0;
        }
    }
}
