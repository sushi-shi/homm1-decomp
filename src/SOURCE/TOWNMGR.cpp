#include <match.h>

#include <BASE/audio.h>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x004cccbc)
static char s_pendingArmyName[1024];
DATA(0x004cd0bc)
static char s_armyCommandText[1024];
DATA(0x004cd4bc)
static char s_dwellingArmyName[1024];

// #line restores the original source file and line numbers of the asserts.

// Reads frame count, rectangle and building id from the .tod resource.
VA(0x0045ee90, 0x1c3)
townObject::townObject(char* name) {
    char fileNameText[16];
    i16 w;
    i16 temp;
    i16 id;
    i16 x;
    i16 h;
    i16 y;

    m_animationFrame = 0;
    m_icon = NULL;
    m_border = NULL;
    m_visible = 1;
    sprintf(fileNameText, "%s.tod", name);
    id = gpResourceManager->MakeId(fileNameText);
    gpResourceManager->PointToFile(id);
    m_animationFrameCount = gpResourceManager->ReadByte();
    x = gpResourceManager->ReadWord();
    y = gpResourceManager->ReadWord();
    w = gpResourceManager->ReadWord();
    h = gpResourceManager->ReadWord();
    id = gpResourceManager->ReadWord();
    m_buildingId = id;
    sprintf(fileNameText, "%s.icn", name);
    m_icon = gpResourceManager->GetIcon(fileNameText);
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

VA(0x0045f053, 0x56)
townObject::~townObject() {
    if (m_border != NULL)
        delete m_border;
    gpResourceManager->Dispose(m_icon);
}

// Draws the base frame, then the castle's mage-guild levels and the
// animation frame.
VA(0x0045f0a9, 0x103)
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

VA(0x0045f1ac, 0x69)
townManager::townManager(void) {
    m_town = NULL;
    m_townObjectCount = 0;
    m_heroWindow0 = NULL;
    m_coverWindow = NULL;
    m_selectedBuilding = TOWN_BUILDING_NONE;
    m_castleDialogActive = 0;
    m_dispatchMask = TOWN_MANAGER_DISPATCH_MASK;
}

// Retail vtable slot 0 (0x0048d468). Builds the town window, objects,
// strips and bank box.
VA(0x0045f215, 0x728)
i16 townManager::Open(i16 id) {
    i16 crestFrame;
    tag_message message;
    i16 i;
    i8 buildId;

    gpGame->CheckHeroConsistency();
    PlayMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE);
    PollSound();
    m_townWindow = new heroWindow(0, 0, "townwind.bin");
    if (m_townWindow == NULL)
        MemError();
    sprintf(gText, GetTownName(m_town->m_id));
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_NAME_TEXT_CONTROL);
    message.text = gText;
    m_townWindow->BroadcastMessage(message);
    strcpy(gText, localization::Tr("town.screen.title"));
    message.id = TOWN_STATUS_TEXT_CONTROL;
    message.text = gText;
    m_townWindow->BroadcastMessage(message);
    sprintf(gText, "townbkg%d.bmp", m_town->m_type);
    m_backgroundBitmap = gpResourceManager->GetBitmap(gText);
    m_townObjectCount = 0;
    for (i = 0; i < TOWN_MANAGER_OBJECT_CAPACITY; i++) {
        buildId = gTownObjectType[m_town->m_type][i];
        if (buildId != TOWN_OBJECT_NONE) {
            // One name table: neutral objects, four town-type prefixes, then
            // the faction-object suffixes.
            if (buildId < TOWN_FIRST_FACTION_OBJECT)
                strcpy(gText, gTownObjectNames[buildId]);
            else
                sprintf(
                    gText,
                    "%s%s",
                    gTownObjectNames[TOWN_FIRST_FACTION_OBJECT + m_town->m_type],
                    gTownObjectNames[buildId + TOWN_TYPE_COUNT]
                );
            m_townObjects[m_townObjectCount] = new townObject(gText);
            if (m_townObjects[m_townObjectCount] == NULL)
                MemError();
            if (m_townObjects[m_townObjectCount]->m_border) {
                if (!(m_town->m_buildings & (1 << buildId))) {
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
    crestFrame = gpCurPlayer->m_color;
    if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        crestFrame *= HERO_CLASS_COUNT;
        crestFrame += gpGame->GetHero(m_town->m_occupyingHeroId)->m_heroClass;
    } else
        crestFrame += TOWN_CREST_NO_HERO_OFFSET;
    sprintf(gText, "crst%04d.icn", crestFrame);
    m_garrisonStrip = new strip(
        0,
        TOWN_GARRISON_STRIP_Y,
        m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
            ? static_cast<i8>(TOWN_CREST_FRAME_WITHOUT_HERO)
            : static_cast<i8>(TOWN_CREST_FRAME_WITH_HERO),
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
    m_messageMask = BASE_MANAGER_ACCEPT_TOWN_EVENT;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "townManager");
    return BASE_MANAGER_SUCCESS;
}

// Retail vtable slot 1.
VA(0x0045f93d, 0x1db)
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
    StopMusic();
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpMouseManager->SetPointer(MOUSE_INVALID_CURSOR_FRAME);
    m_active = 0;
}

// Matches the dragged creature against every slot of the target army and
// keeps word-sized flags.
VA(0x0045fb18, 0x77c)
void townManager::SetArmyCommand(i16 qualifier) {
    i16 lastArmy;
    i16 i;
    i16 sameType;

    m_command = TOWN_ARMY_COMMAND_NONE;
    lastArmy = 0;
    if (m_swapStrip->m_army->GetNumArmies() == 1 && m_swapStrip == m_heroStrip
        && m_pendingStrip != m_swapStrip)
        lastArmy = 1;

    if (m_swapStrip != m_pendingStrip) {
        sameType = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_pendingStrip->m_army->m_creatureTypes[i]
                == m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot])
                sameType = 1;
        }
        if (sameType) {
            if (qualifier) {
                strcpy(
                    s_armyCommandText,
                    gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
                );
                s_armyCommandText[0] = CyrillicToLower(s_armyCommandText[0]);
                sprintf(m_statusText, gTownCommand[TOWN_TEXT_REDISTRIBUTE_ARMY], s_armyCommandText);
                m_command = TOWN_ARMY_COMMAND_SPLIT;
            } else if (lastArmy) {
                strcpy(s_armyCommandText, gTownCommand[TOWN_TEXT_CANNOT_COMBINE_LAST_ARMY]);
                s_armyCommandText[0] = CyrillicToLower(s_armyCommandText[0]);
                strcpy(m_statusText, s_armyCommandText);
                return;
            } else {
                strcpy(
                    s_armyCommandText,
                    gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
                );
                s_armyCommandText[0] = CyrillicToLower(s_armyCommandText[0]);
                sprintf(m_statusText, gTownCommand[TOWN_TEXT_COMBINE_ARMIES], s_armyCommandText);
                m_command = TOWN_ARMY_COMMAND_MERGE;
            }
        } else if (qualifier
                   && m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == CREATURE_NONE) {
            strcpy(
                s_armyCommandText,
                gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
            );
            s_armyCommandText[0] = CyrillicToLower(s_armyCommandText[0]);
            sprintf(
                m_statusText,
                gTownCommand[TOWN_TEXT_REDISTRIBUTE_TO_EMPTY_SLOT],
                s_armyCommandText
            );
            m_command = TOWN_ARMY_COMMAND_SPLIT;
        }
    } else if (m_swapArmySlot == m_pendingArmySlot) {
        sprintf(
            m_statusText,
            gTownCommand[TOWN_TEXT_VIEW_ARMY],
            gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
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
            strcpy(
                s_armyCommandText,
                gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
            );
            s_armyCommandText[0] = CyrillicToLower(s_armyCommandText[0]);
            sprintf(m_statusText, gTownCommand[TOWN_TEXT_MOVE_ARMY], s_armyCommandText);
            m_command = TOWN_ARMY_COMMAND_SWAP;
        }
    } else {
        strcpy(
            s_armyCommandText,
            gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]
        );
        s_armyCommandText[0] = CyrillicToLower(s_armyCommandText[0]);
        strcpy(
            s_pendingArmyName,
            gArmyNamesPlural[m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]]
        );
        s_pendingArmyName[0] = CyrillicToLower(s_pendingArmyName[0]);
        sprintf(
            m_statusText,
            gTownCommand[TOWN_TEXT_EXCHANGE_ARMIES],
            s_armyCommandText,
            s_pendingArmyName
        );
        m_command = TOWN_ARMY_COMMAND_SWAP;
    }
}

// Names the six dwellings through gDwellingType.
VA(0x00460294, 0x4dc)
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
                        gArmyNamesPlural[m_selectedStrip->m_army
                                             ->m_creatureTypes[m_selectedArmySlot]]
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
                        gArmyNamesPlural[m_selectedStrip->m_army
                                             ->m_creatureTypes[m_selectedArmySlot]]
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
            strcpy(
                s_dwellingArmyName,
                gArmyNamesPlural[gDwellingType[m_town->m_type][id - BUILDING_SLOT_DWELLING_FIRST]]
            );
            s_dwellingArmyName[0] = CyrillicToLower(s_dwellingArmyName[0]);
            sprintf(m_statusText, gTownCommand[TOWN_TEXT_DWELLING], s_dwellingArmyName);
            break;
    }
    ShowText(m_statusText);
}

VA(0x00460770, 0x6b)
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

// Opens the castle, mage guild, well and thieves guild over a bottom cover
// window, sells the spell book and builds boats.
VA(0x004607db, 0x11da)
i16 townManager::Main(struct tag_message& message) {
    i32 done;
    i8 rightButton;
    class sample* res;
    recruitUnit* recruitMgr;

    res = NULL;
    done = 0;
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        rightButton = 1;
    else
        rightButton = 0;
    if (glTimers[TOWN_FRAME_TIMER_SLOT] < KBTickCount()) {
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
                            if (rightButton) {
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
                            if (rightButton)
                                break;
                            gpWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_SET_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            m_coverWindow =
                                new heroWindow(0, 0x100, LOGICAL_SCREEN_WIDTH, 6, WINDOW_FLAG_SAVE_BACKGROUND);
                            if (m_coverWindow == NULL)
                                MemError();
                            gpWindowManager->AddWindow(m_coverWindow, WINDOW_Z_ORDER_APPEND, 1);
                            m_heroWindow0 = NULL;
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
                                                localization::Tr("town.spellbook.no_space"),
                                                NORMAL_DIALOG_TYPE_OK
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
                            if (m_heroWindow0 != NULL)
                                delete m_heroWindow0;
                            gpWindowManager->RemoveWindow(m_coverWindow);
                            delete m_coverWindow;
                            if (m_selectedBuilding != TOWN_BUILDING_NONE)
                                BuildObj(m_selectedBuilding);
                            if (m_recruitResult) {
                                hero* visitingHero;
                                i32 i;
                                i32 width;

                                gpMouseManager->ReallyHidePointer();
                                res = LoadPlaySample("buildtwn.82M");
                                visitingHero = gpGame->GetHero(m_town->m_occupyingHeroId);
                                width = 0;
                                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                                    if (visitingHero->m_army.m_creatureTypes[i] != CREATURE_NONE)
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
                                WaitSample(res);
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
                            if (rightButton)
                                break;
                            DoTavern();
                            break;
                        case BUILDING_SLOT_TENT:
                            if (rightButton)
                                return MESSAGE_DISPATCH_CONSUME;
                            if (BuyBuild(
                                    BUILDING_SLOT_CASTLE,
                                    !CanBuy(m_town, BUILDING_SLOT_CASTLE),
                                    rightButton
                                )) {
                                BuildObj(BUILDING_SLOT_CASTLE);
                                m_town->XformToCastle();
                            }
                            break;
                        case BUILDING_SLOT_SHIPYARD:
                            if (rightButton)
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
                                && gpAdvManager->m_cursorMapX != m_town->m_x - 1
                                && gpAdvManager->m_cursorMapY != m_town->m_y + 1) {
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
                                        res = LoadPlaySample("buildtwn.82M");
                                        gpGame->m_players[giCurPlayer].m_resources[RESOURCE_GOLD] -=
                                            TOWN_BOAT_GOLD_COST;
                                        gpGame->m_players[giCurPlayer].m_resources[RESOURCE_WOOD] -=
                                            TOWN_BOAT_WOOD_COST;
                                        m_bankBox->Update();
                                        WaitSample(res);
                                    }
                                }
                            } else
                                NormalDialog(
                                    localization::Tr("town.boat.unavailable"),
                                    NORMAL_DIALOG_TYPE_OK,
                                    0xd0,
                                    0x28
                                );
                            gpWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_CLEAR_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            break;
                        case TOWN_CLOSE_CONTROL:
                            if (rightButton)
                                break;
                            done++;
                            break;
                        default:
                            if (rightButton) {
                                i32 hasHero;
                                hero* viewHero;

                                hasHero = 0;
                                if (message.id >= TOWN_GARRISON_SLOT_FIRST
                                    && message.id <= TOWN_GARRISON_SLOT_FIRST + 4) {
                                    m_selectedStrip = m_garrisonStrip;
                                    m_selectedArmySlot = message.id - TOWN_GARRISON_SLOT_FIRST;
                                    hasHero = 1;
                                }
                                if (message.id >= TOWN_HERO_SLOT_FIRST
                                    && message.id <= TOWN_HERO_SLOT_FIRST + 4) {
                                    m_selectedStrip = m_heroStrip;
                                    m_selectedArmySlot = message.id - TOWN_HERO_SLOT_FIRST;
                                    hasHero = 1;
                                }
                                if (hasHero
                                    && m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]
                                           != CREATURE_NONE) {
                                    viewHero = m_selectedStrip == m_heroStrip
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
                                if (message.id >= TOWN_GARRISON_SLOT_FIRST
                                    && message.id <= TOWN_GARRISON_SLOT_LAST) {
                                    m_selectedStrip = m_garrisonStrip;
                                    m_selectedArmySlot = message.id - TOWN_GARRISON_SLOT_FIRST;
                                }
                                if (message.id >= TOWN_HERO_SLOT_FIRST
                                    && message.id <= TOWN_HERO_SLOT_LAST) {
                                    m_selectedStrip = m_heroStrip;
                                    m_selectedArmySlot = message.id - TOWN_HERO_SLOT_FIRST;
                                }
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
                    done++;
                    break;
            }
            break;
    }
    if (done == 1) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Merges duplicate stacks after a swap and opens the kingdom overview from
// the town.
VA(0x004619b5, 0x601)
void townManager::DoCommand(i8 command) {
    hero* visitor;
    i32 oldValue;
    i16 i;
    hero* viewedHero;

    switch (command) {
        case TOWN_ARMY_COMMAND_SELECT:
            m_swapStrip = m_selectedStrip;
            m_swapArmySlot = m_selectedArmySlot;
            m_swapStrip->m_selectedSlot = m_swapArmySlot;
            m_swapStrip->Draw();
            break;
        case TOWN_ARMY_COMMAND_VIEW:
            viewedHero =
                m_selectedStrip == m_heroStrip ? gpGame->GetHero(m_town->m_occupyingHeroId) : NULL;
            gpGame->ViewArmy(
                TOWN_ARMY_VIEW_X,
                TOWN_ARMY_VIEW_Y,
                m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot],
                m_selectedStrip->m_army->m_creatureCounts[m_selectedArmySlot],
                m_town,
                m_castleDialogActive == 1
                    || (m_selectedStrip == m_heroStrip
                        && m_selectedStrip->m_army->GetNumArmies() == 1),
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
                if (m_pendingStrip->m_army->m_creatureTypes[i]
                    == m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot])
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
            oldValue = m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot];
            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
            m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] = oldValue;
            oldValue = m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot];
            m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot];
            m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot] = oldValue;
            if (m_swapStrip != m_pendingStrip) {
                for (oldValue = 0; oldValue < ARMY_GROUP_SLOT_COUNT; oldValue++) {
                    if (m_pendingStrip->m_army->m_creatureTypes[oldValue]
                            == m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]
                        && oldValue != m_pendingArmySlot) {
                        m_pendingStrip->m_army->m_creatureCounts[oldValue] +=
                            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot];
                        m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] = CREATURE_NONE;
                        m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] = 0;
                    }
                    if (m_swapStrip->m_army->m_creatureTypes[oldValue]
                            == m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                        && oldValue != m_swapArmySlot) {
                        m_swapStrip->m_army->m_creatureCounts[oldValue] +=
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

// Redraws strips before the status text.
VA(0x00461fb6, 0x9d)
void townManager::RedrawTownScreen(void) {
    tag_message message;

    DrawTown(1, 1);
    m_garrisonStrip->DrawIcons(1);
    m_heroStrip->DrawIcons(1);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_STATUS_TEXT_CONTROL);
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0);
    gpWindowManager->UpdateScreenRegion(0, 0x100, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    m_bankBox->Update();
}

// Always names both armies and merges into the first matching slot of the
// target army.
VA(0x00462053, 0x334)
void townManager::SplitArmy(void) {
    i16 messageIdIndex = 1;
    tag_message message;
    i16 theMerge;
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
        localization::Tr("town.split.prompt"),
        gArmyNamesPlural[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
        m_swapStrip == m_heroStrip ? localization::Tr("town.split.hero_army")
                                   : localization::Tr("town.split.garrison"),
        m_pendingStrip == m_heroStrip ? localization::Tr("town.split.hero_army")
                                      : localization::Tr("town.split.garrison")
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
        theMerge = 0;
        for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
            if (m_pendingStrip->m_army->m_creatureTypes[n]
                == m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]) {
                theMerge = 1;
                break;
            }
        }
        if (theMerge)
            m_pendingStrip->m_army->m_creatureCounts[n] += m_splitAmount;
        else {
            m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot];
            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] = m_splitAmount;
        }
        m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] -= m_splitAmount;
    }
}

// Re-evaluates the pending strip command when the shift qualifier changes,
// then refreshes the status line.
VA(0x00462387, 0xb6)
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

VA(0x0046243d, 0x8b)
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

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x004624c8, 0x85)
void townManager::Toggle(i8 building) {
    i16 index;

    if (m_town->m_buildings & (1 << building)) {
        for (index = 0; index < m_townObjectCount; index++) {
            if (m_townObjects[index]->m_buildingId == building)
                m_townObjects[index]->m_visible ^= 1;
        }
    }
}

// Draws a bitmap background and folds the mouse pointer into the screen
// buffer around the viewport blit.
VA(0x0046254d, 0xe3)
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

// Reads the mage/neutral/dwelling cost tables with asserts, sizes resource
// slots by the gold-icon width and draws the building through the castle
// frame of buybuil%d.bin.
VA(0x00462630, 0xdd2)
#line 1483 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\TOWNMGR.CPP"
i16 townManager::BuyBuild(i16 building, i8 cannotBuy, i8 quickView) {
    i32 entryWidth;
    i16 buildCosts[RESOURCE_COUNT];
    textWidget* descriptionWidget;
    i16 dialogRight;
    i16 resourceX;
    i16 requiredCount;
    i32 dwelling;
    font* lineFont;
    i16 dialogLeft;
    i32 layoutSize;
    i32 guildRank;
    heroWindow* panel;
    i32 totalWidth;
    i16 dialogTop;
    i32 lineTotal;
    i32 panelExtent;
    i16 dialogFlags;
    i32 i;
    i32 space;
    i16 lowerResources;
    i32 rowY;
    i32 originY;
    i8 resourceTypes[RESOURCE_COUNT];
    i32 costCount;
    i32 inRow;
    i32 typeId;
    i16 startX;
    i16 dialogButton;
    i16 dialogWidth;
    i32 rowIndex;
    iconWidget* resWidgets[RESOURCE_COUNT];
    i32 typeList[4];
    i16 topResources;
    tag_message msg;
    char* infoBuffer;
    i16 dialogResult;
    textWidget* amountWidgets[RESOURCE_COUNT];
    char* amountText[RESOURCE_COUNT];

    guildRank = 0;
    i = 0;
    costCount = 0;
    infoBuffer = static_cast<char*>(malloc(300));
    for (i = 0; i < RESOURCE_COUNT; i++)
        resourceTypes[i] = buildCosts[i] = RESOURCE_NONE;
    dwelling = -1;
    if (building > TOWN_NEUTRAL_BUILDING_LAST)
        dwelling =
            building - BUILDING_SLOT_DWELLING_FIRST + m_town->m_type * TOWN_DWELLINGS_PER_FACTION;
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (m_town->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            guildRank = gpTownManager->m_town->m_buildState + 1;
        else
            guildRank = 0;
        if (guildRank > TOWN_MAGE_GUILD_COST_LEVEL_LAST)
            guildRank = TOWN_MAGE_GUILD_COST_LEVEL_LAST;
        for (i = 0; i < RESOURCE_COUNT; i++) {
            if (gMageBuildingCosts[guildRank][i] > 0) {
                resourceTypes[costCount] = i;
                buildCosts[costCount] = gMageBuildingCosts[guildRank][i];
                costCount++;
            }
        }
    } else if (building <= TOWN_NEUTRAL_BUILDING_LAST) {
        for (i = 0; i < RESOURCE_COUNT; i++) {
            // clang-format off
#line 1555
            H1_ASSERT(building >= 0 && building < TOWN_NEUTRAL_BUILDING_COUNT);
            // clang-format on
#line 1556
            H1_ASSERT(i >= 0 && i <= 6);
            if (gNeutralBuildingCosts[building][i] > 0) {
                resourceTypes[costCount] = i;
                buildCosts[costCount] = gNeutralBuildingCosts[building][i];
                costCount++;
            }
        }
    } else {
        for (i = 0; i < RESOURCE_COUNT; i++) {
            // clang-format off
#line 1570
            H1_ASSERT(dwelling >= 0 && dwelling < TOWN_DWELLING_COST_ROWS);
            // clang-format on
#line 1571
            H1_ASSERT(i >= 0 && i <= 6);
            if (gDwellingCosts[dwelling][i] > 0) {
                resourceTypes[costCount] = i;
                buildCosts[costCount] = gDwellingCosts[dwelling][i];
                costCount++;
            }
        }
    }
    dialogResult = 80;
    dialogRight = 40;
    dialogButton = 32;
    dialogLeft = 286;
    dialogFlags = 0;
    dialogTop = 2;
    dialogWidth = 3;
    typeId = 0;
    requiredCount = 0;
    topResources = 0;
    lowerResources = 0;
    for (i = 0; i < RESOURCE_COUNT; i++) {
        if (resourceTypes[i] != RESOURCE_NONE)
            requiredCount++;
    }
    if (requiredCount <= 4)
        topResources = requiredCount;
    else if (requiredCount == BUILD_RESOURCES_FIVE) {
        topResources = 2;
        lowerResources = 3;
    } else if (requiredCount == BUILD_RESOURCES_SIX) {
        topResources = 3;
        lowerResources = 3;
    } else if (requiredCount == BUILD_RESOURCES_SEVEN) {
        topResources = 3;
        lowerResources = 4;
    }
    if (building <= TOWN_NEUTRAL_BUILDING_LAST)
        sprintf(infoBuffer, gNeutralBuildingDescriptions[building]);
    else
        sprintf(infoBuffer, gDwellingDescriptions[dwelling]);
    if (dwelling >= 0) {
        u16 prerequisiteMask;
        i32 prerequisiteCount = 0;
        prerequisiteMask = gDwellingRequirements
            [m_town->m_type * TOWN_DWELLINGS_PER_FACTION
             + (building - BUILDING_SLOT_DWELLING_FIRST)];
        for (i = 0; i < BUILDING_SLOT_REQUIREMENT_END; i++) {
            if (prerequisiteMask & (1 << i)) {
                if (prerequisiteCount == 0)
                    strcat(infoBuffer, localization::Tr("town.build.requires"));
                prerequisiteCount++;
                strcat(infoBuffer, "\n");
                if (i <= BUILDING_SLOT_STRUCTURE_LAST)
                    strcat(infoBuffer, gNeutralBuildingNames[i]);
                else
                    strcat(
                        infoBuffer,
                        gDwellingNames
                            [i - BUILDING_SLOT_DWELLING_FIRST
                             + m_town->m_type * TOWN_DWELLINGS_PER_FACTION]
                    );
            }
        }
    }
    strcat(infoBuffer, "\n ");
    lineFont = gpResourceManager->GetFont("bigfont.fnt");
    lineTotal = lineFont->LineLength(infoBuffer, 0xee);
    gpResourceManager->Dispose(lineFont);
    originY = 0x97;
    panelExtent = originY;
    panelExtent += lineTotal << 4;
    if (requiredCount <= 4)
        panelExtent += 0x2c;
    else
        panelExtent += 0x58;
    if (!quickView)
        panelExtent += 0x27;
    layoutSize = (panelExtent - 0x35) / 0x2d;
    if (layoutSize < 3)
        layoutSize = 3;
    if (layoutSize > 7)
        layoutSize = 7;
    sprintf(gText, "buybuil%d.bin", layoutSize);
    panel = new heroWindow(0xb1, 0x10, gText);
    if (panel == NULL)
        MemError();
    SetWinText(panel, WINDOW_TEXT_BUILD);
    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_FRAME, BUY_BUILD_ICON_CONTROL);
    msg.value = building < BUILDING_SLOT_RACE_FIRST
                    ? building + 1
                    : (gpTownManager->m_town->m_type + 1) * 7 + building - 6;
    panel->BroadcastMessage(msg);
    if (building == BUILDING_SLOT_MAGE_GUILD)
        sprintf(gText, localization::Tr("town.build.mage_guild_level"), guildRank + 1);
    else
        strcpy(gText, GetBuildingName(building));
    msg.command = WIDGET_COMMAND_SET_TEXT;
    msg.id = BUY_BUILD_NAME_CONTROL;
    msg.text = gText;
    panel->BroadcastMessage(msg);
    descriptionWidget = new textWidget(
        0x18,
        originY,
        0xee,
        (lineTotal << 4) + 6,
        infoBuffer,
        "bigfont.fnt",
        1,
        WIDGET_ID_NONE,
        8
    );
    if (descriptionWidget == NULL)
        MemError();
    panel->AddWidget(descriptionWidget, WINDOW_Z_ORDER_APPEND);
    typeId = 0;
    for (rowIndex = 0; rowIndex < 2; rowIndex++) {
        rowY = lineTotal * 16 + originY + rowIndex * 44 + 12;
        inRow = rowIndex == 0 ? topResources : lowerResources;
        if (inRow > 0) {
            totalWidth = 0;
            costCount = typeId;
            for (i = 0; i < 4; i++) {
                if (i < inRow) {
                    while (resourceTypes[costCount] == RESOURCE_NONE)
                        costCount++;
                    typeList[i] = resourceTypes[costCount];
                    costCount++;
                } else
                    typeList[i] = RESOURCE_NONE;
            }
            for (i = 0; i < inRow; i++) {
                totalWidth += static_cast<i16>(typeList[i] == RESOURCE_GOLD ? 80 : 40);
            }
            space = (266 - totalWidth) / (inRow + 1);
            resourceX = startX = space + 10;
            for (i = 0; i < inRow; i++) {
                entryWidth = static_cast<i16>(typeList[i] == RESOURCE_GOLD ? 80 : 40);
                amountText[typeId] = static_cast<char*>(malloc(10));
                sprintf(amountText[typeId], "%d", buildCosts[typeId]);
                amountWidgets[typeId] = new textWidget(
                    resourceX,
                    rowY + 32,
                    entryWidth,
                    12,
                    amountText[typeId],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    8
                );
                if (amountWidgets[typeId] == NULL)
                    MemError();
                resWidgets[typeId] = new iconWidget(
                    resourceX,
                    rowY,
                    entryWidth,
                    12,
                    "resource.icn",
                    resourceTypes[typeId],
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (resWidgets[typeId] == NULL)
                    MemError();
                panel->AddWidget(amountWidgets[typeId], WINDOW_Z_ORDER_APPEND);
                panel->AddWidget(resWidgets[typeId], WINDOW_Z_ORDER_APPEND);
                typeId++;
                resourceX += entryWidth + space;
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
        msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
        msg.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        msg.id = DIALOG_BUTTON_2;
        panel->BroadcastMessage(msg);
        msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
        msg.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        msg.id = DIALOG_BUTTON_1;
        panel->BroadcastMessage(msg);
        msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
        msg.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        msg.id = 0;
        panel->BroadcastMessage(msg);
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(panel, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(panel);
        gpMouseManager->ReallyShowPointer();
    } else {
        if (cannotBuy) {
            msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
            msg.id = DIALOG_BUTTON_2;
            msg.value = WIDGET_FLAG_ENABLED;
            panel->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_SET_FLAGS;
            msg.id = DIALOG_BUTTON_2;
            msg.value = WIDGET_COMMAND_DIMMED;
            panel->BroadcastMessage(msg);
        }
        gpWindowManager->DoDialog(panel, TrueFalseDialogHandler, 0);
        if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
            m_selectedBuilding = building;
            for (i = 0; i < requiredCount; i++)
                gpCurPlayer->m_resources[resourceTypes[i]] -= buildCosts[i];
        }
    }
    if (!quickView)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            TOWN_CLOSE_CONTROL,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
    delete panel;
    if (quickView)
        return 0;
    else
        return gpWindowManager->m_dialogResult == DIALOG_BUTTON_2;
}

// Fizzles a fixed per-building rectangle.
VA(0x00463402, 0x35f)
void townManager::BuildObj(i16 building) {
    i16 i;
    class sample* sample;

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
    sample = LoadPlaySample("buildtwn.82M");
    gpWindowManager->FizzleForward(
        gTownBuildingExtents[m_town->m_type][building].x,
        gTownBuildingExtents[m_town->m_type][building].y,
        gTownBuildingExtents[m_town->m_type][building].width,
        gTownBuildingExtents[m_town->m_type][building].height,
        FIZZLE_USE_DEFAULT_DELAY
    );
    WaitSample(sample);
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

// Lays out five special buildings and six dwellings plus the hero-recruit
// slot with fixed frames.
VA(0x00463761, 0x45a)
void townManager::SetupCastle(class heroWindow* window) {
    i16 builtIcon = TOWN_CASTLE_FRAME_BUILT;
    i16 buildVal = TOWN_CASTLE_FRAME_CANNOT_BUILD;
    i16 noMoney = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    i16 i;
    tag_message msg;
    i32 stateFrame;

    m_affordableBuildings = m_buildableBuildings = 0;
    for (i = 0; i < BUILDING_SLOT_COUNT; i++) {
        if (CanBuy(m_town, i))
            m_affordableBuildings |= 1 << i;
        if (CanBuild(m_town, i))
            m_buildableBuildings |= 1 << i;
    }
    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        msg.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        msg.value = (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(msg);
    }
    msg.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        msg.id = i + TOWN_CASTLE_FIRST_DWELLING_NAME_CONTROL;
        msg.text = GetBuildingName(i + BUILDING_SLOT_DWELLING_FIRST);
        window->BroadcastMessage(msg);
    }
    for (i = 0; i < TOWN_CASTLE_SPECIAL_BUILDING_COUNT; i++) {
        stateFrame = TOWN_CASTLE_FRAME_NONE;
        if (TOWN_BUILDING_COMPLETE(*m_town, i))
            stateFrame = TOWN_CASTLE_FRAME_BUILT;
        else if (!(m_buildableBuildings & (1 << i)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
        else if (!(m_affordableBuildings & (1 << i)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
        if (stateFrame != TOWN_CASTLE_FRAME_NONE) {
            msg.command = WIDGET_COMMAND_SET_FLAGS;
            msg.id = i + TOWN_CASTLE_FIRST_STATE_CONTROL;
            msg.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.value = stateFrame;
            window->BroadcastMessage(msg);
        } else {
            msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
            msg.id = i + TOWN_CASTLE_FIRST_STATE_CONTROL;
            msg.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(msg);
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
            msg.command = WIDGET_COMMAND_SET_FLAGS;
            msg.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            msg.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.value = stateFrame;
            window->BroadcastMessage(msg);
        } else {
            msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
            msg.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            msg.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(msg);
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
    msg.id = TOWN_CASTLE_HERO_STATE_CONTROL;
    msg.value = WIDGET_FLAG_DRAW;
    if (stateFrame != TOWN_CASTLE_FRAME_NONE) {
        msg.command = WIDGET_COMMAND_SET_FLAGS;
        window->BroadcastMessage(msg);
        msg.command = WIDGET_COMMAND_SET_FRAME;
        msg.value = stateFrame;
        window->BroadcastMessage(msg);
    } else {
        msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
        window->BroadcastMessage(msg);
    }
}

// Six fixed dwellings; capitalises the creature name in gText.
VA(0x00463bbb, 0x2b5)
void townManager::SetupWell(class heroWindow* window) {
    i16 iconBase = TOWN_WELL_FIRST_ICON_CONTROL;
    i16 buildingName = TOWN_WELL_FIRST_NAME_CONTROL;
    i16 theRate;
    i16 firstMonsterIcon = TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
    i16 idPos = TOWN_WELL_FIRST_CREATURE_CONTROL;
    i16 firstAvailableVal = TOWN_WELL_FIRST_AVAILABLE_CONTROL;
    tag_message msg;
    i16 i;

    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        msg.id = i + TOWN_WELL_FIRST_ICON_CONTROL;
        msg.value = (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(msg);
        msg.id = i + TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
        msg.value = gDwellingType[m_town->m_type][i];
        window->BroadcastMessage(msg);
    }
    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        msg.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        msg.text = GetBuildingName(i + BUILDING_SLOT_DWELLING_FIRST);
        window->BroadcastMessage(msg);
        msg.id = i + TOWN_WELL_FIRST_CREATURE_CONTROL;
        strcpy(gText, gArmyNames[gDwellingType[m_town->m_type][i]]);
        gText[0] = CyrillicToUpper(gText[0]);
        msg.text = gText;
        window->BroadcastMessage(msg);
    }
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        msg.id = i + TOWN_WELL_FIRST_AVAILABLE_CONTROL;
        if (!(m_town->m_buildings & (1 << (i + BUILDING_SLOT_DWELLING_FIRST))))
            strcpy(gText, localization::Tr("town.well.empty"));
        else {
            theRate = gMonsterDatabase[gDwellingType[m_town->m_type][i]].growth;
            theRate += WEEKLY_WELL_GROWTH_BONUS;
            sprintf(gText, localization::Tr("town.well.growth"), m_town->m_garrison[i], theRate);
        }
        msg.text = gText;
        window->BroadcastMessage(msg);
    }
}

// Shows nine guild spells, hiding the levels above the guild and stacking
// tower frames by level.
VA(0x00463e70, 0x2bc)
void townManager::SetupMage(class heroWindow* window) {
    i16 curOff = 0;
    i16 shown = 1;
    i16 iconFrameNum = 2;
    i16 messageId = TOWN_MAGE_DESCRIPTION_CONTROL;
    i16 slotBase = TOWN_MAGE_FIRST_SPELL_CONTROL;
    i16 thisIconOffset = TOWN_MAGE_FIRST_ICON_CONTROL;
    i16 nameBase = TOWN_MAGE_FIRST_NAME_CONTROL;
    i16 firstTower = TOWN_MAGE_FIRST_TOWER_CONTROL;
    tag_message message;
    i16 spellNo;
    i32 spellStateVal;

    message.type = MESSAGE_WIDGET;
    if (m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE) {
        strcpy(gText, localization::Tr("town.mage.spells"));
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.id = TOWN_MAGE_DESCRIPTION_CONTROL;
        message.text = gText;
        window->BroadcastMessage(message);
    }
    for (spellNo = 0; spellNo < TOWN_MAGE_GUILD_SPELL_COUNT; spellNo++) {
        switch (spellNo) {
            case MAGE_GUILD_SLOT_LEVEL_1_FIRST:
            case MAGE_GUILD_SLOT_LEVEL_1_SECOND:
            case MAGE_GUILD_SLOT_LEVEL_1_THIRD:
                spellStateVal = static_cast<i16>(m_town->m_buildState < MAGE_GUILD_STATE_LEVEL_1);
                break;
            case MAGE_GUILD_SLOT_LEVEL_2_FIRST:
            case MAGE_GUILD_SLOT_LEVEL_2_SECOND:
                spellStateVal = static_cast<i16>(m_town->m_buildState < MAGE_GUILD_STATE_LEVEL_2);
                break;
            case MAGE_GUILD_SLOT_LEVEL_3_FIRST:
            case MAGE_GUILD_SLOT_LEVEL_3_SECOND:
                spellStateVal = static_cast<i16>(m_town->m_buildState < MAGE_GUILD_STATE_LEVEL_3);
                break;
            default:
                spellStateVal = static_cast<i16>(m_town->m_buildState < MAGE_GUILD_STATE_LEVEL_4);
                break;
        }
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = spellNo + TOWN_MAGE_FIRST_SPELL_CONTROL;
        message.value = spellStateVal;
        window->BroadcastMessage(message);
        if (spellStateVal == MAGE_GUILD_SPELL_FRAME_LOCKED) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            message.id = spellNo + TOWN_MAGE_FIRST_ICON_CONTROL;
            window->BroadcastMessage(message);
            message.id = spellNo + TOWN_MAGE_FIRST_NAME_CONTROL;
            window->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = spellNo + TOWN_MAGE_FIRST_ICON_CONTROL;
            message.value = m_town->m_mageGuildSpells[spellNo];
            window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = spellNo + TOWN_MAGE_FIRST_NAME_CONTROL;
            message.text = gSpellNames[m_town->m_mageGuildSpells[spellNo]];
            window->BroadcastMessage(message);
        }
    }
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (spellNo = 1; spellNo < TOWN_MAGE_TOWER_FRAME_COUNT; spellNo++) {
        message.id = spellNo + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (spellNo = 0; spellNo < m_town->m_buildState; spellNo++) {
        message.id =
            (spellNo + 1) * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.id = m_town->m_buildState * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE
                 + TOWN_MAGE_FIRST_TOWER_CONTROL + 1;
    window->BroadcastMessage(message);
}

// Numbers spells 1-9 and icons 10-18 and bounds them by the guild level.
VA(0x0046412c, 0x14b)
i16 MageGuildHandler(struct tag_message& message) {
    i16 firstSpell = TOWN_MAGE_FIRST_SPELL_CONTROL;
    i16 iconBaseVal = TOWN_MAGE_FIRST_ICON_CONTROL;
    i32 quickViewVal;
    i32 spellId;
    i32 theMageLevel;
    i32 spellPosIndex;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                quickViewVal = message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON;
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
                        spellPosIndex = message.id - TOWN_MAGE_FIRST_SPELL_CONTROL;
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
                        spellPosIndex = message.id - TOWN_MAGE_FIRST_ICON_CONTROL;
                    showSpell:
                        theMageLevel = gpTownManager->m_town->m_buildState;
                        if ((theMageLevel == MAGE_GUILD_STATE_LEVEL_1
                             && spellPosIndex > MAGE_GUILD_LEVEL_1_LAST_SLOT)
                            || (theMageLevel == MAGE_GUILD_STATE_LEVEL_2
                                && spellPosIndex > MAGE_GUILD_LEVEL_2_LAST_SLOT)
                            || (theMageLevel == MAGE_GUILD_STATE_LEVEL_3
                                && spellPosIndex > MAGE_GUILD_LEVEL_3_LAST_SLOT))
                            return MESSAGE_DISPATCH_CONSUME;
                        spellId = gpTownManager->m_town->m_mageGuildSpells[spellPosIndex];
                        NormalDialog(
                            gSpellDesc[spellId],
                            quickViewVal ? NORMAL_DIALOG_TYPE_QUICK_VIEW : NORMAL_DIALOG_TYPE_OK,
                            -1,
                            -1,
                            NORMAL_DIALOG_SPELL,
                            spellId
                        );
                        return MESSAGE_DISPATCH_CONSUME;
                }
        }
    }
    return EventWindowHandler(message);
}

// Draws only the ranking flags, with the category count taken from the
// number of guilds owned.
VA(0x00464277, 0x2b5)
void townManager::SetupThievesGuild(class heroWindow* window, i16 categories) {
    iconWidget* marker;
    i16 firstPlayer;
    i32 numThieves;
    i16 posX = THIEVES_RANK_FIRST_X;
    i16 theIUnusedRankWidth = THIEVES_PLAYER_COLUMN_WIDTH;
    i16 topNum = THIEVES_FIRST_CATEGORY_Y;
    i16 oldSpacing = THIEVES_CATEGORY_ROW_HEIGHT;
    i16 frameBase = THIEVES_FLAG_FRAME_BASE;
    i16 newPos;
    i16 savedLMarkWidth = THIEVES_RANK_ICON_WIDTH;
    i16 iconHeightOn = THIEVES_RANK_ICON_HEIGHT;
    i16 bColWidth = THIEVES_PLAYER_WIDTH;
    i8 baseRanking[GAME_PLAYER_COUNT];
    i16 bestRank;
    i16 categoryIndex;
    i32 savedTotals[GAME_PLAYER_COUNT];
    i16 savedPos;
    i16 realHi;
    i16 isTied;

    if (categories == THIEVES_CATEGORIES_BY_GUILDS) {
        numThieves = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (numThieves >= 4)
            categories = THIEVES_CATEGORY_COUNT;
        else if (numThieves == THIEVES_GUILDS_THREE)
            categories = THIEVES_CATEGORY_ARMY_STRENGTH;
        else if (numThieves == THIEVES_GUILDS_TWO)
            categories = THIEVES_CATEGORY_RARE_RESOURCES;
        else
            categories = THIEVES_CATEGORY_GOLD;
    }
    if (categories > THIEVES_CATEGORY_COUNT)
        categories = THIEVES_CATEGORY_COUNT;
    for (categoryIndex = 0; categoryIndex < categories; categoryIndex++) {
        GetCategoryStats(categoryIndex, savedTotals, baseRanking);
        SortStats(savedTotals, baseRanking);
        firstPlayer = 0;
        realHi = 0;
        for (bestRank = 0; bestRank < THIEVES_RANK_COUNT; bestRank++) {
            if (firstPlayer == gpGame->m_playerCount - gpGame->m_deadPlayerCount)
                break;
            isTied = 1;
            while (realHi + 1 < gpGame->m_playerCount
                   && savedTotals[realHi + 1] == savedTotals[realHi]) {
                isTied++;
                realHi++;
            }
            savedPos = bestRank * THIEVES_PLAYER_COLUMN_WIDTH + THIEVES_RANK_FIRST_X
                       - (isTied - 1) * THIEVES_TIE_CENTERING_STEP;
            for (newPos = firstPlayer; !(newPos > realHi); newPos++) {
                marker = new iconWidget(
                    savedPos + (newPos - firstPlayer) * THIEVES_RANK_ICON_WIDTH,
                    categoryIndex * THIEVES_CATEGORY_ROW_HEIGHT + THIEVES_FIRST_CATEGORY_Y,
                    THIEVES_RANK_ICON_WIDTH,
                    THIEVES_RANK_ICON_HEIGHT,
                    "townwind.icn",
                    gpGame->m_players[baseRanking[newPos]].m_color + THIEVES_FLAG_FRAME_BASE,
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (marker == NULL)
                    MemError();
                window->AddWidget(marker, WINDOW_Z_ORDER_APPEND);
            }
            realHi++;
            firstPlayer = realHi;
        }
    }
}

// Eight categories; sums three resources per row and counts obelisks
// through playerData.
VA(0x0046452c, 0x40b)
void townManager::GetCategoryStats(i8 category, i32* const stats, i8* const order) {
    i16 townIndex;
    i16 index;
    i32 strengthValue;
    i16 player;
    i16 firstNumTowns;
    i16 numCastles;
    hero* playerHeroData;
    town* townItem;

    for (player = 0; player < gpGame->m_playerCount; player++) {
        firstNumTowns = 0;
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
                            firstNumTowns++;
                    }
                    stats[player] = firstNumTowns;
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
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_WOOD]
                                    + gpGame->m_players[player].m_resources[RESOURCE_CRYSTAL]
                                    + gpGame->m_players[player].m_resources[RESOURCE_ORE];
                    break;
                case THIEVES_CATEGORY_RARE_RESOURCES:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_GEMS]
                                    + gpGame->m_players[player].m_resources[RESOURCE_SULFUR]
                                    + gpGame->m_players[player].m_resources[RESOURCE_MERCURY];
                    break;
                case THIEVES_CATEGORY_OBELISKS:
                    stats[player] = gpGame->m_players[player].CountVisitedObelisks();
                    break;
                case THIEVES_CATEGORY_ARMY_STRENGTH:
                    strengthValue = 0;
                    for (index = 0; index < gpGame->m_players[player].m_heroCount; index++) {
                        playerHeroData = gpGame->GetPlayerHero(player, index);
                        strengthValue +=
                            gpPhilAI->FightValueOfStack(&playerHeroData->m_army, playerHeroData, 0);
                    }
                    for (index = 0; index < gpGame->m_players[player].m_townCount; index++) {
                        townItem = gpGame->GetPlayerTown(player, index);
                        if (townItem->HasGarrison())
                            strengthValue +=
                                gpPhilAI->FightValueOfStack(&townItem->m_army, NULL, 0);
                    }
                    stats[player] = strengthValue;
                    break;
            }
        }
    }
}

VA(0x00464937, 0xeb)
void townManager::SortStats(i32* const stats, i8* const order) {
    i32 tempStat;
    i16 firstPlayer;
    i16 secondPlayer;
    i8 temporaryOrder;

    for (firstPlayer = 0; firstPlayer < gpGame->m_playerCount - 1; firstPlayer++) {
        for (secondPlayer = firstPlayer + 1; secondPlayer < gpGame->m_playerCount; secondPlayer++) {
            if (stats[secondPlayer] > stats[firstPlayer]) {
                tempStat = stats[firstPlayer];
                stats[firstPlayer] = stats[secondPlayer];
                stats[secondPlayer] = tempStat;
                temporaryOrder = order[firstPlayer];
                order[firstPlayer] = order[secondPlayer];
                order[secondPlayer] = temporaryOrder;
            }
        }
    }
}

// Town-type wrapper over the global building-name table lookup.
VA(0x00464a22, 0x25)
char* townManager::GetBuildingName(i16 building) {
    return ::GetBuildingName(m_town->m_type, building);
}

// The tavern shows both candidate heroes, a cannot-recruit view is a timed
// quick view, and the town strips are rebuilt.
VA(0x00464a47, 0x92f)
i8 townManager::RecruitHero(i8 cannotRecruit) {
    tag_message evtCopy;
    i16 unusedButtonTextVal = 1;
    i16 oldState = 2;
    i16 unusedControlId = 3;
    i16 curState = 4;
    i16 unusedTextState = 6;
    i16 unusedPortraitControl = 7;
    i16 curUnusedButtonIcon = 8;
    i16 unusedMode = 9;

    m_heroWindow1 = new heroWindow(0xb1, 0x10, "rcrthero.bin");
    if (m_heroWindow1 == NULL)
        MemError();
    SetWinText(m_heroWindow1, WINDOW_TEXT_RECRUIT_HERO);
    m_recruitHeroes[0] = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[0]);
    m_recruitHeroes[1] = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[1]);
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = giCurPlayer;
    evtCopy.type = MESSAGE_WIDGET;
    if (cannotRecruit) {
        evtCopy.command = WIDGET_COMMAND_CLEAR_FLAGS;
        evtCopy.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        evtCopy.id = RECRUIT_HERO_SELECT_FIRST;
        m_heroWindow1->BroadcastMessage(evtCopy);
        evtCopy.id = RECRUIT_HERO_SELECT_SECOND;
        m_heroWindow1->BroadcastMessage(evtCopy);
        evtCopy.id = DIALOG_BUTTON_1;
        m_heroWindow1->BroadcastMessage(evtCopy);
    }
    sprintf(gText, "port%04d.icn", m_recruitHeroes[0]->m_portrait);
    evtCopy.command = WIDGET_COMMAND_SET_ICON;
    evtCopy.id = RECRUIT_HERO_PORTRAIT_FIRST;
    evtCopy.text = gText;
    m_heroWindow1->BroadcastMessage(evtCopy);
    sprintf(gText, "port%04d.icn", m_recruitHeroes[1]->m_portrait);
    evtCopy.id = RECRUIT_HERO_PORTRAIT_SECOND;
    m_heroWindow1->BroadcastMessage(evtCopy);
    evtCopy.command = WIDGET_COMMAND_SET_TEXT;
    evtCopy.id = RECRUIT_HERO_CLASS_FIRST;
    evtCopy.text = gClassNames[m_recruitHeroes[0]->m_heroClass];
    m_heroWindow1->BroadcastMessage(evtCopy);
    evtCopy.id = RECRUIT_HERO_CLASS_SECOND;
    evtCopy.text = gClassNames[m_recruitHeroes[1]->m_heroClass];
    m_heroWindow1->BroadcastMessage(evtCopy);
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
            gpCurPlayer->Color() * HERO_CLASS_COUNT + m_recruitHeroes[m_recruitState]->m_heroClass
        );
        m_garrisonStrip = new strip(
            0,
            TOWN_GARRISON_STRIP_Y,
            m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                ? static_cast<i8>(TOWN_CREST_FRAME_WITHOUT_HERO)
                : static_cast<i8>(TOWN_CREST_FRAME_WITH_HERO),
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

// Animates frames 1-8 of control 2.
VA(0x00465376, 0x125)
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

// Plays the tavern theme and restores the town theme afterwards.
VA(0x0046549b, 0xff)
void townManager::DoTavern(void) {
    m_heroWindow0 = new heroWindow(TOWN_TAVERN_WINDOW_X, TOWN_TAVERN_WINDOW_Y, "tavwin.bin");
    if (m_heroWindow0 == NULL)
        MemError();
    SetWinText(m_heroWindow0, WINDOW_TEXT_TAVERN);
    PlayMusic(TOWN_TAVERN_MUSIC);
    gpWindowManager->DoDialog(m_heroWindow0, TavernHandler, 0);
    delete m_heroWindow0;
    PlayMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE);
}

// Hovers by widget id and recruits a single hero (control 0x30).
VA(0x0046559a, 0x650)
i16 CastleHandler(struct tag_message& message) {
    i16 statusId = TOWN_CASTLE_STATUS_CONTROL;
    i32 result = 0;
    i32 baseQuick;
    i32 objNum;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (gpTownManager->m_lastHoverId == message.id)
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
                baseQuick = (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) != 0;
                switch (message.id) {
                    case BUILDING_SLOT_MAGE_GUILD:
                        if (!baseQuick
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
                        if (!baseQuick
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
                            baseQuick
                        );
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (baseQuick)
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

// Handles the amount buttons on selection and redraws the whole split
// window.
VA(0x00465bea, 0x2cc)
i16 SplitArmyHandler(struct tag_message& message) {
    i16 plusControlNum = TOWN_SPLIT_INCREASE_CONTROL;
    i32 unusedActionVal;
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
DATA(0x0048a70c)
const i8 gTownObjectType[4][16] = {
    {5, 6, 8, 11, 7, 0, 1, 2, 10, 9, 3, 4, 12, -1, -1, -1},
    {5, 6, 12, 8, 0, 9, 10, 1, 2, 11, 3, 4, 7, -1, -1, -1},
    {13, 5, 6, 9, 7, 11, 0, 1, 2, 10, 8, 12, 3, 4, -1, -1},
    {5, 6, 12, 9, 0, 11, 10, 1, 2, 7, 3, 4, 8, -1, -1, -1},
};
