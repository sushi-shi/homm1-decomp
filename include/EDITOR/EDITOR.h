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

H1_ENUM_CONST_BEGIN(EditorToolHelpConstant)
    EDITOR_TERRAIN_TOOL_HELP_COUNT = 8,
    EDITOR_CLEAR_TOOL_HELP_COUNT = 2
H1_ENUM_CONST_END(EditorToolHelpConstant)

extern char* gTerrainToolHelp[];
extern char* gClearToolHelp[];

// The random map generator's slider rows.
H1_ENUM_CONST_BEGIN(EditorGeneratorConstant)
    EDITOR_GENERATOR_TERRAIN_COUNT = 7,
    EDITOR_GENERATOR_DENSITY_COUNT = 5
H1_ENUM_CONST_END(EditorGeneratorConstant)

// The cell an eventsManager dialog edits, and the dialog's window.
extern struct editMapCell* gEditCell;
extern class heroWindow* gEditDialog;
// The edited map's header (difficulty, size, name and description).
extern struct SMapHeader* gMapHeader;
// The random map generator's settings (editnew.bin): the share of each
// terrain and the density of each object class, in percent; whether towns are
// scattered rather than centred; whether the map is saved unseen.
extern double gTerrainPercent[EDITOR_GENERATOR_TERRAIN_COUNT];
extern double gDensityPercent[EDITOR_GENERATOR_DENSITY_COUNT];
extern i32 gScatterTowns;
extern i32 gSaveUnseen;

void ShowStatusText(char* text);
void ClearStatusText(void);

#endif // HOMM1_EDITOR_EDITOR_H
