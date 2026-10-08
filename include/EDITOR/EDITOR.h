#ifndef HOMM1_EDITOR_EDITOR_H
#define HOMM1_EDITOR_EDITOR_H

// The editor program unit's own interface (src/EDITOR/EDITOR.cpp); the
// functions it shares with the game keep their SOURCE/KB.h declarations.

#include <Domains.h>
#include <SOURCE/terrainTypes.h>

// The terrains the terrain tool paints and the generator mixes
// (TerrainType).
H1_ENUM_CONST_BEGIN(EditorTerrainConstant)
    EDITOR_TERRAIN_COUNT = H1_ENUM_ENCODE(TerrainType, TERRAIN_LAST) + 1
H1_ENUM_CONST_END(EditorTerrainConstant)

// The status bar: the bottom 16-pixel row of the 640x480 screen.
H1_ENUM_CONST_BEGIN(EditorStatusBarConstant)
    EDITOR_STATUS_BAR_X = 0,
    EDITOR_STATUS_BAR_Y = 464,
    EDITOR_STATUS_BAR_WIDTH = 480,
    EDITOR_STATUS_BAR_HEIGHT = 16,
    EDITOR_STATUS_TEXT_CAPACITY = 200,
    // ShowStatusText keeps the text this long.
    EDITOR_STATUS_TEXT_HOLD_MILLISECONDS = 3000,
    // ShowStatusWarning's text is cleared after this long.
    EDITOR_STATUS_WARNING_MILLISECONDS = 1500,
    // gStatusTextClearTime when the text stays until it is replaced.
    EDITOR_STATUS_TEXT_KEPT = 0
H1_ENUM_CONST_END(EditorStatusBarConstant)

extern b8 gCommandLineInterpreted;
extern b32 gStatusTextShown;
// When the status bar text is cleared (EDITOR_STATUS_TEXT_KEPT: kept until
// replaced).
extern i32 gStatusTextClearTime;
#define gStatusTextHoldTime giStatusTextWaitTime // spelling fixes .bss order
extern i32 gStatusTextHoldTime;
#define gStatusText gStatusTxt // spelling fixes .bss order
extern char gStatusText[];

// gClearFlags' initial value: the first fourteen object classes.
H1_ENUM_CONST_BEGIN(EditorClearConstant)
    EDITOR_CLEAR_FLAGS_DEFAULT = 0x3fff
H1_ENUM_CONST_END(EditorClearConstant)

// The map rectangle a drag selects (gSelectionX < 0: none); the map view
// outlines it.
extern i32 gSelectionX;
#define gSelectionY gnSelRow // spelling fixes .bss order
extern i32 gSelectionY;
#define gSelectionWidth gSelectedWidth // spelling fixes .bss order
extern i32 gSelectionWidth;
#define gSelectionHeight gcSelectionRows // spelling fixes .bss order
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

// editManager::Main's right-click help: gEditButtonHelp for editwind.bin's
// buttons (every scroll arrow shares one entry), gEditAreaHelp for its areas
// (both tracks and knobs share one); entry 0 is empty in both.
H1_ENUM_BEGIN(EditButtonHelp)
    EDIT_BUTTON_HELP_NONE = -1,
    EDIT_BUTTON_HELP_SCROLL = 1,
    EDIT_BUTTON_HELP_ZOOM = 2,
    EDIT_BUTTON_HELP_UNDO = 3,
    EDIT_BUTTON_HELP_MAP_INFO = 4,
    EDIT_BUTTON_HELP_NEW = 5,
    EDIT_BUTTON_HELP_LOAD = 6,
    EDIT_BUTTON_HELP_SAVE = 7,
    EDIT_BUTTON_HELP_QUIT = 8,
    EDIT_BUTTON_HELP_RANDOM_MAP = 9
H1_ENUM_END(EditButtonHelp)

H1_ENUM_BEGIN(EditAreaHelp)
    EDIT_AREA_HELP_RADAR = 1,
    EDIT_AREA_HELP_SCROLLING = 2,
    EDIT_AREA_HELP_TERRAIN = 3,
    EDIT_AREA_HELP_OBJECTS = 4,
    EDIT_AREA_HELP_DETAILS = 5,
    EDIT_AREA_HELP_ERASER = 6,
    EDIT_AREA_HELP_MAP = 7
H1_ENUM_END(EditAreaHelp)

H1_ENUM_CONST_BEGIN(EditorToolHelpConstant)
    EDITOR_TERRAIN_TOOL_HELP_COUNT = EDITOR_TERRAIN_COUNT + 1,
    EDITOR_CLEAR_TOOL_HELP_COUNT = 2,
    EDITOR_OVERLAY_TOOL_HELP_COUNT = 2,
    EDITOR_BUTTON_HELP_COUNT = 10,
    EDITOR_AREA_HELP_COUNT = 8
H1_ENUM_CONST_END(EditorToolHelpConstant)

extern H1_ENUM_ARRAY(char*, gTerrainToolHelp, TerrainToolHelp, EDITOR_TERRAIN_TOOL_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gClearToolHelp, ClearToolHelp, EDITOR_CLEAR_TOOL_HELP_COUNT);

// The object tool's preview-border help and category names.
extern H1_ENUM_ARRAY(char*, gOverlayToolHelp, OverlayToolHelp, EDITOR_OVERLAY_TOOL_HELP_COUNT);
extern char* gOverlayCategoryNames[];
// The category the object tool places from and the one its panel shows.
extern i32 gOverlayCategory;
extern i32 gOverlayShownCategory;
// The id of the last placed object (editManager::m_cellPairs).
extern i16 gNextObjectId;

// The random map generator's slider rows.
H1_ENUM_CONST_BEGIN(EditorGeneratorConstant)
    EDITOR_GENERATOR_DENSITY_COUNT = 5
H1_ENUM_CONST_END(EditorGeneratorConstant)

// The cell an eventsManager dialog edits, and the dialog's window.
#define gEditCell gpCell // spelling fixes .bss order
extern class mapCell* gEditCell;
#define gEditDialog gEditDlg // spelling fixes .bss order
extern class heroWindow* gEditDialog;
// The edited map's header (difficulty, size, name and description).
#define gMapHeader gpMapHeader // spelling fixes .bss order
extern struct SMapHeader* gMapHeader;
// The random map generator's settings (editnew.bin): the share of each
// terrain and the density of each object class, in percent; whether towns are
// scattered rather than centred; whether the map is saved unseen.
extern double gTerrainPercent[EDITOR_TERRAIN_COUNT];
extern double gDensityPercent[EDITOR_GENERATOR_DENSITY_COUNT];
extern b32 gScatterTowns;
extern i32 gSaveUnseen;
// gDensityPercent's rows.
H1_ENUM_BEGIN(GeneratorDensity)
    GENERATOR_DENSITY_MOUNTAINS = 0,
    GENERATOR_DENSITY_TREES = 1,
    GENERATOR_DENSITY_OBJECTS = 2,
    GENERATOR_DENSITY_TREASURE = 3,
    GENERATOR_DENSITY_MONSTERS = 4
H1_ENUM_END(GeneratorDensity)
// The terrain names the generator's status line shows.
extern char* gGeneratorTerrainNames[];
// RemoveSmallRegions counts the map's land cells here.
#define gLandCellCount gcLandSquares // spelling fixes .bss order
extern i32 gLandCellCount;
// Set while the generator works unseen (gSaveUnseen): the map view draws
// clouds only and the radar black.
extern b32 gGeneratingUnseen;
// Cleared while a map without the editor's format word is loaded: such maps
// keep no object ids, so the eraser clears whole cells.
extern b32 gNewMapFormat;
// The right-click help of editwind.bin's buttons and areas.
extern char* gEditButtonHelp[];
extern char* gEditAreaHelp[];

void ShowStatusText(char* text);
void ClearStatusText(void);

#endif // HOMM1_EDITOR_EDITOR_H
