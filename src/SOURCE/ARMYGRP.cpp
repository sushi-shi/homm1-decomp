// Army-group morale, membership, composition and damage.

#include <match.h>

#include <BASE/Misc.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/town.h>

#include <string.h>

VA(0x004184b0, 0x31)
armyGroup::armyGroup(void) {
    CLEAR_ARMY_GROUP(*this);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x004184e1, 0xd)
void armyGroup::View(i32) {}

// HoMM1 adds the town's building bit 4 and clamps to -3..3 in AX.
VA(0x004184ee, 0x11b)
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
        if (armyHero->HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE))
            morale -= 2;
    }
    if (occupiedTown && (occupiedTown->m_buildings & (1 << BUILDING_SLOT_TAVERN)))
        morale++;
    if (morale < ARMY_GROUP_MORALE_MIN)
        morale = ARMY_GROUP_MORALE_MIN;
    else if (morale > ARMY_GROUP_MORALE_MAX)
        morale = ARMY_GROUP_MORALE_MAX;
    return morale;
}

VA(0x00418609, 0x26)
void armyGroup::Dismiss(i8 slot) {
    m_creatureTypes[slot] = CREATURE_NONE;
    m_creatureCounts[slot] = 0;
}

// HoMM1 retail reads a signed byte parameter and returns in AL.
VA(0x0041862f, 0x47)
i8 armyGroup::IsMember(i8 creatureType) {
    for (i16 slot = 0; slot < ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (m_creatureTypes[slot] == creatureType)
            return 1;
    }
    return 0;
}

// Races are six consecutive creature ids.
VA(0x00418676, 0x13e)
H1_ENUM_RETURN(ArmyGroupAlignmentResult, i8) armyGroup::IsHomogeneous(i8 alignmentMode) {
    i32 numTypeRuns = 0;
    i8 raceUsed[ARMY_GROUP_RACE_COUNT];
    raceUsed[0] = raceUsed[1] = raceUsed[2] = raceUsed[3] = raceUsed[4] = 0;
    i32 prevType = -1;
    i32 numRaces;
    i16 i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE) {
            if (alignmentMode == ARMY_GROUP_EMPTY_SLOT)
                ++raceUsed[m_creatureTypes[i] / CREATURE_FACTION_SIZE];
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

// HoMM1 retail returns in AL and sign-extends its IsMember call results.
VA(0x004187b4, 0x3b)
i8 armyGroup::CanJoin(i8 creatureType) {
    if (IsMember(creatureType))
        return 1;
    if (IsMember(CREATURE_NONE))
        return 1;
    return 0;
}

VA(0x004187ef, 0x52)
i16 armyGroup::GetNumArmies(void) {
    i16 numArmies = 0;
    for (i16 i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE)
            ++numArmies;
    }
    return numArmies;
}

VA(0x00418841, 0xfe)
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

VA(0x0041893f, 0x77)
void armyGroup::Swap(i8 slot, armyGroup* otherGroup, i8 otherSlot) {
    i32 temporary = m_creatureTypes[slot];
    m_creatureTypes[slot] = otherGroup->m_creatureTypes[otherSlot];
    otherGroup->m_creatureTypes[otherSlot] = temporary;

    temporary = m_creatureCounts[slot];
    m_creatureCounts[slot] = otherGroup->m_creatureCounts[otherSlot];
    otherGroup->m_creatureCounts[otherSlot] = temporary;
}

VA(0x004189b6, 0x133)
void armyGroup::DamageGroup(float casualtyFraction) {
    i32 killed;
    i32 killChance = casualtyFraction * 100.0f;
    i32 i;
    i32 isFirstTroop = 1;
    i32 j;

    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE) {
            killed = 0;
            for (j = 0; j < m_creatureCounts[i]; ++j) {
                if (SRandom(0, 100) < killChance)
                    ++killed;
            }
            if (isFirstTroop && killed == m_creatureCounts[i] && casualtyFraction < 0.999)
                --killed;
            m_creatureCounts[i] -= killed;
            if (m_creatureCounts[i] <= 0 || casualtyFraction >= 1.0) {
                m_creatureCounts[i] = 0;
                m_creatureTypes[i] = CREATURE_NONE;
            }
            isFirstTroop = 0;
        } else {
            m_creatureCounts[i] = 0;
        }
    }
}
