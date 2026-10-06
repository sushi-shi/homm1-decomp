#ifndef HOMM1_EDITOR_EDITMANAGER_H
#define HOMM1_EDITOR_EDITMANAGER_H

// The scenario editor's main manager (Editor\EDITMGR.CPP). EDITOR.CPP
// allocates one (InitMainClasses: 0x2546d bytes) and runs it as the only
// executive manager; Open stores the class name "editManager". It owns the
// edited map, its undo copy and the map-extra records, draws the map view,
// the radar and the coordinate rulers, and runs the tool managers.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/KB.h>
#include <SOURCE/mapCell.h>

class font;
class heroWindow;
class icon;
class iconWidget;
class tileset;
struct tag_message;

H1_ENUM_CONST_BEGIN(EditManagerConstant)
    EDIT_MANAGER_NO_TOOL = -1,
    // Main tests message.type against this mask (key, mouse and widget
    // messages).
    EDIT_MANAGER_DISPATCH_MASK = 0x32f,
    // m_objectIcons: the adventure tileset slots (MapTileset), each loaded at
    // both zoom levels.
    EDIT_MANAGER_TILESET_COUNT = 21,
    EDIT_MANAGER_ZOOM_COUNT = 2,
    // m_mapFileName: an 8.3 map file name.
    EDIT_MAP_FILE_NAME_SIZE = 16,
    // gEditErrors holds at most this many save-check messages.
    EDIT_MANAGER_ERROR_CAPACITY = 100,
    // The map file's random-artifact table (game::m_randomArtifacts).
    EDIT_MAP_ARTIFACT_SLOTS = 37,
    // The save check allows at most this many obelisks.
    EDIT_MAP_OBELISK_LIMIT = 48,
    // NewMap fills this many of the header's names and descriptions.
    EDIT_MAP_DEFAULT_TEXTS = 8,
    // The version word the editor writes after the header (hexadecimal 1112;
    // the game reads map extras from version MAP_EXTRA_VERSION on).
    EDIT_MAP_VERSION = 0x1112
H1_ENUM_CONST_END(EditManagerConstant)

// m_zoom: 32-pixel cells (14 visible per side) or 16-pixel cells (28).
H1_ENUM_BEGIN(EditZoom)
    EDIT_ZOOM_NORMAL = 0,
    EDIT_ZOOM_OUT = 1
H1_ENUM_END(EditZoom)

// The map view, its rulers, the radar and the scroll bars (editwind.bin).
H1_ENUM_CONST_BEGIN(EditViewGeometry)
    EDIT_VIEW_LEFT = 16,
    EDIT_VIEW_TOP = 16,
    EDIT_VIEW_PIXELS = 448,
    EDIT_VIEW_CELL_PIXELS = 32,
    EDIT_VIEW_ZOOMED_CELL_PIXELS = 16,
    EDIT_VIEW_CELLS = 14,
    EDIT_VIEW_ZOOMED_CELLS = 28,
    // A ruler numbers every view cell, every second one when zoomed out.
    EDIT_RULER_SLOTS = 28,
    EDIT_RULER_SLOT_PIXELS = 16,
    EDIT_RADAR_LEFT = 480,
    EDIT_RADAR_TOP = 16,
    EDIT_RADAR_PIXELS = 144,
    EDIT_RADAR_CELL_PIXELS = 2,
    // The scroll knobs travel 35..428 along their tracks.
    EDIT_KNOB_FIRST = 35,
    EDIT_KNOB_LAST = 428
H1_ENUM_CONST_END(EditViewGeometry)

// editwind.bin widget ids and the tool commands.
H1_ENUM_BEGIN(EditWindowControlId)
    EDIT_CONTROL_MAP = 9,
    EDIT_CONTROL_HORIZONTAL_TRACK = 10,
    EDIT_CONTROL_VERTICAL_TRACK = 11,
    EDIT_CONTROL_HORIZONTAL_KNOB = 12,
    EDIT_CONTROL_VERTICAL_KNOB = 13,
    EDIT_CONTROL_SCROLL_UP = 14,
    EDIT_CONTROL_SCROLL_DOWN = 15,
    EDIT_CONTROL_SCROLL_RIGHT = 16,
    EDIT_CONTROL_SCROLL_LEFT = 17,
    EDIT_CONTROL_SCROLL_UP_LEFT = 18,
    EDIT_CONTROL_SCROLL_UP_RIGHT = 19,
    EDIT_CONTROL_SCROLL_DOWN_LEFT = 20,
    EDIT_CONTROL_SCROLL_DOWN_RIGHT = 21,
    EDIT_CONTROL_RADAR = 39,
    EDIT_CONTROL_UNDO = 101,
    EDIT_CONTROL_ZOOM = 102,
    EDIT_CONTROL_TERRAIN = 103,
    EDIT_CONTROL_OBJECTS = 104,
    EDIT_CONTROL_DETAILS = 105,
    EDIT_CONTROL_ERASER = 106,
    EDIT_CONTROL_LOAD = 107,
    EDIT_CONTROL_SAVE = 108,
    EDIT_CONTROL_QUIT = 109,
    EDIT_CONTROL_MAP_INFO = 110,
    EDIT_CONTROL_NEW = 111,
    EDIT_CONTROL_RANDOM_MAP = 113
H1_ENUM_END(EditWindowControlId)

// SelectTool's tools; their buttons follow EDIT_CONTROL_TERRAIN in this
// order.
H1_ENUM_BEGIN(EditTool)
    EDIT_TOOL_TERRAIN = 0,
    EDIT_TOOL_OBJECTS = 1,
    EDIT_TOOL_DETAILS = 2,
    EDIT_TOOL_ERASER = 3,
    EDIT_TOOL_COUNT = 4
H1_ENUM_END(EditTool)

// IsCleared's mask: a bit per terrain (TerrainType) for the objects standing
// on it, then the object classes the eraser lists after the terrains.
H1_ENUM_BEGIN(EditClearMask)
    EDIT_CLEAR_TOWNS = 0x80,
    EDIT_CLEAR_MONSTERS = 0x100,
    EDIT_CLEAR_ARTIFACTS = 0x200,
    EDIT_CLEAR_TREASURE = 0x400,
    EDIT_CLEAR_ALL = 0xffff
H1_ENUM_END(EditClearMask)

// The looped environment sounds SetCellSound gives a cell (the game's
// game::m_mapSounds ids; loop%04d.82M). Ids named by a number come from
// object frames no table names.
H1_ENUM_BEGIN(EditMapSound)
    MAP_SOUND_BUOY = 0,
    MAP_SOUND_SHIPWRECK = 1,
    MAP_SOUND_WHIRLPOOL = 2,
    MAP_SOUND_RANKING_SHRINE = 3,
    MAP_SOUND_STONE_LITHS = 4,
    MAP_SOUND_LOOP_5 = 5,
    MAP_SOUND_LOOP_6 = 6,
    MAP_SOUND_LOOP_7 = 7,
    MAP_SOUND_ALCHEMIST_LAB = 8,
    MAP_SOUND_WATERWHEEL = 9,
    MAP_SOUND_CAMPFIRE = 10,
    MAP_SOUND_WINDMILL = 11,
    MAP_SOUND_FOUNTAIN = 12,
    MAP_SOUND_LOOP_13 = 13,
    MAP_SOUND_LOOP_14 = 14,
    MAP_SOUND_MINE = 15,
    MAP_SOUND_SAWMILL = 16,
    MAP_SOUND_DAEMON_CAVE = 17,
    MAP_SOUND_SPELL_SHRINE = 18,
    MAP_SOUND_LOOP_19 = 19,
    MAP_SOUND_COAST = 20
H1_ENUM_END(EditMapSound)

// Object frames the editor recognises (frame indices of the named ICN
// tilesets).
H1_ENUM_CONST_BEGIN(EditObjectFrame)
    // town32.icn: the entrance of the first race's castle; each race adds
    // TOWN_RACE_FRAME_STRIDE and a town without a castle has its entrance
    // TOWN_CASTLE_FRAME_OFFSET frames before.
    EDIT_CASTLE_ENTRANCE_FRAME = 22,
    // The sawmill frame WriteMines records as a wood mine (other mills of the
    // sawmill and alchemist types are mercury).
    EDIT_SAWMILL_FRAME = 7,
    // rsrc32.icn: the treasure chest; the resource piles start at
    // RESOURCE_PILE_OBJECT_BASE.
    EDIT_TREASURE_CHEST_FRAME = 84,
    // obj32-07.icn frames the eraser counts as treasure.
    EDIT_TREASURE_OBJECT_FRAME_A = 3,
    EDIT_TREASURE_OBJECT_FRAME_B = 4,
    EDIT_TREASURE_OBJECT_FRAME_C = 42,
    // The frames whose objects loop an environment sound (SetCellSound).
    EDIT_SOUND_ALCHEMIST_FRAME_A = 28,
    EDIT_SOUND_ALCHEMIST_FRAME_B = 36,
    EDIT_SOUND_ALCHEMIST_FRAME_C = 44,
    EDIT_SOUND_WATERWHEEL_FRAME_A = 63,
    EDIT_SOUND_WATERWHEEL_FRAME_B = 70,
    EDIT_SOUND_WATERWHEEL_LOOP_14_FRAME = 33,
    EDIT_SOUND_LAVA_LOOP_5_FRAME = 39,
    EDIT_SOUND_LAVA_LOOP_7_FRAME = 11,
    EDIT_SOUND_LAVA_LOOP_7_FIRST = 23,
    EDIT_SOUND_LAVA_LOOP_7_LAST = 26,
    EDIT_SOUND_LAVA_LOOP_6_FIRST = 17,
    EDIT_SOUND_LAVA_LOOP_6_LAST = 22,
    EDIT_SOUND_GRASS_LOOP_13_FIRST = 30,
    EDIT_SOUND_GRASS_LOOP_13_LAST = 121,
    EDIT_SOUND_LOOP_14_FIRST = 147,
    EDIT_SOUND_LOOP_14_LAST = 167,
    EDIT_SOUND_WATER_LOOP_19_FIRST = 2,
    EDIT_SOUND_WATER_LOOP_19_LAST = 5,
    EDIT_SOUND_WATER_LOOP_19_FRAME_A = 72,
    EDIT_SOUND_WATER_LOOP_19_FRAME_B = 73
H1_ENUM_CONST_END(EditObjectFrame)

// Water tiles come in five runs of four: open water, then the coast's
// straight edge, outer corner, side and inner corner (SmoothTerrain's
// border tiles and SetCoast).
H1_ENUM_BEGIN(EditCoastTile)
    EDIT_COAST_OPEN = 0,
    EDIT_COAST_EDGE = 1,
    EDIT_COAST_OUTER_CORNER = 2,
    EDIT_COAST_SIDE = 3,
    EDIT_COAST_INNER_CORNER = 4,
    EDIT_COAST_TILE_VARIANTS = 4
H1_ENUM_END(EditCoastTile)

// DrawCell's layers.
H1_ENUM_BEGIN(EditDrawLayer)
    EDIT_DRAW_GROUND = 1,
    EDIT_DRAW_OBJECT = 2,
    EDIT_DRAW_OVERLAY = 4,
    EDIT_DRAW_ALL = 7
H1_ENUM_END(EditDrawLayer)

#pragma pack(push, 1)
// Each cell's object and overlay belong to a placed object, numbered from
// gNextCellOwner; ClearArea erases every cell of the object it hits (0: none).
struct editCellOwner {
    u16 object;
    u16 overlay;
};

// The edited map: the cells and their owners, copied whole to and from the
// undo map.
struct editMap {
    mapCell cells[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    editCellOwner owners[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
};

class editManager : public baseManager {
public:
    // The selected tool (EditTool, or EDIT_MANAGER_NO_TOOL).
    i16 m_tool;
    // The rulers' backgrounds and the radar's colour cells.
    icon* m_buttons;
    // The rulers' digits and the status bar's text.
    font* m_statusFont;
    tileset* m_groundTiles[EDIT_MANAGER_ZOOM_COUNT];
    tileset* m_cloudTiles[EDIT_MANAGER_ZOOM_COUNT];
    icon* m_objectIcons[EDIT_MANAGER_TILESET_COUNT][EDIT_MANAGER_ZOOM_COUNT];
    iconWidget* m_horizontalTrack;
    iconWidget* m_verticalTrack;
    iconWidget* m_horizontalKnob;
    iconWidget* m_verticalKnob;
    H1_ENUM_STORAGE(EditZoom, u8) m_zoom;
    // Set when the map changes; saving, a new map and loading clear it.
    i16 m_mapChanged;
    // The map cell of the object the object tool placed last (-1 none);
    // saving and loading forget it.
    i16 m_lastPlacedX;
    i16 m_lastPlacedY;
    // The object tool clears it when it places an object (-1 at start).
    i16 m_placedState;
    // The widget id of the last tool command (-1 none).
    i16 m_lastCommandId;
    // The object animation frame DrawCell adds (0..5).
    i16 m_animationFrame;
    // The executive manager of the selected tool.
    baseManager* m_toolManager;
    heroWindow* m_window;
    editMap m_map;
    editMap m_undoMap;
    // m_mapExtras[1..m_mapExtraCount-1] with their sizes, as the map file
    // stores them (record 0 is never allocated).
    i32 m_mapExtraCount;
    i32 m_mapExtraSizes[MAP_EXTRA_RECORD_CAPACITY];
    void* m_mapExtras[MAP_EXTRA_RECORD_CAPACITY];
    i8 m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    // The map cell shown at the view's top-left corner.
    i16 m_viewX;
    i16 m_viewY;
    // The map cell under the cursor.
    i16 m_cursorX;
    i16 m_cursorY;
    char m_mapFileName[EDIT_MAP_FILE_NAME_SIZE];
    H1_ENUM_STORAGE(BaseManagerMessageMask, i16) m_dispatchMask;

    editManager(void);
    void LoadObjectIcons(i16 tileset, char* largeName, char* smallName);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(tag_message& message) OVERRIDE;
    // Copies the map into the undo map.
    void SaveUndo(void);
    // Copies the map view's 448x448 square to the screen.
    void UpdateMapView(void);
    // Redraws the rulers' cursor marks.
    void UpdateCursor(void);
    void DrawRulers(i16 viewX, i16 viewY, i16 cursorX, i16 cursorY);
    // Turns screen coordinates into the view cell under them (clamped).
    void ScreenToCell(i16& x, i16& y);
    // Redraws the map view at the current view origin.
    void DrawMap(void);
    void DrawView(i16 viewX, i16 viewY);
    // Redraws the radar map, the scroll knobs and the rulers.
    void DrawRadar(i32 unused);
    void DrawCell(i16 x, i16 y, i16 column, i16 row, u8 layers);
    void ToggleZoom(void);
    void SelectTool(i16 tool);
    void Scroll(i16 dx, i16 dy);
    void UpdateKnobs(i16 update);
    void PaintTerrain(i16 column, i16 row, i16 width, i16 height, i16 terrain);
    void FillTerrain(i16 x, i16 y, i16 width, i16 height, i16 terrain);
    void SmoothTerrain(i16 terrain, i16 unused, u8 fromUndo, u8 skipBorders, u8 skipFill);
    void DoRadar(void);
    void DoHorizontalKnob(void);
    void DoVerticalKnob(void);
    void SetCellSound(i16 x, i16 y);
    void SetCoast(i16 x, i16 y);
    void CheckObjects(void);
    void UpdateTriggers(void);
    u8 Confirm(char* question);
    i32 HasTrigger(i32 trigger);
    i32 CountArtifacts(void);
    i32 CountTowns(void);
    i32 CountMines(void);
    void WriteTowns(i32 file);
    void WriteMines(i32 file);
    void WriteArtifacts(i32 file);
    void WriteObelisks(i32 file);
    i16 SaveMap(char* name);
    i16 LoadMap(char* name);
    i16 PickMap(char* unusedName, char* unusedExtension, i16 mode);
    void ClearErrors(void);
    void ShowErrors(void);
    void AddError(char* text);
    i32 IsCleared(i32 tileset, i32 index, i32 mask, i32 x, i32 y);
    // Erases the object classes in mask from the width x height cells at
    // (x, y): each cell's first layer, and its second when secondLayer is set.
    void ClearArea(i32 x, i32 y, i32 width, i32 height, u16 mask, i32 secondLayer);
    void ResetArea(i32 x, i32 y, i32 width, i32 height);
    void FreeMapExtras(void);
    void NewMap(i32 random);
    // Defined by MAPOBJ.
    void GenerateRandomMap(void);
};
#pragma pack(pop)

extern editManager* gpEditManager;
extern char* gMapCodeLetters;
extern i32 gSelectionColor;
extern SMapHeader gMapHeader;
extern char* gEditErrors[];
extern i32 gEditErrorCount;
// Set while the random-map generator lays terrain: SetTileVariant then
// re-rolls every border tile's variant.
extern i32 gVaryTiles;
extern char gPickMapNameDummy[];

void SetTileVariant(mapCell* cell, i32 tile);
char* MakeMapCode(i32 serial);
void ShowStatusAlert(char* text);
void ScatterDetails(void);
// The map-details dialog (EVENTMGR).
i32 EditMapDetails(i32 randomMap);

#endif // HOMM1_EDITOR_EDITMANAGER_H
