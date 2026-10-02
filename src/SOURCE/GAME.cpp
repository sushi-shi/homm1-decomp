// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/Misc.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/TILE.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/FINDPATH.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// donor PoL RVA 0x00088607; preferred Buka symbol ?ClearEffects@combatManager@@QAEXXZ
// donor Buka TU SOURCE/SPELLAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.425101;margin=0.364782;shape=0.143;size=0.828;calls=1.000;alternate=pol20:void combatManager::ClearEffects(void)@0x00088607
VA(0x00437977, 0x63)
void combatManager::ClearEffects(void) {
    int side;
    int index;
    for (side = 0; side < COMBAT_EFFECT_SIDE_COUNT; ++side) {
        for (index = 0; index < COMBAT_EFFECT_SLOT_COUNT; ++index)
            gArmyEffected[side][index] = 0;
    }
}

// Buka 2.1 NextPos with HoMM1's retail-backed nine-hex row width.
VA(0x004379da, 0x40)
void combatManager::NextPos(int* hex) {
    if ((*hex + COMBAT_SPELL_AI_ROW_END_OFFSET) % COMBAT_SPELL_AI_ROW_LENGTH == 0)
        *hex += COMBAT_SPELL_AI_ROW_SKIP;
    else
        (*hex)++;
}

// donor PoL RVA 0x0000bd60; preferred Buka symbol ?ViewGeneral@combatManager@@QAEHHHH@Z
// donor Buka TU SOURCE/VIEW; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.677383;margin=0.274305;shape=0.458;size=0.778;calls=0.853;strings=port%04d.icn|vgenwin.bin;alternate=pol20:int combatManager::ViewGeneral(int, int, int)@0x0000bd60
VA(0x00438310, 0x56d)
int combatManager::ViewGeneral(int, int, int) {
    return 0;
}

// donor PoL RVA 0x0000c784; preferred Buka symbol ?ViewArmy@combatManager@@QAEXPAVarmy@@H@Z
// donor Buka TU SOURCE/VIEW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.474703;margin=0.690322;shape=0.250;size=0.915;calls=1.000;alternate=pol20:void combatManager::ViewArmy(class army *, int)@0x0000c784
VA(0x00438a9f, 0x161)
void combatManager::ViewArmy(class army*, int) {}

// donor PoL RVA 0x000708b0; preferred Buka symbol ?Write@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.582104;margin=0.473963;shape=0.500;size=0.883;calls=0.885;alternate=pol20:void playerData::Write(int)@0x000708b0
VA(0x00438c00, 0x1f8)
void playerData::Write(int file) {
    char unused[52];

    write(file, m_unknown00, sizeof(m_unknown00));
    write(file, &m_unknown11, 1);
    write(file, &m_difficulty, 1);
    write(file, &m_heroCount, 1);
    write(file, &m_currentHero, 1);
    write(file, &m_heroLocatorPage, 1);
    write(file, m_heroIds, sizeof(m_heroIds));
    write(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    memset(unused, 0, 50);
    write(file, unused, 50);
    write(file, &m_unknown52, 1);
    write(file, &m_unknown53, 1);
    write(file, &m_unknown54, 1);
    write(file, &m_unknown55, 1);
    write(file, &m_townCount, 1);
    write(file, &m_currentTown, 1);
    write(file, &m_townLocatorPage, 1);
    write(file, m_townIds, sizeof(m_townIds));
    write(file, m_resources, sizeof(m_resources));
    write(file, m_aiData.m_income, sizeof(m_aiData.m_income));
    write(file, &m_unknown99[1], 1);
    write(file, &m_unknown99[1], 1);
    write(file, m_obelisksVisited, sizeof(m_obelisksVisited));
}

// donor PoL RVA 0x00070aed; preferred Buka symbol ?Read@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.555449;margin=0.750197;shape=0.444;size=0.878;calls=0.880;alternate=pol20:void playerData::Read(int)@0x00070aed
VA(0x00438df8, 0x1e8)
void playerData::Read(int file) {
    char unused[52];

    read(file, m_unknown00, sizeof(m_unknown00));
    read(file, &m_unknown11, 1);
    read(file, &m_difficulty, 1);
    read(file, &m_heroCount, 1);
    read(file, &m_currentHero, 1);
    read(file, &m_heroLocatorPage, 1);
    read(file, m_heroIds, sizeof(m_heroIds));
    read(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    read(file, unused, 50);
    read(file, &m_unknown52, 1);
    read(file, &m_unknown53, 1);
    read(file, &m_unknown54, 1);
    read(file, &m_unknown55, 1);
    read(file, &m_townCount, 1);
    read(file, &m_currentTown, 1);
    read(file, &m_townLocatorPage, 1);
    read(file, m_townIds, sizeof(m_townIds));
    read(file, m_resources, sizeof(m_resources));
    read(file, m_aiData.m_income, sizeof(m_aiData.m_income));
    read(file, &m_unknown99[1], 1);
    read(file, &m_unknown99[1], 1);
    read(file, m_obelisksVisited, sizeof(m_obelisksVisited));
}

// donor PoL RVA 0x00070d1a; preferred Buka symbol ?NextHero@playerData@@QAEHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.473378;margin=0.450383;shape=0.230;size=0.850;calls=1.000;alternate=pol20:int playerData::NextHero(int)@0x00070d1a
VA(0x00438fe0, 0x12c)
signed char playerData::NextHero(int) {
    int curHero = -1;
    int i;

    if (gpCurPlayer->m_currentHero != -1) {
        for (i = 0; i < gpCurPlayer->m_heroCount; ++i) {
            if (gpCurPlayer->m_heroIds[i] == gpCurPlayer->m_currentHero)
                curHero = i;
        }
    }

    for (i = curHero + 1; i < gpCurPlayer->m_heroCount; ++i) {
        if (gpGame->IsMobile(gpCurPlayer->m_heroIds[i]))
            return m_heroIds[i];
    }
    for (i = 0; i < curHero + 1; ++i) {
        if (gpGame->IsMobile(gpCurPlayer->m_heroIds[i]))
            return m_heroIds[i];
    }
    return -1;
}

// Buka 2.1 playerData::HasMobileHero.
VA(0x0043910c, 0x68)
signed char playerData::HasMobileHero(void) {
    for (short i = 0; i < m_heroCount; ++i) {
        if (gpGame->IsMobile(m_heroIds[i]))
            return 1;
    }
    return 0;
}

// HoMM1 counts this player's visited-obelisk bits.
VA(0x00439174, 0x5f)
signed char playerData::CountVisitedObelisks(void) {
    signed char count = 0;
    for (short i = 0; i < 48; ++i) {
        if (BitTest(m_obelisksVisited, i))
            ++count;
    }
    return count;
}

// Buka 2.1 playerData::BuildingsOwned; slot 0 is the mage guild.
VA(0x004391d3, 0xd1)
int playerData::BuildingsOwned(int townType, int buildingIndex, int buildState) {
    int count = 0;
    int i;
    for (i = 0; i < m_townCount; ++i) {
        town* ownedTown = &gpGame->m_castleRecs[m_townIds[i]];
        if (buildingIndex < 7 || ownedTown->m_type == townType) {
            if (buildingIndex == 0) {
                if (ownedTown->m_buildings & 1) {
                    if (ownedTown->m_buildState == buildState)
                        ++count;
                }
            } else {
                if (ownedTown->m_buildings & (1 << buildingIndex))
                    ++count;
            }
        }
    }
    return count;
}

// Buka 2.1 game::IsMobile.
VA(0x00439873, 0xb3)
signed char game::IsMobile(signed char heroId) {
    if (heroId == -1)
        return 0;
    hero* mobileHero = &m_heroRecs[heroId];
    int terrain = giGroundToTerrain[gpAdvManager->GetCell(mobileHero->m_x, mobileHero->m_y)->m_tileIndex];
    return mobileHero->m_remainingMobility >= CalcTerrainCost(
               terrain,
               mobileHero->m_direction & 1,
               mobileHero->m_remainingMobility,
               mobileHero->m_unknown1c
           );
}

// Buka 2.1 game::GetWorldMapData.
VA(0x00439926, 0x1e)
mapCell (*game::GetWorldMapData(void))[MAP_CELL_GRID_SIZE] {
    return m_map;
}

// Buka 2.1 game::CreateBoat without the network map-change notice.
VA(0x00439944, 0xd9)
signed char game::CreateBoat(signed char x, signed char y) {
    signed char boatIdx = Scan(m_boatSlots, 0, GAME_BOAT_COUNT);
    if (boatIdx != -1) {
        m_boatSlots[boatIdx] = boatIdx;
        boatRecord* boat = &m_boats[boatIdx];
        boat->id = boatIdx;
        boat->x = x;
        boat->y = y;
        boat->direction = 2;
        boat->owner = giCurPlayer;
        mapCell* square = &m_map[x][y];
        boat->savedTriggerType = square->m_triggerType;
        boat->savedEventData = square->m_objectMetadata;
        square->m_triggerType = 0xbe;
        square->m_objectMetadata = boatIdx;
    }
    return boatIdx;
}

// Buka 2.1 game::Scan.
VA(0x00439a1d, 0x5f)
signed char game::Scan(signed char* array, signed char start, signed char length) {
    signed char i;
    for (i = start; i < start + length; ++i) {
        if (array[i] == -1)
            return i;
    }
    return -1;
}

// Buka 2.1 game::RandomScan; HoMM1 always looks for a free (-1) entry.
VA(0x00439a7c, 0x74)
signed char game::RandomScan(signed char* array, signed char start, signed char range, int) {
    signed char index = -1;
    int i;
    for (i = 0; i < 10000; ++i) {
        index = start + Random(0, range - 1);
        if (array[index] == -1)
            return index;
    }
    return -1;
}

// HoMM1 nine heroes per class; a 0x40 entry is the fallback pick.
VA(0x00439af0, 0x10e)
signed char game::GetNewHeroId(signed char heroClass) {
    signed char freeSlot = -1;
    signed char id = -1;
    short first = heroClass * 9;
    int i;
    freeSlot = Scan(m_availableHeroes, first, 9);
    if (freeSlot != -1) {
        id = RandomScan(m_availableHeroes, first, 9, 9);
    } else {
        freeSlot = Scan(m_availableHeroes, 0, GAME_HERO_COUNT);
        if (freeSlot != -1) {
            id = RandomScan(m_availableHeroes, 0, GAME_HERO_COUNT, GAME_HERO_COUNT);
        } else {
            for (i = 0; i < GAME_HERO_COUNT; ++i) {
                if (m_availableHeroes[i] == 0x40)
                    id = i;
            }
        }
    }
    if (id != -1)
        return id;
    else
        return 0;
}

// Buka 2.1 game::GetTownId.
VA(0x00439bfe, 0x8f)
signed char game::GetTownId(signed char x, signed char y) {
    for (short i = 0; i < GAME_TOWN_COUNT; ++i) {
        if (m_castleRecs[i].m_x == x && m_castleRecs[i].m_y == y)
            return i;
    }
    return -1;
}

// Buka 2.1 game::GetMineId.
VA(0x00439c8d, 0x87)
signed char game::GetMineId(signed char x, signed char y) {
    for (short i = 0; i < GAME_MINE_COUNT; ++i) {
        if (m_mines[i].x == x && m_mines[i].y == y)
            return i;
    }
    return -1;
}

// donor PoL RVA 0x00071d89; preferred Buka symbol ?GenerateStandardFileName@@YIXPAD0@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.427111;margin=0.052277;shape=0.267;size=0.694;calls=1.000;alternate=pol20:void GenerateStandardFileName(char *, char *)@0x00071d89
VA(0x00439d14, 0x129)
void GenerateStandardFileName(char *source, char *destination) {
    char *extension;
    int indexOut;
    int idx;
    char character;
    int size;

    extension = FindLastToken(source, '.');
    if (!extension) {
        strcpy(destination, source);
        return;
    }
    *extension = 0;
    indexOut = 0;
    size = strlen(source);
    for (idx = 0; idx < size; idx++) {
        character = source[idx];
        if (character >= 'a' && character <= 'z')
            character = character - ('a' - 'A');
        if ((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9')
            || character == '_') {
            destination[indexOut] = character;
            indexOut++;
        }
        if (indexOut >= 8)
            idx = 999;
    }
    *extension = '.';
    strcpy(destination + indexOut, extension);
}

// donor PoL RVA 0x00071eb7; preferred Buka symbol ?SaveGame@game@@QAEHPADHC@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.606443;margin=0.348788;shape=0.410;size=0.678;calls=0.698;strings=%s%s|%s.%s|%s.GM%d;alternate=pol20:int game::SaveGame(char *, int, signed char)@0x00071eb7
VA(0x00439e3d, 0x7b2)
short game::SaveGame(char *, signed char) { return 0; }

// donor PoL RVA 0x000735bf; preferred Buka symbol ?LoadGame@game@@QAEXPADHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.668603;margin=0.422052;shape=0.401;size=0.926;calls=0.741;strings=%s%s|.\DATA\|.\GAMES\;alternate=pol20:void game::LoadGame(char *, int, int)@0x000735bf
VA(0x0043a5ef, 0x9b2)
void game::LoadGame(char*, int, int) {}

// donor PoL RVA 0x000b88d6; preferred Buka symbol ?UpdateNewGameWindow@game@@QAEXXZ
// donor Buka TU SOURCE/Newgame; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.454910;margin=0.298327;shape=0.215;size=0.546;calls=0.480;strings=%s %d%%;alternate=pol20:void game::UpdateNewGameWindow(void)@0x000b88d6
VA(0x0043b522, 0x2c3)
void game::UpdateNewGameWindow(void) {}

// donor PoL RVA 0x000bc00e; preferred Buka symbol ?ShowInfo@ExpCampaign@@QAEXHH@Z
// donor Buka TU SOURCE/X_CAMPGN; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.710255;margin=0.146523;shape=0.500;size=0.813;calls=0.958;strings=advmice.mse;alternate=pol20:void ExpCampaign::ShowInfo(int, int)@0x000bc00e
// HoMM1 identity: advManager::ControlPanel calls it on gpGame with three
// arguments and the callee returns with `ret 0xc` (Buka game::ShowCampaignInfo).
VA(0x0043be93, 0x2ad)
void game::ShowCampaignInfo(int, int, int) {}

// donor PoL RVA 0x000bb843; preferred Buka symbol ?InitMap@ExpCampaign@@QAEXXZ
// donor Buka TU SOURCE/X_CAMPGN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.361177;margin=0.269979;shape=0.160;size=0.387;calls=0.167;strings=origdata.bin;alternate=pol20:void ExpCampaign::InitMap(void)@0x000bb843
VA(0x0043c1bf, 0x28c)
void ExpCampaign::InitMap(void) {}

// donor PoL RVA 0x00078b72; preferred Buka symbol ?LoadMap@game@@QAEHPAD@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.657346;margin=0.109543;shape=0.244;size=0.995;calls=1.000;strings=%s%s|.\MAPS\;alternate=pol20:int game::LoadMap(char *)@0x00078b72
VA(0x0043e30a, 0x43a)
int game::LoadMap(char*) {
    return 0;
}

// Vision radius a claimed town grants its new owner.
extern signed char giVisRangeTown;

// donor PoL RVA 0x00078fea; preferred Buka symbol ?ClaimTown@game@@QAEXHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.415111;margin=0.758393;shape=0.164;size=0.968;calls=0.500;alternate=pol20:void game::ClaimTown(int, int, int)@0x00078fea
VA(0x0043e744, 0x321)
void game::ClaimTown(signed char townId, signed char player) {
    int i;
    town* townRec;
    mapCell* cell;

    townRec = &m_castleRecs[townId];
    if (townRec->m_owner == player)
        return;
    if (m_townOwners[townId] != -1)
        gpGame->GetTown(townId)->Deallocate();
    for (i = 0; i < 5; ++i) {
        townRec->m_army.m_creatureTypes[i] = -1;
        townRec->m_army.m_creatureCounts[i] = 0;
    }
    if (m_castleRecs[townId].m_owner == -1)
        m_castleRecs[townId].m_turnsOwned = 2;
    else
        m_castleRecs[townId].m_turnsOwned = 0;
    m_castleRecs[townId].m_owner = player;
    m_townOwners[townId] = player;
    m_players[player].m_townIds[m_players[player].m_townCount] = townId;
    m_players[player].m_townCount++;

    cell = &m_map[m_castleRecs[townId].m_x - 1][m_castleRecs[townId].m_y];
    cell->m_flags |= 0x10;
    cell->m_objectTileset |= 0xe0;
    cell->m_unknown05 = m_players[player].Color() * 2;
    cell = &m_map[m_castleRecs[townId].m_x + 1][m_castleRecs[townId].m_y];
    cell->m_flags |= 0x10;
    cell->m_objectTileset |= 0xe0;
    cell->m_unknown05 = m_players[player].Color() * 2 + 1;
    SetVisibility(m_castleRecs[townId].m_x, m_castleRecs[townId].m_y, player, giVisRangeTown);
    CheckEndGame(0);
}

// Buka 2.1 game::ClaimMine reduced to HoMM1's flag placement: the flag cell
// sits beside the mine by type and shows the owner's colour frame.
VA(0x0043ea65, 0x2c9)
void game::ClaimMine(signed char mineId, signed char player) {
    short frame;
    mapCell* cell;
    m_mines[mineId].owner = player;
    m_mineOwners[mineId] = player;
    switch (m_mines[mineId].type) {
        case 0:
            frame = 0x14;
            break;
        case 1:
            frame = 0x18;
            break;
        case 0x16:
            frame = 0x10;
            break;
        case 0x17:
            frame = 0xc;
            break;
        default:
            frame = 8;
            break;
    }
    switch (m_mines[mineId].type) {
        case 1:
            cell = &m_map[m_mines[mineId].x][m_mines[mineId].y - 2];
            break;
        case 0x16:
            cell = &m_map[m_mines[mineId].x - 1][m_mines[mineId].y - 3];
            break;
        case 0x17:
            cell = &m_map[m_mines[mineId].x - 2][m_mines[mineId].y];
            break;
        default:
            cell = &m_map[m_mines[mineId].x][m_mines[mineId].y - 1];
            break;
    }
    if (player == -1) {
        cell->m_flags ^= 0x20;
    } else {
        cell->m_flags |= 0x20;
        cell->m_overlayTileset |= 0xe0;
        cell->m_unknown05 = m_players[player].Color() + frame;
    }
}

// donor PoL RVA 0x00079856; preferred Buka symbol ?ViewSpells@game@@QAEHPAVhero@@HP6IHAAUtag_message@@@ZH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.637141;margin=0.178696;shape=0.427;size=0.725;calls=0.733;strings=spellwin.bin;alternate=pol20:int game::ViewSpells(class hero *, int, int (*)(struct tag_message &), int)@0x00079856
VA(0x0043ed2e, 0x297)
int game::ViewSpells(class hero*, int, short (*)(struct tag_message&), int) {
    return 0;
}

// donor PoL RVA 0x0007a649; preferred Buka symbol ?ViewArmy@game@@QAEXHHHHPAVtown@@HHHPAVhero@@PAVarmy@@PAVarmyGroup@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.612909;margin=0.340762;shape=0.385;size=0.681;calls=0.829;strings= (%d)|%s%d|armywin.bin;alternate=pol20:void game::ViewArmy(int, int, int, int, class town *, int, int, int, class hero *, class army *, class armyGroup *, int)@0x0007a649
VA(0x0043f8cd, 0x8e1)
void game::ViewArmy(
    int,
    int,
    signed char,
    short,
    class town*,
    signed char,
    signed char,
    signed char,
    class hero*,
    class army*,
    class armyGroup*
) {}

extern signed char gbDismissArmy;
extern long gViewArmyAnimTimer;

// donor PoL RVA 0x0007b2cf; preferred Buka symbol ?ViewArmyHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.308927;margin=0.170943;shape=0.232;size=0.493;calls=0.556;alternate=pol20:int ViewArmyHandler(struct tag_message &)@0x0007b2cf
VA(0x004401ae, 0x1b8)
short ViewArmyHandler(tag_message& message) {
    short frameDelay;
    short offset;
    gbDismissArmy = 0;
    frameDelay = 5;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.payload.widget.id) {
                    case 0x7800:
                    case 0x7801:
                        gpWindowManager->m_dialogResult = message.payload.widget.id;
                        message.payload.widget.command = message.payload.widget.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                    case 0x7803:
                        NormalDialog("Are you sure you want to dismiss this army?", 2, 0xb1, 0x36, -1, 0, -1, 0, -1);
                        if (gpWindowManager->m_dialogResult == 0x7805) {
                            gbDismissArmy = 1;
                            message.payload.widget.command = message.payload.widget.id =
                                WIDGET_COMMAND_DIALOG_SELECT;
                            return MESSAGE_DISPATCH_FORWARD;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (KBTickCount() > gViewArmyAnimTimer) {
        message.type = MESSAGE_WIDGET;
        message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
        message.payload.widget.id = 5;
        gpGame->m_viewArmyResult++;
        message.payload.widget.data.value = gpGame->m_viewArmyResult % 6;
        gpGame->m_viewArmyWindow->BroadcastMessage(message);
        gpGame->m_viewArmyWindow->DrawWindow();
        gViewArmyAnimTimer = KBTickCount() + 90;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 game::TurnOnAIMusic.
VA(0x004411e3, 0x3d)
void game::TurnOnAIMusic(void) {
    gpSoundManager->StopAllSamples();
    gpSoundManager->SwitchAmbientMusic(49);
    gpSoundManager->m_musicReady = 0;
}

// Buka 2.1 game::TurnOffAIMusic.
VA(0x00441220, 0x25)
void game::TurnOffAIMusic(void) {
    gpSoundManager->m_musicReady = 1;
}

// donor PoL RVA 0x0007bd99; preferred Buka symbol ?NextPlayer@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.506997;margin=1.216721;shape=0.284;size=0.968;calls=0.889;alternate=pol20:void game::NextPlayer(void)@0x0007bd99
VA(0x00441245, 0x4e1)
void game::NextPlayer(void) {}

// HoMM1 picks an unused random artifact (ids 4..36), else the first free one.
VA(0x004439c1, 0x79)
signed char game::GetRandomArtifactId(void) {
    signed char freeSlot = Scan(m_randomArtifacts, 4, 33);
    if (freeSlot == -1)
        return -1;
    signed char artifact = RandomScan(m_randomArtifacts, 4, 33, 37);
    if (artifact == -1)
        return freeSlot;
    else
        return artifact;
}

// donor PoL RVA 0x00080b64; preferred Buka symbol ?SetVisibility@game@@QAEXHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.449662;margin=0.505159;shape=0.181;size=0.875;calls=1.000;alternate=pol20:void game::SetVisibility(int, int, int, int)@0x00080b64
VA(0x004440e9, 0x259)
void game::SetVisibility(short x, short y, short player, short radius) {
    int i;
    int j;
    int cutoff;
    int rangeLeft;
    unsigned char viewMask = 1 << player;
    unsigned char outerMask = 1 << (player + 4);

    if (radius >= 5)
        cutoff = 3;
    else
        cutoff = 2;

    for (j = y - radius; j <= y + radius; ++j) {
        for (i = x - radius; i <= x + radius; ++i) {
            rangeLeft = radius - abs(y - j) + radius - abs(x - i);
            if (rangeLeft >= cutoff && i >= 0 && j >= 0 && i < MAP_CELL_GRID_SIZE
                && j < MAP_CELL_GRID_SIZE)
                gpGame->m_mapExtra[i][j] |= viewMask;
        }
    }
    if (gbHumanPlayer[player]) {
        for (j = y - radius - 1; j <= y + radius + 1; ++j) {
            for (i = x - radius - 1; i <= x + radius + 1; ++i) {
                rangeLeft = radius - abs(y - j) + radius - abs(x - i);
                if (rangeLeft + 1 >= cutoff && i >= 0 && j >= 0 && i < MAP_CELL_GRID_SIZE
                    && j < MAP_CELL_GRID_SIZE)
                    gpGame->m_mapExtra[i][j] |= outerMask;
            }
        }
    }
}

// donor PoL RVA 0x00080e6c; preferred Buka symbol ?GiveArmy@game@@QAEXPAVarmyGroup@@HHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.542661;margin=0.479075;shape=0.407;size=0.794;calls=1.000;alternate=pol20:void game::GiveArmy(class armyGroup *, int, int, int)@0x00080e6c
VA(0x00444342, 0xfc)
void game::GiveArmy(armyGroup* group, int type, int count, int slot) {
    int swap;
    int i;
    if (slot >= 0) {
        i = slot;
        group->m_creatureTypes[i] = type;
        group->m_creatureCounts[i] = 0;
    } else {
        for (i = 0; i < 5; ++i) {
            if (group->m_creatureTypes[i] == type)
                break;
        }
        if (i >= 5) {
            for (i = 0; i < 5; ++i) {
                if (group->m_creatureTypes[i] < 0) {
                    group->m_creatureCounts[i] = 0;
                    break;
                }
            }
        }
        if (i >= 5)
            return;
    }
    group->m_creatureTypes[i] = type;
    group->m_creatureCounts[i] += count;
}

// donor PoL RVA 0x00080f68; preferred Buka symbol ?ExperienceValueOfStack@game@@QAEHPAVarmyGroup@@PAVhero@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.491573;margin=0.501516;shape=0.250;size=0.900;calls=1.000;alternate=pol20:int game::ExperienceValueOfStack(class armyGroup *, class hero *)@0x00080f68
VA(0x0044443e, 0x8c)
int game::ExperienceValueOfStack(armyGroup* group, hero* h) {
    int expValue = 0;
    int i;
    for (i = 0; i < 5; ++i) {
        if (group->m_creatureCounts[i] > 0)
            expValue += group->m_creatureCounts[i] * gMonsterDatabase[group->m_creatureTypes[i]].hitPoints;
    }
    if (h)
        expValue += 500;
    return expValue;
}

// Buka 2.1 MiscRuntime seeded generator; HoMM1 keeps it in this TU.
VA(0x004444ca, 0x92)
int SGenRand(void)
{
    int value = 0;
    int i;
    int bitMask;
    iLastSeed &= 0xfff;
    iLastSeed *= 7;
    iLastSeed += (iLastSeed & 0xff0) >> 4;
    for (i = 31; i >= 0; --i) {
        bitMask = 1 << i;
        if (iLastSeed & bitMask)
            value |= 1 << i;
    }
    return value;
}

VA(0x0044455c, 0x52)
int SRandom(int low, int high)
{
    int result;
    SIncRandomize(low, high);
    result = SGenRand();
    iLastSeed += low;
    iLastSeed += high * 8;
    return result % (high - low + 1) + low;
}

VA(0x004445ae, 0x89)
void SIncRandomize(int x, int y)
{
    int feedback;
    x *= 13;
    y *= 13;
    x &= 0xff;
    y &= 0xff;
    iLastSeed += y << 5;
    iLastSeed += x * 13233;
    iLastSeed += y;
    feedback = iLastSeed & 0x3f;
    iLastSeed += feedback << 8;
}

VA(0x00444637, 0x24)
void SRand(int seed)
{
    iLastSeed = seed;
    srand(seed);
}

// donor PoL RVA 0x00080ff9; preferred Buka symbol ?GetLuck@game@@QAEHPAVhero@@PAVarmy@@PAVtown@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.340271;margin=0.529148;shape=0.188;size=0.654;calls=0.667;alternate=pol20:int game::GetLuck(class hero *, class army *, class town *)@0x00080ff9
VA(0x0044465b, 0xbf)
int game::GetLuck(class hero*, class army*) {
    return 0;
}

// Buka 2.1 keeps the scan cursor in file statics.
static int s_adjacentMonsterEndX;
static int s_adjacentMonsterEndY;
static int s_adjacentMonsterX;
static int s_adjacentMonsterY;
static int s_adjacentMonsterMinX;
static int s_adjacentMonsterMinY;

// donor PoL RVA 0x00069bef; preferred Buka symbol ?FindAdjacentMonster@advManager@@QAEHHHPAH0HH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.502628;margin=0.574839;shape=0.364;size=0.953;calls=0.667;alternate=pol20:int advManager::FindAdjacentMonster(int, int, int *, int *, int, int)@0x00069bef
// HoMM1 retail returns the found flag in AL (xor al,al / mov al,1).
VA(0x0044471a, 0x350)
signed char advManager::FindAdjacentMonster(
    int originX, int originY, int* monsterX, int* monsterY, int excludedX, int excludedY
) {
    s_adjacentMonsterEndX = originX + 2;
    s_adjacentMonsterEndY = originY + 2;

    if (originX > 0 && originY > 0 && originX < MAP_CELL_GRID_SIZE - 1
        && originY < MAP_CELL_GRID_SIZE - 1) {
        for (s_adjacentMonsterX = originX - 1; s_adjacentMonsterX < s_adjacentMonsterEndX;
             ++s_adjacentMonsterX) {
            for (s_adjacentMonsterY = originY - 1; s_adjacentMonsterY < s_adjacentMonsterEndY;
                 ++s_adjacentMonsterY) {
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType == 0x9a) {
                    if (s_adjacentMonsterY < originY) {
                        if ((GetCell(originX, originY)->m_objectIndex == 0xff
                             || (GetCell(originX, originY)->m_flags & 0x80))
                            && (s_adjacentMonsterX != excludedX
                                || s_adjacentMonsterY != excludedY))
                            goto foundAdjacentMonster;
                    } else if (s_adjacentMonsterX != excludedX || s_adjacentMonsterY != excludedY) {
                        goto foundAdjacentMonster;
                    }
                }
            }
        }
    } else {
        if (originX == MAP_CELL_GRID_SIZE - 1)
            s_adjacentMonsterEndX = originX + 1;
        if (originY == MAP_CELL_GRID_SIZE - 1)
            s_adjacentMonsterEndY = originY + 1;
        if (originX == 0)
            s_adjacentMonsterMinX = 0;
        else
            s_adjacentMonsterMinX = originX - 1;
        if (originY == 0)
            s_adjacentMonsterMinY = 0;
        else
            s_adjacentMonsterMinY = originY - 1;

        for (s_adjacentMonsterX = s_adjacentMonsterMinX; s_adjacentMonsterX < s_adjacentMonsterEndX;
             ++s_adjacentMonsterX) {
            for (s_adjacentMonsterY = s_adjacentMonsterMinY;
                 s_adjacentMonsterY < s_adjacentMonsterEndY;
                 ++s_adjacentMonsterY) {
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType == 0x9a) {
                    if (s_adjacentMonsterY < originY) {
                        if ((GetCell(originX, originY)->m_objectIndex == 0xff
                             || (GetCell(originX, originY)->m_flags & 0x80))
                            && (s_adjacentMonsterX != excludedX
                                || s_adjacentMonsterY != excludedY))
                            goto foundAdjacentMonster;
                    } else if (s_adjacentMonsterX != excludedX || s_adjacentMonsterY != excludedY) {
                        goto foundAdjacentMonster;
                    }
                }
            }
        }
    }
    return 0;

foundAdjacentMonster:
    *monsterX = s_adjacentMonsterX;
    *monsterY = s_adjacentMonsterY;
    return 1;
}

// donor PoL RVA 0x0008111f; preferred Buka symbol ?SetupAdjacentMons@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.519474;margin=0.741945;shape=0.279;size=0.996;calls=1.000;alternate=pol20:void game::SetupAdjacentMons(void)@0x0008111f
VA(0x00444a6a, 0xde)
void game::SetupAdjacentMons(void) {
    int monX;
    int monY;
    int x;
    int mask = 0x7f;
    int y;

    for (x = 0; x < MAP_CELL_GRID_SIZE; ++x) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; ++y) {
            if (gpAdvManager->FindAdjacentMonster(x, y, &monX, &monY, -1, -1))
                mapExtra[x][y] |= 0x80;
            else
                mapExtra[x][y] &= mask;
        }
    }
}

// donor PoL RVA 0x00081210; preferred Buka symbol ?CancelComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526467;margin=0.434153;shape=0.371;size=0.866;calls=1.000;alternate=pol20:void game::CancelComputerScreen(void)@0x00081210
VA(0x00444b48, 0x61)
void game::CancelComputerScreen(void) {
    TurnOffAIMusic();
    bShowIt = 1;
    int i;
    for (i = 1; i <= 6; ++i)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            i,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
}

// donor PoL RVA 0x00081271; preferred Buka symbol ?ShowComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.481260;margin=0.673888;shape=0.345;size=0.812;calls=0.778;alternate=pol20:void game::ShowComputerScreen(void)@0x00081271
VA(0x00444ba9, 0x115)
void game::ShowComputerScreen(void) {
    if (gConfig.blackoutComputer || gbRemoteOn) {
        int saved = gbThisNetHumanPlayer[giCurPlayer];
        gbThisNetHumanPlayer[giCurPlayer] = 1;
        int i;
        for (i = 1; i <= 6; ++i)
            gpWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                WIDGET_COMMAND_SET_FLAGS,
                i,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        gpMouseManager->ReallyHidePointer();
        gbAllBlack = 1;
        gpAdvManager->CompleteDraw(1);
        gpAdvManager->UpdateHeroLocators(1, 1);
        gpAdvManager->UpdateTownLocators(1, 1);
        gpAdvManager->UpdBottomView(1, 1, 1);
        gpAdvManager->UpdateScreen(0, 1);
        gbAllBlack = 0;
        gbThisNetHumanPlayer[giCurPlayer] = saved;
        gpMouseManager->ReallyShowPointer();
    }
    ShowHeroesLogo();
}

// Buka 2.1 game::ShowHeroesLogo; HoMM1 draws the logo from a tileset.
VA(0x00444cbe, 0xa8)
void game::ShowHeroesLogo(void) {
    tileset* logo;
    if (!gpAdvManager->m_openState) {
        gpMouseManager->ReallyHidePointer();
        gpAdvManager->m_openState = 1;
        logo = gpResourceManager->GetTileset("herologo.til");
        TileToBitmap(logo, 0, gpWindowManager->m_screen, 480, 16);
        gpWindowManager->UpdateScreenRegion(480, 16, 144, 144);
        gpResourceManager->Dispose(logo);
        gpMouseManager->ReallyShowPointer();
    }
}

// donor PoL RVA 0x000813fe; preferred Buka symbol ?WaitForPlayer@game@@QAEXPADH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547163;margin=0.571086;shape=0.476;size=0.842;calls=0.833;alternate=pol20:void game::WaitForPlayer(char *, int)@0x000813fe
VA(0x00444d66, 0x155)
void game::WaitForPlayer(char* text, int player) {
    if (gbBlackoutPlayer && giNumHumanPlayers > 1 && !gbRemoteOn) {
        gpMouseManager->SetPointer(0);
        gbAllBlack = 1;
        giBottomViewOverrideEndTime = KBTickCount() + 9999999;
        if (gbThisNetHumanPlayer[giCurPlayer])
            giBottomViewOverride = 1;
        else
            giBottomViewOverride = 0;
        gpSoundManager->m_musicReady = 1;
        gpSoundManager->SwitchAmbientMusic(15);
        gpMouseManager->ReallyHidePointer();
        gpAdvManager->CompleteDraw(1);
        gpAdvManager->UpdateHeroLocators(1, 1);
        gpAdvManager->UpdateTownLocators(1, 1);
        gpAdvManager->UpdateScreen(0, 1);
        ShowHeroesLogo();
        gbAllBlack = 0;
        gpMouseManager->ReallyShowPointer();
        NormalDialog(text, 1, 0x61, -1, 9, gpGame->m_players[player].m_unknown11, -1, 0, -1);
        gpSoundManager->SwitchAmbientMusic(-1);
    }
}

// donor PoL RVA 0x00082547; preferred Buka symbol ?ProcessOnMapHeroes@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.296138;margin=0.478209;shape=0.233;size=0.458;calls=0.545;alternate=pol20:void game::ProcessOnMapHeroes(void)@0x00082547
VA(0x004452a9, 0x347)
void game::ProcessOnMapHeroes(void) {}

// donor PoL RVA 0x00082cbb; preferred Buka symbol ?CheckHeroConsistency@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.487324;margin=0.544395;shape=0.344;size=0.775;calls=1.000;alternate=pol20:void game::CheckHeroConsistency(void)@0x00082cbb
VA(0x004455f0, 0x3b5)
void game::CheckHeroConsistency(void) {}

// donor PoL RVA 0x00083219; preferred Buka symbol ?TransmitSaveGame@game@@QAEHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.660125;margin=0.426397;shape=0.321;size=0.898;calls=0.886;strings=%s%s|.\DATA\|PostWait;alternate=pol20:int game::TransmitSaveGame(int, int, int)@0x00083219
VA(0x004459a5, 0x6e9)
int game::TransmitSaveGame(int, int) { return 0; }

// donor PoL RVA 0x00083937; preferred Buka symbol ?ReceiveSaveGame@game@@QAEHHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.655741;margin=0.222523;shape=0.420;size=0.807;calls=0.714;strings=%s%s|.\DATA\|Receive End;alternate=pol20:int game::ReceiveSaveGame(int, int, int, int)@0x00083937
VA(0x0044608e, 0x579)
int game::ReceiveSaveGame(int, int) { return 0; }

// New-turn texts: days-left and last-day warnings, then the month/week banners.
extern char* gNewTurnText[];
extern char* gColorNames[];
extern char* gMonsterNames[];
extern char* gMonthNames[];
extern char* gWeekNames[];
extern signed char giWeekType;
extern signed char giMonthType;
extern signed char giWeekSpecial;
extern signed char giMonthSpecial;

// donor PoL RVA 0x00083fc4; preferred Buka symbol ?DoNewTurn@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.487826;margin=0.297906;shape=0.344;size=0.836;calls=0.867;alternate=pol20:void game::DoNewTurn(void)@0x00083fc4
VA(0x00446607, 0x42b)
void game::DoNewTurn(void) {
    int track;
    char monsterName[52];

    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        CheckEndGame(0);
        return;
    }
    giBottomViewOverrideEndTime = KBTickCount() + 3000;
    giBottomViewOverride = 1;
    gpAdvManager->UpdBottomView(1, 1, 1);
    gpAdvManager->SetInitialMapOrigin();
    gpAdvManager->CompleteDraw(0);
    gpAdvManager->UpdateScreen(0, 0);
    CheckEndGame(0);
    if (gpCurPlayer->m_unknown55 >= 0) {
        if (gpCurPlayer->m_unknown55 == 1) {
            sprintf(gText, gNewTurnText[1], gColorNames[gpGame->m_players[giCurPlayer].Color()]);
            gText[0] -= 32;
        } else {
            sprintf(
                gText,
                gNewTurnText[0],
                gColorNames[gpGame->m_players[giCurPlayer].Color()],
                gpCurPlayer->m_unknown55
            );
            gText[0] -= 32;
        }
        NormalDialog(gText, 1, 0x61, -1, 9, gpGame->m_players[giCurPlayer].Color(), -1, 0, -1);
    }
    if (gpCurPlayer->m_heroCount > 0)
        gpAdvManager->SetHeroContext(gpCurPlayer->NextHero(0), 0);
    else if (gpCurPlayer->m_townCount > 0)
        gpAdvManager->SetTownContext(gpCurPlayer->m_townIds[0]);
    gpAdvManager->CheckDimNextHeroBut();
    gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    if (m_day == 1) {
        if ((m_month != 1 || m_week != 1 || m_day != 1) && giWeekType != -1) {
            track = -1;
            if (m_week == 1) {
                track = 0x33;
                if (giMonthType == 0) {
                    sprintf(gText, gNewTurnText[2], gMonthNames[giMonthSpecial]);
                } else if (giMonthType == 1) {
                    strcpy(monsterName, gMonsterNames[giMonthSpecial]);
                    monsterName[0] -= 32;
                    sprintf(gText, gNewTurnText[3], gMonsterNames[giMonthSpecial], monsterName);
                } else {
                    sprintf(gText, gNewTurnText[4]);
                }
            } else {
                track = 0x32;
                if (giWeekType == 0) {
                    sprintf(gText, gNewTurnText[5], gWeekNames[giWeekSpecial]);
                } else {
                    strcpy(monsterName, gMonsterNames[giWeekSpecial]);
                    monsterName[0] -= 32;
                    sprintf(gText, gNewTurnText[6], gMonsterNames[giWeekSpecial], monsterName);
                }
            }
            gpSoundManager->SwitchAmbientMusic(track);
            gpMouseManager->SetPointer(0);
            NormalDialog(gText, 1, 0x61, -1, -1, 0, -1, 0, -1);
            gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
        }
    }
}

// Buka 2.1 game::GetBoatsBuilt.
VA(0x00446a32, 0x58)
int game::GetBoatsBuilt(void) {
    int count = 0;
    int i;
    for (i = 0; i < GAME_BOAT_COUNT; ++i) {
        if (m_boatSlots[i] != -1)
            ++count;
    }
    return count;
}

// donor PoL RVA 0x000b6f40; preferred Buka symbol ?GetMap@game@@QAEXXZ
// donor Buka TU SOURCE/Newgame; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547944;margin=0.077231;shape=0.356;size=0.530;calls=0.607;strings=.\MAPS\;alternate=pol20:void game::GetMap(void)@0x000b6f40
VA(0x00446a8a, 0x36f)
void game::GetMap(void) {}

// donor PoL RVA 0x000333c0; preferred Buka symbol ?ViewWorld@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/Viewwrld; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.364493;margin=0.061615;shape=0.277;size=0.633;calls=0.682;alternate=pol20:void advManager::ViewWorld(int, int, int)@0x000333c0
// Retail loads sceninfo.bin and is called on gpGame with no arguments:
// Buka's game::ShowScenInfo, not the adventure-map ViewWorld (0x431507).
VA(0x004472d8, 0x44e)
void game::ShowScenInfo(void) {}

// Buka 2.1 game::GetNumThievesGuilds.
VA(0x00446df9, 0x98)
int game::GetNumThievesGuilds(int color) {
    int numGuilds = 0;
    int i;
    for (i = 0; i < m_players[color].m_townCount; ++i) {
        if (gpGame->m_castleRecs[m_players[color].m_townIds[i]].m_buildings & 2)
            ++numGuilds;
    }
    return numGuilds;
}

// donor PoL RVA 0x0008480a; preferred Buka symbol ?RestoreCell@game@@QAEXHHHHPAVmapCell@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.572404;margin=0.482531;shape=0.423;size=0.977;calls=1.000;alternate=pol20:void game::RestoreCell(int, int, int, int, class mapCell *, int)@0x0008480a
VA(0x00447875, 0xa1)
void game::RestoreCell(int x, int y, int obj, int barrier, mapCell* passedCell, int) {
    mapCell* cell;
    if (passedCell)
        cell = passedCell;
    else
        cell = gpAdvManager->GetCell(x, y);
    if (y > 0 && obj == 0xa8 && gpAdvManager->GetCell(x, y - 1)->m_triggerType != 0x28) {
        cell->m_triggerType = 0;
        cell->m_objectMetadata = 0;
    } else {
        cell->m_triggerType = obj;
        cell->m_objectMetadata = barrier;
    }
}

// donor PoL RVA 0x0008c040; preferred Buka symbol ??0armyGroup@@QAE@XZ
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.513410;margin=0.267257;shape=0.385;size=0.817;calls=1.000;alternate=pol20:void armyGroup::constructor(void)@0x0008c040
VA(0x00447920, 0x3c)
armyGroup::armyGroup(void) {
    memset(m_creatureTypes, -1, sizeof(m_creatureTypes));
    memset(m_creatureCounts, 0, sizeof(m_creatureCounts));
}

VA(0x0044795c, 0x18)
void armyGroup::View(int) {}

VA(0x00447a8f, 0x31)
void armyGroup::Dismiss(signed char slot) {
    m_creatureTypes[slot] = -1;
    m_creatureCounts[slot] = 0;
}

// donor PoL RVA 0x0008c3f6; preferred Buka symbol ?IsMember@armyGroup@@QAEHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.401440;margin=0.383163;shape=0.171;size=0.719;calls=1.000;alternate=pol20:int armyGroup::IsMember(int)@0x0008c3f6
// HoMM1 retail reads a signed byte parameter and returns in AL.
VA(0x00447ac0, 0x59)
signed char armyGroup::IsMember(signed char creatureType) {
    for (short slot = 0; slot < 5; ++slot) {
        if (m_creatureTypes[slot] == creatureType)
            return 1;
    }
    return 0;
}

// Buka 2.1 IsHomogeneous; HoMM1 races are six consecutive creature ids.
VA(0x00447b19, 0x153)
signed char armyGroup::IsHomogeneous(signed char countRaces) {
    int numTypes = 0;
    signed char raceSeen[5];
    raceSeen[0] = raceSeen[1] = raceSeen[2] = raceSeen[3] = raceSeen[4] = 0;
    int previous = -1;
    int numRaces;
    short i;
    for (i = 0; i < 5; ++i) {
        if (m_creatureTypes[i] != -1) {
            if (countRaces == -1)
                ++raceSeen[m_creatureTypes[i] / 6];
            if (m_creatureTypes[i] != previous) {
                ++numTypes;
                previous = m_creatureTypes[i];
            }
        }
    }

    if (numTypes <= 1)
        return 0;

    numRaces = 0;
    for (i = 0; i < 5; ++i) {
        if (raceSeen[i])
            ++numRaces;
    }

    if (numRaces == 1)
        return 1;
    if (numRaces == 3)
        return -1;
    if (numRaces == 4)
        return -2;
    if (numRaces == 5)
        return -3;
    return 0;
}

// donor PoL RVA 0x0008c599; preferred Buka symbol ?CanJoin@armyGroup@@QAEHH@Z
// donor Buka TU SOURCE/ARMYGRP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.455116;margin=0.419277;shape=0.310;size=0.702;calls=1.000;alternate=pol20:int armyGroup::CanJoin(int)@0x0008c599
// HoMM1 retail returns in AL and sign-extends its IsMember call results.
VA(0x00447c6c, 0x54)
signed char armyGroup::CanJoin(signed char creatureType) {
    if (IsMember(creatureType))
        return 1;
    if (IsMember(-1))
        return 1;
    return 0;
}

VA(0x00447cc0, 0x59)
short armyGroup::GetNumArmies(void) {
    short numArmies = 0;
    for (short i = 0; i < 5; ++i) {
        if (m_creatureTypes[i] != -1)
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
    if (slot == -1) {
        for (searchSlot = 0; searchSlot < 5; ++searchSlot) {
            if (m_creatureTypes[searchSlot] == creatureType) {
                slot = searchSlot;
                break;
            }
        }
    }
    if (slot == -1) {
        for (searchSlot = 0; searchSlot < 5; ++searchSlot) {
            if (m_creatureTypes[searchSlot] == -1 || m_creatureTypes[searchSlot] == creatureType) {
                slot = searchSlot;
                break;
            }
        }
    }
    if (slot >= 5)
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
    int chance = (int)(damagePercent * 100.0f);
    int isFirstTroop = 1;
    int i;
    int j;

    for (i = 0; i < 5; ++i) {
        if (m_creatureTypes[i] != -1) {
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
                m_creatureTypes[i] = -1;
            }
            isFirstTroop = 0;
        } else {
            m_creatureCounts[i] = 0;
        }
    }
}
