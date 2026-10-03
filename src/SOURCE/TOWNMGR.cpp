// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/BITS.h>
#include <BASE/border.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/soundManager.h>
#include <BASE/textWidget.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/bankBox.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/recruitUnit.h>
#include <SOURCE/strip.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>
#include <SOURCE/townObject.h>

#include <stdlib.h>
#include <string.h>

// Retail compiled this file incrementally (/Gi): each ProcessAssert line is the
// function's compiler line static plus an offset; #line restores the original
// file and lines (docs/patterns/vc4-gi-line-var.md).

// donor PoL RVA 0x00013900; preferred Buka symbol ??0townObject@@QAE@HHPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.615649;margin=0.314990;shape=0.431;size=0.799;calls=0.333;strings=%s.icn;alternate=pol20:void townObject::constructor(int, int, char *)@0x00013900
// Buka TOWNMGR.cpp townObject ctor; HoMM1 reads frame count, rectangle and
// building id from the .tod resource instead of sBuildingInfo.
VA(0x00407d90, 0x1f1)
townObject::townObject(char* name) {
    char fileName[16];
    i16 w;
    i16 tmp;
    i16 id;
    i16 x;
    i16 h;
    i16 y;

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
    if (id == BUILDING_SLOT_MAGE_GUILD) {
        h = gpTownManager->m_town->m_buildState * TOWN_MAGE_GUILD_LEVEL_HEIGHT
            + TOWN_MAGE_GUILD_BASE_HEIGHT;
        y = TOWN_MAGE_GUILD_BOTTOM_Y - h;
    }
    if (id != TOWN_OBJECT_NONE) {
        m_border = new border(x, y, w, h, id, WIDGET_KIND_TRANSPARENT, 0, NULL);
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
void townObject::Draw(i8 advanceAnimation) {
    i16 level;

    if (!m_visible)
        return;
    m_icon->DrawToBuffer(0, 0, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    if (m_buildingId == BUILDING_SLOT_MAGE_GUILD) {
        for (level = 0; level < gpTownManager->m_town->m_buildState; level++)
            m_icon->DrawToBuffer(
                0,
                0,
                (level + 1) * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        m_icon->DrawToBuffer(
            0,
            0,
            gpTownManager->m_town->m_buildState * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE + 1,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
    }
    if (m_animationFrameCount) {
        m_icon->DrawToBuffer(0, 0, m_animationFrame + 1, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
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
townManager::townManager(void) {
    m_town = NULL;
    m_townObjectCount = 0;
    m_heroWindow0 = NULL;
    m_coverWindow = NULL;
    m_selectedBuilding = TOWN_BUILDING_NONE;
    m_castleDialogActive = 0;
    m_dispatchMask = TOWN_MANAGER_DISPATCH_MASK;
}

// Buka TOWNMGR.cpp Open/SetupTown; retail vtable slot 0 (0x0048c068).
// HoMM1 builds the town window, objects, strips and bank box here.
VA(0x0040816c, 0x7ec)
i16 townManager::Open(i16 id) {
    i16 crest;
    tag_message message;
    i16 i;
    i8 buildingType;

    gpGame->CheckHeroConsistency();
    gpSoundManager->PlayAmbientMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE, 0, -1);
    PollSound();
    m_townWindow = new heroWindow(0, 0, "townwind.bin");
    if (m_townWindow == NULL)
        MemError();
    sprintf(gText, GetTownName(m_town->m_id));
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_NAME_TEXT_CONTROL);
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
        if (buildingType != TOWN_OBJECT_NONE) {
            // One name table: neutral objects, four town-type prefixes, then
            // the faction-object suffixes.
            if (buildingType < TOWN_FIRST_FACTION_OBJECT)
                strcpy(gText, gTownObjectNames[buildingType]);
            else
                sprintf(
                    gText,
                    "%s%s",
                    gTownObjectNames[TOWN_FIRST_FACTION_OBJECT + m_town->m_type],
                    gTownObjectNames[buildingType + TOWN_TYPE_COUNT]
                );
            m_townObjects[m_townObjectCount] = new townObject(gText);
            if (m_townObjects[m_townObjectCount] == NULL)
                MemError();
            if (m_townObjects[m_townObjectCount]->m_border) {
                if (!(m_town->m_buildings & (1 << buildingType))) {
                    m_townObjects[m_townObjectCount]->m_border->m_flags &= ~WIDGET_FLAG_ENABLED;
                    m_townObjects[m_townObjectCount]->m_visible = 0;
                }
                m_townWindow->AddWidget(
                    m_townObjects[m_townObjectCount]->m_border,
                    WINDOW_Z_ORDER_APPEND
                );
            }
            m_townObjectCount++;
        }
    }
    glTimers[TOWN_FRAME_TIMER_SLOT] = KBTickCount() + TOWN_REDRAW_INTERVAL;
    gpWindowManager->AddWindow(m_townWindow, 0, 1);
    crest = gpCurPlayer->m_color;
    if (m_town->OccupyingHero() != TOWN_OCCUPYING_HERO_NONE) {
        crest = crest * HERO_CLASS_COUNT;
        crest += gpGame->GetHero(m_town->m_occupyingHeroId)->m_heroClass;
    } else
        crest += TOWN_CREST_NO_HERO_OFFSET;
    sprintf(gText, "crst%04d.icn", crest);
    m_garrisonStrip = new strip(
        0,
        TOWN_GARRISON_STRIP_Y,
        m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE ? TOWN_CREST_FRAME_WITHOUT_HERO
                                                              : TOWN_CREST_FRAME_WITH_HERO,
        gpResourceManager->MakeId(gText),
        0,
        &m_town->m_army,
        TOWN_GARRISON_FIRST_CONTROL,
        1
    );
    if (m_garrisonStrip == NULL)
        MemError();
    if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        sprintf(gText, "port%04d.icn", gpGame->GetHero(m_town->m_occupyingHeroId)->m_portrait);
        m_heroStrip = new strip(
            0,
            TOWN_HERO_STRIP_Y,
            TOWN_HERO_STRIP_FRAME_COUNT,
            gpResourceManager->MakeId(gText),
            0,
            &gpGame->GetHero(m_town->m_occupyingHeroId)->m_army,
            TOWN_HERO_FIRST_CONTROL,
            1
        );
        if (m_heroStrip == NULL)
            MemError();
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            m_town->GiveSpells();
    } else {
        m_heroStrip = new strip(
            0,
            TOWN_HERO_STRIP_Y,
            TOWN_HERO_STRIP_FRAME_COUNT,
            gpResourceManager->MakeId("strip.icn"),
            TOWN_EMPTY_HERO_PORTRAIT_FRAME,
            NULL,
            -1,
            1
        );
        if (m_heroStrip == NULL)
            MemError();
    }
    m_bankBox = new bankBox(TOWN_BANK_BOX_X, TOWN_BANK_BOX_Y, gpCurPlayer);
    if (m_bankBox == NULL)
        MemError();
    m_selectedStrip = m_swapStrip = m_pendingStrip = NULL;
    m_selectedArmySlot = m_swapArmySlot = m_pendingArmySlot = STRIP_SLOT_NONE;
    DrawTown(0, 0);
    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gpMouseManager->ReallyShowPointer();
    gpMouseManager->NewUpdate(1);
    KBChangeMenu(hmnuTown);
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
    m_castleDialogActive = 0;
    m_recruitResult = 0;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    m_messageMask = TOWN_MANAGER_MESSAGE_MASK;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "townManager");
    return BASE_MANAGER_SUCCESS;
}

// donor PoL RVA 0x00014cc9; preferred Buka symbol ?UnloadTown@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.470224;margin=0.176996;shape=0.284;size=0.924;calls=0.667;alternate=pol20:void townManager::UnloadTown(void)@0x00014cc9
// Retail vtable slot 1: HoMM1's Close performs Buka's UnloadTown work.
VA(0x00408958, 0x1c4)
void townManager::Close(void) {
    i16 index;

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
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NONE);
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpMouseManager->SetPointer(MOUSE_INVALID_CURSOR_FRAME);
    m_active = 0;
}

// Buka TOWNMGR.cpp:944-1020; HoMM1 matches the dragged creature against
// every slot of the target army and keeps word-sized flags.
VA(0x00408b1c, 0x3b6)
void townManager::SetArmyCommand(i16 qualifier) {
    i16 lastArmy;
    i16 i;
    i16 sameType;

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
                sprintf(
                    m_statusText,
                    gTownCommand[TOWN_TEXT_REDISTRIBUTE_ARMY],
                    gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
                );
                m_command = TOWN_ARMY_COMMAND_SPLIT;
            } else if (lastArmy) {
                strcpy(m_statusText, gTownCommand[TOWN_TEXT_CANNOT_COMBINE_LAST_ARMY]);
                return;
            } else {
                sprintf(
                    m_statusText,
                    gTownCommand[TOWN_TEXT_COMBINE_ARMIES],
                    gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
                );
                m_command = TOWN_ARMY_COMMAND_MERGE;
            }
        } else if (qualifier
                   && m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == CREATURE_NONE) {
            sprintf(
                m_statusText,
                gTownCommand[TOWN_TEXT_REDISTRIBUTE_TO_EMPTY_SLOT],
                gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
            );
            m_command = TOWN_ARMY_COMMAND_SPLIT;
        }
    } else if (m_swapArmySlot == m_pendingArmySlot) {
        sprintf(
            m_statusText,
            gTownCommand[TOWN_TEXT_VIEW_ARMY],
            gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
        );
        m_command = TOWN_ARMY_COMMAND_VIEW;
    }

    if (m_command != TOWN_ARMY_COMMAND_NONE)
        return;
    if (m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == CREATURE_NONE) {
        if (lastArmy) {
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_CANNOT_MOVE_LAST_ARMY]);
            return;
        } else {
            sprintf(
                m_statusText,
                gTownCommand[TOWN_TEXT_MOVE_ARMY],
                gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
            );
            m_command = TOWN_ARMY_COMMAND_SWAP;
        }
    } else {
        sprintf(
            m_statusText,
            gTownCommand[TOWN_TEXT_EXCHANGE_ARMIES],
            gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
            gArmyNames[m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]]
        );
        m_command = TOWN_ARMY_COMMAND_SWAP;
    }
}

// Buka TOWNMGR.cpp:1022-1176; HoMM1 has no calendar entry and names the
// six dwellings through gDwellingType.
VA(0x00408ed2, 0x468)
void townManager::SetCommandAndText(struct tag_message& message) {
    i16 id;

    id = message.id;
    m_command = TOWN_ARMY_COMMAND_NONE;
    switch (id) {
        case TOWN_CLOSE_CONTROL:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_EXIT]);
            break;
        case WINDOW_MANAGER_NO_HOVER_WIDGET:
        case TOWN_EMPTY_STATUS_CONTROL_FIRST:
        case TOWN_EMPTY_STATUS_CONTROL_LAST:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_EMPTY_STATUS]);
            break;
        case TOWN_GARRISON_FIRST_CONTROL:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_GARRISON]);
            m_command = TOWN_ARMY_COMMAND_GARRISON;
            break;
        case TOWN_GARRISON_SLOT_FIRST:
        case TOWN_GARRISON_SLOT_FIRST + 1:
        case TOWN_GARRISON_SLOT_FIRST + 2:
        case TOWN_GARRISON_SLOT_FIRST + 3:
        case TOWN_GARRISON_SLOT_FIRST + 4:
            if (m_swapArmySlot != STRIP_SLOT_NONE) {
                m_pendingStrip = m_garrisonStrip;
                m_pendingArmySlot = id - TOWN_GARRISON_SLOT_FIRST;
                SetArmyCommand(message.modifiers & TOWN_SHIFT_QUALIFIER_MASK);
            } else {
                m_selectedStrip = m_garrisonStrip;
                m_selectedArmySlot = id - TOWN_GARRISON_SLOT_FIRST;
                if (m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] == CREATURE_NONE)
                    strcpy(m_statusText, gTownCommand[TOWN_TEXT_EMPTY_SLOT]);
                else {
                    sprintf(
                        m_statusText,
                        gTownCommand[TOWN_TEXT_SELECT_ARMY],
                        gArmyNames[m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]]
                    );
                    m_command = TOWN_ARMY_COMMAND_SELECT;
                }
            }
            break;
        case TOWN_HERO_FIRST_CONTROL:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_VIEW_HERO]);
            m_command = TOWN_ARMY_COMMAND_VIEW_HERO;
            break;
        case TOWN_HERO_SLOT_FIRST:
        case TOWN_HERO_SLOT_FIRST + 1:
        case TOWN_HERO_SLOT_FIRST + 2:
        case TOWN_HERO_SLOT_FIRST + 3:
        case TOWN_HERO_SLOT_FIRST + 4:
            if (m_swapArmySlot != STRIP_SLOT_NONE) {
                m_pendingStrip = m_heroStrip;
                m_pendingArmySlot = id - TOWN_HERO_SLOT_FIRST;
                SetArmyCommand(message.modifiers & TOWN_SHIFT_QUALIFIER_MASK);
            } else {
                m_selectedStrip = m_heroStrip;
                m_selectedArmySlot = id - TOWN_HERO_SLOT_FIRST;
                if (m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] == CREATURE_NONE) {
                    strcpy(m_statusText, gTownCommand[TOWN_TEXT_EMPTY_SLOT]);
                    m_command = TOWN_ARMY_COMMAND_NONE;
                } else {
                    sprintf(
                        m_statusText,
                        gTownCommand[TOWN_TEXT_SELECT_ARMY],
                        gArmyNames[m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]]
                    );
                    m_command = TOWN_ARMY_COMMAND_SELECT;
                }
            }
            break;
        case BUILDING_SLOT_MAGE_GUILD:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_MAGE_GUILD]);
            break;
        case BUILDING_SLOT_THIEVES_GUILD:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_THIEVES_GUILD]);
            break;
        case BUILDING_SLOT_TAVERN:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_TAVERN]);
            break;
        case BUILDING_SLOT_SHIPYARD:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_SHIPYARD]);
            break;
        case BUILDING_SLOT_WELL:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_WELL]);
            break;
        case BUILDING_SLOT_TENT:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_TENT]);
            break;
        case BUILDING_SLOT_CASTLE:
            strcpy(m_statusText, gTownCommand[TOWN_TEXT_BUILDING_0 + BUILDING_SLOT_CASTLE]);
            break;
        case BUILDING_SLOT_DWELLING_1:
        case BUILDING_SLOT_DWELLING_2:
        case BUILDING_SLOT_DWELLING_3:
        case BUILDING_SLOT_DWELLING_4:
        case BUILDING_SLOT_DWELLING_5:
        case BUILDING_SLOT_DWELLING_6:
            sprintf(
                m_statusText,
                gTownCommand[TOWN_TEXT_DWELLING],
                gArmyNames[gDwellingType[m_town->m_type][id - BUILDING_SLOT_DWELLING_FIRST]]
            );
            break;
    }
    ShowText(m_statusText);
}

// donor PoL RVA 0x000158e0; preferred Buka symbol ?ShowText@townManager@@QAEXPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.613333;margin=0.109874;shape=0.519;size=1.000;calls=1.000;alternate=pol20:void townManager::ShowText(char *)@0x000158e0
VA(0x0040933a, 0x74)
void townManager::ShowText(char*) {
    tag_message message;

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_STATUS_TEXT_CONTROL);
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0, TOWN_STATUS_TEXT_CONTROL - 2, TOWN_STATUS_TEXT_CONTROL);
    gpWindowManager->UpdateScreenRegion(
        0,
        TOWN_STATUS_REGION_Y,
        TOWN_STATUS_REGION_WIDTH,
        TOWN_STATUS_REGION_HEIGHT
    );
}

// Buka TOWNMGR.cpp Main; HoMM1 opens the castle, mage guild, well and thieves
// guild over a bottom cover window, sells the spell book and builds boats.
VA(0x004093ae, 0x131f)
i16 townManager::Main(struct tag_message& message) {
    i32 exitTown;
    i8 rightClick;
    SAMPLE2 res;
    recruitUnit* recruitMgr;

    exitTown = 0;
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        rightClick = 1;
    else
        rightClick = 0;
    if (KBTickCount() > glTimers[TOWN_FRAME_TIMER_SLOT]) {
        DrawTown(1, 1);
        glTimers[TOWN_FRAME_TIMER_SLOT] = KBTickCount() + TOWN_REDRAW_INTERVAL;
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
                        case BUILDING_SLOT_DWELLING_1:
                        case BUILDING_SLOT_DWELLING_2:
                        case BUILDING_SLOT_DWELLING_3:
                        case BUILDING_SLOT_DWELLING_4:
                        case BUILDING_SLOT_DWELLING_5:
                        case BUILDING_SLOT_DWELLING_6:
                            if (rightClick) {
                                QuickViewRecruit(m_town, message.id - BUILDING_SLOT_DWELLING_FIRST);
                                break;
                            }
                            gpMouseManager->ReallyHidePointer();
                            DrawTown(1, 1);
                            recruitMgr =
                                new recruitUnit(m_town, message.id - BUILDING_SLOT_DWELLING_FIRST);
                            if (recruitMgr == NULL)
                                MemError();
                            gpExec->DoDialog(recruitMgr);
                            delete recruitMgr;
                            break;
                        case BUILDING_SLOT_MAGE_GUILD:
                        case BUILDING_SLOT_THIEVES_GUILD:
                        case BUILDING_SLOT_WELL:
                        case BUILDING_SLOT_CASTLE:
                            if (rightClick)
                                break;
                            gpWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_SET_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            m_coverWindow =
                                new heroWindow(0, 0x100, 0x280, 6, WINDOW_FLAG_SAVE_BACKGROUND);
                            if (m_coverWindow == NULL)
                                MemError();
                            gpWindowManager->AddWindow(m_coverWindow, WINDOW_Z_ORDER_APPEND, 1);
                            switch (message.id) {
                                case BUILDING_SLOT_CASTLE:
                                    gpWindowManager->SaveFizzleSource(0, 0x100, 0x228, 0xcc);
                                    m_heroWindow0 = new heroWindow(0, 0, "caslwind.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetWinText(m_heroWindow0, WINDOW_TEXT_CASTLE);
                                    SetupCastle(m_heroWindow0);
                                    m_castleDialogActive = 1;
                                    gpWindowManager->DoDialog(m_heroWindow0, CastleHandler, 0);
                                    m_castleDialogActive = 0;
                                    break;
                                case BUILDING_SLOT_MAGE_GUILD:
                                    if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE
                                        && !gpGame->GetHero(m_town->m_occupyingHeroId)
                                                ->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                                        if (gpGame->GetHero(m_town->m_occupyingHeroId)
                                                ->NumArtifacts()
                                            == HERO_ARTIFACT_SLOT_COUNT)
                                            NormalDialog(
                                                "You must purchase a spell book to use the mage "
                                                "guild, but "
                                                "you currently have no room for a spell book.  Try "
                                                "giving "
                                                "one of your artifacts to another hero.",
                                                NORMAL_DIALOG_TYPE_OK,
                                                -1,
                                                -1,
                                                NORMAL_DIALOG_NO_RESOURCE,
                                                0,
                                                NORMAL_DIALOG_NO_RESOURCE,
                                                0,
                                                NORMAL_DIALOG_NO_OR_TEXT
                                            );
                                        else {
                                            m_heroWindow0 =
                                                new heroWindow(0xb1, 0x14, "buybook.bin");
                                            if (m_heroWindow0 == NULL)
                                                MemError();
                                            SetWinText(m_heroWindow0, WINDOW_TEXT_BUY_SPELL_BOOK);
                                            if (gpGame
                                                    ->m_players
                                                        [gpGame->GetHero(m_town->m_occupyingHeroId)
                                                             ->m_owner]
                                                    .m_resources[RESOURCE_GOLD]
                                                < TOWN_SPELL_BOOK_COST) {
                                                SET_WIDGET_MESSAGE(
                                                    message,
                                                    WIDGET_COMMAND_SET_FLAGS,
                                                    DIALOG_BUTTON_2
                                                );
                                                message.value = WIDGET_FLAG_DIMMED;
                                                m_heroWindow0->BroadcastMessage(message);
                                                message.command = WIDGET_COMMAND_CLEAR_FLAGS;
                                                message.value = WIDGET_FLAG_ENABLED;
                                                m_heroWindow0->BroadcastMessage(message);
                                            }
                                            gpWindowManager->DoDialog(
                                                m_heroWindow0,
                                                TrueFalseDialogHandler,
                                                0
                                            );
                                            if (gpWindowManager->m_dialogResult
                                                == DIALOG_BUTTON_2) {
                                                gpAdvManager->GiveArtifact(
                                                    gpGame->GetHero(m_town->m_occupyingHeroId),
                                                    ARTIFACT_MAGIC_BOOK
                                                );
                                                gpCurPlayer->m_resources[RESOURCE_GOLD] -=
                                                    TOWN_SPELL_BOOK_COST;
                                                m_bankBox->Update();
                                                m_townWindow->DrawWindow();
                                                m_town->GiveSpells();
                                            }
                                        }
                                    } else {
                                        m_heroWindow0 = new heroWindow(0, 0, "magewind.bin");
                                        if (m_heroWindow0 == NULL)
                                            MemError();
                                        SetWinText(m_heroWindow0, WINDOW_TEXT_MAGE_GUILD);
                                        SetupMage(m_heroWindow0);
                                        gpWindowManager
                                            ->DoDialog(m_heroWindow0, MageGuildHandler, 0);
                                    }
                                    m_town->GiveSpells();
                                    break;
                                case BUILDING_SLOT_WELL:
                                    m_heroWindow0 = new heroWindow(0, 0, "wellwind.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetupWell(m_heroWindow0);
                                    gpWindowManager
                                        ->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                    break;
                                case BUILDING_SLOT_THIEVES_GUILD:
                                    m_heroWindow0 = new heroWindow(0, 0, "thiefwin.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetWinText(m_heroWindow0, WINDOW_TEXT_THIEVES_GUILD);
                                    SetupThievesGuild(m_heroWindow0, THIEVES_CATEGORIES_BY_GUILDS);
                                    gpWindowManager
                                        ->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                    break;
                            }
                            delete m_heroWindow0;
                            gpWindowManager->RemoveWindow(m_coverWindow);
                            delete m_coverWindow;
                            if (m_selectedBuilding != TOWN_BUILDING_NONE)
                                BuildObj(m_selectedBuilding);
                            if (m_recruitResult) {
                                hero* theHero;
                                i32 i;
                                i32 width;

                                gpMouseManager->ReallyHidePointer();
                                res = NULL_SAMPLE2;
                                res = LoadPlaySample("buildtwn.82M");
                                theHero = gpGame->GetHero(m_town->m_occupyingHeroId);
                                width = 0;
                                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                                    if (theHero->m_army.m_creatureTypes[i] != CREATURE_NONE)
                                        width = i + 1;
                                }
                                width = width * 88 + 0x70;
                                DrawTown(1, 1);
                                gpWindowManager->FizzleForward(
                                    0,
                                    0x100,
                                    width,
                                    0xcc,
                                    FIZZLE_USE_DEFAULT_DELAY
                                );
                                WaitEndSample(res, SAMPLE_WAIT_DEFAULT);
                                m_recruitResult = 0;
                                gpMouseManager->ReallyShowPointer();
                            }
                            gpWindowManager->ReleaseFizzleSource();
                            gpWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_CLEAR_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            break;
                        case BUILDING_SLOT_TAVERN:
                            if (rightClick)
                                break;
                            DoTavern();
                            break;
                        case BUILDING_SLOT_TENT:
                            if (rightClick)
                                return MESSAGE_DISPATCH_CONSUME;
                            if (BuyBuild(
                                    BUILDING_SLOT_CASTLE,
                                    !CanBuy(m_town, BUILDING_SLOT_CASTLE),
                                    rightClick
                                )) {
                                BuildObj(BUILDING_SLOT_CASTLE);
                                m_town->XformToCastle();
                            }
                            break;
                        case BUILDING_SLOT_SHIPYARD:
                            if (rightClick)
                                break;
                            gpWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_SET_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            if (gpGame->GetBoatsBuilt() < GAME_BOAT_COUNT
                                && gpAdvManager->GetCell(m_town->m_x - 1, m_town->m_y + 1)
                                           ->m_triggerType
                                       == MAP_OBJECT_NONE
                                && m_town->m_x - 1 != gpAdvManager->m_cursorMapX
                                && m_town->m_y + 1 != gpAdvManager->m_cursorMapY) {
                                m_heroWindow0 = new heroWindow(0xb1, 0x14, "shipwind.bin");
                                if (m_heroWindow0 == NULL)
                                    MemError();
                                SetWinText(m_heroWindow0, WINDOW_TEXT_SHIPYARD);
                                if (gpGame->m_players[giCurPlayer].m_resources[RESOURCE_GOLD]
                                        < TOWN_BOAT_GOLD_COST
                                    || gpGame->m_players[giCurPlayer].m_resources[RESOURCE_WOOD]
                                           < TOWN_BOAT_WOOD_COST) {
                                    SET_WIDGET_MESSAGE(
                                        message,
                                        WIDGET_COMMAND_SET_FLAGS,
                                        DIALOG_BUTTON_2
                                    );
                                    message.value = WIDGET_FLAG_DIMMED;
                                    m_heroWindow0->BroadcastMessage(message);
                                    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
                                    message.value = WIDGET_FLAG_ENABLED;
                                    m_heroWindow0->BroadcastMessage(message);
                                }
                                gpWindowManager->DoDialog(m_heroWindow0, TrueFalseDialogHandler, 0);
                                delete m_heroWindow0;
                                if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
                                    if (gpGame->CreateBoat(m_town->m_x - 1, m_town->m_y + 1)
                                        != GAME_TABLE_FREE) {
                                        res = NULL_SAMPLE2;
                                        res = LoadPlaySample("buildtwn.82M");
                                        gpGame->m_players[giCurPlayer].m_resources[RESOURCE_GOLD] -=
                                            TOWN_BOAT_GOLD_COST;
                                        gpGame->m_players[giCurPlayer].m_resources[RESOURCE_WOOD] -=
                                            TOWN_BOAT_WOOD_COST;
                                        m_bankBox->Update();
                                        WaitEndSample(res, SAMPLE_WAIT_DEFAULT);
                                    } else
                                        LogStr("Can't create boat!");
                                }
                            } else
                                NormalDialog(
                                    "Cannot build another boat.",
                                    NORMAL_DIALOG_TYPE_OK,
                                    0xd0,
                                    0x28,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_OR_TEXT
                                );
                            gpWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_CLEAR_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            break;
                        case TOWN_CLOSE_CONTROL:
                            if (rightClick)
                                break;
                            exitTown++;
                            break;
                        default:
                            if (rightClick) {
                                i32 found;
                                hero* viewHero;

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
                                if (found
                                    && m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]
                                           != CREATURE_NONE) {
                                    viewHero = m_heroStrip == m_selectedStrip
                                                   ? gpGame->GetHero(m_town->m_occupyingHeroId)
                                                   : NULL;
                                    gpGame->ViewArmy(
                                        TOWN_ARMY_VIEW_X,
                                        TOWN_ARMY_VIEW_Y,
                                        m_selectedStrip->m_army
                                            ->m_creatureTypes[m_selectedArmySlot],
                                        m_selectedStrip->m_army
                                            ->m_creatureCounts[m_selectedArmySlot],
                                        m_town,
                                        1,
                                        0,
                                        1,
                                        viewHero,
                                        NULL,
                                        m_selectedStrip->m_army
                                    );
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
void townManager::DoCommand(i8 command) {
    hero* visitor;
    i32 temp;
    i16 i;
    hero* viewedHero;
    i32 single;

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
            gpGame->ViewArmy(
                TOWN_ARMY_VIEW_X,
                TOWN_ARMY_VIEW_Y,
                m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot],
                m_selectedStrip->m_army->m_creatureCounts[m_selectedArmySlot],
                m_town,
                single,
                0,
                0,
                viewedHero,
                NULL,
                m_selectedStrip->m_army
            );
            if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
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
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
            break;
        case TOWN_ARMY_COMMAND_GARRISON:
            gpGame->Overview();
            RedrawTownScreen();
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
            break;
        case TOWN_ARMY_COMMAND_SPLIT:
            SplitArmy();
            ResetStrips();
            break;
    }
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
}

// Buka TOWNMGR.cpp:1905-1921; HoMM1 redraws strips before the status text.
VA(0x0040ad2c, 0xa5)
void townManager::RedrawTownScreen(void) {
    tag_message message;

    DrawTown(1, 1);
    m_garrisonStrip->DrawIcons(1);
    m_heroStrip->DrawIcons(1);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_STATUS_TEXT_CONTROL);
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
void townManager::SplitArmy(void) {
    i16 messageId = 1;
    tag_message message;
    i16 merge;
    i16 numberId = TOWN_SPLIT_SETUP_AMOUNT_CONTROL;
    i16 n;

    m_heroWindow1 = new heroWindow(TOWN_SPLIT_WINDOW_X, TOWN_SPLIT_WINDOW_Y, "splitwin.bin");
    if (m_heroWindow1 == NULL)
        MemError();
    m_splitAmount = 0;
    m_splitMaximum = m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
    message.type = MESSAGE_WIDGET;
    sprintf(
        gText,
        "Move how many %s troops from %s to %s?",
        gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
        m_swapStrip == m_heroStrip ? "Hero's Army" : "Garrison",
        m_pendingStrip == m_heroStrip ? "Hero's Army" : "Garrison"
    );
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_SPLIT_PROMPT_CONTROL;
    message.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "%d", m_splitAmount);
    message.id = TOWN_SPLIT_SETUP_AMOUNT_CONTROL;
    message.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    gpWindowManager->DoDialog(m_heroWindow1, SplitArmyHandler, 0);
    delete m_heroWindow1;
    if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
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
void townManager::ShiftQualChange(void) {
    tag_message message;

    if (m_swapStrip != m_pendingStrip
        && (m_command == TOWN_ARMY_COMMAND_NONE || m_command == TOWN_ARMY_COMMAND_SPLIT
            || m_command == TOWN_ARMY_COMMAND_MERGE || m_command == TOWN_ARMY_COMMAND_SWAP))
        SetArmyCommand(gpInputManager->GetModifiers() & TOWN_SHIFT_QUALIFIER_MASK);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_STATUS_TEXT_CONTROL);
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow();
}

// donor PoL RVA 0x00017ab2; preferred Buka symbol ?ResetStrips@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.515580;margin=0.398432;shape=0.375;size=0.860;calls=1.000;alternate=pol20:void townManager::ResetStrips(void)@0x00017ab2
VA(0x0040b21d, 0xab)
void townManager::ResetStrips(void) {
    if (m_swapStrip)
        m_swapStrip->m_selectedSlot = STRIP_SLOT_NONE;
    if (m_pendingStrip)
        m_pendingStrip->m_selectedSlot = STRIP_SLOT_NONE;
    m_heroStrip->Draw();
    m_garrisonStrip->Draw();
    m_swapStrip = m_pendingStrip = NULL;
    m_swapArmySlot = m_pendingArmySlot = STRIP_SLOT_NONE;
}

// Buka TOWNMGR.cpp:1993-2003.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0040b2c8, 0x95)
void townManager::Toggle(i8 building) {
    i16 index;

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
void townManager::DrawTown(i8 updateScreen, i32 drawFlags) {
    i16 index;
    i16 x;
    i16 y;

    m_backgroundBitmap->DrawToBuffer(0, 0);
    for (index = 0; index < m_townObjectCount; index++)
        m_townObjects[index]->Draw(drawFlags);
    m_townWindow->DrawWindow(0, TOWN_REDRAW_FIRST_CONTROL, TOWN_REDRAW_LAST_CONTROL);
    gpMouseManager->MouseCoords(x, y);
    if (y < TOWN_VIEWPORT_HEIGHT)
        gpMouseManager->SaveAndDraw(gpWindowManager->m_screen, 0, 0, 1);
    if (updateScreen)
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            0,
            0,
            TOWN_VIEWPORT_WIDTH,
            TOWN_VIEWPORT_HEIGHT,
            0,
            0
        );
    if (y < TOWN_VIEWPORT_HEIGHT)
        gpMouseManager->RestoreUnderlying();
}

// Buka TOWNMGR.cpp BuyBuild; HoMM1 reads the mage/neutral/dwelling cost
// tables with asserts, sizes resource slots by the gold-icon width and
// draws the building through the castle frame of buybuil%d.bin.
VA(0x0040b455, 0x1023)
#line 1483 "D:\\Heroes\\Source\\TOWNMGR.CPP"
i16 townManager::BuyBuild(i16 building, i8 cannotBuy, i8 quickView) {
    u16 requirements;
    i32 yPos;
    i32 resIndex;
    char* descText;
    textWidget* amountWidgets[RESOURCE_COUNT];
    i32 nRowTypes[4];
    i32 row;
    i16 currX;
    i16 unusedTop;
    i32 totalWidth;
    i32 numLines;
    heroWindow* nBuildWindow;
    i16 unusedValue;
    i16 unusedField;
    i16 unusedControl;
    i16 firstRow;
    i32 space;
    tag_message iEvt;
    i8 resType[RESOURCE_COUNT];
    i32 binSize;
    i32 dwellIndex;
    iconWidget* resWidgets[RESOURCE_COUNT];
    i16 startX;
    font* iF;
    i16 unusedType;
    i16 nBottomCount;
    i32 inRow;
    i16 pResourceCount;
    i32 j;
    textWidget* descWidget;
    i32 iTotalHeight;
    i16 prices[RESOURCE_COUNT];
    i16 unusedKind;
    i32 nEntryWidth;
    i32 curCost;
    i32 numPrereqs;
    i16 unusedMode1;
    i32 iMageLevel;
    char* amountText[RESOURCE_COUNT];
    i32 baseY;

    iMageLevel = 0;
    j = 0;
    curCost = 0;
    descText = static_cast<char*>(malloc(300));
    for (j = 0; j < RESOURCE_COUNT; j++)
        resType[j] = prices[j] = RESOURCE_NONE;
    dwellIndex = -1;
    if (building > TOWN_NEUTRAL_BUILDING_LAST)
        dwellIndex =
            building - BUILDING_SLOT_DWELLING_FIRST + m_town->m_type * TOWN_DWELLINGS_PER_FACTION;
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            iMageLevel = gpTownManager->m_town->m_buildState + 1;
        else
            iMageLevel = 0;
        if (iMageLevel > TOWN_MAGE_GUILD_COST_LEVEL_LAST)
            iMageLevel = TOWN_MAGE_GUILD_COST_LEVEL_LAST;
        for (j = 0; j < RESOURCE_COUNT; j++) {
            if (gMageBuildingCosts[iMageLevel][j] > 0) {
                resType[curCost] = j;
                prices[curCost] = gMageBuildingCosts[iMageLevel][j];
                curCost++;
            }
        }
    } else if (building <= TOWN_NEUTRAL_BUILDING_LAST) {
        for (j = 0; j < RESOURCE_COUNT; j++) {
            // clang-format off
#line 1525
            ProcessAssert(building >= 0 && building < TOWN_NEUTRAL_BUILDING_COUNT, __FILE__, __LINE__);
            // clang-format on
#line 1526
            ProcessAssert(j >= 0 && j <= 6, __FILE__, __LINE__);
            if (gNeutralBuildingCosts[building][j] > 0) {
                resType[curCost] = j;
                prices[curCost] = gNeutralBuildingCosts[building][j];
                curCost++;
            }
        }
    } else {
        for (j = 0; j < RESOURCE_COUNT; j++) {
            // clang-format off
#line 1540
            ProcessAssert(dwellIndex >= 0 && dwellIndex < TOWN_DWELLING_COST_ROWS, __FILE__, __LINE__);
            // clang-format on
#line 1541
            ProcessAssert(j >= 0 && j <= 6, __FILE__, __LINE__);
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
    for (j = 0; j < RESOURCE_COUNT; j++) {
        if (resType[j] != RESOURCE_NONE)
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
    if (building <= TOWN_NEUTRAL_BUILDING_LAST)
        sprintf(descText, gNeutralBuildingDescriptions[building]);
    else
        sprintf(descText, gDwellingDescriptions[dwellIndex]);
    if (dwellIndex >= 0) {
        numPrereqs = 0;
        requirements = gDwellingRequirements
            [building - BUILDING_SLOT_DWELLING_FIRST + m_town->m_type * TOWN_DWELLINGS_PER_FACTION];
        for (j = 0; j < BUILDING_SLOT_REQUIREMENT_END; j++) {
            if (requirements & (1 << j)) {
                if (numPrereqs == 0)
                    strcat(descText, "\n\nRequires:");
                numPrereqs++;
                strcat(descText, "\n");
                if (j <= BUILDING_SLOT_STRUCTURE_LAST)
                    strcat(descText, gNeutralBuildingNames[j]);
                else
                    strcat(
                        descText,
                        gDwellingNames
                            [j - BUILDING_SLOT_DWELLING_FIRST
                             + m_town->m_type * TOWN_DWELLINGS_PER_FACTION]
                    );
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
    if (nBuildWindow == NULL)
        MemError();
    SetWinText(nBuildWindow, WINDOW_TEXT_BUILD);
    SET_WIDGET_MESSAGE(iEvt, WIDGET_COMMAND_SET_FRAME, BUY_BUILD_ICON_CONTROL);
    if (building >= BUILDING_SLOT_RACE_FIRST)
        iEvt.value = (gpTownManager->m_town->m_type + 1) * 7 + building - 6;
    else
        iEvt.value = building + 1;
    nBuildWindow->BroadcastMessage(iEvt);
    if (building == BUILDING_SLOT_MAGE_GUILD)
        sprintf(gText, "Mage Guild, Level %d", iMageLevel + 1);
    else
        strcpy(gText, GetBuildingName(building));
    iEvt.command = WIDGET_COMMAND_SET_TEXT;
    iEvt.id = BUY_BUILD_NAME_CONTROL;
    iEvt.text = gText;
    nBuildWindow->BroadcastMessage(iEvt);
    descWidget = new textWidget(
        0x18,
        baseY,
        0xee,
        (numLines << 4) + 6,
        descText,
        "bigfont.fnt",
        1,
        WIDGET_ID_NONE,
        8
    );
    if (descWidget == NULL)
        MemError();
    nBuildWindow->AddWidget(descWidget, WINDOW_Z_ORDER_APPEND);
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
                    while (resType[curCost] == RESOURCE_NONE)
                        curCost++;
                    nRowTypes[j] = resType[curCost];
                    curCost++;
                } else
                    nRowTypes[j] = RESOURCE_NONE;
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
                amountWidgets[resIndex] = new textWidget(
                    currX,
                    yPos + 32,
                    nEntryWidth,
                    12,
                    amountText[resIndex],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    8
                );
                if (amountWidgets[resIndex] == NULL)
                    MemError();
                resWidgets[resIndex] = new iconWidget(
                    currX,
                    yPos,
                    nEntryWidth,
                    12,
                    "resource.icn",
                    resType[resIndex],
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (resWidgets[resIndex] == NULL)
                    MemError();
                nBuildWindow->AddWidget(amountWidgets[resIndex], WINDOW_Z_ORDER_APPEND);
                nBuildWindow->AddWidget(resWidgets[resIndex], WINDOW_Z_ORDER_APPEND);
                resIndex++;
                currX = currX + space + nEntryWidth;
            }
        }
    }
    if (!quickView)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            TOWN_CLOSE_CONTROL,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
    m_selectedBuilding = TOWN_BUILDING_NONE;
    if (quickView) {
        iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
        iEvt.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        iEvt.id = DIALOG_BUTTON_2;
        nBuildWindow->BroadcastMessage(iEvt);
        iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
        iEvt.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        iEvt.id = DIALOG_BUTTON_1;
        nBuildWindow->BroadcastMessage(iEvt);
        iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
        iEvt.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        iEvt.id = 0;
        nBuildWindow->BroadcastMessage(iEvt);
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(nBuildWindow, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(nBuildWindow);
        gpMouseManager->ReallyShowPointer();
    } else {
        if (cannotBuy) {
            iEvt.command = WIDGET_COMMAND_CLEAR_FLAGS;
            iEvt.id = DIALOG_BUTTON_2;
            iEvt.value = WIDGET_FLAG_ENABLED;
            nBuildWindow->BroadcastMessage(iEvt);
            iEvt.command = WIDGET_COMMAND_SET_FLAGS;
            iEvt.id = DIALOG_BUTTON_2;
            iEvt.value = WIDGET_COMMAND_DIMMED;
            nBuildWindow->BroadcastMessage(iEvt);
        }
        gpWindowManager->DoDialog(nBuildWindow, TrueFalseDialogHandler, 0);
        if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
            m_selectedBuilding = building;
            for (j = 0; j < pResourceCount; j++)
                gpCurPlayer->m_resources[resType[j]] -= prices[j];
        }
    }
    if (!quickView)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            TOWN_CLOSE_CONTROL,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
    delete nBuildWindow;
    if (quickView)
        return 0;
    else
        return gpWindowManager->m_dialogResult == DIALOG_BUTTON_2;
}

// donor PoL RVA 0x00018bd2; preferred Buka symbol ?BuildObj@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.630156;margin=0.342681;shape=0.206;size=0.999;calls=0.933;strings=buildtwn.82M;alternate=pol20:void townManager::BuildObj(int)@0x00018bd2
// Buka TOWNMGR.cpp:2475; HoMM1 fizzles a fixed per-building rectangle
// instead of computing the drawn extent.
VA(0x0040c478, 0x3a0)
void townManager::BuildObj(i16 building) {
    i16 i;
    SAMPLE2 sample;

    gpMouseManager->ReallyHidePointer();
    DrawTown(1, 1);
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            m_town->m_buildState++;
        if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            m_town->GiveSpells();
    }
    m_town->m_buildings |= 1 << building;
    if (building >= BUILDING_SLOT_DWELLING_FIRST && building <= BUILDING_SLOT_DWELLING_LAST)
        m_town->m_garrison[building - BUILDING_SLOT_DWELLING_FIRST] =
            gMonsterDatabase[gDwellingType[m_town->m_type][building - BUILDING_SLOT_DWELLING_FIRST]]
                .growth;
    for (i = 0; i < m_townObjectCount; i++) {
        if (m_townObjects[i]->m_buildingId == building) {
            m_townObjects[i]->m_visible = 1;
            m_townObjects[i]->m_border->m_flags |= WIDGET_FLAG_ENABLED;
        }
    }
    if (building == BUILDING_SLOT_CASTLE) {
        m_town->m_buildings &= ~(1 << BUILDING_SLOT_TENT);
        for (i = 0; i < m_townObjectCount; i++) {
            if (m_townObjects[i]->m_buildingId == BUILDING_SLOT_TENT) {
                m_townObjects[i]->m_visible = 0;
                m_townObjects[i]->m_border->m_flags &= ~WIDGET_FLAG_ENABLED;
            }
        }
    }
    gpWindowManager->SaveFizzleSource(
        gTownBuildingExtents[m_town->m_type][building].x,
        gTownBuildingExtents[m_town->m_type][building].y,
        gTownBuildingExtents[m_town->m_type][building].width,
        gTownBuildingExtents[m_town->m_type][building].height
    );
    DrawTown(0, 1);
    sample = NULL_SAMPLE2;
    sample = LoadPlaySample("buildtwn.82M");
    gpWindowManager->FizzleForward(
        gTownBuildingExtents[m_town->m_type][building].x,
        gTownBuildingExtents[m_town->m_type][building].y,
        gTownBuildingExtents[m_town->m_type][building].width,
        gTownBuildingExtents[m_town->m_type][building].height,
        FIZZLE_USE_DEFAULT_DELAY
    );
    WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
    m_selectedBuilding = TOWN_BUILDING_NONE;
    m_bankBox->Update();
    m_townWindow->DrawWindow();
    gpMouseManager->ReallyShowPointer();
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_CLEAR_FLAGS,
        DIALOG_BUTTON_0,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    BitSet(gpGame->m_townBuiltToday, m_town->m_id);
    m_town->GiveSpells();
}

// Buka Castle.cpp SetupCastle; HoMM1 lays out five special buildings and
// six dwellings plus the hero-recruit slot with fixed frames.
VA(0x0040c818, 0x4b5)
void townManager::SetupCastle(class heroWindow* window) {
    i16 builtIcon = TOWN_CASTLE_FRAME_BUILT;
    i16 cannotBuild = TOWN_CASTLE_FRAME_CANNOT_BUILD;
    i16 noMoney = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    i16 i;
    tag_message message;
    i32 stateFrame;

    m_affordableBuildings = m_buildableBuildings = 0;
    for (i = 0; i < BUILDING_SLOT_COUNT; i++) {
        if (CanBuy(m_town, i))
            m_affordableBuildings |= 1 << i;
        if (CanBuild(m_town, i))
            m_buildableBuildings |= 1 << i;
    }
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        message.value = (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        message.id = i + TOWN_CASTLE_FIRST_DWELLING_NAME_CONTROL;
        message.text = GetBuildingName(i + BUILDING_SLOT_DWELLING_FIRST);
        window->BroadcastMessage(message);
    }
    for (i = 0; i < TOWN_CASTLE_SPECIAL_BUILDING_COUNT; i++) {
        stateFrame = TOWN_CASTLE_FRAME_NONE;
        if ((m_town->m_buildings & (1 << i))
            && (i != BUILDING_SLOT_MAGE_GUILD || m_town->m_buildState == MAGE_GUILD_STATE_LEVEL_4))
            stateFrame = TOWN_CASTLE_FRAME_BUILT;
        else if (!(m_buildableBuildings & (1 << i)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
        else if (!(m_affordableBuildings & (1 << i)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
        if (stateFrame != TOWN_CASTLE_FRAME_NONE) {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_STATE_CONTROL;
            message.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = stateFrame;
            window->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_STATE_CONTROL;
            message.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(message);
        }
    }
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        stateFrame = TOWN_CASTLE_FRAME_NONE;
        if (m_town->m_buildings & (1 << (i + BUILDING_SLOT_DWELLING_FIRST)))
            stateFrame = TOWN_CASTLE_FRAME_BUILT;
        else if (!(m_buildableBuildings & (1 << (i + BUILDING_SLOT_DWELLING_FIRST))))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
        else if (!(m_affordableBuildings & (1 << (i + BUILDING_SLOT_DWELLING_FIRST))))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
        if (stateFrame != TOWN_CASTLE_FRAME_NONE) {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            message.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = stateFrame;
            window->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            message.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(message);
        }
    }
    if (gpCurPlayer->m_resources[RESOURCE_GOLD] < gHeroGoldCost)
        stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    else if (gpCurPlayer->m_heroCount == PLAYER_HERO_CAPACITY
             || m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
        stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
    else if (m_recruitResult)
        stateFrame = TOWN_CASTLE_FRAME_BUILT;
    else
        stateFrame = TOWN_CASTLE_FRAME_NONE;
    message.id = TOWN_CASTLE_HERO_STATE_CONTROL;
    message.value = WIDGET_FLAG_DRAW;
    if (stateFrame != TOWN_CASTLE_FRAME_NONE) {
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
void townManager::SetupWell(class heroWindow* window) {
    i16 iconBase = TOWN_WELL_FIRST_ICON_CONTROL;
    i16 buildingName = TOWN_WELL_FIRST_NAME_CONTROL;
    i16 growthRate;
    i16 firstMonsterIcon = TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
    i16 creatureId = TOWN_WELL_FIRST_CREATURE_CONTROL;
    i16 firstAvailable = TOWN_WELL_FIRST_AVAILABLE_CONTROL;
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_ICON_CONTROL;
        message.value = (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(message);
        message.id = i + TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
        message.value = gDwellingType[m_town->m_type][i];
        window->BroadcastMessage(message);
    }
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        message.text = GetBuildingName(i + BUILDING_SLOT_DWELLING_FIRST);
        window->BroadcastMessage(message);
        message.id = i + TOWN_WELL_FIRST_CREATURE_CONTROL;
        strcpy(gText, gArmyNames[gDwellingType[m_town->m_type][i]]);
        gText[0] -= 'a' - 'A';
        message.text = gText;
        window->BroadcastMessage(message);
    }
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        message.id = i + TOWN_WELL_FIRST_AVAILABLE_CONTROL;
        if (!(m_town->m_buildings & (1 << (i + BUILDING_SLOT_DWELLING_FIRST))))
            strcpy(gText, "Available:\nNONE\nGrowth Rate:\nN/A");
        else {
            growthRate = gMonsterDatabase[gDwellingType[m_town->m_type][i]].growth;
            growthRate += WEEKLY_WELL_GROWTH_BONUS;
            sprintf(
                gText,
                "Available:\n%d\nGrowth Rate:\n%d/week",
                m_town->m_garrison[i],
                growthRate
            );
        }
        message.text = gText;
        window->BroadcastMessage(message);
    }
}

// Buka TOWNMGR.cpp:2597 SetupMage; HoMM1 shows nine guild spells, hiding
// the levels above the guild and stacking tower frames by level.
VA(0x0040cf1a, 0x331)
void townManager::SetupMage(class heroWindow* window) {
    i16 off = 0;
    i16 shown = 1;
    i16 iconFrame = 2;
    i16 messageId = TOWN_MAGE_DESCRIPTION_CONTROL;
    i16 slotBase = TOWN_MAGE_FIRST_SPELL_CONTROL;
    i16 iconOffset = TOWN_MAGE_FIRST_ICON_CONTROL;
    i16 nameBase = TOWN_MAGE_FIRST_NAME_CONTROL;
    i16 firstTower = TOWN_MAGE_FIRST_TOWER_CONTROL;
    tag_message message;
    i16 spellIndex;
    i32 spellState;

    message.type = MESSAGE_WIDGET;
    if (m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE) {
        strcpy(gText, "The above spells are available here.");
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.id = TOWN_MAGE_DESCRIPTION_CONTROL;
        message.text = gText;
        window->BroadcastMessage(message);
    }
    for (spellIndex = 0; spellIndex < TOWN_MAGE_GUILD_SPELL_COUNT; spellIndex++) {
        switch (spellIndex) {
            case 0:
            case 1:
            case 2:
                if (m_town->m_buildState >= MAGE_GUILD_STATE_LEVEL_1)
                    spellState = MAGE_GUILD_SPELL_FRAME_SHOWN;
                else
                    spellState = MAGE_GUILD_SPELL_FRAME_LOCKED;
                break;
            case 3:
            case 4:
                if (m_town->m_buildState >= MAGE_GUILD_STATE_LEVEL_2)
                    spellState = MAGE_GUILD_SPELL_FRAME_SHOWN;
                else
                    spellState = MAGE_GUILD_SPELL_FRAME_LOCKED;
                break;
            case 5:
            case 6:
                if (m_town->m_buildState >= MAGE_GUILD_STATE_LEVEL_3)
                    spellState = MAGE_GUILD_SPELL_FRAME_SHOWN;
                else
                    spellState = MAGE_GUILD_SPELL_FRAME_LOCKED;
                break;
            case 8:;
            default:
                if (m_town->m_buildState >= MAGE_GUILD_STATE_LEVEL_4)
                    spellState = MAGE_GUILD_SPELL_FRAME_SHOWN;
                else
                    spellState = MAGE_GUILD_SPELL_FRAME_LOCKED;
                break;
        }
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = spellIndex + TOWN_MAGE_FIRST_SPELL_CONTROL;
        message.value = spellState;
        window->BroadcastMessage(message);
        if (spellState == MAGE_GUILD_SPELL_FRAME_LOCKED) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
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
    message.value = WIDGET_FLAG_DRAW;
    for (spellIndex = 1; spellIndex < TOWN_MAGE_TOWER_FRAME_COUNT; spellIndex++) {
        message.id = spellIndex + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (spellIndex = 0; spellIndex < m_town->m_buildState; spellIndex++) {
        message.id =
            (spellIndex + 1) * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.id = m_town->m_buildState * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE
                 + TOWN_MAGE_FIRST_TOWER_CONTROL + 1;
    window->BroadcastMessage(message);
}

// Buka TOWNMGR.cpp:2735 MageGuildHandler; HoMM1 numbers spells 1-9 and
// icons 10-18 and bounds them by the guild level.
VA(0x0040d24b, 0x186)
i16 MageGuildHandler(struct tag_message& message) {
    i16 firstSpell = TOWN_MAGE_FIRST_SPELL_CONTROL;
    i16 iconBase = TOWN_MAGE_FIRST_ICON_CONTROL;
    i32 quickView;
    i32 spellId;
    i32 mageLevel;
    i32 spellPos;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                quickView = message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON;
                switch (message.id) {
                    case TOWN_MAGE_FIRST_SPELL_CONTROL:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 1:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 2:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 3:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 4:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 5:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 6:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 7:
                    case TOWN_MAGE_FIRST_SPELL_CONTROL + 8:
                        spellPos = message.id - TOWN_MAGE_FIRST_SPELL_CONTROL;
                        goto showSpell;
                    case TOWN_MAGE_FIRST_ICON_CONTROL:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 1:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 2:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 3:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 4:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 5:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 6:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 7:
                    case TOWN_MAGE_FIRST_ICON_CONTROL + 8:
                        spellPos = message.id - TOWN_MAGE_FIRST_ICON_CONTROL;
                    showSpell:
                        mageLevel = gpTownManager->m_town->m_buildState;
                        if ((mageLevel == MAGE_GUILD_STATE_LEVEL_1
                             && spellPos > MAGE_GUILD_LEVEL_1_LAST_SLOT)
                            || (mageLevel == MAGE_GUILD_STATE_LEVEL_2
                                && spellPos > MAGE_GUILD_LEVEL_2_LAST_SLOT)
                            || (mageLevel == MAGE_GUILD_STATE_LEVEL_3
                                && spellPos > MAGE_GUILD_LEVEL_3_LAST_SLOT))
                            return MESSAGE_DISPATCH_CONSUME;
                        spellId = gpTownManager->m_town->m_mageGuildSpells[spellPos];
                        NormalDialog(
                            gSpellDesc[spellId],
                            quickView ? NORMAL_DIALOG_TYPE_QUICK_VIEW : NORMAL_DIALOG_TYPE_OK,
                            -1,
                            -1,
                            NORMAL_DIALOG_SPELL,
                            spellId,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
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
void townManager::SetupThievesGuild(class heroWindow* window, i16 categories) {
    iconWidget* marker;
    i16 firstPlayer;
    i32 numThieves;
    i16 wUnusedRankX = THIEVES_RANK_FIRST_X;
    i16 iUnusedRankWidth = THIEVES_PLAYER_COLUMN_WIDTH;
    i16 top = THIEVES_FIRST_CATEGORY_Y;
    i16 rowSpacing = THIEVES_CATEGORY_ROW_HEIGHT;
    i16 frameBase = THIEVES_FLAG_FRAME_BASE;
    i16 pos;
    i16 lMarkWidth = THIEVES_RANK_ICON_WIDTH;
    i16 bIconHeight = THIEVES_RANK_ICON_HEIGHT;
    i16 bColWidth = THIEVES_PLAYER_WIDTH;
    i8 ranking[GAME_PLAYER_COUNT];
    i16 rank;
    i16 categoryIndex;
    i32 totals[GAME_PLAYER_COUNT];
    i16 startPos;
    i16 hi;
    i16 tied;

    if (categories == THIEVES_CATEGORIES_BY_GUILDS) {
        numThieves = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (numThieves >= 4)
            categories = THIEVES_CATEGORY_COUNT;
        else if (numThieves == 3)
            categories = THIEVES_CATEGORY_ARMY_STRENGTH;
        else if (numThieves == 2)
            categories = THIEVES_CATEGORY_RARE_RESOURCES;
        else
            categories = THIEVES_CATEGORY_GOLD;
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
            while (hi + 1 < gpGame->m_playerCount && totals[hi + 1] == totals[hi]) {
                tied++;
                hi++;
            }
            startPos = rank * THIEVES_PLAYER_COLUMN_WIDTH + THIEVES_RANK_FIRST_X
                       - (tied - 1) * THIEVES_TIE_CENTERING_STEP;
            for (pos = firstPlayer; !(pos > hi); pos++) {
                marker = new iconWidget(
                    (pos - firstPlayer) * THIEVES_RANK_ICON_WIDTH + startPos,
                    categoryIndex * THIEVES_CATEGORY_ROW_HEIGHT + THIEVES_FIRST_CATEGORY_Y,
                    THIEVES_RANK_ICON_WIDTH,
                    THIEVES_RANK_ICON_HEIGHT,
                    "townwind.icn",
                    gpGame->m_players[ranking[pos]].m_color + THIEVES_FLAG_FRAME_BASE,
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (marker == NULL)
                    MemError();
                window->AddWidget(marker, WINDOW_Z_ORDER_APPEND);
            }
            hi++;
            firstPlayer = hi;
        }
    }
}

// Buka TOWNMGR.cpp:3727-3833; HoMM1 has eight categories, sums three
// resources per row and counts obelisks through playerData.
VA(0x0040d6bd, 0x484)
void townManager::GetCategoryStats(i8 category, i32* const stats, i8* const order) {
    i16 townIndex;
    i16 index;
    i32 strength;
    i16 player;
    i16 numTowns;
    i16 numCastles;
    hero* playerHero;
    town* theTown;

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
                            && (gpGame->m_castleRecs[townIndex].m_buildings
                                & (1 << BUILDING_SLOT_TENT)))
                            numTowns++;
                    }
                    stats[player] = numTowns;
                    break;
                case THIEVES_CATEGORY_CASTLES:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gpGame->m_castleRecs[townIndex].m_owner == player
                            && (gpGame->m_castleRecs[townIndex].m_buildings
                                & (1 << BUILDING_SLOT_CASTLE)))
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
                    for (index = 0; index < gpGame->m_players[player].m_heroCount; index++) {
                        playerHero = gpGame->GetHero(gpGame->m_players[player].m_heroIds[index]);
                        strength +=
                            gpPhilAI->FightValueOfStack(&playerHero->m_army, playerHero, 0, 0, 0);
                    }
                    for (index = 0; index < gpGame->m_players[player].m_townCount; index++) {
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
void townManager::SortStats(i32* const stats, i8* const order) {
    i32 temp;
    i16 firstPlayer;
    i16 secondPlayer;
    i8 tempColor;

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
char* townManager::GetBuildingName(i16 building) {
    return ::GetBuildingName(m_town->m_type, building);
}

// Buka TOWNMGR.cpp RecruitHero; HoMM1's tavern shows both candidate heroes,
// a cannot-recruit view is a timed quick view, and the town strips are rebuilt.
VA(0x0040dc5a, 0x981)
i8 townManager::RecruitHero(i8 cannotRecruit) {
    tag_message message;
    i16 unusedButtonText = 1;
    i16 unusedDimState = 2;
    i16 unusedControlId = 3;
    i16 unusedPortraitState = 4;
    i16 unusedTextState = 6;
    i16 unusedPortraitControl = 7;
    i16 unusedButtonIcon = 8;
    i16 unusedMode = 9;

    m_heroWindow1 = new heroWindow(0xb1, 0x10, "rcrthero.bin");
    if (m_heroWindow1 == NULL)
        MemError();
    SetWinText(m_heroWindow1, WINDOW_TEXT_RECRUIT_HERO);
    m_recruitHeroes[0] = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[0]);
    m_recruitHeroes[1] = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[1]);
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = giCurPlayer;
    message.type = MESSAGE_WIDGET;
    if (cannotRecruit) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = RECRUIT_HERO_SELECT_FIRST;
        m_heroWindow1->BroadcastMessage(message);
        message.id = RECRUIT_HERO_SELECT_SECOND;
        m_heroWindow1->BroadcastMessage(message);
        message.id = DIALOG_BUTTON_1;
        m_heroWindow1->BroadcastMessage(message);
    }
    sprintf(gText, "port%04d.icn", m_recruitHeroes[0]->m_portrait);
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = RECRUIT_HERO_PORTRAIT_FIRST;
    message.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "port%04d.icn", m_recruitHeroes[1]->m_portrait);
    message.id = RECRUIT_HERO_PORTRAIT_SECOND;
    m_heroWindow1->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = RECRUIT_HERO_CLASS_FIRST;
    message.text = gClassNames[m_recruitHeroes[0]->m_heroClass];
    m_heroWindow1->BroadcastMessage(message);
    message.id = RECRUIT_HERO_CLASS_SECOND;
    message.text = gClassNames[m_recruitHeroes[1]->m_heroClass];
    m_heroWindow1->BroadcastMessage(message);
    m_recruitState = RECRUIT_HERO_NONE;
    if (cannotRecruit) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(m_heroWindow1, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(m_heroWindow1);
        gpMouseManager->ReallyShowPointer();
    } else
        gpWindowManager->DoDialog(m_heroWindow1, RecruitHeroHandler, 0);
    delete m_heroWindow1;
    if (m_recruitState != RECRUIT_HERO_NONE) {
        i32 newHeroClass;
        i16 townY;
        i16 townX;

        gpCurPlayer->m_resources[RESOURCE_GOLD] -= gHeroGoldCost;
        gpCurPlayer->m_heroIds[gpCurPlayer->m_heroCount] =
            gpCurPlayer->m_availableHeroIds[m_recruitState];
        gpCurPlayer->m_heroCount++;
        townX = m_town->m_x;
        townY = m_town->m_y;
        m_recruitHeroes[m_recruitState]->m_x = townX;
        m_recruitHeroes[m_recruitState]->m_y = townY;
        m_recruitHeroes[m_recruitState]->m_eventFlags = 0;
        m_recruitHeroes[m_recruitState]->m_direction = MAP_DIRECTION_EAST;
        m_recruitHeroes[m_recruitState]->m_remainingMobility =
            m_recruitHeroes[m_recruitState]->CalcMobility();
        m_recruitHeroes[m_recruitState]->m_mobility =
            m_recruitHeroes[m_recruitState]->m_remainingMobility;
        m_recruitHeroes[m_recruitState]->m_locationType = gpGame->m_map[townX][townY].m_triggerType;
        m_recruitHeroes[m_recruitState]->m_occupiedTown =
            gpGame->m_map[townX][townY].m_objectMetadata;
        gpGame->m_map[townX][townY].m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO);
        gpGame->m_map[townX][townY].m_objectMetadata =
            gpCurPlayer->m_availableHeroIds[m_recruitState];
        m_recruitResult = 1;
        m_town->m_occupyingHeroId = m_recruitHeroes[m_recruitState]->m_id;
        gpGame->m_availableHeroes[gpCurPlayer->m_availableHeroIds[m_recruitState]] = giCurPlayer;
        delete m_garrisonStrip;
        sprintf(
            gText,
            "crst%04d.icn",
            m_recruitHeroes[m_recruitState]->m_heroClass + gpCurPlayer->Color() * HERO_CLASS_COUNT
        );
        m_garrisonStrip = new strip(
            0,
            TOWN_GARRISON_STRIP_Y,
            m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE ? TOWN_CREST_FRAME_WITHOUT_HERO
                                                                  : TOWN_CREST_FRAME_WITH_HERO,
            gpResourceManager->MakeId(gText),
            0,
            &m_town->m_army,
            TOWN_GARRISON_FIRST_CONTROL,
            0
        );
        if (m_garrisonStrip == NULL)
            MemError();
        delete m_heroStrip;
        sprintf(gText, "port%04d.icn", m_recruitHeroes[m_recruitState]->m_portrait);
        m_heroStrip = new strip(
            0,
            TOWN_HERO_STRIP_Y,
            TOWN_HERO_STRIP_FRAME_COUNT,
            gpResourceManager->MakeId(gText),
            0,
            &m_recruitHeroes[m_recruitState]->m_army,
            TOWN_HERO_FIRST_CONTROL,
            0
        );
        if (m_heroStrip == NULL)
            MemError();
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            m_town->GiveSpells();
        newHeroClass = gpCurPlayer->m_availableHeroIds[1 - m_recruitState] / HERO_PER_CLASS_COUNT;
        newHeroClass = (newHeroClass + Random(1, 3)) % HERO_CLASS_COUNT;
        gpCurPlayer->m_availableHeroIds[m_recruitState] = gpGame->GetNewHeroId(newHeroClass);
        gpGame->m_availableHeroes[gpCurPlayer->m_availableHeroIds[m_recruitState]] =
            HERO_AVAILABILITY_RETREATED;
    } else {
        if (m_castleDialogActive)
            SetupCastle(m_heroWindow0);
        if (m_castleDialogActive)
            m_heroWindow0->DrawWindow();
    }
    m_bankBox->Update();
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_CLEAR_FLAGS,
        TOWN_CLOSE_CONTROL,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = GAME_PLAYER_NONE;
    if (m_recruitState != RECRUIT_HERO_NONE)
        m_recruitHeroes[m_recruitState]->m_owner = giCurPlayer;
    return gpWindowManager->m_dialogResult != DIALOG_BUTTON_1;
}

// donor PoL RVA 0x00019c29; preferred Buka symbol ?TavernHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.461090;margin=0.267847;shape=0.244;size=0.845;calls=1.000;alternate=pol20:int TavernHandler(struct tag_message &)@0x00019c29
// Buka TOWNMGR.cpp:2968-3000; HoMM1 animates frames 1-8 of control 2.
VA(0x0040e5db, 0x155)
i16 TavernHandler(struct tag_message& message) {
    i32 unusedDelay = TOWN_TAVERN_ANIMATION_DELAY;
    i16 unusedFrame = TOWN_TAVERN_UNUSED_FRAME;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                    case DIALOG_BUTTON_2:
                        FINISH_DIALOG_MESSAGE(message);
                        return MESSAGE_DISPATCH_FORWARD;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (glTimers[TOWN_FRAME_TIMER_SLOT] < KBTickCount()) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, TOWN_TAVERN_ANIMATION_CONTROL);
        ++gpGame->m_viewArmyResult;
        message.value = gpGame->m_viewArmyResult % TOWN_TAVERN_ANIMATION_FRAME_COUNT
                        + TOWN_TAVERN_FIRST_ANIMATION_FRAME;
        gpTownManager->m_heroWindow0->BroadcastMessage(message);
        gpTownManager->m_heroWindow0->MoveWindow(0, 0);
        glTimers[TOWN_FRAME_TIMER_SLOT] = KBTickCount() + TOWN_TAVERN_ANIMATION_DELAY;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x00019d7c; preferred Buka symbol ?DoTavern@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.728216;margin=0.164549;shape=0.467;size=0.965;calls=0.889;strings=tavwin.bin;alternate=pol20:void townManager::DoTavern(void)@0x00019d7c
// Buka TOWNMGR.cpp:3003-3032; HoMM1 plays the tavern theme instead of a
// rumour and restores the town theme afterwards.
VA(0x0040e730, 0x136)
void townManager::DoTavern(void) {
    i32 unusedValue = 0;

    m_heroWindow0 = new heroWindow(TOWN_TAVERN_WINDOW_X, TOWN_TAVERN_WINDOW_Y, "tavwin.bin");
    if (m_heroWindow0 == NULL)
        MemError();
    SetWinText(m_heroWindow0, WINDOW_TEXT_TAVERN);
    gpSoundManager->SwitchAmbientMusic(TOWN_TAVERN_MUSIC);
    gpWindowManager->DoDialog(m_heroWindow0, TavernHandler, 0);
    delete m_heroWindow0;
    gpSoundManager->SwitchAmbientMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE);
}

// Buka Castle.cpp CastleHandler; HoMM1 hovers by widget id, has no
// captain or formation controls and recruits a single hero (control 0x30).
VA(0x0040e866, 0x726)
i16 CastleHandler(struct tag_message& message) {
    i16 statusId = TOWN_CASTLE_STATUS_CONTROL;
    i32 result = 0;
    i32 quickFlag;
    i32 objNum;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpTownManager->m_lastHoverId)
                    break;
                gpTownManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case BUILDING_SLOT_MAGE_GUILD:
                        if (!(gpTownManager->m_buildableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_BUILD],
                                gpTownManager->GetBuildingName(message.id)
                            );
                        else if (!(gpTownManager->m_affordableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD],
                                gpTownManager->GetBuildingName(message.id)
                            );
                        else {
                            if (!(gpTownManager->m_town->m_buildings
                                  & (1 << BUILDING_SLOT_MAGE_GUILD)))
                                objNum = TOWN_CASTLE_INFO_BUILD_MAGE_GUILD;
                            else if (gpTownManager->m_town->m_buildState
                                     == MAGE_GUILD_STATE_LEVEL_4)
                                objNum = TOWN_CASTLE_INFO_MAGE_GUILD_MAX_LEVEL;
                            else if (!CanBuy(gpTownManager->m_town, BUILDING_SLOT_MAGE_GUILD))
                                objNum = TOWN_CASTLE_INFO_CANNOT_AFFORD_MAGE_LEVEL;
                            else
                                objNum = TOWN_CASTLE_INFO_ADD_MAGE_GUILD_LEVEL;
                            strcpy(gText, gCastleInfo[objNum]);
                        }
                        break;
                    case BUILDING_SLOT_THIEVES_GUILD:
                    case BUILDING_SLOT_TAVERN:
                    case BUILDING_SLOT_SHIPYARD:
                    case BUILDING_SLOT_WELL:
                    case BUILDING_SLOT_DWELLING_1:
                    case BUILDING_SLOT_DWELLING_2:
                    case BUILDING_SLOT_DWELLING_3:
                    case BUILDING_SLOT_DWELLING_4:
                    case BUILDING_SLOT_DWELLING_5:
                    case BUILDING_SLOT_DWELLING_6:
                        if (gpTownManager->m_town->m_buildings & (1 << message.id))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_ALREADY_BUILT],
                                gpTownManager->GetBuildingName(message.id)
                            );
                        else if (!(gpTownManager->m_buildableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_BUILD],
                                gpTownManager->GetBuildingName(message.id)
                            );
                        else if (!(gpTownManager->m_affordableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD],
                                gpTownManager->GetBuildingName(message.id)
                            );
                        else
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_BUILD],
                                gpTownManager->GetBuildingName(message.id)
                            );
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (gpCurPlayer->m_resources[RESOURCE_GOLD] < gHeroGoldCost)
                            strcpy(gText, gCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD_HERO]);
                        else if (gpCurPlayer->m_heroCount == PLAYER_HERO_CAPACITY)
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_TOO_MANY_HEROES],
                                PLAYER_HERO_CAPACITY
                            );
                        else if (gpTownManager->m_town->m_occupyingHeroId
                                 != TOWN_OCCUPYING_HERO_NONE)
                            strcpy(gText, gCastleInfo[TOWN_CASTLE_INFO_TOWN_OCCUPIED]);
                        else
                            strcpy(gText, gCastleInfo[TOWN_CASTLE_INFO_RECRUIT_HERO]);
                        break;
                    case DIALOG_BUTTON_0:
                        strcpy(gText, gCastleInfo[TOWN_CASTLE_INFO_EXIT]);
                        break;
                    default:
                        strcpy(gText, gCastleInfo[TOWN_CASTLE_INFO_OPTIONS]);
                        break;
                }
                message.command = WIDGET_COMMAND_SET_TEXT;
                message.id = TOWN_CASTLE_STATUS_TEXT_CONTROL;
                message.text = gText;
                gpTownManager->m_heroWindow0->BroadcastMessage(message);
                gpTownManager->m_heroWindow0->DrawWindow(
                    0,
                    TOWN_CASTLE_STATUS_FIRST_CONTROL,
                    TOWN_CASTLE_STATUS_TEXT_CONTROL
                );
                gpWindowManager->UpdateScreenRegion(
                    TOWN_CASTLE_STATUS_X,
                    TOWN_CASTLE_STATUS_Y,
                    TOWN_CASTLE_STATUS_WIDTH,
                    TOWN_CASTLE_STATUS_HEIGHT
                );
                return MESSAGE_DISPATCH_CONSUME;
            case WIDGET_NOTIFY_SELECT:
                if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                    quickFlag = 1;
                else
                    quickFlag = 0;
                switch (message.id) {
                    case BUILDING_SLOT_MAGE_GUILD:
                        if (!quickFlag
                            && (gpTownManager->m_town->m_buildState == MAGE_GUILD_STATE_LEVEL_4
                                || !(gpTownManager->m_buildableBuildings & (1 << message.id))))
                            break;
                        else
                            goto buy_building;
                    case BUILDING_SLOT_THIEVES_GUILD:
                    case BUILDING_SLOT_TAVERN:
                    case BUILDING_SLOT_SHIPYARD:
                    case BUILDING_SLOT_WELL:
                    case BUILDING_SLOT_DWELLING_1:
                    case BUILDING_SLOT_DWELLING_2:
                    case BUILDING_SLOT_DWELLING_3:
                    case BUILDING_SLOT_DWELLING_4:
                    case BUILDING_SLOT_DWELLING_5:
                    case BUILDING_SLOT_DWELLING_6:
                        if (!quickFlag
                            && ((gpTownManager->m_town->m_buildings & (1 << message.id))
                                || !(gpTownManager->m_buildableBuildings & (1 << message.id))))
                            break;
                    buy_building:
                        for (objNum = 0; objNum < gpTownManager->m_townObjectCount; objNum++) {
                            if (gpTownManager->m_townObjects[objNum]->m_buildingId == message.id)
                                break;
                        }
                        result = gpTownManager->BuyBuild(
                            message.id,
                            (gpTownManager->m_affordableBuildings & (1 << message.id)) == 0,
                            quickFlag
                        );
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (quickFlag)
                            gpTownManager->RecruitHero(1);
                        else if (!gpTownManager->m_recruitResult
                                 && gpCurPlayer->m_resources[RESOURCE_GOLD] >= gHeroGoldCost
                                 && gpCurPlayer->m_heroCount < PLAYER_HERO_CAPACITY
                                 && gpTownManager->m_town->m_occupyingHeroId
                                        == TOWN_OCCUPYING_HERO_NONE)
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
i16 SplitArmyHandler(struct tag_message& message) {
    i16 plusControl = TOWN_SPLIT_INCREASE_CONTROL;
    i32 unusedAction;
    i16 minusButton = TOWN_SPLIT_DECREASE_CONTROL;
    i16 amountText = TOWN_SPLIT_AMOUNT_CONTROL;
    i32 handled = 0;

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
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                        gpTownManager->m_splitAmount = 0;
                        gpWindowManager->m_dialogResult = message.id;
                        handled = 1;
                        break;
                    case DIALOG_BUTTON_2:
                        if (gpTownManager->m_splitAmount == 0)
                            gpWindowManager->m_dialogResult = DIALOG_BUTTON_1;
                        else
                            gpWindowManager->m_dialogResult = DIALOG_BUTTON_2;
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
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_SPLIT_AMOUNT_CONTROL);
    message.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    gpTownManager->m_heroWindow1->DrawWindow();
    return MESSAGE_DISPATCH_CONSUME;
}
// TOWNMGR's .rdata: Open's per-type town-object layout.
DATA(0x0048c028)
const i8 gTownObjectType[4][16] = {
    {5, 6, 8, 11, 7, 0, 1, 2, 10, 9, 3, 4, 12, -1, -1, -1},
    {5, 6, 12, 8, 0, 9, 10, 1, 2, 11, 3, 4, 7, -1, -1, -1},
    {13, 5, 6, 9, 7, 11, 0, 1, 2, 10, 8, 12, 3, 4, -1, -1},
    {5, 6, 12, 9, 0, 11, 10, 1, 2, 7, 3, 4, 8, -1, -1, -1},
};
