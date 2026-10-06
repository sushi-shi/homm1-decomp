#include <H1/Ints.h>

#include <BASE/bmap2.h>
#include <BASE/border.h>
#include <BASE/button.h>
#include <BASE/font.h>
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
#include <EDITOR/overlayManager.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>

#include <stdio.h>
#include <string.h>

#define OVERLAY_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Editor\\OVERLAY.CPP"

i16 gSelectedOverlay = OVERLAY_NO_SELECTION;
i32 gOverlayCategoryKinds[OVERLAY_CATEGORY_COUNT] = {
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TERRAIN,
    OVERLAY_KIND_TOWN,
    OVERLAY_KIND_MONSTER,
    OVERLAY_KIND_ARTIFACT,
    OVERLAY_KIND_TREASURE
};

overlayManager::overlayManager(void) {
    m_dispatchMask = EDIT_MANAGER_DISPATCH_MASK;
}

i16 overlayManager::Open(i16 priority) {
    gOverlayCategory = gOverlayShownCategory;
    m_previewDrawn = false;
    m_panel = new iconWidget(
        EDIT_TOOL_PANEL_X,
        EDIT_TOOL_PANEL_Y,
        EDIT_TOOL_PANEL_WIDTH,
        EDIT_TOOL_PANEL_HEIGHT,
        "buttons.icn",
        EDIT_FRAME_TOOL_PANEL,
        ICON_DRAW_NORMAL,
        WIDGET_ID_NONE,
        ICON_WIDGET_DRAW,
        1
    );
    gEditManager->m_window->AddWidget(m_panel, WINDOW_Z_ORDER_APPEND);
    m_font = gResourceManager->GetFont("smalfont.fnt");
    m_icon = gResourceManager->GetIcon("overlay.icn");
    m_previewBorder = new border(
        OVERLAY_PREVIEW_X,
        OVERLAY_PREVIEW_Y,
        OVERLAY_PREVIEW_WIDTH,
        OVERLAY_PREVIEW_HEIGHT,
        OVERLAY_PREVIEW_BORDER,
        WIDGET_KIND_TRANSPARENT,
        0,
        NULL
    );
    gEditManager->m_window->AddWidget(m_previewBorder, WINDOW_Z_ORDER_APPEND);
    m_previousButton = new button(
        OVERLAY_PREVIOUS_CATEGORY_X,
        OVERLAY_CATEGORY_BUTTON_Y,
        OVERLAY_CATEGORY_BUTTON_SIZE,
        OVERLAY_CATEGORY_BUTTON_SIZE,
        "escroll.icn",
        EDIT_SCROLL_LEFT_ARROW,
        EDIT_SCROLL_LEFT_ARROW_PRESSED,
        0,
        BUTTON_NO_HOTKEY,
        OVERLAY_PREVIOUS_CATEGORY_BUTTON,
        WIDGET_KIND_DEFAULT
    );
    m_nextButton = new button(
        OVERLAY_NEXT_CATEGORY_X,
        OVERLAY_CATEGORY_BUTTON_Y,
        OVERLAY_CATEGORY_BUTTON_SIZE,
        OVERLAY_CATEGORY_BUTTON_SIZE,
        "escroll.icn",
        EDIT_SCROLL_RIGHT_ARROW,
        EDIT_SCROLL_RIGHT_ARROW_PRESSED,
        0,
        BUTTON_NO_HOTKEY,
        OVERLAY_NEXT_CATEGORY_BUTTON,
        WIDGET_KIND_DEFAULT
    );
    gEditManager->m_window->AddWidget(m_previousButton, WINDOW_Z_ORDER_APPEND);
    gEditManager->m_window->AddWidget(m_nextButton, WINDOW_Z_ORDER_APPEND);
    gEditManager->m_window->DrawWindow();
    gEditManager->m_window->RemoveWidget(m_panel);
    delete m_panel;
    m_icon->DrawToBuffer(
        OVERLAY_PREVIEW_X,
        OVERLAY_PREVIEW_Y,
        OVERLAY_PREVIEW_FRAME,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL
    );
    DrawCategoryName(true);
    gEditManager->m_placedY = EDIT_NO_CELL;
    gEditManager->m_placedX = EDIT_NO_CELL;
    LoadCategory(gOverlayCategory);
    DrawSelectedOverlay();
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "overlayManager");
    return BASE_MANAGER_SUCCESS;
}

void overlayManager::Close(void) {
    gResourceManager->Dispose(m_icon);
    gEditManager->m_window->RemoveWidget(m_previewBorder);
    delete m_previewBorder;
    gEditManager->m_window->RemoveWidget(m_nextButton);
    delete m_nextButton;
    gEditManager->m_window->RemoveWidget(m_previousButton);
    delete m_previousButton;
    gResourceManager->Dispose(m_font);
    gEditManager->m_window->DrawWindow(0);
    gEditManager->DrawMap();
    gEditManager->UpdateMapView();
    m_active = 0;
}

i16 overlayManager::Main(tag_message& message) {
    b32 finished;
    i32 objectPlaced;
    i32 helpItem;
    i8 previousCategory;
    i16 cellX;
    i16 cellY;

    finished = false;
    helpItem = OVERLAY_TOOL_HELP_NONE;
    if (!(message.type & m_dispatchMask))
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                        switch (message.id) {
                            case OVERLAY_PREVIEW_BORDER:
                                helpItem = OVERLAY_TOOL_HELP_SELECTED;
                                break;
                        }
                        if (helpItem >= 0)
                            NormalDialog(gOverlayToolHelp[helpItem], NORMAL_DIALOG_TYPE_QUICK_VIEW);
                        break;
                    }
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            if (gSelectedOverlay < 0)
                                break;
                            ClearStatusText();
                            gEditManager->SaveUndo();
                            if (m_types[gSelectedOverlay].flags & OVERLAY_TYPE_RESOURCE_MARKER)
                                objectPlaced = PlaceMineResource(
                                    &m_types[gSelectedOverlay],
                                    gEditManager->m_viewX + gEditManager->m_cursorX,
                                    gEditManager->m_viewY + gEditManager->m_cursorY,
                                    true
                                );
                            else
                                objectPlaced = PlaceOverlay(
                                    &m_types[gSelectedOverlay],
                                    gEditManager->m_viewX + gEditManager->m_cursorX,
                                    gEditManager->m_viewY + gEditManager->m_cursorY
                                );
                            if (objectPlaced) {
                                gEditManager->m_placedX =
                                    gEditManager->m_viewX + gEditManager->m_cursorX;
                                gEditManager->m_placedY =
                                    gEditManager->m_viewY + gEditManager->m_cursorY;
                                gEditManager->m_placedState = 0;
                                gEditManager->DrawMap();
                                gEditManager->UpdateMapView();
                                gEditManager->DrawRadar(true);
                                gEditManager->m_mapChanged = 1;
                            }
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case OVERLAY_NEXT_CATEGORY_BUTTON:
                            gOverlayShownCategory =
                                (gOverlayShownCategory + OVERLAY_CATEGORY_COUNT + 1)
                                % OVERLAY_CATEGORY_COUNT;
                            goto showCategory;
                        case OVERLAY_PREVIOUS_CATEGORY_BUTTON:
                            gOverlayShownCategory =
                                (gOverlayShownCategory + OVERLAY_CATEGORY_COUNT - 1)
                                % OVERLAY_CATEGORY_COUNT;
                        showCategory:
                            gSelectedOverlay = OVERLAY_NO_SELECTION;
                            DrawSelectedOverlay();
                            DrawCategoryName(true);
                            break;
                        case OVERLAY_PREVIEW_BORDER:
                            previousCategory = gOverlayCategory;
                            gOverlayCategory = gOverlayShownCategory;
                            gSelectedOverlay = PickOverlay(gOverlayCategory);
                            DrawSelectedOverlay();
                            DrawCategoryName(true);
                            gInputManager->Flush();
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    if (message.id != EDIT_CONTROL_MAP) {
                        if (m_previewDrawn) {
                            m_previewDrawn = false;
                            gEditManager->DrawMap();
                            gEditManager->UpdateMapView();
                            gEditManager->UpdateCursor();
                        }
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    gEditManager->m_lastHoverId = message.id;
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            gMouseManager->MouseCoords(cellX, cellY);
                            gEditManager->ScreenToCell(cellX, cellY);
                            if (gEditManager->m_cursorX != cellX
                                || gEditManager->m_cursorY != cellY) {
                                if (gSelectedOverlay != OVERLAY_NO_SELECTION) {
                                    if (cellX + m_width
                                        > (gEditManager->m_zoomedOut
                                               ? EDIT_VIEW_ZOOMED_CELLS
                                               : EDIT_VIEW_CELLS))
                                        cellX = !gEditManager->m_zoomedOut
                                                    ? EDIT_VIEW_CELLS - m_width
                                                    : EDIT_VIEW_ZOOMED_CELLS - m_width;
                                    if (cellY - m_height + 1 < 0)
                                        cellY = m_height - 1;
                                    gEditManager->DrawMap();
                                    gEditManager->m_cursorX = cellX;
                                    gEditManager->m_cursorY = cellY;
                                    cellX =
                                        cellX
                                            * (gEditManager->m_zoomedOut
                                                   ? EDIT_VIEW_ZOOMED_CELL_PIXELS
                                                   : EDIT_VIEW_CELL_PIXELS)
                                        + EDIT_VIEW_LEFT;
                                    cellY =
                                        cellY
                                            * (gEditManager->m_zoomedOut
                                                   ? EDIT_VIEW_ZOOMED_CELL_PIXELS
                                                   : EDIT_VIEW_CELL_PIXELS)
                                        + EDIT_VIEW_TOP;
                                    if (gSelectedOverlay != OVERLAY_NO_SELECTION) {
                                        DrawFootprint(
                                            cellX,
                                            cellY,
                                            m_types[gSelectedOverlay].footprintMask,
                                            m_types[gSelectedOverlay].overlayMask,
                                            OVERLAY_FOOTPRINT_COLUMNS,
                                            OVERLAY_FOOTPRINT_HEIGHT
                                        );
                                        DrawOverlay(
                                            &m_types[gSelectedOverlay],
                                            cellX,
                                            cellY,
                                            OVERLAY_FOOTPRINT_COLUMNS,
                                            OVERLAY_FOOTPRINT_HEIGHT,
                                            true
                                        );
                                        m_previewDrawn = true;
                                    }
                                    gEditManager->UpdateMapView();
                                    gEditManager->UpdateCursor();
                                } else {
                                    gEditManager->m_cursorX = cellX;
                                    gEditManager->m_cursorY = cellY;
                                }
                                gEditManager->UpdateCursor();
                            }
                            break;
                    }
                    return MESSAGE_DISPATCH_CONSUME;
            }
            break;
    }
    if (finished == true) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}

void overlayManager::DrawFootprint(
    i16 x,
    i16 y,
    i16 footprintMask,
    i16 overlayMask,
    i16 width,
    i16 height
) {
    i16 cellSize;
    i16 frame;
    i16 cx;
    i16 cy;

    frame = !gEditManager->m_zoomedOut;
    cellSize = gEditManager->m_zoomedOut ? EDIT_VIEW_ZOOMED_CELL_PIXELS
                                                                   : EDIT_VIEW_CELL_PIXELS;
    for (cy = 0; cy < OVERLAY_FOOTPRINT_ROWS; cy++)
        for (cx = 0; cx < OVERLAY_FOOTPRINT_COLUMNS; cx++)
            if (cy < height && cx < width && (footprintMask & OVERLAY_FOOTPRINT_BIT(cx, cy)))
                m_icon->FillToBuffer(
                    x + cx * cellSize,
                    y - cy * cellSize,
                    frame,
                    (overlayMask & OVERLAY_FOOTPRINT_BIT(cx, cy)) ? OVERLAY_FOOTPRINT_OVERLAY_COLOR
                                                                  : OVERLAY_FOOTPRINT_GROUND_COLOR,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
    if ((footprintMask & OVERLAY_FOOTPRINT_CORNER_BIT) && height >= OVERLAY_FOOTPRINT_CORNER_ROW + 1
        && width >= OVERLAY_FOOTPRINT_CORNER_COLUMN + 1)
        m_icon->FillToBuffer(
            x + cellSize * OVERLAY_FOOTPRINT_CORNER_COLUMN,
            y - cellSize * OVERLAY_FOOTPRINT_CORNER_ROW,
            frame,
            (overlayMask & OVERLAY_FOOTPRINT_CORNER_BIT) ? OVERLAY_FOOTPRINT_OVERLAY_COLOR
                                                         : OVERLAY_FOOTPRINT_GROUND_COLOR,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
}

i16 CanPlaceOverlay(overlayType* type, i16 x, i16 y) {
    i16 cx;
    i16 cy;

    for (cy = 0; cy < OVERLAY_FOOTPRINT_ROWS; cy++)
        for (cx = 0; cx < OVERLAY_FOOTPRINT_COLUMNS; cx++) {
            if (type->overlayMask & OVERLAY_FOOTPRINT_BIT(cx, cy)) {
                if (x + cx < 0 || x + cx > MAP_CELL_GRID_SIZE - 1 || y - cy < 0
                    || y - cy > MAP_CELL_GRID_SIZE - 1
                    || gEditManager->m_map.cells[x + cx][y - cy].m_overlayIndex
                           != MAP_CELL_NO_FRAME)
                    return 0;
            }
            if (!(type->overlayMask & OVERLAY_FOOTPRINT_BIT(cx, cy))
                && (type->footprintMask & OVERLAY_FOOTPRINT_BIT(cx, cy))) {
                if (x + cx < 0 || x + cx > MAP_CELL_GRID_SIZE - 1 || y - cy < 0
                    || y - cy > MAP_CELL_GRID_SIZE - 1
                    || gEditManager->m_map.cells[x + cx][y - cy].m_objectIndex != MAP_CELL_NO_FRAME
                    || !(
                        type->terrainMask
                        & OVERLAY_TERRAIN_BIT(
                            gEditManager->m_map.cells[x + cx][y - cy].m_tileIndex
                            / MAP_CELL_TILES_PER_TERRAIN
                        )
                    ))
                    return 0;
            }
        }
    if (type->overlayMask & OVERLAY_FOOTPRINT_CORNER_BIT) {
        if (x + OVERLAY_FOOTPRINT_CORNER_COLUMN < 0
            || x + OVERLAY_FOOTPRINT_CORNER_COLUMN > MAP_CELL_GRID_SIZE - 1
            || y - OVERLAY_FOOTPRINT_CORNER_ROW < 0
            || y - OVERLAY_FOOTPRINT_CORNER_ROW > MAP_CELL_GRID_SIZE - 1
            || gEditManager->m_map
                       .cells[x + OVERLAY_FOOTPRINT_CORNER_COLUMN][y - OVERLAY_FOOTPRINT_CORNER_ROW]
                       .m_overlayIndex
                   != MAP_CELL_NO_FRAME)
            return 0;
    }
    if (!(type->overlayMask & OVERLAY_FOOTPRINT_CORNER_BIT)
        && (type->footprintMask & OVERLAY_FOOTPRINT_CORNER_BIT)) {
        if (x + OVERLAY_FOOTPRINT_CORNER_COLUMN < 0
            || x + OVERLAY_FOOTPRINT_CORNER_COLUMN > MAP_CELL_GRID_SIZE - 1
            || y - OVERLAY_FOOTPRINT_CORNER_ROW < 0
            || y - OVERLAY_FOOTPRINT_CORNER_ROW > MAP_CELL_GRID_SIZE - 1
            || gEditManager->m_map
                       .cells[x + OVERLAY_FOOTPRINT_CORNER_COLUMN][y - OVERLAY_FOOTPRINT_CORNER_ROW]
                       .m_objectIndex
                   != MAP_CELL_NO_FRAME
            || !(
                type->terrainMask
                & OVERLAY_TERRAIN_BIT(
                    gEditManager->m_map
                        .cells[x + OVERLAY_FOOTPRINT_CORNER_COLUMN]
                              [y - OVERLAY_FOOTPRINT_CORNER_ROW]
                        .m_tileIndex
                    / MAP_CELL_TILES_PER_TERRAIN
                )
            ))
            return 0;
    }
    return 1;
}

b32 PlaceOverlay(overlayType* type, i16 x, i16 y) {
    i16 col;
    i32 footBit;
    editMapCellPair* placedIds;
    i16 row;
    i16 piece;
    mapCell* dest;
    editTownExtra* newTown;
    editHeroExtra* newHero;
    i32 i;

    if (!CanPlaceOverlay(type, x, y)) {
        ShowStatusWarning(localization::Tr("editor.overlay.place.unsuitable"));
        return false;
    }
    if (type->tileset == TILESET_TOWN32 && gEditManager->CountTowns() >= GAME_TOWN_COUNT) {
        sprintf(gText, localization::Tr("editor.overlay.limit.towns"), GAME_TOWN_COUNT);
        ShowStatusWarning(gText);
        return false;
    }
    if ((!strcmp(type->name, "sawmill ") || !strcmp(type->name, "alch-00 ")
         || !strcmp(type->name, "alch-01 ") || !strcmp(type->name, "alch-02 ")
         || !strcmp(type->name, "mine-00 ") || !strcmp(type->name, "mine-01 ")
         || !strcmp(type->name, "mine-02 ") || !strcmp(type->name, "mine-03 ")
         || !strcmp(type->name, "mine-04 ") || !strnicmp(type->name, "nmine", 5))
        && gEditManager->CountMines() >= OVERLAY_MINE_LIMIT) {
        sprintf(gText, localization::Tr("editor.overlay.limit.mines"), OVERLAY_MINE_LIMIT);
        ShowStatusWarning(gText);
        return false;
    }
    if (!strcmp(type->name, "litehous")
        && gEditManager->HasObject(MAP_EVENT_TRIGGER(MAP_OBJECT_LIGHTHOUSE)) >= 1) {
        sprintf(gText, localization::Tr("editor.overlay.limit.lighthouse"));
        ShowStatusWarning(gText);
        return false;
    }
    if (!strcmp(type->name, "bigkeep ")
        && gEditManager->HasObject(MAP_EVENT_TRIGGER(MAP_OBJECT_DRAGON_CITY)) >= 1) {
        sprintf(gText, localization::Tr("editor.overlay.limit.dragon_city"));
        ShowStatusWarning(gText);
        return false;
    }
    if (type->tileset == TILESET_ART32 && gEditManager->CountArtifacts() > OVERLAY_ARTIFACT_LIMIT) {
        sprintf(gText, localization::Tr("editor.overlay.limit.artifacts"));
        ShowStatusWarning(gText);
        return false;
    }
    gNextObjectId++;
    for (piece = 0; piece < OVERLAY_FOOTPRINT_CELLS; piece++) {
        if (piece == OVERLAY_FOOTPRINT_CORNER) {
            col = OVERLAY_FOOTPRINT_CORNER_COLUMN;
            row = OVERLAY_FOOTPRINT_CORNER_ROW;
        } else {
            row = piece / OVERLAY_FOOTPRINT_COLUMNS;
            col = piece - row * OVERLAY_FOOTPRINT_COLUMNS;
        }
        dest = &gEditManager->m_map.cells[x + col][y - row];
        placedIds = &gEditManager->m_map.cellPairs[x + col][y - row];
        footBit = OVERLAY_CELL_BIT(piece);
        if (type->frames[piece] != MAP_CELL_NO_FRAME) {
            if (type->overlayMask & footBit) {
                placedIds->overlayId = gNextObjectId;
                dest->m_overlayIndex = type->frames[piece];
                dest->m_overlayTileset = type->tileset;
                if (type->animatedMask & footBit)
                    dest->m_flags |= MAP_CELL_OVERLAY_ANIMATED;
            } else {
                placedIds->objectId = gNextObjectId;
                dest->m_objectIndex = type->frames[piece];
                dest->m_objectTileset = type->tileset;
                if (type->animatedMask & footBit)
                    dest->m_flags |= MAP_CELL_OBJECT_ANIMATED;
            }
            if (type->blockMask & footBit) {
                dest->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
                H1_ASSERT(!(type->overlayMask & footBit));
            }
            if (type->footprintMask & footBit) {
                if (dest->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                    if (type->overlayMask & footBit)
                        dest->m_secondaryTrigger |= type->trigger;
                    else {
                        dest->m_secondaryTrigger |= dest->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                        dest->m_triggerType &= MAP_TRIGGER_EVENT;
                        dest->m_triggerType |= type->trigger;
                    }
                } else
                    dest->m_triggerType = type->trigger;
            }
            if (type->eventMask & footBit)
                dest->m_triggerType |= MAP_TRIGGER_EVENT;
            if (dest->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
                || dest->m_triggerType == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_TOWN)
                || dest->m_triggerType == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE)) {
                newTown = new editTownExtra;
                memset(newTown, 0, sizeof(editTownExtra));
                dest->m_objectMetadata = gEditManager->m_extraCount;
                gEditManager->m_extras[gEditManager->m_extraCount] = newTown;
                gEditManager->m_extraSizes[gEditManager->m_extraCount] = sizeof(editTownExtra);
                gEditManager->m_extraCount++;
            }
            if (dest->m_triggerType == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_HERO)) {
                newHero = new editHeroExtra;
                memset(newHero, 0, sizeof(editHeroExtra));
                newHero->record.artifacts[0] = ARTIFACT_NONE;
                newHero->record.artifacts[1] = ARTIFACT_NONE;
                newHero->record.artifacts[2] = ARTIFACT_NONE;
                newHero->record.artifacts[3] = ARTIFACT_NONE;
                dest->m_objectMetadata = gEditManager->m_extraCount;
                gEditManager->m_extras[gEditManager->m_extraCount] = newHero;
                gEditManager->m_extraSizes[gEditManager->m_extraCount] = sizeof(editHeroExtra);
                gEditManager->m_extraCount++;
            }
            if (type->resourceMask & footBit) {
                if (type->flags == OVERLAY_TYPE_SHOWS_RESOURCE) {
                    for (i = 0; i < OVERLAY_TYPE_COUNT; i++)
                        if ((gOverlayTypes[i].flags & OVERLAY_TYPE_RESOURCE_MARKER)
                            && gOverlayTypes[i].frames[OVERLAY_FOOTPRINT_ANCHOR]
                                   == type->resourceFrame) {
                            PlaceMineResource(&gOverlayTypes[i], x + col, y - row, false);
                            i = EDIT_END_SCAN;
                        }
                } else {
                    for (i = 0; i < OVERLAY_TYPE_COUNT; i++)
                        if (gOverlayTypes[i].flags & OVERLAY_TYPE_RESOURCE_MARKER) {
                            PlaceMineResource(&gOverlayTypes[i], x + col, y - row, false);
                            i = EDIT_END_SCAN;
                        }
                }
            }
        }
    }
    return true;
}

b32 PlaceMineResource(overlayType* type, i16 x, i16 y, b32 checkMine) {
    b32 fits;
    mapCell* target;

    fits = true;
    if (checkMine && (type->flags & OVERLAY_TYPE_RESOURCE_MARKER)
        && (gEditManager->m_map.cells[x][y].m_triggerType != MAP_OBJECT_TRIGGER(MAP_OBJECT_MINE)
            || x < 1
            || gEditManager->m_map.cells[x - 1][y].m_triggerType
                   != MAP_EVENT_TRIGGER(MAP_OBJECT_MINE)))
        fits = false;
    if (!fits) {
        ShowStatusWarning(localization::Tr("editor.overlay.resource.unsuitable"));
        return false;
    }
    target = &gEditManager->m_map.cells[x][y];
    target->m_flags |= MAP_CELL_OBJECT_EXTRA;
    target->m_objectTileset |= TILESET_RSRC32
                               << MAP_CELL_EXTRA_TILESET_SHIFT;
    target->m_extraFrame = type->frames[OVERLAY_FOOTPRINT_ANCHOR];
    return true;
}

void overlayManager::DrawOverlay(
    overlayType* type,
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    b32 update
) {
    i16 cellSize;
    i16 cx;
    i16 cy;

    cellSize = gEditManager->m_zoomedOut ? EDIT_VIEW_ZOOMED_CELL_PIXELS
                                                                   : EDIT_VIEW_CELL_PIXELS;
    for (cy = 0; cy < OVERLAY_FOOTPRINT_ROWS; cy++)
        for (cx = 0; cx < OVERLAY_FOOTPRINT_COLUMNS; cx++)
            if (cy < height && cx < width
                && type->frames[OVERLAY_FOOTPRINT_CELL(cx, cy)] != MAP_CELL_NO_FRAME) {
                gEditManager->m_objectIcons[type->tileset][gEditManager->m_zoomedOut]->DrawToBuffer(
                    x + cx * cellSize,
                    y - cy * cellSize,
                    type->frames[OVERLAY_FOOTPRINT_CELL(cx, cy)],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                if (type->animatedMask & OVERLAY_FOOTPRINT_BIT(cx, cy))
                    gEditManager->m_objectIcons[type->tileset][gEditManager->m_zoomedOut]
                        ->DrawToBuffer(
                            x + cx * cellSize,
                            y - cy * cellSize,
                            type->frames[OVERLAY_FOOTPRINT_CELL(cx, cy)]
                                + gEditManager->m_animationFrame + 1,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                if ((type->flags & OVERLAY_TYPE_SHOWS_RESOURCE)
                    && cx == OVERLAY_SHOWN_RESOURCE_COLUMN && cy == OVERLAY_SHOWN_RESOURCE_ROW)
                    gEditManager->m_objectIcons[type->tileset][gEditManager->m_zoomedOut]
                        ->DrawToBuffer(
                            x + cx * cellSize,
                            y - cy * cellSize,
                            type->resourceFrame,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                if (update)
                    gWindowManager->UpdateScreenRegion(
                        OVERLAY_PREVIEW_X,
                        OVERLAY_PREVIEW_UPDATE_Y,
                        OVERLAY_PREVIEW_WIDTH,
                        OVERLAY_PREVIEW_HEIGHT
                    );
            }
    if (type->frames[OVERLAY_FOOTPRINT_CORNER] != MAP_CELL_NO_FRAME
        && height >= OVERLAY_FOOTPRINT_CORNER_ROW + 1
        && width >= OVERLAY_FOOTPRINT_CORNER_COLUMN + 1) {
        gEditManager->m_objectIcons[type->tileset][gEditManager->m_zoomedOut]->DrawToBuffer(
            x + cellSize * OVERLAY_FOOTPRINT_CORNER_COLUMN,
            y - cellSize * OVERLAY_FOOTPRINT_CORNER_ROW,
            type->frames[OVERLAY_FOOTPRINT_CORNER],
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        if (type->animatedMask & OVERLAY_FOOTPRINT_CORNER_BIT)
            gEditManager->m_objectIcons[type->tileset][gEditManager->m_zoomedOut]->DrawToBuffer(
                x + cellSize * OVERLAY_FOOTPRINT_CORNER_COLUMN,
                y - cellSize * OVERLAY_FOOTPRINT_CORNER_ROW,
                type->frames[OVERLAY_FOOTPRINT_CORNER] + gEditManager->m_animationFrame + 1,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        if (update)
            gWindowManager->UpdateScreenRegion(
                OVERLAY_PREVIEW_X,
                OVERLAY_PREVIEW_UPDATE_Y,
                OVERLAY_PREVIEW_WIDTH,
                OVERLAY_PREVIEW_HEIGHT
            );
    }
}

i16 overlayManager::LoadCategory(i16 category) {
    i32 unusedCount;
    i32 unusedFlag;
    i32 terrainBits;
    i16 i;

    m_typeCount = 0;
    if (gOverlayCategoryKinds[category] == OVERLAY_KIND_TERRAIN)
        terrainBits = OVERLAY_TERRAIN_BIT(category);
    else
        terrainBits = OVERLAY_TERRAIN_MASK_ANY;
    unusedCount = 0;
    for (i = 0; i < OVERLAY_TYPE_COUNT; i++)
        if (gOverlayTypes[i].kind == gOverlayCategoryKinds[category]
            && (gOverlayTypes[i].terrainMask & terrainBits)
            && strnicmp(gOverlayTypes[i].name, "mine-", 5)
            && strnicmp(gOverlayTypes[i].name, "rovr-", 5))
            m_types[m_typeCount++] = gOverlayTypes[i];
    if (m_typeCount)
        return 1;
    return 0;
}

i16 overlayManager::PickOverlay(i16 category) {
    i16 selected;
    i16 needDraw;
    i32 unusedSlot;
    i32 perPage;
    i16 first;
    i16 slot;
    i16 done;
    i16 x;
    i32 unusedIndex;
    heroWindow* screenWindow;
    tag_message message;
    i16 y;
    i32 perLine;
    i32 rowsPerPage;
    i16 previousZoom;

    slot = 0;
    previousZoom = gEditManager->m_zoomedOut;
    done = 0;
    first = 0;
    selected = 0;
    needDraw = 1;
    unusedSlot = 0;
    unusedIndex = 0;
    perLine = OVERLAY_PICKER_COLUMNS;
    rowsPerPage = OVERLAY_PICKER_ROWS;
    perPage = OVERLAY_PICKER_PAGE;
    screenWindow = new heroWindow(
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        WINDOW_FLAG_SAVE_BACKGROUND
    );
    gWindowManager->AddWindow(screenWindow, WINDOW_Z_ORDER_APPEND, 1);
    gEditManager->m_zoomedOut = EDIT_ZOOM_OUT;
    LoadCategory(category);
    gInputManager->Flush();
    while (!done) {
        Process1WindowsMessage();
        if (needDraw) {
            FillBitmapArea(
                gWindowManager->m_screen,
                0,
                0,
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT,
                0
            );
            for (x = 0; x < OVERLAY_PICKER_COLUMNS; x++)
                for (y = 0; y < OVERLAY_PICKER_ROWS; y++)
                    m_icon->DrawToBuffer(
                        x * OVERLAY_PICKER_CELL_WIDTH,
                        y * OVERLAY_PICKER_CELL_HEIGHT,
                        OVERLAY_PICKER_CELL_FRAME,
                        ICON_DRAW_NORMAL,
                        ICON_DRAW_OFFSET_FULL
                    );
            slot = 0;
            while (first + slot < m_typeCount && slot < OVERLAY_PICKER_PAGE) {
                overlayType* type = &m_types[first + slot];
                x = slot % OVERLAY_PICKER_COLUMNS * OVERLAY_PICKER_CELL_WIDTH
                    + OVERLAY_PICKER_OBJECT_X;
                y = slot / OVERLAY_PICKER_COLUMNS * OVERLAY_PICKER_CELL_HEIGHT
                    + OVERLAY_PICKER_OBJECT_Y;
                DrawFootprint(
                    x,
                    y,
                    type->footprintMask,
                    type->overlayMask,
                    OVERLAY_PICKER_FOOTPRINT_COLUMNS,
                    OVERLAY_PICKER_FOOTPRINT_ROWS
                );
                DrawOverlay(
                    type,
                    x,
                    y,
                    OVERLAY_PICKER_FOOTPRINT_COLUMNS,
                    OVERLAY_PICKER_FOOTPRINT_ROWS,
                    false
                );
                slot++;
            }
            gWindowManager->UpdateScreen();
            needDraw = 0;
        }
        message = gInputManager->GetEvent();
        gMouseManager->Main(message);
        switch (message.type) {
            case MESSAGE_KEY_DOWN:
                switch (message.keyCode) {
                    case INPUT_SCAN_NUMPAD_8:
                        first -= OVERLAY_PICKER_COLUMNS;
                        if (first < 0)
                            first = 0;
                        needDraw = 1;
                        break;
                    case INPUT_SCAN_NUMPAD_2:
                        if (m_typeCount < OVERLAY_PICKER_PAGE)
                            break;
                        if (first + OVERLAY_PICKER_PAGE < m_typeCount)
                            first += OVERLAY_PICKER_COLUMNS;
                        needDraw = 1;
                        break;
                    case INPUT_SCAN_NUMPAD_9:
                        first -= OVERLAY_PICKER_PAGE;
                        if (first < 0)
                            first = 0;
                        needDraw = 1;
                        break;
                    case INPUT_SCAN_NUMPAD_3:
                        if (m_typeCount < OVERLAY_PICKER_PAGE)
                            break;
                        first += OVERLAY_PICKER_PAGE;
                        if (first + OVERLAY_PICKER_PAGE >= m_typeCount)
                            first =
                                m_typeCount
                                + (OVERLAY_PICKER_COLUMNS - m_typeCount % OVERLAY_PICKER_COLUMNS)
                                - OVERLAY_PICKER_PAGE;
                        needDraw = 1;
                        break;
                    case INPUT_SCAN_ESCAPE:
                        selected = OVERLAY_NO_SELECTION;
                        done = 1;
                        break;
                    case INPUT_SCAN_NUMPAD_4:
                        gOverlayShownCategory = (gOverlayShownCategory + OVERLAY_CATEGORY_COUNT - 1)
                                                % OVERLAY_CATEGORY_COUNT;
                        goto showCategory;
                    case INPUT_SCAN_TAB:
                    case INPUT_SCAN_NUMPAD_6:
                        gOverlayShownCategory = (gOverlayShownCategory + OVERLAY_CATEGORY_COUNT + 1)
                                                % OVERLAY_CATEGORY_COUNT;
                    showCategory:
                        first = 0;
                        gOverlayCategory = gOverlayShownCategory;
                        gSelectedOverlay = 0;
                        needDraw = 1;
                        LoadCategory(gOverlayShownCategory);
                        break;
                }
                break;
            case MESSAGE_LEFT_BUTTON_UP:
                x = message.x / OVERLAY_PICKER_CELL_WIDTH;
                y = message.y / OVERLAY_PICKER_CELL_HEIGHT;
                if (x >= OVERLAY_PICKER_COLUMNS || y >= OVERLAY_PICKER_ROWS)
                    break;
                if (x < 0 || y < 0)
                    break;
                selected = first + y * OVERLAY_PICKER_COLUMNS + x;
                if (selected < m_typeCount)
                    done = 1;
                break;
        }
    }
    gWindowManager->RemoveWindow(screenWindow);
    delete screenWindow;
    gInputManager->Flush();
    gEditManager->m_zoomedOut = previousZoom;
    return selected;
}

void overlayManager::MeasureOverlay(overlayType* type) {
    if (type->footprintMask & OVERLAY_FOOTPRINT_CORNER_BIT)
        m_height = 4;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_ROW_2_MASK)
        m_height = 3;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_ROW_1_MASK)
        m_height = 2;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_ROW_0_MASK)
        m_height = 1;
    if (type->footprintMask & OVERLAY_FOOTPRINT_COLUMN_4_MASK)
        m_width = 5;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_COLUMN_3_MASK)
        m_width = 4;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_COLUMN_2_MASK)
        m_width = 3;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_COLUMN_1_MASK)
        m_width = 2;
    else if (type->footprintMask & OVERLAY_FOOTPRINT_COLUMN_0_MASK)
        m_width = 1;
}

void overlayManager::DrawCategoryName(b32) {
    FillBitmapArea(
        gWindowManager->m_screen,
        OVERLAY_CATEGORY_NAME_X,
        OVERLAY_CATEGORY_NAME_Y,
        OVERLAY_CATEGORY_NAME_WIDTH,
        OVERLAY_CATEGORY_NAME_HEIGHT,
        0
    );
    m_font->DrawString(
        gOverlayCategoryNames[gOverlayShownCategory],
        OVERLAY_CATEGORY_TEXT_X,
        OVERLAY_CATEGORY_TEXT_Y,
        1
    );
    gWindowManager->UpdateScreenRegion(
        OVERLAY_CATEGORY_NAME_X,
        OVERLAY_CATEGORY_NAME_Y,
        OVERLAY_CATEGORY_NAME_WIDTH,
        OVERLAY_CATEGORY_NAME_HEIGHT
    );
}

void overlayManager::DrawSelectedOverlay(void) {
    i16 previousZoom;

    if (gSelectedOverlay != OVERLAY_NO_SELECTION) {
        MeasureOverlay(&m_types[gSelectedOverlay]);
        previousZoom = gEditManager->m_zoomedOut;
        gEditManager->m_zoomedOut = EDIT_ZOOM_OUT;
        m_icon->DrawToBuffer(
            OVERLAY_PREVIEW_X,
            OVERLAY_PREVIEW_Y,
            OVERLAY_PREVIEW_FRAME,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        DrawFootprint(
            OVERLAY_PREVIEW_OBJECT_X,
            OVERLAY_PREVIEW_OBJECT_Y,
            m_types[gSelectedOverlay].footprintMask,
            m_types[gSelectedOverlay].overlayMask,
            OVERLAY_FOOTPRINT_COLUMNS,
            OVERLAY_FOOTPRINT_HEIGHT
        );
        DrawOverlay(
            &m_types[gSelectedOverlay],
            OVERLAY_PREVIEW_OBJECT_X,
            OVERLAY_PREVIEW_OBJECT_Y,
            OVERLAY_FOOTPRINT_COLUMNS,
            OVERLAY_FOOTPRINT_HEIGHT,
            true
        );
        gWindowManager->UpdateScreenRegion(
            OVERLAY_PREVIEW_X,
            OVERLAY_PREVIEW_Y,
            OVERLAY_PREVIEW_WIDTH,
            OVERLAY_PREVIEW_HEIGHT
        );
        gEditManager->m_zoomedOut = previousZoom;
    } else {
        m_icon->DrawToBuffer(
            OVERLAY_PREVIEW_X,
            OVERLAY_PREVIEW_Y,
            OVERLAY_PREVIEW_FRAME,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        gWindowManager->UpdateScreenRegion(
            OVERLAY_PREVIEW_X,
            OVERLAY_PREVIEW_Y,
            OVERLAY_PREVIEW_WIDTH,
            OVERLAY_PREVIEW_HEIGHT
        );
    }
}
