#ifndef HOMM1_EDITOR_EDITOR_H
#define HOMM1_EDITOR_EDITOR_H

#include <SOURCE/terrainTypes.h>

enum EditorTerrainConstant {
    EDITOR_TERRAIN_COUNT = TERRAIN_LAST + 1
};

enum EditorStatusBarConstant {
    EDITOR_STATUS_BAR_X = 0,
    EDITOR_STATUS_BAR_Y = 464,
    EDITOR_STATUS_BAR_WIDTH = 480,
    EDITOR_STATUS_BAR_HEIGHT = 16,
    EDITOR_STATUS_TEXT_CAPACITY = 200,
    EDITOR_STATUS_TEXT_HOLD_MILLISECONDS = 3000,
    EDITOR_STATUS_WARNING_MILLISECONDS = 1500,
    EDITOR_STATUS_TEXT_KEPT = 0
};

extern b8 gCommandLineInterpreted;
extern b32 gStatusTextShown;
extern i32 gStatusTextClearTime;
extern i32 gStatusTextHoldTime;
extern char gStatusText[];

enum EditorClearConstant {
    EDITOR_CLEAR_FLAGS_DEFAULT = 0x3fff
};

extern i32 gSelectionX;
extern i32 gSelectionY;
extern i32 gSelectionWidth;
extern i32 gSelectionHeight;
extern i32 gClearFlags;
enum TerrainToolHelp {
    TERRAIN_TOOL_HELP_NONE = -1,
    TERRAIN_TOOL_HELP_WATER = 1,
    TERRAIN_TOOL_HELP_GRASS = 2,
    TERRAIN_TOOL_HELP_SNOW = 3,
    TERRAIN_TOOL_HELP_SWAMP = 4,
    TERRAIN_TOOL_HELP_LAVA = 5,
    TERRAIN_TOOL_HELP_DESERT = 6,
    TERRAIN_TOOL_HELP_DIRT = 7
};

enum ClearToolHelp {
    CLEAR_TOOL_HELP_OPTIONS = 1
};

enum OverlayToolHelp {
    OVERLAY_TOOL_HELP_NONE = -1,
    OVERLAY_TOOL_HELP_SELECTED = 1
};

enum EditButtonHelp {
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
};

enum EditAreaHelp {
    EDIT_AREA_HELP_RADAR = 1,
    EDIT_AREA_HELP_SCROLLING = 2,
    EDIT_AREA_HELP_TERRAIN = 3,
    EDIT_AREA_HELP_OBJECTS = 4,
    EDIT_AREA_HELP_DETAILS = 5,
    EDIT_AREA_HELP_ERASER = 6,
    EDIT_AREA_HELP_MAP = 7
};

enum EditorToolHelpConstant {
    EDITOR_TERRAIN_TOOL_HELP_COUNT = EDITOR_TERRAIN_COUNT + 1,
    EDITOR_CLEAR_TOOL_HELP_COUNT = 2,
    EDITOR_OVERLAY_TOOL_HELP_COUNT = 2,
    EDITOR_BUTTON_HELP_COUNT = 10,
    EDITOR_AREA_HELP_COUNT = 8
};

extern char* gTerrainToolHelp[EDITOR_TERRAIN_TOOL_HELP_COUNT];
extern char* gClearToolHelp[EDITOR_CLEAR_TOOL_HELP_COUNT];

extern char* gOverlayToolHelp[EDITOR_OVERLAY_TOOL_HELP_COUNT];
extern char* gOverlayCategoryNames[];
extern i32 gOverlayCategory;
extern i32 gOverlayShownCategory;
extern i16 gNextObjectId;

enum EditorGeneratorConstant {
    EDITOR_GENERATOR_DENSITY_COUNT = 5
};

extern class mapCell* gEditCell;
extern class heroWindow* gEditDialog;
extern struct SMapHeader* gMapHeader;
extern double gTerrainPercent[EDITOR_TERRAIN_COUNT];
extern double gDensityPercent[EDITOR_GENERATOR_DENSITY_COUNT];
extern b32 gScatterTowns;
extern i32 gSaveUnseen;
enum GeneratorDensity {
    GENERATOR_DENSITY_MOUNTAINS = 0,
    GENERATOR_DENSITY_TREES = 1,
    GENERATOR_DENSITY_OBJECTS = 2,
    GENERATOR_DENSITY_TREASURE = 3,
    GENERATOR_DENSITY_MONSTERS = 4
};
extern char* gGeneratorTerrainNames[];
extern i32 gLandCellCount;
extern b32 gGeneratingMaps;
extern b32 gNewMapFormat;
extern char* gEditButtonHelp[];
extern char* gEditAreaHelp[];

void ShowStatusText(char* text);
void ClearStatusText(void);

#endif
