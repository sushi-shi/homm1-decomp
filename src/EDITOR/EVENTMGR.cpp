#include <H1/Ints.h>

#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <EDITOR/editManager.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/eventsManager.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

iconWidget* gDensityTracks[EDITOR_GENERATOR_DENSITY_COUNT] = {NULL};
editHeroExtra gHeroEdit = {0};
heroWindow* gClearWindow = NULL;
i16 gEventsLastHoverId = 0;
iconWidget* gTerrainKnobs[EDITOR_TERRAIN_COUNT] = {NULL};
editMapCellPair* gEditCellPair = NULL;
iconWidget* gDensityKnobs[EDITOR_GENERATOR_DENSITY_COUNT] = {NULL};
iconWidget* gTerrainTracks[EDITOR_TERRAIN_COUNT] = {NULL};
editTownExtra gTownEdit = {false};
i32 gMonsterCountEdit = 0;
heroWindow* gDetailsWindow = NULL;
heroWindow* gNewMapWindow = NULL;

eventsManager::eventsManager(void) {
    m_dispatchMask = EDIT_MANAGER_DISPATCH_MASK;
    m_panel = NULL;
}

i16 eventsManager::Open(i16 priority) {
    m_panel = new iconWidget(
        EDIT_TOOL_PANEL_X,
        EDIT_TOOL_PANEL_Y,
        EDIT_TOOL_PANEL_WIDTH,
        EDIT_TOOL_PANEL_HEIGHT,
        "buttons.icn",
        EDIT_FRAME_EVENTS_PANEL,
        ICON_DRAW_NORMAL,
        WIDGET_ID_NONE,
        ICON_WIDGET_DRAW,
        1
    );
    gEditManager->m_window->AddWidget(m_panel, WINDOW_Z_ORDER_APPEND);
    gEditManager->m_window->DrawWindow();
    m_cursorIcon = gResourceManager->GetIcon("overlay.icn");
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "eventsManager");
    return BASE_MANAGER_SUCCESS;
}

void eventsManager::Close(void) {
    if (m_panel) {
        gEditManager->m_window->RemoveWidget(m_panel);
        delete m_panel;
        m_panel = NULL;
    }
    gResourceManager->Dispose(m_cursorIcon);
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
    gEditManager->DrawRadar(true);
    m_active = 0;
}

i16 eventsManager::Main(tag_message& message) {
    i16 x;
    i16 y;
    i16 newX;
    mapCell* cell;
    i16 newY;

    if (!(message.type & m_dispatchMask))
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                        break;
                    x = gEditManager->m_viewX + gEditManager->m_cursorX;
                    y = gEditManager->m_viewY + gEditManager->m_cursorY;
                    cell = &gEditManager->m_map.cells[x][y];
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
                                || cell->m_triggerType
                                       == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_TOWN)
                                || cell->m_triggerType
                                       == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE))
                                EditTown(x, y);
                            else if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER)
                                     || cell->m_triggerType
                                            == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER)
                                     || cell->m_triggerType
                                            == MAP_EVENT_TRIGGER(
                                                MAP_FILE_OBJECT_RANDOM_MONSTER_WEAK
                                            )
                                     || cell->m_triggerType
                                            == MAP_EVENT_TRIGGER(
                                                MAP_FILE_OBJECT_RANDOM_MONSTER_MEDIUM
                                            )
                                     || cell->m_triggerType
                                            == MAP_EVENT_TRIGGER(
                                                MAP_FILE_OBJECT_RANDOM_MONSTER_STRONG
                                            )
                                     || cell->m_triggerType
                                            == MAP_EVENT_TRIGGER(
                                                MAP_FILE_OBJECT_RANDOM_MONSTER_VERY_STRONG
                                            ))
                                EditMonster(x, y);
                            else if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_HERO))
                                EditHero(x, y);
                            else
                                EditCell(x, y);
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    if (message.id != EDIT_CONTROL_MAP && message.id == gEventsLastHoverId)
                        return MESSAGE_DISPATCH_CONSUME;
                    gEventsLastHoverId = message.id;
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            gMouseManager->MouseCoords(newX, newY);
                            gEditManager->ScreenToCell(newX, newY);
                            if (gEditManager->m_cursorX != newX
                                || gEditManager->m_cursorY != newY) {
                                gEditManager->m_cursorX = newX;
                                gEditManager->m_cursorY = newY;
                                newX = newX
                                           * (gEditManager->m_zoomedOut
                                                  ? EDIT_VIEW_ZOOMED_CELL_PIXELS
                                                  : EDIT_VIEW_CELL_PIXELS)
                                       + EDIT_VIEW_LEFT;
                                newY = newY
                                           * (gEditManager->m_zoomedOut
                                                  ? EDIT_VIEW_ZOOMED_CELL_PIXELS
                                                  : EDIT_VIEW_CELL_PIXELS)
                                       + EDIT_VIEW_TOP;
                                gEditManager->DrawMap();
                                m_cursorIcon->FillToBuffer(
                                    newX,
                                    newY,
                                    !gEditManager->m_zoomedOut,
                                    EVENTS_HOVER_COLOR,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                gEditManager->UpdateMapView();
                                gEditManager->UpdateCursor();
                            }
                            break;
                    }
                    return MESSAGE_DISPATCH_CONSUME;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}

void eventsManager::EditCell(i16 x, i16 y) {
    mapCell original;
    char text[20];
    editMapCellPair savedPair;

    if (gDebugLevel < EVENTS_CELL_EDIT_DEBUG_LEVEL_MIN) {
        NormalDialog(localization::Tr("editor.events.cell.unavailable"), NORMAL_DIALOG_TYPE_OK);
        return;
    }
    const i16 firstByteId = CELL_WINDOW_FIRST_BYTE;
    const i16 firstToggle = CELL_WINDOW_FIRST_FLAG;
    tag_message msg;
    gEditCell = &gEditManager->m_map.cells[x][y];
    gEditCellPair = &gEditManager->m_map.cellPairs[x][y];
    original = *gEditCell;
    gEditDialog = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "cellwin.bin");
    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_SET_TEXT;
    msg.text = text;
    sprintf(text, "%d", gEditCell->m_tileIndex);
    msg.id = firstByteId + CELL_FIELD_TILE_INDEX;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_objectTileset);
    msg.id = firstByteId + CELL_FIELD_OBJECT_TILESET;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_objectIndex);
    msg.id = firstByteId + CELL_FIELD_OBJECT_INDEX;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_overlayTileset);
    msg.id = firstByteId + CELL_FIELD_OVERLAY_TILESET;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_overlayIndex);
    msg.id = firstByteId + CELL_FIELD_OVERLAY_INDEX;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_extraFrame);
    msg.id = firstByteId + CELL_FIELD_EXTRA_FRAME;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_flags);
    msg.id = firstByteId + CELL_FIELD_FLAGS;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_secondaryTrigger);
    msg.id = firstByteId + CELL_FIELD_SECONDARY_TRIGGER;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_triggerType);
    msg.id = firstByteId + CELL_FIELD_TRIGGER_TYPE;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCell->m_objectMetadata);
    msg.id = firstByteId + CELL_FIELD_OBJECT_METADATA;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCellPair->objectId);
    msg.id = firstByteId + CELL_FIELD_OBJECT_ID;
    gEditDialog->BroadcastMessage(msg);
    sprintf(text, "%d", gEditCellPair->overlayId);
    msg.id = firstByteId + CELL_FIELD_OVERLAY_ID;
    gEditDialog->BroadcastMessage(msg);
    gWindowManager->DoDialog(gEditDialog, CellWindowHandler, false);
    delete gEditDialog;
    if (gWindowManager->m_dialogResult == EVENTS_DIALOG_CANCEL)
        gEditManager->m_map.cells[x][y] = original;
    else
        gEditManager->m_mapChanged = 1;
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
}

i16 CellWindowHandler(tag_message& message) {
    const i16 firstByteId = CELL_WINDOW_FIRST_BYTE;
    const i16 firstToggleId = CELL_WINDOW_FIRST_FLAG;
    i32 value;

    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case EVENTS_DIALOG_CANCEL:
                        case EVENTS_DIALOG_OK:
                            gWindowManager->m_dialogResult = message.id;
                            FINISH_DIALOG_SELECT(message);
                            return MESSAGE_DISPATCH_FORWARD;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_TILE_INDEX:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OBJECT_TILESET:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OBJECT_INDEX:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OVERLAY_TILESET:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OVERLAY_INDEX:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_EXTRA_FRAME:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_FLAGS:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_SECONDARY_TRIGGER:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_TRIGGER_TYPE:
                        case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OBJECT_METADATA:
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            value = atoi(message.text);
                            if (value < 0)
                                break;
                            switch (message.id) {
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_TILE_INDEX:
                                    gEditCell->m_tileIndex = value & CELL_WINDOW_BYTE_MASK;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OBJECT_TILESET:
                                    if (value > MAP_CELL_TILESET_MASK)
                                        value = MAP_CELL_TILESET_MASK;
                                    gEditCell->m_objectTileset = value;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OBJECT_INDEX:
                                    gEditCell->m_objectIndex = value & CELL_WINDOW_BYTE_MASK;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OVERLAY_TILESET:
                                    if (value > MAP_CELL_TILESET_MASK)
                                        value = MAP_CELL_TILESET_MASK;
                                    gEditCell->m_overlayTileset = value;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OVERLAY_INDEX:
                                    gEditCell->m_overlayIndex = value & CELL_WINDOW_BYTE_MASK;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_EXTRA_FRAME:
                                    gEditCell->m_extraFrame = value;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_FLAGS:
                                    gEditCell->m_flags = value;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_SECONDARY_TRIGGER:
                                    gEditCell->m_secondaryTrigger = value;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_TRIGGER_TYPE:
                                    gEditCell->m_triggerType = value;
                                    break;
                                case CELL_WINDOW_FIRST_BYTE + CELL_FIELD_OBJECT_METADATA:
                                    gEditCell->m_objectMetadata = value;
                                    break;
                            }
                            gEditDialog->DrawWindow();
                            break;
                        case CELL_WINDOW_FIRST_FLAG:
                        case CELL_WINDOW_FIRST_FLAG + 1:
                        case CELL_WINDOW_FIRST_FLAG + 2:
                        case CELL_WINDOW_FIRST_FLAG + 3:
                        case CELL_WINDOW_FIRST_FLAG + 4:
                        case CELL_WINDOW_FIRST_FLAG + 5:
                        case CELL_WINDOW_FIRST_FLAG + 6:
                        case CELL_WINDOW_FIRST_FLAG + 7:
                            gEditCell->m_flags ^=
                                (1 << (message.id - firstToggleId));
                            message.type = MESSAGE_WIDGET;
                            message.command =
                                gEditCell->m_flags & (1 << (message.id - firstToggleId))
                                    ? WIDGET_COMMAND_SET_FLAGS
                                    : WIDGET_COMMAND_CLEAR_FLAGS;
                            message.value = WIDGET_FLAG_DRAW;
                            gEditDialog->BroadcastMessage(message);
                            gEditDialog->DrawWindow();
                            break;
                        case CELL_WINDOW_EVENT_TOGGLE:
                            gEditCell->m_triggerType ^= MAP_TRIGGER_EVENT;
                            message.type = MESSAGE_WIDGET;
                            message.command = gEditCell->m_triggerType & MAP_TRIGGER_EVENT
                                                  ? WIDGET_COMMAND_SET_FLAGS
                                                  : WIDGET_COMMAND_CLEAR_FLAGS;
                            message.value = WIDGET_FLAG_DRAW;
                            gEditDialog->BroadcastMessage(message);
                            gEditDialog->DrawWindow();
                            break;
                    }
                    break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_ESCAPE:
                    FINISH_DIALOG_SELECT(message);
                    return MESSAGE_DISPATCH_FORWARD;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void eventsManager::EditTown(i16 x, i16 y) {
    mapCell saved;
    i32 unusedResult;

    gEditCell = &gEditManager->m_map.cells[x][y];
    if (gEditCell->m_objectMetadata == 0) {
        NormalDialog(localization::Tr("editor.events.town.old_editor"), NORMAL_DIALOG_TYPE_OK);
        return;
    }
    saved = *gEditCell;
    gTownEdit = *static_cast<editTownExtra*>(gEditManager->m_extras[gEditCell->m_objectMetadata]);
    gEditDialog = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "edittown.bin");
    SetWinText(
        gEditDialog,
        EVENTS_WINDOW_TEXT_TOWN
    );
    UpdateTownWindow(&gTownEdit);
    gWindowManager->DoDialog(gEditDialog, TownWindowHandler, false);
    delete gEditDialog;
    if (gWindowManager->m_dialogResult != EVENTS_DIALOG_CANCEL) {
        gEditManager->m_mapChanged = 1;
        *static_cast<editTownExtra*>(gEditManager->m_extras[gEditCell->m_objectMetadata]) =
            gTownEdit;
    }
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
}

void eventsManager::UpdateTownWindow(editTownExtra* town) {
    tag_message message;
    i32 i;
    char text[20];
    i32 toggleIndex;

    SET_WIDGET_MESSAGE(
        message,
        town->record.customized ? WIDGET_COMMAND_SET_FLAGS : WIDGET_COMMAND_CLEAR_FLAGS,
        TOWN_WINDOW_CUSTOMIZED
    );
    message.value = WIDGET_FLAG_DRAW;
    gEditDialog->BroadcastMessage(message);
    if (!town->record.customized) {
        for (i = TOWN_WINDOW_FIRST_FIELD; i <= TOWN_WINDOW_LAST_FIELD; i++) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i;
            message.value = WIDGET_FLAG_DRAW;
            gEditDialog->BroadcastMessage(message);
        }
    } else {
        for (i = TOWN_WINDOW_FIRST_FIELD; i <= TOWN_WINDOW_LAST_FIELD; i++) {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.id = i;
            message.value = WIDGET_FLAG_DRAW;
            gEditDialog->BroadcastMessage(message);
        }
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.text = text;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            sprintf(text, "%d", town->record.troopCounts[i]);
            message.id = i + EXTRA_WINDOW_FIRST_COUNT;
            gEditDialog->BroadcastMessage(message);
            sprintf(text, "%d", town->record.troopTypes[i]);
            message.id = i + EXTRA_WINDOW_FIRST_TYPE;
            gEditDialog->BroadcastMessage(message);
            sprintf(text, "%s", gArmyNames[town->record.troopTypes[i]]);
            text[0] = CyrillicToUpper(text[0]);
            message.id = i + EXTRA_WINDOW_FIRST_NAME;
            gEditDialog->BroadcastMessage(message);
        }
        sprintf(text, "%d", town->record.buildState + 1);
        message.id = TOWN_WINDOW_MAGE_GUILD;
        gEditDialog->BroadcastMessage(message);
        toggleIndex = 0;
        message.value = WIDGET_FLAG_DRAW;
        for (i = 0; i <= BUILDING_SLOT_DWELLING_LAST; i++) {
            if (i != BUILDING_SLOT_TENT
                && i != BUILDING_SLOT_CASTLE) {
                message.command =
                    town->record.buildings
                            & (1 << i)
                        ? WIDGET_COMMAND_SET_FLAGS
                        : WIDGET_COMMAND_CLEAR_FLAGS;
                message.id = toggleIndex + TOWN_WINDOW_FIRST_BUILDING;
                gEditDialog->BroadcastMessage(message);
                toggleIndex++;
            }
        }
        message.value = WIDGET_FLAG_DRAW;
        for (i = EXTRA_WINDOW_FIRST_OWNER; i < EXTRA_WINDOW_OWNER_END; i++) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i;
            gEditDialog->BroadcastMessage(message);
        }
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.id = town->record.owner + EXTRA_WINDOW_FIRST_PLAYER;
        gEditDialog->BroadcastMessage(message);
    }
}

i16 TownWindowHandler(tag_message& message) {
    const i16 escapeKey = INPUT_SCAN_ESCAPE;
    const i16 toggleBase = CELL_WINDOW_FIRST_FLAG;
    b32 changed = false;
    i32 amount;
    i32 has;
    i32 bit;

    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case EVENTS_DIALOG_CANCEL:
                        case EVENTS_DIALOG_OK:
                            gWindowManager->m_dialogResult = message.id;
                            FINISH_DIALOG_SELECT(message);
                            return MESSAGE_DISPATCH_FORWARD;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case TOWN_WINDOW_CUSTOMIZED:
                            gTownEdit.record.customized = !gTownEdit.record.customized;
                            changed = true;
                            break;
                        case EXTRA_WINDOW_FIRST_COUNT:
                        case EXTRA_WINDOW_FIRST_COUNT + 1:
                        case EXTRA_WINDOW_FIRST_COUNT + 2:
                        case EXTRA_WINDOW_FIRST_COUNT + 3:
                        case EXTRA_WINDOW_FIRST_COUNT + 4:
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            amount = atoi(message.text);
                            if (amount < 0)
                                amount = 0;
                            if (amount > EXTRA_WINDOW_MAX_COUNT)
                                amount = EXTRA_WINDOW_MAX_COUNT;
                            gTownEdit.record.troopCounts[message.id - EXTRA_WINDOW_FIRST_COUNT] =
                                amount;
                            changed = true;
                            break;
                        case EXTRA_WINDOW_FIRST_TYPE:
                        case EXTRA_WINDOW_FIRST_TYPE + 1:
                        case EXTRA_WINDOW_FIRST_TYPE + 2:
                        case EXTRA_WINDOW_FIRST_TYPE + 3:
                        case EXTRA_WINDOW_FIRST_TYPE + 4:
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            amount = atoi(message.text);
                            if (amount < 0 || amount > EXTRA_WINDOW_MAX_TYPE) {
                                NormalDialog(
                                    localization::Tr("editor.events.town.troop_type.range"),
                                    NORMAL_DIALOG_TYPE_OK
                                );
                                amount = 0;
                            }
                            gTownEdit.record.troopTypes[message.id - EXTRA_WINDOW_FIRST_TYPE] =
                                amount;
                            changed = true;
                            break;
                        case EXTRA_WINDOW_FIRST_PLAYER + MAP_TOWN_OWNER_UNSET:
                        case EXTRA_WINDOW_FIRST_PLAYER + GAME_PLAYER_NONE:
                        case EXTRA_WINDOW_FIRST_PLAYER:
                        case EXTRA_WINDOW_FIRST_PLAYER + 1:
                        case EXTRA_WINDOW_FIRST_PLAYER + 2:
                        case EXTRA_WINDOW_FIRST_PLAYER + 3:
                            gTownEdit.record.owner = message.id - EXTRA_WINDOW_FIRST_PLAYER;
                            changed = true;
                            break;
                        case TOWN_WINDOW_FIRST_BUILDING:
                        case TOWN_WINDOW_FIRST_BUILDING + 1:
                        case TOWN_WINDOW_FIRST_BUILDING + 2:
                        case TOWN_WINDOW_FIRST_BUILDING + 3:
                        case TOWN_WINDOW_FIRST_BUILDING + 4:
                        case TOWN_WINDOW_FIRST_BUILDING + 5:
                        case TOWN_WINDOW_FIRST_BUILDING + 6:
                        case TOWN_WINDOW_FIRST_BUILDING + 7:
                        case TOWN_WINDOW_FIRST_BUILDING + 8:
                        case TOWN_WINDOW_FIRST_BUILDING + 9:
                        case TOWN_WINDOW_FIRST_BUILDING + 10:
                            amount = message.id - TOWN_WINDOW_FIRST_BUILDING;
                            if (amount
                                >= BUILDING_SLOT_RACE_FIRST)
                                amount += BUILDING_SLOT_DWELLING_FIRST - BUILDING_SLOT_RACE_FIRST;
                            bit = (1 << amount);
                            has = gTownEdit.record.buildings & bit;
                            if (has)
                                gTownEdit.record.buildings -= bit;
                            else
                                gTownEdit.record.buildings += bit;
                            changed = true;
                            break;
                        case TOWN_WINDOW_MAGE_GUILD:
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            amount = atoi(message.text);
                            if (amount < TOWN_WINDOW_MIN_MAGE_GUILD
                                || amount > TOWN_WINDOW_MAX_MAGE_GUILD) {
                                NormalDialog(
                                    localization::Tr("editor.events.town.mage_guild.range"),
                                    NORMAL_DIALOG_TYPE_OK
                                );
                                amount = TOWN_WINDOW_MIN_MAGE_GUILD;
                            }
                            gTownEdit.record.buildState = amount - 1;
                            changed = true;
                            break;
                    }
                    break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_ESCAPE:
                    FINISH_DIALOG_SELECT(message);
                    return MESSAGE_DISPATCH_FORWARD;
            }
            break;
    }
    if (changed) {
        static_cast<eventsManager*>(gEditManager->m_toolManager)->UpdateTownWindow(&gTownEdit);
        gEditDialog->DrawWindow();
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void eventsManager::EditMonster(i16 x, i16 y) {
    i32 unused;
    char text[20];
    tag_message widgetMessage;

    gEditCell = &gEditManager->m_map.cells[x][y];
    gEditDialog = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "monedit.bin");
    SetWinText(
        gEditDialog,
        EVENTS_WINDOW_TEXT_MONSTER
    );
    gMonsterCountEdit = gEditCell->m_objectMetadata;
    sprintf(text, "%d", gEditCell->m_objectMetadata);
    SET_WIDGET_MESSAGE(widgetMessage, WIDGET_COMMAND_SET_TEXT, MONSTER_WINDOW_COUNT);
    widgetMessage.text = text;
    gEditDialog->BroadcastMessage(widgetMessage);
    gWindowManager->DoDialog(gEditDialog, MonsterWindowHandler, false);
    delete gEditDialog;
    if (gWindowManager->m_dialogResult != EVENTS_DIALOG_CANCEL) {
        gEditManager->m_mapChanged = 1;
        gEditCell->m_objectMetadata = gMonsterCountEdit;
    }
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
}

i16 MonsterWindowHandler(tag_message& message) {
    b8 outOfRange;
    i32 value;
    tag_message reply;

    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case EVENTS_DIALOG_CANCEL:
                        case EVENTS_DIALOG_OK:
                            gWindowManager->m_dialogResult = message.id;
                            FINISH_DIALOG_SELECT(message);
                            return MESSAGE_DISPATCH_FORWARD;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case MONSTER_WINDOW_COUNT:
                            outOfRange = false;
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            value = atoi(message.text);
                            if (value < 0 || value > MONSTER_WINDOW_MAX_COUNT)
                                outOfRange = true;
                            if (value < 0)
                                value = 0;
                            if (value > MONSTER_WINDOW_MAX_COUNT)
                                value = MONSTER_WINDOW_MAX_COUNT;
                            gMonsterCountEdit = value;
                            sprintf(gText, "%d", gMonsterCountEdit);
                            SET_WIDGET_MESSAGE(
                                reply,
                                WIDGET_COMMAND_SET_TEXT,
                                MONSTER_WINDOW_COUNT
                            );
                            reply.text = gText;
                            gEditDialog->BroadcastMessage(reply);
                            gEditDialog->DrawWindow();
                            if (outOfRange)
                                NormalDialog(
                                    localization::Tr("editor.events.monster.count.range"),
                                    NORMAL_DIALOG_TYPE_OK
                                );
                            break;
                    }
                    break;
                case MESSAGE_KEY_DOWN:
                    switch (message.keyCode) {
                        case INPUT_SCAN_ESCAPE:
                            FINISH_DIALOG_SELECT(message);
                            return MESSAGE_DISPATCH_FORWARD;
                    }
                    break;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void eventsManager::EditHero(i16 x, i16 y) {
    mapCell saved;
    i32 unusedResult;

    gEditCell = &gEditManager->m_map.cells[x][y];
    // A hero without a record (placed by the old editor, or uncovered from
    // under another object) has nothing to edit.
    if (gEditCell->m_objectMetadata == 0) {
        NormalDialog(localization::Tr("editor.events.hero.old_editor"), NORMAL_DIALOG_TYPE_OK);
        return;
    }
    saved = *gEditCell;
    gHeroEdit = *static_cast<editHeroExtra*>(gEditManager->m_extras[gEditCell->m_objectMetadata]);
    gEditDialog = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "heroedit.bin");
    SetWinText(
        gEditDialog,
        EVENTS_WINDOW_TEXT_HERO
    );
    UpdateHeroWindow(&gHeroEdit);
    gWindowManager->DoDialog(gEditDialog, HeroWindowHandler, false);
    delete gEditDialog;
    if (gWindowManager->m_dialogResult != EVENTS_DIALOG_CANCEL) {
        gEditManager->m_mapChanged = 1;
        *static_cast<editHeroExtra*>(gEditManager->m_extras[gEditCell->m_objectMetadata]) =
            gHeroEdit;
    }
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
}

void eventsManager::UpdateHeroWindow(editHeroExtra* hero) {
    tag_message message;
    i32 i;
    char text[50];

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.text = text;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        sprintf(text, "%d", hero->record.troopCounts[i]);
        message.id = i + EXTRA_WINDOW_FIRST_COUNT;
        gEditDialog->BroadcastMessage(message);
        sprintf(text, "%d", hero->record.troopTypes[i]);
        message.id = i + EXTRA_WINDOW_FIRST_TYPE;
        gEditDialog->BroadcastMessage(message);
        sprintf(text, "%s", gArmyNames[hero->record.troopTypes[i]]);
        text[0] = CyrillicToUpper(text[0]);
        message.id = i + EXTRA_WINDOW_FIRST_NAME;
        gEditDialog->BroadcastMessage(message);
    }
    message.value = WIDGET_FLAG_DRAW;
    for (i = EXTRA_WINDOW_FIRST_OWNER; i < EXTRA_WINDOW_OWNER_END; i++) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i;
        gEditDialog->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.id = hero->record.owner + EXTRA_WINDOW_FIRST_PLAYER;
    gEditDialog->BroadcastMessage(message);
    sprintf(text, "%d", hero->record.heroId);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_WINDOW_HERO_ID;
    message.text = text;
    gEditDialog->BroadcastMessage(message);
    sprintf(text, "%s", gHeroNames[hero->record.heroId][0]);
    message.id = HERO_WINDOW_HERO_NAME;
    gEditDialog->BroadcastMessage(message);
    for (i = 0; i < MAP_HERO_EXTRA_ARTIFACT_COUNT; i++) {
        sprintf(text, "%d", hero->record.artifacts[i] + 1);
        message.id = i + HERO_WINDOW_FIRST_ARTIFACT;
        gEditDialog->BroadcastMessage(message);
        if (hero->record.artifacts[i] >= ARTIFACT_FIRST)
            sprintf(text, "%s", gArtifactNames[hero->record.artifacts[i]]);
        else
            sprintf(text, "(none)");
        message.id = i + HERO_WINDOW_FIRST_ARTIFACT_NAME;
        gEditDialog->BroadcastMessage(message);
    }
    sprintf(text, "%d", hero->record.experience);
    message.id = HERO_WINDOW_EXPERIENCE;
    gEditDialog->BroadcastMessage(message);
}

i16 HeroWindowHandler(tag_message& message) {
    b32 changed = false;
    i32 amount;
    i32 fieldIndex;

    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case EVENTS_DIALOG_CANCEL:
                        case EVENTS_DIALOG_OK:
                            gWindowManager->m_dialogResult = message.id;
                            FINISH_DIALOG_SELECT(message);
                            return MESSAGE_DISPATCH_FORWARD;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case EXTRA_WINDOW_FIRST_COUNT:
                        case EXTRA_WINDOW_FIRST_COUNT + 1:
                        case EXTRA_WINDOW_FIRST_COUNT + 2:
                        case EXTRA_WINDOW_FIRST_COUNT + 3:
                        case EXTRA_WINDOW_FIRST_COUNT + 4:
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            amount = atoi(message.text);
                            if (amount < 0)
                                amount = 0;
                            if (amount > EXTRA_WINDOW_MAX_COUNT)
                                amount = EXTRA_WINDOW_MAX_COUNT;
                            gHeroEdit.record.troopCounts[message.id - EXTRA_WINDOW_FIRST_COUNT] =
                                amount;
                            changed = true;
                            break;
                        case EXTRA_WINDOW_FIRST_TYPE:
                        case EXTRA_WINDOW_FIRST_TYPE + 1:
                        case EXTRA_WINDOW_FIRST_TYPE + 2:
                        case EXTRA_WINDOW_FIRST_TYPE + 3:
                        case EXTRA_WINDOW_FIRST_TYPE + 4:
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            amount = atoi(message.text);
                            if (amount < 0 || amount > EXTRA_WINDOW_MAX_TYPE) {
                                NormalDialog(
                                    localization::Tr("editor.events.hero.troop_type.range"),
                                    NORMAL_DIALOG_TYPE_OK
                                );
                                amount = 0;
                            }
                            gHeroEdit.record.troopTypes[message.id - EXTRA_WINDOW_FIRST_TYPE] =
                                amount;
                            changed = true;
                            break;
                        case EXTRA_WINDOW_FIRST_PLAYER:
                        case EXTRA_WINDOW_FIRST_PLAYER + 1:
                        case EXTRA_WINDOW_FIRST_PLAYER + 2:
                        case EXTRA_WINDOW_FIRST_PLAYER + 3:
                            gHeroEdit.record.owner = message.id - EXTRA_WINDOW_FIRST_PLAYER;
                            changed = true;
                            break;
                        case HERO_WINDOW_HERO_ID:
                        case HERO_WINDOW_FIRST_ARTIFACT:
                        case HERO_WINDOW_FIRST_ARTIFACT + 1:
                        case HERO_WINDOW_FIRST_ARTIFACT + 2:
                        case HERO_WINDOW_FIRST_ARTIFACT + 3:
                        case HERO_WINDOW_EXPERIENCE:
                            fieldIndex = message.id - HERO_WINDOW_HERO_ID;
                            message.command = WIDGET_COMMAND_GET_TEXT;
                            gEditDialog->BroadcastMessage(message);
                            amount = atoi(message.text);
                            if (fieldIndex == HERO_FIELD_HERO_ID) {
                                if (amount < 0)
                                    amount = 0;
                                if (amount > GAME_HERO_COUNT - 1)
                                    amount = GAME_HERO_COUNT - 1;
                                gHeroEdit.record.heroId = amount;
                            }
                            if (fieldIndex >= HERO_FIELD_FIRST_ARTIFACT
                                && fieldIndex <= HERO_FIELD_LAST_ARTIFACT) {
                                if (amount < 0)
                                    amount = 0;
                                if (amount > HERO_WINDOW_MAX_ARTIFACT)
                                    amount = HERO_WINDOW_MAX_ARTIFACT;
                                gHeroEdit.record.artifacts[fieldIndex - HERO_FIELD_FIRST_ARTIFACT] =
                                    (amount - 1);
                            }
                            if (fieldIndex == HERO_FIELD_EXPERIENCE) {
                                if (amount < 0)
                                    amount = 0;
                                if (amount > HERO_WINDOW_MAX_EXPERIENCE)
                                    amount = HERO_WINDOW_MAX_EXPERIENCE;
                                gHeroEdit.record.experience = amount;
                            }
                            changed = true;
                            break;
                    }
                    break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_ESCAPE:
                    FINISH_DIALOG_SELECT(message);
                    return MESSAGE_DISPATCH_FORWARD;
            }
            break;
    }
    if (changed) {
        static_cast<eventsManager*>(gEditManager->m_toolManager)->UpdateHeroWindow(&gHeroEdit);
        gEditDialog->DrawWindow();
    }
    return MESSAGE_DISPATCH_CONSUME;
}

b32 ClearOptionsDialog(void) {
    i32 unusedResult;
    const i16 firstBitId = CLEAR_WINDOW_FIRST_TOGGLE;

    gClearWindow = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "clearwin.bin");
    SetWinText(
        gClearWindow,
        EVENTS_WINDOW_TEXT_CLEAR
    );
    UpdateClearWindow();
    gWindowManager->DoDialog(gClearWindow, ClearWindowHandler, false);
    delete gClearWindow;
    if (gWindowManager->m_dialogResult == EVENTS_DIALOG_OK
        && (gClearFlags & CLEAR_FLAG_WHOLE_MAP)) {
        gEditManager->SaveUndo();
        gEditManager->ClearArea(
            0,
            0,
            MAP_CELL_GRID_SIZE,
            MAP_CELL_GRID_SIZE,
            gClearFlags & EDIT_CLEAR_ALL,
            false
        );
    }
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
    if (gWindowManager->m_dialogResult == EVENTS_DIALOG_OK)
        return true;
    return false;
}

void UpdateClearWindow(void) {
    const i16 toggleBase = CLEAR_WINDOW_FIRST_TOGGLE;
    tag_message message;
    i32 i;

    message.type = MESSAGE_WIDGET;
    message.value = WIDGET_FLAG_DRAW;
    for (i = 0; i < CLEAR_WINDOW_TOGGLE_COUNT; i++) {
        if (gClearFlags & (1 << i))
            message.command = WIDGET_COMMAND_SET_FLAGS;
        else
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + toggleBase;
        gClearWindow->BroadcastMessage(message);
    }
}

i16 ClearWindowHandler(tag_message& message) {
    const i16 firstToggleId = CLEAR_WINDOW_FIRST_TOGGLE;
    b32 wasSet;
    i32 flagIndex;

    if (message.type != MESSAGE_WIDGET)
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.command) {
        case WIDGET_NOTIFY_DESELECT:
            switch (message.id) {
                case EVENTS_DIALOG_CANCEL:
                case EVENTS_DIALOG_OK:
                    gWindowManager->m_dialogResult = message.id;
                    FINISH_DIALOG_SELECT(message);
                    return MESSAGE_DISPATCH_FORWARD;
            }
            break;
        case WIDGET_NOTIFY_SELECT:
            if (message.id >= CLEAR_WINDOW_FIRST_TOGGLE && message.id <= CLEAR_WINDOW_LAST_TOGGLE) {
                flagIndex = message.id - firstToggleId;
                wasSet = (gClearFlags & (1 << flagIndex)) != 0;
                if (flagIndex <= CLEAR_WINDOW_LAST_CLASS) {
                    gClearFlags &= CLEAR_FLAG_ALL - CLEAR_FLAG_EVERYTHING;
                    if (wasSet)
                        gClearFlags &= CLEAR_FLAG_ALL - (1 << flagIndex);
                    else
                        gClearFlags |= 1 << flagIndex;
                } else {
                    if (wasSet)
                        gClearFlags &= CLEAR_FLAG_ALL - (1 << flagIndex);
                    else
                        gClearFlags |= 1 << flagIndex;
                    if (flagIndex == CLEAR_WINDOW_EVERYTHING && !wasSet)
                        gClearFlags |= CLEAR_FLAG_EVERY_CLASS;
                }
                UpdateClearWindow();
                gClearWindow->DrawWindow();
            }
            break;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

i32 MapDetailsDialog(b32) {
    SMapHeader saved;

    saved = *gMapHeader;
    gDetailsWindow = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "dtlwind.bin");
    if (gDetailsWindow == NULL)
        MemError();
    SetWinText(
        gDetailsWindow,
        EVENTS_WINDOW_TEXT_MAP_DETAILS
    );
    UpdateMapDetailsWindow();
    gWindowManager->DoDialog(gDetailsWindow, MapDetailsWindowHandler, false);
    delete gDetailsWindow;
    gDetailsWindow = NULL;
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
    if (gWindowManager->m_dialogResult == EVENTS_DIALOG_CANCEL)
        *gMapHeader = saved;
    return gWindowManager->m_dialogResult != EVENTS_DIALOG_CANCEL;
}

void UpdateMapDetailsWindow(void) {
    tag_message message;
    i32 i;

    message.type = MESSAGE_WIDGET;
    message.value = WIDGET_FLAG_DRAW;
    for (i = 0; i < DETAILS_WINDOW_DIFFICULTY_COUNT; i++) {
        if (i == gMapHeader->difficulty)
            message.command = WIDGET_COMMAND_SET_FLAGS;
        else
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + DETAILS_WINDOW_FIRST_DIFFICULTY;
        gDetailsWindow->BroadcastMessage(message);
    }
    for (i = 0; i < DETAILS_WINDOW_SIZE_COUNT; i++) {
        if (i == gMapHeader->size)
            message.command = WIDGET_COMMAND_SET_FLAGS;
        else
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + DETAILS_WINDOW_FIRST_SIZE;
        gDetailsWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = DETAILS_WINDOW_NAME;
    message.text = gMapHeader->name[0];
    gDetailsWindow->BroadcastMessage(message);
    message.id = DETAILS_WINDOW_DESCRIPTION;
    message.text = gMapHeader->description[0];
    gDetailsWindow->BroadcastMessage(message);
    message.id = DETAILS_WINDOW_MAP_CODE;
    strcpy(gText, gEditManager->m_mapFileName);
    gText[DETAILS_WINDOW_MAP_CODE_LENGTH] = '\0';
    message.text = gText;
    gDetailsWindow->BroadcastMessage(message);
}

i16 MapDetailsWindowHandler(tag_message& message) {
    bool modified = false;
    tag_message request;
    i32 i;

    SET_WIDGET_MESSAGE(request, WIDGET_COMMAND_GET_TEXT, message.id);
    if (message.type != MESSAGE_WIDGET)
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.command) {
        case WIDGET_NOTIFY_DESELECT:
            switch (message.id) {
                case EVENTS_DIALOG_CANCEL:
                case EVENTS_DIALOG_OK:
                    gWindowManager->m_dialogResult = message.id;
                    FINISH_DIALOG_SELECT(message);
                    return MESSAGE_DISPATCH_FORWARD;
            }
            break;
        case WIDGET_NOTIFY_SELECT:
            modified = true;
            if (message.id >= DETAILS_WINDOW_FIRST_DIFFICULTY
                && message.id
                       <= DETAILS_WINDOW_FIRST_DIFFICULTY + DETAILS_WINDOW_DIFFICULTY_COUNT - 1)
                gMapHeader->difficulty =
                    (message.id - DETAILS_WINDOW_FIRST_DIFFICULTY);
            else if (message.id >= DETAILS_WINDOW_FIRST_SIZE
                     && message.id <= DETAILS_WINDOW_FIRST_SIZE + DETAILS_WINDOW_SIZE_COUNT - 1)
                gMapHeader->size = (message.id - DETAILS_WINDOW_FIRST_SIZE);
            else if (message.id == DETAILS_WINDOW_NAME) {
                gDetailsWindow->BroadcastMessage(request);
                for (i = 0; i < MAP_HEADER_LANGUAGE_COUNT; i++)
                    strcpy(gMapHeader->name[i], request.text);
            } else if (message.id == DETAILS_WINDOW_DESCRIPTION) {
                gDetailsWindow->BroadcastMessage(request);
                for (i = 0; i < MAP_HEADER_LANGUAGE_COUNT; i++)
                    strcpy(gMapHeader->description[i], request.text);
            } else if (message.id == DETAILS_WINDOW_MAP_CODE) {
                gDetailsWindow->BroadcastMessage(request);
                strcpy(gText, request.text);
                for (i = 0; i < DETAILS_WINDOW_MAP_CODE_LENGTH; i++) {
                    if (i >= strlen(gText))
                        gEditManager->m_mapFileName[i] = '-';
                    else if ((static_cast<u8>(gText[i]) >= 'A' && static_cast<u8>(gText[i]) <= 'Z')
                             || (static_cast<u8>(gText[i]) >= 'a'
                                 && static_cast<u8>(gText[i]) <= 'z')
                             || (static_cast<u8>(gText[i]) >= '0'
                                 && static_cast<u8>(gText[i]) <= '9')
                             || (static_cast<u8>(gText[i]) >= CYRILLIC_CAPITAL_A
                                 && static_cast<u8>(gText[i]) <= CYRILLIC_CAPITAL_YA)
                             || (static_cast<u8>(gText[i]) >= CYRILLIC_SMALL_A
                                 && static_cast<u8>(gText[i]) <= CYRILLIC_SMALL_YA)
                             || static_cast<u8>(gText[i]) == CYRILLIC_CAPITAL_YO
                             || static_cast<u8>(gText[i]) == CYRILLIC_SMALL_YO
                             || static_cast<u8>(gText[i]) == '-')
                        gEditManager->m_mapFileName[i] = gText[i];
                    else
                        gEditManager->m_mapFileName[i] = '-';
                }
            } else
                modified = false;
            break;
    }
    if (modified) {
        UpdateMapDetailsWindow();
        gDetailsWindow->DrawWindow();
    }
    return MESSAGE_DISPATCH_CONSUME;
}

b32 NewMapDialog(void) {
    i32 i;

    gNewMapWindow = new heroWindow(EVENTS_DIALOG_X, EVENTS_DIALOG_Y, "editnew.bin");
    if (gNewMapWindow == NULL)
        MemError();
    SetWinText(
        gNewMapWindow,
        EVENTS_WINDOW_TEXT_NEW_MAP
    );
    for (i = 0; i < EDITOR_TERRAIN_COUNT; i++) {
        gTerrainTracks[i] = new iconWidget(
            NEW_MAP_TRACK_X,
            i * NEW_MAP_ROW_HEIGHT + NEW_MAP_FIRST_TERRAIN_Y,
            NEW_MAP_TRACK_WIDTH,
            NEW_MAP_TRACK_HEIGHT,
            "escroll.icn",
            EDIT_SCROLL_SHORT_TRACK,
            ICON_DRAW_NORMAL,
            i + NEW_MAP_FIRST_TERRAIN_TRACK,
            ICON_WIDGET_DRAW,
            1
        );
        gNewMapWindow->AddWidget(gTerrainTracks[i], WINDOW_Z_ORDER_APPEND);
        gTerrainKnobs[i] = new iconWidget(
            NEW_MAP_KNOB_X,
            i * NEW_MAP_ROW_HEIGHT + NEW_MAP_FIRST_TERRAIN_Y + NEW_MAP_KNOB_Y_OFFSET,
            NEW_MAP_KNOB_WIDTH,
            NEW_MAP_KNOB_HEIGHT,
            "escroll.icn",
            EDIT_SCROLL_HORIZONTAL_KNOB,
            ICON_DRAW_NORMAL,
            i + NEW_MAP_FIRST_TERRAIN_KNOB,
            ICON_WIDGET_DRAW,
            1
        );
        gNewMapWindow->AddWidget(gTerrainKnobs[i], WINDOW_Z_ORDER_APPEND);
    }
    for (i = 0; i < EDITOR_GENERATOR_DENSITY_COUNT; i++) {
        gDensityTracks[i] = new iconWidget(
            NEW_MAP_TRACK_X,
            i * NEW_MAP_ROW_HEIGHT + NEW_MAP_FIRST_DENSITY_Y,
            NEW_MAP_TRACK_WIDTH,
            NEW_MAP_TRACK_HEIGHT,
            "escroll.icn",
            EDIT_SCROLL_SHORT_TRACK,
            ICON_DRAW_NORMAL,
            i + NEW_MAP_FIRST_DENSITY_TRACK,
            ICON_WIDGET_DRAW,
            1
        );
        gNewMapWindow->AddWidget(gDensityTracks[i], WINDOW_Z_ORDER_APPEND);
        gDensityKnobs[i] = new iconWidget(
            NEW_MAP_KNOB_X,
            i * NEW_MAP_ROW_HEIGHT + NEW_MAP_FIRST_DENSITY_Y + NEW_MAP_KNOB_Y_OFFSET,
            NEW_MAP_KNOB_WIDTH,
            NEW_MAP_KNOB_HEIGHT,
            "escroll.icn",
            EDIT_SCROLL_HORIZONTAL_KNOB,
            ICON_DRAW_NORMAL,
            i + NEW_MAP_FIRST_DENSITY_KNOB,
            ICON_WIDGET_DRAW,
            1
        );
        gNewMapWindow->AddWidget(gDensityKnobs[i], WINDOW_Z_ORDER_APPEND);
    }
    UpdateNewMapWindow();
    gWindowManager->DoDialog(gNewMapWindow, NewMapWindowHandler, false);
    delete gNewMapWindow;
    gNewMapWindow = NULL;
    BalanceTerrainPercents(TERRAIN_INVALID);
    if (gWindowManager->m_dialogResult == EVENTS_DIALOG_CANCEL)
        return false;
    return true;
}

void UpdateNewMapWindow(void) {
    tag_message message;
    i32 i;

    for (i = 0; i < EDITOR_TERRAIN_COUNT; i++)
        gTerrainKnobs[i]->m_x = NEW_MAP_KNOB_TRAVEL * gTerrainPercent[i] / 100.0 + NEW_MAP_KNOB_X;
    for (i = 0; i < EDITOR_GENERATOR_DENSITY_COUNT; i++)
        gDensityKnobs[i]->m_x = NEW_MAP_KNOB_TRAVEL * gDensityPercent[i] / 100.0 + NEW_MAP_KNOB_X;
    message.type = MESSAGE_WIDGET;
    message.value = WIDGET_FLAG_DRAW;
    message.id = NEW_MAP_SCATTER_TOWNS;
    message.command = gScatterTowns ? WIDGET_COMMAND_SET_FLAGS : WIDGET_COMMAND_CLEAR_FLAGS;
    gNewMapWindow->BroadcastMessage(message);
    message.id = NEW_MAP_CENTRE_TOWNS;
    message.command = gScatterTowns ? WIDGET_COMMAND_CLEAR_FLAGS : WIDGET_COMMAND_SET_FLAGS;
    gNewMapWindow->BroadcastMessage(message);
    message.id = NEW_MAP_SAVE_UNSEEN;
    message.command = gSaveUnseen ? WIDGET_COMMAND_SET_FLAGS : WIDGET_COMMAND_CLEAR_FLAGS;
    gNewMapWindow->BroadcastMessage(message);
}

void BalanceTerrainPercents(i32 changedTerrain) {
    double othersTotal;
    double remaining;
    double ratio;
    i32 i;
    double landTotal;

    // Called with TERRAIN_INVALID (-1) when the dialog closes: nothing has
    // changed then.
    remaining = 100.0 - (changedTerrain >= 0 ? gTerrainPercent[changedTerrain] : 0.0);
    othersTotal = 0.0;
    landTotal = 0.0;
    for (i = 0; i < EDITOR_TERRAIN_COUNT; i++)
        if (i != changedTerrain)
            othersTotal += gTerrainPercent[i];
    for (i = TERRAIN_GRASS; i < EDITOR_TERRAIN_COUNT; i++)
        landTotal += gTerrainPercent[i];
    if (changedTerrain != TERRAIN_INVALID) {
        if (landTotal < NEW_MAP_MINIMUM_LAND)
            gTerrainPercent[TERRAIN_GRASS] +=
                NEW_MAP_MINIMUM_LAND - landTotal;
    } else {
        if (othersTotal < 1.0) {
            othersTotal = 1.0;
            if (gTerrainPercent[TERRAIN_WATER] < 1.0)
                gTerrainPercent[TERRAIN_WATER] = 1.0;
            else
                gTerrainPercent[TERRAIN_GRASS] = 1.0;
        }
        ratio = remaining / othersTotal;
        for (i = 0; i < EDITOR_TERRAIN_COUNT; i++)
            if (i != changedTerrain)
                gTerrainPercent[i] = ratio * gTerrainPercent[i];
        if (gTerrainPercent[TERRAIN_WATER]
            > NEW_MAP_MAXIMUM_WATER + 0.5) {
            gTerrainPercent[TERRAIN_WATER] = NEW_MAP_MAXIMUM_WATER;
            BalanceTerrainPercents(TERRAIN_WATER);
        }
    }
}

i16 NewMapWindowHandler(tag_message& message) {
    bool redraw = false;
    i32 index;

    if (message.type != MESSAGE_WIDGET)
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.command) {
        case WIDGET_NOTIFY_DESELECT:
            redraw = true;
            if (message.id >= NEW_MAP_FIRST_TERRAIN_DECREASE
                && message.id < NEW_MAP_FIRST_TERRAIN_DECREASE + EDITOR_TERRAIN_COUNT) {
                index = message.id - NEW_MAP_FIRST_TERRAIN_DECREASE;
                gTerrainPercent[index] -= 1.0;
                if (gTerrainPercent[index] < 0.0)
                    gTerrainPercent[index] = 0.0;
                BalanceTerrainPercents(index);
            } else if (message.id >= NEW_MAP_FIRST_TERRAIN_INCREASE
                       && message.id < NEW_MAP_FIRST_TERRAIN_INCREASE + EDITOR_TERRAIN_COUNT) {
                index = message.id - NEW_MAP_FIRST_TERRAIN_INCREASE;
                gTerrainPercent[index] += 1.0;
                if (gTerrainPercent[index] > 100.0)
                    gTerrainPercent[index] = 100.0;
                BalanceTerrainPercents(index);
            } else if (message.id >= NEW_MAP_FIRST_DENSITY_DECREASE
                       && message.id
                              < NEW_MAP_FIRST_DENSITY_DECREASE + EDITOR_GENERATOR_DENSITY_COUNT) {
                index = message.id - NEW_MAP_FIRST_DENSITY_DECREASE;
                gDensityPercent[index] -= 1.0;
                if (gDensityPercent[index] < 0.0)
                    gDensityPercent[index] = 0.0;
            } else if (message.id >= NEW_MAP_FIRST_DENSITY_INCREASE
                       && message.id
                              < NEW_MAP_FIRST_DENSITY_INCREASE + EDITOR_GENERATOR_DENSITY_COUNT) {
                index = message.id - NEW_MAP_FIRST_DENSITY_INCREASE;
                gDensityPercent[index] += 1.0;
                if (gDensityPercent[index] > 100.0)
                    gDensityPercent[index] = 100.0;
            } else {
                redraw = false;
                if (message.id == EVENTS_DIALOG_OK || message.id == EVENTS_DIALOG_CANCEL) {
                    gWindowManager->m_dialogResult = message.id;
                    FINISH_DIALOG_SELECT(message);
                    return MESSAGE_DISPATCH_FORWARD;
                }
            }
            break;
        case WIDGET_NOTIFY_SELECT:
            if (message.id >= NEW_MAP_FIRST_TERRAIN_TRACK
                && message.id < NEW_MAP_FIRST_TERRAIN_TRACK + EDITOR_TERRAIN_COUNT)
                DragNewMapSlider(true, message.id - NEW_MAP_FIRST_TERRAIN_TRACK);
            else if (message.id >= NEW_MAP_FIRST_TERRAIN_KNOB
                     && message.id < NEW_MAP_FIRST_TERRAIN_KNOB + EDITOR_TERRAIN_COUNT)
                DragNewMapSlider(true, message.id - NEW_MAP_FIRST_TERRAIN_KNOB);
            else if (message.id >= NEW_MAP_FIRST_DENSITY_TRACK
                     && message.id < NEW_MAP_FIRST_DENSITY_TRACK + EDITOR_TERRAIN_COUNT)
                DragNewMapSlider(false, message.id - NEW_MAP_FIRST_DENSITY_TRACK);
            else if (message.id >= NEW_MAP_FIRST_DENSITY_KNOB
                     && message.id < NEW_MAP_FIRST_DENSITY_KNOB + EDITOR_TERRAIN_COUNT)
                DragNewMapSlider(false, message.id - NEW_MAP_FIRST_DENSITY_KNOB);
            if (message.id >= NEW_MAP_SCATTER_TOWNS && message.id <= NEW_MAP_CENTRE_TOWNS) {
                gScatterTowns = message.id == NEW_MAP_SCATTER_TOWNS;
                redraw = true;
            }
            if (message.id == NEW_MAP_SAVE_UNSEEN) {
                gSaveUnseen = 1 - gSaveUnseen;
                redraw = true;
            }
            break;
    }
    if (redraw) {
        UpdateNewMapWindow();
        gNewMapWindow->DrawWindow();
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void DragNewMapSlider(b32 terrainRow, i32 index) {
    tag_message last;
    double knobPercent;
    i16 x;
    i16 y;
    tag_message event;

    gMouseManager->SetCursorShape(EDIT_CURSOR_HORIZONTAL_DRAG);
    gMouseManager->MouseCoords(x, y);
    gInputManager->Flush();
    event.type = MESSAGE_MOUSE_MOVE;
    event.x = x;
    event.y = y;
    while (!IS_BUTTON_RELEASE_MESSAGE(event.type)) {
        Process1WindowsMessage();
        if (event.type == MESSAGE_MOUSE_MOVE) {
            last = event;
            while (event.type == MESSAGE_MOUSE_MOVE) {
                last = event;
                event = gInputManager->GetEvent();
            }
            last.x -= EVENTS_DIALOG_X;
            last.x -= NEW_MAP_KNOB_HALF_WIDTH;
            if (last.x < NEW_MAP_KNOB_X)
                last.x = NEW_MAP_KNOB_X;
            if (last.x > NEW_MAP_KNOB_RIGHT)
                last.x = NEW_MAP_KNOB_RIGHT;
            gMouseManager->Main(last);
            knobPercent = (last.x - NEW_MAP_KNOB_X) * 100 / (NEW_MAP_KNOB_RIGHT - NEW_MAP_KNOB_X);
            if (terrainRow) {
                gTerrainPercent[index] = knobPercent;
                BalanceTerrainPercents(index);
            } else
                gDensityPercent[index] = knobPercent;
            UpdateNewMapWindow();
            gNewMapWindow->DrawWindow();
        } else
            event = gInputManager->GetEvent();
    }
    gMouseManager->SetCursorShape(EDIT_CURSOR_NORMAL);
    gInputManager->Flush();
    if (terrainRow) {
        gTerrainKnobs[index]->m_flags &= ~WIDGET_FLAG_SELECTED;
        gTerrainTracks[index]->m_flags &= ~WIDGET_FLAG_SELECTED;
    } else {
        gDensityKnobs[index]->m_flags &= ~WIDGET_FLAG_SELECTED;
        gDensityTracks[index]->m_flags &= ~WIDGET_FLAG_SELECTED;
    }
}
