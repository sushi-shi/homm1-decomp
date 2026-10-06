#ifndef HOMM1_EDITOR_EDITOR_H
#define HOMM1_EDITOR_EDITOR_H

// The editor program unit's own interface (src/EDITOR/EDITOR.cpp); the
// functions it shares with the game keep their SOURCE/KB.h declarations.

#include <Domains.h>

// The status bar: the bottom 16-pixel row of the 640x480 screen.
H1_ENUM_CONST_BEGIN(EditorStatusBarConstant)
    EDITOR_STATUS_BAR_X = 0,
    EDITOR_STATUS_BAR_Y = 464,
    EDITOR_STATUS_BAR_WIDTH = 480,
    EDITOR_STATUS_BAR_HEIGHT = 16,
    EDITOR_STATUS_TEXT_CAPACITY = 200,
    // ShowStatusText keeps the text this long.
    EDITOR_STATUS_TEXT_HOLD_MILLISECONDS = 3000
H1_ENUM_CONST_END(EditorStatusBarConstant)

extern i8 gCommandLineInterpreted;
extern i32 gStatusTextShown;
// When the status bar text is cleared (0: kept until replaced).
extern i32 gStatusTextClearTime;
extern i32 gStatusTextHoldTime;
extern char gStatusText[];

// gClearFlags' initial value: the first fourteen object classes.
H1_ENUM_CONST_BEGIN(EditorClearConstant)
    EDITOR_CLEAR_FLAGS_DEFAULT = 0x3fff
H1_ENUM_CONST_END(EditorClearConstant)

// The map rectangle a drag selects (gSelectionX < 0: none); the map view
// outlines it.
extern i32 gSelectionX;
extern i32 gSelectionY;
extern i32 gSelectionWidth;
extern i32 gSelectionHeight;
// The object classes the eraser removes (one bit per clearwin.bin toggle).
extern i32 gClearFlags;
// The tool panels' right-click help, indexed by the help ids below (0: none).
H1_ENUM_BEGIN(TerrainToolHelp)
    TERRAIN_TOOL_HELP_NONE = -1,
    TERRAIN_TOOL_HELP_WATER = 1,
    TERRAIN_TOOL_HELP_GRASS = 2,
    TERRAIN_TOOL_HELP_SNOW = 3,
    TERRAIN_TOOL_HELP_SWAMP = 4,
    TERRAIN_TOOL_HELP_LAVA = 5,
    TERRAIN_TOOL_HELP_DESERT = 6,
    TERRAIN_TOOL_HELP_DIRT = 7
H1_ENUM_END(TerrainToolHelp)

H1_ENUM_BEGIN(ClearToolHelp)
    CLEAR_TOOL_HELP_OPTIONS = 1
H1_ENUM_END(ClearToolHelp)

H1_ENUM_BEGIN(OverlayToolHelp)
    OVERLAY_TOOL_HELP_NONE = -1,
    OVERLAY_TOOL_HELP_SELECTED = 1
H1_ENUM_END(OverlayToolHelp)

H1_ENUM_CONST_BEGIN(EditorToolHelpConstant)
    EDITOR_TERRAIN_TOOL_HELP_COUNT = 8,
    EDITOR_CLEAR_TOOL_HELP_COUNT = 2,
    EDITOR_OVERLAY_TOOL_HELP_COUNT = 2
H1_ENUM_CONST_END(EditorToolHelpConstant)

extern char* gTerrainToolHelp[];
extern char* gClearToolHelp[];

// The object tool's preview-border help and category names.
extern char* gOverlayToolHelp[];
extern char* gOverlayCategoryNames[];
// The category the object tool places from and the one its panel shows.
extern i32 gOverlayCategory;
extern i32 gOverlayShownCategory;
// The id of the last placed object (editManager::m_cellPairs).
extern i16 gNextObjectId;

void ShowStatusText(char* text);
void ClearStatusText(void);

#endif // HOMM1_EDITOR_EDITOR_H
