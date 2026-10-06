#ifndef HOMM1_EDITOR_TERRAINMANAGER_H
#define HOMM1_EDITOR_TERRAINMANAGER_H

#include <BASE/baseManager.h>
#include <EDITOR/EDITOR.h>
#include <SOURCE/terrainTypes.h>

class backdropWidget;
class iconWidget;
struct tag_message;

enum TerrainManagerLayout {
    TERRAIN_BUTTON_X = 488,
    TERRAIN_BUTTON_FIRST_Y = 204,
    TERRAIN_BUTTON_WIDTH = 96,
    TERRAIN_BUTTON_HEIGHT = 18,
    TERRAIN_HIGHLIGHT_FRAME = 7,
    TERRAIN_HIGHLIGHT_FILL_COLOR = 0x76,
    TERRAIN_BACKDROP_Y = 160,
    TERRAIN_BACKDROP_KIND = 0x20
};

enum TerrainManagerWidgetId {
TERRAIN_HIGHLIGHT_WIDGET = 0x19,
    TERRAIN_BUTTON_WATER = 0x33, TERRAIN_BUTTON_GRASS = 0x34, TERRAIN_BUTTON_SNOW = 0x35,
    TERRAIN_BUTTON_SWAMP = 0x36, TERRAIN_BUTTON_LAVA = 0x37, TERRAIN_BUTTON_DESERT = 0x38,
    TERRAIN_BUTTON_DIRT = 0x39 };

    enum TerrainBrushConstant {
    TERRAIN_BRUSH_SIZE = 2
};

enum TerrainDragMode {
    TERRAIN_DRAG_CELLS = 0,
    TERRAIN_DRAG_RECTANGLE = 1,
    TERRAIN_DRAG_BRUSH = 2
};

#pragma pack(push, 1)
class terrainManager : public baseManager {
public:
    u8 m_terrain;
    iconWidget* m_terrainButtons[EDITOR_TERRAIN_COUNT];
    backdropWidget* m_backdrop;
    iconWidget* m_highlight;
    iconWidget* m_panel;
    i16 m_lastX;
    i16 m_lastY;
    i16 m_dispatchMask;

    terrainManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(tag_message& message) ;
    void RandomizeTiles(void);
    void SelectTerrain(i16 terrain);
};
#pragma pack(pop)

extern i32 gLastTerrain;

#endif
