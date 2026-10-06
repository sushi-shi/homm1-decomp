#ifndef HOMM1_EDITOR_EDITMANAGER_H
#define HOMM1_EDITOR_EDITMANAGER_H

// The scenario editor's main manager (Editor\EDITMGR.CPP). EDITOR.CPP
// allocates one (InitMainClasses: 0x2546d bytes) and runs it as the only
// executive manager. The class name is descriptive: the retail strings give
// none. Members are recovered from the constructor and the editor units read
// so far; spans no recovered code reads yet are m_unknown<offset>.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/mapCell.h>

class font;
class heroWindow;
class icon;
struct tag_message;

H1_ENUM_CONST_BEGIN(EditManagerConstant)
    EDIT_MANAGER_NO_TOOL = -1,
    // The editor edits one map of this many cells per side.
    EDIT_MANAGER_MAP_SIZE = 72,
    // Open loads this many object icon sets (the adventure MapTileset order).
    EDIT_MANAGER_OBJECT_ICON_SETS = 21,
    // The map-extra table (index 0 unused; the constructor starts the count
    // at 1).
    EDIT_MANAGER_EXTRA_CAPACITY = 255
H1_ENUM_CONST_END(EditManagerConstant)

// The map view: 14 cells of 32 pixels, or zoomed out 28 cells of 16 pixels,
// drawn from screen (16, 16).
H1_ENUM_CONST_BEGIN(EditManagerViewConstant)
    EDIT_MANAGER_VIEW_ORIGIN = 16,
    EDIT_MANAGER_VIEW_CELLS = 14,
    EDIT_MANAGER_ZOOMED_OUT_VIEW_CELLS = 28,
    EDIT_MANAGER_CELL_PIXELS = 32,
    EDIT_MANAGER_ZOOMED_OUT_CELL_PIXELS = 16
H1_ENUM_CONST_END(EditManagerViewConstant)

// Widget ids of the editor's main window the tool managers handle.
H1_ENUM_BEGIN(EditorWidgetId)
    // The map view.
    EDITOR_MAP_WIDGET = 9,
    // A tool panel's options button.
    EDITOR_TOOL_OPTIONS_BUTTON = 0x70
H1_ENUM_END(EditorWidgetId)

#pragma pack(push, 1)
// The per-cell ids of the placed objects whose frames the cell shows on its
// object and overlay layers (PlaceOverlay numbers each placed
// object); the map reset clears them.
struct editMapCellPair {
    i16 objectId;
    i16 overlayId;
};

class editManager : public baseManager {
public:
    // The selected tool (-1 none); SelectTool replaces the tool's manager.
    i16 m_tool;
    u8 m_unknown32[4];
    // The bottom bar's status text is drawn with this font.
    font* m_statusFont;
    u8 m_unknown3a[0x10];
    // Each object tileset's 32- and 16-pixel icons (index: m_zoomedOut).
    icon* m_objectIcons[EDIT_MANAGER_OBJECT_ICON_SETS][2];
    u8 m_unknownf2[0x10];
    // The map view shows 16-pixel cells.
    u8 m_zoomedOut;
    // clearManager sets it after changing the map.
    i16 m_mapChanged;
    // The map cell of the last placed object (-1: none); the tools reset it
    // when they open.
    i16 m_placedX;
    i16 m_placedY;
    i16 m_unknown109;
    // The widget id of the last tool command (-1 none).
    i16 m_lastCommandId;
    // The animated frames' current offset (added to frame + 1).
    i16 m_animationFrame;
    // The executive manager of the selected tool.
    baseManager* m_toolManager;
    heroWindow* m_window;
    // The edited map, indexed [x][y].
    mapCell m_cells[EDIT_MANAGER_MAP_SIZE][EDIT_MANAGER_MAP_SIZE];
    editMapCellPair m_cellPairs[EDIT_MANAGER_MAP_SIZE][EDIT_MANAGER_MAP_SIZE];
    u8 m_unknown11c97[0x11b80];
    // The map-extra records (towns, heroes) cells name by m_objectMetadata.
    i32 m_extraCount;
    i32 m_extraSizes[EDIT_MANAGER_EXTRA_CAPACITY];
    void* m_extras[EDIT_MANAGER_EXTRA_CAPACITY];
    u8 m_unknown24013[0x1440];
    // The map cell shown at the view's top-left corner.
    i16 m_viewX;
    i16 m_viewY;
    // The map cell under the cursor.
    i16 m_cursorX;
    i16 m_cursorY;
    u8 m_unknown2545b[0x10];
    i16 m_dispatchMask;

    editManager(void);
    void SelectTool(i16 tool);
    // Copies the map into the undo buffer.
    void SaveUndo(void);
    // Copies the map view's 448x448 square to the screen.
    void UpdateMapView(void);
    // Redraws the cursor cell and its coordinates.
    void UpdateCursor(void);
    // Turns screen coordinates into the view cell under them (clamped).
    void ScreenToCell(i16& x, i16& y);
    // Redraws the map view at the current view origin.
    void DrawMap(void);
    // Redraws the radar map (and copies it to the screen when update is set).
    void DrawRadar(i32 update);
    // Erases the object classes in mask from the width x height cells at
    // (x, y): each cell's first layer, and its second when secondLayer is set.
    void ClearArea(i32 x, i32 y, i32 width, i32 height, u16 mask, i32 secondLayer);
    // Sets the ground of the width x height view cells at (x, y) to the
    // terrain's plain tile and redraws them.
    void PaintGround(i16 x, i16 y, i16 width, i16 height, i16 terrain);
    // The same for map cells, filling the rectangle.
    void FillGround(i16 x, i16 y, i16 width, i16 height, i16 terrain);
    // Fits the terrain's edge tiles to their neighbours over the whole map.
    void BlendTerrain(i16 terrain, u8, u8 restoreOthers, u8 secondPass, u8 skipFirstPass);
    // 1 when a cell's trigger byte equals trigger.
    i32 HasObject(i32 trigger);
    // The placed artifacts (including random ones).
    i32 CountArtifacts(void);
    // The placed towns and castles (including random ones).
    i32 CountTowns(void);
    // The placed mines, sawmills and alchemist's labs.
    i32 CountMines(void);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(tag_message& message) OVERRIDE;
};
#pragma pack(pop)

extern editManager* gEditManager;

// Shows text in the status bar with a beep and clears it after 1.5 seconds.
void ShowStatusWarning(char* text);

#endif // HOMM1_EDITOR_EDITMANAGER_H
