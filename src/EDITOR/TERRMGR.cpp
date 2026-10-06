// The terrain painting tool (terrainManager). The unit name TERRMGR is
// descriptive: no retail assertion names it; Open stores the class name
// "terrainManager".

#include <match.h>

#include <BASE/backdropWidget.h>
#include <BASE/heroWindow.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <EDITOR/editManager.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/terrainManager.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>

#include <stdlib.h>
#include <string.h>

DATA(0x00452f1c)
H1_ENUM_STORAGE(TerrainType, i32) gLastTerrain;

VA(0x004184a0, 0x41)
terrainManager::terrainManager(void)
    : m_terrain(TERRAIN_WATER), m_lastX(EDIT_NO_CELL), m_lastY(EDIT_NO_CELL) {
    m_dispatchMask = EDIT_MANAGER_DISPATCH_MASK;
}

VA(0x004184e1, 0x52c)
H1_ENUM_RETURN(BaseManagerStatus, i16) terrainManager::Open(i16 priority) {
    H1_ENUM_LOCAL(TerrainType, i16) i;

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
    m_terrainButtons[TERRAIN_WATER] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_WATER),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_WATER,
        ICON_WIDGET_DRAW,
        1
    );
    m_terrainButtons[TERRAIN_GRASS] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, TERRAIN_GRASS) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_GRASS),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_GRASS,
        ICON_WIDGET_DRAW,
        1
    );
    m_terrainButtons[TERRAIN_SNOW] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, TERRAIN_SNOW) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_SNOW),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_SNOW,
        ICON_WIDGET_DRAW,
        1
    );
    m_terrainButtons[TERRAIN_SWAMP] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, TERRAIN_SWAMP) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_SWAMP),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_SWAMP,
        ICON_WIDGET_DRAW,
        1
    );
    m_terrainButtons[TERRAIN_LAVA] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, TERRAIN_LAVA) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_LAVA),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_LAVA,
        ICON_WIDGET_DRAW,
        1
    );
    m_terrainButtons[TERRAIN_DESERT] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y
            + H1_ENUM_ENCODE(TerrainType, TERRAIN_DESERT) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_DESERT),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_DESERT,
        ICON_WIDGET_DRAW,
        1
    );
    m_terrainButtons[TERRAIN_DIRT] = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, TERRAIN_DIRT) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        H1_ENUM_ENCODE(TerrainType, TERRAIN_DIRT),
        ICON_DRAW_NORMAL,
        TERRAIN_BUTTON_DIRT,
        ICON_WIDGET_DRAW,
        1
    );
    for (i = TERRAIN_WATER; i < TERRAIN_COUNT; i++)
        gEditManager->m_window->AddWidget(m_terrainButtons[i], WINDOW_Z_ORDER_APPEND);
    m_highlight = new iconWidget(
        TERRAIN_BUTTON_X,
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, m_terrain) * TERRAIN_BUTTON_HEIGHT,
        TERRAIN_BUTTON_WIDTH,
        TERRAIN_BUTTON_HEIGHT,
        "terrains.icn",
        TERRAIN_HIGHLIGHT_FRAME,
        ICON_DRAW_NORMAL,
        TERRAIN_HIGHLIGHT_WIDGET,
        ICON_WIDGET_FILL,
        TERRAIN_HIGHLIGHT_FILL_COLOR
    );
    gEditManager->m_window->AddWidget(m_highlight, WINDOW_Z_ORDER_APPEND);
    m_backdrop = new backdropWidget(
        EDIT_TOOL_PANEL_X,
        TERRAIN_BACKDROP_Y,
        EDIT_TOOL_PANEL_WIDTH,
        EDIT_TOOL_PANEL_HEIGHT,
        WIDGET_ID_NONE,
        TERRAIN_BACKDROP_KIND
    );
    gEditManager->m_window->AddWidget(m_backdrop, WINDOW_Z_ORDER_APPEND);
    gEditManager->m_placedY = EDIT_NO_CELL;
    gEditManager->m_placedX = EDIT_NO_CELL;
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "terrainManager");
    SelectTerrain(gLastTerrain);
    return BASE_MANAGER_SUCCESS;
}

VA(0x00418a0d, 0x166)
void terrainManager::Close(void) {
    H1_ENUM_LOCAL(TerrainType, i16) i;

    for (i = TERRAIN_WATER; i < TERRAIN_COUNT; i++) {
        gEditManager->m_window->RemoveWidget(m_terrainButtons[i]);
        delete m_terrainButtons[i];
    }
    gEditManager->m_window->RemoveWidget(m_highlight);
    delete m_highlight;
    gEditManager->m_window->DrawWindow();
    gEditManager->m_window->RemoveWidget(m_panel);
    delete m_panel;
    gEditManager->m_window->RemoveWidget(m_backdrop);
    delete m_backdrop;
    m_active = 0;
}

VA(0x00418b73, 0x741)
H1_ENUM_RETURN(MessageDispatchResult, i16) terrainManager::Main(tag_message& message) {
    i16 newX;
    i16 newY;
    i32 tilesHigh;
    i32 tilesWide;
    i16 anchorX;
    tag_message event;
    H1_ENUM_LOCAL(TerrainType, i16) ground;
    H1_ENUM_LOCAL(TerrainDragMode, i16) dragMode;
    i16 y;
    i16 x;
    i16 anchorY;
    H1_ENUM_LOCAL(TerrainToolHelp, i32) rightClickHelp;

    if (!(message.type & m_dispatchMask))
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                        rightClickHelp = TERRAIN_TOOL_HELP_NONE;
                        switch (message.id) {
                            case TERRAIN_BUTTON_WATER:
                                rightClickHelp = TERRAIN_TOOL_HELP_WATER;
                                break;
                            case TERRAIN_BUTTON_GRASS:
                                rightClickHelp = TERRAIN_TOOL_HELP_GRASS;
                                break;
                            case TERRAIN_BUTTON_SNOW:
                                rightClickHelp = TERRAIN_TOOL_HELP_SNOW;
                                break;
                            case TERRAIN_BUTTON_SWAMP:
                                rightClickHelp = TERRAIN_TOOL_HELP_SWAMP;
                                break;
                            case TERRAIN_BUTTON_LAVA:
                                rightClickHelp = TERRAIN_TOOL_HELP_LAVA;
                                break;
                            case TERRAIN_BUTTON_DESERT:
                                rightClickHelp = TERRAIN_TOOL_HELP_DESERT;
                                break;
                            case TERRAIN_BUTTON_DIRT:
                                rightClickHelp = TERRAIN_TOOL_HELP_DIRT;
                                break;
                        }
                        if (H1_ENUM_ENCODE(TerrainToolHelp, rightClickHelp) >= 0)
                            NormalDialog(
                                gTerrainToolHelp[rightClickHelp],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW
                            );
                        break;
                    }
                    switch (message.id) {
                        case TERRAIN_BUTTON_WATER:
                            SelectTerrain(TERRAIN_WATER);
                            break;
                        case TERRAIN_BUTTON_GRASS:
                            SelectTerrain(TERRAIN_GRASS);
                            break;
                        case TERRAIN_BUTTON_SNOW:
                            SelectTerrain(TERRAIN_SNOW);
                            break;
                        case TERRAIN_BUTTON_SWAMP:
                            SelectTerrain(TERRAIN_SWAMP);
                            break;
                        case TERRAIN_BUTTON_LAVA:
                            SelectTerrain(TERRAIN_LAVA);
                            break;
                        case TERRAIN_BUTTON_DESERT:
                            SelectTerrain(TERRAIN_DESERT);
                            break;
                        case TERRAIN_BUTTON_DIRT:
                            SelectTerrain(TERRAIN_DIRT);
                            break;
                        case EDIT_CONTROL_MAP:
                            dragMode = TERRAIN_DRAG_CELLS;
                            if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                                ground = TERRAIN_WATER;
                            else
                                ground = m_terrain;
                            gSelectionX = EDIT_NO_CELL;
                            if (message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS)
                                dragMode = TERRAIN_DRAG_CELLS;
                            else if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                                dragMode = TERRAIN_DRAG_BRUSH;
                            else
                                dragMode = TERRAIN_DRAG_RECTANGLE;
                            gMouseManager->MouseCoords(anchorX, anchorY);
                            gEditManager->ScreenToCell(anchorX, anchorY);
                            anchorX += gEditManager->m_viewX;
                            anchorY += gEditManager->m_viewY;
                            gEditManager->SaveUndo();
                            event = gInputManager->GetEvent();
                            while (!IS_BUTTON_RELEASE_MESSAGE(event.type)) {
                                Process1WindowsMessage();
                                gMouseManager->Main(event);
                                if (event.type == MESSAGE_MOUSE_MOVE) {
                                    gMouseManager->MouseCoords(x, y);
                                    gEditManager->ScreenToCell(x, y);
                                    x += gEditManager->m_viewX;
                                    y += gEditManager->m_viewY;
                                    if (x != m_lastX || y != m_lastY) {
                                        m_lastX = x;
                                        m_lastY = y;
                                        switch (dragMode) {
                                            case TERRAIN_DRAG_CELLS:
                                                gEditManager->PaintGround(
                                                    x - gEditManager->m_viewX,
                                                    y - gEditManager->m_viewY,
                                                    1,
                                                    1,
                                                    H1_ENUM_ENCODE(TerrainType, ground)
                                                );
                                                break;
                                            case TERRAIN_DRAG_BRUSH:
                                                tilesWide = x < MAP_CELL_GRID_SIZE - 1
                                                                ? TERRAIN_BRUSH_SIZE
                                                                : 1;
                                                tilesHigh = y < MAP_CELL_GRID_SIZE - 1
                                                                ? TERRAIN_BRUSH_SIZE
                                                                : 1;
                                                gEditManager->PaintGround(
                                                    x - gEditManager->m_viewX,
                                                    y - gEditManager->m_viewY,
                                                    tilesWide,
                                                    tilesHigh,
                                                    H1_ENUM_ENCODE(TerrainType, ground)
                                                );
                                                break;
                                            case TERRAIN_DRAG_RECTANGLE:
                                                gSelectionX = x < anchorX ? x : anchorX;
                                                gSelectionY = y < anchorY ? y : anchorY;
                                                gSelectionWidth = abs(x - anchorX) + 1;
                                                gSelectionHeight = abs(y - anchorY) + 1;
                                                gEditManager->DrawMap();
                                                gEditManager->UpdateMapView();
                                                gEditManager->DrawRadar(1);
                                                break;
                                        }
                                    }
                                }
                                event = gInputManager->GetEvent();
                            }
                            if (dragMode == TERRAIN_DRAG_RECTANGLE && gSelectionX >= 0)
                                gEditManager->FillGround(
                                    gSelectionX,
                                    gSelectionY,
                                    gSelectionWidth,
                                    gSelectionHeight,
                                    H1_ENUM_ENCODE(TerrainType, ground)
                                );
                            gSelectionX = gSelectionY = EDIT_NO_CELL;
                            gEditManager->BlendTerrain(m_terrain, 0, 1, 0, 0);
                            gEditManager->DrawMap();
                            gEditManager->UpdateMapView();
                            gEditManager->DrawRadar(1);
                            m_lastY = EDIT_NO_CELL;
                            m_lastX = EDIT_NO_CELL;
                            gEditManager->m_mapChanged = 1;
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    if (message.id != EDIT_CONTROL_MAP && message.id == gEditManager->m_lastHoverId)
                        return MESSAGE_DISPATCH_CONSUME;
                    gEditManager->m_lastHoverId = message.id;
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            gMouseManager->MouseCoords(newX, newY);
                            gEditManager->ScreenToCell(newX, newY);
                            if (gEditManager->m_cursorX != newX
                                || gEditManager->m_cursorY != newY) {
                                gEditManager->m_cursorX = newX;
                                gEditManager->m_cursorY = newY;
                                gEditManager->UpdateCursor();
                            }
                            break;
                    }
                    return MESSAGE_DISPATCH_CONSUME;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_R:
                    RandomizeTiles();
                    gEditManager->m_mapChanged = 1;
                    gEditManager->DrawMap();
                    gEditManager->UpdateMapView();
                    gEditManager->DrawRadar(1);
                    break;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}

VA(0x004192b4, 0x150)
void terrainManager::RandomizeTiles(void) {
    H1_ENUM_LOCAL(TerrainType, i16) i;
    i16 x;
    i16 y;
    u8 tileTerrain;
    u8 tile;

    for (i = TERRAIN_WATER; i < TERRAIN_COUNT; i++)
        gEditManager->BlendTerrain(i, 1, 1, 0, 0);
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            tileTerrain = gEditManager->m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
            tile = gEditManager->m_map.cells[x][y].m_tileIndex;
            if (tileTerrain * MAP_CELL_TILES_PER_TERRAIN == tile)
                gEditManager->m_map.cells[x][y].m_tileIndex +=
                    Random(0, TERRAIN_TILE_VARIANT_COUNT - 1);
        }
    }
}

VA(0x00419404, 0x47)
void terrainManager::SelectTerrain(H1_ENUM_PARAM(TerrainType, i16) terrain) {
    m_terrain = terrain;
    m_highlight->m_y =
        TERRAIN_BUTTON_FIRST_Y + H1_ENUM_ENCODE(TerrainType, terrain) * TERRAIN_BUTTON_HEIGHT;
    gEditManager->m_window->DrawWindow();
    gLastTerrain = terrain;
}
