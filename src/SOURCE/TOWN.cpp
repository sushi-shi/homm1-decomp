#include <match.h>

#include <BASE/executive.h>
#include <BASE/heroWindowManager.h>
#include <SOURCE/advManager.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/playerData.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>

#include <stdlib.h>

VA(0x0045e9c0, 0x60)
town::town(void) {
    m_type = 0;
    m_nameIndex = 0;
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    m_buildings = (1 << BUILDING_SLOT_TENT);
    m_buildState = 0;
    m_unknown19 = 0;
}

VA(0x0045ea20, 0x43)
i8 town::HasGarrison(void) {
    for (i16 slot = 0; slot < ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (m_army.m_creatureTypes[slot] != CREATURE_NONE)
            return 1;
    }
    return 0;
}

VA(0x0045ea63, 0xb9)
void town::GiveSpells(void) {
    hero* visitingHero;
    i16 i;

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

VA(0x0045eb1c, 0x168)
void town::XformToCastle(void) {
    i16 i;

    for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
        gpGame->m_map[m_x - TOWN_FOOTPRINT_LEFT + i][m_y - TOWN_FOOTPRINT_TOP].m_overlayIndex +=
            TOWN_CASTLE_FRAME_OFFSET;
        gpGame->m_map[m_x - TOWN_FOOTPRINT_LEFT + i][m_y - 1].m_objectIndex +=
            TOWN_CASTLE_FRAME_OFFSET;
        gpGame->m_map[m_x - TOWN_FOOTPRINT_LEFT + i][m_y].m_objectIndex += TOWN_CASTLE_FRAME_OFFSET;
    }
}

// The callee returns with `ret` and always fades.
VA(0x0045ec84, 0x8c)
void town::View(void) {
    if (gHighMemBuffer > TOWN_VIEW_HIGH_MEMORY_LIMIT)
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

VA(0x0045ed10, 0x13d)
void town::Deallocate(void) {
    playerData* ownerData;
    i16 i;
    i8 foundIndex;

    ownerData = &gpGame->m_players[m_owner];
    foundIndex = -1;
    for (i = 0; i < ownerData->m_townCount; i++) {
        if (ownerData->m_townIds[i] == m_id)
            foundIndex = i;
    }
    for (i = foundIndex; i < ownerData->m_townCount - 1; i++)
        ownerData->m_townIds[i] = ownerData->m_townIds[i + 1];
    ownerData->m_townIds[ownerData->m_townCount - 1] = GAME_TOWN_NONE;
    if (ownerData->m_currentTown == m_id)
        ownerData->m_currentTown = GAME_TOWN_NONE;
    ownerData->m_townCount--;
    if (ownerData->m_townCount < LOCATOR_PAGE_THRESHOLD)
        ownerData->m_townLocatorPage = 0;
    else if (ownerData->m_townLocatorPage + LOCATOR_PAGE_THRESHOLD > ownerData->m_townCount)
        ownerData->m_townLocatorPage = ownerData->m_townCount - LOCATOR_PAGE_THRESHOLD;
    gpGame->m_townOwners[m_id] = GAME_PLAYER_NONE;
    m_owner = GAME_PLAYER_NONE;
}
