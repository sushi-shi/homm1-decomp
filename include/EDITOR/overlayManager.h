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
    // (bits row * 5 + column) plus one corner cell two right, three up
    // (bit 15).
    OVERLAY_FOOTPRINT_ROWS = 3,
    OVERLAY_FOOTPRINT_COLUMNS = 5,
    OVERLAY_FOOTPRINT_CELLS = 16,
    OVERLAY_FOOTPRINT_CORNER = 15,
    OVERLAY_FOOTPRINT_CORNER_BIT = 0x8000,
    OVERLAY_FOOTPRINT_CORNER_COLUMN = 2,
    OVERLAY_FOOTPRINT_CORNER_ROW = 3,
    // overlayType::flags.
    OVERLAY_TYPE_MINE_RESOURCE = 1,
    // The mine draws its resource (resourceFrame) in column 1 of row 0.
    OVERLAY_TYPE_SHOWS_RESOURCE = 2,
    // The editor's object table (gOverlayTypes).
    OVERLAY_TYPE_COUNT = 295
H1_ENUM_CONST_END(OverlayTypeConstant)

// One placeable object of the editor's object table: its tileset frames per
// footprint cell and the masks of the cells it occupies.
#pragma pack(push, 1)
struct overlayType {
    char name[OVERLAY_TYPE_NAME_LENGTH];
    // The object tileset (editManager::m_objectIcons).
    H1_ENUM_STORAGE(MapTileset, i8) tileset;
    // The category class (gOverlayCategoryKinds); 0 for terrain objects.
    i8 kind;
    // How often the generator's ScatterDecorations picks it (in 100).
    u16 frequency;
    // Cells on the object layer, which need free ground of a terrainMask
    // terrain.
    u16 groundMask;
    // One bit per terrain (TerrainType) the object stands on.
    u8 terrainMask;
    // Cells on the overlay layer.
    u16 overlayMask;
    // Cells that block movement.
    u16 blockMask;
    u16 animatedMask;
    // The cell a mine's resource marker goes to.
    u16 resourceMask;
    u8 resourceFrame;
    u8 flags;
    // Cells whose entry runs the object's event.
    u16 eventMask;
    u8 trigger;
    // Each footprint cell's frame (0xff none).
    u8 frames[OVERLAY_FOOTPRINT_CELLS];
};
#pragma pack(pop)

// Main tests message.type against the dispatch mask the managers share.
H1_ENUM_CONST_BEGIN(OverlayManagerConstant)
    OVERLAY_MANAGER_DISPATCH_MASK = 0x32f,
    OVERLAY_MANAGER_TYPE_CAPACITY = 128,
    OVERLAY_CATEGORY_COUNT = 11,
    OVERLAY_NO_SELECTION = -1,
    // Limits PlaceOverlay enforces.
    OVERLAY_TOWN_LIMIT = 36,
    OVERLAY_MINE_LIMIT = 34,
    OVERLAY_ARTIFACT_LIMIT = 32
H1_ENUM_CONST_END(OverlayManagerConstant)

// The tool panel: the object preview border and the category arrows.
H1_ENUM_BEGIN(OverlayControlId)
    OVERLAY_PREVIEW_BORDER = 0x26,
    OVERLAY_NEXT_CATEGORY_BUTTON = 0x28,
    OVERLAY_PREVIOUS_CATEGORY_BUTTON = 0x29
H1_ENUM_END(OverlayControlId)

H1_ENUM_CONST_BEGIN(OverlayManagerLayout)
    OVERLAY_PANEL_X = 480,
    OVERLAY_PANEL_Y = 197,
    OVERLAY_PANEL_WIDTH = 144,
    OVERLAY_PANEL_HEIGHT = 139,
    OVERLAY_PANEL_FRAME = 20,
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
    OVERLAY_PREVIOUS_CATEGORY_FRAME = 8,
    OVERLAY_NEXT_CATEGORY_FRAME = 10,
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
    OVERLAY_PICKER_PAGE = 81,
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
    u8 m_unknown16ca[4];
    font* m_font;
    // Main drew the selected object over the map view.
    i32 m_previewDrawn;
    H1_ENUM_STORAGE(BaseManagerMessageMask, i16) m_dispatchMask;

    overlayManager(void);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(tag_message& message) OVERRIDE;
    // Outlines a footprint at screen (x, y): groundMask cells, coloured by
    // overlayMask, within width columns and height rows.
    void DrawFootprint(i16 x, i16 y, i16 groundMask, i16 overlayMask, i16 width, i16 height);
    void DrawOverlay(overlayType* type, i16 x, i16 y, i16 width, i16 height, i32 update);
    // Fills m_types with the category's objects; 0 when it has none.
    i16 LoadCategory(i16 category);
    // The full-screen object picker; returns the chosen m_types index or -1.
    i16 PickOverlay(i16 category);
    void MeasureOverlay(overlayType* type);
    void DrawCategoryName(i32 update);
    void DrawSelectedOverlay(void);
};
#pragma pack(pop)

// 1 when type fits at map cell (x, y): its cells inside the map and free.
i16 CanPlaceOverlay(overlayType* type, i16 x, i16 y);
// Places type with its anchor at map cell (x, y); 0 (with a status warning)
// when it does not fit or a limit is reached.
i32 PlaceOverlay(overlayType* type, i16 x, i16 y);
// Puts a resource marker on the mine at (x, y) (checkMine: require the mine's
// marker cell there).
i32 PlaceMineResource(overlayType* type, i16 x, i16 y, i32 checkMine);

// The editor's object table (EDITMGR's data).
extern overlayType gOverlayTypes[];
extern i32 gOverlayCategoryKinds[];
extern i16 gSelectedOverlay;

#endif // HOMM1_EDITOR_OVERLAYMANAGER_H
