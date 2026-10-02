// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>

#include <stdlib.h>
#include <string.h>

// Compiler line-base word for TOWNMGR.CPP's ProcessAssert sites.
DATA(0x0048ed8c) short gTownMgrAssertLine = 1483;

// donor PoL RVA 0x00013900; preferred Buka symbol ??0townObject@@QAE@HHPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.615649;margin=0.314990;shape=0.431;size=0.799;calls=0.333;strings=%s.icn;alternate=pol20:void townObject::constructor(int, int, char *)@0x00013900
// Buka TOWNMGR.cpp townObject ctor; HoMM1 reads frame count, rectangle and
// building id from the .tod resource instead of sBuildingInfo.
VA(0x00407d90, 0x1f1)
townObject::townObject(char *name)
{
    char fileName[16];
    short w;
    short tmp;
    short id;
    short x;
    short h;
    short y;

    m_animationFrame = 0;
    m_icon = NULL;
    m_border = NULL;
    m_visible = 1;
    sprintf(fileName, "%s.tod", name);
    id = gpResourceManager->MakeId(fileName);
    gpResourceManager->PointToFile(id);
    m_animationFrameCount = gpResourceManager->ReadByte();
    x = gpResourceManager->ReadWord();
    y = gpResourceManager->ReadWord();
    w = gpResourceManager->ReadWord();
    h = gpResourceManager->ReadWord();
    id = gpResourceManager->ReadWord();
    m_buildingId = id;
    sprintf(fileName, "%s.icn", name);
    m_icon = gpResourceManager->GetIcon(fileName);
    if (id == 0) {
        h = gpTownManager->m_town->m_buildState * 20 + 0x61;
        y = 0x99 - h;
    }
    if (id != -1) {
        m_border = new border(x, y, w, h, id, 1, 0, NULL);
        if (m_border == NULL)
            MemError();
    }
}

// donor PoL RVA 0x00013a6a; preferred Buka symbol ??1townObject@@QAE@XZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.564007;margin=0.293257;shape=0.438;size=0.896;calls=1.000;alternate=pol20:void townObject::~destructor(void)@0x00013a6a
VA(0x00407f81, 0x60)
townObject::~townObject() {
    if (m_border != NULL)
        delete m_border;
    gpResourceManager->Dispose(m_icon);
}

// Buka TOWNMGR.cpp:537-625; HoMM1 draws the base frame, then the castle's
// mage-guild levels and the animation frame.
VA(0x00407fe1, 0x117)
void townObject::Draw(signed char advanceAnimation)
{
    short level;

    if (!m_visible)
        return;
    m_icon->DrawToBuffer(0, 0, 0, ICON_DRAW_NORMAL, 0);
    if (m_buildingId == 0) {
        for (level = 0; level < gpTownManager->m_town->m_buildState; level++)
            m_icon->DrawToBuffer(0, 0, (level + 1) * 2, ICON_DRAW_NORMAL, 0);
        m_icon->DrawToBuffer(0, 0, gpTownManager->m_town->m_buildState * 2 + 1, ICON_DRAW_NORMAL, 0);
    }
    if (m_animationFrameCount) {
        m_icon->DrawToBuffer(0, 0, m_animationFrame + 1, ICON_DRAW_NORMAL, 0);
        if (advanceAnimation == 1) {
            m_animationFrame++;
            if (m_animationFrame == m_animationFrameCount)
                m_animationFrame = 0;
        }
    }
}

// Buka TOWNMGR.cpp:627-633; HoMM1 also clears the object count and adds
// its dispatch mask.
VA(0x004080f8, 0x74)
townManager::townManager(void)
{
    m_town = NULL;
    m_townObjectCount = 0;
    m_heroWindow0 = NULL;
    m_coverWindow = NULL;
    m_selectedBuilding = -1;
    m_castleDialogActive = 0;
    m_dispatchMask = TOWN_MANAGER_DISPATCH_MASK;
}

// Buka TOWNMGR.cpp Open/SetupTown; retail vtable slot 0 (0x0048c068).
// HoMM1 builds the town window, objects, strips and bank box here.
VA(0x0040816c, 0x7ec)
short townManager::Open(short id)
{
    short crest;
    tag_message message;
    short i;
    signed char buildingType;

    gpGame->CheckHeroConsistency();
    gpSoundManager->PlayAmbientMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE, 0, -1);
    PollSound();
    m_townWindow = new heroWindow(0, 0, "townwind.bin");
    if (m_townWindow == NULL)
        MemError();
    sprintf(gText, GetTownName(m_town->m_id));
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_NAME_TEXT_CONTROL;
    message.text = gText;
    m_townWindow->BroadcastMessage(message);
    strcpy(gText, "Town Screen");
    message.id = TOWN_STATUS_TEXT_CONTROL;
    message.text = gText;
    m_townWindow->BroadcastMessage(message);
    sprintf(gText, "townbkg%d.bmp", m_town->m_type);
    m_backgroundBitmap = gpResourceManager->GetBitmap(gText);
    m_townObjectCount = 0;
    for (i = 0; i < TOWN_MANAGER_OBJECT_CAPACITY; i++) {
        buildingType = gTownObjectType[m_town->m_type][i];
        if (buildingType != -1) {
            // One name table: neutral objects, four town-type prefixes, then
            // the faction-object suffixes.
            if (buildingType < TOWN_FIRST_FACTION_OBJECT)
                strcpy(gText, cTownObjectNames[buildingType]);
            else
                sprintf(gText, "%s%s", cTownObjectNames[TOWN_FIRST_FACTION_OBJECT + m_town->m_type],
                        cTownObjectNames[buildingType + 4]);
            m_townObjects[m_townObjectCount] = new townObject(gText);
            if (m_townObjects[m_townObjectCount] == NULL)
                MemError();
            if (m_townObjects[m_townObjectCount]->m_border) {
                if (!(m_town->m_buildings & (1 << buildingType))) {
                    m_townObjects[m_townObjectCount]->m_border->m_flags &= ~TOWN_OBJECT_ENABLED_FLAG;
                    m_townObjects[m_townObjectCount]->m_visible = 0;
                }
                m_townWindow->AddWidget(m_townObjects[m_townObjectCount]->m_border, -1);
            }
            m_townObjectCount++;
        }
    }
    glTimers[0] = KBTickCount() + TOWN_REDRAW_INTERVAL;
    gpWindowManager->AddWindow(m_townWindow, 0, 1);
    crest = gpCurPlayer->m_color;
    if (m_town->OccupyingHero() != -1) {
        crest = crest << 2;
        crest += gpGame->GetHero(m_town->m_occupyingHeroId)->m_heroClass;
    } else
        crest += TOWN_CREST_NO_HERO_OFFSET;
    sprintf(gText, "crst%04d.icn", crest);
    m_garrisonStrip = new strip(0, 0x100, m_town->m_occupyingHeroId == -1 ? 4 : 1,
                                gpResourceManager->MakeId(gText), 0, &m_town->m_army, 0x10, 1);
    if (m_garrisonStrip == NULL)
        MemError();
    if (m_town->m_occupyingHeroId != -1) {
        sprintf(gText, "port%04d.icn", gpGame->GetHero(m_town->m_occupyingHeroId)->m_portrait);
        m_heroStrip = new strip(0, 0x163, 3, gpResourceManager->MakeId(gText), 0,
                                &gpGame->GetHero(m_town->m_occupyingHeroId)->m_army, 0x16, 1);
        if (m_heroStrip == NULL)
            MemError();
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            m_town->GiveSpells();
    } else {
        m_heroStrip = new strip(0, 0x163, 3, gpResourceManager->MakeId("strip.icn"), 8, NULL, -1, 1);
        if (m_heroStrip == NULL)
            MemError();
    }
    m_bankBox = new bankBox(0x222, 0x100, gpCurPlayer);
    if (m_bankBox == NULL)
        MemError();
    m_selectedStrip = m_swapStrip = m_pendingStrip = NULL;
    m_selectedArmySlot = m_swapArmySlot = m_pendingArmySlot = -1;
    DrawTown(0, 0);
    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    gpMouseManager->SetPointer("advmice.mse", 0);
    gpMouseManager->ReallyShowPointer();
    gpMouseManager->NewUpdate(1);
    KBChangeMenu(hmnuTown);
    gpWindowManager->FadeScreen(0, 8, NULL);
    m_castleDialogActive = 0;
    m_recruitResult = 0;
    m_lastHoverId = -1;
    m_messageMask = TOWN_MANAGER_MESSAGE_MASK;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "townManager");
    return 0;
}

// donor PoL RVA 0x00014cc9; preferred Buka symbol ?UnloadTown@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.470224;margin=0.176996;shape=0.284;size=0.924;calls=0.667;alternate=pol20:void townManager::UnloadTown(void)@0x00014cc9
// Retail vtable slot 1: HoMM1's Close performs Buka's UnloadTown work.
VA(0x00408958, 0x1c4)
void townManager::Close(void)
{
    short index;

    delete m_bankBox;
    if (m_heroStrip)
        delete m_heroStrip;
    delete m_garrisonStrip;
    for (index = 0; index < m_townObjectCount; index++) {
        m_townWindow->RemoveWidget(m_townObjects[index]->m_border);
        delete m_townObjects[index];
    }
    gpResourceManager->Dispose(m_backgroundBitmap);
    gpWindowManager->RemoveWindow(m_townWindow);
    delete m_townWindow;
    gpSoundManager->SwitchAmbientMusic(-1);
    gpWindowManager->FadeScreen(1, 8, NULL);
    gpMouseManager->SetPointer(-1);
    m_active = 0;
}

// Buka TOWNMGR.cpp:944-1020; HoMM1 matches the dragged creature against
// every slot of the target army and keeps word-sized flags.
VA(0x00408b1c, 0x3b6)
void townManager::SetArmyCommand(short qualifier)
{
    short lastArmy;
    short i;
    short sameType;

    m_command = TOWN_ARMY_COMMAND_NONE;
    lastArmy = 0;
    if (m_swapStrip->m_army->GetNumArmies() == 1 && m_swapStrip == m_heroStrip
        && m_swapStrip != m_pendingStrip)
        lastArmy = 1;

    if (m_swapStrip != m_pendingStrip) {
        sameType = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                == m_pendingStrip->m_army->m_creatureTypes[i])
                sameType = 1;
        }
        if (sameType) {
            if (qualifier) {
                sprintf(m_statusText, cTownCommand[TOWN_TEXT_REDISTRIBUTE_ARMY],
                        gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
                m_command = TOWN_ARMY_COMMAND_SPLIT;
            } else if (lastArmy) {
                strcpy(m_statusText, cTownCommand[TOWN_TEXT_CANNOT_COMBINE_LAST_ARMY]);
                return;
            } else {
                sprintf(m_statusText, cTownCommand[TOWN_TEXT_COMBINE_ARMIES],
                        gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
                m_command = TOWN_ARMY_COMMAND_MERGE;
            }
        } else if (qualifier && m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == CREATURE_NONE) {
            sprintf(m_statusText, cTownCommand[TOWN_TEXT_REDISTRIBUTE_TO_EMPTY_SLOT],
                    gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
            m_command = TOWN_ARMY_COMMAND_SPLIT;
        }
    } else if (m_swapArmySlot == m_pendingArmySlot) {
        sprintf(m_statusText, cTownCommand[TOWN_TEXT_VIEW_ARMY],
                gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
        m_command = TOWN_ARMY_COMMAND_VIEW;
    }

    if (m_command != TOWN_ARMY_COMMAND_NONE)
        return;
    if (m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == CREATURE_NONE) {
        if (lastArmy) {
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_CANNOT_MOVE_LAST_ARMY]);
            return;
        } else {
            sprintf(m_statusText, cTownCommand[TOWN_TEXT_MOVE_ARMY],
                    gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
            m_command = TOWN_ARMY_COMMAND_SWAP;
        }
    } else {
        sprintf(m_statusText, cTownCommand[TOWN_TEXT_EXCHANGE_ARMIES],
                gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
                gArmyNames[m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]]);
        m_command = TOWN_ARMY_COMMAND_SWAP;
    }
}

// Buka TOWNMGR.cpp:1022-1176; HoMM1 has no calendar entry and names the
// six dwellings through gDwellingType.
VA(0x00408ed2, 0x468)
void townManager::SetCommandAndText(struct tag_message &message)
{
    short id;

    id = message.id;
    m_command = TOWN_ARMY_COMMAND_NONE;
    switch (id) {
        case TOWN_CLOSE_CONTROL:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_EXIT]);
            break;
        case -1:
        case TOWN_EMPTY_STATUS_CONTROL_FIRST:
        case TOWN_EMPTY_STATUS_CONTROL_LAST:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_EMPTY_STATUS]);
            break;
        case TOWN_GARRISON_FIRST_CONTROL:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_GARRISON]);
            m_command = TOWN_ARMY_COMMAND_GARRISON;
            break;
        case TOWN_GARRISON_SLOT_FIRST:
        case TOWN_GARRISON_SLOT_FIRST + 1:
        case TOWN_GARRISON_SLOT_FIRST + 2:
        case TOWN_GARRISON_SLOT_FIRST + 3:
        case TOWN_GARRISON_SLOT_FIRST + 4:
            if (m_swapArmySlot != -1) {
                m_pendingStrip = m_garrisonStrip;
                m_pendingArmySlot = id - TOWN_GARRISON_SLOT_FIRST;
                SetArmyCommand(message.modifiers & TOWN_SHIFT_QUALIFIER_MASK);
            } else {
                m_selectedStrip = m_garrisonStrip;
                m_selectedArmySlot = id - TOWN_GARRISON_SLOT_FIRST;
                if (m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] == CREATURE_NONE)
                    strcpy(m_statusText, cTownCommand[TOWN_TEXT_EMPTY_SLOT]);
                else {
                    sprintf(m_statusText, cTownCommand[TOWN_TEXT_SELECT_ARMY],
                            gArmyNames[m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]]);
                    m_command = TOWN_ARMY_COMMAND_SELECT;
                }
            }
            break;
        case TOWN_HERO_FIRST_CONTROL:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_VIEW_HERO]);
            m_command = TOWN_ARMY_COMMAND_VIEW_HERO;
            break;
        case TOWN_HERO_SLOT_FIRST:
        case TOWN_HERO_SLOT_FIRST + 1:
        case TOWN_HERO_SLOT_FIRST + 2:
        case TOWN_HERO_SLOT_FIRST + 3:
        case TOWN_HERO_SLOT_FIRST + 4:
            if (m_swapArmySlot != -1) {
                m_pendingStrip = m_heroStrip;
                m_pendingArmySlot = id - TOWN_HERO_SLOT_FIRST;
                SetArmyCommand(message.modifiers & TOWN_SHIFT_QUALIFIER_MASK);
            } else {
                m_selectedStrip = m_heroStrip;
                m_selectedArmySlot = id - TOWN_HERO_SLOT_FIRST;
                if (m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] == CREATURE_NONE) {
                    strcpy(m_statusText, cTownCommand[TOWN_TEXT_EMPTY_SLOT]);
                    m_command = TOWN_ARMY_COMMAND_NONE;
                } else {
                    sprintf(m_statusText, cTownCommand[TOWN_TEXT_SELECT_ARMY],
                            gArmyNames[m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]]);
                    m_command = TOWN_ARMY_COMMAND_SELECT;
                }
            }
            break;
        case 0:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0]);
            break;
        case 1:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 1]);
            break;
        case 2:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 2]);
            break;
        case 3:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 3]);
            break;
        case 4:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 4]);
            break;
        case 5:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 5]);
            break;
        case 6:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 6]);
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            sprintf(m_statusText, cTownCommand[TOWN_TEXT_DWELLING],
                    gArmyNames[gDwellingType[m_town->m_type][id - 7]]);
            break;
    }
    ShowText(m_statusText);
}

// donor PoL RVA 0x000158e0; preferred Buka symbol ?ShowText@townManager@@QAEXPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.613333;margin=0.109874;shape=0.519;size=1.000;calls=1.000;alternate=pol20:void townManager::ShowText(char *)@0x000158e0
VA(0x0040933a, 0x74)
void townManager::ShowText(char *)
{
    tag_message message;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_STATUS_TEXT_CONTROL;
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0, TOWN_STATUS_TEXT_CONTROL - 2, TOWN_STATUS_TEXT_CONTROL);
    gpWindowManager->UpdateScreenRegion(0, TOWN_STATUS_REGION_Y, TOWN_STATUS_REGION_WIDTH,
                                        TOWN_STATUS_REGION_HEIGHT);
}

// Buka TOWNMGR.cpp Main; HoMM1 opens the castle, mage guild, well and thieves
// guild over a bottom cover window, sells the spell book and builds boats.
VA(0x004093ae, 0x131f)
short townManager::Main(struct tag_message &message)
{
    int exitTown;
    signed char rightClick;
    SAMPLE2 res;
    recruitUnit *recruitMgr;

    exitTown = 0;
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        rightClick = 1;
    else
        rightClick = 0;
    if (KBTickCount() > glTimers[0]) {
        DrawTown(1, 1);
        glTimers[0] = KBTickCount() + TOWN_REDRAW_INTERVAL;
    }
    if ((message.type & m_dispatchMask) == 0) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                            if (rightClick) {
                                QuickViewRecruit(m_town, message.id - 7);
                                break;
                            }
                            gpMouseManager->ReallyHidePointer();
                            DrawTown(1, 1);
                            recruitMgr = new recruitUnit(m_town, message.id - 7);
                            if (recruitMgr == NULL)
                                MemError();
                            gpExec->DoDialog(recruitMgr);
                            delete recruitMgr;
                            break;
                        case 0:
                        case 1:
                        case 4:
                        case 6:
                            if (rightClick)
                                break;
                            gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_SET_FLAGS,
                                                              TOWN_CLOSE_CONTROL, 0x4008);
                            m_coverWindow = new heroWindow(0, 0x100, 0x280, 6, 2);
                            if (m_coverWindow == NULL)
                                MemError();
                            gpWindowManager->AddWindow(m_coverWindow, -1, 1);
                            switch (message.id) {
                                case 6:
                                    gpWindowManager->SaveFizzleSource(0, 0x100, 0x228, 0xcc);
                                    m_heroWindow0 = new heroWindow(0, 0, "caslwind.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetWinText(m_heroWindow0, 2);
                                    SetupCastle(m_heroWindow0);
                                    m_castleDialogActive = 1;
                                    gpWindowManager->DoDialog(m_heroWindow0, CastleHandler, 0);
                                    m_castleDialogActive = 0;
                                    break;
                                case 0:
                                    if (m_town->m_occupyingHeroId != -1
                                        && !gpGame->GetHero(m_town->m_occupyingHeroId)->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                                        if (gpGame->GetHero(m_town->m_occupyingHeroId)->NumArtifacts() == 14)
                                            NormalDialog("You must purchase a spell book to use the mage guild, but "
                                                         "you currently have no room for a spell book.  Try giving "
                                                         "one of your artifacts to another hero.",
                                                         NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                                        else {
                                            m_heroWindow0 = new heroWindow(0xb1, 0x14, "buybook.bin");
                                            if (m_heroWindow0 == NULL)
                                                MemError();
                                            SetWinText(m_heroWindow0, 0);
                                            if (gpGame->m_players[gpGame->GetHero(m_town->m_occupyingHeroId)->m_owner]
                                                    .m_resources[RESOURCE_GOLD]
                                                < 500) {
                                                message.type = MESSAGE_WIDGET;
                                                message.command = WIDGET_COMMAND_SET_FLAGS;
                                                message.id = TOWN_DIALOG_BUTTON_2;
                                                message.value = WIDGET_FLAG_DIMMED;
                                                m_heroWindow0->BroadcastMessage(message);
                                                message.command = WIDGET_COMMAND_CLEAR_FLAGS;
                                                message.value = WIDGET_FLAG_ENABLED;
                                                m_heroWindow0->BroadcastMessage(message);
                                            }
                                            gpWindowManager->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                            if (gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2) {
                                                gpAdvManager->GiveArtifact(gpGame->GetHero(m_town->m_occupyingHeroId),
                                                                           ARTIFACT_MAGIC_BOOK);
                                                gpCurPlayer->m_resources[RESOURCE_GOLD] -= 500;
                                                m_bankBox->Update();
                                                m_townWindow->DrawWindow();
                                                m_town->GiveSpells();
                                            }
                                        }
                                    } else {
                                        m_heroWindow0 = new heroWindow(0, 0, "magewind.bin");
                                        if (m_heroWindow0 == NULL)
                                            MemError();
                                        SetWinText(m_heroWindow0, 6);
                                        SetupMage(m_heroWindow0);
                                        gpWindowManager->DoDialog(m_heroWindow0, MageGuildHandler, 0);
                                    }
                                    m_town->GiveSpells();
                                    break;
                                case 4:
                                    m_heroWindow0 = new heroWindow(0, 0, "wellwind.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetupWell(m_heroWindow0);
                                    gpWindowManager->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                    break;
                                case 1:
                                    m_heroWindow0 = new heroWindow(0, 0, "thiefwin.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetWinText(m_heroWindow0, 0xf);
                                    SetupThievesGuild(m_heroWindow0, -1);
                                    gpWindowManager->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                    break;
                            }
                            delete m_heroWindow0;
                            gpWindowManager->RemoveWindow(m_coverWindow);
                            delete m_coverWindow;
                            if (m_selectedBuilding != -1)
                                BuildObj(m_selectedBuilding);
                            if (m_recruitResult) {
                                hero *theHero;
                                int i;
                                int width;

                                gpMouseManager->ReallyHidePointer();
                                res = NULL_SAMPLE2;
                                res = LoadPlaySample("buildtwn.82M");
                                theHero = gpGame->GetHero(m_town->m_occupyingHeroId);
                                width = 0;
                                for (i = 0; i < 5; i++) {
                                    if (theHero->m_army.m_creatureTypes[i] != CREATURE_NONE)
                                        width = i + 1;
                                }
                                width = width * 88 + 0x70;
                                DrawTown(1, 1);
                                gpWindowManager->FizzleForward(0, 0x100, width, 0xcc, -1);
                                WaitEndSample(res, -1);
                                m_recruitResult = 0;
                                gpMouseManager->ReallyShowPointer();
                            }
                            gpWindowManager->ReleaseFizzleSource();
                            gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_CLEAR_FLAGS,
                                                              TOWN_CLOSE_CONTROL, 0x4008);
                            break;
                        case 2:
                            if (rightClick)
                                break;
                            DoTavern();
                            break;
                        case 5:
                            if (rightClick)
                                return MESSAGE_DISPATCH_CONSUME;
                            if (BuyBuild(6, !CanBuy(m_town, 6), rightClick)) {
                                BuildObj(6);
                                m_town->XformToCastle();
                            }
                            break;
                        case 3:
                            if (rightClick)
                                break;
                            gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_SET_FLAGS,
                                                              TOWN_CLOSE_CONTROL, 0x4008);
                            if (gpGame->GetBoatsBuilt() < 32
                                && gpAdvManager->GetCell(m_town->m_x - 1, m_town->m_y + 1)->m_triggerType == MAP_OBJECT_NONE
                                && m_town->m_x - 1 != gpAdvManager->m_cursorMapX
                                && m_town->m_y + 1 != gpAdvManager->m_cursorMapY) {
                                m_heroWindow0 = new heroWindow(0xb1, 0x14, "shipwind.bin");
                                if (m_heroWindow0 == NULL)
                                    MemError();
                                SetWinText(m_heroWindow0, 0xc);
                                if (gpGame->m_players[giCurPlayer].m_resources[RESOURCE_GOLD] < 1000
                                    || gpGame->m_players[giCurPlayer].m_resources[RESOURCE_WOOD] < 10) {
                                    message.type = MESSAGE_WIDGET;
                                    message.command = WIDGET_COMMAND_SET_FLAGS;
                                    message.id = TOWN_DIALOG_BUTTON_2;
                                    message.value = WIDGET_FLAG_DIMMED;
                                    m_heroWindow0->BroadcastMessage(message);
                                    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
                                    message.value = WIDGET_FLAG_ENABLED;
                                    m_heroWindow0->BroadcastMessage(message);
                                }
                                gpWindowManager->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                delete m_heroWindow0;
                                if (gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2) {
                                    if (gpGame->CreateBoat(m_town->m_x - 1, m_town->m_y + 1) != -1) {
                                        res = NULL_SAMPLE2;
                                        res = LoadPlaySample("buildtwn.82M");
                                        gpGame->m_players[giCurPlayer].m_resources[RESOURCE_GOLD] -= 1000;
                                        gpGame->m_players[giCurPlayer].m_resources[RESOURCE_WOOD] -= 10;
                                        m_bankBox->Update();
                                        WaitEndSample(res, -1);
                                    } else
                                        LogStr("Can't create boat!");
                                }
                            } else
                                NormalDialog("Cannot build another boat.", NORMAL_DIALOG_TYPE_OK, 0xd0, 0x28, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                            gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_CLEAR_FLAGS,
                                                              TOWN_CLOSE_CONTROL, 0x4008);
                            break;
                        case TOWN_CLOSE_CONTROL:
                            if (rightClick)
                                break;
                            exitTown++;
                            break;
                        default:
                            if (rightClick) {
                                int found;
                                hero *viewHero;

                                found = 0;
                                if (message.id >= TOWN_GARRISON_SLOT_FIRST
                                    && message.id <= TOWN_GARRISON_SLOT_FIRST + 4) {
                                    m_selectedStrip = m_garrisonStrip;
                                    m_selectedArmySlot = message.id - TOWN_GARRISON_SLOT_FIRST;
                                    found = 1;
                                }
                                if (message.id >= TOWN_HERO_SLOT_FIRST
                                    && message.id <= TOWN_HERO_SLOT_FIRST + 4) {
                                    m_selectedStrip = m_heroStrip;
                                    m_selectedArmySlot = message.id - TOWN_HERO_SLOT_FIRST;
                                    found = 1;
                                }
                                if (found && m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] != CREATURE_NONE) {
                                    viewHero = m_heroStrip == m_selectedStrip
                                                   ? gpGame->GetHero(m_town->m_occupyingHeroId)
                                                   : NULL;
                                    gpGame->ViewArmy(TOWN_ARMY_VIEW_X, TOWN_ARMY_VIEW_Y,
                                                     m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot],
                                                     m_selectedStrip->m_army->m_creatureCounts[m_selectedArmySlot],
                                                     m_town, 1, 0, 1, viewHero, NULL, m_selectedStrip->m_army);
                                }
                            } else {
                                DoCommand(m_command);
                                SetCommandAndText(message);
                            }
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    if (message.id == m_lastHoverId)
                        return MESSAGE_DISPATCH_CONSUME;
                    else
                        m_lastHoverId = message.id;
                    SetCommandAndText(message);
                    return MESSAGE_DISPATCH_CONSUME;
                    break;
            }
            break;
        case MESSAGE_KEY_UP:
            switch (message.keyCode) {
                case INPUT_SCAN_LEFT_SHIFT:
                case INPUT_SCAN_RIGHT_SHIFT:
                    ShiftQualChange();
                    break;
                default:
                    break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_LEFT_SHIFT:
                case INPUT_SCAN_RIGHT_SHIFT:
                    ShiftQualChange();
                    break;
                case INPUT_SCAN_ESCAPE:
                    exitTown++;
                    break;
            }
            break;
    }
    if (exitTown == 1) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka TOWNMGR.cpp:1817-1902; HoMM1 merges duplicate stacks after a swap
// and opens the kingdom overview from the town.
VA(0x0040a6cd, 0x65f)
void townManager::DoCommand(signed char command)
{
    hero *visitor;
    int temp;
    short i;
    hero *viewedHero;
    int single;

    switch (command) {
        case TOWN_ARMY_COMMAND_SELECT:
            m_swapStrip = m_selectedStrip;
            m_swapArmySlot = m_selectedArmySlot;
            m_swapStrip->m_selectedSlot = m_swapArmySlot;
            m_swapStrip->Draw();
            break;
        case TOWN_ARMY_COMMAND_VIEW:
            viewedHero =
                m_heroStrip == m_selectedStrip ? gpGame->GetHero(m_town->m_occupyingHeroId) : NULL;
            if (m_castleDialogActive == 1
                || (m_heroStrip == m_selectedStrip && m_selectedStrip->m_army->GetNumArmies() == 1))
                single = 1;
            else
                single = 0;
            gpGame->ViewArmy(TOWN_ARMY_VIEW_X, TOWN_ARMY_VIEW_Y,
                             m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot],
                             m_selectedStrip->m_army->m_creatureCounts[m_selectedArmySlot], m_town, single,
                             0, 0, viewedHero, NULL, m_selectedStrip->m_army);
            if (gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2) {
                m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] = CREATURE_NONE;
                m_selectedStrip->m_army->m_creatureCounts[m_selectedArmySlot] = 0;
            }
            ResetStrips();
            break;
        case TOWN_ARMY_COMMAND_MERGE:
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                    == m_pendingStrip->m_army->m_creatureTypes[i])
                    break;
            }
            if (i < ARMY_GROUP_SLOT_COUNT)
                m_pendingArmySlot = i;
            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] +=
                m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
            m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot] = CREATURE_NONE;
            m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] = 0;
            ResetStrips();
            break;
        case TOWN_ARMY_COMMAND_SWAP:
            temp = m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot];
            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
            m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] = temp;
            temp = m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot];
            m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot];
            m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot] = temp;
            if (m_swapStrip != m_pendingStrip) {
                for (temp = 0; temp < ARMY_GROUP_SLOT_COUNT; temp++) {
                    if (m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]
                            == m_pendingStrip->m_army->m_creatureTypes[temp]
                        && m_pendingArmySlot != temp) {
                        m_pendingStrip->m_army->m_creatureCounts[temp] +=
                            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot];
                        m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] = CREATURE_NONE;
                        m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] = 0;
                    }
                    if (m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                            == m_swapStrip->m_army->m_creatureTypes[temp]
                        && m_swapArmySlot != temp) {
                        m_swapStrip->m_army->m_creatureCounts[temp] +=
                            m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
                        m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot] = CREATURE_NONE;
                        m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] = 0;
                    }
                }
            }
            ResetStrips();
            break;
        case TOWN_ARMY_COMMAND_VIEW_HERO:
            visitor = gpGame->GetHero(m_town->m_occupyingHeroId);
            visitor->HeroView(1);
            RedrawTownScreen();
            gpWindowManager->FadeScreen(0, 8, NULL);
            break;
        case TOWN_ARMY_COMMAND_GARRISON:
            gpGame->Overview();
            RedrawTownScreen();
            gpWindowManager->FadeScreen(0, 8, NULL);
            break;
        case TOWN_ARMY_COMMAND_SPLIT:
            SplitArmy();
            ResetStrips();
            break;
    }
    m_lastHoverId = -1;
}

// Buka TOWNMGR.cpp:1905-1921; HoMM1 redraws strips before the status text.
VA(0x0040ad2c, 0xa5)
void townManager::RedrawTownScreen(void)
{
    tag_message message;

    DrawTown(1, 1);
    m_garrisonStrip->DrawIcons(1);
    m_heroStrip->DrawIcons(1);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_STATUS_TEXT_CONTROL;
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0);
    gpWindowManager->UpdateScreenRegion(0, 0x100, 0x280, 0x1e0);
    m_bankBox->Update();
}

// donor PoL RVA 0x0001771d; preferred Buka symbol ?SplitArmy@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.732616;margin=0.021648;shape=0.430;size=0.991;calls=1.000;strings=splitwin.bin;alternate=pol20:void townManager::SplitArmy(void)@0x0001771d
// Buka TOWNMGR.cpp:1923-1970; HoMM1 always names both armies and merges
// into the first matching slot of the target army.
VA(0x0040add1, 0x37e)
void townManager::SplitArmy(void)
{
    short messageId = 1;
    tag_message message;
    short merge;
    short numberId = TOWN_SPLIT_SETUP_AMOUNT_CONTROL;
    short n;

    m_heroWindow1 = new heroWindow(TOWN_SPLIT_WINDOW_X, TOWN_SPLIT_WINDOW_Y, "splitwin.bin");
    if (m_heroWindow1 == NULL)
        MemError();
    m_splitAmount = 0;
    m_splitMaximum = m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
    message.type = MESSAGE_WIDGET;
    sprintf(gText, "Move how many %s troops from %s to %s?",
            gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
            m_swapStrip == m_heroStrip ? "Hero's Army" : "Garrison",
            m_pendingStrip == m_heroStrip ? "Hero's Army" : "Garrison");
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "%d", m_splitAmount);
    message.id = TOWN_SPLIT_SETUP_AMOUNT_CONTROL;
    message.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    gpWindowManager->DoDialog(m_heroWindow1, SplitArmyHandler, 0);
    delete m_heroWindow1;
    if (gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2) {
        merge = 0;
        for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
            if (m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                == m_pendingStrip->m_army->m_creatureTypes[n]) {
                merge = 1;
                break;
            }
        }
        if (merge)
            m_pendingStrip->m_army->m_creatureCounts[n] += m_splitAmount;
        else {
            m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot];
            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] = m_splitAmount;
        }
        m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] -= m_splitAmount;
    }
}

// HoMM1 re-evaluates the pending strip command when the shift qualifier
// changes, then refreshes the status line.
VA(0x0040b14f, 0xce)
void townManager::ShiftQualChange(void)
{
    tag_message message;

    if (m_swapStrip != m_pendingStrip
        && (m_command == TOWN_ARMY_COMMAND_NONE || m_command == TOWN_ARMY_COMMAND_SPLIT
            || m_command == TOWN_ARMY_COMMAND_MERGE || m_command == TOWN_ARMY_COMMAND_SWAP))
        SetArmyCommand(gpInputManager->GetModifiers() & TOWN_SHIFT_QUALIFIER_MASK);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_STATUS_TEXT_CONTROL;
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow();
}

// donor PoL RVA 0x00017ab2; preferred Buka symbol ?ResetStrips@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.515580;margin=0.398432;shape=0.375;size=0.860;calls=1.000;alternate=pol20:void townManager::ResetStrips(void)@0x00017ab2
VA(0x0040b21d, 0xab)
void townManager::ResetStrips(void)
{
    if (m_swapStrip)
        m_swapStrip->m_selectedSlot = -1;
    if (m_pendingStrip)
        m_pendingStrip->m_selectedSlot = -1;
    m_heroStrip->Draw();
    m_garrisonStrip->Draw();
    m_swapStrip = m_pendingStrip = NULL;
    m_swapArmySlot = m_pendingArmySlot = -1;
}

// Buka TOWNMGR.cpp:1993-2003.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0040b2c8, 0x95)
void townManager::Toggle(signed char building)
{
    short index;

    if (m_town->m_buildings & (1 << building)) {
        for (index = 0; index < m_townObjectCount; index++) {
            if (m_townObjects[index]->m_buildingId == building)
                m_townObjects[index]->m_visible ^= 1;
        }
    }
}

// Buka TOWNMGR.cpp:2005-2029; HoMM1 draws a bitmap background and folds
// the mouse pointer into the screen buffer around the viewport blit.
VA(0x0040b35d, 0xf8)
void townManager::DrawTown(signed char updateScreen, int drawFlags)
{
    short index;
    short x;
    short y;

    m_backgroundBitmap->DrawToBuffer(0, 0);
    for (index = 0; index < m_townObjectCount; index++)
        m_townObjects[index]->Draw(drawFlags);
    m_townWindow->DrawWindow(0, TOWN_REDRAW_FIRST_CONTROL, TOWN_REDRAW_LAST_CONTROL);
    gpMouseManager->MouseCoords(x, y);
    if (y < TOWN_VIEWPORT_HEIGHT)
        gpMouseManager->SaveAndDraw(gpWindowManager->m_screen, 0, 0, 1);
    if (updateScreen)
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, TOWN_VIEWPORT_WIDTH,
                           TOWN_VIEWPORT_HEIGHT, 0, 0);
    if (y < TOWN_VIEWPORT_HEIGHT)
        gpMouseManager->RestoreUnderlying();
}

// Buka TOWNMGR.cpp BuyBuild; HoMM1 reads the mage/neutral/dwelling cost
// tables with asserts, sizes resource slots by the gold-icon width and
// draws the building through the castle frame of buybuil%d.bin.
VA(0x0040b455, 0x1023)
short townManager::BuyBuild(short building, signed char cannotBuy, signed char quickView)
{
    unsigned short requirements;
    int yPos;
    int resIndex;
    char *descText;
    textWidget *amountWidgets[7];
    int nRowTypes[4];
    int row;
    short currX;
    short unusedTop;
    int totalWidth;
    int numLines;
    heroWindow *nBuildWindow;
    short unusedValue;
    short unusedField;
    short unusedControl;
    short firstRow;
    int space;
    tag_message iEvt;
    signed char resType[7];
    int binSize;
    int dwellIndex;
    iconWidget *resWidgets[7];
    short startX;
    font *iF;
    short unusedType;
    short nBottomCount;
    int inRow;
    short pResourceCount;
    int j;
    textWidget *descWidget;
    int iTotalHeight;
    short prices[7];
    short unusedKind;
    int nEntryWidth;
    int curCost;
    int numPrereqs;
    short unusedMode1;
    int iMageLevel;
    char *amountText[7];
    int baseY;

    iMageLevel = 0;
    j = 0;
    curCost = 0;
    descText = static_cast<char*>(malloc(300));
    for (j = 0; j < 7; j++)
        resType[j] = prices[j] = -1;
    dwellIndex = -1;
    if (building > 6)
        dwellIndex = building - BUILDING_SLOT_DWELLING_FIRST + m_town->m_type * 6;
    if (building == 0) {
        if (m_town->m_buildings & 1)
            iMageLevel = gpTownManager->m_town->m_buildState + 1;
        else
            iMageLevel = 0;
        if (iMageLevel > 3)
            iMageLevel = 3;
        for (j = 0; j < 7; j++) {
            if (gMageBuildingCosts[iMageLevel][j] > 0) {
                resType[curCost] = j;
                prices[curCost] = gMageBuildingCosts[iMageLevel][j];
                curCost++;
            }
        }
    } else if (building <= 6) {
        for (j = 0; j < 7; j++) {
            ProcessAssert(building >= 0 && building < 7, "D:\\Heroes\\Source\\TOWNMGR.CPP",
                          gTownMgrAssertLine + 42);
            ProcessAssert(j >= 0 && j <= 6, "D:\\Heroes\\Source\\TOWNMGR.CPP", gTownMgrAssertLine + 43);
            if (gNeutralBuildingCosts[building][j] > 0) {
                resType[curCost] = j;
                prices[curCost] = gNeutralBuildingCosts[building][j];
                curCost++;
            }
        }
    } else {
        for (j = 0; j < 7; j++) {
            ProcessAssert(dwellIndex >= 0 && dwellIndex < 28, "D:\\Heroes\\Source\\TOWNMGR.CPP",
                          gTownMgrAssertLine + 57);
            ProcessAssert(j >= 0 && j <= 6, "D:\\Heroes\\Source\\TOWNMGR.CPP", gTownMgrAssertLine + 58);
            LogStr("DwellCost", gDwellingCosts[dwellIndex][j], dwellIndex, j, 0, 0);
            if (gDwellingCosts[dwellIndex][j] > 0) {
                resType[curCost] = j;
                prices[curCost] = gDwellingCosts[dwellIndex][j];
                curCost++;
            }
        }
    }
    unusedKind = 80;
    unusedValue = 40;
    unusedMode1 = 32;
    unusedControl = 286;
    unusedTop = 0;
    unusedField = 2;
    unusedType = 3;
    resIndex = 0;
    pResourceCount = 0;
    firstRow = 0;
    nBottomCount = 0;
    for (j = 0; j < 7; j++) {
        if (resType[j] != -1)
            pResourceCount++;
    }
    if (pResourceCount <= 4)
        firstRow = pResourceCount;
    else if (pResourceCount == 5) {
        firstRow = 2;
        nBottomCount = 3;
    } else if (pResourceCount == 6) {
        firstRow = 3;
        nBottomCount = 3;
    } else if (pResourceCount == 7) {
        firstRow = 3;
        nBottomCount = 4;
    }
    if (building <= 6)
        sprintf(descText, gNeutralBuildingDescriptions[building]);
    else
        sprintf(descText, gDwellingDescriptions[dwellIndex]);
    if (dwellIndex >= 0) {
        numPrereqs = 0;
        requirements = gDwellingRequirements[building - BUILDING_SLOT_DWELLING_FIRST + m_town->m_type * 6];
        for (j = 0; j < 12; j++) {
            if (requirements & (1 << j)) {
                if (numPrereqs == 0)
                    strcat(descText, "\n\nRequires:");
                numPrereqs++;
                strcat(descText, "\n");
                if (j <= 6)
                    strcat(descText, gNeutralBuildingNames[j]);
                else
                    strcat(descText, gDwellingNames[j - BUILDING_SLOT_DWELLING_FIRST + m_town->m_type * 6]);
            }
        }
    }
    strcat(descText, "\n ");
    iF = gpResourceManager->GetFont("bigfont.fnt");
    numLines = iF->LineLength(descText, 0xee);
    gpResourceManager->Dispose(iF);
    baseY = 0x97;
    iTotalHeight = baseY;
    iTotalHeight += numLines << 4;
    if (pResourceCount <= 4)
        iTotalHeight += 0x2c;
    else
        iTotalHeight += 0x58;
    if (!quickView)
        iTotalHeight += 0x27;
    binSize = (iTotalHeight - 0x35) / 0x2d;
    if (binSize < 3)
        binSize = 3;
    if (binSize > 7)
        binSize = 7;
    sprintf(gText, "buybuil%d.bin", binSize);
    nBuildWindow = new heroWindow(0xb1, 0x10, gText);
    if (nBuildWindow == 0)
        MemError();
    SetWinText(nBuildWindow, 1);
    iEvt.type = MESSAGE_WIDGET;
    iEvt.command = WIDGET_COMMAND_SET_FRAME;
    iEvt.id = 2;
    if (building >= 5)
        iEvt.value = (gpTownManager->m_town->m_type + 1) * 7 + building - 6;
    else
        iEvt.value = building + 1;
    nBuildWindow->BroadcastMessage(iEvt);
    if (building == 0)
        sprintf(gText, "Mage Guild, Level %d", iMageLevel + 1);
    else
        strcpy(gText, GetBuildingName(building));
    iEvt.command = WIDGET_COMMAND_SET_TEXT;
    iEvt.id = 3;
    iEvt.text = gText;
    nBuildWindow->BroadcastMessage(iEvt);
    descWidget = new textWidget(0x18, baseY, 0xee, (numLines << 4) + 6, descText, "bigfont.fnt", 1, -1, 8);
    if (descWidget == 0)
        MemError();
    nBuildWindow->AddWidget(descWidget, -1);
    resIndex = 0;
    for (row = 0; row < 2; row++) {
        yPos = numLines * 16 + baseY + row * 44 + 12;
        if (row == 0)
            inRow = firstRow;
        else
            inRow = nBottomCount;
        if (inRow > 0) {
            totalWidth = 0;
            curCost = resIndex;
            for (j = 0; j < 4; j++) {
                if (j < inRow) {
                    while (resType[curCost] == -1)
                        curCost++;
                    nRowTypes[j] = resType[curCost];
                    curCost++;
                } else
                    nRowTypes[j] = -1;
            }
            for (j = 0; j < inRow; j++) {
                if (nRowTypes[j] == RESOURCE_GOLD)
                    totalWidth += 80;
                else
                    totalWidth += 40;
            }
            space = (266 - totalWidth) / (inRow + 1);
            currX = startX = space + 10;
            for (j = 0; j < inRow; j++) {
                if (nRowTypes[j] == RESOURCE_GOLD)
                    nEntryWidth = 80;
                else
                    nEntryWidth = 40;
                amountText[resIndex] = static_cast<char*>(malloc(10));
                sprintf(amountText[resIndex], "%d", prices[resIndex]);
                amountWidgets[resIndex] = new textWidget(currX, yPos + 32, nEntryWidth, 12, amountText[resIndex],
                                                         "smalfont.fnt", 1, -1, 8);
                if (amountWidgets[resIndex] == 0)
                    MemError();
                resWidgets[resIndex] = new iconWidget(currX, yPos, nEntryWidth, 12, "resource.icn",
                                                      resType[resIndex], ICON_DRAW_NORMAL, -1, ICON_WIDGET_DRAW, 1);
                if (resWidgets[resIndex] == 0)
                    MemError();
                nBuildWindow->AddWidget(amountWidgets[resIndex], -1);
                nBuildWindow->AddWidget(resWidgets[resIndex], -1);
                resIndex++;
                currX = currX + space + nEntryWidth;
            }
        }
    }
    if (!quickView)
        gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_SET_FLAGS, TOWN_CLOSE_CONTROL, 0x4008);
    m_selectedBuilding = -1;
    if (quickView) {
        iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
        iEvt.value = 6;
        iEvt.id = TOWN_DIALOG_BUTTON_2;
        nBuildWindow->BroadcastMessage(iEvt);
        iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
        iEvt.value = 6;
        iEvt.id = TOWN_DIALOG_BUTTON_1;
        nBuildWindow->BroadcastMessage(iEvt);
        iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
        iEvt.value = 6;
        iEvt.id = 0;
        nBuildWindow->BroadcastMessage(iEvt);
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(nBuildWindow, -1, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(nBuildWindow);
        gpMouseManager->ReallyShowPointer();
    } else {
        if (cannotBuy) {
            iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
            iEvt.id = TOWN_DIALOG_BUTTON_2;
            iEvt.value = 2;
            nBuildWindow->BroadcastMessage(iEvt);
            iEvt.command = WIDGET_COMMAND_SET_FLAGS;
            iEvt.id = TOWN_DIALOG_BUTTON_2;
            iEvt.value = WIDGET_COMMAND_DIMMED;
            nBuildWindow->BroadcastMessage(iEvt);
        }
        gpWindowManager->DoDialog(nBuildWindow, TrueFalseDialogHandler, 0);
        if (gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2) {
            m_selectedBuilding = building;
            for (j = 0; j < pResourceCount; j++)
                gpCurPlayer->m_resources[resType[j]] -= prices[j];
        }
    }
    if (!quickView)
        gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_CLEAR_FLAGS, TOWN_CLOSE_CONTROL, 0x4008);
    delete nBuildWindow;
    if (quickView)
        return 0;
    else
        return gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2;
}

// donor PoL RVA 0x00018bd2; preferred Buka symbol ?BuildObj@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.630156;margin=0.342681;shape=0.206;size=0.999;calls=0.933;strings=buildtwn.82M;alternate=pol20:void townManager::BuildObj(int)@0x00018bd2
// Buka TOWNMGR.cpp:2475; HoMM1 fizzles a fixed per-building rectangle
// instead of computing the drawn extent.
VA(0x0040c478, 0x3a0)
void townManager::BuildObj(short building)
{
    short i;
    SAMPLE2 sample;

    gpMouseManager->ReallyHidePointer();
    DrawTown(1, 1);
    if (building == TOWN_BUILDING_MAGE_GUILD) {
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            m_town->m_buildState++;
        if (m_town->m_occupyingHeroId != -1)
            m_town->GiveSpells();
    }
    m_town->m_buildings |= 1 << building;
    if (building >= TOWN_BUILDING_FIRST_DWELLING && building <= TOWN_BUILDING_LAST_DWELLING)
        m_town->m_garrison[building - TOWN_BUILDING_FIRST_DWELLING] =
            gMonsterDatabase[gDwellingType[m_town->m_type][building - TOWN_BUILDING_FIRST_DWELLING]]
                .growth;
    for (i = 0; i < m_townObjectCount; i++) {
        if (m_townObjects[i]->m_buildingId == building) {
            m_townObjects[i]->m_visible = 1;
            m_townObjects[i]->m_border->m_flags |= TOWN_OBJECT_ENABLED_FLAG;
        }
    }
    if (building == TOWN_BUILDING_CASTLE) {
        m_town->m_buildings &= ~(1 << TOWN_BUILDING_TENT);
        for (i = 0; i < m_townObjectCount; i++) {
            if (m_townObjects[i]->m_buildingId == TOWN_BUILDING_TENT) {
                m_townObjects[i]->m_visible = 0;
                m_townObjects[i]->m_border->m_flags &= ~TOWN_OBJECT_ENABLED_FLAG;
            }
        }
    }
    gpWindowManager->SaveFizzleSource(gTownBuildingExtents[m_town->m_type][building].x,
                                      gTownBuildingExtents[m_town->m_type][building].y,
                                      gTownBuildingExtents[m_town->m_type][building].width,
                                      gTownBuildingExtents[m_town->m_type][building].height);
    DrawTown(0, 1);
    sample = NULL_SAMPLE2;
    sample = LoadPlaySample("buildtwn.82M");
    gpWindowManager->FizzleForward(gTownBuildingExtents[m_town->m_type][building].x,
                                   gTownBuildingExtents[m_town->m_type][building].y,
                                   gTownBuildingExtents[m_town->m_type][building].width,
                                   gTownBuildingExtents[m_town->m_type][building].height, -1);
    WaitEndSample(sample, -1);
    m_selectedBuilding = -1;
    m_bankBox->Update();
    m_townWindow->DrawWindow();
    gpMouseManager->ReallyShowPointer();
    gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_CLEAR_FLAGS, TOWN_DIALOG_BUTTON_0,
                                      0x4008);
    BitSet(gpGame->m_townBuiltToday, m_town->m_id);
    m_town->GiveSpells();
}

// Buka Castle.cpp SetupCastle; HoMM1 lays out five special buildings and
// six dwellings plus the hero-recruit slot with fixed frames.
VA(0x0040c818, 0x4b5)
void townManager::SetupCastle(class heroWindow *window)
{
    short builtIcon = TOWN_CASTLE_FRAME_BUILT;
    short cannotBuild = TOWN_CASTLE_FRAME_CANNOT_BUILD;
    short noMoney = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    short i;
    tag_message message;
    int stateFrame;

    m_affordableBuildings = m_buildableBuildings = 0;
    for (i = 0; i < TOWN_CASTLE_BUILDING_COUNT; i++) {
        if (CanBuy(m_town, i))
            m_affordableBuildings |= 1 << i;
        if (CanBuild(m_town, i))
            m_buildableBuildings |= 1 << i;
    }
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        message.value =
            (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.id = i + TOWN_CASTLE_FIRST_DWELLING_NAME_CONTROL;
        message.text = GetBuildingName(i + TOWN_BUILDING_FIRST_DWELLING);
        window->BroadcastMessage(message);
    }
    for (i = 0; i < TOWN_CASTLE_SPECIAL_BUILDING_COUNT; i++) {
        stateFrame = -1;
        if ((m_town->m_buildings & (1 << i)) && (i != 0 || m_town->m_buildState == 3))
            stateFrame = TOWN_CASTLE_FRAME_BUILT;
        else if (!(m_buildableBuildings & (1 << i)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
        else if (!(m_affordableBuildings & (1 << i)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
        if (stateFrame != -1) {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_STATE_CONTROL;
            message.value = TOWN_WIDGET_VISIBLE_FLAG;
            window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = stateFrame;
            window->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_STATE_CONTROL;
            message.value = TOWN_WIDGET_VISIBLE_FLAG;
            window->BroadcastMessage(message);
        }
    }
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        stateFrame = -1;
        if (m_town->m_buildings & (1 << (i + TOWN_BUILDING_FIRST_DWELLING)))
            stateFrame = TOWN_CASTLE_FRAME_BUILT;
        else if (!(m_buildableBuildings & (1 << (i + TOWN_BUILDING_FIRST_DWELLING))))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
        else if (!(m_affordableBuildings & (1 << (i + TOWN_BUILDING_FIRST_DWELLING))))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
        if (stateFrame != -1) {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            message.value = TOWN_WIDGET_VISIBLE_FLAG;
            window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = stateFrame;
            window->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            message.value = TOWN_WIDGET_VISIBLE_FLAG;
            window->BroadcastMessage(message);
        }
    }
    if (gpCurPlayer->m_resources[RESOURCE_GOLD] < gHeroGoldCost)
        stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    else if (gpCurPlayer->m_heroCount == PLAYER_HERO_CAPACITY || m_town->m_occupyingHeroId != -1)
        stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
    else if (m_recruitResult)
        stateFrame = TOWN_CASTLE_FRAME_BUILT;
    else
        stateFrame = -1;
    message.id = TOWN_CASTLE_HERO_STATE_CONTROL;
    message.value = TOWN_WIDGET_VISIBLE_FLAG;
    if (stateFrame != -1) {
        message.command = WIDGET_COMMAND_SET_FLAGS;
        window->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = stateFrame;
        window->BroadcastMessage(message);
    } else {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        window->BroadcastMessage(message);
    }
}

// Buka TOWNMGR.cpp:3131 SetupWell; HoMM1 has six fixed dwellings and
// capitalises the creature name in gText.
VA(0x0040cccd, 0x24d)
void townManager::SetupWell(class heroWindow *window)
{
    short iconBase = 1;
    short buildingName = 7;
    short growthRate;
    short firstMonsterIcon = 0xd;
    short creatureId = 0x13;
    short firstAvailable = 0x19;
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_ICON_CONTROL;
        message.value =
            (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(message);
        message.id = i + TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
        message.value = gDwellingType[m_town->m_type][i];
        window->BroadcastMessage(message);
    }
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        message.text = GetBuildingName(i + TOWN_WELL_FIRST_DWELLING_BUILDING);
        window->BroadcastMessage(message);
        message.id = i + TOWN_WELL_FIRST_CREATURE_CONTROL;
        strcpy(gText, gArmyNames[gDwellingType[m_town->m_type][i]]);
        gText[0] -= 'a' - 'A';
        message.text = gText;
        window->BroadcastMessage(message);
    }
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_AVAILABLE_CONTROL;
        if (!(m_town->m_buildings & (1 << (i + TOWN_WELL_FIRST_DWELLING_BUILDING))))
            strcpy(gText, "Available:\nNONE\nGrowth Rate:\nN/A");
        else {
            growthRate = gMonsterDatabase[gDwellingType[m_town->m_type][i]].growth;
            growthRate += 2;
            sprintf(gText, "Available:\n%d\nGrowth Rate:\n%d/week", m_town->m_garrison[i],
                    growthRate);
        }
        message.text = gText;
        window->BroadcastMessage(message);
    }
}

// Buka TOWNMGR.cpp:2597 SetupMage; HoMM1 shows nine guild spells, hiding
// the levels above the guild and stacking tower frames by level.
VA(0x0040cf1a, 0x331)
void townManager::SetupMage(class heroWindow *window)
{
    short off = 0;
    short shown = 1;
    short iconFrame = 2;
    short messageId = TOWN_MAGE_DESCRIPTION_CONTROL;
    short slotBase = TOWN_MAGE_FIRST_SPELL_CONTROL;
    short iconOffset = TOWN_MAGE_FIRST_ICON_CONTROL;
    short nameBase = TOWN_MAGE_FIRST_NAME_CONTROL;
    short firstTower = TOWN_MAGE_FIRST_TOWER_CONTROL;
    tag_message message;
    short spellIndex;
    int spellState;

    message.type = MESSAGE_WIDGET;
    if (m_town->m_occupyingHeroId == -1) {
        strcpy(gText, "The above spells are available here.");
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.id = TOWN_MAGE_DESCRIPTION_CONTROL;
        message.text = gText;
        window->BroadcastMessage(message);
    }
    for (spellIndex = 0; spellIndex < TOWN_MAGE_SPELL_COUNT; spellIndex++) {
        switch (spellIndex) {
            case 0:
            case 1:
            case 2:
                if (m_town->m_buildState >= 0)
                    spellState = 0;
                else
                    spellState = 1;
                break;
            case 3:
            case 4:
                if (m_town->m_buildState >= 1)
                    spellState = 0;
                else
                    spellState = 1;
                break;
            case 5:
            case 6:
                if (m_town->m_buildState >= 2)
                    spellState = 0;
                else
                    spellState = 1;
                break;
            case 8:
                ;
            default:
                if (m_town->m_buildState >= 3)
                    spellState = 0;
                else
                    spellState = 1;
                break;
        }
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = spellIndex + TOWN_MAGE_FIRST_SPELL_CONTROL;
        message.value = spellState;
        window->BroadcastMessage(message);
        if (spellState == 1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = TOWN_MAGE_WIDGET_VISIBLE_FLAG;
            message.id = spellIndex + TOWN_MAGE_FIRST_ICON_CONTROL;
            window->BroadcastMessage(message);
            message.id = spellIndex + TOWN_MAGE_FIRST_NAME_CONTROL;
            window->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = spellIndex + TOWN_MAGE_FIRST_ICON_CONTROL;
            message.value = m_town->m_mageGuildSpells[spellIndex];
            window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = spellIndex + TOWN_MAGE_FIRST_NAME_CONTROL;
            message.text = gSpellNames[m_town->m_mageGuildSpells[spellIndex]];
            window->BroadcastMessage(message);
        }
    }
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = TOWN_MAGE_WIDGET_VISIBLE_FLAG;
    for (spellIndex = 1; spellIndex < TOWN_MAGE_TOWER_FRAME_COUNT; spellIndex++) {
        message.id = spellIndex + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.value = TOWN_MAGE_WIDGET_VISIBLE_FLAG;
    for (spellIndex = 0; spellIndex < m_town->m_buildState; spellIndex++) {
        message.id = (spellIndex + 1) * 2 + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.id = m_town->m_buildState * 2 + TOWN_MAGE_FIRST_TOWER_CONTROL + 1;
    window->BroadcastMessage(message);
}

// Buka TOWNMGR.cpp:2735 MageGuildHandler; HoMM1 numbers spells 1-9 and
// icons 10-18 and bounds them by the guild level.
VA(0x0040d24b, 0x186)
short MageGuildHandler(struct tag_message &message)
{
    short firstSpell = TOWN_MAGE_FIRST_SPELL_CONTROL;
    short iconBase = TOWN_MAGE_FIRST_ICON_CONTROL;
    int quickView;
    int spellId;
    int mageLevel;
    int spellPos;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                quickView = message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON;
                switch (message.id) {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                        spellPos = message.id - TOWN_MAGE_FIRST_SPELL_CONTROL;
                        goto showSpell;
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                        spellPos = message.id - TOWN_MAGE_FIRST_ICON_CONTROL;
                    showSpell:
                        mageLevel = gpTownManager->m_town->m_buildState;
                        if ((mageLevel == 0 && spellPos > 2) || (mageLevel == 1 && spellPos > 4)
                            || (mageLevel == 2 && spellPos > 6))
                            return MESSAGE_DISPATCH_CONSUME;
                        spellId = gpTownManager->m_town->m_mageGuildSpells[spellPos];
                        NormalDialog(gSpellDesc[spellId], quickView ? NORMAL_DIALOG_TYPE_QUICK_VIEW : NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_SPELL, spellId, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                        return MESSAGE_DISPATCH_CONSUME;
                }
        }
    }
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0001a783; preferred Buka symbol ?SetupThievesGuild@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.350822;margin=0.362412;shape=0.263;size=0.210;calls=0.163;strings=townwind.icn;alternate=pol20:void townManager::SetupThievesGuild(class heroWindow *, int)@0x0001a783
// Buka TOWNMGR.cpp:3328 SetupThievesGuild; HoMM1 only draws the ranking
// flags, with the category count taken from the number of guilds owned.
VA(0x0040d3d1, 0x2ec)
void townManager::SetupThievesGuild(class heroWindow *window, short categories)
{
    iconWidget *marker;
    short firstPlayer;
    int numThieves;
    short wUnusedRankX = THIEVES_RANK_FIRST_X;
    short iUnusedRankWidth = THIEVES_PLAYER_COLUMN_WIDTH;
    short top = THIEVES_FIRST_CATEGORY_Y;
    short rowSpacing = THIEVES_CATEGORY_ROW_HEIGHT;
    short frameBase = THIEVES_FLAG_FRAME_BASE;
    short pos;
    short lMarkWidth = THIEVES_RANK_ICON_WIDTH;
    short bIconHeight = THIEVES_RANK_ICON_HEIGHT;
    short bColWidth = THIEVES_PLAYER_WIDTH;
    signed char ranking[GAME_PLAYER_COUNT];
    short rank;
    short categoryIndex;
    long totals[GAME_PLAYER_COUNT];
    short startPos;
    short hi;
    short tied;

    if (categories == -1) {
        numThieves = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (numThieves >= 4)
            categories = 8;
        else if (numThieves == 3)
            categories = 7;
        else if (numThieves == 2)
            categories = 5;
        else
            categories = 3;
    }
    if (categories > THIEVES_CATEGORY_COUNT)
        categories = THIEVES_CATEGORY_COUNT;
    for (categoryIndex = 0; categoryIndex < categories; categoryIndex++) {
        GetCategoryStats(categoryIndex, totals, ranking);
        SortStats(totals, ranking);
        firstPlayer = 0;
        hi = 0;
        for (rank = 0; rank < THIEVES_RANK_COUNT; rank++) {
            if (firstPlayer == gpGame->m_playerCount - gpGame->m_deadPlayerCount)
                break;
            tied = 1;
            while (hi + 1 < gpGame->m_playerCount
                   && totals[hi + 1] == totals[hi]) {
                tied++;
                hi++;
            }
            startPos = rank * THIEVES_PLAYER_COLUMN_WIDTH + THIEVES_RANK_FIRST_X
                    - (tied - 1) * THIEVES_TIE_CENTERING_STEP;
            for (pos = firstPlayer; !(pos > hi); pos++) {
                marker = new iconWidget(
                    (pos - firstPlayer) * THIEVES_RANK_ICON_WIDTH + startPos,
                    categoryIndex * THIEVES_CATEGORY_ROW_HEIGHT + THIEVES_FIRST_CATEGORY_Y,
                    THIEVES_RANK_ICON_WIDTH, THIEVES_RANK_ICON_HEIGHT, "townwind.icn",
                    gpGame->m_players[ranking[pos]].m_color + THIEVES_FLAG_FRAME_BASE,
                    ICON_DRAW_NORMAL, -1, ICON_WIDGET_DRAW, 1);
                if (marker == NULL)
                    MemError();
                window->AddWidget(marker, -1);
            }
            hi++;
            firstPlayer = hi;
        }
    }
}

// Buka TOWNMGR.cpp:3727-3833; HoMM1 has eight categories, sums three
// resources per row and counts obelisks through playerData.
VA(0x0040d6bd, 0x484)
void townManager::GetCategoryStats(signed char category, long *const stats,
                                   signed char *const order)
{
    short townIndex;
    short index;
    long strength;
    short player;
    short numTowns;
    short numCastles;
    hero *playerHero;
    town *theTown;

    for (player = 0; player < gpGame->m_playerCount; player++) {
        numTowns = 0;
        numCastles = 0;
        order[player] = player;
        if (gpGame->m_playerDead[player]) {
            stats[player] = TOWN_THIEVES_DEAD_PLAYER_STAT;
        } else {
            switch (category) {
                case THIEVES_CATEGORY_TOWNS:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gpGame->m_castleRecs[townIndex].m_owner == player
                            && (gpGame->m_castleRecs[townIndex].m_buildings & TOWN_BUILDING_TENT_FLAG))
                            numTowns++;
                    }
                    stats[player] = numTowns;
                    break;
                case THIEVES_CATEGORY_CASTLES:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gpGame->m_castleRecs[townIndex].m_owner == player
                            && (gpGame->m_castleRecs[townIndex].m_buildings & TOWN_BUILDING_CASTLE_FLAG))
                            numCastles++;
                    }
                    stats[player] = numCastles;
                    break;
                case THIEVES_CATEGORY_HEROES:
                    stats[player] = gpGame->m_players[player].m_heroCount;
                    break;
                case THIEVES_CATEGORY_GOLD:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_GOLD];
                    break;
                case THIEVES_CATEGORY_WOOD_AND_ORE:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_ORE]
                                    + gpGame->m_players[player].m_resources[RESOURCE_CRYSTAL]
                                    + gpGame->m_players[player].m_resources[RESOURCE_WOOD];
                    break;
                case THIEVES_CATEGORY_RARE_RESOURCES:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_MERCURY]
                                    + gpGame->m_players[player].m_resources[RESOURCE_SULFUR]
                                    + gpGame->m_players[player].m_resources[RESOURCE_GEMS];
                    break;
                case THIEVES_CATEGORY_OBELISKS:
                    stats[player] = gpGame->m_players[player].CountVisitedObelisks();
                    break;
                case THIEVES_CATEGORY_ARMY_STRENGTH:
                    strength = 0;
                    for (index = 0; index < gpGame->m_players[player].m_heroCount;
                         index++) {
                        playerHero = gpGame->GetHero(gpGame->m_players[player].m_heroIds[index]);
                        strength +=
                            gpPhilAI->FightValueOfStack(&playerHero->m_army, playerHero, 0, 0, 0);
                    }
                    for (index = 0; index < gpGame->m_players[player].m_townCount;
                         index++) {
                        theTown = gpGame->GetTown(gpGame->m_players[player].m_townIds[index]);
                        if (theTown->HasGarrison())
                            strength +=
                                gpPhilAI->FightValueOfStack(&theTown->m_army, NULL, 0, 0, 0);
                    }
                    stats[player] = strength;
                    break;
            }
        }
    }
}

// Buka TOWNMGR.cpp:3843-3862 SortStats, a townManager member in HoMM1.
VA(0x0040db41, 0xea)
void townManager::SortStats(long *const stats, signed char *const order)
{
    long temp;
    short secondPlayer;
    short firstPlayer;
    signed char tempColor;

    for (firstPlayer = 0; firstPlayer < gpGame->m_playerCount - 1; firstPlayer++) {
        for (secondPlayer = firstPlayer + 1; secondPlayer < gpGame->m_playerCount; secondPlayer++) {
            if (stats[secondPlayer] > stats[firstPlayer]) {
                temp = stats[firstPlayer];
                stats[firstPlayer] = stats[secondPlayer];
                stats[secondPlayer] = temp;
                tempColor = order[firstPlayer];
                order[firstPlayer] = order[secondPlayer];
                order[secondPlayer] = tempColor;
            }
        }
    }
}

// HoMM1 town-type wrapper over the global building-name table lookup.
VA(0x0040dc2b, 0x2f)
char *townManager::GetBuildingName(short building)
{
    return ::GetBuildingName(m_town->m_type, building);
}

// Buka TOWNMGR.cpp RecruitHero; HoMM1's tavern shows both candidate heroes,
// a cannot-recruit view is a timed quick view, and the town strips are rebuilt.
VA(0x0040dc5a, 0x981)
signed char townManager::RecruitHero(signed char cannotRecruit)
{
    tag_message message;
    short unusedButtonText = 1;
    short unusedDimState = 2;
    short unusedControlId = 3;
    short unusedPortraitState = 4;
    short unusedTextState = 6;
    short unusedPortraitControl = 7;
    short unusedButtonIcon = 8;
    short unusedMode = 9;

    m_heroWindow1 = new heroWindow(0xb1, 0x10, "rcrthero.bin");
    if (m_heroWindow1 == 0)
        MemError();
    SetWinText(m_heroWindow1, 0xb);
    m_recruitHeroes[0] = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[0]);
    m_recruitHeroes[1] = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[1]);
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = giCurPlayer;
    message.type = MESSAGE_WIDGET;
    if (cannotRecruit) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = 6;
        message.id = 8;
        m_heroWindow1->BroadcastMessage(message);
        message.id = 9;
        m_heroWindow1->BroadcastMessage(message);
        message.id = TOWN_DIALOG_BUTTON_1;
        m_heroWindow1->BroadcastMessage(message);
    }
    sprintf(gText, "port%04d.icn", m_recruitHeroes[0]->m_portrait);
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = 2;
    message.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "port%04d.icn", m_recruitHeroes[1]->m_portrait);
    message.id = 3;
    m_heroWindow1->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 6;
    message.text = gClassNames[m_recruitHeroes[0]->m_heroClass];
    m_heroWindow1->BroadcastMessage(message);
    message.id = 7;
    message.text = gClassNames[m_recruitHeroes[1]->m_heroClass];
    m_heroWindow1->BroadcastMessage(message);
    m_recruitState = -1;
    if (cannotRecruit) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(m_heroWindow1, -1, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(m_heroWindow1);
        gpMouseManager->ReallyShowPointer();
    } else
        gpWindowManager->DoDialog(m_heroWindow1, RecruitHeroHandler, 0);
    delete m_heroWindow1;
    if (m_recruitState != -1) {
        int newHeroClass;
        short townY;
        short townX;

        gpCurPlayer->m_resources[RESOURCE_GOLD] -= gHeroGoldCost;
        gpCurPlayer->m_heroIds[gpCurPlayer->m_heroCount] =
            gpCurPlayer->m_availableHeroIds[m_recruitState];
        gpCurPlayer->m_heroCount++;
        townX = m_town->m_x;
        townY = m_town->m_y;
        m_recruitHeroes[m_recruitState]->m_x = townX;
        m_recruitHeroes[m_recruitState]->m_y = townY;
        m_recruitHeroes[m_recruitState]->m_eventFlags = 0;
        m_recruitHeroes[m_recruitState]->m_direction = 2;
        m_recruitHeroes[m_recruitState]->m_remainingMobility =
            m_recruitHeroes[m_recruitState]->CalcMobility();
        m_recruitHeroes[m_recruitState]->m_mobility =
            m_recruitHeroes[m_recruitState]->m_remainingMobility;
        m_recruitHeroes[m_recruitState]->m_locationType =
            gpGame->m_map[townX][townY].m_triggerType;
        m_recruitHeroes[m_recruitState]->m_occupiedTown =
            gpGame->m_map[townX][townY].m_objectMetadata;
        gpGame->m_map[townX][townY].m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO);
        gpGame->m_map[townX][townY].m_objectMetadata =
            gpCurPlayer->m_availableHeroIds[m_recruitState];
        m_recruitResult = 1;
        m_town->m_occupyingHeroId = m_recruitHeroes[m_recruitState]->m_id;
        gpGame->m_availableHeroes[gpCurPlayer->m_availableHeroIds[m_recruitState]] = giCurPlayer;
        delete m_garrisonStrip;
        sprintf(gText, "crst%04d.icn",
                m_recruitHeroes[m_recruitState]->m_heroClass + gpCurPlayer->Color() * 4);
        m_garrisonStrip = new strip(0, 0x100, m_town->m_occupyingHeroId == -1 ? 4 : 1,
                                    gpResourceManager->MakeId(gText), 0, &m_town->m_army, 0x10, 0);
        if (m_garrisonStrip == 0)
            MemError();
        delete m_heroStrip;
        sprintf(gText, "port%04d.icn", m_recruitHeroes[m_recruitState]->m_portrait);
        m_heroStrip = new strip(0, 0x163, 3, gpResourceManager->MakeId(gText), 0,
                                &m_recruitHeroes[m_recruitState]->m_army, 0x16, 0);
        if (m_heroStrip == 0)
            MemError();
        if (m_town->m_buildings & 1)
            m_town->GiveSpells();
        newHeroClass = gpCurPlayer->m_availableHeroIds[1 - m_recruitState] / 9;
        newHeroClass = (newHeroClass + Random(1, 3)) % 4;
        gpCurPlayer->m_availableHeroIds[m_recruitState] = gpGame->GetNewHeroId(newHeroClass);
        gpGame->m_availableHeroes[gpCurPlayer->m_availableHeroIds[m_recruitState]] = 0x40;
    } else {
        if (m_castleDialogActive)
            SetupCastle(m_heroWindow0);
        if (m_castleDialogActive)
            m_heroWindow0->DrawWindow();
    }
    m_bankBox->Update();
    gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_CLEAR_FLAGS, TOWN_CLOSE_CONTROL, 0x4008);
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = -1;
    if (m_recruitState != -1)
        m_recruitHeroes[m_recruitState]->m_owner = giCurPlayer;
    return gpWindowManager->m_dialogResult != TOWN_DIALOG_BUTTON_1;
}

// donor PoL RVA 0x00019c29; preferred Buka symbol ?TavernHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.461090;margin=0.267847;shape=0.244;size=0.845;calls=1.000;alternate=pol20:int TavernHandler(struct tag_message &)@0x00019c29
// Buka TOWNMGR.cpp:2968-3000; HoMM1 animates frames 1-8 of control 2.
VA(0x0040e5db, 0x155)
short TavernHandler(struct tag_message &message)
{
    int unusedDelay = TOWN_TAVERN_ANIMATION_DELAY;
    short unusedFrame = TOWN_TAVERN_UNUSED_FRAME;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case TOWN_DIALOG_BUTTON_0:
                    case TOWN_DIALOG_BUTTON_1:
                    case TOWN_DIALOG_BUTTON_2:
                        gpWindowManager->m_dialogResult = message.id;
                        message.command = message.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (glTimers[0] < KBTickCount()) {
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = TOWN_TAVERN_ANIMATION_CONTROL;
        ++gpGame->m_viewArmyResult;
        message.value =
            gpGame->m_viewArmyResult % TOWN_TAVERN_ANIMATION_FRAME_COUNT
            + TOWN_TAVERN_FIRST_ANIMATION_FRAME;
        gpTownManager->m_heroWindow0->BroadcastMessage(message);
        gpTownManager->m_heroWindow0->MoveWindow(0, 0);
        glTimers[0] = KBTickCount() + TOWN_TAVERN_ANIMATION_DELAY;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x00019d7c; preferred Buka symbol ?DoTavern@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.728216;margin=0.164549;shape=0.467;size=0.965;calls=0.889;strings=tavwin.bin;alternate=pol20:void townManager::DoTavern(void)@0x00019d7c
// Buka TOWNMGR.cpp:3003-3032; HoMM1 plays the tavern theme instead of a
// rumour and restores the town theme afterwards.
VA(0x0040e730, 0x136)
void townManager::DoTavern(void)
{
    int unusedValue = 0;

    m_heroWindow0 = new heroWindow(TOWN_TAVERN_WINDOW_X, TOWN_TAVERN_WINDOW_Y, "tavwin.bin");
    if (m_heroWindow0 == NULL)
        MemError();
    SetWinText(m_heroWindow0, TOWN_TAVERN_WINDOW_TEXT);
    gpSoundManager->SwitchAmbientMusic(TOWN_TAVERN_MUSIC);
    gpWindowManager->DoDialog(m_heroWindow0, TavernHandler, 0);
    delete m_heroWindow0;
    gpSoundManager->SwitchAmbientMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE);
}

// Buka Castle.cpp CastleHandler; HoMM1 hovers by widget id, has no
// captain or formation controls and recruits a single hero (control 0x30).
VA(0x0040e866, 0x726)
short CastleHandler(struct tag_message &message)
{
    short statusId = TOWN_CASTLE_STATUS_CONTROL;
    int result = 0;
    int quickFlag;
    int objNum;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpTownManager->m_lastHoverId)
                    break;
                gpTownManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case TOWN_BUILDING_MAGE_GUILD:
                        if (!(gpTownManager->m_buildableBuildings & (1 << message.id)))
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_CANNOT_BUILD],
                                    gpTownManager->GetBuildingName(message.id));
                        else if (!(gpTownManager->m_affordableBuildings & (1 << message.id)))
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD],
                                    gpTownManager->GetBuildingName(message.id));
                        else {
                            if (!(gpTownManager->m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD)))
                                objNum = TOWN_CASTLE_INFO_BUILD_MAGE_GUILD;
                            else if (gpTownManager->m_town->m_buildState == 3)
                                objNum = TOWN_CASTLE_INFO_MAGE_GUILD_MAX_LEVEL;
                            else if (!CanBuy(gpTownManager->m_town, TOWN_BUILDING_MAGE_GUILD))
                                objNum = TOWN_CASTLE_INFO_CANNOT_AFFORD_MAGE_LEVEL;
                            else
                                objNum = TOWN_CASTLE_INFO_ADD_MAGE_GUILD_LEVEL;
                            strcpy(gText, cCastleInfo[objNum]);
                        }
                        break;
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        if (gpTownManager->m_town->m_buildings & (1 << message.id))
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_ALREADY_BUILT],
                                    gpTownManager->GetBuildingName(message.id));
                        else if (!(gpTownManager->m_buildableBuildings & (1 << message.id)))
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_CANNOT_BUILD],
                                    gpTownManager->GetBuildingName(message.id));
                        else if (!(gpTownManager->m_affordableBuildings & (1 << message.id)))
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD],
                                    gpTownManager->GetBuildingName(message.id));
                        else
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_BUILD],
                                    gpTownManager->GetBuildingName(message.id));
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (gpCurPlayer->m_resources[RESOURCE_GOLD] < gHeroGoldCost)
                            strcpy(gText, cCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD_HERO]);
                        else if (gpCurPlayer->m_heroCount == PLAYER_HERO_CAPACITY)
                            sprintf(gText, cCastleInfo[TOWN_CASTLE_INFO_TOO_MANY_HEROES],
                                    PLAYER_HERO_CAPACITY);
                        else if (gpTownManager->m_town->m_occupyingHeroId != -1)
                            strcpy(gText, cCastleInfo[TOWN_CASTLE_INFO_TOWN_OCCUPIED]);
                        else
                            strcpy(gText, cCastleInfo[TOWN_CASTLE_INFO_RECRUIT_HERO]);
                        break;
                    case TOWN_DIALOG_BUTTON_0:
                        strcpy(gText, cCastleInfo[TOWN_CASTLE_INFO_EXIT]);
                        break;
                    default:
                        strcpy(gText, cCastleInfo[TOWN_CASTLE_INFO_OPTIONS]);
                        break;
                }
                message.command = WIDGET_COMMAND_SET_TEXT;
                message.id = TOWN_CASTLE_STATUS_TEXT_CONTROL;
                message.text = gText;
                gpTownManager->m_heroWindow0->BroadcastMessage(message);
                gpTownManager->m_heroWindow0->DrawWindow(0, TOWN_CASTLE_STATUS_FIRST_CONTROL,
                                                         TOWN_CASTLE_STATUS_TEXT_CONTROL);
                gpWindowManager->UpdateScreenRegion(TOWN_CASTLE_STATUS_X, TOWN_CASTLE_STATUS_Y,
                                                    TOWN_CASTLE_STATUS_WIDTH, TOWN_CASTLE_STATUS_HEIGHT);
                return MESSAGE_DISPATCH_CONSUME;
            case WIDGET_NOTIFY_SELECT:
                if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                    quickFlag = 1;
                else
                    quickFlag = 0;
                switch (message.id) {
                    case TOWN_BUILDING_MAGE_GUILD:
                        if (!quickFlag
                            && (gpTownManager->m_town->m_buildState == 3
                                || !(gpTownManager->m_buildableBuildings
                                     & (1 << message.id))))
                            break;
                        else
                            goto buy_building;
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        if (!quickFlag
                            && ((gpTownManager->m_town->m_buildings & (1 << message.id))
                                || !(gpTownManager->m_buildableBuildings
                                     & (1 << message.id))))
                            break;
                    buy_building:
                        for (objNum = 0; objNum < gpTownManager->m_townObjectCount; objNum++) {
                            if (gpTownManager->m_townObjects[objNum]->m_buildingId
                                == message.id)
                                break;
                        }
                        result = gpTownManager->BuyBuild(
                            message.id,
                            (gpTownManager->m_affordableBuildings & (1 << message.id))
                                == 0,
                            quickFlag);
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (quickFlag)
                            gpTownManager->RecruitHero(1);
                        else if (!gpTownManager->m_recruitResult
                                 && gpCurPlayer->m_resources[RESOURCE_GOLD] >= gHeroGoldCost
                                 && gpCurPlayer->m_heroCount < PLAYER_HERO_CAPACITY
                                 && gpTownManager->m_town->m_occupyingHeroId == -1)
                            result = gpTownManager->RecruitHero(0);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (result) {
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return TrueFalseDialogHandler(message);
}

// Buka TOWNMGR.cpp:3034 SplitArmyHandler; HoMM1 handles the amount
// buttons on selection and redraws the whole split window.
VA(0x0040ef8c, 0x32d)
short SplitArmyHandler(struct tag_message &message)
{
    short plusControl = TOWN_SPLIT_INCREASE_CONTROL;
    int unusedAction;
    short minusButton = TOWN_SPLIT_DECREASE_CONTROL;
    short amountText = TOWN_SPLIT_AMOUNT_CONTROL;
    int handled = 0;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case TOWN_SPLIT_INCREASE_CONTROL:
                        ++gpTownManager->m_splitAmount;
                        if (gpTownManager->m_splitAmount >= gpTownManager->m_splitMaximum)
                            gpTownManager->m_splitAmount = gpTownManager->m_splitMaximum - 1;
                        goto update_amount;
                    case TOWN_SPLIT_DECREASE_CONTROL:
                        --gpTownManager->m_splitAmount;
                        if (gpTownManager->m_splitAmount < 0)
                            gpTownManager->m_splitAmount = 0;
                        goto update_amount;
                    case TOWN_SPLIT_AMOUNT_CONTROL:
                        message.command = WIDGET_COMMAND_GET_TEXT;
                        gpTownManager->m_heroWindow1->BroadcastMessage(message);
                        gpTownManager->m_splitAmount = atoi(message.text);
                        if (gpTownManager->m_splitAmount < 0)
                            gpTownManager->m_splitAmount = 0;
                        if (gpTownManager->m_splitAmount >= gpTownManager->m_splitMaximum)
                            gpTownManager->m_splitAmount = gpTownManager->m_splitMaximum - 1;
                        goto update_amount;
                }
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case TOWN_DIALOG_BUTTON_0:
                    case TOWN_DIALOG_BUTTON_1:
                        gpTownManager->m_splitAmount = 0;
                        gpWindowManager->m_dialogResult = message.id;
                        handled = 1;
                        break;
                    case TOWN_DIALOG_BUTTON_2:
                        if (gpTownManager->m_splitAmount == 0)
                            gpWindowManager->m_dialogResult = TOWN_DIALOG_BUTTON_1;
                        else
                            gpWindowManager->m_dialogResult = TOWN_DIALOG_BUTTON_2;
                        handled = 1;
                        break;
                }
                break;
            default:
                break;
        }
    }

    if (handled == 1) {
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;

update_amount:
    sprintf(gText, "%d", gpTownManager->m_splitAmount);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_SPLIT_AMOUNT_CONTROL;
    message.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    gpTownManager->m_heroWindow1->DrawWindow();
    return MESSAGE_DISPATCH_CONSUME;
}
// TOWNMGR's .rdata: Open's per-type town-object layout.
DATA(0x0048c028)
const signed char gTownObjectType[4][16] = {
    {5, 6, 8, 11, 7, 0, 1, 2, 10, 9, 3, 4, 12, -1, -1, -1},
    {5, 6, 12, 8, 0, 9, 10, 1, 2, 11, 3, 4, 7, -1, -1, -1},
    {13, 5, 6, 9, 7, 11, 0, 1, 2, 10, 8, 12, 3, 4, -1, -1},
    {5, 6, 12, 9, 0, 11, 10, 1, 2, 7, 3, 4, 8, -1, -1, -1},
};
