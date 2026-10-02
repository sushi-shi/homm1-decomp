// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/WINMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/X_GLOBAL.h>

// donor PoL RVA 0x00032c00; preferred Buka symbol ??0town@@QAE@XZ
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.468183;margin=0.431232;shape=0.273;size=0.841;calls=1.000;alternate=pol20:void town::constructor(void)@0x00032c00
VA(0x00463f10, 0x6b)
town::town(void) {
    m_type = 0;
    m_threat = 0;
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    m_buildings = (1 << BUILDING_SLOT_TENT);
    m_buildState = 0;
    m_unknown19 = 0;
}

// donor PoL RVA 0x00032c65; preferred Buka symbol ?HasGarrison@town@@QAEHXZ
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410733;margin=0.365325;shape=0.175;size=0.741;calls=1.000;alternate=pol20:int town::HasGarrison(void)@0x00032c65
// HoMM1 retail returns in AL; the HoMM2 int return is a later signature.
VA(0x00463f7b, 0x55)
signed char town::HasGarrison(void) {
    for (short slot = 0; slot < ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (m_army.m_creatureTypes[slot] != CREATURE_NONE)
            return 1;
    }
    return 0;
}

// donor PoL RVA 0x00032cb9; preferred Buka symbol ?GiveSpells@town@@QAEXPAVhero@@@Z
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.430933;margin=0.601053;shape=0.167;size=0.987;calls=0.667;alternate=pol20:void town::GiveSpells(class hero *)@0x00032cb9
VA(0x00463fd0, 0xe1)
void town::GiveSpells(void) {
    hero* visitingHero;
    short i;

    if (m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE)
        return;
    visitingHero = gpGame->GetHero(m_occupyingHeroId);
    if (!visitingHero->HasArtifact(ARTIFACT_MAGIC_BOOK))
        return;
    if (!(m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD)))
        return;
    if (visitingHero->m_owner == m_owner) {
        for (i = 0; i < gMageGuildSpellCount[m_buildState]; i++)
            visitingHero->AddSpell(
                m_mageGuildSpells[i],
                visitingHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                0
            );
    }
}

VA(0x004640b1, 0x17c)
void town::XformToCastle(void) {
    short i;

    for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
        gpGame->m_map[m_x - TOWN_FOOTPRINT_LEFT + i][m_y - TOWN_FOOTPRINT_TOP].m_overlayIndex +=
            TOWN_CASTLE_FRAME_OFFSET;
        gpGame->m_map[m_x - TOWN_FOOTPRINT_LEFT + i][m_y - 1].m_objectIndex +=
            TOWN_CASTLE_FRAME_OFFSET;
        gpGame->m_map[m_x - TOWN_FOOTPRINT_LEFT + i][m_y].m_objectIndex += TOWN_CASTLE_FRAME_OFFSET;
    }
}

// donor PoL RVA 0x00032e74; preferred Buka symbol ?View@town@@QAEXH@Z
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.510810;margin=0.878787;shape=0.327;size=0.842;calls=1.000;alternate=pol20:void town::View(int)@0x00032e74
// HoMM1's callee returns with `ret` and always fades; the donor's noFade
// argument and memory-limit calculation belong to its later revision.
VA(0x0046422d, 0xa5)
void town::View(void) {
    if (giHighMemBuffer > TOWN_VIEW_HIGH_MEMORY_LIMIT)
        gAdvDisposeLevel = ADV_DISPOSE_FULL;
    else
        gAdvDisposeLevel = ADV_DISPOSE_PARTIAL;

    townManager* manager = gpTownManager;
    manager->SetTown(this);
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpExec->CallManager(gpTownManager);
    if (m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
        gpAdvManager->SetHeroContext(m_occupyingHeroId, 0);
    gAdvDisposeLevel = ADV_DISPOSE_NONE;
}

VA(0x004642d2, 0x152)
void town::Deallocate(void) {
    playerData* ownerData;
    short i;
    signed char found;

    ownerData = &gpGame->m_players[m_owner];
    found = -1;
    for (i = 0; i < ownerData->m_townCount; i++) {
        if (ownerData->m_townIds[i] == m_id)
            found = i;
    }
    for (i = found; i < ownerData->m_townCount - 1; i++)
        ownerData->m_townIds[i] = ownerData->m_townIds[i + 1];
    ownerData->m_townIds[ownerData->m_townCount - 1] = GAME_TOWN_NONE;
    if (ownerData->m_currentTown == m_id)
        ownerData->m_currentTown = GAME_TOWN_NONE;
    ownerData->m_townCount--;
    if (ownerData->m_townCount < 5)
        ownerData->m_townLocatorPage = 0;
    else if (ownerData->m_townLocatorPage + 5 > ownerData->m_townCount)
        ownerData->m_townLocatorPage = ownerData->m_townCount - 5;
    gpGame->m_townOwners[m_id] = GAME_PLAYER_NONE;
    m_owner = GAME_PLAYER_NONE;
}
