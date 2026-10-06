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
struct tag_message;

H1_ENUM_CONST_BEGIN(EditManagerConstant)
    EDIT_MANAGER_NO_TOOL = -1,
    // The editor edits one map of this many cells per side.
    EDIT_MANAGER_MAP_SIZE = 72,
    // m_mapFileName: an 8.3 map file name.
    EDIT_MAP_FILE_NAME_SIZE = 16
H1_ENUM_CONST_END(EditManagerConstant)

// Widget ids of the editor's main window the tool managers handle.
H1_ENUM_BEGIN(EditorWidgetId)
    // The map view.
    EDITOR_MAP_WIDGET = 9,
    // A tool panel's options button.
    EDITOR_TOOL_OPTIONS_BUTTON = 0x70
H1_ENUM_END(EditorWidgetId)

#pragma pack(push, 1)
// A per-cell word pair the map reset clears.
struct editMapCellPair {
    i16 first;
    i16 second;
};

class editManager : public baseManager {
public:
    // The selected tool (-1 none); SelectTool replaces the tool's manager.
    i16 m_tool;
    u8 m_unknown32[4];
    // The bottom bar's status text is drawn with this font.
    font* m_statusFont;
    u8 m_unknown3a[0xc8];
    i8 m_unknown102;
    // clearManager sets it after changing the map.
    i16 m_mapChanged;
    // The map cell of the last placed object (-1: none); the tools reset it
    // when they open.
    i16 m_placedX;
    i16 m_placedY;
    i16 m_unknown109;
    // The widget id of the last tool command (-1 none).
    i16 m_lastCommandId;
    i16 m_unknown10d;
    // The executive manager of the selected tool.
    baseManager* m_toolManager;
    heroWindow* m_window;
    // The edited map: the game's ten-byte cells (the random map generator
    // reads the tile, object, overlay and trigger bytes at the game's
    // offsets).
    mapCell m_cells[EDIT_MANAGER_MAP_SIZE][EDIT_MANAGER_MAP_SIZE];
    editMapCellPair m_cellPairs[EDIT_MANAGER_MAP_SIZE][EDIT_MANAGER_MAP_SIZE];
    u8 m_unknown11c97[0x11b80];
    i32 m_unknown23817;
    u8 m_unknown2381b[0x1c38];
    // The map cell shown at the view's top-left corner.
    i16 m_viewX;
    i16 m_viewY;
    // The map cell under the cursor.
    i16 m_cursorX;
    i16 m_cursorY;
    // The map's file name (SaveMap's argument).
    char m_mapFileName[EDIT_MAP_FILE_NAME_SIZE];
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
    i16 SaveMap(char* name);
    // Clears the map's objects and ground in the width x height cells at
    // (x, y).
    void ResetArea(i32 x, i32 y, i32 width, i32 height);
    // Starts a new map (random: for the generator).
    void NewMap(i32 random);
    // The random map generator (src/EDITOR/MAPOBJ.cpp).
    void GenerateRandomMap(void);
    // At least four castles stand on the map.
    i32 HasEnoughCastles(void);
    // Grows `percent` of the map's cells of terrain from random seeds over
    // cells of baseTerrain (100: the whole map).
    void PaintRandomTerrain(i32 terrain, i32 percent, i32 baseTerrain);
    // Merges terrain regions of at most fifteen cells into a neighbour and
    // counts the land cells.
    void RemoveSmallRegions(void);
    // Lays chains of the tileset's mountains or trees.
    void PlaceObstacleChains(i32 density, i32 tileset);
    // Places one chain link at (*x, *y) facing `direction` and steps on.
    i32 PlaceChainLink(i32* x, i32* y, i32 direction, i32 tileset, char kind);
    void PlaceTowns(void);
    // Places a sawmill (kind 0), an alchemist's lab (1) or the mine of
    // resource `kind` with its river at (x, y).
    void PlaceResourceSite(i32 x, i32 y, i32 kind);
    void PlaceRandomObjects(i32 density, i32 strength);
    void PlaceTreasures(i32 density, i32 strength);
    // Scatters decorative objects over bare ground.
    void ScatterDecorations(void);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(tag_message& message) OVERRIDE;
};
#pragma pack(pop)

extern editManager* gEditManager;
// Set while the generator blends terrain: every border tile's variant is
// re-rolled (EDITMGR).
extern i32 gVaryTiles;

// Scales a generator count by a 0..100 density setting (50: unchanged apart
// from the size bonus).
void ScaleByDensity(i32* count, i32 density);

#endif // HOMM1_EDITOR_EDITMANAGER_H
