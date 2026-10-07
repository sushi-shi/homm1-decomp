#ifndef HOMM1_EDITOR_OVERLAYMANAGER_H
#define HOMM1_EDITOR_OVERLAYMANAGER_H

// The object-placement tool (src/EDITOR/OVERLAY.cpp; Editor\OVERLAY.CPP).
// editManager::SelectTool runs it as the tool manager; Open stores the class
// name "overlayManager". Descriptive names: overlayType and its fields,
// gOverlayTypes, gOverlayCategoryKinds, gSelectedOverlay and the methods.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/game.h>
#include <SOURCE/mapCell.h>

class border;
class button;
class font;
class icon;
class iconWidget;
struct tag_message;

H1_ENUM_CONST_BEGIN(OverlayTypeConstant)
    OVERLAY_TYPE_NAME_LENGTH = 9,
    // An object spans up to 3 rows of 5 cells above and right of its anchor
    // (cell row * 5 + column, OVERLAY_FOOTPRINT_CELL) plus one corner cell two
    // right, three up (cell 15): four rows in all.
    OVERLAY_FOOTPRINT_ROWS = 3,
    OVERLAY_FOOTPRINT_COLUMNS = 5,
    OVERLAY_FOOTPRINT_CELLS = 16,
    OVERLAY_FOOTPRINT_CORNER = 15,
    OVERLAY_FOOTPRINT_CORNER_BIT = 0x8000,
    OVERLAY_FOOTPRINT_CORNER_COLUMN = 2,
    OVERLAY_FOOTPRINT_CORNER_ROW = 3,
    OVERLAY_FOOTPRINT_HEIGHT = OVERLAY_FOOTPRINT_CORNER_ROW + 1,
    // The anchor cell (row 0, column 0): a resource marker's only frame.
    OVERLAY_FOOTPRINT_ANCHOR = 0,
    // A footprint mask's cells per row and per column (MeasureOverlay); the
    // corner cell lies in row 3 and column 2.
    OVERLAY_FOOTPRINT_ROW_0_MASK = 0x001f,
    OVERLAY_FOOTPRINT_ROW_1_MASK = 0x03e0,
    OVERLAY_FOOTPRINT_ROW_2_MASK = 0x7c00,
    OVERLAY_FOOTPRINT_COLUMN_0_MASK = 0x0421,
    OVERLAY_FOOTPRINT_COLUMN_1_MASK = 0x0842,
    OVERLAY_FOOTPRINT_COLUMN_2_MASK = 0x1084 | OVERLAY_FOOTPRINT_CORNER_BIT,
    OVERLAY_FOOTPRINT_COLUMN_3_MASK = 0x2108,
    OVERLAY_FOOTPRINT_COLUMN_4_MASK = 0x4210,
    // overlayType::flags: a mine's resource marker (placed on a mine's
    // resourceMask cell), and a mine drawn with its resource (resourceFrame)
    // in its column 1 of row 0.
    OVERLAY_TYPE_RESOURCE_MARKER = 1,
    OVERLAY_TYPE_SHOWS_RESOURCE = 2,
    OVERLAY_SHOWN_RESOURCE_COLUMN = 1,
    OVERLAY_SHOWN_RESOURCE_ROW = 0,
    // overlayType::terrainMask bits LoadCategory accepts for the classes that
    // are not listed by terrain.
    OVERLAY_TERRAIN_MASK_ANY = 0xff,
    // The editor's object table (gOverlayTypes).
    OVERLAY_TYPE_COUNT = 295
H1_ENUM_CONST_END(OverlayTypeConstant)

// A footprint cell's index (overlayType::frames) and its bit in the
// footprint masks; a terrain's bit in overlayType::terrainMask.
#define OVERLAY_FOOTPRINT_CELL(column, row) ((row) * OVERLAY_FOOTPRINT_COLUMNS + (column))
#define OVERLAY_CELL_BIT(cell) (1 << (cell))
#define OVERLAY_FOOTPRINT_BIT(column, row) OVERLAY_CELL_BIT(OVERLAY_FOOTPRINT_CELL(column, row))
#define OVERLAY_TERRAIN_BIT(terrain) (1 << (terrain))

// overlayType::kind and gOverlayCategoryKinds: an object's class in the
// object tool's categories. Terrain objects are listed by the terrain they
// stand on (a category per terrain); each other class has one category.
H1_ENUM_BEGIN(OverlayKind)
    OVERLAY_KIND_TERRAIN = 0,
    OVERLAY_KIND_TOWN = 1,
    OVERLAY_KIND_MONSTER = 2,
    OVERLAY_KIND_ARTIFACT = 3,
    OVERLAY_KIND_TREASURE = 4
H1_ENUM_END(OverlayKind)

// One placeable object of the editor's object table: its tileset frames per
// footprint cell and the masks of the cells it occupies.
#pragma pack(push, 1)
struct overlayType {
    char name[OVERLAY_TYPE_NAME_LENGTH];
    // The object tileset (editManager::m_objectIcons).
    H1_ENUM_STORAGE(MapTileset, i8) tileset;
    H1_ENUM_STORAGE(OverlayKind, i8) kind;
    // How often the generator's ScatterDecorations picks it (in 100).
    u16 frequency;
    // Every cell the object occupies: those not on the overlay layer go on
    // the object layer and need free ground of a terrainMask terrain.
    u16 footprintMask;
    // One bit per terrain (TerrainType) the object stands on.
    u8 terrainMask;
    // The footprint cells on the overlay layer.
    u16 overlayMask;
    // Cells that block movement.
    u16 blockMask;
    u16 animatedMask;
    // The cell a mine's resource marker goes to.
    u16 resourceMask;
    // OVERLAY_TYPE_SHOWS_RESOURCE: the rsrc32.icn frame of the resource it
    // shows (the marker type whose first frame it is).
    u8 resourceFrame;
    u8 flags;
    // Cells whose entry runs the object's event.
    u16 eventMask;
    // The object code (MapObjectType, or a MapFileObjectType placeholder)
    // its footprint cells take as mapCell::m_triggerType.
    u8 trigger;
    // Each footprint cell's frame (0xff none).
    u8 frames[OVERLAY_FOOTPRINT_CELLS];
};
#pragma pack(pop)

H1_ENUM_CONST_BEGIN(OverlayManagerConstant)
    OVERLAY_MANAGER_TYPE_CAPACITY = 128,
    OVERLAY_CATEGORY_COUNT = 11,
    OVERLAY_NO_SELECTION = -1,
    // Limits PlaceOverlay enforces beside GAME_TOWN_COUNT towns: the map
    // file's mine table after its two unique sites (game::m_mines from
    // MINE_SLOT_STANDARD_FIRST), and artifacts.
    OVERLAY_MINE_LIMIT = 34,
    OVERLAY_ARTIFACT_LIMIT = 32
H1_ENUM_CONST_END(OverlayManagerConstant)

// The tool panel: the object preview border and the category arrows.
H1_ENUM_ID_BEGIN(OverlayControlId)
    OVERLAY_PREVIEW_BORDER = 0x26,
    OVERLAY_NEXT_CATEGORY_BUTTON = 0x28,
    OVERLAY_PREVIOUS_CATEGORY_BUTTON = 0x29
H1_ENUM_ID_END(OverlayControlId)

H1_ENUM_CONST_BEGIN(OverlayManagerLayout)
    OVERLAY_PREVIEW_X = 509,
    OVERLAY_PREVIEW_Y = 227,
    OVERLAY_PREVIEW_WIDTH = 86,
    OVERLAY_PREVIEW_HEIGHT = 70,
    OVERLAY_PREVIEW_FRAME = 2,
    // DrawOverlay's update region starts 33 rows above the preview border.
    OVERLAY_PREVIEW_UPDATE_Y = 194,
    OVERLAY_PREVIEW_OBJECT_X = 512,
    OVERLAY_PREVIEW_OBJECT_Y = 273,
    OVERLAY_PREVIOUS_CATEGORY_X = 485,
    OVERLAY_NEXT_CATEGORY_X = 604,
    OVERLAY_CATEGORY_BUTTON_Y = 313,
    OVERLAY_CATEGORY_BUTTON_SIZE = 16,
    OVERLAY_CATEGORY_NAME_X = 500,
    OVERLAY_CATEGORY_NAME_Y = 312,
    OVERLAY_CATEGORY_NAME_WIDTH = 104,
    OVERLAY_CATEGORY_NAME_HEIGHT = 16,
    OVERLAY_CATEGORY_TEXT_X = 505,
    OVERLAY_CATEGORY_TEXT_Y = 313,
    // Footprint cell colours: an overlay-layer cell, an object-layer cell.
    OVERLAY_FOOTPRINT_OVERLAY_COLOR = 0x65,
    OVERLAY_FOOTPRINT_GROUND_COLOR = 0xc7
H1_ENUM_CONST_END(OverlayManagerLayout)

// PickOverlay's full-screen grid of 9 x 9 objects.
H1_ENUM_CONST_BEGIN(OverlayPickerLayout)
    OVERLAY_PICKER_COLUMNS = 9,
    OVERLAY_PICKER_ROWS = 9,
    OVERLAY_PICKER_PAGE = OVERLAY_PICKER_COLUMNS * OVERLAY_PICKER_ROWS,
    OVERLAY_PICKER_CELL_WIDTH = 69,
    OVERLAY_PICKER_CELL_HEIGHT = 53,
    OVERLAY_PICKER_OBJECT_X = 2,
    OVERLAY_PICKER_OBJECT_Y = 34,
    OVERLAY_PICKER_CELL_FRAME = 3,
    OVERLAY_PICKER_FOOTPRINT_COLUMNS = 4,
    OVERLAY_PICKER_FOOTPRINT_ROWS = 3
H1_ENUM_CONST_END(OverlayPickerLayout)

#pragma pack(push, 1)
class overlayManager : public baseManager {
public:
    // The tool panel's backdrop while Open draws it.
    iconWidget* m_panel;
    // overlay.icn: the preview frame, the footprint cells and the picker grid.
    icon* m_icon;
    // The objects of the shown category (LoadCategory).
    overlayType m_types[OVERLAY_MANAGER_TYPE_CAPACITY];
    i16 m_typeCount;
    // The selected object's footprint extent in cells.
    i16 m_width;
    i16 m_height;
    border* m_previewBorder;
    button* m_nextButton;
    button* m_previousButton;
    u8 m_unused16ca[4];
    font* m_font;
    // Main drew the selected object over the map view.
    b32 m_previewDrawn;
    // Main's message.type mask (MessageType bits, as the game's managers).
    i16 m_dispatchMask;

    overlayManager(void);
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(tag_message& message) OVERRIDE;
    // Outlines a footprint at screen (x, y): footprintMask cells, coloured
    // by overlayMask, within width columns and height rows.
    void DrawFootprint(i16 x, i16 y, i16 footprintMask, i16 overlayMask, i16 width, i16 height);
    void DrawOverlay(overlayType* type, i16 x, i16 y, i16 width, i16 height, b32 update);
    // Fills m_types with the category's objects; 0 when it has none.
    i16 LoadCategory(i16 category);
    // The full-screen object picker; returns the chosen m_types index or -1.
    i16 PickOverlay(i16 category);
    void MeasureOverlay(overlayType* type);
    void DrawCategoryName(b32 update);
    void DrawSelectedOverlay(void);
};
#pragma pack(pop)

// 1 when type fits at map cell (x, y): its cells inside the map and free.
i16 CanPlaceOverlay(overlayType* type, i16 x, i16 y);
// Places type with its anchor at map cell (x, y); 0 (with a status warning)
// when it does not fit or a limit is reached.
b32 PlaceOverlay(overlayType* type, i16 x, i16 y);
// Puts a resource marker on the mine at (x, y) (checkMine: require the mine's
// marker cell there).
b32 PlaceMineResource(overlayType* type, i16 x, i16 y, b32 checkMine);

// The editor's object table (EDITMGR's data).
extern overlayType gOverlayTypes[];
extern H1_ENUM_STORAGE(OverlayKind, i32) gOverlayCategoryKinds[];
extern i16 gSelectedOverlay;

#endif // HOMM1_EDITOR_OVERLAYMANAGER_H
