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
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/town.h>

class font;
class heroWindow;
class icon;
class iconWidget;
class tileset;
struct tag_message;

H1_ENUM_CONST_BEGIN(EditManagerConstant)
    EDIT_MANAGER_NO_TOOL = -1,
    // Every editor manager's Main (this one and the four tool managers)
    // tests message.type against this mask (key, mouse and widget messages).
    EDIT_MANAGER_DISPATCH_MASK = 0x32f,
    // m_placedX/m_placedY and the tool managers' last drag cell when there is
    // none.
    EDIT_NO_CELL = -1,
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
    // A map needs at least this many castles (or random castles): the save
    // check warns below it and the generator retries.
    EDIT_MAP_MIN_CASTLES = 4,
    // The version word the editor writes after the header (hexadecimal 1112;
    // the game reads map extras from version MAP_EXTRA_VERSION on).
    EDIT_MAP_VERSION = 0x1112
H1_ENUM_CONST_END(EditManagerConstant)

// m_zoomedOut: 32-pixel cells (14 visible per side) or 16-pixel cells (28);
// it indexes the zoom-level tables.
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
    EDIT_KNOB_LAST = 428,
    // The selected tool's panel below the radar (each tool manager's
    // backdrop and controls).
    EDIT_TOOL_PANEL_X = 480,
    EDIT_TOOL_PANEL_Y = 197,
    EDIT_TOOL_PANEL_WIDTH = 144,
    EDIT_TOOL_PANEL_HEIGHT = 139
H1_ENUM_CONST_END(EditViewGeometry)

// buttons.icn frames: the ruler cells (one square cell for both rulers when
// zoomed out), the radar's colour cell and view boxes, the tool buttons'
// frame pairs (normal, selected; EDIT_TOOL_COUNT of them) and the tool
// panels' backdrops.
H1_ENUM_CONST_BEGIN(EditButtonsFrame)
    EDIT_FRAME_ZOOMED_RULER_CELL = 18,
    EDIT_FRAME_TOOL_PANEL = 20,
    EDIT_FRAME_RADAR_CELL = 21,
    EDIT_FRAME_RADAR_VIEW = 22,
    EDIT_FRAME_RADAR_ZOOMED_VIEW = 23,
    EDIT_FRAME_TOP_RULER_CELL = 24,
    EDIT_FRAME_LEFT_RULER_CELL = 25,
    EDIT_FRAME_TOOL_BUTTONS = 26,
    EDIT_FRAME_CLEAR_OPTIONS = 34,
    EDIT_FRAME_CLEAR_OPTIONS_PRESSED = 35,
    EDIT_FRAME_CLEAR_PANEL = 36,
    EDIT_FRAME_EVENTS_PANEL = 37
H1_ENUM_CONST_END(EditButtonsFrame)

// escroll.icn frames: the map view's scroll tracks and knobs (the generator's
// sliders reuse the horizontal knob), the arrow buttons (normal, pressed) and
// the generator's short track.
H1_ENUM_CONST_BEGIN(EditScrollFrame)
    EDIT_SCROLL_HORIZONTAL_TRACK = 0,
    EDIT_SCROLL_VERTICAL_TRACK = 1,
    EDIT_SCROLL_HORIZONTAL_KNOB = 2,
    EDIT_SCROLL_VERTICAL_KNOB = 3,
    EDIT_SCROLL_LEFT_ARROW = 8,
    EDIT_SCROLL_RIGHT_ARROW = 10,
    EDIT_SCROLL_SHORT_TRACK = 20
H1_ENUM_CONST_END(EditScrollFrame)

// editor.mse pointer frames (mouseManager::SetPointer).
H1_ENUM_CONST_BEGIN(EditPointerFrame)
    EDIT_POINTER_DEFAULT = 0,
    // Shown while a map is saved or loaded.
    EDIT_POINTER_WAIT = 1
H1_ENUM_CONST_END(EditPointerFrame)

// The cursor shapes the drag loops request (mouseManager::SetCursorShape,
// which ignores them under Windows): horizontal and vertical slider drags,
// then the normal arrow.
H1_ENUM_CONST_BEGIN(EditCursorShape)
    EDIT_CURSOR_HORIZONTAL_DRAG = 2,
    EDIT_CURSOR_VERTICAL_DRAG = 4,
    EDIT_CURSOR_NORMAL = 6
H1_ENUM_CONST_END(EditCursorShape)

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
    // A tool panel's options button (the tool managers handle it).
    EDIT_CONTROL_TOOL_OPTIONS = 112,
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

// IsCleared's mask: a bit per object-tool category (gOverlayCategoryNames):
// one per terrain (TerrainType) for the terrain objects standing on it, then
// towns, monsters, artifacts and treasure.
H1_ENUM_BEGIN(EditClearMask)
    EDIT_CLEAR_TOWNS = 0x80,
    EDIT_CLEAR_MONSTERS = 0x100,
    EDIT_CLEAR_ARTIFACTS = 0x200,
    EDIT_CLEAR_TREASURE = 0x400,
    EDIT_CLEAR_ALL = 0xffff,
    // What a generator road between castles erases: all but towns, monsters
    // and artifacts.
    EDIT_CLEAR_ROAD =
        EDIT_CLEAR_ALL & ~(EDIT_CLEAR_TOWNS | EDIT_CLEAR_MONSTERS | EDIT_CLEAR_ARTIFACTS)
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
    // town32.icn: the entrance of the first race's castle (EDIT_CASTLE_FRAME).
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

// town32.icn: the entrance frame of a race's castle (TownType: each race's
// frames follow the previous race's by TOWN_RACE_FRAME_STRIDE) and of its
// town without a castle, TOWN_CASTLE_FRAME_OFFSET frames before.
#define EDIT_CASTLE_FRAME(type) (EDIT_CASTLE_ENTRANCE_FRAME + (type) * TOWN_RACE_FRAME_STRIDE)
#define EDIT_TOWN_FRAME(type) (EDIT_CASTLE_FRAME(type) - TOWN_CASTLE_FRAME_OFFSET)

// A terrain's MAP_CELL_TILES_PER_TERRAIN ground tiles are five runs of
// TERRAIN_TILE_VARIANT_COUNT interchangeable variants: the plain ground, then
// the border tiles BlendTerrain fits against another terrain. A border tile
// is drawn for a differing neighbour to the north or east (or north-east);
// the ground-flip bits mirror it to the south and west. SetCoast reads the
// water's border runs.
H1_ENUM_BEGIN(EditTileRun)
    EDIT_TILE_PLAIN = 0,
    EDIT_TILE_NORTH_EDGE = 1,
    EDIT_TILE_NORTH_EAST_CORNER = 2,
    EDIT_TILE_EAST_EDGE = 3,
    // Only the diagonal neighbour differs.
    EDIT_TILE_NORTH_EAST_INNER_CORNER = 4
H1_ENUM_END(EditTileRun)

// DrawCell's layers.
H1_ENUM_BEGIN(EditDrawLayer)
    EDIT_DRAW_GROUND = 1,
    EDIT_DRAW_OBJECT = 2,
    EDIT_DRAW_OVERLAY = 4,
    EDIT_DRAW_ALL = 7
H1_ENUM_END(EditDrawLayer)

#pragma pack(push, 1)
// The per-cell ids of the placed objects whose frames the cell shows on its
// object and overlay layers: PlaceOverlay numbers each placed object from
// gNextObjectId, and ClearArea erases every cell of the object it hits
// (0: none).
struct editMapCellPair {
    u16 objectId;
    u16 overlayId;
};

// The edited map: the cells and their object ids, copied whole to and from
// the undo map.
struct editMap {
    mapCell cells[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    editMapCellPair cellPairs[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
};

// The editor's town and hero map-extra records end in a reserved block:
// PlaceOverlay zero-fills the new record, the town and hero dialogs copy it
// whole and SaveMap writes m_extraSizes bytes, but no code of either program
// reads it (the game's readers stop at mapTownExtra/mapHeroExtra).
H1_ENUM_CONST_BEGIN(EditExtraConstant)
    EDIT_EXTRA_RESERVED_SIZE = 50
H1_ENUM_CONST_END(EditExtraConstant)

// A town's map-extra record as the editor keeps it.
struct editTownExtra {
    mapTownExtra record;
    u8 reserved[EDIT_EXTRA_RESERVED_SIZE];
};

// A placed hero's map-extra record as the editor keeps it.
struct editHeroExtra {
    mapHeroExtra record;
    u8 reserved[EDIT_EXTRA_RESERVED_SIZE];
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
    H1_ENUM_STORAGE(EditZoom, u8) m_zoomedOut;
    // Set when the map changes; saving, a new map and loading clear it.
    i16 m_mapChanged;
    // The map cell of the object the object tool placed last (-1 none);
    // saving, loading and the tools' Open forget it.
    i16 m_placedX;
    i16 m_placedY;
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
    // The map-extra records (towns, heroes, events) cells name by
    // m_objectMetadata: m_extras[1..m_extraCount-1] with their sizes, as the
    // map file stores them (record 0 is never allocated).
    i32 m_extraCount;
    i32 m_extraSizes[MAP_EXTRA_RECORD_CAPACITY];
    void* m_extras[MAP_EXTRA_RECORD_CAPACITY];
    i8 m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    // The map cell shown at the view's top-left corner.
    i16 m_viewX;
    i16 m_viewY;
    // The map cell under the cursor.
    i16 m_cursorX;
    i16 m_cursorY;
    // The map's file name ("<code>1234.MAP"); the map-details dialog edits
    // its code.
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
    // Sets the ground of the width x height view cells at (column, row) to
    // the terrain's plain tile and redraws them.
    void PaintGround(i16 column, i16 row, i16 width, i16 height, i16 terrain);
    // The same for map cells, filling the rectangle with random variants.
    void FillGround(i16 x, i16 y, i16 width, i16 height, i16 terrain);
    // Fits the terrain's edge tiles to their neighbours over the whole map.
    void BlendTerrain(i16 terrain, u8 unused, u8 fromUndo, u8 skipBorders, u8 skipFill);
    void DoRadar(void);
    void DoHorizontalKnob(void);
    void DoVerticalKnob(void);
    void SetCellSound(i16 x, i16 y);
    void SetCoast(i16 x, i16 y);
    void CheckObjects(void);
    void UpdateTriggers(void);
    u8 Confirm(char* question);
    // 1 when a cell's trigger byte equals trigger.
    i32 HasObject(i32 trigger);
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
    // Places one chain link at (*x, *y) facing `direction` and steps on; a
    // tree chain keeps to treeFamily (its objects' first letter, 0: any).
    i32 PlaceChainLink(i32* x, i32* y, i32 direction, i32 tileset, char treeFamily);
    void PlaceTowns(void);
    // Places the site producing `resource` at (x, y): a sawmill, an
    // alchemist's lab, or a mine with the resource's marker to its right.
    void PlaceResourceSite(i32 x, i32 y, i32 resource);
    // Places towns, mines and obelisks.
    void PlaceRandomObjects(i32 density, i32 strength);
    // Places treasure (guarded in map corners) and wandering monsters.
    void PlaceTreasures(i32 density, i32 strength);
    // Scatters decorative objects over bare ground.
    void ScatterDecorations(void);
};
#pragma pack(pop)

extern editManager* gEditManager;
extern char* gMapCodeLetters;
extern i32 gSelectionColor;
extern SMapHeader gEditMapHeader;
extern char* gEditErrors[];
extern i32 gEditErrorCount;
// Set while the random-map generator lays terrain: SetTileVariant then
// re-rolls every border tile's variant.
extern i32 gVaryTiles;
extern char gPickMapNameDummy[];

void SetTileVariant(mapCell* cell, i32 tile);
char* MakeMapCode(i32 serial);
// Shows text in the status bar with a beep and clears it after 1.5 seconds.
void ShowStatusWarning(char* text);
// Scales a generator count by a 0..100 density setting (50: unchanged apart
// from the size bonus).
void ScaleByDensity(i32* count, i32 density);
void ScatterDetails(void);

#endif // HOMM1_EDITOR_EDITMANAGER_H
