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
static char s_dwellingArmyLabel[1024];

// #line restores the original source file and line numbers of the asserts.

// Reads frame count, rectangle and building id from the .tod resource.
VA(0x0045ee90, 0x1c3)
townObject::townObject(char* name) {
    char fileNameText[16];
    i16 w;
    i16 temp;
    // The .tod resource id, then the building id the file names.
    H1_ENUM_SHARED(BuildingSlotType, i16) id;
    i16 x;
    i16 h;
    i16 y;

    m_animationFrame = 0;
    m_icon = NULL;
    m_border = NULL;
    m_visible = 1;
    sprintf(fileNameText, "%s.tod", name);
    id = gResourceManager->MakeId(fileNameText);
    gResourceManager->PointToFile(id);
    m_animationFrameCount = gResourceManager->ReadByte();
    x = gResourceManager->ReadWord();
    y = gResourceManager->ReadWord();
    w = gResourceManager->ReadWord();
    h = gResourceManager->ReadWord();
    id = gResourceManager->ReadWord();
    m_buildingId = id;
    sprintf(fileNameText, "%s.icn", name);
    m_icon = gResourceManager->GetIcon(fileNameText);
    if (id == BUILDING_SLOT_MAGE_GUILD) {
        h = gTownManager->m_town->m_buildState * TOWN_MAGE_GUILD_LEVEL_HEIGHT
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
    gResourceManager->Dispose(m_icon);
}

// Draws the base frame, then the castle's mage-guild levels and the
// animation frame.
VA(0x0045f0a9, 0x103)
void townObject::Draw(b8 advanceAnimation) {
    i16 level;

    if (!m_visible)
        return;
    m_icon->DrawToBuffer(0, 0, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    if (m_buildingId == BUILDING_SLOT_MAGE_GUILD) {
        for (level = 0; level < gTownManager->m_town->m_buildState; level++)
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
            gTownManager->m_town->m_buildState * TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE + 1,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
    }
    if (m_animationFrameCount) {
        m_icon->DrawToBuffer(0, 0, m_animationFrame + 1, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
        if (advanceAnimation == true) {
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
    m_selectedBuilding = BUILDING_SLOT_NONE;
    m_castleDialogActive = false;
    m_dispatchMask = TOWN_MANAGER_DISPATCH_MASK;
}

// Retail vtable slot 0 (0x0048d468). Builds the town window, objects,
// strips and bank box.
VA(0x0045f215, 0x728)
H1_ENUM_RETURN(BaseManagerStatus, i16) townManager::Open(i16 priority) {
    i16 crestFrame;
    tag_message message;
    i16 i;
    i8 buildId;

    gGame->CheckHeroConsistency();
    PlayMusic(gTownTheme[m_town->m_type] + MUSIC_TRACK_TOWN_FIRST);
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
    m_backgroundBitmap = gResourceManager->GetBitmap(gText);
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
                    gTownObjectNames
                        [TOWN_FIRST_FACTION_OBJECT + H1_ENUM_ENCODE(TownType, m_town->m_type)],
                    gTownObjectNames[buildId + H1_ENUM_ENCODE(TownType, TOWN_TYPE_COUNT)]
                );
            m_townObjects[m_townObjectCount] = new townObject(gText);
            if (m_townObjects[m_townObjectCount] == NULL)
                MemError();
            if (m_townObjects[m_townObjectCount]->m_border) {
                if (!(m_town->m_buildings
                      & H1_ENUM_BIT(BuildingSlotType, H1_ENUM_DECODE(BuildingSlotType, buildId)))) {
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
    gTimers[TOWN_FRAME_TIMER_SLOT] = KBTickCount() + TOWN_REDRAW_INTERVAL;
    gWindowManager->AddWindow(m_townWindow, WINDOW_Z_ORDER_BOTTOM, 1);
    crestFrame = H1_ENUM_ENCODE(PlayerColor, gCurPlayerData->m_color);
    if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        crestFrame *= HERO_CLASS_COUNT;
        crestFrame += gGame->GetHero(m_town->m_occupyingHeroId)->m_heroClass;
    } else
        crestFrame += TOWN_CREST_NO_HERO_OFFSET;
    sprintf(gText, "crst%04d.icn", crestFrame);
    m_garrisonStrip = new strip(
        0,
        TOWN_GARRISON_STRIP_Y,
        m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
            ? static_cast<i8>(TOWN_CREST_FRAME_WITHOUT_HERO)
            : static_cast<i8>(TOWN_CREST_FRAME_WITH_HERO),
        gResourceManager->MakeId(gText),
        0,
        &m_town->m_army,
        TOWN_GARRISON_FIRST_CONTROL,
        1
    );
    if (m_garrisonStrip == NULL)
        MemError();
    if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        sprintf(gText, "port%04d.icn", gGame->GetHero(m_town->m_occupyingHeroId)->m_portrait);
        m_heroStrip = new strip(
            0,
            TOWN_HERO_STRIP_Y,
            TOWN_HERO_STRIP_FRAME_COUNT,
            gResourceManager->MakeId(gText),
            0,
            &gGame->GetHero(m_town->m_occupyingHeroId)->m_army,
            TOWN_HERO_FIRST_CONTROL,
            1
        );
        if (m_heroStrip == NULL)
            MemError();
        if (m_town->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))
            m_town->GiveSpells();
    } else {
        m_heroStrip = new strip(
            0,
            TOWN_HERO_STRIP_Y,
            TOWN_HERO_STRIP_FRAME_COUNT,
            gResourceManager->MakeId("strip.icn"),
            TOWN_EMPTY_HERO_PORTRAIT_FRAME,
            NULL,
            -1,
            1
        );
        if (m_heroStrip == NULL)
            MemError();
    }
    m_bankBox = new bankBox(TOWN_BANK_BOX_X, TOWN_BANK_BOX_Y, gCurPlayerData);
    if (m_bankBox == NULL)
        MemError();
    m_selectedStrip = m_swapStrip = m_pendingStrip = NULL;
    m_selectedArmySlot = m_swapArmySlot = m_pendingArmySlot = STRIP_SLOT_NONE;
    DrawTown(false, false);
    gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gMouseManager->ReallyShowPointer();
    gMouseManager->NewUpdate(true);
    KBChangeMenu(gTownMenu);
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
    m_castleDialogActive = false;
    m_recruitResult = false;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    m_messageMask = BASE_MANAGER_ACCEPT_TOWN_EVENT;
    m_priority = priority;
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
    gResourceManager->Dispose(m_backgroundBitmap);
    gWindowManager->RemoveWindow(m_townWindow);
    delete m_townWindow;
    StopMusic();
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    gMouseManager->SetPointer(MOUSE_INVALID_CURSOR_FRAME);
    m_active = 0;
}

// Matches the dragged creature against every slot of the target army and
// keeps word-sized flags.
VA(0x0045fb18, 0x77c)
void townManager::SetArmyCommand(H1_ENUM_PARAM(MessageModifier, i16) qualifier) {
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
    // A town object's border carries its building slot as the widget id.
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
                SetArmyCommand(message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS);
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
                SetArmyCommand(message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS);
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
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0
                     + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_THIEVES_GUILD):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0
                     + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_THIEVES_GUILD)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TAVERN):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0 + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TAVERN)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_SHIPYARD):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0
                     + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_SHIPYARD)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_WELL):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0 + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_WELL)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TENT):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0 + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TENT)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_CASTLE):
            strcpy(
                m_statusText,
                gTownCommand
                    [TOWN_TEXT_BUILDING_0 + H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_CASTLE)]
            );
            break;
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_1):
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_2):
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_3):
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_4):
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_5):
        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_6):
            strcpy(
                s_dwellingArmyLabel,
                gArmyNamesPlural[gDwellingType[m_town->m_type]
                                              [H1_ENUM_DECODE(BuildingSlotType, id)
                                               - BUILDING_SLOT_DWELLING_FIRST]]
            );
            s_dwellingArmyLabel[0] = CyrillicToLower(s_dwellingArmyLabel[0]);
            sprintf(m_statusText, gTownCommand[TOWN_TEXT_DWELLING], s_dwellingArmyLabel);
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
    gWindowManager->UpdateScreenRegion(
        0,
        TOWN_STATUS_REGION_Y,
        TOWN_STATUS_REGION_WIDTH,
        TOWN_STATUS_REGION_HEIGHT
    );
}

// Opens the castle, mage guild, well and thieves guild over a bottom cover
// window, sells the spell book and builds boats.
VA(0x004607db, 0x11da)
H1_ENUM_RETURN(MessageDispatchResult, i16) townManager::Main(struct tag_message& message) {
    i32 done;
    b8 rightButton;
    class sample* res;
    recruitUnit* recruitMgr;

    res = NULL;
    done = 0;
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        rightButton = true;
    else
        rightButton = false;
    if (gTimers[TOWN_FRAME_TIMER_SLOT] < KBTickCount()) {
        DrawTown(true, true);
        gTimers[TOWN_FRAME_TIMER_SLOT] = KBTickCount() + TOWN_REDRAW_INTERVAL;
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
                    // A town object's border carries its building slot as the
                    // widget id.
                    switch (message.id) {
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_1):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_2):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_3):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_4):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_5):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_6):
                            if (rightButton) {
                                QuickViewRecruit(
                                    m_town,
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                        - BUILDING_SLOT_DWELLING_FIRST
                                );
                                break;
                            }
                            gMouseManager->ReallyHidePointer();
                            DrawTown(true, true);
                            recruitMgr = new recruitUnit(
                                m_town,
                                H1_ENUM_DECODE(BuildingSlotType, message.id)
                                    - BUILDING_SLOT_DWELLING_FIRST
                            );
                            if (recruitMgr == NULL)
                                MemError();
                            gExec->DoDialog(recruitMgr);
                            delete recruitMgr;
                            break;
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_THIEVES_GUILD):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_WELL):
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_CASTLE):
                            if (rightButton)
                                break;
                            gWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_SET_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            m_coverWindow = new heroWindow(
                                0,
                                0x100,
                                LOGICAL_SCREEN_WIDTH,
                                6,
                                WINDOW_FLAG_SAVE_BACKGROUND
                            );
                            if (m_coverWindow == NULL)
                                MemError();
                            gWindowManager->AddWindow(m_coverWindow, WINDOW_Z_ORDER_APPEND, 1);
                            m_heroWindow0 = NULL;
                            switch (H1_ENUM_DECODE(BuildingSlotType, message.id)) {
                                case BUILDING_SLOT_CASTLE:
                                    gWindowManager->SaveFizzleSource(0, 0x100, 0x228, 0xcc);
                                    m_heroWindow0 = new heroWindow(0, 0, "caslwind.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetWinText(m_heroWindow0, WINDOW_TEXT_CASTLE);
                                    SetupCastle(m_heroWindow0);
                                    m_castleDialogActive = true;
                                    gWindowManager->DoDialog(m_heroWindow0, CastleHandler, false);
                                    m_castleDialogActive = false;
                                    break;
                                case BUILDING_SLOT_MAGE_GUILD:
                                    if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE
                                        && !gGame->GetHero(m_town->m_occupyingHeroId)
                                                ->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                                        if (gGame->GetHero(m_town->m_occupyingHeroId)
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
                                            if (gGame
                                                    ->m_players
                                                        [gGame->GetHero(m_town->m_occupyingHeroId)
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
                                            gWindowManager->DoDialog(
                                                m_heroWindow0,
                                                TrueFalseDialogHandler,
                                                false
                                            );
                                            if (gWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
                                                gAdvManager->GiveArtifact(
                                                    gGame->GetHero(m_town->m_occupyingHeroId),
                                                    ARTIFACT_MAGIC_BOOK
                                                );
                                                gCurPlayerData->m_resources[RESOURCE_GOLD] -=
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
                                        gWindowManager
                                            ->DoDialog(m_heroWindow0, MageGuildHandler, false);
                                    }
                                    m_town->GiveSpells();
                                    break;
                                case BUILDING_SLOT_WELL:
                                    m_heroWindow0 = new heroWindow(0, 0, "wellwind.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetupWell(m_heroWindow0);
                                    gWindowManager
                                        ->DoDialog(m_heroWindow0, TrueFalseDialogHandler, false);
                                    break;
                                case BUILDING_SLOT_THIEVES_GUILD:
                                    m_heroWindow0 = new heroWindow(0, 0, "thiefwin.bin");
                                    if (m_heroWindow0 == NULL)
                                        MemError();
                                    SetWinText(m_heroWindow0, WINDOW_TEXT_THIEVES_GUILD);
                                    SetupThievesGuild(m_heroWindow0, THIEVES_CATEGORIES_BY_GUILDS);
                                    gWindowManager
                                        ->DoDialog(m_heroWindow0, TrueFalseDialogHandler, false);
                                    break;
                            }
                            if (m_heroWindow0 != NULL)
                                delete m_heroWindow0;
                            gWindowManager->RemoveWindow(m_coverWindow);
                            delete m_coverWindow;
                            if (m_selectedBuilding != BUILDING_SLOT_NONE)
                                BuildObj(m_selectedBuilding);
                            if (m_recruitResult) {
                                hero* visitingHero;
                                i32 i;
                                i32 width;

                                gMouseManager->ReallyHidePointer();
                                res = LoadPlaySample("buildtwn.82M");
                                visitingHero = gGame->GetHero(m_town->m_occupyingHeroId);
                                width = 0;
                                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                                    if (visitingHero->m_army.m_creatureTypes[i] != CREATURE_NONE)
                                        width = i + 1;
                                }
                                width = width * 88 + 0x70;
                                DrawTown(true, true);
                                gWindowManager->FizzleForward(
                                    0,
                                    0x100,
                                    width,
                                    0xcc,
                                    FIZZLE_USE_DEFAULT_DELAY
                                );
                                WaitSample(res);
                                m_recruitResult = false;
                                gMouseManager->ReallyShowPointer();
                            }
                            gWindowManager->ReleaseFizzleSource();
                            gWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_CLEAR_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            break;
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TAVERN):
                            if (rightButton)
                                break;
                            DoTavern();
                            break;
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TENT):
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
                        case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_SHIPYARD):
                            if (rightButton)
                                break;
                            gWindowManager->BroadcastMessage(
                                MESSAGE_WIDGET,
                                WIDGET_COMMAND_SET_FLAGS,
                                TOWN_CLOSE_CONTROL,
                                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
                            );
                            if (gGame->GetBoatsBuilt() < GAME_BOAT_COUNT
                                && gAdvManager->GetCell(m_town->m_x - 1, m_town->m_y + 1)
                                           ->m_triggerType
                                       == MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE)
                                && gAdvManager->m_cursorMapX != m_town->m_x - 1
                                && gAdvManager->m_cursorMapY != m_town->m_y + 1) {
                                m_heroWindow0 = new heroWindow(0xb1, 0x14, "shipwind.bin");
                                if (m_heroWindow0 == NULL)
                                    MemError();
                                SetWinText(m_heroWindow0, WINDOW_TEXT_SHIPYARD);
                                if (gGame->m_players[gCurPlayer].m_resources[RESOURCE_GOLD]
                                        < TOWN_BOAT_GOLD_COST
                                    || gGame->m_players[gCurPlayer].m_resources[RESOURCE_WOOD]
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
                                gWindowManager
                                    ->DoDialog(m_heroWindow0, TrueFalseDialogHandler, false);
                                delete m_heroWindow0;
                                if (gWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
                                    if (gGame->CreateBoat(m_town->m_x - 1, m_town->m_y + 1)
                                        != GAME_TABLE_FREE) {
                                        res = LoadPlaySample("buildtwn.82M");
                                        gGame->m_players[gCurPlayer].m_resources[RESOURCE_GOLD] -=
                                            TOWN_BOAT_GOLD_COST;
                                        gGame->m_players[gCurPlayer].m_resources[RESOURCE_WOOD] -=
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
                            gWindowManager->BroadcastMessage(
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
                                b32 isArmySlot;
                                hero* viewHero;

                                isArmySlot = false;
                                if (message.id >= TOWN_GARRISON_SLOT_FIRST
                                    && message.id <= TOWN_GARRISON_SLOT_FIRST + 4) {
                                    m_selectedStrip = m_garrisonStrip;
                                    m_selectedArmySlot = message.id - TOWN_GARRISON_SLOT_FIRST;
                                    isArmySlot = true;
                                }
                                if (message.id >= TOWN_HERO_SLOT_FIRST
                                    && message.id <= TOWN_HERO_SLOT_FIRST + 4) {
                                    m_selectedStrip = m_heroStrip;
                                    m_selectedArmySlot = message.id - TOWN_HERO_SLOT_FIRST;
                                    isArmySlot = true;
                                }
                                if (isArmySlot
                                    && m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]
                                           != CREATURE_NONE) {
                                    viewHero = m_selectedStrip == m_heroStrip
                                                   ? gGame->GetHero(m_town->m_occupyingHeroId)
                                                   : NULL;
                                    gGame->ViewArmy(
                                        TOWN_ARMY_VIEW_X,
                                        TOWN_ARMY_VIEW_Y,
                                        m_selectedStrip->m_army
                                            ->m_creatureTypes[m_selectedArmySlot],
                                        m_selectedStrip->m_army
                                            ->m_creatureCounts[m_selectedArmySlot],
                                        m_town,
                                        true,
                                        ARMY_FACING_RIGHT,
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
void townManager::DoCommand(H1_ENUM_PARAM(TownArmyCommand, i8) command) {
    hero* visitor;
    // The swap's held count, then its creature, then the merge scan's slot.
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
                m_selectedStrip == m_heroStrip ? gGame->GetHero(m_town->m_occupyingHeroId) : NULL;
            gGame->ViewArmy(
                TOWN_ARMY_VIEW_X,
                TOWN_ARMY_VIEW_Y,
                m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot],
                m_selectedStrip->m_army->m_creatureCounts[m_selectedArmySlot],
                m_town,
                m_castleDialogActive == true
                    || (m_selectedStrip == m_heroStrip
                        && m_selectedStrip->m_army->GetNumArmies() == 1),
                ARMY_FACING_RIGHT,
                0,
                viewedHero,
                NULL,
                m_selectedStrip->m_army
            );
            if (gWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
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
            oldValue = H1_ENUM_ENCODE(
                CreatureType,
                m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]
            );
            m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot];
            m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot] =
                H1_ENUM_DECODE(CreatureType, oldValue);
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
            visitor = gGame->GetHero(m_town->m_occupyingHeroId);
            visitor->HeroView(true);
            RedrawTownScreen();
            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
            break;
        case TOWN_ARMY_COMMAND_GARRISON:
            gGame->Overview();
            RedrawTownScreen();
            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
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

    DrawTown(true, true);
    m_garrisonStrip->DrawIcons(1);
    m_heroStrip->DrawIcons(1);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_STATUS_TEXT_CONTROL);
    message.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0);
    gWindowManager->UpdateScreenRegion(0, 0x100, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
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
    gWindowManager->DoDialog(m_heroWindow1, SplitArmyHandler, false);
    delete m_heroWindow1;
    if (gWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
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
        SetArmyCommand(gInputManager->GetModifiers() & MESSAGE_MODIFIER_SHIFT_KEYS);
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
void townManager::Toggle(H1_ENUM_PARAM(BuildingSlotType, i8) building) {
    i16 index;

    if (m_town->m_buildings & H1_ENUM_BIT(BuildingSlotType, building)) {
        for (index = 0; index < m_townObjectCount; index++) {
            if (m_townObjects[index]->m_buildingId == building)
                m_townObjects[index]->m_visible ^= 1;
        }
    }
}

// Draws a bitmap background and folds the mouse pointer into the screen
// buffer around the viewport blit.
VA(0x0046254d, 0xe3)
void townManager::DrawTown(b8 updateScreen, b32 advanceAnimation) {
    i16 index;
    i16 x;
    i16 y;

    m_backgroundBitmap->DrawToBuffer(0, 0);
    for (index = 0; index < m_townObjectCount; index++)
        m_townObjects[index]->Draw(advanceAnimation);
    m_townWindow->DrawWindow(0, TOWN_REDRAW_FIRST_CONTROL, TOWN_REDRAW_LAST_CONTROL);
    gMouseManager->MouseCoords(x, y);
    if (y < TOWN_VIEWPORT_HEIGHT)
        gMouseManager->SaveAndDraw(gWindowManager->m_screen, 0, 0, 1);
    if (updateScreen)
        BlitBitmapToScreen(
            gWindowManager->m_screen,
            0,
            0,
            TOWN_VIEWPORT_WIDTH,
            TOWN_VIEWPORT_HEIGHT,
            0,
            0
        );
    if (y < TOWN_VIEWPORT_HEIGHT)
        gMouseManager->RestoreUnderlying();
}

// Reads the mage/neutral/dwelling cost tables with asserts, sizes resource
// slots by the gold-icon width and draws the building through the castle
// frame of buybuil%d.bin.
VA(0x00462630, 0xdd2)
#line 1483 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\TOWNMGR.CPP"
i16 townManager::BuyBuild(
    H1_ENUM_PARAM(BuildingSlotType, i16) building,
    b8 cannotBuy,
    i8 quickView
) {
    i32 entryWidth;
    // The cost list holds at most one entry per resource.
    i16 buildCosts[H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT)];
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
    H1_ENUM_STORAGE(ResourceType, i8) resourceTypes[H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT)];
    i32 costCount;
    i32 inRow;
    i32 widgetSlot;
    i16 startX;
    i16 dialogButton;
    i16 dialogWidth;
    i32 rowIndex;
    iconWidget* resWidgets[H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT)];
    H1_ENUM_STORAGE(ResourceType, i32) typeList[4];
    i16 topResources;
    tag_message msg;
    char* infoBuffer;
    i16 dialogResult;
    textWidget* amountWidgets[H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT)];
    char* amountText[H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT)];

    guildRank = 0;
    i = 0;
    costCount = 0;
    infoBuffer = static_cast<char*>(malloc(300));
    for (i = 0; i < H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT); i++)
        resourceTypes[i] = H1_ENUM_DECODE(
            ResourceType,
            buildCosts[i] = H1_ENUM_ENCODE(ResourceType, RESOURCE_NONE)
        );
    dwelling = -1;
    if (building > BUILDING_SLOT_STRUCTURE_LAST)
        dwelling = building - BUILDING_SLOT_DWELLING_FIRST
                   + H1_ENUM_ENCODE(TownType, m_town->m_type) * TOWN_DWELLINGS_PER_FACTION;
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (m_town->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))
            guildRank = gTownManager->m_town->m_buildState + 1;
        else
            guildRank = 0;
        if (guildRank > TOWN_MAGE_GUILD_COST_LEVEL_LAST)
            guildRank = TOWN_MAGE_GUILD_COST_LEVEL_LAST;
        for (i = 0; H1_ENUM_DECODE(ResourceType, i) < RESOURCE_COUNT; i++) {
            if (gMageBuildingCosts[guildRank][i] > 0) {
                resourceTypes[costCount] = H1_ENUM_DECODE(ResourceType, i);
                buildCosts[costCount] = gMageBuildingCosts[guildRank][i];
                costCount++;
            }
        }
    } else if (building <= BUILDING_SLOT_STRUCTURE_LAST) {
        for (i = 0; H1_ENUM_DECODE(ResourceType, i) < RESOURCE_COUNT; i++) {
            // clang-format off
#line 1555
            H1_ASSERT(building >= BUILDING_SLOT_FIRST && building < BUILDING_SLOT_NEUTRAL_COUNT);
            // clang-format on
#line 1556
            H1_ASSERT(i >= 0 && i <= 6);
            if (gNeutralBuildingCosts[building][i] > 0) {
                resourceTypes[costCount] = H1_ENUM_DECODE(ResourceType, i);
                buildCosts[costCount] = gNeutralBuildingCosts[building][i];
                costCount++;
            }
        }
    } else {
        for (i = 0; H1_ENUM_DECODE(ResourceType, i) < RESOURCE_COUNT; i++) {
            // clang-format off
#line 1570
            H1_ASSERT(dwelling >= 0 && dwelling < TOWN_DWELLING_COST_ROWS);
            // clang-format on
#line 1571
            H1_ASSERT(i >= 0 && i <= 6);
            if (gDwellingCosts[dwelling][i] > 0) {
                resourceTypes[costCount] = H1_ENUM_DECODE(ResourceType, i);
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
    widgetSlot = 0;
    requiredCount = 0;
    topResources = 0;
    lowerResources = 0;
    for (i = 0; i < H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT); i++) {
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
    if (building <= BUILDING_SLOT_STRUCTURE_LAST)
        sprintf(infoBuffer, gNeutralBuildingDescriptions[building]);
    else
        sprintf(infoBuffer, gDwellingDescriptions[dwelling]);
    if (dwelling >= 0) {
        u16 prerequisiteMask;
        i32 prerequisiteCount = 0;
        prerequisiteMask = gDwellingRequirements
            [H1_ENUM_ENCODE(TownType, m_town->m_type) * TOWN_DWELLINGS_PER_FACTION
             + (building - BUILDING_SLOT_DWELLING_FIRST)];
        // i walks the prerequisite building slots.
        for (i = 0; H1_ENUM_DECODE(BuildingSlotType, i) < BUILDING_SLOT_REQUIREMENT_END; i++) {
            if (prerequisiteMask & (1 << i)) {
                if (prerequisiteCount == 0)
                    strcat(infoBuffer, localization::Tr("town.build.requires"));
                prerequisiteCount++;
                strcat(infoBuffer, "\n");
                if (H1_ENUM_DECODE(BuildingSlotType, i) <= BUILDING_SLOT_STRUCTURE_LAST)
                    strcat(infoBuffer, gNeutralBuildingNames[H1_ENUM_DECODE(BuildingSlotType, i)]);
                else
                    strcat(
                        infoBuffer,
                        gDwellingNames
                            [H1_ENUM_DECODE(BuildingSlotType, i) - BUILDING_SLOT_DWELLING_FIRST
                             + H1_ENUM_ENCODE(TownType, m_town->m_type)
                                   * TOWN_DWELLINGS_PER_FACTION]
                    );
            }
        }
    }
    strcat(infoBuffer, "\n ");
    lineFont = gResourceManager->GetFont("bigfont.fnt");
    lineTotal = lineFont->LineLength(infoBuffer, 0xee);
    gResourceManager->Dispose(lineFont);
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
    // The picture frame: the generic buildings first, then a block of seven
    // per town type from the tent on.
    msg.value = building < BUILDING_SLOT_RACE_FIRST
                    ? H1_ENUM_ENCODE(BuildingSlotType, building) + 1
                    : (H1_ENUM_ENCODE(TownType, gTownManager->m_town->m_type) + 1) * 7
                          + H1_ENUM_ENCODE(BuildingSlotType, building) - 6;
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
    widgetSlot = 0;
    for (rowIndex = 0; rowIndex < 2; rowIndex++) {
        rowY = lineTotal * 16 + originY + rowIndex * 44 + 12;
        inRow = rowIndex == 0 ? topResources : lowerResources;
        if (inRow > 0) {
            totalWidth = 0;
            costCount = widgetSlot;
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
                amountText[widgetSlot] = static_cast<char*>(malloc(10));
                sprintf(amountText[widgetSlot], "%d", buildCosts[widgetSlot]);
                amountWidgets[widgetSlot] = new textWidget(
                    resourceX,
                    rowY + 32,
                    entryWidth,
                    12,
                    amountText[widgetSlot],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    8
                );
                if (amountWidgets[widgetSlot] == NULL)
                    MemError();
                resWidgets[widgetSlot] = new iconWidget(
                    resourceX,
                    rowY,
                    entryWidth,
                    12,
                    "resource.icn",
                    H1_ENUM_ENCODE(ResourceType, resourceTypes[widgetSlot]),
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (resWidgets[widgetSlot] == NULL)
                    MemError();
                panel->AddWidget(amountWidgets[widgetSlot], WINDOW_Z_ORDER_APPEND);
                panel->AddWidget(resWidgets[widgetSlot], WINDOW_Z_ORDER_APPEND);
                widgetSlot++;
                resourceX += entryWidth + space;
            }
        }
    }
    if (!quickView)
        gWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            TOWN_CLOSE_CONTROL,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
    m_selectedBuilding = BUILDING_SLOT_NONE;
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
        gMouseManager->ReallyHidePointer();
        gWindowManager->AddWindow(panel, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gWindowManager->RemoveWindow(panel);
        gMouseManager->ReallyShowPointer();
    } else {
        if (cannotBuy) {
            msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
            msg.id = DIALOG_BUTTON_2;
            msg.value = WIDGET_FLAG_ENABLED;
            panel->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_SET_FLAGS;
            msg.id = DIALOG_BUTTON_2;
            msg.value = WIDGET_FLAG_DIM_REQUEST;
            panel->BroadcastMessage(msg);
        }
        gWindowManager->DoDialog(panel, TrueFalseDialogHandler, false);
        if (gWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
            m_selectedBuilding = building;
            for (i = 0; i < requiredCount; i++)
                gCurPlayerData->m_resources[resourceTypes[i]] -= buildCosts[i];
        }
    }
    if (!quickView)
        gWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            TOWN_CLOSE_CONTROL,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
    delete panel;
    if (quickView)
        return 0;
    else
        return gWindowManager->m_dialogResult == DIALOG_BUTTON_2;
}

// Fizzles a fixed per-building rectangle.
VA(0x00463402, 0x35f)
void townManager::BuildObj(H1_ENUM_PARAM(BuildingSlotType, i16) building) {
    i16 i;
    class sample* sample;

    gMouseManager->ReallyHidePointer();
    DrawTown(true, true);
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (m_town->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))
            m_town->m_buildState++;
        if (m_town->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            m_town->GiveSpells();
    }
    m_town->m_buildings |= H1_ENUM_BIT(BuildingSlotType, building);
    if (building >= BUILDING_SLOT_DWELLING_FIRST && building <= BUILDING_SLOT_DWELLING_LAST)
        m_town->m_dwellingAvailable[building - BUILDING_SLOT_DWELLING_FIRST] =
            gMonsterDatabase[gDwellingType[m_town->m_type][building - BUILDING_SLOT_DWELLING_FIRST]]
                .growth;
    for (i = 0; i < m_townObjectCount; i++) {
        if (m_townObjects[i]->m_buildingId == building) {
            m_townObjects[i]->m_visible = 1;
            m_townObjects[i]->m_border->m_flags |= WIDGET_FLAG_ENABLED;
        }
    }
    if (building == BUILDING_SLOT_CASTLE) {
        m_town->m_buildings &= ~H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT);
        for (i = 0; i < m_townObjectCount; i++) {
            if (m_townObjects[i]->m_buildingId == BUILDING_SLOT_TENT) {
                m_townObjects[i]->m_visible = 0;
                m_townObjects[i]->m_border->m_flags &= ~WIDGET_FLAG_ENABLED;
            }
        }
    }
    gWindowManager->SaveFizzleSource(
        gTownBuildingExtents[m_town->m_type][building].x,
        gTownBuildingExtents[m_town->m_type][building].y,
        gTownBuildingExtents[m_town->m_type][building].width,
        gTownBuildingExtents[m_town->m_type][building].height
    );
    DrawTown(false, true);
    sample = LoadPlaySample("buildtwn.82M");
    gWindowManager->FizzleForward(
        gTownBuildingExtents[m_town->m_type][building].x,
        gTownBuildingExtents[m_town->m_type][building].y,
        gTownBuildingExtents[m_town->m_type][building].width,
        gTownBuildingExtents[m_town->m_type][building].height,
        FIZZLE_USE_DEFAULT_DELAY
    );
    WaitSample(sample);
    m_selectedBuilding = BUILDING_SLOT_NONE;
    m_bankBox->Update();
    m_townWindow->DrawWindow();
    gMouseManager->ReallyShowPointer();
    gWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_CLEAR_FLAGS,
        DIALOG_BUTTON_0,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    BitSet(gGame->m_townBuiltToday, m_town->m_id);
    m_town->GiveSpells();
}

// Lays out five special buildings and six dwellings plus the hero-recruit
// slot with fixed frames.
VA(0x00463761, 0x45a)
void townManager::SetupCastle(class heroWindow* window) {
    H1_ENUM_LOCAL(TownCastleFrame, i16) builtIcon = TOWN_CASTLE_FRAME_BUILT;
    H1_ENUM_LOCAL(TownCastleFrame, i16) buildVal = TOWN_CASTLE_FRAME_CANNOT_BUILD;
    H1_ENUM_LOCAL(TownCastleFrame, i16) noMoney = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    // A building slot, then a dwelling index.
    i16 i;
    tag_message msg;
    H1_ENUM_LOCAL(TownCastleFrame, i32) stateFrame;

    m_affordableBuildings = m_buildableBuildings = 0;
    for (i = 0; H1_ENUM_DECODE(BuildingSlotType, i) < BUILDING_SLOT_COUNT; i++) {
        if (CanBuy(m_town, H1_ENUM_DECODE(BuildingSlotType, i)))
            m_affordableBuildings |= 1 << i;
        if (CanBuild(m_town, H1_ENUM_DECODE(BuildingSlotType, i)))
            m_buildableBuildings |= 1 << i;
    }
    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        msg.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        msg.value =
            (H1_ENUM_ENCODE(TownType, m_town->m_type) + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
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
        if (TOWN_BUILDING_COMPLETE(*m_town, H1_ENUM_DECODE(BuildingSlotType, i)))
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
            msg.value = H1_ENUM_ENCODE(TownCastleFrame, stateFrame);
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
        if (m_town->m_buildings & H1_ENUM_BIT(BuildingSlotType, i + BUILDING_SLOT_DWELLING_FIRST))
            stateFrame = TOWN_CASTLE_FRAME_BUILT;
        else if (!(m_buildableBuildings
                   & H1_ENUM_BIT(BuildingSlotType, i + BUILDING_SLOT_DWELLING_FIRST)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_BUILD;
        else if (!(m_affordableBuildings
                   & H1_ENUM_BIT(BuildingSlotType, i + BUILDING_SLOT_DWELLING_FIRST)))
            stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
        if (stateFrame != TOWN_CASTLE_FRAME_NONE) {
            msg.command = WIDGET_COMMAND_SET_FLAGS;
            msg.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            msg.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(msg);
            msg.command = WIDGET_COMMAND_SET_FRAME;
            msg.value = H1_ENUM_ENCODE(TownCastleFrame, stateFrame);
            window->BroadcastMessage(msg);
        } else {
            msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
            msg.id = i + TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL;
            msg.value = WIDGET_FLAG_DRAW;
            window->BroadcastMessage(msg);
        }
    }
    if (gCurPlayerData->m_resources[RESOURCE_GOLD] < gHeroGoldCost)
        stateFrame = TOWN_CASTLE_FRAME_CANNOT_AFFORD;
    else if (gCurPlayerData->m_heroCount == PLAYER_HERO_CAPACITY
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
        msg.value = H1_ENUM_ENCODE(TownCastleFrame, stateFrame);
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
        msg.value =
            (H1_ENUM_ENCODE(TownType, m_town->m_type) + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(msg);
        msg.id = i + TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
        msg.value = H1_ENUM_ENCODE(CreatureType, gDwellingType[m_town->m_type][i]);
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
        if (!(m_town->m_buildings
              & H1_ENUM_BIT(BuildingSlotType, i + BUILDING_SLOT_DWELLING_FIRST)))
            strcpy(gText, localization::Tr("town.well.empty"));
        else {
            theRate = gMonsterDatabase[gDwellingType[m_town->m_type][i]].growth;
            theRate += WEEKLY_WELL_GROWTH_BONUS;
            sprintf(
                gText,
                localization::Tr("town.well.growth"),
                m_town->m_dwellingAvailable[i],
                theRate
            );
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
            message.value = H1_ENUM_ENCODE(SpellType, m_town->m_mageGuildSpells[spellNo]);
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
H1_ENUM_RETURN(MessageDispatchResult, i16) MageGuildHandler(struct tag_message& message) {
    i16 firstSpell = TOWN_MAGE_FIRST_SPELL_CONTROL;
    i16 iconBaseVal = TOWN_MAGE_FIRST_ICON_CONTROL;
    i32 quickViewVal;
    H1_ENUM_LOCAL(SpellType, i32) spellId;
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
                        theMageLevel = gTownManager->m_town->m_buildState;
                        if ((theMageLevel == MAGE_GUILD_STATE_LEVEL_1
                             && spellPosIndex > MAGE_GUILD_LEVEL_1_LAST_SLOT)
                            || (theMageLevel == MAGE_GUILD_STATE_LEVEL_2
                                && spellPosIndex > MAGE_GUILD_LEVEL_2_LAST_SLOT)
                            || (theMageLevel == MAGE_GUILD_STATE_LEVEL_3
                                && spellPosIndex > MAGE_GUILD_LEVEL_3_LAST_SLOT))
                            return MESSAGE_DISPATCH_CONSUME;
                        spellId = gTownManager->m_town->m_mageGuildSpells[spellPosIndex];
                        NormalDialog(
                            gSpellDesc[spellId],
                            quickViewVal ? NORMAL_DIALOG_TYPE_QUICK_VIEW : NORMAL_DIALOG_TYPE_OK,
                            -1,
                            -1,
                            NORMAL_DIALOG_SPELL,
                            H1_ENUM_ENCODE(SpellType, spellId)
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
void townManager::SetupThievesGuild(
    class heroWindow* window,
    H1_ENUM_PARAM(TownThievesCategory, i16) categories
) {
    iconWidget* marker;
    i16 firstPlayer;
    i32 numGuilds;
    i16 posX = THIEVES_RANK_FIRST_X;
    i16 theIUnusedRankWidth = THIEVES_PLAYER_COLUMN_WIDTH;
    i16 topNum = THIEVES_FIRST_CATEGORY_Y;
    i16 oldSpacing = THIEVES_CATEGORY_ROW_HEIGHT;
    i16 frameBase = THIEVES_FLAG_FRAME_BASE;
    i16 tiedPlayer;
    i16 savedLMarkWidth = THIEVES_RANK_ICON_WIDTH;
    i16 iconHeightOn = THIEVES_RANK_ICON_HEIGHT;
    i16 bColWidth = THIEVES_PLAYER_WIDTH;
    i8 playerOrder[GAME_PLAYER_COUNT];
    i16 bestRank;
    H1_ENUM_LOCAL(TownThievesCategory, i16) categoryIndex;
    i32 savedTotals[GAME_PLAYER_COUNT];
    i16 savedPos;
    i16 realHi;
    i16 numAtRank;

    if (categories == THIEVES_CATEGORIES_BY_GUILDS) {
        numGuilds = gGame->GetNumThievesGuilds(gCurPlayer);
        if (numGuilds >= 4)
            categories = THIEVES_CATEGORY_COUNT;
        else if (numGuilds == THIEVES_GUILDS_THREE)
            categories = THIEVES_CATEGORY_ARMY_STRENGTH;
        else if (numGuilds == THIEVES_GUILDS_TWO)
            categories = THIEVES_CATEGORY_RARE_RESOURCES;
        else
            categories = THIEVES_CATEGORY_GOLD;
    }
    if (categories > THIEVES_CATEGORY_COUNT)
        categories = THIEVES_CATEGORY_COUNT;
    for (categoryIndex = THIEVES_CATEGORY_TOWNS; categoryIndex < categories; categoryIndex++) {
        GetCategoryStats(categoryIndex, savedTotals, playerOrder);
        SortStats(savedTotals, playerOrder);
        firstPlayer = 0;
        realHi = 0;
        for (bestRank = 0; bestRank < THIEVES_RANK_COUNT; bestRank++) {
            if (firstPlayer == gGame->m_playerCount - gGame->m_deadPlayerCount)
                break;
            numAtRank = 1;
            while (realHi + 1 < gGame->m_playerCount
                   && savedTotals[realHi + 1] == savedTotals[realHi]) {
                numAtRank++;
                realHi++;
            }
            savedPos = bestRank * THIEVES_PLAYER_COLUMN_WIDTH + THIEVES_RANK_FIRST_X
                       - (numAtRank - 1) * THIEVES_TIE_CENTERING_STEP;
            for (tiedPlayer = firstPlayer; !(tiedPlayer > realHi); tiedPlayer++) {
                marker = new iconWidget(
                    savedPos + (tiedPlayer - firstPlayer) * THIEVES_RANK_ICON_WIDTH,
                    H1_ENUM_ENCODE(TownThievesCategory, categoryIndex) * THIEVES_CATEGORY_ROW_HEIGHT
                        + THIEVES_FIRST_CATEGORY_Y,
                    THIEVES_RANK_ICON_WIDTH,
                    THIEVES_RANK_ICON_HEIGHT,
                    "townwind.icn",
                    H1_ENUM_ENCODE(PlayerColor, gGame->m_players[playerOrder[tiedPlayer]].m_color)
                        + THIEVES_FLAG_FRAME_BASE,
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
void townManager::GetCategoryStats(
    H1_ENUM_PARAM(TownThievesCategory, i8) category,
    i32* const stats,
    i8* const order
) {
    i16 townIndex;
    i16 index;
    i32 strengthValue;
    i16 player;
    i16 firstNumTowns;
    i16 numCastles;
    hero* playerHeroData;
    town* townItem;

    for (player = 0; player < gGame->m_playerCount; player++) {
        firstNumTowns = 0;
        numCastles = 0;
        order[player] = player;
        if (gGame->m_playerDead[player]) {
            stats[player] = TOWN_THIEVES_DEAD_PLAYER_STAT;
        } else {
            switch (category) {
                case THIEVES_CATEGORY_TOWNS:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gGame->m_castleRecs[townIndex].m_owner == player
                            && (gGame->m_castleRecs[townIndex].m_buildings
                                & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT)))
                            firstNumTowns++;
                    }
                    stats[player] = firstNumTowns;
                    break;
                case THIEVES_CATEGORY_CASTLES:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gGame->m_castleRecs[townIndex].m_owner == player
                            && (gGame->m_castleRecs[townIndex].m_buildings
                                & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE)))
                            numCastles++;
                    }
                    stats[player] = numCastles;
                    break;
                case THIEVES_CATEGORY_HEROES:
                    stats[player] = gGame->m_players[player].m_heroCount;
                    break;
                case THIEVES_CATEGORY_GOLD:
                    stats[player] = gGame->m_players[player].m_resources[RESOURCE_GOLD];
                    break;
                case THIEVES_CATEGORY_WOOD_AND_ORE:
                    stats[player] = gGame->m_players[player].m_resources[RESOURCE_WOOD]
                                    + gGame->m_players[player].m_resources[RESOURCE_CRYSTAL]
                                    + gGame->m_players[player].m_resources[RESOURCE_ORE];
                    break;
                case THIEVES_CATEGORY_RARE_RESOURCES:
                    stats[player] = gGame->m_players[player].m_resources[RESOURCE_GEMS]
                                    + gGame->m_players[player].m_resources[RESOURCE_SULFUR]
                                    + gGame->m_players[player].m_resources[RESOURCE_MERCURY];
                    break;
                case THIEVES_CATEGORY_OBELISKS:
                    stats[player] = gGame->m_players[player].CountPuzzlePiecesRemoved();
                    break;
                case THIEVES_CATEGORY_ARMY_STRENGTH:
                    strengthValue = 0;
                    for (index = 0; index < gGame->m_players[player].m_heroCount; index++) {
                        playerHeroData = gGame->GetPlayerHero(player, index);
                        strengthValue += gPhilAI->FightValueOfStack(
                            &playerHeroData->m_army,
                            playerHeroData,
                            false
                        );
                    }
                    for (index = 0; index < gGame->m_players[player].m_townCount; index++) {
                        townItem = gGame->GetPlayerTown(player, index);
                        if (townItem->HasGarrison())
                            strengthValue +=
                                gPhilAI->FightValueOfStack(&townItem->m_army, NULL, false);
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

    for (firstPlayer = 0; firstPlayer < gGame->m_playerCount - 1; firstPlayer++) {
        for (secondPlayer = firstPlayer + 1; secondPlayer < gGame->m_playerCount; secondPlayer++) {
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
char* townManager::GetBuildingName(H1_ENUM_PARAM(BuildingSlotType, i16) building) {
    return ::GetBuildingName(m_town->m_type, building);
}

// The castle's recruit window shows both candidate heroes, a right-click
// view is a timed quick view, and the town strips are rebuilt.
VA(0x00464a47, 0x92f)
b8 townManager::RecruitHero(b8 quickView) {
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
    m_recruitHeroes[0] = gGame->GetHero(gCurPlayerData->m_availableHeroIds[0]);
    m_recruitHeroes[1] = gGame->GetHero(gCurPlayerData->m_availableHeroIds[1]);
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = gCurPlayer;
    evtCopy.type = MESSAGE_WIDGET;
    if (quickView) {
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
    if (quickView) {
        gMouseManager->ReallyHidePointer();
        gWindowManager->AddWindow(m_heroWindow1, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gWindowManager->RemoveWindow(m_heroWindow1);
        gMouseManager->ReallyShowPointer();
    } else
        gWindowManager->DoDialog(m_heroWindow1, RecruitHeroHandler, false);
    delete m_heroWindow1;
    if (m_recruitState != RECRUIT_HERO_NONE) {
        i32 newHeroClass;
        i16 townY;
        i16 townX;

        gCurPlayerData->m_resources[RESOURCE_GOLD] -= gHeroGoldCost;
        gCurPlayerData->m_heroIds[gCurPlayerData->m_heroCount] =
            gCurPlayerData->m_availableHeroIds[m_recruitState];
        gCurPlayerData->m_heroCount++;
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
        m_recruitHeroes[m_recruitState]->m_locationType = gGame->m_map[townX][townY].m_triggerType;
        m_recruitHeroes[m_recruitState]->m_occupiedTown =
            gGame->m_map[townX][townY].m_objectMetadata;
        gGame->m_map[townX][townY].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_HERO);
        gGame->m_map[townX][townY].m_objectMetadata =
            gCurPlayerData->m_availableHeroIds[m_recruitState];
        m_recruitResult = true;
        m_town->m_occupyingHeroId = m_recruitHeroes[m_recruitState]->m_id;
        gGame->m_availableHeroes[gCurPlayerData->m_availableHeroIds[m_recruitState]] = gCurPlayer;
        delete m_garrisonStrip;
        sprintf(
            gText,
            "crst%04d.icn",
            H1_ENUM_ENCODE(PlayerColor, gCurPlayerData->Color()) * HERO_CLASS_COUNT
                + m_recruitHeroes[m_recruitState]->m_heroClass
        );
        m_garrisonStrip = new strip(
            0,
            TOWN_GARRISON_STRIP_Y,
            m_town->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                ? static_cast<i8>(TOWN_CREST_FRAME_WITHOUT_HERO)
                : static_cast<i8>(TOWN_CREST_FRAME_WITH_HERO),
            gResourceManager->MakeId(gText),
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
            gResourceManager->MakeId(gText),
            0,
            &m_recruitHeroes[m_recruitState]->m_army,
            TOWN_HERO_FIRST_CONTROL,
            0
        );
        if (m_heroStrip == NULL)
            MemError();
        if (m_town->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))
            m_town->GiveSpells();
        newHeroClass =
            gCurPlayerData->m_availableHeroIds[1 - m_recruitState] / HERO_PER_CLASS_COUNT;
        newHeroClass = (newHeroClass + Random(1, 3)) % HERO_CLASS_COUNT;
        gCurPlayerData->m_availableHeroIds[m_recruitState] = gGame->GetNewHeroId(newHeroClass);
        gGame->m_availableHeroes[gCurPlayerData->m_availableHeroIds[m_recruitState]] =
            HERO_AVAILABILITY_RETREATED;
    } else {
        if (m_castleDialogActive)
            SetupCastle(m_heroWindow0);
        if (m_castleDialogActive)
            m_heroWindow0->DrawWindow();
    }
    m_bankBox->Update();
    gWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_CLEAR_FLAGS,
        TOWN_CLOSE_CONTROL,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    m_recruitHeroes[0]->m_owner = m_recruitHeroes[1]->m_owner = GAME_PLAYER_NONE;
    if (m_recruitState != RECRUIT_HERO_NONE)
        m_recruitHeroes[m_recruitState]->m_owner = gCurPlayer;
    return gWindowManager->m_dialogResult != DIALOG_BUTTON_1;
}

// Animates frames 1-8 of control 2.
VA(0x00465376, 0x125)
H1_ENUM_RETURN(MessageDispatchResult, i16) TavernHandler(struct tag_message& message) {
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
    if (gTimers[TOWN_FRAME_TIMER_SLOT] < KBTickCount()) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, TOWN_TAVERN_ANIMATION_CONTROL);
        ++gGame->m_dialogAnimationCounter;
        message.value = gGame->m_dialogAnimationCounter % TOWN_TAVERN_ANIMATION_FRAME_COUNT
                        + TOWN_TAVERN_FIRST_ANIMATION_FRAME;
        gTownManager->m_heroWindow0->BroadcastMessage(message);
        gTownManager->m_heroWindow0->MoveWindow(0, 0);
        gTimers[TOWN_FRAME_TIMER_SLOT] = KBTickCount() + TOWN_TAVERN_ANIMATION_DELAY;
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
    PlayMusic(MUSIC_TRACK_TAVERN);
    gWindowManager->DoDialog(m_heroWindow0, TavernHandler, false);
    delete m_heroWindow0;
    PlayMusic(gTownTheme[m_town->m_type] + MUSIC_TRACK_TOWN_FIRST);
}

// Hovers by widget id and recruits a single hero (control 0x30).
VA(0x0046559a, 0x650)
H1_ENUM_RETURN(MessageDispatchResult, i16) CastleHandler(struct tag_message& message) {
    i16 statusId = TOWN_CASTLE_STATUS_CONTROL;
    i32 result = 0;
    i32 quickViewVal;
    // A gCastleInfo row, then the town-object scan index.
    H1_ENUM_SHARED(TownCastleInfoText, i32) objNum;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (gTownManager->m_lastHoverId == message.id)
                    break;
                gTownManager->m_lastHoverId = message.id;
                // castle.bin's building widgets carry their building slot as
                // the widget id.
                switch (message.id) {
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD):
                        if (!(gTownManager->m_buildableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_BUILD],
                                gTownManager->GetBuildingName(
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                )
                            );
                        else if (!(gTownManager->m_affordableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD],
                                gTownManager->GetBuildingName(
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                )
                            );
                        else {
                            if (!(gTownManager->m_town->m_buildings
                                  & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD)))
                                objNum = TOWN_CASTLE_INFO_BUILD_MAGE_GUILD;
                            else if (gTownManager->m_town->m_buildState == MAGE_GUILD_STATE_LEVEL_4)
                                objNum = TOWN_CASTLE_INFO_MAGE_GUILD_MAX_LEVEL;
                            else if (!CanBuy(gTownManager->m_town, BUILDING_SLOT_MAGE_GUILD))
                                objNum = TOWN_CASTLE_INFO_CANNOT_AFFORD_MAGE_LEVEL;
                            else
                                objNum = TOWN_CASTLE_INFO_ADD_MAGE_GUILD_LEVEL;
                            strcpy(gText, gCastleInfo[objNum]);
                        }
                        break;
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_THIEVES_GUILD):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TAVERN):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_SHIPYARD):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_WELL):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_1):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_2):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_3):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_4):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_5):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_6):
                        if (gTownManager->m_town->m_buildings & (1 << message.id))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_ALREADY_BUILT],
                                gTownManager->GetBuildingName(
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                )
                            );
                        else if (!(gTownManager->m_buildableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_BUILD],
                                gTownManager->GetBuildingName(
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                )
                            );
                        else if (!(gTownManager->m_affordableBuildings & (1 << message.id)))
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD],
                                gTownManager->GetBuildingName(
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                )
                            );
                        else
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_BUILD],
                                gTownManager->GetBuildingName(
                                    H1_ENUM_DECODE(BuildingSlotType, message.id)
                                )
                            );
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (gCurPlayerData->m_resources[RESOURCE_GOLD] < gHeroGoldCost)
                            strcpy(gText, gCastleInfo[TOWN_CASTLE_INFO_CANNOT_AFFORD_HERO]);
                        else if (gCurPlayerData->m_heroCount == PLAYER_HERO_CAPACITY)
                            sprintf(
                                gText,
                                gCastleInfo[TOWN_CASTLE_INFO_TOO_MANY_HEROES],
                                PLAYER_HERO_CAPACITY
                            );
                        else if (gTownManager->m_town->m_occupyingHeroId
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
                gTownManager->m_heroWindow0->BroadcastMessage(message);
                gTownManager->m_heroWindow0->DrawWindow(
                    0,
                    TOWN_CASTLE_STATUS_FIRST_CONTROL,
                    TOWN_CASTLE_STATUS_TEXT_CONTROL
                );
                gWindowManager->UpdateScreenRegion(
                    TOWN_CASTLE_STATUS_X,
                    TOWN_CASTLE_STATUS_Y,
                    TOWN_CASTLE_STATUS_WIDTH,
                    TOWN_CASTLE_STATUS_HEIGHT
                );
                return MESSAGE_DISPATCH_CONSUME;
            case WIDGET_NOTIFY_SELECT:
                quickViewVal =
                    (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) != MESSAGE_MODIFIER_NONE;
                switch (message.id) {
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD):
                        if (!quickViewVal
                            && (gTownManager->m_town->m_buildState == MAGE_GUILD_STATE_LEVEL_4
                                || !(gTownManager->m_buildableBuildings & (1 << message.id))))
                            break;
                        else
                            goto buy_building;
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_THIEVES_GUILD):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_TAVERN):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_SHIPYARD):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_WELL):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_1):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_2):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_3):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_4):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_5):
                    case H1_ENUM_ENCODE(BuildingSlotType, BUILDING_SLOT_DWELLING_6):
                        if (!quickViewVal
                            && ((gTownManager->m_town->m_buildings & (1 << message.id))
                                || !(gTownManager->m_buildableBuildings & (1 << message.id))))
                            break;
                    buy_building:
                        for (objNum = 0; objNum < gTownManager->m_townObjectCount; objNum++) {
                            if (gTownManager->m_townObjects[objNum]->m_buildingId
                                == H1_ENUM_DECODE(BuildingSlotType, message.id))
                                break;
                        }
                        result = gTownManager->BuyBuild(
                            H1_ENUM_DECODE(BuildingSlotType, message.id),
                            (gTownManager->m_affordableBuildings & (1 << message.id)) == 0,
                            quickViewVal
                        );
                        break;
                    case TOWN_CASTLE_HERO_CONTROL:
                        if (quickViewVal)
                            gTownManager->RecruitHero(true);
                        else if (!gTownManager->m_recruitResult
                                 && gCurPlayerData->m_resources[RESOURCE_GOLD] >= gHeroGoldCost
                                 && gCurPlayerData->m_heroCount < PLAYER_HERO_CAPACITY
                                 && gTownManager->m_town->m_occupyingHeroId
                                        == TOWN_OCCUPYING_HERO_NONE)
                            result = gTownManager->RecruitHero(false);
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
        message.command = H1_ENUM_DECODE(
            BaseWidgetCommand,
            message.id = H1_ENUM_ENCODE(BaseWidgetCommand, WIDGET_COMMAND_DIALOG_SELECT)
        );
        return MESSAGE_DISPATCH_FORWARD;
    }
    return TrueFalseDialogHandler(message);
}

// Handles the amount buttons on selection and redraws the whole split
// window.
VA(0x00465bea, 0x2cc)
H1_ENUM_RETURN(MessageDispatchResult, i16) SplitArmyHandler(struct tag_message& message) {
    i16 plusControlNum = TOWN_SPLIT_INCREASE_CONTROL;
    i32 unusedActionVal;
    i16 minusButton = TOWN_SPLIT_DECREASE_CONTROL;
    i16 amountText = TOWN_SPLIT_AMOUNT_CONTROL;
    b32 handled = false;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case TOWN_SPLIT_INCREASE_CONTROL:
                        ++gTownManager->m_splitAmount;
                        if (gTownManager->m_splitAmount >= gTownManager->m_splitMaximum)
                            gTownManager->m_splitAmount = gTownManager->m_splitMaximum - 1;
                        goto update_amount;
                    case TOWN_SPLIT_DECREASE_CONTROL:
                        --gTownManager->m_splitAmount;
                        if (gTownManager->m_splitAmount < 0)
                            gTownManager->m_splitAmount = 0;
                        goto update_amount;
                    case TOWN_SPLIT_AMOUNT_CONTROL:
                        message.command = WIDGET_COMMAND_GET_TEXT;
                        gTownManager->m_heroWindow1->BroadcastMessage(message);
                        gTownManager->m_splitAmount = atoi(message.text);
                        if (gTownManager->m_splitAmount < 0)
                            gTownManager->m_splitAmount = 0;
                        if (gTownManager->m_splitAmount >= gTownManager->m_splitMaximum)
                            gTownManager->m_splitAmount = gTownManager->m_splitMaximum - 1;
                        goto update_amount;
                }
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                        gTownManager->m_splitAmount = 0;
                        gWindowManager->m_dialogResult = message.id;
                        handled = true;
                        break;
                    case DIALOG_BUTTON_2:
                        if (gTownManager->m_splitAmount == 0)
                            gWindowManager->m_dialogResult = DIALOG_BUTTON_1;
                        else
                            gWindowManager->m_dialogResult = DIALOG_BUTTON_2;
                        handled = true;
                        break;
                }
                break;
            default:
                break;
        }
    }

    if (handled == true) {
        message.command = H1_ENUM_DECODE(
            BaseWidgetCommand,
            message.id = H1_ENUM_ENCODE(BaseWidgetCommand, WIDGET_COMMAND_DIALOG_SELECT)
        );
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;

update_amount:
    sprintf(gText, "%d", gTownManager->m_splitAmount);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, TOWN_SPLIT_AMOUNT_CONTROL);
    message.text = gText;
    gTownManager->m_heroWindow1->BroadcastMessage(message);
    gTownManager->m_heroWindow1->DrawWindow();
    return MESSAGE_DISPATCH_CONSUME;
}
// TOWNMGR's .rdata: Open's per-type town-object layout.
DATA(0x0048a70c)
const H1_ENUM_ARRAY_ROWS(
    i8,
    gTownObjectType,
    TownType,
    TOWN_TYPE_COUNT,
    TOWN_MANAGER_OBJECT_CAPACITY
) = {
    {5, 6, 8, 11, 7, 0, 1, 2, 10, 9, 3, 4, 12, -1, -1, -1},
    {5, 6, 12, 8, 0, 9, 10, 1, 2, 11, 3, 4, 7, -1, -1, -1},
    {13, 5, 6, 9, 7, 11, 0, 1, 2, 10, 8, 12, 3, 4, -1, -1},
    {5, 6, 12, 9, 0, 11, 10, 1, 2, 7, 3, 4, 8, -1, -1, -1},
};
