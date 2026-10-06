#ifndef HOMM1_EDITOR_TERRAINMANAGER_H
#define HOMM1_EDITOR_TERRAINMANAGER_H

// The terrain painting tool (src/EDITOR/TERRMGR.cpp): editManager::SelectTool
// runs it as the tool manager. The unit name TERRMGR is descriptive; Open
// stores the class name "terrainManager".

#include <BASE/baseManager.h>
#include <Domains.h>
#include <EDITOR/EDITOR.h>
#include <H1/Macros.h>
#include <SOURCE/terrainTypes.h>

class backdropWidget;
class iconWidget;
struct tag_message;

// The tool panel: a column of seven terrains.icn buttons and a highlight
// that frames the selected one.
H1_ENUM_CONST_BEGIN(TerrainManagerLayout)
    TERRAIN_BUTTON_X = 488,
    TERRAIN_BUTTON_FIRST_Y = 204,
    TERRAIN_BUTTON_WIDTH = 96,
    TERRAIN_BUTTON_HEIGHT = 18,
    TERRAIN_HIGHLIGHT_FRAME = 7,
    TERRAIN_HIGHLIGHT_FILL_COLOR = 0x76,
    TERRAIN_BACKDROP_Y = 160,
    TERRAIN_BACKDROP_KIND = 0x20
H1_ENUM_CONST_END(TerrainManagerLayout)

H1_ENUM_ID_BEGIN(TerrainManagerWidgetId)
TERRAIN_HIGHLIGHT_WIDGET = 0x19,
    TERRAIN_BUTTON_WATER = 0x33, TERRAIN_BUTTON_GRASS = 0x34, TERRAIN_BUTTON_SNOW = 0x35,
    TERRAIN_BUTTON_SWAMP = 0x36, TERRAIN_BUTTON_LAVA = 0x37, TERRAIN_BUTTON_DESERT = 0x38,
    TERRAIN_BUTTON_DIRT = 0x39 H1_ENUM_ID_END(TerrainManagerWidgetId)

    // Main's drag modes: shift paints the cells the cursor crosses, control a
    // brush of TERRAIN_BRUSH_SIZE cells square (fewer at the map's last row and
    // column), a plain drag fills the spanned rectangle.
    H1_ENUM_CONST_BEGIN(TerrainBrushConstant)
    TERRAIN_BRUSH_SIZE = 2
H1_ENUM_CONST_END(TerrainBrushConstant)

H1_ENUM_BEGIN(TerrainDragMode)
    TERRAIN_DRAG_CELLS = 0,
    TERRAIN_DRAG_RECTANGLE = 1,
    TERRAIN_DRAG_BRUSH = 2
H1_ENUM_END(TerrainDragMode)

#pragma pack(push, 1)
class terrainManager : public baseManager {
public:
    H1_ENUM_STORAGE(TerrainType, u8) m_terrain;
    H1_ENUM_ARRAY(iconWidget*, m_terrainButtons, TerrainType, EDITOR_TERRAIN_COUNT);
    backdropWidget* m_backdrop;
    iconWidget* m_highlight;
    iconWidget* m_panel;
    // The last map cell a drag step visited.
    i16 m_lastX;
    i16 m_lastY;
    // Main's message.type mask (MessageType bits, as the game's managers).
    i16 m_dispatchMask;

    terrainManager(void);
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(tag_message& message) OVERRIDE;
    // Refits every terrain's edges, then rerolls each plain ground tile to
    // one of its terrain's variants (the R key).
    void RandomizeTiles(void);
    void SelectTerrain(H1_ENUM_PARAM(TerrainType, i16) terrain);
};
#pragma pack(pop)

// The terrain the tool last selected; Open restores it.
extern H1_ENUM_STORAGE(TerrainType, i32) gLastTerrain;

#endif // HOMM1_EDITOR_TERRAINMANAGER_H
