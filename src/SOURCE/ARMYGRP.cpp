// armyGroup methods. Buka 2.1 ARMYGRP correspondence: the same methods in
// the same order. Retail begins this object at 0x00447920 on a 16-byte
// boundary after int3 fill, so it is not part of the packed GAME object.

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/town.h>

#include <string.h>

// donor PoL RVA 0x0008c040; preferred Buka symbol ??0armyGroup@@QAE@XZ
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.513410;margin=0.267257;shape=0.385;size=0.817;calls=1.000;alternate=pol20:void armyGroup::constructor(void)@0x0008c040
VA(0x00447920, 0x3c)
armyGroup::armyGroup(void) {
    memset(m_creatureTypes, -1, sizeof(m_creatureTypes));
    memset(m_creatureCounts, 0, sizeof(m_creatureCounts));
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0044795c, 0x18)
void armyGroup::View(int) {}

// HoMM1 adds the town's building bit 4 and clamps to -3..3 in AX.
VA(0x00447974, 0x11b)
short armyGroup::GetMorale(hero* h, town* t) {
    int morale;
    int alignment;

    morale = 0;
    alignment = IsHomogeneous(ARMY_GROUP_EMPTY_SLOT);
    morale += alignment;
    if (h) {
        if (!h->m_heroClass)
            morale++;
        morale += h->m_morale;
        if (h->HasArtifact(ARTIFACT_MEDAL_OF_VALOR))
            morale++;
        if (h->HasArtifact(ARTIFACT_MEDAL_OF_COURAGE))
            morale++;
        if (h->HasArtifact(ARTIFACT_MEDAL_OF_HONOR))
            morale++;
        if (h->HasArtifact(ARTIFACT_MEDAL_OF_DISTINCTION))
            morale++;
        if (h->HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE))
            morale -= 2;
    }
    if (t && (t->m_buildings & (1 << BUILDING_SLOT_TAVERN)))
        morale++;
    if (morale < ARMY_GROUP_MORALE_MIN)
        morale = ARMY_GROUP_MORALE_MIN;
    else if (morale > ARMY_GROUP_MORALE_MAX)
        morale = ARMY_GROUP_MORALE_MAX;
    return morale;
}

VA(0x00447a8f, 0x31)
void armyGroup::Dismiss(signed char slot) {
    m_creatureTypes[slot] = CREATURE_NONE;
    m_creatureCounts[slot] = 0;
}

// donor PoL RVA 0x0008c3f6; preferred Buka symbol ?IsMember@armyGroup@@QAEHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.401440;margin=0.383163;shape=0.171;size=0.719;calls=1.000;alternate=pol20:int armyGroup::IsMember(int)@0x0008c3f6
// HoMM1 retail reads a signed byte parameter and returns in AL.
VA(0x00447ac0, 0x59)
signed char armyGroup::IsMember(signed char creatureType) {
    for (short slot = 0; slot < ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (m_creatureTypes[slot] == creatureType)
            return 1;
    }
    return 0;
}

// Buka 2.1 IsHomogeneous; HoMM1 races are six consecutive creature ids.
VA(0x00447b19, 0x153)
H1_ENUM_RETURN(ArmyGroupAlignmentResult, signed char) armyGroup::IsHomogeneous(signed char countRaces) {
    int numTypes = 0;
    signed char raceSeen[ARMY_GROUP_RACE_COUNT];
    raceSeen[0] = raceSeen[1] = raceSeen[2] = raceSeen[3] = raceSeen[4] = 0;
    int previous = -1;
    int numRaces;
    short i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE) {
            if (countRaces == ARMY_GROUP_EMPTY_SLOT)
                ++raceSeen[m_creatureTypes[i] / CREATURE_FACTION_SIZE];
            if (m_creatureTypes[i] != previous) {
                ++numTypes;
                previous = m_creatureTypes[i];
            }
        }
    }

    if (numTypes <= 1)
        return ARMY_GROUP_ALIGNMENT_NO_MODIFIER;

    numRaces = 0;
    for (i = 0; i < ARMY_GROUP_RACE_COUNT; ++i) {
        if (raceSeen[i])
            ++numRaces;
    }

    if (numRaces == 1)
        return ARMY_GROUP_ALIGNMENT_SAME;
    if (numRaces == 3)
        return ARMY_GROUP_ALIGNMENT_THREE;
    if (numRaces == 4)
        return ARMY_GROUP_ALIGNMENT_FOUR;
    if (numRaces == 5)
        return ARMY_GROUP_ALIGNMENT_FIVE_OR_MORE;
    return ARMY_GROUP_ALIGNMENT_NO_MODIFIER;
}

// donor PoL RVA 0x0008c599; preferred Buka symbol ?CanJoin@armyGroup@@QAEHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.455116;margin=0.419277;shape=0.310;size=0.702;calls=1.000;alternate=pol20:int armyGroup::CanJoin(int)@0x0008c599
// HoMM1 retail returns in AL and sign-extends its IsMember call results.
VA(0x00447c6c, 0x54)
signed char armyGroup::CanJoin(signed char creatureType) {
    if (IsMember(creatureType))
        return 1;
    if (IsMember(CREATURE_NONE))
        return 1;
    return 0;
}

VA(0x00447cc0, 0x59)
short armyGroup::GetNumArmies(void) {
    short numArmies = 0;
    for (short i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE)
            ++numArmies;
    }
    return numArmies;
}

// donor PoL RVA 0x0008c641; preferred Buka symbol ?Add@armyGroup@@QAEHHHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418523;margin=0.356193;shape=0.176;size=0.729;calls=1.000;alternate=pol20:int armyGroup::Add(int, int, int)@0x0008c641
VA(0x00447d19, 0x132)
short armyGroup::Add(signed char creatureType, short quantity, signed char slot) {
    short searchSlot;
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

VA(0x00447e4b, 0x7d)
void armyGroup::Swap(signed char slot, armyGroup* otherGroup, signed char otherSlot) {
    int temporary = m_creatureTypes[slot];
    m_creatureTypes[slot] = otherGroup->m_creatureTypes[otherSlot];
    otherGroup->m_creatureTypes[otherSlot] = temporary;

    temporary = m_creatureCounts[slot];
    m_creatureCounts[slot] = otherGroup->m_creatureCounts[otherSlot];
    otherGroup->m_creatureCounts[otherSlot] = temporary;
}

// donor PoL RVA 0x0008c7d2; preferred Buka symbol ?DamageGroup@armyGroup@@QAEXM@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.521804;margin=0.531953;shape=0.341;size=0.892;calls=1.000;alternate=pol20:void armyGroup::DamageGroup(float)@0x0008c7d2
VA(0x00447ec8, 0x14d)
void armyGroup::DamageGroup(float damagePercent) {
    int killed;
    int chance = static_cast<int>(damagePercent * 100.0f);
    int isFirstTroop = 1;
    int i;
    int j;

    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_creatureTypes[i] != CREATURE_NONE) {
            killed = 0;
            for (j = 0; j < m_creatureCounts[i]; ++j) {
                if (SRandom(0, 100) < chance)
                    ++killed;
            }
            if (isFirstTroop && m_creatureCounts[i] == killed && damagePercent < 0.999)
                --killed;
            m_creatureCounts[i] -= killed;
            if (m_creatureCounts[i] <= 0 || damagePercent >= 1.0) {
                m_creatureCounts[i] = 0;
                m_creatureTypes[i] = CREATURE_NONE;
            }
            isFirstTroop = 0;
        } else {
            m_creatureCounts[i] = 0;
        }
    }
}
