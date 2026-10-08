#ifndef HOMM1_EDITOR_EDITMANAGER_H
#define HOMM1_EDITOR_EDITMANAGER_H

#include <BASE/baseManager.h>
#include <EDITOR/randomMap.h>
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

enum EditManagerConstant {
    EDIT_MANAGER_DISPATCH_MASK = 0x32f,
    EDIT_NO_CELL = -1,
    EDIT_END_SCAN = 999,
    EDIT_MANAGER_TILESET_COUNT = 21,
    EDIT_MANAGER_ZOOM_COUNT = 2,
    EDIT_MAP_FILE_NAME_SIZE = 16,
    EDIT_MANAGER_ERROR_CAPACITY = 100,
    EDIT_MAP_ARTIFACT_SLOTS = 37,
    EDIT_MAP_OBELISK_LIMIT = 48,
    EDIT_MAP_MIN_CASTLES = 4,
    EDIT_MAP_NO_RECORD = 0xff,
    EDIT_MAP_RANDOM_MINE_TYPE = 0xff,
    EDIT_MAP_FORMAT_RANGE = 10,
    EDIT_MAP_SERIAL_LIMIT = 65000,
    EDIT_MAP_CODE_LENGTH = 4,
    EDIT_MAP_CODE_LETTER_COUNT = 26,
    EDIT_MAP_CODE_FIRST_LETTERS = 5,
    EDIT_MAP_NAME_SERIAL_MODULUS = 1000,
    EDIT_MAP_VERSION = 0x1112,
    EDIT_MAP_HEADER_BUFFER_SIZE = 2000
};

enum EditZoom {
    EDIT_ZOOM_NORMAL = 0,
    EDIT_ZOOM_OUT = 1
};

enum EditViewGeometry {
    EDIT_VIEW_LEFT = 16,
    EDIT_VIEW_TOP = 16,
    EDIT_VIEW_PIXELS = 448,
    EDIT_VIEW_CELL_PIXELS = 32,
    EDIT_VIEW_ZOOMED_CELL_PIXELS = 16,
    EDIT_VIEW_CELLS = 14,
    EDIT_VIEW_ZOOMED_CELLS = 28,
    EDIT_VIEW_CENTER = EDIT_VIEW_CELLS / 2,
    EDIT_VIEW_ZOOMED_CENTER = EDIT_VIEW_ZOOMED_CELLS / 2,
    EDIT_VIEW_ORIGINS = MAP_CELL_GRID_SIZE - EDIT_VIEW_CELLS + 1,
    EDIT_VIEW_ZOOMED_ORIGINS = MAP_CELL_GRID_SIZE - EDIT_VIEW_ZOOMED_CELLS + 1,
    EDIT_SELECTION_LINE_WIDTH = 2,
    EDIT_RULER_SLOTS = 28,
    EDIT_RULER_SLOT_PIXELS = 16,
    EDIT_RULER_SLOTS_PER_CELL = 2,
    EDIT_RULER_CELL_TEXT_OFFSET = 8,
    EDIT_TOP_RULER_TEXT_X = EDIT_VIEW_LEFT + 3,
    EDIT_TOP_RULER_TEXT_Y = 2,
    EDIT_LEFT_RULER_TEXT_X = 3,
    EDIT_LEFT_RULER_TEXT_Y = EDIT_VIEW_TOP + 2,
    EDIT_KNOB_FIRST = 35,
    EDIT_KNOB_LAST = 428,
    EDIT_TOOL_PANEL_X = 480,
    EDIT_TOOL_PANEL_Y = 197,
    EDIT_TOOL_PANEL_WIDTH = 144,
    EDIT_TOOL_PANEL_HEIGHT = 139
};

enum EditButtonsFrame {
    EDIT_FRAME_ZOOMED_RULER_CELL = 18,
    EDIT_FRAME_TOOL_PANEL = 20,
    EDIT_FRAME_RADAR_CELL = 21,
    EDIT_FRAME_RADAR_VIEW = 22,
    EDIT_FRAME_RADAR_ZOOMED_VIEW = 23,
    EDIT_FRAME_TOP_RULER_CELL = 24,
    EDIT_FRAME_LEFT_RULER_CELL = 25,
    EDIT_FRAME_TOOL_BUTTONS = 26,
    EDIT_FRAMES_PER_TOOL_BUTTON = 2,
    EDIT_FRAME_CLEAR_OPTIONS = 34,
    EDIT_FRAME_CLEAR_OPTIONS_PRESSED = 35,
    EDIT_FRAME_CLEAR_PANEL = 36,
    EDIT_FRAME_EVENTS_PANEL = 37
};

enum EditScrollFrame {
    EDIT_SCROLL_HORIZONTAL_TRACK = 0,
    EDIT_SCROLL_VERTICAL_TRACK = 1,
    EDIT_SCROLL_HORIZONTAL_KNOB = 2,
    EDIT_SCROLL_VERTICAL_KNOB = 3,
    EDIT_SCROLL_LEFT_ARROW = 8,
    EDIT_SCROLL_LEFT_ARROW_PRESSED = 9,
    EDIT_SCROLL_RIGHT_ARROW = 10,
    EDIT_SCROLL_RIGHT_ARROW_PRESSED = 11,
    EDIT_SCROLL_SHORT_TRACK = 20
};

enum EditPointerFrame {
    EDIT_POINTER_DEFAULT = 0,
    EDIT_POINTER_WAIT = 1
};

enum EditCursorShape {
    EDIT_CURSOR_HORIZONTAL_DRAG = 2,
    EDIT_CURSOR_VERTICAL_DRAG = 4,
    EDIT_CURSOR_NORMAL = 6
};

enum EditViewColor {
    EDIT_RULER_CURSOR_COLOR = 1,
    EDIT_RULER_TEXT_COLOR = 192,
    EDIT_RADAR_TOWN_COLOR = 4,
    EDIT_RADAR_RESOURCE_COLOR = 10,
    EDIT_RADAR_UNSEEN_COLOR = 0,
    EDIT_SELECTION_COLOR = 190,
    EDIT_CLOUD_TILE_MASK = 3
};

enum EditClearLayer {
    EDIT_CLEAR_OBJECT_LAYER = 0,
    EDIT_CLEAR_OVERLAY_LAYER = 1,
    EDIT_CLEAR_LAYER_COUNT = 2
};

enum EditWindowControlId {
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
    EDIT_CONTROL_TOOL_OPTIONS = 112,
    EDIT_CONTROL_RANDOM_MAP = 113
};

enum EditTool {
    EDIT_TOOL_NONE = -1,
    EDIT_TOOL_TERRAIN = 0,
    EDIT_TOOL_OBJECTS = 1,
    EDIT_TOOL_DETAILS = 2,
    EDIT_TOOL_ERASER = 3,
    EDIT_TOOL_COUNT = 4
};

enum EditClearMask {
    EDIT_CLEAR_NONE = 0,
    EDIT_CLEAR_TOWNS = 0x80,
    EDIT_CLEAR_MONSTERS = 0x100,
    EDIT_CLEAR_ARTIFACTS = 0x200,
    EDIT_CLEAR_TREASURE = 0x400,
    EDIT_CLEAR_ALL = 0xffff,
    EDIT_CLEAR_ROAD =
        EDIT_CLEAR_ALL & ~(EDIT_CLEAR_TOWNS | EDIT_CLEAR_MONSTERS | EDIT_CLEAR_ARTIFACTS)
};

enum EditMapSound {
    EDIT_MAP_SOUND_NONE = MAP_SOUND_NONE,
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
};

enum EditObjectFrame {
    EDIT_CASTLE_ENTRANCE_FRAME = 22,
    EDIT_SAWMILL_FRAME = 7,
    EDIT_TREASURE_CHEST_FRAME = 84,
    EDIT_CHEST_OBJECT_FRAME = 3,
    EDIT_LAMP_OBJECT_FRAME = 4,
    EDIT_CAMPFIRE_OBJECT_FRAME = 42,
    EDIT_LAVA_DETAIL_FRAME = 0,
    EDIT_DESERT_DETAIL_FRAME = 2,
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
};

#define EDIT_RESOURCE_PILE_FRAME(resource)                                                         \
    (RESOURCE_PILE_OBJECT_BASE + resource)

#define EDIT_CASTLE_FRAME(type)                                                                    \
    (EDIT_CASTLE_ENTRANCE_FRAME + type * TOWN_RACE_FRAME_STRIDE)
#define EDIT_TOWN_FRAME(type) (EDIT_CASTLE_FRAME(type) - TOWN_CASTLE_FRAME_OFFSET)

enum EditTileRun {
    EDIT_TILE_PLAIN = 0,
    EDIT_TILE_NORTH_EDGE = 1,
    EDIT_TILE_NORTH_EAST_CORNER = 2,
    EDIT_TILE_EAST_EDGE = 3,
    EDIT_TILE_NORTH_EAST_INNER_CORNER = 4
};
#define EDIT_TILE_RUN_FIRST(run) (run * TERRAIN_TILE_VARIANT_COUNT)

enum EditDrawLayer {
    EDIT_DRAW_GROUND = 1,
    EDIT_DRAW_OBJECT = 2,
    EDIT_DRAW_OVERLAY = 4,
    EDIT_DRAW_ALL = 7
};

#pragma pack(push, 1)
enum EditCheckConstant {
    EDIT_WHIRLPOOL_SECOND_CELL_X = 2,
    EDIT_WHIRLPOOL_SECOND_CELL_Y = 1,
    EDIT_DETAIL_PERCENT = 3
};

struct editMapCellPair {
    u16 objectId;
    u16 overlayId;
};

struct editMap {
    mapCell cells[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    editMapCellPair cellPairs[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
};

struct editMapRecord {
    u8 x;
    u8 y;
    u8 type;
};

enum EditExtraConstant {
    EDIT_EXTRA_UNUSED_SIZE = 50
};

struct editTownExtra {
    mapTownExtra record;
    u8 unused14[EDIT_EXTRA_UNUSED_SIZE];
};

struct editHeroExtra {
    mapHeroExtra record;
    u8 unused19[EDIT_EXTRA_UNUSED_SIZE];
};

class editManager : public baseManager {
public:
    i16 m_tool;
    icon* m_buttons;
    font* m_statusFont;
    tileset* m_groundTiles[EDIT_MANAGER_ZOOM_COUNT];
    tileset* m_cloudTiles[EDIT_MANAGER_ZOOM_COUNT];
    icon* m_objectIcons[EDIT_MANAGER_TILESET_COUNT][EDIT_MANAGER_ZOOM_COUNT];
    iconWidget* m_horizontalTrack;
    iconWidget* m_verticalTrack;
    iconWidget* m_horizontalKnob;
    iconWidget* m_verticalKnob;
    u8 m_zoomedOut;
    i16 m_mapChanged;
    i16 m_placedX;
    i16 m_placedY;
    i16 m_placedState;
    i16 m_lastHoverId;
    i16 m_animationFrame;
    baseManager* m_toolManager;
    heroWindow* m_window;
    editMap m_map;
    editMap m_undoMap;
    i32 m_extraCount;
    i32 m_extraSizes[MAP_EXTRA_RECORD_CAPACITY];
    void* m_extras[MAP_EXTRA_RECORD_CAPACITY];
    i8 m_mapSounds[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    i16 m_viewX;
    i16 m_viewY;
    i16 m_cursorX;
    i16 m_cursorY;
    char m_mapFileName[EDIT_MAP_FILE_NAME_SIZE];
    i16 m_dispatchMask;

    editManager(void);
    void LoadObjectIcons(i16 tileset, char* largeName, char* smallName);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(tag_message& message) ;
    void SaveUndo(void);
    void UpdateMapView(void);
    void UpdateCursor(void);
    void DrawRulers(i16 viewX, i16 viewY, i16 cursorX, i16 cursorY);
    void ScreenToCell(i16& x, i16& y);
    void DrawMap(void);
    void DrawView(i16 viewX, i16 viewY);
    void DrawRadar(b32 updateScreen);
    void DrawCell(i16 x, i16 y, i16 column, i16 row, u8 layers);
    void ToggleZoom(void);
    void SelectTool(i16 tool);
    void Scroll(i16 dx, i16 dy);
    void UpdateKnobs(i16 update);
    void PaintGround(i16 column, i16 row, i16 width, i16 height, i16 terrain);
    void FillGround(i16 x, i16 y, i16 width, i16 height, i16 terrain);
    void BlendTerrain(
        i16 terrain,
        u8 unused,
        u8 fromUndo,
        u8 skipBorders,
        u8 skipFill
    );
    void DoRadar(void);
    void DoHorizontalKnob(void);
    void DoVerticalKnob(void);
    void SetCellSound(i16 x, i16 y);
    void SetCoast(i16 x, i16 y);
    void CheckObjects(void);
    void UpdateTriggers(void);
    u8 Confirm(char* question);
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
    i16
    PickMap(char* unusedName, char* unusedExtension, i16 mode);
    void ClearErrors(void);
    void ShowErrors(void);
    void AddError(char* text);
    i32 IsCleared(i32 tileset, i32 index, i32 mask, i32 x, i32 y);
    void ClearArea(i32 x, i32 y, i32 width, i32 height, u16 mask, b32 secondLayer);
    void ResetArea(i32 x, i32 y, i32 width, i32 height);
    void FreeMapExtras(void);
    void FreeUnusedExtras(void);
    void NewMap(b32 random);
    void GenerateRandomMap(void);
    b32 HasEnoughCastles(void);
    void PaintRandomTerrain(
        i32 terrain,
        i32 percent,
        i32 baseTerrain
    );
    void RemoveSmallRegions(void);
    void PlaceObstacleChains(i32 density, i32 tileset);
    b32 PlaceChainLink(
        i32* x,
        i32* y,
        i32 direction,
        i32 tileset,
        char treeFamily
    );
    void PlaceCastles(void);
    void PlaceResourceSite(i32 x, i32 y, i32 resource);
    void PlaceRandomObjects(i32 density, i32 strength);
    void PlaceTreasures(i32 density, i32 strength);
    void ScatterDecorations(void);
};
#pragma pack(pop)

extern editManager* gEditManager;
extern char* gMapCodeLetters;
extern i32 gSelectionColor;
extern char gEditMapHeader[];
#define EDIT_MAP_HEADER() (reinterpret_cast<SMapHeader*>(gEditMapHeader))
extern char* gEditErrors[];
extern i32 gEditErrorCount;
extern b32 gVaryTiles;

void SetTileVariant(mapCell* cell, i32 firstTile);
char* MakeMapCode(i32 serial);
void ShowStatusWarning(char* text);
void ScatterDetails(void);

#endif
