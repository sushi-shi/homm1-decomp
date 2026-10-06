// The editor's main manager (Editor\EDITMGR.CPP): the edited map and its
// undo copy, the map view with its rulers, scroll bars and radar, the tool
// selection, terrain painting and smoothing, the save checks and the .MAP
// reader and writer.
// Descriptive names: every editManager member and method other than
// SelectTool, Open, Close and Main; SetTileVariant, MakeMapCode,
// ShowStatusWarning, ScatterDetails, gMapCodeLetters, gEditMapHeader,
// gSelectionColor, gEditErrors, gEditErrorCount, gVaryTiles,
// gPickMapNameDummy.

#include <match.h>

#include <windows.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/TILE.h>
#include <BASE/tileset.h>
#include <EDITOR/clearManager.h>
#include <EDITOR/editManager.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/eventsManager.h>
#include <EDITOR/overlayManager.h>
#include <EDITOR/terrainManager.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/gameTypes.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/terrainTypes.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define EDITMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Editor\\EDITMGR.CPP"

DATA(0x0043e5d4)
char* gMapCodeLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
DATA(0x00451218)
i32 gSelectionColor;
DATA(0x0045121c)
SMapHeader gEditMapHeader;
DATA(0x004519ec)
char* gEditErrors[EDIT_MANAGER_ERROR_CAPACITY];
DATA(0x00451b7c)
i32 gEditErrorCount;
DATA(0x00451b80)
i32 gVaryTiles;
DATA(0x00451b84)
char gPickMapNameDummy[4];

VA(0x004017e0, 0xfb)
editManager::editManager(void) {
    m_viewX = 0;
    m_viewY = 0;
    m_cursorX = 0;
    m_cursorY = 0;
    m_lastCommandId = -1;
    m_dispatchMask = EDIT_MANAGER_DISPATCH_MASK;
    gNextObjectId = 1;
    m_zoomedOut = EDIT_ZOOM_OUT;
    ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
    SaveUndo();
    gMapHeader = &gEditMapHeader;
    m_animationFrame = 0;
    m_tool = EDIT_MANAGER_NO_TOOL;
    m_toolManager = NULL;
    m_mapChanged = 0;
    m_placedX = m_placedY = -1;
    m_extraCount = MAP_EXTRA_FIRST_RECORD;
    m_placedState = -1;
}

VA(0x004018db, 0x41)
void editManager::LoadObjectIcons(i16 tileset, char* largeName, char* smallName) {
    m_objectIcons[tileset][EDIT_ZOOM_NORMAL] = gResourceManager->GetIcon(largeName);
    m_objectIcons[tileset][EDIT_ZOOM_OUT] = gResourceManager->GetIcon(smallName);
}

VA(0x0040191c, 0x57a)
i16 editManager::Open(i16 priority) {
    bitmap* borderImage;
    i32 i;

    NewMap(0);
    borderImage = gResourceManager->GetBitmap("bordedit.bmp");
    BlitBitmapToScreen(borderImage, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0, 0);
    gResourceManager->Dispose(borderImage);
    m_statusFont = gResourceManager->GetFont("smalfont.fnt");
    m_window = new heroWindow(0, 0, "editwind.bin");
    m_horizontalTrack = new iconWidget(
        32,
        464,
        416,
        16,
        "escroll.icn",
        0,
        0,
        EDIT_CONTROL_HORIZONTAL_TRACK,
        ICON_WIDGET_DRAW,
        1
    );
    m_verticalTrack = new iconWidget(
        464,
        32,
        16,
        416,
        "escroll.icn",
        1,
        0,
        EDIT_CONTROL_VERTICAL_TRACK,
        ICON_WIDGET_DRAW,
        1
    );
    m_horizontalKnob = new iconWidget(
        EDIT_KNOB_FIRST,
        468,
        17,
        8,
        "escroll.icn",
        2,
        0,
        EDIT_CONTROL_HORIZONTAL_KNOB,
        ICON_WIDGET_DRAW,
        1
    );
    m_verticalKnob = new iconWidget(
        468,
        EDIT_KNOB_FIRST,
        8,
        17,
        "escroll.icn",
        3,
        0,
        EDIT_CONTROL_VERTICAL_KNOB,
        ICON_WIDGET_DRAW,
        1
    );
    m_window->AddWidget(m_horizontalTrack, -1);
    m_window->AddWidget(m_verticalTrack, -1);
    m_window->AddWidget(m_horizontalKnob, -1);
    m_window->AddWidget(m_verticalKnob, -1);
    gWindowManager->AddWindow(m_window, -1, 1);
    m_groundTiles[EDIT_ZOOM_NORMAL] = gResourceManager->GetTileset("ground32.til");
    m_groundTiles[EDIT_ZOOM_OUT] = gResourceManager->GetTileset("ground16.til");
    m_cloudTiles[EDIT_ZOOM_NORMAL] = gResourceManager->GetTileset("clof32.til");
    m_cloudTiles[EDIT_ZOOM_OUT] = gResourceManager->GetTileset("clof16.til");
    for (i = 0; i < EDIT_MANAGER_TILESET_COUNT; i++)
        m_objectIcons[i][EDIT_ZOOM_NORMAL] = m_objectIcons[i][EDIT_ZOOM_OUT] = NULL;
    LoadObjectIcons(TILESET_OBJ32_00, "obj32-00.icn", "obj16-00.icn");
    LoadObjectIcons(TILESET_OBJ32_01, "obj32-01.icn", "obj16-01.icn");
    LoadObjectIcons(TILESET_OBJ32_02, "obj32-02.icn", "obj16-02.icn");
    LoadObjectIcons(TILESET_OBJ32_03, "obj32-03.icn", "obj16-03.icn");
    LoadObjectIcons(TILESET_OBJ32_04, "obj32-04.icn", "obj16-04.icn");
    LoadObjectIcons(TILESET_OBJ32_05, "obj32-05.icn", "obj16-05.icn");
    LoadObjectIcons(TILESET_OBJ32_06, "obj32-06.icn", "obj16-06.icn");
    LoadObjectIcons(TILESET_OBJ32_07, "obj32-07.icn", "obj16-07.icn");
    LoadObjectIcons(TILESET_MTN32, "mtn32.icn", "mtn16.icn");
    LoadObjectIcons(TILESET_TREE32, "tree32.icn", "tree16.icn");
    LoadObjectIcons(TILESET_TOWN32, "town32.icn", "town16.icn");
    LoadObjectIcons(TILESET_RSRC32, "rsrc32.icn", "rsrc16.icn");
    LoadObjectIcons(TILESET_MONS32, "mons32.icn", "mons16.icn");
    LoadObjectIcons(TILESET_ART32, "art32.icn", "art16.icn");
    m_buttons = gResourceManager->GetIcon("buttons.icn");
    m_window->DrawWindow(0);
    DrawRadar(1);
    DrawView(m_viewX, m_viewY);
    UpdateMapView();
    gMouseManager->SetPointer("editor.mse", 0);
    gMouseManager->WarpPointer(LOGICAL_SCREEN_WIDTH / 2, 200);
    gMouseManager->ReallyShowPointer();
    gMouseManager->NewUpdate(1);
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "editManager");
    SelectTool(EDIT_TOOL_TERRAIN);
    return BASE_MANAGER_SUCCESS;
}

VA(0x00401e96, 0x11e)
void editManager::Close(void) {
    i32 i;

    NewMap(0);
    ClearErrors();
    SelectTool(EDIT_MANAGER_NO_TOOL);
    gWindowManager->RemoveWindow(m_window);
    delete m_window;
    gResourceManager->Dispose(m_groundTiles[EDIT_ZOOM_NORMAL]);
    gResourceManager->Dispose(m_groundTiles[EDIT_ZOOM_OUT]);
    gResourceManager->Dispose(m_cloudTiles[EDIT_ZOOM_NORMAL]);
    gResourceManager->Dispose(m_cloudTiles[EDIT_ZOOM_OUT]);
    for (i = 0; i < EDIT_MANAGER_TILESET_COUNT; i++) {
        gResourceManager->Dispose(m_objectIcons[i][EDIT_ZOOM_NORMAL]);
        gResourceManager->Dispose(m_objectIcons[i][EDIT_ZOOM_OUT]);
    }
    gResourceManager->Dispose(m_buttons);
    gResourceManager->Dispose(m_statusFont);
    gMouseManager->SetPointer(-1);
    m_active = 0;
}

VA(0x00401fb4, 0x883)
i16 editManager::Main(tag_message& message) {
    i32 spare1;
    i32 spare2;
    i32 helpIndex;
    editMap* swap;

    if (!(message.type & m_dispatchMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    helpIndex = -1;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                        break;
                    switch (message.id) {
                        case EDIT_CONTROL_TERRAIN:
                            SelectTool(EDIT_TOOL_TERRAIN);
                            break;
                        case EDIT_CONTROL_OBJECTS:
                            SelectTool(EDIT_TOOL_OBJECTS);
                            break;
                        case EDIT_CONTROL_DETAILS:
                            SelectTool(EDIT_TOOL_DETAILS);
                            break;
                        case EDIT_CONTROL_ERASER:
                            SelectTool(EDIT_TOOL_ERASER);
                            break;
                        case EDIT_CONTROL_LOAD:
                            if (!PickMap(gPickMapNameDummy, "map", FILE_REQUESTER_LOAD))
                                break;
                            if (!LoadMap(m_mapFileName)) {
                                m_mapChanged = 0;
                                m_placedX = m_placedY = -1;
                            }
                            m_viewX = m_viewY = 0;
                            m_window->DrawWindow(0);
                            DrawRadar(1);
                            DrawMap();
                            UpdateMapView();
                            break;
                        case EDIT_CONTROL_SAVE:
                            if (!SaveMap(m_mapFileName)) {
                                sprintf(
                                    gText,
                                    localization::Tr("editor.map.saved"),
                                    gMapHeader->name[0]
                                );
                                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
                                m_mapChanged = 0;
                                m_placedX = m_placedY = -1;
                            }
                            break;
                        case EDIT_CONTROL_ZOOM:
                            ToggleZoom();
                            break;
                        case EDIT_CONTROL_QUIT:
                            if (Confirm(localization::Tr("editor.quit.confirm")) == 1)
                                ShutDown(NULL);
                            else
                                return MESSAGE_DISPATCH_CONSUME;
                            break;
                        case EDIT_CONTROL_MAP_INFO:
                            MapDetailsDialog(0);
                            break;
                        case EDIT_CONTROL_RANDOM_MAP:
                            GenerateRandomMap();
                            break;
                        case EDIT_CONTROL_NEW:
                            if (Confirm(localization::Tr("editor.map.new.confirm"))) {
                                ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
                                NewMap(0);
                                DrawMap();
                                UpdateMapView();
                                DrawRadar(1);
                            }
                            break;
                        case EDIT_CONTROL_UNDO:
                            swap = static_cast<editMap*>(malloc(sizeof(editMap)));
                            memcpy(swap, &m_map, sizeof(editMap));
                            memcpy(&m_map, &m_undoMap, sizeof(editMap));
                            memcpy(&m_undoMap, swap, sizeof(editMap));
                            free(swap);
                            DrawView(m_viewX, m_viewY);
                            UpdateMapView();
                            DrawRadar(1);
                            break;
                        case EDIT_CONTROL_SCROLL_UP:
                            Scroll(0, -1);
                            break;
                        case EDIT_CONTROL_SCROLL_DOWN:
                            Scroll(0, 1);
                            break;
                        case EDIT_CONTROL_SCROLL_RIGHT:
                            Scroll(1, 0);
                            break;
                        case EDIT_CONTROL_SCROLL_LEFT:
                            Scroll(-1, 0);
                            break;
                        case EDIT_CONTROL_SCROLL_UP_LEFT:
                            Scroll(-1, -1);
                            break;
                        case EDIT_CONTROL_SCROLL_UP_RIGHT:
                            Scroll(1, -1);
                            break;
                        case EDIT_CONTROL_SCROLL_DOWN_LEFT:
                            Scroll(-1, 1);
                            break;
                        case EDIT_CONTROL_SCROLL_DOWN_RIGHT:
                            Scroll(1, 1);
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_RIGHT_CLICK:
                    switch (message.id) {
                        case EDIT_CONTROL_SCROLL_UP:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_DOWN:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_RIGHT:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_LEFT:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_UP_LEFT:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_UP_RIGHT:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_DOWN_LEFT:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_SCROLL_DOWN_RIGHT:
                            helpIndex = 1;
                            break;
                        case EDIT_CONTROL_ZOOM:
                            helpIndex = 2;
                            break;
                        case EDIT_CONTROL_UNDO:
                            helpIndex = 3;
                            break;
                        case EDIT_CONTROL_MAP_INFO:
                            helpIndex = 4;
                            break;
                        case EDIT_CONTROL_NEW:
                            helpIndex = 5;
                            break;
                        case EDIT_CONTROL_LOAD:
                            helpIndex = 6;
                            break;
                        case EDIT_CONTROL_SAVE:
                            helpIndex = 7;
                            break;
                        case EDIT_CONTROL_QUIT:
                            helpIndex = 8;
                            break;
                        case EDIT_CONTROL_RANDOM_MAP:
                            helpIndex = 9;
                            break;
                    }
                    if (helpIndex >= 0)
                        NormalDialog(gEditButtonHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
                    break;
                case WIDGET_NOTIFY_SELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                        switch (message.id) {
                            case EDIT_CONTROL_RADAR:
                                helpIndex = 1;
                                break;
                            case EDIT_CONTROL_HORIZONTAL_TRACK:
                                helpIndex = 2;
                                break;
                            case EDIT_CONTROL_HORIZONTAL_KNOB:
                                helpIndex = 2;
                                break;
                            case EDIT_CONTROL_VERTICAL_TRACK:
                                helpIndex = 2;
                                break;
                            case EDIT_CONTROL_VERTICAL_KNOB:
                                helpIndex = 2;
                                break;
                            case EDIT_CONTROL_TERRAIN:
                                helpIndex = 3;
                                break;
                            case EDIT_CONTROL_OBJECTS:
                                helpIndex = 4;
                                break;
                            case EDIT_CONTROL_DETAILS:
                                helpIndex = 5;
                                break;
                            case EDIT_CONTROL_ERASER:
                                helpIndex = 6;
                                break;
                            case EDIT_CONTROL_MAP:
                                helpIndex = 7;
                                break;
                        }
                        if (helpIndex >= 0)
                            NormalDialog(gEditAreaHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
                    } else {
                        switch (message.id) {
                            case EDIT_CONTROL_RADAR:
                                DoRadar();
                                break;
                            case EDIT_CONTROL_HORIZONTAL_TRACK:
                            case EDIT_CONTROL_HORIZONTAL_KNOB:
                                DoHorizontalKnob();
                                break;
                            case EDIT_CONTROL_VERTICAL_TRACK:
                            case EDIT_CONTROL_VERTICAL_KNOB:
                                DoVerticalKnob();
                                break;
                        }
                    }
                    break;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_F3:
                    ScatterDetails();
                    DrawMap();
                    UpdateMapView();
                    break;
                case INPUT_SCAN_TAB:
                    SelectTool((m_tool + 1) % EDIT_TOOL_COUNT);
                    break;
                case INPUT_SCAN_ESCAPE:
                    if (Confirm(localization::Tr("editor.quit.confirm")) == 1)
                        ShutDown(NULL);
                    else
                        return MESSAGE_DISPATCH_CONSUME;
                    break;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}

VA(0x00402837, 0x28)
void editManager::SaveUndo(void) {
    memcpy(&m_undoMap, &m_map, sizeof(m_undoMap));
}

VA(0x0040285f, 0x24)
void editManager::UpdateMapView(void) {
    gWindowManager
        ->UpdateScreenRegion(EDIT_VIEW_LEFT, EDIT_VIEW_TOP, EDIT_VIEW_PIXELS, EDIT_VIEW_PIXELS);
}

VA(0x00402883, 0x3f)
void editManager::UpdateCursor(void) {
    DrawRulers(m_viewX, m_viewY, m_cursorX, m_cursorY);
}

// The rulers number the view's columns along the top and its rows down the
// left side, marking the cursor's column and row while the mouse is over the
// view.
VA(0x004028c2, 0x28b)
void editManager::DrawRulers(i16 viewX, i16 viewY, i16 cursorX, i16 cursorY) {
    i16 mouseX;
    i16 mouseY;
    u8 color;
    char text[8];
    i16 i;

    gMouseManager->MouseCoords(mouseX, mouseY);
    if (mouseX < EDIT_VIEW_LEFT || mouseX >= EDIT_VIEW_LEFT + EDIT_VIEW_PIXELS
        || mouseY < EDIT_VIEW_TOP || mouseY > EDIT_VIEW_TOP + EDIT_VIEW_PIXELS) {
        cursorX = -1;
        cursorY = -1;
    }
    for (i = 0; i < EDIT_RULER_SLOTS; i++) {
        if (!m_zoomedOut && (i & 1))
            continue;
        if (!m_zoomedOut)
            m_buttons->DrawToBuffer(i * EDIT_RULER_SLOT_PIXELS + EDIT_VIEW_LEFT, 0, 24, 0, 0);
        else
            m_buttons->DrawToBuffer(i * EDIT_RULER_SLOT_PIXELS + EDIT_VIEW_LEFT, 0, 18, 0, 0);
        sprintf(text, "%02d", viewX + i / (m_zoomedOut ? 1 : 2));
        if (i / (m_zoomedOut ? 1 : 2) == cursorX)
            color = 1;
        else
            color = 192;
        m_statusFont
            ->DrawString(text, i * EDIT_RULER_SLOT_PIXELS + (m_zoomedOut ? 0 : 8) + 19, 2, color);
        if (!m_zoomedOut)
            m_buttons->DrawToBuffer(0, i * EDIT_RULER_SLOT_PIXELS + EDIT_VIEW_TOP, 25, 0, 0);
        else
            m_buttons->DrawToBuffer(0, i * EDIT_RULER_SLOT_PIXELS + EDIT_VIEW_TOP, 18, 0, 0);
        sprintf(text, "%02d", viewY + i / (m_zoomedOut ? 1 : 2));
        if (i / (m_zoomedOut ? 1 : 2) == cursorY)
            color = 1;
        else
            color = 192;
        m_statusFont
            ->DrawString(text, 3, i * EDIT_RULER_SLOT_PIXELS + (m_zoomedOut ? 0 : 8) + 18, color);
    }
    gWindowManager->UpdateScreenRegion(EDIT_VIEW_LEFT, 0, EDIT_VIEW_PIXELS, EDIT_VIEW_TOP);
    gWindowManager->UpdateScreenRegion(0, EDIT_VIEW_TOP, EDIT_VIEW_LEFT, EDIT_VIEW_PIXELS);
}

VA(0x00402b4d, 0x136)
void editManager::ScreenToCell(i16& x, i16& y) {
    x -= EDIT_VIEW_LEFT;
    y -= EDIT_VIEW_TOP;
    switch (m_zoomedOut) {
        case EDIT_ZOOM_NORMAL:
            x /= EDIT_VIEW_CELL_PIXELS;
            y /= EDIT_VIEW_CELL_PIXELS;
            if (x > EDIT_VIEW_CELLS - 1)
                x = EDIT_VIEW_CELLS - 1;
            if (x < 0)
                x = 0;
            if (y > EDIT_VIEW_CELLS - 1)
                y = EDIT_VIEW_CELLS - 1;
            if (y < 0)
                y = 0;
            break;
        case EDIT_ZOOM_OUT:
            x /= EDIT_VIEW_ZOOMED_CELL_PIXELS;
            y /= EDIT_VIEW_ZOOMED_CELL_PIXELS;
            if (x > EDIT_VIEW_ZOOMED_CELLS - 1)
                x = EDIT_VIEW_ZOOMED_CELLS - 1;
            if (x < 0)
                x = 0;
            if (y > EDIT_VIEW_ZOOMED_CELLS - 1)
                y = EDIT_VIEW_ZOOMED_CELLS - 1;
            if (y < 0)
                y = 0;
            break;
    }
}

VA(0x00402c83, 0x29)
void editManager::DrawMap(void) {
    DrawView(m_viewX, m_viewY);
}

// While the random-map generator runs, the view draws at full zoom and only
// as clouds.
VA(0x00402cac, 0x282)
void editManager::DrawView(i16 viewX, i16 viewY) {
    i32 lineWidth;
    i32 cx;
    i32 cy;
    i32 cellPixels;
    i32 numCells;
    i32 savedZoom;

    savedZoom = m_zoomedOut;
    if (gGeneratingMaps)
        m_zoomedOut = EDIT_ZOOM_NORMAL;
    numCells = m_zoomedOut ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS;
    for (cy = 0; cy < numCells; cy++)
        for (cx = 0; cx < numCells; cx++)
            DrawCell(viewX + cx, viewY + cy, cx, cy, EDIT_DRAW_ALL);
    m_animationFrame++;
    m_animationFrame %= 6;
    cellPixels = m_zoomedOut ? EDIT_VIEW_ZOOMED_CELL_PIXELS : EDIT_VIEW_CELL_PIXELS;
    lineWidth = m_zoomedOut ? 1 : 2;
    if (gSelectionX >= 0) {
        gSelectionColor = gMonoColorMap[190];
        FillBitmapArea(
            gWindowManager->m_screen,
            (gSelectionX - viewX) * cellPixels + EDIT_VIEW_LEFT,
            (gSelectionY - viewY) * cellPixels + EDIT_VIEW_TOP,
            lineWidth,
            gSelectionHeight * cellPixels - 1,
            gSelectionColor
        );
        FillBitmapArea(
            gWindowManager->m_screen,
            (gSelectionX - viewX) * cellPixels + EDIT_VIEW_LEFT,
            (gSelectionY - viewY) * cellPixels + EDIT_VIEW_TOP,
            gSelectionWidth * cellPixels - 1,
            lineWidth,
            gSelectionColor
        );
        FillBitmapArea(
            gWindowManager->m_screen,
            (gSelectionX - viewX + gSelectionWidth) * cellPixels + EDIT_VIEW_LEFT - lineWidth,
            (gSelectionY - viewY) * cellPixels + EDIT_VIEW_TOP,
            lineWidth,
            gSelectionHeight * cellPixels - 1,
            gSelectionColor
        );
        FillBitmapArea(
            gWindowManager->m_screen,
            (gSelectionX - viewX) * cellPixels + EDIT_VIEW_LEFT,
            (gSelectionY - viewY + gSelectionHeight) * cellPixels + EDIT_VIEW_TOP - lineWidth,
            gSelectionWidth * cellPixels - 1,
            lineWidth,
            gSelectionColor
        );
    }
    m_zoomedOut = savedZoom;
}

VA(0x00402f2e, 0x255)
void editManager::DrawRadar(i32) {
    u8 color;
    i16 x;
    i16 y;
    icon* buttonIcn;

    buttonIcn = gResourceManager->GetIcon("buttons.icn");
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            if (m_map.cells[x][y].m_objectIndex != MAP_CELL_NO_FRAME) {
                switch (m_map.cells[x][y].m_objectTileset & MAP_CELL_TILESET_MASK) {
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        color = gRadarTerrainColor
                                    [m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN]
                                + 3;
                        break;
                    case TILESET_TOWN32:
                        color = 4;
                        break;
                    case TILESET_RSRC32:
                        color = 10;
                        break;
                    default:
                        color = gRadarTerrainColor
                            [m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN];
                        break;
                }
            } else {
                color =
                    gRadarTerrainColor[m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN];
            }
            if (gGeneratingMaps)
                color = 0;
            buttonIcn->FillToBuffer(
                x * EDIT_RADAR_CELL_PIXELS + EDIT_RADAR_LEFT,
                y * EDIT_RADAR_CELL_PIXELS + EDIT_RADAR_TOP,
                21,
                color,
                0,
                0
            );
        }
    }
    buttonIcn->FillToBuffer(
        m_viewX * EDIT_RADAR_CELL_PIXELS + EDIT_RADAR_LEFT,
        m_viewY * EDIT_RADAR_CELL_PIXELS + EDIT_RADAR_TOP,
        m_zoomedOut ? 23 : 22,
        190,
        0,
        0
    );
    gResourceManager->Dispose(buttonIcn);
    gWindowManager
        ->UpdateScreenRegion(EDIT_RADAR_TOP, EDIT_RADAR_LEFT, EDIT_RADAR_PIXELS, EDIT_RADAR_PIXELS);
    UpdateKnobs(1);
    UpdateCursor();
}

VA(0x00403183, 0x34b)
void editManager::DrawCell(i16 x, i16 y, i16 column, i16 row, u8 layers) {
    u8 tileset;
    u16 groundFrame;
    i16 sx;
    i16 sy;
    mapCell* cell;

    cell = &m_map.cells[x][y];
    sx = column * (m_zoomedOut ? EDIT_VIEW_ZOOMED_CELL_PIXELS : EDIT_VIEW_CELL_PIXELS)
         + EDIT_VIEW_LEFT;
    sy = row * (m_zoomedOut ? EDIT_VIEW_ZOOMED_CELL_PIXELS : EDIT_VIEW_CELL_PIXELS) + EDIT_VIEW_TOP;
    if (gGeneratingMaps) {
        if (layers & EDIT_DRAW_OVERLAY)
            TileToBitmap(m_cloudTiles[m_zoomedOut], (x + y) & 3, gWindowManager->m_screen, sx, sy);
        return;
    }
    if (layers & EDIT_DRAW_GROUND) {
        groundFrame = cell->m_flags;
        groundFrame <<= MAP_CELL_GROUND_FLIP_SHIFT;
        groundFrame |= cell->m_tileIndex & 0xff;
        TileToBitmap(m_groundTiles[m_zoomedOut], groundFrame, gWindowManager->m_screen, sx, sy);
    }
    if (layers & EDIT_DRAW_OBJECT) {
        if (cell->m_objectIndex != MAP_CELL_NO_FRAME) {
            tileset = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
            IconToBitmap(
                m_objectIcons[tileset][m_zoomedOut],
                gWindowManager->m_screen,
                sx,
                sy,
                cell->m_objectIndex,
                ICON_DRAW_OFFSET_FULL
            );
            if (cell->m_flags & MAP_CELL_OBJECT_ANIMATED)
                IconToBitmap(
                    m_objectIcons[tileset][m_zoomedOut],
                    gWindowManager->m_screen,
                    sx,
                    sy,
                    cell->m_objectIndex + m_animationFrame + 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        if (cell->m_flags & MAP_CELL_OBJECT_EXTRA)
            IconToBitmap(
                m_objectIcons[TILESET_RSRC32][m_zoomedOut],
                gWindowManager->m_screen,
                sx,
                sy,
                cell->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
    if (layers & EDIT_DRAW_OVERLAY) {
        if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
            tileset = cell->m_overlayTileset & MAP_CELL_TILESET_MASK;
            IconToBitmap(
                m_objectIcons[tileset][m_zoomedOut],
                gWindowManager->m_screen,
                sx,
                sy,
                cell->m_overlayIndex,
                ICON_DRAW_OFFSET_FULL
            );
            if (cell->m_flags & MAP_CELL_OVERLAY_ANIMATED)
                IconToBitmap(
                    m_objectIcons[tileset][m_zoomedOut],
                    gWindowManager->m_screen,
                    sx,
                    sy,
                    cell->m_overlayIndex + m_animationFrame + 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
}

// Zooming keeps the view centred: out moves the origin back by ten cells,
// in moves it forward by ten.
VA(0x004034ce, 0x78)
void editManager::ToggleZoom(void) {
    if (!m_zoomedOut) {
        m_zoomedOut = EDIT_ZOOM_OUT;
        Scroll(-10, -10);
    } else {
        m_zoomedOut = EDIT_ZOOM_NORMAL;
        Scroll(10, 10);
    }
    DrawView(m_viewX, m_viewY);
    DrawRadar(1);
    UpdateMapView();
}

// The tool buttons show their frame pair from EDIT_TOOL_BUTTON_FRAME, the
// second frame for the selected tool.
VA(0x00403546, 0x29c)
void editManager::SelectTool(i16 tool) {
    i32 i;
    tag_message msg;

    if (m_tool == tool)
        return;
    if (m_toolManager) {
        gExec->RemoveManager(m_toolManager);
        delete m_toolManager;
        m_toolManager = NULL;
    }
    for (i = 0; i < EDIT_TOOL_COUNT; i++) {
        msg.type = MESSAGE_WIDGET;
        msg.id = i + EDIT_CONTROL_TERRAIN;
        msg.command = WIDGET_COMMAND_SET_FRAME;
        msg.value = (i == tool) + i * 2 + EDIT_TOOL_BUTTON_FRAME;
        m_window->BroadcastMessage(msg);
    }
    switch (tool) {
        case EDIT_TOOL_TERRAIN:
            m_toolManager = new terrainManager;
            break;
        case EDIT_TOOL_OBJECTS:
            m_toolManager = new overlayManager;
            break;
        case EDIT_TOOL_DETAILS:
            m_toolManager = new eventsManager;
            break;
        case EDIT_TOOL_ERASER:
            m_toolManager = new clearManager;
            break;
    }
    if (m_toolManager) {
        if (!gExec->AddManager(m_toolManager, BASE_MANAGER_PRIORITY_UNASSIGNED)) {
            m_tool = tool;
        } else {
            m_toolManager = NULL;
            m_tool = EDIT_MANAGER_NO_TOOL;
        }
    } else {
        m_tool = EDIT_MANAGER_NO_TOOL;
    }
    m_window->DrawWindow();
}

VA(0x004037e2, 0x141)
void editManager::Scroll(i16 dx, i16 dy) {
    m_viewX += dx;
    if (m_viewX < 0)
        m_viewX = 0;
    if (m_viewX > MAP_CELL_GRID_SIZE - (m_zoomedOut ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS))
        m_viewX = MAP_CELL_GRID_SIZE - (m_zoomedOut ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS);
    m_viewY += dy;
    if (m_viewY < 0)
        m_viewY = 0;
    if (m_viewY > MAP_CELL_GRID_SIZE - (m_zoomedOut ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS))
        m_viewY = MAP_CELL_GRID_SIZE - (m_zoomedOut ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS);
    DrawView(m_viewX, m_viewY);
    UpdateMapView();
    DrawRadar(1);
}

VA(0x00403923, 0xcd)
void editManager::UpdateKnobs(i16 update) {
    double scaleX;
    double scaleY;
    i16 xPos;
    i16 yPos;

    scaleX = 402.0 / (m_zoomedOut ? 45 : 59);
    scaleY = 402.0 / (m_zoomedOut ? 45 : 59);
    xPos = m_viewX * scaleX;
    yPos = m_viewY * scaleY;
    m_horizontalKnob->m_x = xPos + EDIT_KNOB_FIRST;
    m_verticalKnob->m_y = yPos + EDIT_KNOB_FIRST;
    m_window->DrawWindow(update, EDIT_CONTROL_HORIZONTAL_TRACK, EDIT_CONTROL_SCROLL_DOWN_RIGHT);
}

// Paints the view cells' ground with the terrain's first tile (the cells are
// redrawn at the brush's origin).
VA(0x004039f0, 0x1e0)
void editManager::PaintGround(i16 column, i16 row, i16 width, i16 height, i16 terrain) {
    i16 startY;
    i16 startX;
    i32 redrawHeight;
    i32 j;
    i32 spacing;
    i32 redrawTop;
    i32 i;
    i32 redrawLeft;
    i32 redrawWidth;

    startX = m_viewX + column;
    startY = m_viewY + row;
    spacing = m_zoomedOut == EDIT_ZOOM_OUT ? EDIT_VIEW_ZOOMED_CELL_PIXELS : EDIT_VIEW_CELL_PIXELS;
    gEditManager->ClearArea(startX, startY, width, height, EDIT_CLEAR_ALL, 0);
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            m_map.cells[startX + i][startY + j].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
            if (column + i
                    < (m_zoomedOut == EDIT_ZOOM_OUT ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS)
                && row + j
                       < (m_zoomedOut == EDIT_ZOOM_OUT ? EDIT_VIEW_ZOOMED_CELLS : EDIT_VIEW_CELLS))
                DrawCell(startX, startY, column + i, row + j, EDIT_DRAW_ALL);
        }
    }
    redrawLeft = column * spacing + EDIT_VIEW_LEFT;
    redrawWidth = width * spacing;
    if (redrawLeft + redrawWidth > LOGICAL_SCREEN_WIDTH - 1)
        redrawWidth = LOGICAL_SCREEN_WIDTH - 1 - redrawLeft;
    redrawTop = row * spacing + EDIT_VIEW_TOP;
    redrawHeight = height * spacing;
    if (redrawTop + redrawHeight > LOGICAL_SCREEN_HEIGHT - 1)
        redrawHeight = LOGICAL_SCREEN_HEIGHT - 1 - redrawTop;
    gWindowManager->UpdateScreenRegion(redrawLeft, redrawTop, redrawWidth, redrawHeight);
    DrawRadar(1);
}

VA(0x00403bd0, 0xf7)
void editManager::FillGround(i16 x, i16 y, i16 width, i16 height, i16 terrain) {
    i32 i;
    i32 j;

    gEditManager->ClearArea(x, y, width, height, EDIT_CLEAR_ALL, 0);
    for (i = x; i < x + width; i++) {
        for (j = y; j < y + height; j++) {
            m_map.cells[i][j].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN + Random(0, 3);
            m_map.cells[i][j].m_flags &= ~(MAP_CELL_OVERLAY_EXTRA | MAP_CELL_HERO_CURSOR);
        }
    }
}

// A cell keeps its ground tile unless it is outside the four variants from
// tile on (or gVaryTiles is set); an object's cell keeps a tile of another
// terrain.
VA(0x00403cc7, 0x71)
void SetTileVariant(mapCell* cell, i32 tile) {
    if (cell->m_objectTileset
        && cell->m_tileIndex / MAP_CELL_TILES_PER_TERRAIN != tile / MAP_CELL_TILES_PER_TERRAIN)
        return;
    if (cell->m_tileIndex < tile || cell->m_tileIndex >= tile + 4 || gVaryTiles)
        cell->m_tileIndex = tile + Random(0, 3);
}

// Removes single cells of terrain (cells whose neighbours of the terrain do
// not meet at a corner take the other terrain or, with fromUndo, their undo
// terrain), then gives every other terrain cell its border tile and flips.
VA(0x00403d38, 0xbdb)
void editManager::BlendTerrain(i16 terrain, u8, u8 fromUndo, u8 skipBorders, u8 skipFill) {
    u8 east;
    u8 ne;
    i32 terrainBase;
    i32 surrounding;
    u8 nw;
    i16 x;
    u8 se;
    i16 y;
    u8 south;
    u8 north;
    u8 sw;
    u8 west;
    mapCell* cell;
    i32 thisTerrain;

    surrounding = 0;
    if (skipFill)
        goto borders;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            if (m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN == terrain) {
                north = south = east = west = nw = ne = sw = se = 0;
                if (y == 0 || gGroundToTerrain[m_map.cells[x][y - 1].m_tileIndex] == terrain)
                    north = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x][y - 1].m_tileIndex];
                if (y == MAP_CELL_GRID_SIZE
                    || gGroundToTerrain[m_map.cells[x][y + 1].m_tileIndex] == terrain)
                    south = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x][y + 1].m_tileIndex];
                if (x == MAP_CELL_GRID_SIZE
                    || gGroundToTerrain[m_map.cells[x + 1][y].m_tileIndex] == terrain)
                    east = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x + 1][y].m_tileIndex];
                if (x == 0 || gGroundToTerrain[m_map.cells[x - 1][y].m_tileIndex] == terrain)
                    west = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x - 1][y].m_tileIndex];
                if (!((north && west) || (north && east) || (south && west) || (south && east))) {
                    if (fromUndo)
                        m_map.cells[x][y].m_tileIndex =
                            gGroundToTerrain[m_undoMap.cells[x][y].m_tileIndex]
                            * MAP_CELL_TILES_PER_TERRAIN;
                    else
                        m_map.cells[x][y].m_tileIndex = surrounding * MAP_CELL_TILES_PER_TERRAIN;
                }
                if (x == 0 || y == 0
                    || gGroundToTerrain[m_map.cells[x - 1][y - 1].m_tileIndex] == terrain)
                    nw = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x - 1][y - 1].m_tileIndex];
                if (x == 0 || y == MAP_CELL_GRID_SIZE
                    || gGroundToTerrain[m_map.cells[x - 1][y + 1].m_tileIndex] == terrain)
                    sw = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x - 1][y + 1].m_tileIndex];
                if (x == MAP_CELL_GRID_SIZE || y == MAP_CELL_GRID_SIZE
                    || gGroundToTerrain[m_map.cells[x + 1][y + 1].m_tileIndex] == terrain)
                    se = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x + 1][y + 1].m_tileIndex];
                if (x == MAP_CELL_GRID_SIZE || y == 0
                    || gGroundToTerrain[m_map.cells[x + 1][y - 1].m_tileIndex] == terrain)
                    ne = 1;
                else
                    surrounding = gGroundToTerrain[m_map.cells[x + 1][y + 1].m_tileIndex];
                if (!((north && ne && east) || (north && nw && west) || (south && se && east)
                      || (south && sw && west))
                    && !m_map.cells[x][y].m_objectTileset) {
                    if (fromUndo)
                        m_map.cells[x][y].m_tileIndex =
                            gGroundToTerrain[m_undoMap.cells[x][y].m_tileIndex]
                            * MAP_CELL_TILES_PER_TERRAIN;
                    else
                        m_map.cells[x][y].m_tileIndex = surrounding * MAP_CELL_TILES_PER_TERRAIN;
                }
            }
        }
    }
borders:
    if (skipBorders)
        return;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            thisTerrain = cell->m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
            terrainBase = thisTerrain * MAP_CELL_TILES_PER_TERRAIN;
            north = south = east = west = 0;
            if (thisTerrain != TERRAIN_DIRT) {
                if (y > 0 && gGroundToTerrain[m_map.cells[x][y - 1].m_tileIndex] != thisTerrain)
                    north = 1;
                if (y < MAP_CELL_GRID_SIZE - 1
                    && gGroundToTerrain[m_map.cells[x][y + 1].m_tileIndex] != thisTerrain)
                    south = 1;
                if (x < MAP_CELL_GRID_SIZE - 1
                    && gGroundToTerrain[m_map.cells[x + 1][y].m_tileIndex] != thisTerrain)
                    east = 1;
                if (x > 0 && gGroundToTerrain[m_map.cells[x - 1][y].m_tileIndex] != thisTerrain)
                    west = 1;
                cell->m_flags &= ~(MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL);
                if (north) {
                    if (west) {
                        SetTileVariant(cell, terrainBase + 8);
                        cell->m_flags |= MAP_CELL_GROUND_FLIP_HORIZONTAL;
                    } else if (east) {
                        SetTileVariant(cell, terrainBase + 8);
                    } else {
                        SetTileVariant(cell, terrainBase + 4);
                    }
                } else if (south) {
                    if (west) {
                        SetTileVariant(cell, terrainBase + 8);
                        cell->m_flags |=
                            MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL;
                    } else if (east) {
                        SetTileVariant(cell, terrainBase + 8);
                        cell->m_flags |= MAP_CELL_GROUND_FLIP_VERTICAL;
                    } else {
                        SetTileVariant(cell, terrainBase + 4);
                        cell->m_flags |= MAP_CELL_GROUND_FLIP_VERTICAL;
                    }
                } else if (west) {
                    SetTileVariant(cell, terrainBase + 12);
                    cell->m_flags |= MAP_CELL_GROUND_FLIP_HORIZONTAL;
                } else if (east) {
                    SetTileVariant(cell, terrainBase + 12);
                }
                if (!(north | south | east | west)) {
                    if (x > 0 && y > 0
                        && gGroundToTerrain[m_map.cells[x - 1][y - 1].m_tileIndex] != thisTerrain)
                        north = 1;
                    if (x < MAP_CELL_GRID_SIZE - 1 && y < MAP_CELL_GRID_SIZE - 1
                        && gGroundToTerrain[m_map.cells[x + 1][y + 1].m_tileIndex] != thisTerrain)
                        south = 1;
                    if (x < MAP_CELL_GRID_SIZE - 1 && y > 0
                        && gGroundToTerrain[m_map.cells[x + 1][y - 1].m_tileIndex] != thisTerrain)
                        east = 1;
                    if (x > 0 && y < MAP_CELL_GRID_SIZE - 1
                        && gGroundToTerrain[m_map.cells[x - 1][y + 1].m_tileIndex] != thisTerrain)
                        west = 1;
                    if (north) {
                        SetTileVariant(cell, terrainBase + 16);
                        cell->m_flags |= MAP_CELL_GROUND_FLIP_HORIZONTAL;
                    } else if (south) {
                        SetTileVariant(cell, terrainBase + 16);
                        cell->m_flags |= MAP_CELL_GROUND_FLIP_VERTICAL;
                    } else if (east) {
                        SetTileVariant(cell, terrainBase + 16);
                    } else if (west) {
                        SetTileVariant(cell, terrainBase + 16);
                        cell->m_flags |=
                            MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL;
                    } else {
                        SetTileVariant(cell, terrainBase);
                    }
                }
            }
        }
    }
}

// Clicking the radar centres the view on the clicked cell and follows the
// mouse until the button is released.
VA(0x00404913, 0x3c4)
void editManager::DoRadar(void) {
    tag_message input;
    i16 x;
    tag_message mouseMove;
    i16 y;

    gMouseManager->MouseCoords(x, y);
    if (x < EDIT_RADAR_LEFT || x > EDIT_RADAR_LEFT + EDIT_RADAR_PIXELS || y < EDIT_RADAR_TOP
        || y > EDIT_RADAR_TOP + EDIT_RADAR_PIXELS)
        return;
    x = (x - EDIT_RADAR_LEFT) / EDIT_RADAR_CELL_PIXELS;
    y = (y - EDIT_RADAR_TOP) / EDIT_RADAR_CELL_PIXELS;
    switch (m_zoomedOut) {
        case EDIT_ZOOM_OUT:
            m_viewX = x - EDIT_VIEW_ZOOMED_CELLS / 2;
            if (m_viewX + EDIT_VIEW_ZOOMED_CELLS > MAP_CELL_GRID_SIZE)
                m_viewX = MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS;
            m_viewY = y - EDIT_VIEW_ZOOMED_CELLS / 2;
            if (m_viewY + EDIT_VIEW_ZOOMED_CELLS > MAP_CELL_GRID_SIZE)
                m_viewY = MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS;
            break;
        case EDIT_ZOOM_NORMAL:
            m_viewX = x - EDIT_VIEW_CELLS / 2;
            if (m_viewX + EDIT_VIEW_CELLS > MAP_CELL_GRID_SIZE)
                m_viewX = MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS;
            m_viewY = y - EDIT_VIEW_CELLS / 2;
            if (m_viewY + EDIT_VIEW_CELLS > MAP_CELL_GRID_SIZE)
                m_viewY = MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS;
            break;
    }
    if (m_viewX < 0)
        m_viewX = 0;
    if (m_viewY < 0)
        m_viewY = 0;
    DrawMap();
    DrawRadar(1);
    UpdateMapView();
    input = gInputManager->GetEvent();
    while (input.type != MESSAGE_LEFT_BUTTON_UP && input.type != MESSAGE_RIGHT_BUTTON_UP) {
        Process1WindowsMessage();
        if (input.type == MESSAGE_MOUSE_MOVE) {
            while (input.type == MESSAGE_MOUSE_MOVE) {
                mouseMove = input;
                input = gInputManager->GetEvent();
            }
            gMouseManager->Main(mouseMove);
            x = (mouseMove.x - EDIT_RADAR_LEFT) / EDIT_RADAR_CELL_PIXELS;
            y = (mouseMove.y - EDIT_RADAR_TOP) / EDIT_RADAR_CELL_PIXELS;
            switch (m_zoomedOut) {
                case EDIT_ZOOM_OUT:
                    m_viewX = x - EDIT_VIEW_ZOOMED_CELLS / 2;
                    if (m_viewX + EDIT_VIEW_ZOOMED_CELLS > MAP_CELL_GRID_SIZE)
                        m_viewX = MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS;
                    m_viewY = y - EDIT_VIEW_ZOOMED_CELLS / 2;
                    if (m_viewY + EDIT_VIEW_ZOOMED_CELLS > MAP_CELL_GRID_SIZE)
                        m_viewY = MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS;
                    break;
                case EDIT_ZOOM_NORMAL:
                    m_viewX = x - EDIT_VIEW_CELLS / 2;
                    if (m_viewX + EDIT_VIEW_CELLS > MAP_CELL_GRID_SIZE)
                        m_viewX = MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS;
                    m_viewY = y - EDIT_VIEW_CELLS / 2;
                    if (m_viewY + EDIT_VIEW_CELLS > MAP_CELL_GRID_SIZE)
                        m_viewY = MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS;
                    break;
            }
            if (m_viewX < 0)
                m_viewX = 0;
            if (m_viewY < 0)
                m_viewY = 0;
            DrawMap();
            DrawRadar(1);
            UpdateMapView();
        } else {
            input = gInputManager->GetEvent();
        }
    }
}

VA(0x00404cd7, 0x256)
void editManager::DoHorizontalKnob(void) {
    double scale;
    tag_message latest;
    i16 x;
    tag_message message;
    i16 y;
    i16 newX;

    gMouseManager->SetCursorShape(2);
    scale = 402.0 / (m_zoomedOut ? 45 : 59);
    gMouseManager->MouseCoords(x, y);
    gInputManager->Flush();
    message.type = MESSAGE_MOUSE_MOVE;
    message.x = x;
    message.y = y;
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        Process1WindowsMessage();
        if (message.type == MESSAGE_MOUSE_MOVE) {
            latest = message;
            while (message.type == MESSAGE_MOUSE_MOVE) {
                latest = message;
                message = gInputManager->GetEvent();
            }
            if (latest.x < EDIT_KNOB_FIRST)
                latest.x = EDIT_KNOB_FIRST;
            if (latest.x > EDIT_KNOB_LAST)
                latest.x = EDIT_KNOB_LAST;
            gMouseManager->Main(latest);
            m_horizontalKnob->m_x = latest.x;
            newX = latest.x;
            newX = (newX - EDIT_KNOB_FIRST) / scale;
            if (m_viewX != newX) {
                if (newX
                    > (m_zoomedOut ? MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS
                                   : MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS))
                    newX = m_zoomedOut ? MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS
                                       : MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS;
                if (newX < 0)
                    newX = 0;
                m_viewX = newX;
                DrawMap();
                UpdateMapView();
                DrawRadar(1);
            }
        } else {
            message = gInputManager->GetEvent();
        }
    }
    gMouseManager->SetCursorShape(6);
    m_horizontalKnob->m_flags &= ~WIDGET_FLAG_SELECTED;
    m_horizontalTrack->m_flags &= ~WIDGET_FLAG_SELECTED;
}

VA(0x00404f2d, 0x256)
void editManager::DoVerticalKnob(void) {
    double scale;
    tag_message latest;
    i16 x;
    tag_message message;
    i16 y;
    i16 newY;

    gMouseManager->SetCursorShape(4);
    scale = 402.0 / (m_zoomedOut ? 45 : 59);
    gMouseManager->MouseCoords(x, y);
    gInputManager->Flush();
    message.type = MESSAGE_MOUSE_MOVE;
    message.x = x;
    message.y = y;
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        Process1WindowsMessage();
        if (message.type == MESSAGE_MOUSE_MOVE) {
            latest = message;
            while (message.type == MESSAGE_MOUSE_MOVE) {
                latest = message;
                message = gInputManager->GetEvent();
            }
            // Clamps the horizontal coordinate, as the horizontal knob does.
            if (latest.x < EDIT_KNOB_FIRST)
                latest.x = EDIT_KNOB_FIRST;
            if (latest.x > EDIT_KNOB_LAST)
                latest.x = EDIT_KNOB_LAST;
            gMouseManager->Main(latest);
            m_verticalKnob->m_y = latest.y;
            newY = latest.y;
            newY = (newY - EDIT_KNOB_FIRST) / scale;
            if (m_viewY != newY) {
                if (newY
                    > (m_zoomedOut ? MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS
                                   : MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS))
                    newY = m_zoomedOut ? MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS
                                       : MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS;
                if (newY < 0)
                    newY = 0;
                m_viewY = newY;
                DrawMap();
                UpdateMapView();
                DrawRadar(1);
            }
        } else {
            message = gInputManager->GetEvent();
        }
    }
    gMouseManager->SetCursorShape(6);
    m_verticalKnob->m_flags &= ~WIDGET_FLAG_SELECTED;
    m_verticalTrack->m_flags &= ~WIDGET_FLAG_SELECTED;
}

// The environment sound the game loops near a cell (game::m_mapSounds):
// open water, then the cell's sounding object.
VA(0x00405183, 0x50c)
void editManager::SetCellSound(i16 x, i16 y) {
    mapCell* cell;

    cell = &m_map.cells[x][y];
    m_mapSounds[x][y] = MAP_SOUND_NONE;
    if (cell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN && cell->m_tileIndex > 3)
        m_mapSounds[x][y] = MAP_SOUND_COAST;
    switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
        case MAP_OBJECT_BUOY:
            m_mapSounds[x][y] = MAP_SOUND_BUOY;
            break;
        case MAP_OBJECT_SHIPWRECK:
            m_mapSounds[x][y] = MAP_SOUND_SHIPWRECK;
            break;
        case MAP_OBJECT_WHIRLPOOL:
            m_mapSounds[x][y] = MAP_SOUND_WHIRLPOOL;
            break;
        case MAP_OBJECT_RANKING_SHRINE:
            if (cell->m_triggerType & MAP_TRIGGER_EVENT)
                m_mapSounds[x][y] = MAP_SOUND_RANKING_SHRINE;
            break;
        case MAP_OBJECT_STONE_LITHS:
            m_mapSounds[x][y] = MAP_SOUND_STONE_LITHS;
            break;
        case MAP_OBJECT_ALCHEMIST_LAB:
            if (cell->m_objectIndex == EDIT_SOUND_ALCHEMIST_FRAME_A
                || cell->m_objectIndex == EDIT_SOUND_ALCHEMIST_FRAME_B
                || cell->m_objectIndex == EDIT_SOUND_ALCHEMIST_FRAME_C)
                m_mapSounds[x][y] = MAP_SOUND_ALCHEMIST_LAB;
            break;
        case MAP_OBJECT_WATERWHEEL:
            if (cell->m_objectIndex == EDIT_SOUND_WATERWHEEL_FRAME_A
                || cell->m_objectIndex == EDIT_SOUND_WATERWHEEL_FRAME_B)
                m_mapSounds[x][y] = MAP_SOUND_WATERWHEEL;
            else if (cell->m_objectIndex == EDIT_SOUND_WATERWHEEL_LOOP_14_FRAME)
                m_mapSounds[x][y] = MAP_SOUND_LOOP_14;
            break;
        case MAP_OBJECT_CAMPFIRE:
            m_mapSounds[x][y] = MAP_SOUND_CAMPFIRE;
            break;
        case MAP_OBJECT_WINDMILL:
            m_mapSounds[x][y] = MAP_SOUND_WINDMILL;
            break;
        case MAP_OBJECT_FOUNTAIN:
            m_mapSounds[x][y] = MAP_SOUND_FOUNTAIN;
            break;
        case MAP_OBJECT_MINE:
            if (cell->m_triggerType & MAP_TRIGGER_EVENT)
                m_mapSounds[x][y] = MAP_SOUND_MINE;
            break;
        case MAP_OBJECT_SAWMILL:
            if (cell->m_objectIndex != MAP_CELL_NO_FRAME)
                m_mapSounds[x][y] = MAP_SOUND_SAWMILL;
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            if (cell->m_triggerType & MAP_TRIGGER_EVENT)
                m_mapSounds[x][y] = MAP_SOUND_DAEMON_CAVE;
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            m_mapSounds[x][y] = MAP_SOUND_SPELL_SHRINE;
            break;
        default:
            switch (cell->m_objectTileset & MAP_CELL_TILESET_MASK) {
                case TILESET_OBJ32_04:
                    if (cell->m_objectIndex == EDIT_SOUND_LAVA_LOOP_5_FRAME)
                        m_mapSounds[x][y] = MAP_SOUND_LOOP_5;
                    else if (cell->m_objectIndex == EDIT_SOUND_LAVA_LOOP_7_FRAME
                             || (cell->m_objectIndex >= 23 && cell->m_objectIndex <= 26))
                        m_mapSounds[x][y] = MAP_SOUND_LOOP_7;
                    else if (cell->m_objectIndex >= 17 && cell->m_objectIndex <= 22)
                        m_mapSounds[x][y] = MAP_SOUND_LOOP_6;
                    break;
                case TILESET_OBJ32_01:
                    if (cell->m_objectIndex >= 30 && cell->m_objectIndex <= 121)
                        m_mapSounds[x][y] = MAP_SOUND_LOOP_13;
                    break;
                case TILESET_OBJ32_07:
                    if (cell->m_objectIndex >= 147 && cell->m_objectIndex <= 167)
                        m_mapSounds[x][y] = MAP_SOUND_LOOP_14;
                    break;
                case TILESET_OBJ32_00:
                    if ((cell->m_objectIndex >= 2 && cell->m_objectIndex <= 5)
                        || cell->m_objectIndex == EDIT_SOUND_WATER_LOOP_19_FRAME_A
                        || cell->m_objectIndex == EDIT_SOUND_WATER_LOOP_19_FRAME_B)
                        m_mapSounds[x][y] = MAP_SOUND_LOOP_19;
                    break;
            }
            break;
    }
}

// Marks the land cells next to a coast tile as coast (where a disembarking
// hero lands); water tiles 4..19 are the coast's four border kinds.
VA(0x0040568f, 0x830)
void editManager::SetCoast(i16 x, i16 y) {
    mapCell* cell;

    cell = &m_map.cells[x][y];
    if (cell->m_tileIndex > MAP_CELL_TILES_PER_TERRAIN - 1)
        return;
    switch (cell->m_tileIndex / EDIT_COAST_TILE_VARIANTS) {
        case EDIT_COAST_OPEN:
            return;
        case EDIT_COAST_EDGE:
            if (cell->m_flags & MAP_CELL_GROUND_FLIP_VERTICAL) {
                if (y < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x][y + 1].m_triggerType)
                    m_map.cells[x][y + 1].m_triggerType = MAP_OBJECT_COAST;
            } else {
                if (y > 0 && !m_map.cells[x][y - 1].m_triggerType)
                    m_map.cells[x][y - 1].m_triggerType = MAP_OBJECT_COAST;
            }
            break;
        case EDIT_COAST_OUTER_CORNER:
            if ((cell->m_flags & (MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL))
                == (MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL)) {
                if (y < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x][y + 1].m_triggerType)
                    m_map.cells[x][y + 1].m_triggerType = MAP_OBJECT_COAST;
                if (x > 0 && !m_map.cells[x - 1][y].m_triggerType)
                    m_map.cells[x - 1][y].m_triggerType = MAP_OBJECT_COAST;
                if (y < MAP_CELL_GRID_SIZE - 1 && x > 0 && !m_map.cells[x - 1][y + 1].m_triggerType)
                    m_map.cells[x - 1][y + 1].m_triggerType = MAP_OBJECT_COAST;
            } else if (cell->m_flags & MAP_CELL_GROUND_FLIP_HORIZONTAL) {
                if (y > 0 && !m_map.cells[x][y - 1].m_triggerType)
                    m_map.cells[x][y - 1].m_triggerType = MAP_OBJECT_COAST;
                if (x > 0 && !m_map.cells[x - 1][y].m_triggerType)
                    m_map.cells[x - 1][y].m_triggerType = MAP_OBJECT_COAST;
                if (y > 0 && x > 0 && !m_map.cells[x - 1][y - 1].m_triggerType)
                    m_map.cells[x - 1][y - 1].m_triggerType = MAP_OBJECT_COAST;
            } else if (cell->m_flags & MAP_CELL_GROUND_FLIP_VERTICAL) {
                if (y < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x][y + 1].m_triggerType)
                    m_map.cells[x][y + 1].m_triggerType = MAP_OBJECT_COAST;
                if (x < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x + 1][y].m_triggerType)
                    m_map.cells[x + 1][y].m_triggerType = MAP_OBJECT_COAST;
                if (y < MAP_CELL_GRID_SIZE - 1 && x < MAP_CELL_GRID_SIZE - 1
                    && !m_map.cells[x + 1][y + 1].m_triggerType)
                    m_map.cells[x + 1][y + 1].m_triggerType = MAP_OBJECT_COAST;
            } else {
                if (y > 0 && !m_map.cells[x][y - 1].m_triggerType)
                    m_map.cells[x][y - 1].m_triggerType = MAP_OBJECT_COAST;
                if (x < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x + 1][y].m_triggerType)
                    m_map.cells[x + 1][y].m_triggerType = MAP_OBJECT_COAST;
                if (y > 0 && x < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x + 1][y - 1].m_triggerType)
                    m_map.cells[x + 1][y - 1].m_triggerType = MAP_OBJECT_COAST;
            }
            break;
        case EDIT_COAST_SIDE:
            if (cell->m_flags & MAP_CELL_GROUND_FLIP_HORIZONTAL) {
                if (x > 0 && !m_map.cells[x - 1][y].m_triggerType)
                    m_map.cells[x - 1][y].m_triggerType = MAP_OBJECT_COAST;
            } else {
                if (x < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x + 1][y].m_triggerType)
                    m_map.cells[x + 1][y].m_triggerType = MAP_OBJECT_COAST;
            }
            break;
        case EDIT_COAST_INNER_CORNER:
            if ((cell->m_flags & (MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL))
                == (MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL)) {
                if (y < MAP_CELL_GRID_SIZE - 1 && x > 0 && !m_map.cells[x - 1][y + 1].m_triggerType)
                    m_map.cells[x - 1][y + 1].m_triggerType = MAP_OBJECT_COAST;
            } else if (cell->m_flags & MAP_CELL_GROUND_FLIP_HORIZONTAL) {
                if (y > 0 && x > 0 && !m_map.cells[x - 1][y - 1].m_triggerType)
                    m_map.cells[x - 1][y - 1].m_triggerType = MAP_OBJECT_COAST;
            } else if (cell->m_flags & MAP_CELL_GROUND_FLIP_VERTICAL) {
                if (y < MAP_CELL_GRID_SIZE - 1 && x < MAP_CELL_GRID_SIZE - 1
                    && !m_map.cells[x + 1][y + 1].m_triggerType)
                    m_map.cells[x + 1][y + 1].m_triggerType = MAP_OBJECT_COAST;
            } else {
                if (y > 0 && x < MAP_CELL_GRID_SIZE - 1 && !m_map.cells[x + 1][y - 1].m_triggerType)
                    m_map.cells[x + 1][y - 1].m_triggerType = MAP_OBJECT_COAST;
            }
            break;
    }
}

// Save checks: stone liths and whirlpools come in pairs; a random town keeps
// its approach clear; every placed hero gets a distinct hero id.
VA(0x00405ebf, 0x28b)
void editManager::CheckObjects(void) {
    i32 whirlpools;
    i32 y;
    i32 notUsed;
    i32 x;
    mapHeroExtra* heroRecord;
    u8 heroIdUsed[GAME_HERO_COUNT];
    i32 lithsSeen;
    i32 placedHeroes;

    lithsSeen = 0;
    whirlpools = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            if ((m_map.cells[x][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_STONE_LITHS)
                lithsSeen++;
            if (x < MAP_CELL_GRID_SIZE - 2 && y < MAP_CELL_GRID_SIZE - 1
                && (m_map.cells[x][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_WHIRLPOOL
                && (m_map.cells[x + 2][y + 1].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       == MAP_OBJECT_WHIRLPOOL)
                whirlpools++;
        }
    }
    if (lithsSeen == 1)
        AddError(localization::Tr("editor.check.stone_liths.single"));
    if (whirlpools == 1)
        AddError(localization::Tr("editor.check.whirlpool.single"));
    placedHeroes = 0;
    memset(heroIdUsed, 0, sizeof(heroIdUsed));
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            if ((m_map.cells[x][y].m_triggerType
                     == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_TOWN)
                 || m_map.cells[x][y].m_triggerType
                        == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE))
                && x > 1 && y > 1)
                ClearArea(x - 2, y - 2, 4, 1, EDIT_CLEAR_ALL, 0);
            if ((m_map.cells[x][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_FILE_OBJECT_HERO) {
                heroRecord =
                    static_cast<mapHeroExtra*>(m_extras[m_map.cells[x][y].m_objectMetadata]);
                if (++placedHeroes > GAME_HERO_COUNT)
                    continue;
                while (heroIdUsed[heroRecord->heroId])
                    heroRecord->heroId = (heroRecord->heroId + 1) % GAME_HERO_COUNT;
                heroIdUsed[heroRecord->heroId] = 1;
            }
        }
    }
}

VA(0x0040614a, 0x210)
void editManager::UpdateTriggers(void) {
    i32 unused;
    i16 x;
    i16 y;
    mapCell* cell;

    cell = NULL;
    unused = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++)
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++)
            if (m_map.cells[x][y].m_triggerType == MAP_OBJECT_COAST)
                m_map.cells[x][y].m_triggerType = MAP_OBJECT_NONE;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            SetCoast(x, y);
            SetCellSound(x, y);
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if ((cell->m_overlayTileset == TILESET_MTN32
                 || cell->m_overlayTileset == TILESET_TREE32)
                && !cell->m_triggerType) {
                if (cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) {
                    cell->m_triggerType = cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK;
                    cell->m_secondaryTrigger -= cell->m_triggerType;
                } else {
                    sprintf(
                        gText,
                        "Data corrupt at    X:%d   Y:%d.  Try clearing the affected area and "
                        "relaying anything you have there.",
                        x,
                        y
                    );
                    AddError(gText);
                }
            }
        }
    }
}

VA(0x0040635a, 0x3e)
u8 editManager::Confirm(char* question) {
    NormalDialog(question, NORMAL_DIALOG_TYPE_YES_NO);
    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
        return 1;
    return 0;
}

VA(0x00406398, 0x70)
i32 editManager::HasObject(i32 trigger) {
    i32 x;
    i32 y;

    for (y = 0; y < MAP_CELL_GRID_SIZE; y++)
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++)
            if (m_map.cells[x][y].m_triggerType == trigger)
                return 1;
    return 0;
}

VA(0x00406408, 0xa5)
i32 editManager::CountArtifacts(void) {
    i32 count;
    i32 x;
    i32 y;

    count = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++)
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++)
            if ((m_map.cells[x][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_ARTIFACT
                || (m_map.cells[x][y].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       == MAP_FILE_OBJECT_RANDOM_ARTIFACT)
                count++;
    return count;
}

VA(0x004064ad, 0xa7)
i32 editManager::CountTowns(void) {
    i16 count;
    i32 x;
    i32 y;
    mapCell* cell;

    count = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_TOWN)
                || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE)
                || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
                count++;
        }
    }
    return count;
}

VA(0x00406554, 0xbb)
i32 editManager::CountMines(void) {
    i32 y;
    i32 x;
    i16 mineTotal;
    mapCell* cell;

    mineTotal = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MINE)
                || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL)
                || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB)
                || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MINE))
                mineTotal++;
        }
    }
    return mineTotal;
}

// The map file's town table: castles first, then towns, three bytes each
// (x, y, faction; castles set bit 7), padded with empty records.
VA(0x0040660f, 0x37e)
void editManager::WriteTowns(i32 file) {
    i16 count;
    i32 setTowns;
    i32 type;
    i32 setCastles;
    i32 x;
    mapCell* spot;
    i32 y;
    i32 notUsed;
    char* townExtra;
    u8 empty[3];
    i16 castleCount;

    count = 0;
    castleCount = 0;
    setCastles = 0;
    setTowns = 0;
    spot = NULL;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            spot = &m_map.cells[x][y];
            if (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE)
                || (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                    && (spot->m_objectIndex == EDIT_CASTLE_ENTRANCE_FRAME
                        || spot->m_objectIndex
                               == EDIT_CASTLE_ENTRANCE_FRAME + TOWN_RACE_FRAME_STRIDE
                        || spot->m_objectIndex
                               == EDIT_CASTLE_ENTRANCE_FRAME + 2 * TOWN_RACE_FRAME_STRIDE
                        || spot->m_objectIndex
                               == EDIT_CASTLE_ENTRANCE_FRAME + 3 * TOWN_RACE_FRAME_STRIDE))) {
                write(file, &x, 1);
                write(file, &y, 1);
                type = spot->m_objectIndex / TOWN_RACE_FRAME_STRIDE | MAP_TOWN_CASTLE_FLAG;
                write(file, &type, 1);
                count++;
                if (spot->m_objectMetadata) {
                    townExtra = static_cast<char*>(gEditManager->m_extras[spot->m_objectMetadata]);
                    if (*townExtra)
                        setCastles++;
                }
            }
        }
    }
    castleCount = count;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            spot = &m_map.cells[x][y];
            if (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_TOWN)
                || (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                    && (spot->m_objectIndex == EDIT_CASTLE_ENTRANCE_FRAME - TOWN_CASTLE_FRAME_OFFSET
                        || spot->m_objectIndex
                               == EDIT_CASTLE_ENTRANCE_FRAME - TOWN_CASTLE_FRAME_OFFSET
                                      + TOWN_RACE_FRAME_STRIDE
                        || spot->m_objectIndex
                               == EDIT_CASTLE_ENTRANCE_FRAME - TOWN_CASTLE_FRAME_OFFSET
                                      + 2 * TOWN_RACE_FRAME_STRIDE
                        || spot->m_objectIndex
                               == EDIT_CASTLE_ENTRANCE_FRAME - TOWN_CASTLE_FRAME_OFFSET
                                      + 3 * TOWN_RACE_FRAME_STRIDE))) {
                write(file, &x, 1);
                write(file, &y, 1);
                type = spot->m_objectIndex / TOWN_RACE_FRAME_STRIDE;
                write(file, &type, 1);
                sprintf(gText, "Town %02d: (%02d,%02d) type: %02d\n", count, x, y, type);
                count++;
                if (spot->m_objectMetadata) {
                    townExtra = static_cast<char*>(gEditManager->m_extras[spot->m_objectMetadata]);
                    if (*townExtra)
                        setTowns++;
                }
            }
        }
    }
    if (castleCount < 4)
        AddError(localization::Tr("editor.check.castles.few"));
    else if (setCastles >= 1 && setCastles < castleCount)
        AddError(localization::Tr("editor.check.castles.unset"));
    else if (setTowns > 0 && setCastles < castleCount)
        AddError(localization::Tr("editor.check.castles.mixed"));
    empty[0] = 0xff;
    empty[1] = 0xff;
    empty[2] = 0;
    for (x = 0; x < GAME_TOWN_COUNT - count; x++)
        write(file, empty, 3);
}

// The map file's mine table: the dragon city and the lighthouse take the
// first two records, then every mine, sawmill and alchemist lab.
VA(0x0040698d, 0x41f)
void editManager::WriteMines(i32 file) {
    u8 type;
    i32 beaconCount;
    u8 cityX;
    u8 cityY;
    u8 x;
    mapCell* spot;
    u8 y;
    u8 empty[3];
    u8 beaconX;
    i16 mineNumber;
    u8 beaconY;
    i32 numCities;
    mapCell* neighbour;

    cityX = 0xff;
    cityY = 0xff;
    beaconX = 0xff;
    beaconY = 0xff;
    beaconCount = 0;
    numCities = 0;
    spot = NULL;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            spot = &m_map.cells[x][y];
            if (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_DRAGON_CITY)) {
                numCities++;
                cityX = x;
                cityY = y;
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            spot = &m_map.cells[x][y];
            if (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_LIGHTHOUSE)) {
                beaconCount++;
                beaconX = x;
                beaconY = y;
            }
        }
    }
    empty[0] = 0xff;
    empty[1] = 0xff;
    empty[2] = 0xff;
    if (cityX != -1) {
        type = MAP_OBJECT_DRAGON_CITY;
        write(file, &cityX, 1);
        write(file, &cityY, 1);
        write(file, &type, 1);
    } else {
        write(file, empty, 3);
    }
    if (beaconX != -1) {
        type = MAP_OBJECT_LIGHTHOUSE;
        write(file, &beaconX, 1);
        write(file, &beaconY, 1);
        write(file, &type, 1);
    } else {
        write(file, empty, 3);
    }
    if (beaconCount > 1)
        AddError(localization::Tr("editor.check.lighthouse.multiple"));
    if (numCities > 1)
        AddError(localization::Tr("editor.check.dragon_city.multiple"));
    mineNumber = 2;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            spot = &m_map.cells[x][y];
            if (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MINE)
                || spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL)
                || spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB)
                || spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MINE)) {
                if (spot->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MINE)) {
                    type = 0xff;
                } else {
                    neighbour = &m_map.cells[x + 1][y];
                    if (neighbour->m_flags & MAP_CELL_OBJECT_EXTRA)
                        type = neighbour->m_extraFrame + 2;
                    else if (spot->m_objectIndex == EDIT_SAWMILL_FRAME)
                        type = 0;
                    else
                        type = 1;
                }
                write(file, &x, 1);
                write(file, &y, 1);
                write(file, &type, 1);
                sprintf(gText, "Mine %02d: (%02d,%02d) type: %02d\n", mineNumber, x, y, type);
                mineNumber++;
            }
        }
    }
    for (x = 0; x < GAME_MINE_COUNT - mineNumber; x++)
        write(file, empty, 3);
}

// The random-artifact table: each artifact on the map is marked placed.
VA(0x00406dac, 0xba)
void editManager::WriteArtifacts(i32 file) {
    u8 x;
    u8 y;
    mapCell* cell;
    i8 artifactHolders[EDIT_MAP_ARTIFACT_SLOTS];

    memset(artifactHolders, -1, sizeof(artifactHolders));
    cell = NULL;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT))
                artifactHolders[cell->m_objectIndex] = GAME_ARTIFACT_ON_MAP;
        }
    }
    write(file, artifactHolders, sizeof(artifactHolders));
}

VA(0x00406e66, 0xf5)
void editManager::WriteObelisks(i32 file) {
    u8 obeliskCount;
    u8 x;
    u8 y;
    mapCell* cell;

    obeliskCount = 0;
    cell = NULL;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_OBELISK))
                obeliskCount++;
        }
    }
    if (!obeliskCount)
        obeliskCount++;
    write(file, &obeliskCount, 1);
    if (obeliskCount > EDIT_MAP_OBELISK_LIMIT) {
        sprintf(gText, localization::Tr("editor.check.obelisks.many"), obeliskCount);
        AddError(gText);
    }
}

// Writes the map in the game's .MAP layout (game::LoadMap) after the save
// checks, then shows the checks' messages. Maps in the old format keep the
// header without its format word and no object owners.
VA(0x00406f5b, 0x25c)
i16 editManager::SaveMap(char* name) {
    char fileName[40];
    i16 width;
    i16 data;
    i16 mapHeight;
    i32 i;
    i32 handle;
    i16 formatWord;

    ClearErrors();
    gMouseManager->SetPointer(1);
    CheckObjects();
    UpdateTriggers();
    sprintf(fileName, ".\\maps\\%s", name);
    handle = open(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (handle == -1)
        return BASE_MANAGER_ERROR;
    data = MAP_HEADER_ID;
    write(handle, &data, sizeof(data));
    if (gNewMapFormat) {
        write(handle, &gEditMapHeader.difficulty, sizeof(gEditMapHeader) - 2 * sizeof(i16));
        formatWord = MAP_HEADER_ID;
        write(handle, &formatWord, sizeof(formatWord));
    } else {
        write(handle, &gEditMapHeader, sizeof(gEditMapHeader) - sizeof(i16));
    }
    data = EDIT_MAP_VERSION;
    write(handle, &data, sizeof(data));
    data = MAP_CELL_GRID_SIZE;
    write(handle, &data, sizeof(data));
    data = MAP_CELL_GRID_SIZE;
    write(handle, &data, sizeof(data));
    write(handle, m_map.cells, sizeof(m_map.cells));
    WriteTowns(handle);
    WriteMines(handle);
    WriteArtifacts(handle);
    WriteObelisks(handle);
    write(handle, m_mapSounds, sizeof(m_mapSounds));
    write(handle, &m_extraCount, sizeof(m_extraCount));
    for (i = MAP_EXTRA_FIRST_RECORD; i < m_extraCount; i++) {
        write(handle, &m_extraSizes[i], sizeof(m_extraSizes[i]));
        write(handle, m_extras[i], m_extraSizes[i]);
    }
    if (gNewMapFormat) {
        write(handle, m_map.cellPairs, sizeof(m_map.cellPairs));
        write(handle, &gNextObjectId, sizeof(gNextObjectId));
    }
    close(handle);
    gMouseManager->SetPointer(0);
    ShowErrors();
    return BASE_MANAGER_SUCCESS;
}

// Reads a .MAP: the cells and map extras (the town, mine, artifact, obelisk
// and sound tables are rebuilt on save) and, from the new format on, the
// object owners.
VA(0x004071b7, 0x382)
i16 editManager::LoadMap(char* name) {
    char fileName[40];
    u8 ignored[5500];
    i16 width;
    i16 mapFormat;
    i32 unused2;
    i32 unused;
    i32 i;
    i32 handle;
    i16 headerId;
    i16 height;

    FreeMapExtras();
    sprintf(fileName, ".\\maps\\%s", name);
    handle = open(fileName, O_BINARY);
    if (handle == -1)
        return BASE_MANAGER_ERROR;
    read(handle, &headerId, sizeof(headerId));
    if (headerId == MAP_HEADER_ID) {
        gEditMapHeader.id = headerId;
        read(handle, &gEditMapHeader.difficulty, sizeof(gEditMapHeader) - sizeof(i16));
        read(handle, &headerId, sizeof(headerId));
    } else {
        NewMap(0);
    }
    mapFormat = gEditMapHeader.format;
    if (mapFormat >= MAP_HEADER_ID && mapFormat <= MAP_HEADER_ID + 10)
        gNewMapFormat = 1;
    else
        gNewMapFormat = 0;
    gMouseManager->SetPointer(1);
    read(handle, &width, sizeof(width));
    read(handle, &height, sizeof(height));
    read(handle, m_map.cells, sizeof(m_map.cells));
    read(handle, ignored, GAME_TOWN_COUNT * 3);
    read(handle, ignored, GAME_MINE_COUNT * 3);
    read(handle, ignored, EDIT_MAP_ARTIFACT_SLOTS);
    read(handle, ignored, 1);
    read(handle, ignored, sizeof(m_mapSounds));
    if (headerId == EDIT_MAP_VERSION) {
        read(handle, &m_extraCount, sizeof(m_extraCount));
        for (i = MAP_EXTRA_FIRST_RECORD; i < m_extraCount; i++) {
            read(handle, &m_extraSizes[i], sizeof(m_extraSizes[i]));
            m_extras[i] = malloc(m_extraSizes[i]);
            read(handle, m_extras[i], m_extraSizes[i]);
        }
    } else {
        m_extraCount = MAP_EXTRA_FIRST_RECORD;
    }
    if (gNewMapFormat) {
        read(handle, m_map.cellPairs, sizeof(m_map.cellPairs));
        read(handle, &gNextObjectId, sizeof(gNextObjectId));
    }
    close(handle);
    gMouseManager->SetPointer(0);
    gEditManager->SaveUndo();
    if (!gNewMapFormat)
        NormalDialog(localization::Tr("editor.map.old_format"), NORMAL_DIALOG_TYPE_OK);
    return BASE_MANAGER_SUCCESS;
}

VA(0x00407539, 0x12e)
i16 editManager::PickMap(char*, char*, i16 mode) {
    fileRequester* requester;
    i32 picked;
    i16 dialogResult;

    picked = 0;
    requester = new fileRequester(160, 40, mode, "*.MAP", ".\\MAPS\\", ".MAP");
    dialogResult = gExec->DoDialog(requester);
    if (dialogResult == FILE_REQUESTER_OK) {
        picked = 1;
        strcpy(m_mapFileName, gLastFilename);
    }
    delete requester;
    return picked;
}

VA(0x00407667, 0x45)
void editManager::ClearErrors(void) {
    for (; gEditErrorCount > 0; gEditErrorCount--)
        free(gEditErrors[gEditErrorCount - 1]);
    gEditErrorCount = 0;
}

VA(0x004076ac, 0x90)
void editManager::ShowErrors(void) {
    i32 oldDebugLevel;
    i32 i;

    oldDebugLevel = gDebugLevel;
    if (gEditErrorCount > 0) {
        gDebugLevel = oldDebugLevel;
        for (i = 0; i < gEditErrorCount; i++) {
            sprintf(gText, localization::Tr("editor.check.next"), gEditErrors[i]);
            NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                break;
        }
    }
}

VA(0x0040773c, 0x80)
void editManager::AddError(char* text) {
    i32 length;

    if (gEditErrorCount < EDIT_MANAGER_ERROR_CAPACITY) {
        length = strlen(text) + 1;
        gEditErrors[gEditErrorCount] = static_cast<char*>(malloc(length));
        strcpy(gEditErrors[gEditErrorCount], text);
        gEditErrorCount++;
    }
}

// Whether ClearArea's mask selects an object: the map's treasure (resource
// piles and the chest, campfire and lamp of obj32-07) by its category bit,
// towns, monsters and artifacts by theirs, any other object by the bit of
// the terrain under it.
VA(0x004077bc, 0x129)
i32 editManager::IsCleared(i32 tileset, i32 index, i32 mask, i32 x, i32 y) {
    if (tileset <= TILESET_TERRAIN_OBJECT_LAST || tileset == TILESET_RSRC32) {
        if ((tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_WOOD)
            || (tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_MERCURY)
            || (tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_ORE)
            || (tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_SULFUR)
            || (tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_CRYSTAL)
            || (tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_GEMS)
            || (tileset == TILESET_RSRC32 && index == RESOURCE_PILE_OBJECT_BASE + RESOURCE_GOLD)
            || (tileset == TILESET_RSRC32 && index == EDIT_TREASURE_CHEST_FRAME)
            || (tileset == TILESET_OBJ32_07 && index == EDIT_TREASURE_OBJECT_FRAME_A)
            || (tileset == TILESET_OBJ32_07 && index == EDIT_TREASURE_OBJECT_FRAME_C)
            || (tileset == TILESET_OBJ32_07 && index == EDIT_TREASURE_OBJECT_FRAME_B))
            return mask & EDIT_CLEAR_TREASURE;
        return mask & (1 << (m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN));
    }
    if (tileset == TILESET_TOWN32)
        return mask & EDIT_CLEAR_TOWNS;
    if (tileset == TILESET_MONS32)
        return mask & EDIT_CLEAR_MONSTERS;
    if (tileset == TILESET_ART32)
        return mask & EDIT_CLEAR_ARTIFACTS;
#line 2609 EDITMGR_CPP_PATH
    H1_ASSERT(0);
    return 0;
}

VA(0x004078e5, 0x504)
void editManager::ClearArea(i32 x, i32 y, i32 width, i32 height, u16 mask, i32 secondLayer) {
    i32 i;
    editMapCellPair* ownerRec;
    u16 tag;
    i32 j;
    i32 ox;
    i32 pass;
    i32 oy;
    mapCell* thatCell;
    i32 blankSlot;

#line 2619 EDITMGR_CPP_PATH
    H1_ASSERT(
        x >= 0 && x + width - 1 < MAP_CELL_GRID_SIZE && y >= 0
        && y + height - 1 < MAP_CELL_GRID_SIZE
    );
    for (i = x; i < x + width; i++) {
        for (j = y; j < y + height; j++) {
            for (pass = 0; pass < 2; pass++) {
                if (pass == 1 && !secondLayer)
                    continue;
                if ((pass == 0
                     && IsCleared(
                         m_map.cells[i][j].m_objectTileset & MAP_CELL_TILESET_MASK,
                         m_map.cells[i][j].m_objectIndex,
                         mask,
                         i,
                         j
                     ))
                    || (pass == 1
                        && IsCleared(
                            m_map.cells[i][j].m_overlayTileset & MAP_CELL_TILESET_MASK,
                            m_map.cells[i][j].m_overlayIndex,
                            mask,
                            i,
                            j
                        ))) {
                    if (gNewMapFormat) {
                        if ((pass == 0 && m_map.cellPairs[i][j].objectId)
                            || (pass == 1 && m_map.cellPairs[i][j].overlayId)) {
                            if (pass == 0)
                                tag = m_map.cellPairs[i][j].objectId;
                            else
                                tag = m_map.cellPairs[i][j].overlayId;
                            for (ox = 0; ox < MAP_CELL_GRID_SIZE; ox++) {
                                for (oy = 0; oy < MAP_CELL_GRID_SIZE; oy++) {
                                    thatCell = &m_map.cells[ox][oy];
                                    ownerRec = &m_map.cellPairs[ox][oy];
                                    if (ownerRec->objectId == tag) {
                                        ownerRec->objectId = 0;
                                        thatCell->m_objectTileset = 0;
                                        thatCell->m_objectIndex = MAP_CELL_NO_FRAME;
                                        thatCell->m_triggerType = MAP_OBJECT_NONE;
                                        thatCell->m_flags &=
                                            ~(MAP_CELL_OBJECT_ANIMATED | MAP_CELL_OBJECT_EXTRA
                                              | MAP_CELL_OBJECT_SHADOW_ONLY);
                                        if (thatCell->m_secondaryTrigger
                                            & MAP_CELL_SECONDARY_BLOCKED)
                                            thatCell->m_secondaryTrigger -=
                                                MAP_CELL_SECONDARY_BLOCKED;
                                        thatCell->m_triggerType = thatCell->m_secondaryTrigger;
                                        thatCell->m_secondaryTrigger = 0;
                                        thatCell->m_objectMetadata = 0;
                                        thatCell->m_extraFrame = 0;
                                    }
                                    if (ownerRec->overlayId == tag) {
                                        ownerRec->overlayId = 0;
                                        thatCell->m_overlayTileset = 0;
                                        thatCell->m_overlayIndex = MAP_CELL_NO_FRAME;
                                        thatCell->m_flags &=
                                            ~(MAP_CELL_OVERLAY_ANIMATED | MAP_CELL_OVERLAY_EXTRA);
                                        thatCell->m_secondaryTrigger &= MAP_CELL_SECONDARY_BLOCKED;
                                        if (thatCell->m_objectIndex == MAP_CELL_NO_FRAME)
                                            thatCell->m_triggerType = MAP_OBJECT_NONE;
                                    }
                                }
                            }
                        }
                    } else {
                        m_map.cells[i][j].m_objectTileset = 0;
                        m_map.cells[i][j].m_objectIndex = MAP_CELL_NO_FRAME;
                        m_map.cells[i][j].m_overlayTileset = 0;
                        m_map.cells[i][j].m_overlayIndex = MAP_CELL_NO_FRAME;
                        m_map.cells[i][j].m_triggerType &=
                            MAP_CELL_GROUND_FLIP_VERTICAL | MAP_CELL_GROUND_FLIP_HORIZONTAL;
                        m_map.cells[i][j].m_extraFrame = 0;
                        m_map.cells[i][j].m_flags = 0;
                        m_map.cells[i][j].m_secondaryTrigger = 0;
                        m_map.cells[i][j].m_triggerType = MAP_OBJECT_NONE;
                        m_map.cells[i][j].m_objectMetadata = 0;
                    }
                }
            }
        }
    }
}

VA(0x00407de9, 0x1e6)
void editManager::ResetArea(i32 x, i32 y, i32 width, i32 height) {
    i32 i;
    i32 j;

    for (i = x; i < x + width; i++) {
        for (j = y; j < y + height; j++) {
            m_map.cellPairs[i][j].objectId = 0;
            m_map.cellPairs[i][j].overlayId = 0;
            m_map.cells[i][j].m_tileIndex = Random(0, 3);
            m_map.cells[i][j].m_objectTileset = 0;
            m_map.cells[i][j].m_objectIndex = MAP_CELL_NO_FRAME;
            m_map.cells[i][j].m_overlayTileset = 0;
            m_map.cells[i][j].m_overlayIndex = MAP_CELL_NO_FRAME;
            m_map.cells[i][j].m_triggerType = MAP_OBJECT_NONE;
            m_map.cells[i][j].m_extraFrame = 0;
            m_map.cells[i][j].m_flags = 0;
            m_map.cells[i][j].m_secondaryTrigger = 0;
            m_map.cells[i][j].m_triggerType = MAP_OBJECT_NONE;
            m_map.cells[i][j].m_objectMetadata = 0;
        }
    }
}

// The new map's file name: a letter code of the map serial ("V" to "Z" and
// three letters), then "1234.MAP".
VA(0x00407fcf, 0xdb)
char* MakeMapCode(i32 serial) {
    DATA(0x00451210)
    static char code[5];
    i32 unused;

    memset(code, 0, sizeof(code));
    code[3] = gMapCodeLetters[serial % 26];
    serial -= serial % 26;
    serial /= 26;
    code[2] = gMapCodeLetters[serial % 26];
    serial -= serial % 26;
    serial /= 26;
    code[1] = gMapCodeLetters[serial % 26];
    serial -= serial % 26;
    serial /= 26;
    code[0] = serial % 5 + 'V';
    return code;
}

VA(0x004080aa, 0x52)
void editManager::FreeMapExtras(void) {
    i32 i;

    for (i = MAP_EXTRA_FIRST_RECORD; i < m_extraCount; i++)
        free(m_extras[i]);
    m_extraCount = MAP_EXTRA_FIRST_RECORD;
}

// A new map is a normal-difficulty medium map with placeholder names and
// descriptions, named after the next map serial.
VA(0x004080fc, 0x12f)
void editManager::NewMap(i32 random) {
    i32 i;

    gMapHeader->size = MAP_SIZE_MEDIUM;
    gMapHeader->difficulty = MAP_DIFFICULTY_NORMAL;
    gNewMapFormat = 1;
    gConfig.currentMapOffset++;
    if (gConfig.firstMapOffset + gConfig.currentMapOffset > 65000)
        gConfig.currentMapOffset = 0;
    for (i = 0; i < MAP_HEADER_LANGUAGE_COUNT; i++) {
        if (random)
            sprintf(
                gMapHeader->name[i],
                localization::Tr("editor.map.random.name"),
                gConfig.currentMapOffset % 1000
            );
        else
            sprintf(
                gMapHeader->name[i],
                localization::Tr("editor.map.unnamed"),
                gConfig.currentMapOffset % 1000
            );
        sprintf(gMapHeader->description[i], localization::Tr("editor.map.no_description"));
    }
    sprintf(
        m_mapFileName,
        "%s1234.MAP",
        MakeMapCode(gConfig.firstMapOffset + gConfig.currentMapOffset)
    );
    WritePrefs();
    FreeMapExtras();
}

// Shows text on the status bar with a beep for one and a half seconds.
VA(0x0040822b, 0x28)
void ShowStatusWarning(char* text) {
    ShowStatusText(text);
    MessageBeep(0);
    gStatusTextClearTime = KBTickCount() + 1500;
}

// Scatters small terrain details over 3% of the empty plain ground cells.
VA(0x00408253, 0x209)
void ScatterDetails(void) {
    editMapCellPair* cellOwner;
    i32 x;
    i32 y;
    mapCell* cell;

    gEditManager->SaveUndo();
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = &gEditManager->m_map.cells[x][y];
            cellOwner = &gEditManager->m_map.cellPairs[x][y];
            if (cell->m_objectIndex == MAP_CELL_NO_FRAME
                && cell->m_overlayIndex == MAP_CELL_NO_FRAME
                && cell->m_tileIndex % MAP_CELL_TILES_PER_TERRAIN < 4 && Random(1, 100) <= 3) {
                gNextObjectId++;
                cellOwner->objectId = gNextObjectId;
                switch (cell->m_tileIndex / MAP_CELL_TILES_PER_TERRAIN) {
                    case TERRAIN_GRASS:
                        cell->m_objectTileset = TILESET_OBJ32_01;
                        cell->m_objectIndex = Random(0, 9);
                        cell->m_triggerType = MAP_OBJECT_SHADOW;
                        break;
                    case TERRAIN_SNOW:
                        cell->m_objectTileset = TILESET_OBJ32_02;
                        cell->m_objectIndex = Random(0, 2);
                        cell->m_triggerType = MAP_OBJECT_SHADOW;
                        break;
                    case TERRAIN_SWAMP:
                        cell->m_objectTileset = TILESET_OBJ32_03;
                        cell->m_objectIndex = Random(0, 1);
                        cell->m_triggerType = MAP_OBJECT_SHADOW;
                        break;
                    case TERRAIN_LAVA:
                        cell->m_objectTileset = TILESET_OBJ32_04;
                        cell->m_objectIndex = 0;
                        cell->m_triggerType = MAP_OBJECT_SHADOW;
                        break;
                    case TERRAIN_DESERT:
                        cell->m_objectTileset = TILESET_OBJ32_05;
                        cell->m_objectIndex = 2;
                        cell->m_triggerType = MAP_OBJECT_SHADOW;
                        break;
                    case TERRAIN_DIRT:
                        cell->m_objectTileset = TILESET_OBJ32_06;
                        cell->m_objectIndex = Random(0, 10);
                        cell->m_triggerType = MAP_OBJECT_SHADOW;
                        break;
                }
            }
        }
    }
}
