#ifndef HOMM1_EDITOR_OVERLAYMANAGER_H
#define HOMM1_EDITOR_OVERLAYMANAGER_H

#include <BASE/baseManager.h>
#include <SOURCE/game.h>
#include <SOURCE/mapCell.h>

class border;
class button;
class font;
class icon;
class iconWidget;
struct tag_message;

enum OverlayTypeConstant {
    OVERLAY_TYPE_NAME_LENGTH = 9,
    OVERLAY_FOOTPRINT_ROWS = 3,
    OVERLAY_FOOTPRINT_COLUMNS = 5,
    OVERLAY_FOOTPRINT_CELLS = 16,
    OVERLAY_FOOTPRINT_CORNER = 15,
    OVERLAY_FOOTPRINT_CORNER_BIT = 0x8000,
    OVERLAY_FOOTPRINT_CORNER_COLUMN = 2,
    OVERLAY_FOOTPRINT_CORNER_ROW = 3,
    OVERLAY_FOOTPRINT_HEIGHT = OVERLAY_FOOTPRINT_CORNER_ROW + 1,
    OVERLAY_FOOTPRINT_ANCHOR = 0,
    OVERLAY_FOOTPRINT_ROW_0_MASK = 0x001f,
    OVERLAY_FOOTPRINT_ROW_1_MASK = 0x03e0,
    OVERLAY_FOOTPRINT_ROW_2_MASK = 0x7c00,
    OVERLAY_FOOTPRINT_COLUMN_0_MASK = 0x0421,
    OVERLAY_FOOTPRINT_COLUMN_1_MASK = 0x0842,
    OVERLAY_FOOTPRINT_COLUMN_2_MASK = 0x1084 | OVERLAY_FOOTPRINT_CORNER_BIT,
    OVERLAY_FOOTPRINT_COLUMN_3_MASK = 0x2108,
    OVERLAY_FOOTPRINT_COLUMN_4_MASK = 0x4210,
    OVERLAY_TYPE_RESOURCE_MARKER = 1,
    OVERLAY_TYPE_SHOWS_RESOURCE = 2,
    OVERLAY_SHOWN_RESOURCE_COLUMN = 1,
    OVERLAY_SHOWN_RESOURCE_ROW = 0,
    OVERLAY_TERRAIN_MASK_ANY = 0xff,
    OVERLAY_TYPE_COUNT = 295
};

#define OVERLAY_FOOTPRINT_CELL(column, row) ((row) * OVERLAY_FOOTPRINT_COLUMNS + (column))
#define OVERLAY_CELL_BIT(cell) (1 << (cell))
#define OVERLAY_FOOTPRINT_BIT(column, row) OVERLAY_CELL_BIT(OVERLAY_FOOTPRINT_CELL(column, row))
#define OVERLAY_TERRAIN_BIT(terrain) (1 << (terrain))

enum OverlayKind {
    OVERLAY_KIND_TERRAIN = 0,
    OVERLAY_KIND_TOWN = 1,
    OVERLAY_KIND_MONSTER = 2,
    OVERLAY_KIND_ARTIFACT = 3,
    OVERLAY_KIND_TREASURE = 4
};

#pragma pack(push, 1)
struct overlayType {
    char name[OVERLAY_TYPE_NAME_LENGTH];
    i8 tileset;
    i8 kind;
    u16 frequency;
    u16 footprintMask;
    u8 terrainMask;
    u16 overlayMask;
    u16 blockMask;
    u16 animatedMask;
    u16 resourceMask;
    u8 resourceFrame;
    u8 flags;
    u16 eventMask;
    u8 trigger;
    u8 frames[OVERLAY_FOOTPRINT_CELLS];
};
#pragma pack(pop)

enum OverlayManagerConstant {
    OVERLAY_MANAGER_TYPE_CAPACITY = 128,
    OVERLAY_CATEGORY_COUNT = 11,
    OVERLAY_NO_SELECTION = -1,
    OVERLAY_MINE_LIMIT = 34,
    OVERLAY_ARTIFACT_LIMIT = 32
};

enum OverlayControlId {
    OVERLAY_PREVIEW_BORDER = 0x26,
    OVERLAY_NEXT_CATEGORY_BUTTON = 0x28,
    OVERLAY_PREVIOUS_CATEGORY_BUTTON = 0x29
};

enum OverlayManagerLayout {
    OVERLAY_PREVIEW_X = 509,
    OVERLAY_PREVIEW_Y = 227,
    OVERLAY_PREVIEW_WIDTH = 86,
    OVERLAY_PREVIEW_HEIGHT = 70,
    OVERLAY_PREVIEW_FRAME = 2,
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
    OVERLAY_FOOTPRINT_OVERLAY_COLOR = 0x65,
    OVERLAY_FOOTPRINT_GROUND_COLOR = 0xc7
};

enum OverlayPickerLayout {
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
};

#pragma pack(push, 1)
class overlayManager : public baseManager {
public:
    iconWidget* m_panel;
    icon* m_icon;
    overlayType m_types[OVERLAY_MANAGER_TYPE_CAPACITY];
    i16 m_typeCount;
    i16 m_width;
    i16 m_height;
    border* m_previewBorder;
    button* m_nextButton;
    button* m_previousButton;
    u8 m_unused16ca[4];
    font* m_font;
    b32 m_previewDrawn;
    i16 m_dispatchMask;

    overlayManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(tag_message& message) ;
    void DrawFootprint(i16 x, i16 y, i16 footprintMask, i16 overlayMask, i16 width, i16 height);
    void DrawOverlay(overlayType* type, i16 x, i16 y, i16 width, i16 height, b32 update);
    i16 LoadCategory(i16 category);
    i16 PickOverlay(i16 category);
    void MeasureOverlay(overlayType* type);
    void DrawCategoryName(b32 update);
    void DrawSelectedOverlay(void);
};
#pragma pack(pop)

i16 CanPlaceOverlay(overlayType* type, i16 x, i16 y);
b32 PlaceOverlay(overlayType* type, i16 x, i16 y);
b32 PlaceMineResource(overlayType* type, i16 x, i16 y, b32 checkMine);

extern overlayType gOverlayTypes[];
extern i32 gOverlayCategoryKinds[];
extern i16 gSelectedOverlay;

#endif
