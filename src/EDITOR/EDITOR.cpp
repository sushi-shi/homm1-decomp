// The editor's program unit (Editor\EDITOR.CPP): start-up, shut-down, the
// main classes, the shared delay and dialog helpers, the status bar and the
// application-menu hooks kbwin calls. It keeps the game's KB.cpp names for
// the functions both programs define.
// Descriptive names: ShowStatusText, ClearStatusText, gStatusTextShown,
// gStatusTextClearTime, gStatusTextHoldTime, gStatusText,
// gCommandLineInterpreted, gNewMapFormat, gGeneratingMaps, gEditButtonHelp,
// gEditAreaHelp, EditorStartupHook, EditorIdleHook, IncrementArgumentA,
// IncrementArgumentB.

#include <match.h>

#include <EDITOR/EDITOR.h>

#include <BASE/baseManager.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/textWidget.h>
#include <EDITOR/editManager.h>
#include <EDITOR/eventsManager.h>
#include <EDITOR/overlayManager.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/wingraph.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Editor\\EDITOR.CPP"

// The editor's initialized data, in retail order: the definition order is
// the .data order. EDITOR.CPP began as a copy of the game's KB.CPP and keeps
// its paths, menu table and text tables beside the editor's own.
DATA(0x0043eda0)
i16 gRadarTerrainColor[24] = {
    82,  99, 7,   180, 26,  123, 55, 0,  16, 48, 98, 160,
    126, 74, 110, 179, 100, 218, 12, 12, 12, 12, 12, 12,
};
DATA(0x0043edd0)
i16 gVesaMode[6] = {640, 480, 256, 20226, 257, 0};
DATA(0x0043eddc)
i32 gShowIt = 1;
DATA(0x0043ede0)
i32 gEnlargeScreenBlit = 1;
DATA(0x0043ede4)
i8 gCommandLineInterpreted = 1;
DATA(0x0043ede5)
i8 gShowMapInfo = 1;
// No retail code reads this; it holds its retail .data place.
DATA(0x0043ede8)
i32 gUnusedData43ede8 = 1;
DATA(0x0043edec)
char gDataPath[352] = ".\\DATA\\";
DATA(0x0043ef4c)
char gAnimPath[352] = ".\\ANIM\\";
DATA(0x0043f0ac)
char gSoundPath[352] = ".\\SOUND\\";
DATA(0x0043f20c)
char gTracksPath[352] = "\\TRACKS\\";
DATA(0x0043f36c)
char gGamePath[20] = ".\\GAMES\\";
DATA(0x0043f380)
char gMapPath[20] = ".\\MAPS\\";
DATA(0x0043f394)
i32 gCurExe = CONFIG_EXECUTABLE_EDITOR;
DATA(0x0043f398)
i32 gNewMapFormat = 1;
// The tool units' shared state (CLEARMGR, OVERLAY, TERRMGR and EDITMGR read it).
DATA(0x0043f39c)
i32 gClearFlags = EDITOR_CLEAR_FLAGS_DEFAULT;
DATA(0x0043f3a0)
i32 gSelectionX = -1;
// The random map generator's settings (EVENTMGR's editnew.bin and MAPOBJ).
DATA(0x0043f3a8)
double gTerrainPercent[EDITOR_TERRAIN_COUNT] = {30.0, 30.0, 20.0, 0.0, 0.0, 0.0, 20.0};
DATA(0x0043f3e0)
double gDensityPercent[EDITOR_GENERATOR_DENSITY_COUNT] = {50.0, 50.0, 50.0, 50.0, 50.0};
DATA(0x0043f408)
i32 gScatterTowns = 1;
DATA(0x0043f410)
SMenuEnableStatus gMenuEnableStatus[70] = {
    {0, 0, 0, 0},     {40005, 1, 1, 0}, {40006, 1, 1, 0}, {40007, 1, 1, 0}, {40008, 1, 1, 0},
    {40009, 1, 1, 0}, {40012, 0, 0, 0}, {40013, 0, 0, 0}, {40014, 0, 0, 0}, {40015, 0, 0, 0},
    {40016, 1, 0, 0}, {40017, 1, 0, 0}, {40018, 1, 0, 0}, {40019, 1, 0, 0}, {40020, 1, 0, 0},
    {40021, 1, 0, 0}, {40022, 1, 0, 0}, {40023, 1, 0, 0}, {40024, 1, 0, 0}, {40025, 1, 0, 0},
    {40026, 1, 0, 0}, {40028, 1, 0, 0}, {40029, 1, 0, 0}, {40030, 1, 0, 0}, {40031, 1, 0, 0},
    {40032, 1, 0, 0}, {40033, 1, 0, 0}, {40034, 1, 0, 0}, {40035, 1, 0, 0}, {40036, 1, 0, 0},
    {40037, 1, 0, 0}, {40038, 1, 0, 0}, {40040, 0, 0, 0}, {40041, 0, 0, 0}, {40042, 0, 0, 0},
    {40043, 0, 0, 0}, {40044, 0, 0, 0}, {40045, 0, 0, 0}, {40046, 0, 0, 0}, {40047, 0, 0, 0},
    {40052, 1, 1, 0}, {40053, 1, 1, 0}, {40102, 0, 1, 0}, {40104, 0, 1, 0}, {40105, 0, 1, 0},
    {40106, 0, 1, 0}, {40107, 0, 1, 0}, {40109, 0, 1, 0}, {40110, 0, 1, 0}, {40111, 0, 1, 0},
    {40112, 0, 1, 0}, {40114, 0, 1, 0}, {40115, 0, 1, 0}, {40117, 0, 1, 0}, {40118, 0, 1, 0},
    {40120, 0, 1, 0}, {40121, 0, 1, 0}, {40123, 0, 1, 0}, {40124, 0, 1, 0}, {40127, 0, 1, 0},
    {40128, 0, 1, 0}, {40129, 0, 1, 0}, {40131, 0, 1, 0}, {40132, 0, 1, 0}, {40134, 0, 1, 0},
    {40135, 0, 1, 0}, {40137, 0, 1, 0}, {40138, 0, 1, 0}, {40139, 0, 0, 0}, {40140, 0, 0, 0},
};
// The editor dialogs' captions (SetWinText): widget id and dialog.
DATA(0x0043f5fc)
WindowTextEntry gWinSetup[WINDOW_TEXT_EDITOR_ENTRY_COUNT] = {
    {500, EVENTS_WINDOW_TEXT_TOWN},        {501, EVENTS_WINDOW_TEXT_TOWN},
    {504, EVENTS_WINDOW_TEXT_TOWN},        {505, EVENTS_WINDOW_TEXT_TOWN},
    {506, EVENTS_WINDOW_TEXT_TOWN},        {507, EVENTS_WINDOW_TEXT_TOWN},
    {508, EVENTS_WINDOW_TEXT_TOWN},        {509, EVENTS_WINDOW_TEXT_TOWN},
    {510, EVENTS_WINDOW_TEXT_TOWN},        {511, EVENTS_WINDOW_TEXT_TOWN},
    {561, EVENTS_WINDOW_TEXT_TOWN},        {562, EVENTS_WINDOW_TEXT_TOWN},
    {567, EVENTS_WINDOW_TEXT_TOWN},        {527, EVENTS_WINDOW_TEXT_TOWN},
    {528, EVENTS_WINDOW_TEXT_TOWN},        {529, EVENTS_WINDOW_TEXT_TOWN},
    {530, EVENTS_WINDOW_TEXT_TOWN},        {531, EVENTS_WINDOW_TEXT_TOWN},
    {532, EVENTS_WINDOW_TEXT_TOWN},        {533, EVENTS_WINDOW_TEXT_TOWN},
    {534, EVENTS_WINDOW_TEXT_TOWN},        {535, EVENTS_WINDOW_TEXT_TOWN},
    {536, EVENTS_WINDOW_TEXT_TOWN},        {537, EVENTS_WINDOW_TEXT_TOWN},
    {500, EVENTS_WINDOW_TEXT_MONSTER},     {500, EVENTS_WINDOW_TEXT_HERO},
    {504, EVENTS_WINDOW_TEXT_HERO},        {505, EVENTS_WINDOW_TEXT_HERO},
    {506, EVENTS_WINDOW_TEXT_HERO},        {507, EVENTS_WINDOW_TEXT_HERO},
    {508, EVENTS_WINDOW_TEXT_HERO},        {509, EVENTS_WINDOW_TEXT_HERO},
    {510, EVENTS_WINDOW_TEXT_HERO},        {511, EVENTS_WINDOW_TEXT_HERO},
    {561, EVENTS_WINDOW_TEXT_HERO},        {527, EVENTS_WINDOW_TEXT_HERO},
    {528, EVENTS_WINDOW_TEXT_HERO},        {529, EVENTS_WINDOW_TEXT_HERO},
    {530, EVENTS_WINDOW_TEXT_HERO},        {531, EVENTS_WINDOW_TEXT_HERO},
    {532, EVENTS_WINDOW_TEXT_HERO},        {101, EVENTS_WINDOW_TEXT_CLEAR},
    {102, EVENTS_WINDOW_TEXT_CLEAR},       {103, EVENTS_WINDOW_TEXT_CLEAR},
    {104, EVENTS_WINDOW_TEXT_CLEAR},       {101, EVENTS_WINDOW_TEXT_MAP_DETAILS},
    {102, EVENTS_WINDOW_TEXT_MAP_DETAILS}, {103, EVENTS_WINDOW_TEXT_MAP_DETAILS},
    {104, EVENTS_WINDOW_TEXT_MAP_DETAILS}, {105, EVENTS_WINDOW_TEXT_MAP_DETAILS},
    {320, EVENTS_WINDOW_TEXT_NEW_MAP},     {321, EVENTS_WINDOW_TEXT_NEW_MAP},
    {300, EVENTS_WINDOW_TEXT_NEW_MAP},     {301, EVENTS_WINDOW_TEXT_NEW_MAP},
    {302, EVENTS_WINDOW_TEXT_NEW_MAP},     {303, EVENTS_WINDOW_TEXT_NEW_MAP},
    {304, EVENTS_WINDOW_TEXT_NEW_MAP},     {305, EVENTS_WINDOW_TEXT_NEW_MAP},
    {306, EVENTS_WINDOW_TEXT_NEW_MAP},     {820, EVENTS_WINDOW_TEXT_NEW_MAP},
    {821, EVENTS_WINDOW_TEXT_NEW_MAP},     {800, EVENTS_WINDOW_TEXT_NEW_MAP},
    {801, EVENTS_WINDOW_TEXT_NEW_MAP},     {802, EVENTS_WINDOW_TEXT_NEW_MAP},
    {803, EVENTS_WINDOW_TEXT_NEW_MAP},     {804, EVENTS_WINDOW_TEXT_NEW_MAP},
    {1200, EVENTS_WINDOW_TEXT_NEW_MAP},    {1201, EVENTS_WINDOW_TEXT_NEW_MAP},
    {1202, EVENTS_WINDOW_TEXT_NEW_MAP},    {1400, EVENTS_WINDOW_TEXT_NEW_MAP},
};
DATA(0x0043f714)
char* gTerrainToolHelp[EDITOR_TERRAIN_TOOL_HELP_COUNT] = {
    "",
    localization::Tr("editor.terrain.help.water"),
    localization::Tr("editor.terrain.help.grass"),
    localization::Tr("editor.terrain.help.snow"),
    localization::Tr("editor.terrain.help.swamp"),
    localization::Tr("editor.terrain.help.lava"),
    localization::Tr("editor.terrain.help.desert"),
    localization::Tr("editor.terrain.help.dirt")
};
DATA(0x0043f734)
char* gClearToolHelp[EDITOR_CLEAR_TOOL_HELP_COUNT] = {
    "",
    localization::Tr("editor.clear.options.help")
};
DATA(0x0043f73c)
char* gOverlayToolHelp[EDITOR_OVERLAY_TOOL_HELP_COUNT] = {
    "",
    localization::Tr("editor.overlay.selected.help")
};
DATA(0x0043f744)
char* gEditButtonHelp[EDITOR_BUTTON_HELP_COUNT] = {
    "",
    localization::Tr("table.gEditButtonHelp.1"),
    localization::Tr("table.gEditButtonHelp.2"),
    localization::Tr("table.gEditButtonHelp.3"),
    localization::Tr("table.gEditButtonHelp.4"),
    localization::Tr("table.gEditButtonHelp.5"),
    localization::Tr("table.gEditButtonHelp.6"),
    localization::Tr("table.gEditButtonHelp.7"),
    localization::Tr("table.gEditButtonHelp.8"),
    localization::Tr("table.gEditButtonHelp.9"),
};
DATA(0x0043f76c)
char* gEditAreaHelp[EDITOR_AREA_HELP_COUNT] = {
    "",
    localization::Tr("table.gEditAreaHelp.1"),
    localization::Tr("table.gEditAreaHelp.2"),
    localization::Tr("table.gEditAreaHelp.3"),
    localization::Tr("table.gEditAreaHelp.4"),
    localization::Tr("table.gEditAreaHelp.5"),
    localization::Tr("table.gEditAreaHelp.6"),
    localization::Tr("table.gEditAreaHelp.7"),
};
DATA(0x0043f78c)
char* gGeneratorTerrainNames[EDITOR_TERRAIN_COUNT] = {
    localization::Tr("editor.terrain.name.0"),
    localization::Tr("editor.terrain.name.1"),
    localization::Tr("editor.terrain.name.2"),
    localization::Tr("editor.terrain.name.3"),
    localization::Tr("editor.terrain.name.4"),
    localization::Tr("editor.terrain.name.5"),
    localization::Tr("editor.terrain.name.6")
};
DATA(0x0043f7a8)
char* gOverlayCategoryNames[OVERLAY_CATEGORY_COUNT] = {
    localization::Tr("editor.overlay.category.0"),
    localization::Tr("editor.overlay.category.1"),
    localization::Tr("editor.overlay.category.2"),
    localization::Tr("editor.overlay.category.3"),
    localization::Tr("editor.overlay.category.4"),
    localization::Tr("editor.overlay.category.5"),
    localization::Tr("editor.overlay.category.6"),
    localization::Tr("editor.overlay.category.7"),
    localization::Tr("editor.overlay.category.8"),
    localization::Tr("editor.overlay.category.9"),
    localization::Tr("editor.overlay.category.10")
};
DATA(0x0043f7d4)
char* gWinSetupText[WINDOW_TEXT_EDITOR_ENTRY_COUNT] = {
    localization::Tr("editor.table.gWinSetupText.0"),
    localization::Tr("editor.table.gWinSetupText.1"),
    localization::Tr("editor.table.gWinSetupText.2"),
    localization::Tr("editor.table.gWinSetupText.3"),
    localization::Tr("editor.table.gWinSetupText.4"),
    localization::Tr("editor.table.gWinSetupText.5"),
    localization::Tr("editor.table.gWinSetupText.6"),
    localization::Tr("editor.table.gWinSetupText.7"),
    localization::Tr("editor.table.gWinSetupText.8"),
    localization::Tr("editor.table.gWinSetupText.9"),
    localization::Tr("editor.table.gWinSetupText.10"),
    localization::Tr("editor.table.gWinSetupText.11"),
    localization::Tr("editor.table.gWinSetupText.12"),
    localization::Tr("editor.table.gWinSetupText.13"),
    localization::Tr("editor.table.gWinSetupText.14"),
    localization::Tr("editor.table.gWinSetupText.15"),
    localization::Tr("editor.table.gWinSetupText.16"),
    localization::Tr("editor.table.gWinSetupText.17"),
    localization::Tr("editor.table.gWinSetupText.18"),
    localization::Tr("editor.table.gWinSetupText.19"),
    localization::Tr("editor.table.gWinSetupText.20"),
    localization::Tr("editor.table.gWinSetupText.21"),
    localization::Tr("editor.table.gWinSetupText.22"),
    localization::Tr("editor.table.gWinSetupText.23"),
    localization::Tr("editor.table.gWinSetupText.24"),
    localization::Tr("editor.table.gWinSetupText.25"),
    localization::Tr("editor.table.gWinSetupText.26"),
    localization::Tr("editor.table.gWinSetupText.27"),
    localization::Tr("editor.table.gWinSetupText.28"),
    localization::Tr("editor.table.gWinSetupText.29"),
    localization::Tr("editor.table.gWinSetupText.30"),
    localization::Tr("editor.table.gWinSetupText.31"),
    localization::Tr("editor.table.gWinSetupText.32"),
    localization::Tr("editor.table.gWinSetupText.33"),
    localization::Tr("editor.table.gWinSetupText.34"),
    localization::Tr("editor.table.gWinSetupText.35"),
    localization::Tr("editor.table.gWinSetupText.36"),
    localization::Tr("editor.table.gWinSetupText.37"),
    localization::Tr("editor.table.gWinSetupText.38"),
    localization::Tr("editor.table.gWinSetupText.39"),
    localization::Tr("editor.table.gWinSetupText.40"),
    localization::Tr("editor.table.gWinSetupText.41"),
    localization::Tr("editor.table.gWinSetupText.42"),
    localization::Tr("editor.table.gWinSetupText.43"),
    localization::Tr("editor.table.gWinSetupText.44"),
    localization::Tr("editor.table.gWinSetupText.45"),
    localization::Tr("editor.table.gWinSetupText.46"),
    localization::Tr("editor.table.gWinSetupText.47"),
    localization::Tr("editor.table.gWinSetupText.48"),
    localization::Tr("editor.table.gWinSetupText.49"),
    localization::Tr("editor.table.gWinSetupText.50"),
    localization::Tr("editor.table.gWinSetupText.51"),
    localization::Tr("editor.table.gWinSetupText.52"),
    localization::Tr("editor.table.gWinSetupText.53"),
    localization::Tr("editor.table.gWinSetupText.54"),
    localization::Tr("editor.table.gWinSetupText.55"),
    localization::Tr("editor.table.gWinSetupText.56"),
    localization::Tr("editor.table.gWinSetupText.57"),
    localization::Tr("editor.table.gWinSetupText.58"),
    localization::Tr("editor.table.gWinSetupText.59"),
    localization::Tr("editor.table.gWinSetupText.60"),
    localization::Tr("editor.table.gWinSetupText.61"),
    localization::Tr("editor.table.gWinSetupText.62"),
    localization::Tr("editor.table.gWinSetupText.63"),
    localization::Tr("editor.table.gWinSetupText.64"),
    localization::Tr("editor.table.gWinSetupText.65"),
    localization::Tr("editor.table.gWinSetupText.66"),
    localization::Tr("editor.table.gWinSetupText.67"),
    localization::Tr("editor.table.gWinSetupText.68"),
    localization::Tr("editor.table.gWinSetupText.69")
};
DATA(0x0043f8ec)
char* gArtifactNames[38] = {
    localization::Tr("table.gArtifactNames.0"),  localization::Tr("table.gArtifactNames.1"),
    localization::Tr("table.gArtifactNames.2"),  localization::Tr("table.gArtifactNames.3"),
    localization::Tr("table.gArtifactNames.4"),  localization::Tr("table.gArtifactNames.5"),
    localization::Tr("table.gArtifactNames.6"),  localization::Tr("table.gArtifactNames.7"),
    localization::Tr("table.gArtifactNames.8"),  localization::Tr("table.gArtifactNames.9"),
    localization::Tr("table.gArtifactNames.10"), localization::Tr("table.gArtifactNames.11"),
    localization::Tr("table.gArtifactNames.12"), localization::Tr("table.gArtifactNames.13"),
    localization::Tr("table.gArtifactNames.14"), localization::Tr("table.gArtifactNames.15"),
    localization::Tr("table.gArtifactNames.16"), localization::Tr("table.gArtifactNames.17"),
    localization::Tr("table.gArtifactNames.18"), localization::Tr("table.gArtifactNames.19"),
    localization::Tr("table.gArtifactNames.20"), localization::Tr("table.gArtifactNames.21"),
    localization::Tr("table.gArtifactNames.22"), localization::Tr("table.gArtifactNames.23"),
    localization::Tr("table.gArtifactNames.24"), localization::Tr("table.gArtifactNames.25"),
    localization::Tr("table.gArtifactNames.26"), localization::Tr("table.gArtifactNames.27"),
    localization::Tr("table.gArtifactNames.28"), localization::Tr("table.gArtifactNames.29"),
    localization::Tr("table.gArtifactNames.30"), localization::Tr("table.gArtifactNames.31"),
    localization::Tr("table.gArtifactNames.32"), localization::Tr("table.gArtifactNames.33"),
    localization::Tr("table.gArtifactNames.34"), localization::Tr("table.gArtifactNames.35"),
    localization::Tr("table.gArtifactNames.36"), localization::Tr("table.gArtifactNames.37"),
};
DATA(0x0043f984)
char* gArtifactDesc[38] = {
    localization::Tr("table.gArtifactDesc.0"),  localization::Tr("table.gArtifactDesc.1"),
    localization::Tr("table.gArtifactDesc.2"),  localization::Tr("table.gArtifactDesc.3"),
    localization::Tr("table.gArtifactDesc.4"),  localization::Tr("table.gArtifactDesc.5"),
    localization::Tr("table.gArtifactDesc.6"),  localization::Tr("table.gArtifactDesc.7"),
    localization::Tr("table.gArtifactDesc.8"),  localization::Tr("table.gArtifactDesc.9"),
    localization::Tr("table.gArtifactDesc.10"), localization::Tr("table.gArtifactDesc.11"),
    localization::Tr("table.gArtifactDesc.12"), localization::Tr("table.gArtifactDesc.13"),
    localization::Tr("table.gArtifactDesc.14"), localization::Tr("table.gArtifactDesc.15"),
    localization::Tr("table.gArtifactDesc.16"), localization::Tr("table.gArtifactDesc.17"),
    localization::Tr("table.gArtifactDesc.18"), localization::Tr("table.gArtifactDesc.19"),
    localization::Tr("table.gArtifactDesc.20"), localization::Tr("table.gArtifactDesc.21"),
    localization::Tr("table.gArtifactDesc.22"), localization::Tr("table.gArtifactDesc.23"),
    localization::Tr("table.gArtifactDesc.24"), localization::Tr("table.gArtifactDesc.25"),
    localization::Tr("table.gArtifactDesc.26"), localization::Tr("table.gArtifactDesc.27"),
    localization::Tr("table.gArtifactDesc.28"), localization::Tr("table.gArtifactDesc.29"),
    localization::Tr("table.gArtifactDesc.30"), localization::Tr("table.gArtifactDesc.31"),
    localization::Tr("table.gArtifactDesc.32"), localization::Tr("table.gArtifactDesc.33"),
    localization::Tr("table.gArtifactDesc.34"), localization::Tr("table.gArtifactDesc.35"),
    localization::Tr("table.gArtifactDesc.36"), localization::Tr("table.gArtifactDesc.37"),
};
DATA(0x0043fa1c)
char* gArtifactEvent[38] = {
    "",
    "",
    "",
    "",
    localization::Tr("table.gArtifactEvent.4"),
    localization::Tr("table.gArtifactEvent.5"),
    localization::Tr("table.gArtifactEvent.6"),
    localization::Tr("table.gArtifactEvent.7"),
    localization::Tr("table.gArtifactEvent.8"),
    localization::Tr("table.gArtifactEvent.9"),
    localization::Tr("table.gArtifactEvent.10"),
    localization::Tr("table.gArtifactEvent.11"),
    localization::Tr("table.gArtifactEvent.12"),
    localization::Tr("table.gArtifactEvent.13"),
    localization::Tr("table.gArtifactEvent.14"),
    localization::Tr("table.gArtifactEvent.15"),
    localization::Tr("table.gArtifactEvent.16"),
    localization::Tr("table.gArtifactEvent.17"),
    localization::Tr("table.gArtifactEvent.18"),
    localization::Tr("table.gArtifactEvent.19"),
    localization::Tr("table.gArtifactEvent.20"),
    localization::Tr("table.gArtifactEvent.21"),
    localization::Tr("table.gArtifactEvent.22"),
    localization::Tr("table.gArtifactEvent.23"),
    localization::Tr("table.gArtifactEvent.24"),
    localization::Tr("table.gArtifactEvent.25"),
    localization::Tr("table.gArtifactEvent.26"),
    localization::Tr("table.gArtifactEvent.27"),
    localization::Tr("table.gArtifactEvent.28"),
    localization::Tr("table.gArtifactEvent.29"),
    localization::Tr("table.gArtifactEvent.30"),
    localization::Tr("table.gArtifactEvent.31"),
    localization::Tr("table.gArtifactEvent.32"),
    localization::Tr("table.gArtifactEvent.33"),
    localization::Tr("table.gArtifactEvent.34"),
    localization::Tr("table.gArtifactEvent.35"),
    localization::Tr("table.gArtifactEvent.36"),
    localization::Tr("table.gArtifactEvent.37"),
};
DATA(0x0043fab4)
char* gStatNames[5] = {
    localization::Tr("table.gStatNames.0"),
    localization::Tr("table.gStatNames.1"),
    localization::Tr("table.gStatNames.2"),
    localization::Tr("table.gStatNames.3"),
    localization::Tr("table.gStatNames.4")
};
DATA(0x0043fac8)
char* gStatDesc[5] = {
    localization::Tr("table.gStatDesc.0"),
    localization::Tr("table.gStatDesc.1"),
    localization::Tr("table.gStatDesc.2"),
    localization::Tr("table.gStatDesc.3"),
    localization::Tr("table.gStatDesc.4"),
};
DATA(0x0043fadc)
char* gClassNames[4] = {
    localization::Tr("table.gClassNames.0"),
    localization::Tr("table.gClassNames.1"),
    localization::Tr("table.gClassNames.2"),
    localization::Tr("table.gClassNames.3")
};
DATA(0x0043faec)
char* gArmyNames[28] = {
    localization::Tr("table.gArmyNames.0"),  localization::Tr("table.gArmyNames.1"),
    localization::Tr("table.gArmyNames.2"),  localization::Tr("table.gArmyNames.3"),
    localization::Tr("table.gArmyNames.4"),  localization::Tr("table.gArmyNames.5"),
    localization::Tr("table.gArmyNames.6"),  localization::Tr("table.gArmyNames.7"),
    localization::Tr("table.gArmyNames.8"),  localization::Tr("table.gArmyNames.9"),
    localization::Tr("table.gArmyNames.10"), localization::Tr("table.gArmyNames.11"),
    localization::Tr("table.gArmyNames.12"), localization::Tr("table.gArmyNames.13"),
    localization::Tr("table.gArmyNames.14"), localization::Tr("table.gArmyNames.15"),
    localization::Tr("table.gArmyNames.16"), localization::Tr("table.gArmyNames.17"),
    localization::Tr("table.gArmyNames.18"), localization::Tr("table.gArmyNames.19"),
    localization::Tr("table.gArmyNames.20"), localization::Tr("table.gArmyNames.21"),
    localization::Tr("table.gArmyNames.22"), localization::Tr("table.gArmyNames.23"),
    localization::Tr("table.gArmyNames.24"), localization::Tr("table.gArmyNames.25"),
    localization::Tr("table.gArmyNames.26"), localization::Tr("table.gArmyNames.27"),
};
// Buka separates resource stems from translated creature display names.
DATA(0x0043fb5c)
char* gArmySpriteNames[28] = {"peasant", "archer",   "pikeman", "swordsman", "cavalry", "paladin",
                              "goblin",  "orc",      "wolf",    "ogre",      "troll",   "cyclops",
                              "sprite",  "dwarf",    "elf",     "druid",     "unicorn", "phoenix",
                              "centaur", "gargoyle", "griffin", "minotaur",  "hydra",   "dragon",
                              "rogue",   "nomad",    "ghost",   "genie"};
DATA(0x0043fbcc)
char* gArmyNamesPlural[28] = {
    localization::Tr("table.gArmyNamesPlural.0"),  localization::Tr("table.gArmyNamesPlural.1"),
    localization::Tr("table.gArmyNamesPlural.2"),  localization::Tr("table.gArmyNamesPlural.3"),
    localization::Tr("table.gArmyNamesPlural.4"),  localization::Tr("table.gArmyNamesPlural.5"),
    localization::Tr("table.gArmyNamesPlural.6"),  localization::Tr("table.gArmyNamesPlural.7"),
    localization::Tr("table.gArmyNamesPlural.8"),  localization::Tr("table.gArmyNamesPlural.9"),
    localization::Tr("table.gArmyNamesPlural.10"), localization::Tr("table.gArmyNamesPlural.11"),
    localization::Tr("table.gArmyNamesPlural.12"), localization::Tr("table.gArmyNamesPlural.13"),
    localization::Tr("table.gArmyNamesPlural.14"), localization::Tr("table.gArmyNamesPlural.15"),
    localization::Tr("table.gArmyNamesPlural.16"), localization::Tr("table.gArmyNamesPlural.17"),
    localization::Tr("table.gArmyNamesPlural.18"), localization::Tr("table.gArmyNamesPlural.19"),
    localization::Tr("table.gArmyNamesPlural.20"), localization::Tr("table.gArmyNamesPlural.21"),
    localization::Tr("table.gArmyNamesPlural.22"), localization::Tr("table.gArmyNamesPlural.23"),
    localization::Tr("table.gArmyNamesPlural.24"), localization::Tr("table.gArmyNamesPlural.25"),
    localization::Tr("table.gArmyNamesPlural.26"), localization::Tr("table.gArmyNamesPlural.27"),
};
DATA(0x0043fc3c)
char* gSpellNames[29] = {
    localization::Tr("table.gSpellNames.0"),  localization::Tr("table.gSpellNames.1"),
    localization::Tr("table.gSpellNames.2"),  localization::Tr("table.gSpellNames.3"),
    localization::Tr("table.gSpellNames.4"),  localization::Tr("table.gSpellNames.5"),
    localization::Tr("table.gSpellNames.6"),  localization::Tr("table.gSpellNames.7"),
    localization::Tr("table.gSpellNames.8"),  localization::Tr("table.gSpellNames.9"),
    localization::Tr("table.gSpellNames.10"), localization::Tr("table.gSpellNames.11"),
    localization::Tr("table.gSpellNames.12"), localization::Tr("table.gSpellNames.13"),
    localization::Tr("table.gSpellNames.14"), localization::Tr("table.gSpellNames.15"),
    localization::Tr("table.gSpellNames.16"), localization::Tr("table.gSpellNames.17"),
    localization::Tr("table.gSpellNames.18"), localization::Tr("table.gSpellNames.19"),
    localization::Tr("table.gSpellNames.20"), localization::Tr("table.gSpellNames.21"),
    localization::Tr("table.gSpellNames.22"), localization::Tr("table.gSpellNames.23"),
    localization::Tr("table.gSpellNames.24"), localization::Tr("table.gSpellNames.25"),
    localization::Tr("table.gSpellNames.26"), localization::Tr("table.gSpellNames.27"),
    localization::Tr("table.gSpellNames.28"),
};
DATA(0x0043fcb0)
char* gNeutralBuildingNames[7] = {
    localization::Tr("table.gNeutralBuildingNames.0"),
    localization::Tr("table.gNeutralBuildingNames.1"),
    localization::Tr("table.gNeutralBuildingNames.2"),
    localization::Tr("table.gNeutralBuildingNames.3"),
    localization::Tr("table.gNeutralBuildingNames.4"),
    localization::Tr("table.gNeutralBuildingNames.5"),
    localization::Tr("table.gNeutralBuildingNames.6")
};
DATA(0x0043fccc)
char* gDwellingNames[24] = {
    localization::Tr("table.gDwellingNames.0"),  localization::Tr("table.gDwellingNames.1"),
    localization::Tr("table.gDwellingNames.2"),  localization::Tr("table.gDwellingNames.3"),
    localization::Tr("table.gDwellingNames.4"),  localization::Tr("table.gDwellingNames.5"),
    localization::Tr("table.gDwellingNames.6"),  localization::Tr("table.gDwellingNames.7"),
    localization::Tr("table.gDwellingNames.8"),  localization::Tr("table.gDwellingNames.9"),
    localization::Tr("table.gDwellingNames.10"), localization::Tr("table.gDwellingNames.11"),
    localization::Tr("table.gDwellingNames.12"), localization::Tr("table.gDwellingNames.13"),
    localization::Tr("table.gDwellingNames.14"), localization::Tr("table.gDwellingNames.15"),
    localization::Tr("table.gDwellingNames.16"), localization::Tr("table.gDwellingNames.17"),
    localization::Tr("table.gDwellingNames.18"), localization::Tr("table.gDwellingNames.19"),
    localization::Tr("table.gDwellingNames.20"), localization::Tr("table.gDwellingNames.21"),
    localization::Tr("table.gDwellingNames.22"), localization::Tr("table.gDwellingNames.23"),
};
DATA(0x0043fd2c)
char* gTerrainNames[7] = {
    localization::Tr("table.gTerrainNames.0"),
    localization::Tr("table.gTerrainNames.1"),
    localization::Tr("table.gTerrainNames.2"),
    localization::Tr("table.gTerrainNames.3"),
    localization::Tr("table.gTerrainNames.4"),
    localization::Tr("table.gTerrainNames.5"),
    localization::Tr("table.gTerrainNames.6")
};
DATA(0x0043fd48)
char* gResourceNames[7] = {
    localization::Tr("table.gResourceNames.0"),
    localization::Tr("table.gResourceNames.1"),
    localization::Tr("table.gResourceNames.2"),
    localization::Tr("table.gResourceNames.3"),
    localization::Tr("table.gResourceNames.4"),
    localization::Tr("table.gResourceNames.5"),
    localization::Tr("table.gResourceNames.6")
};
DATA(0x0043fd64)
char* gMineNames[7] = {
    localization::Tr("table.gMineNames.0"),
    localization::Tr("table.gMineNames.1"),
    localization::Tr("table.gMineNames.2"),
    localization::Tr("table.gMineNames.3"),
    localization::Tr("table.gMineNames.4"),
    localization::Tr("table.gMineNames.5"),
    localization::Tr("table.gMineNames.6")
};
DATA(0x0043fd80)
char* gObjectNames[63] = {
    "",
    localization::Tr("table.gObjectNames.1"),
    localization::Tr("table.gObjectNames.2"),
    localization::Tr("table.gObjectNames.3"),
    localization::Tr("table.gObjectNames.4"),
    localization::Tr("table.gObjectNames.5"),
    localization::Tr("table.gObjectNames.6"),
    localization::Tr("table.gObjectNames.7"),
    localization::Tr("table.gObjectNames.8"),
    localization::Tr("table.gObjectNames.9"),
    localization::Tr("table.gObjectNames.10"),
    localization::Tr("table.gObjectNames.11"),
    localization::Tr("table.gObjectNames.12"),
    localization::Tr("table.gObjectNames.13"),
    localization::Tr("table.gObjectNames.14"),
    localization::Tr("table.gObjectNames.15"),
    localization::Tr("table.gObjectNames.16"),
    localization::Tr("table.gObjectNames.17"),
    localization::Tr("table.gObjectNames.18"),
    localization::Tr("table.gObjectNames.19"),
    localization::Tr("table.gObjectNames.20"),
    localization::Tr("table.gObjectNames.21"),
    localization::Tr("table.gObjectNames.22"),
    localization::Tr("table.gObjectNames.23"),
    localization::Tr("table.gObjectNames.24"),
    localization::Tr("table.gObjectNames.25"),
    localization::Tr("table.gObjectNames.26"),
    localization::Tr("table.gObjectNames.27"),
    localization::Tr("table.gObjectNames.28"),
    localization::Tr("table.gObjectNames.29"),
    localization::Tr("table.gObjectNames.30"),
    localization::Tr("table.gObjectNames.31"),
    localization::Tr("table.gObjectNames.32"),
    localization::Tr("table.gObjectNames.33"),
    localization::Tr("table.gObjectNames.34"),
    localization::Tr("table.gObjectNames.35"),
    localization::Tr("table.gObjectNames.36"),
    localization::Tr("table.gObjectNames.37"),
    localization::Tr("table.gObjectNames.38"),
    localization::Tr("table.gObjectNames.39"),
    localization::Tr("table.gObjectNames.40"),
    localization::Tr("table.gObjectNames.41"),
    localization::Tr("table.gObjectNames.42"),
    localization::Tr("table.gObjectNames.43"),
    localization::Tr("table.gObjectNames.44"),
    localization::Tr("table.gObjectNames.45"),
    localization::Tr("table.gObjectNames.46"),
    localization::Tr("table.gObjectNames.47"),
    localization::Tr("table.gObjectNames.48"),
    localization::Tr("table.gObjectNames.49"),
    "",
    "",
    localization::Tr("table.gObjectNames.52"),
    localization::Tr("table.gObjectNames.53"),
    localization::Tr("table.gObjectNames.54"),
    localization::Tr("table.gObjectNames.55"),
    localization::Tr("table.gObjectNames.56"),
    localization::Tr("table.gObjectNames.57"),
    localization::Tr("table.gObjectNames.58"),
    localization::Tr("table.gObjectNames.59"),
    localization::Tr("table.gObjectNames.60"),
    "",
    localization::Tr("table.gObjectNames.62")
};
DATA(0x0043fe7c)
char* gTownNames[36] = {
    localization::Tr("table.gTownNames.0"),  localization::Tr("table.gTownNames.1"),
    localization::Tr("table.gTownNames.2"),  localization::Tr("table.gTownNames.3"),
    localization::Tr("table.gTownNames.4"),  localization::Tr("table.gTownNames.5"),
    localization::Tr("table.gTownNames.6"),  localization::Tr("table.gTownNames.7"),
    localization::Tr("table.gTownNames.8"),  localization::Tr("table.gTownNames.9"),
    localization::Tr("table.gTownNames.10"), localization::Tr("table.gTownNames.11"),
    localization::Tr("table.gTownNames.12"), localization::Tr("table.gTownNames.13"),
    localization::Tr("table.gTownNames.14"), localization::Tr("table.gTownNames.15"),
    localization::Tr("table.gTownNames.16"), localization::Tr("table.gTownNames.17"),
    localization::Tr("table.gTownNames.18"), localization::Tr("table.gTownNames.19"),
    localization::Tr("table.gTownNames.20"), localization::Tr("table.gTownNames.21"),
    localization::Tr("table.gTownNames.22"), localization::Tr("table.gTownNames.23"),
    localization::Tr("table.gTownNames.24"), localization::Tr("table.gTownNames.25"),
    localization::Tr("table.gTownNames.26"), localization::Tr("table.gTownNames.27"),
    localization::Tr("table.gTownNames.28"), localization::Tr("table.gTownNames.29"),
    localization::Tr("table.gTownNames.30"), localization::Tr("table.gTownNames.31"),
    localization::Tr("table.gTownNames.32"), localization::Tr("table.gTownNames.33"),
    localization::Tr("table.gTownNames.34"), localization::Tr("table.gTownNames.35"),
};
DATA(0x0043ff0c)
char* gEventText[77] = {
    localization::Tr("table.gEventText.0"),  localization::Tr("table.gEventText.1"),
    localization::Tr("table.gEventText.2"),  localization::Tr("table.gEventText.3"),
    localization::Tr("table.gEventText.4"),  localization::Tr("table.gEventText.5"),
    localization::Tr("table.gEventText.6"),  localization::Tr("table.gEventText.7"),
    localization::Tr("table.gEventText.8"),  localization::Tr("table.gEventText.9"),
    localization::Tr("table.gEventText.10"), localization::Tr("table.gEventText.11"),
    localization::Tr("table.gEventText.12"), localization::Tr("table.gEventText.13"),
    localization::Tr("table.gEventText.14"), localization::Tr("table.gEventText.15"),
    localization::Tr("table.gEventText.16"), localization::Tr("table.gEventText.17"),
    localization::Tr("table.gEventText.18"), localization::Tr("table.gEventText.19"),
    localization::Tr("table.gEventText.20"), localization::Tr("table.gEventText.21"),
    localization::Tr("table.gEventText.22"), localization::Tr("table.gEventText.23"),
    localization::Tr("table.gEventText.24"), localization::Tr("table.gEventText.25"),
    localization::Tr("table.gEventText.26"), localization::Tr("table.gEventText.27"),
    localization::Tr("table.gEventText.28"), localization::Tr("table.gEventText.29"),
    localization::Tr("table.gEventText.30"), localization::Tr("table.gEventText.31"),
    localization::Tr("table.gEventText.32"), localization::Tr("table.gEventText.33"),
    localization::Tr("table.gEventText.34"), localization::Tr("table.gEventText.35"),
    localization::Tr("table.gEventText.36"), localization::Tr("table.gEventText.37"),
    localization::Tr("table.gEventText.38"), localization::Tr("table.gEventText.39"),
    localization::Tr("table.gEventText.40"), localization::Tr("table.gEventText.41"),
    localization::Tr("table.gEventText.42"), localization::Tr("table.gEventText.43"),
    localization::Tr("table.gEventText.44"), localization::Tr("table.gEventText.45"),
    localization::Tr("table.gEventText.46"), localization::Tr("table.gEventText.47"),
    localization::Tr("table.gEventText.48"), localization::Tr("table.gEventText.49"),
    localization::Tr("table.gEventText.50"), localization::Tr("table.gEventText.51"),
    localization::Tr("table.gEventText.52"), localization::Tr("table.gEventText.53"),
    localization::Tr("table.gEventText.54"), localization::Tr("table.gEventText.55"),
    localization::Tr("table.gEventText.56"), localization::Tr("table.gEventText.57"),
    localization::Tr("table.gEventText.58"), localization::Tr("table.gEventText.59"),
    localization::Tr("table.gEventText.60"), localization::Tr("table.gEventText.61"),
    localization::Tr("table.gEventText.62"), localization::Tr("table.gEventText.63"),
    localization::Tr("table.gEventText.64"), localization::Tr("table.gEventText.65"),
    localization::Tr("table.gEventText.66"), localization::Tr("table.gEventText.67"),
    localization::Tr("table.gEventText.68"), localization::Tr("table.gEventText.69"),
    localization::Tr("table.gEventText.70"), localization::Tr("table.gEventText.71"),
    localization::Tr("table.gEventText.72"), localization::Tr("table.gEventText.73"),
    localization::Tr("table.gEventText.74"), localization::Tr("table.gEventText.75"),
    localization::Tr("table.gEventText.76"),
};
DATA(0x00440040)
char* gAPanelHelp[5] = {
    localization::Tr("table.gAPanelHelp.0"),
    localization::Tr("table.gAPanelHelp.1"),
    localization::Tr("table.gAPanelHelp.2"),
    localization::Tr("table.gAPanelHelp.3"),
    localization::Tr("table.gAPanelHelp.4"),
};
DATA(0x00440054)
char* gInitMenuHelp[5] = {
    localization::Tr("table.gInitMenuHelp.0"),
    localization::Tr("table.gInitMenuHelp.1"),
    localization::Tr("table.gInitMenuHelp.2"),
    localization::Tr("table.gInitMenuHelp.3"),
    localization::Tr("table.gInitMenuHelp.4"),
};
DATA(0x00440068)
char* gAdvMenuHelp[6] = {
    localization::Tr("table.gAdvMenuHelp.0"),
    localization::Tr("table.gAdvMenuHelp.1"),
    localization::Tr("table.gAdvMenuHelp.2"),
    localization::Tr("table.gAdvMenuHelp.3"),
    localization::Tr("table.gAdvMenuHelp.4"),
    localization::Tr("table.gAdvMenuHelp.5"),
};
DATA(0x00440080)
char* gLuckText[7] = {
    localization::Tr("table.gLuckText.0"),
    localization::Tr("table.gLuckText.1"),
    localization::Tr("table.gLuckText.2"),
    localization::Tr("table.gLuckText.3"),
    localization::Tr("table.gLuckText.4"),
    localization::Tr("table.gLuckText.5"),
    localization::Tr("table.gLuckText.6"),
};
DATA(0x0044009c)
char* gMoraleText[7] = {
    localization::Tr("table.gMoraleText.0"),
    localization::Tr("table.gMoraleText.1"),
    localization::Tr("table.gMoraleText.2"),
    localization::Tr("table.gMoraleText.3"),
    localization::Tr("table.gMoraleText.4"),
    localization::Tr("table.gMoraleText.5"),
    localization::Tr("table.gMoraleText.6"),
};
DATA(0x004400b8)
char* onOffText[11] = {
    localization::Tr("table.onOffText.0"),
    localization::Tr("table.onOffText.1"),
    localization::Tr("table.onOffText.2"),
    localization::Tr("table.onOffText.3"),
    localization::Tr("table.onOffText.4"),
    localization::Tr("table.onOffText.5"),
    localization::Tr("table.onOffText.6"),
    localization::Tr("table.onOffText.7"),
    localization::Tr("table.onOffText.8"),
    localization::Tr("table.onOffText.9"),
    localization::Tr("table.onOffText.10")
};
DATA(0x004400e4)
char* walkSpeedText[5] = {
    localization::Tr("table.walkSpeedText.0"),
    localization::Tr("table.walkSpeedText.1"),
    localization::Tr("table.walkSpeedText.2"),
    localization::Tr("table.walkSpeedText.3"),
    localization::Tr("table.walkSpeedText.4")
};
DATA(0x004400f8)
char* gColorNames[4] = {
    localization::Tr("table.gColorNames.0"),
    localization::Tr("table.gColorNames.1"),
    localization::Tr("table.gColorNames.2"),
    localization::Tr("table.gColorNames.3")
};
DATA(0x00440108)
char* gAlignmentNames[5] = {
    localization::Tr("table.gAlignmentNames.0"),
    localization::Tr("table.gAlignmentNames.1"),
    localization::Tr("table.gAlignmentNames.2"),
    localization::Tr("table.gAlignmentNames.3"),
    localization::Tr("table.gAlignmentNames.4")
};
DATA(0x0044011c)
char* gSpellDesc[29] = {
    localization::Tr("table.gSpellDesc.0"),  localization::Tr("table.gSpellDesc.1"),
    localization::Tr("table.gSpellDesc.2"),  localization::Tr("table.gSpellDesc.3"),
    localization::Tr("table.gSpellDesc.4"),  localization::Tr("table.gSpellDesc.5"),
    localization::Tr("table.gSpellDesc.6"),  localization::Tr("table.gSpellDesc.7"),
    localization::Tr("table.gSpellDesc.8"),  localization::Tr("table.gSpellDesc.9"),
    localization::Tr("table.gSpellDesc.10"), localization::Tr("table.gSpellDesc.11"),
    localization::Tr("table.gSpellDesc.12"), localization::Tr("table.gSpellDesc.13"),
    localization::Tr("table.gSpellDesc.14"), localization::Tr("table.gSpellDesc.15"),
    localization::Tr("table.gSpellDesc.16"), localization::Tr("table.gSpellDesc.17"),
    localization::Tr("table.gSpellDesc.18"), localization::Tr("table.gSpellDesc.19"),
    localization::Tr("table.gSpellDesc.20"), localization::Tr("table.gSpellDesc.21"),
    localization::Tr("table.gSpellDesc.22"), localization::Tr("table.gSpellDesc.23"),
    localization::Tr("table.gSpellDesc.24"), localization::Tr("table.gSpellDesc.25"),
    localization::Tr("table.gSpellDesc.26"), localization::Tr("table.gSpellDesc.27"),
    localization::Tr("table.gSpellDesc.28"),
};
DATA(0x00440190)
char* gMonthNames[10] = {
    localization::Tr("table.gMonthNames.0"),
    localization::Tr("table.gMonthNames.1"),
    localization::Tr("table.gMonthNames.2"),
    localization::Tr("table.gMonthNames.3"),
    localization::Tr("table.gMonthNames.4"),
    localization::Tr("table.gMonthNames.5"),
    localization::Tr("table.gMonthNames.6"),
    localization::Tr("table.gMonthNames.7"),
    localization::Tr("table.gMonthNames.8"),
    localization::Tr("table.gMonthNames.9"),
};
DATA(0x004401b8)
char* gWeekNames[15] = {
    localization::Tr("table.gWeekNames.0"),
    localization::Tr("table.gWeekNames.1"),
    localization::Tr("table.gWeekNames.2"),
    localization::Tr("table.gWeekNames.3"),
    localization::Tr("table.gWeekNames.4"),
    localization::Tr("table.gWeekNames.5"),
    localization::Tr("table.gWeekNames.6"),
    localization::Tr("table.gWeekNames.7"),
    localization::Tr("table.gWeekNames.8"),
    localization::Tr("table.gWeekNames.9"),
    localization::Tr("table.gWeekNames.10"),
    localization::Tr("table.gWeekNames.11"),
    localization::Tr("table.gWeekNames.12"),
    localization::Tr("table.gWeekNames.13"),
    localization::Tr("table.gWeekNames.14"),
};
DATA(0x004401f4)
char* gDwellingDescriptions[24] = {
    localization::Tr("table.gDwellingDescriptions.0"),
    localization::Tr("table.gDwellingDescriptions.1"),
    localization::Tr("table.gDwellingDescriptions.2"),
    localization::Tr("table.gDwellingDescriptions.3"),
    localization::Tr("table.gDwellingDescriptions.4"),
    localization::Tr("table.gDwellingDescriptions.5"),
    localization::Tr("table.gDwellingDescriptions.6"),
    localization::Tr("table.gDwellingDescriptions.7"),
    localization::Tr("table.gDwellingDescriptions.8"),
    localization::Tr("table.gDwellingDescriptions.9"),
    localization::Tr("table.gDwellingDescriptions.10"),
    localization::Tr("table.gDwellingDescriptions.11"),
    localization::Tr("table.gDwellingDescriptions.12"),
    localization::Tr("table.gDwellingDescriptions.13"),
    localization::Tr("table.gDwellingDescriptions.14"),
    localization::Tr("table.gDwellingDescriptions.15"),
    localization::Tr("table.gDwellingDescriptions.16"),
    localization::Tr("table.gDwellingDescriptions.17"),
    localization::Tr("table.gDwellingDescriptions.18"),
    localization::Tr("table.gDwellingDescriptions.19"),
    localization::Tr("table.gDwellingDescriptions.20"),
    localization::Tr("table.gDwellingDescriptions.21"),
    localization::Tr("table.gDwellingDescriptions.22"),
    localization::Tr("table.gDwellingDescriptions.23"),
};
DATA(0x00440254)
char* gArmySizeNames[6][2] = {
    {localization::Tr("table.gArmySizeNames.0"), localization::Tr("table.gArmySizeNames.1")},
    {localization::Tr("table.gArmySizeNames.2"), localization::Tr("table.gArmySizeNames.3")},
    {localization::Tr("table.gArmySizeNames.4"), localization::Tr("table.gArmySizeNames.5")},
    {localization::Tr("table.gArmySizeNames.6"), localization::Tr("table.gArmySizeNames.7")},
    {localization::Tr("table.gArmySizeNames.8"), localization::Tr("table.gArmySizeNames.9")},
    {localization::Tr("table.gArmySizeNames.10"), localization::Tr("table.gArmySizeNames.11")},
};
DATA(0x00440284)
char* gHeroScreen[19] = {
    localization::Tr("table.gHeroScreen.0"),  localization::Tr("table.gHeroScreen.1"),
    localization::Tr("table.gHeroScreen.2"),  localization::Tr("table.gHeroScreen.3"),
    localization::Tr("table.gHeroScreen.4"),  localization::Tr("table.gHeroScreen.5"),
    localization::Tr("table.gHeroScreen.6"),  localization::Tr("table.gHeroScreen.7"),
    localization::Tr("table.gHeroScreen.8"),  localization::Tr("table.gHeroScreen.9"),
    localization::Tr("table.gHeroScreen.10"), localization::Tr("table.gHeroScreen.11"),
    localization::Tr("table.gHeroScreen.12"), localization::Tr("table.gHeroScreen.13"),
    localization::Tr("table.gHeroScreen.14"), localization::Tr("table.gHeroScreen.15"),
    localization::Tr("table.gHeroScreen.16"), localization::Tr("table.gHeroScreen.17"),
    localization::Tr("table.gHeroScreen.18"),
};
DATA(0x004402d0)
char* gCastleInfo[14] = {
    localization::Tr("table.gCastleInfo.0"),
    localization::Tr("table.gCastleInfo.1"),
    localization::Tr("table.gCastleInfo.2"),
    localization::Tr("table.gCastleInfo.3"),
    localization::Tr("table.gCastleInfo.4"),
    localization::Tr("table.gCastleInfo.5"),
    localization::Tr("table.gCastleInfo.6"),
    localization::Tr("table.gCastleInfo.7"),
    localization::Tr("table.gCastleInfo.8"),
    localization::Tr("table.gCastleInfo.9"),
    localization::Tr("table.gCastleInfo.10"),
    localization::Tr("table.gCastleInfo.11"),
    localization::Tr("table.gCastleInfo.12"),
    localization::Tr("table.gCastleInfo.13"),
};
DATA(0x00440308)
char* gLuckInfoText[11] = {
    localization::Tr("table.gLuckInfoText.0"),
    localization::Tr("table.gLuckInfoText.1"),
    localization::Tr("table.gLuckInfoText.2"),
    localization::Tr("table.gLuckInfoText.3"),
    localization::Tr("table.gLuckInfoText.4"),
    localization::Tr("table.gLuckInfoText.5"),
    localization::Tr("table.gLuckInfoText.6"),
    localization::Tr("table.gLuckInfoText.7"),
    localization::Tr("table.gLuckInfoText.8"),
    localization::Tr("table.gLuckInfoText.9"),
    localization::Tr("table.gLuckInfoText.10"),
};
DATA(0x00440334)
char* gMemoryErrorTitle = localization::Tr("table.gMemoryErrorTitle.0");
DATA(0x00440338)
char* gMemoryRequirements = localization::Tr("table.gMemoryRequirements.0");
DATA(0x0044033c)
char* gExtendedMemoryUnits = localization::Tr("table.gExtendedMemoryUnits.0");
DATA(0x00440340)
char* gConventionalMemoryUnits = localization::Tr("table.gConventionalMemoryUnits.0");
DATA(0x00440344)
char* gPlayerTypeNames[5] = {
    localization::Tr("table.gPlayerTypeNames.0"),
    localization::Tr("table.gPlayerTypeNames.1"),
    localization::Tr("table.gPlayerTypeNames.2"),
    localization::Tr("table.gPlayerTypeNames.3"),
    localization::Tr("table.gPlayerTypeNames.4")
};
DATA(0x00440358)
char* gSpellHelp[8] = {
    localization::Tr("table.gSpellHelp.0"),
    localization::Tr("table.gSpellHelp.1"),
    localization::Tr("table.gSpellHelp.2"),
    localization::Tr("table.gSpellHelp.3"),
    localization::Tr("table.gSpellHelp.4"),
    localization::Tr("table.gSpellHelp.5"),
    localization::Tr("table.gSpellHelp.6"),
    localization::Tr("table.gSpellHelp.7"),
};
DATA(0x00440378)
char* gSpeedText[5] = {
    "",
    localization::Tr("table.gSpeedText.1"),
    localization::Tr("table.gSpeedText.2"),
    localization::Tr("table.gSpeedText.3"),
    localization::Tr("table.gSpeedText.4"),
};
DATA(0x0044038c)
char* gArmyStatText[9] = {
    localization::Tr("table.gArmyStatText.0"),
    localization::Tr("table.gArmyStatText.1"),
    localization::Tr("table.gArmyStatText.2"),
    localization::Tr("table.gArmyStatText.3"),
    localization::Tr("table.gArmyStatText.4"),
    localization::Tr("table.gArmyStatText.5"),
    localization::Tr("table.gArmyStatText.6"),
    localization::Tr("table.gArmyStatText.7"),
    localization::Tr("table.gArmyStatText.8"),
};
DATA(0x004403b0)
char* gOverviewText[3] = {
    localization::Tr("table.gOverviewText.0"),
    localization::Tr("table.gOverviewText.1"),
    localization::Tr("table.gOverviewText.2"),
};
DATA(0x004403bc)
char* gNewTurnText[7] = {
    localization::Tr("table.gNewTurnText.0"),
    localization::Tr("table.gNewTurnText.1"),
    localization::Tr("table.gNewTurnText.2"),
    localization::Tr("table.gNewTurnText.3"),
    localization::Tr("table.gNewTurnText.4"),
    localization::Tr("table.gNewTurnText.5"),
    localization::Tr("table.gNewTurnText.6"),
};
DATA(0x004403d8)
char* gViewGeneralLabels[6] = {
    localization::Tr("table.gViewGeneralLabels.0"),
    localization::Tr("table.gViewGeneralLabels.1"),
    localization::Tr("table.gViewGeneralLabels.2"),
    localization::Tr("table.gViewGeneralLabels.3"),
    localization::Tr("table.gViewGeneralLabels.4"),
    localization::Tr("table.gViewGeneralLabels.5"),
};
DATA(0x004403f0)
char* gViewGeneralHelp[6] = {
    localization::Tr("table.gViewGeneralHelp.0"),
    localization::Tr("table.gViewGeneralHelp.1"),
    localization::Tr("table.gViewGeneralHelp.2"),
    localization::Tr("table.gViewGeneralHelp.3"),
    localization::Tr("table.gViewGeneralHelp.4"),
    localization::Tr("table.gViewGeneralHelp.5"),
};
DATA(0x00440408)
char* gCombatMessage[9] = {
    "",
    localization::Tr("table.gCombatMessage.1"),
    localization::Tr("table.gCombatMessage.2"),
    localization::Tr("table.gCombatMessage.3"),
    localization::Tr("table.gCombatMessage.4"),
    localization::Tr("table.gCombatMessage.5"),
    localization::Tr("table.gCombatMessage.6"),
    localization::Tr("table.gCombatMessage.7"),
    localization::Tr("table.gCombatMessage.8"),
};
DATA(0x0044042c)
char* gHeroLevel[3] = {
    localization::Tr("table.gHeroLevel.0"),
    localization::Tr("table.gHeroLevel.1"),
    localization::Tr("table.gHeroLevel.2")
};
DATA(0x00440438)
char* gCombatHelp[3] =
    {localization::Tr("table.gCombatHelp.0"), localization::Tr("table.gCombatHelp.1"), ""};
DATA(0x00440444)
char* gTownCommand[22] = {
    localization::Tr("table.gTownCommand.0"),  localization::Tr("table.gTownCommand.1"),
    localization::Tr("table.gTownCommand.2"),  localization::Tr("table.gTownCommand.3"),
    localization::Tr("table.gTownCommand.4"),  localization::Tr("table.gTownCommand.5"),
    localization::Tr("table.gTownCommand.6"),  localization::Tr("table.gTownCommand.7"),
    localization::Tr("table.gTownCommand.8"),  "",
    localization::Tr("table.gTownCommand.10"), localization::Tr("table.gTownCommand.11"),
    localization::Tr("table.gTownCommand.12"), localization::Tr("table.gTownCommand.13"),
    localization::Tr("table.gTownCommand.14"), localization::Tr("table.gTownCommand.15"),
    localization::Tr("table.gTownCommand.16"), localization::Tr("table.gTownCommand.17"),
    localization::Tr("table.gTownCommand.18"), localization::Tr("table.gTownCommand.19"),
    localization::Tr("table.gTownCommand.20"), localization::Tr("table.gTownCommand.21"),
};
DATA(0x0044049c)
char* gGameTypeHelp[5] = {
    localization::Tr("table.gGameTypeHelp.0"),
    localization::Tr("table.gGameTypeHelp.1"),
    localization::Tr("table.gGameTypeHelp.2"),
    localization::Tr("table.gGameTypeHelp.3"),
    localization::Tr("table.gGameTypeHelp.4"),
};
DATA(0x004404b0)
char* gHeroNames[36][2] = {
    {localization::Tr("table.gHeroNames.0.0"), localization::Tr("table.gHeroNames.0.1")},
    {localization::Tr("table.gHeroNames.1.0"), localization::Tr("table.gHeroNames.1.1")},
    {localization::Tr("table.gHeroNames.2.0"), localization::Tr("table.gHeroNames.2.1")},
    {localization::Tr("table.gHeroNames.3.0"), localization::Tr("table.gHeroNames.3.1")},
    {localization::Tr("table.gHeroNames.4.0"), localization::Tr("table.gHeroNames.4.1")},
    {localization::Tr("table.gHeroNames.5.0"), localization::Tr("table.gHeroNames.5.1")},
    {localization::Tr("table.gHeroNames.6.0"), localization::Tr("table.gHeroNames.6.1")},
    {localization::Tr("table.gHeroNames.7.0"), localization::Tr("table.gHeroNames.7.1")},
    {localization::Tr("table.gHeroNames.8.0"), localization::Tr("table.gHeroNames.8.1")},
    {localization::Tr("table.gHeroNames.9.0"), localization::Tr("table.gHeroNames.9.1")},
    {localization::Tr("table.gHeroNames.10.0"), localization::Tr("table.gHeroNames.10.1")},
    {localization::Tr("table.gHeroNames.11.0"), localization::Tr("table.gHeroNames.11.1")},
    {localization::Tr("table.gHeroNames.12.0"), localization::Tr("table.gHeroNames.12.1")},
    {localization::Tr("table.gHeroNames.13.0"), localization::Tr("table.gHeroNames.13.1")},
    {localization::Tr("table.gHeroNames.14.0"), localization::Tr("table.gHeroNames.14.1")},
    {localization::Tr("table.gHeroNames.15.0"), localization::Tr("table.gHeroNames.15.1")},
    {localization::Tr("table.gHeroNames.16.0"), localization::Tr("table.gHeroNames.16.1")},
    {localization::Tr("table.gHeroNames.17.0"), localization::Tr("table.gHeroNames.17.1")},
    {localization::Tr("table.gHeroNames.18.0"), localization::Tr("table.gHeroNames.18.1")},
    {localization::Tr("table.gHeroNames.19.0"), localization::Tr("table.gHeroNames.19.1")},
    {localization::Tr("table.gHeroNames.20.0"), localization::Tr("table.gHeroNames.20.1")},
    {localization::Tr("table.gHeroNames.21.0"), localization::Tr("table.gHeroNames.21.1")},
    {localization::Tr("table.gHeroNames.22.0"), localization::Tr("table.gHeroNames.22.1")},
    {localization::Tr("table.gHeroNames.23.0"), localization::Tr("table.gHeroNames.23.1")},
    {localization::Tr("table.gHeroNames.24.0"), localization::Tr("table.gHeroNames.24.1")},
    {localization::Tr("table.gHeroNames.25.0"), localization::Tr("table.gHeroNames.25.1")},
    {localization::Tr("table.gHeroNames.26.0"), localization::Tr("table.gHeroNames.26.1")},
    {localization::Tr("table.gHeroNames.27.0"), localization::Tr("table.gHeroNames.27.1")},
    {localization::Tr("table.gHeroNames.28.0"), localization::Tr("table.gHeroNames.28.1")},
    {localization::Tr("table.gHeroNames.29.0"), localization::Tr("table.gHeroNames.29.1")},
    {localization::Tr("table.gHeroNames.30.0"), localization::Tr("table.gHeroNames.30.1")},
    {localization::Tr("table.gHeroNames.31.0"), localization::Tr("table.gHeroNames.31.1")},
    {localization::Tr("table.gHeroNames.32.0"), localization::Tr("table.gHeroNames.32.1")},
    {localization::Tr("table.gHeroNames.33.0"), localization::Tr("table.gHeroNames.33.1")},
    {localization::Tr("table.gHeroNames.34.0"), localization::Tr("table.gHeroNames.34.1")},
    {localization::Tr("table.gHeroNames.35.0"), localization::Tr("table.gHeroNames.35.1")},
};
DATA(0x004405d0)
char* gCPanelHelp[12] = {
    localization::Tr("table.gCPanelHelp.0"),
    localization::Tr("table.gCPanelHelp.1"),
    localization::Tr("table.gCPanelHelp.2"),
    localization::Tr("table.gCPanelHelp.3"),
    localization::Tr("table.gCPanelHelp.4"),
    localization::Tr("table.gCPanelHelp.5"),
    localization::Tr("table.gCPanelHelp.6"),
    localization::Tr("table.gCPanelHelp.7"),
    localization::Tr("table.gCPanelHelp.8"),
    localization::Tr("table.gCPanelHelp.9"),
    localization::Tr("table.gCPanelHelp.10"),
    localization::Tr("table.gCPanelHelp.11")
};
DATA(0x00440600)
char* gNewGameHelp[9] = {
    localization::Tr("table.gNewGameHelp.0"),
    localization::Tr("table.gNewGameHelp.1"),
    localization::Tr("table.gNewGameHelp.2"),
    localization::Tr("table.gNewGameHelp.3"),
    localization::Tr("table.gNewGameHelp.4"),
    localization::Tr("table.gNewGameHelp.5"),
    localization::Tr("table.gNewGameHelp.6"),
    localization::Tr("table.gNewGameHelp.7"),
    localization::Tr("table.gNewGameHelp.8"),
};
DATA(0x00440624)
char* gSetupCampaignGameHelp[5] = {
    localization::Tr("table.gSetupCampaignGameHelp.0"),
    localization::Tr("table.gSetupCampaignGameHelp.1"),
    localization::Tr("table.gSetupCampaignGameHelp.2"),
    localization::Tr("table.gSetupCampaignGameHelp.3"),
    localization::Tr("table.gSetupCampaignGameHelp.4"),
};
DATA(0x00440638)
char* gSetupBaudHelp[5] = {
    localization::Tr("table.gSetupBaudHelp.0"),
    localization::Tr("table.gSetupBaudHelp.1"),
    localization::Tr("table.gSetupBaudHelp.2"),
    localization::Tr("table.gSetupBaudHelp.3"),
    localization::Tr("table.gSetupBaudHelp.4"),
};
DATA(0x0044064c)
char* gSetupComPortHelp[5] = {
    localization::Tr("table.gSetupComPortHelp.0"),
    localization::Tr("table.gSetupComPortHelp.1"),
    localization::Tr("table.gSetupComPortHelp.2"),
    localization::Tr("table.gSetupComPortHelp.3"),
    localization::Tr("table.gSetupComPortHelp.4"),
};
DATA(0x00440660)
char* gSetupDCBaudHelp[5] = {
    localization::Tr("table.gSetupDCBaudHelp.0"),
    localization::Tr("table.gSetupDCBaudHelp.1"),
    localization::Tr("table.gSetupDCBaudHelp.2"),
    localization::Tr("table.gSetupDCBaudHelp.3"),
    localization::Tr("table.gSetupDCBaudHelp.4"),
};
DATA(0x00440674)
char* gSetupDCComPortHelp[5] = {
    localization::Tr("table.gSetupDCComPortHelp.0"),
    localization::Tr("table.gSetupDCComPortHelp.1"),
    localization::Tr("table.gSetupDCComPortHelp.2"),
    localization::Tr("table.gSetupDCComPortHelp.3"),
    localization::Tr("table.gSetupDCComPortHelp.4"),
};
DATA(0x00440688)
char* gSetupHotSeatGameHelp[4] = {
    localization::Tr("table.gSetupHotSeatGameHelp.0"),
    localization::Tr("table.gSetupHotSeatGameHelp.1"),
    localization::Tr("table.gSetupHotSeatGameHelp.2"),
    localization::Tr("table.gSetupHotSeatGameHelp.3"),
};
DATA(0x00440698)
char* gSetupModemGameHelp[4] = {
    localization::Tr("table.gSetupModemGameHelp.0"),
    localization::Tr("table.gSetupModemGameHelp.1"),
    localization::Tr("table.gSetupModemGameHelp.2"),
    localization::Tr("table.gSetupModemGameHelp.3"),
};
DATA(0x004406a8)
char* gSetupDCGameHelp[4] = {
    localization::Tr("table.gSetupDCGameHelp.0"),
    localization::Tr("table.gSetupDCGameHelp.1"),
    localization::Tr("table.gSetupDCGameHelp.2"),
    localization::Tr("table.gSetupDCGameHelp.3"),
};
DATA(0x004406b8)
char* gSetupMultiPlayerGameHelp[5] = {
    localization::Tr("table.gSetupMultiPlayerGameHelp.0"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.1"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.2"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.3"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.4"),
};
DATA(0x004406cc)
char* gSetupNetworkGameHelp[3] = {
    localization::Tr("table.gSetupNetworkGameHelp.0"),
    localization::Tr("table.gSetupNetworkGameHelp.1"),
    localization::Tr("table.gSetupNetworkGameHelp.2"),
};
DATA(0x004406d8)
char* gSetupGameHelp[4] = {
    localization::Tr("table.gSetupGameHelp.0"),
    localization::Tr("table.gSetupGameHelp.1"),
    localization::Tr("table.gSetupGameHelp.2"),
    localization::Tr("table.gSetupGameHelp.3"),
};
DATA(0x004406e8)
char* gBattleResults[11] = {
    localization::Tr("table.gBattleResults.0"),
    localization::Tr("table.gBattleResults.1"),
    localization::Tr("table.gBattleResults.2"),
    localization::Tr("table.gBattleResults.3"),
    localization::Tr("table.gBattleResults.4"),
    localization::Tr("table.gBattleResults.5"),
    localization::Tr("table.gBattleResults.6"),
    localization::Tr("table.gBattleResults.7"),
    localization::Tr("table.gBattleResults.8"),
    localization::Tr("table.gBattleResults.9"),
    localization::Tr("table.gBattleResults.10"),
};
DATA(0x00440714)
char* gNeutralBuildingDescriptions[7] = {
    localization::Tr("table.gNeutralBuildingDescriptions.0"),
    localization::Tr("table.gNeutralBuildingDescriptions.1"),
    localization::Tr("table.gNeutralBuildingDescriptions.2"),
    localization::Tr("table.gNeutralBuildingDescriptions.3"),
    localization::Tr("table.gNeutralBuildingDescriptions.4"),
    localization::Tr("table.gNeutralBuildingDescriptions.5"),
    localization::Tr("table.gNeutralBuildingDescriptions.6"),
};
DATA(0x00440730)
char* gMoraleInfoText[21] = {
    localization::Tr("table.gMoraleInfoText.0"),  localization::Tr("table.gMoraleInfoText.1"),
    localization::Tr("table.gMoraleInfoText.2"),  localization::Tr("table.gMoraleInfoText.3"),
    localization::Tr("table.gMoraleInfoText.4"),  localization::Tr("table.gMoraleInfoText.5"),
    localization::Tr("table.gMoraleInfoText.6"),  localization::Tr("table.gMoraleInfoText.7"),
    localization::Tr("table.gMoraleInfoText.8"),  localization::Tr("table.gMoraleInfoText.9"),
    localization::Tr("table.gMoraleInfoText.10"), localization::Tr("table.gMoraleInfoText.11"),
    localization::Tr("table.gMoraleInfoText.12"), localization::Tr("table.gMoraleInfoText.13"),
    localization::Tr("table.gMoraleInfoText.14"), localization::Tr("table.gMoraleInfoText.15"),
    localization::Tr("table.gMoraleInfoText.16"), localization::Tr("table.gMoraleInfoText.17"),
    localization::Tr("table.gMoraleInfoText.18"), localization::Tr("table.gMoraleInfoText.19"),
    localization::Tr("table.gMoraleInfoText.20"),
};
DATA(0x00440784)
char* gMapSizeNames[3] = {
    localization::Tr("table.gMapSizeNames.0"),
    localization::Tr("table.gMapSizeNames.1"),
    localization::Tr("table.gMapSizeNames.2"),
};
DATA(0x00440790)
char* gMapDifficultyNames[5] = {
    localization::Tr("table.gMapDifficultyNames.0"),
    localization::Tr("table.gMapDifficultyNames.1"),
    localization::Tr("table.gMapDifficultyNames.2"),
    localization::Tr("table.gMapDifficultyNames.3"),
    localization::Tr("table.gMapDifficultyNames.4"),
};
DATA(0x004407a4)
char* gCampaignScenarioNames[9] = {
    localization::Tr("table.gCampaignScenarioNames.0"),
    localization::Tr("table.gCampaignScenarioNames.1"),
    localization::Tr("table.gCampaignScenarioNames.2"),
    localization::Tr("table.gCampaignScenarioNames.3"),
    localization::Tr("table.gCampaignScenarioNames.4"),
    localization::Tr("table.gCampaignScenarioNames.5"),
    localization::Tr("table.gCampaignScenarioNames.6"),
    localization::Tr("table.gCampaignScenarioNames.7"),
    localization::Tr("table.gCampaignScenarioNames.8"),
};
DATA(0x004407c8)
char* gCampaignWinTexts[9] = {
    localization::Tr("table.gCampaignWinTexts.0"),
    localization::Tr("table.gCampaignWinTexts.1"),
    localization::Tr("table.gCampaignWinTexts.2"),
    localization::Tr("table.gCampaignWinTexts.3"),
    localization::Tr("table.gCampaignWinTexts.4"),
    localization::Tr("table.gCampaignWinTexts.5"),
    localization::Tr("table.gCampaignWinTexts.6"),
    localization::Tr("table.gCampaignWinTexts.7"),
    localization::Tr("table.gCampaignWinTexts.8"),
};
DATA(0x004407ec)
char* gCampaignScenarioText[9] = {
    localization::Tr("table.gCampaignScenarioText.0"),
    localization::Tr("table.gCampaignScenarioText.1"),
    localization::Tr("table.gCampaignScenarioText.2"),
    localization::Tr("table.gCampaignScenarioText.3"),
    localization::Tr("table.gCampaignScenarioText.4"),
    localization::Tr("table.gCampaignScenarioText.5"),
    localization::Tr("table.gCampaignScenarioText.6"),
    localization::Tr("table.gCampaignScenarioText.7"),
    localization::Tr("table.gCampaignScenarioText.8"),
};
DATA(0x00440810)
char* gDifficultyNames[4] = {
    localization::Tr("table.gDifficultyNames.0"),
    localization::Tr("table.gDifficultyNames.1"),
    localization::Tr("table.gDifficultyNames.2"),
    localization::Tr("table.gDifficultyNames.3"),
};
DATA(0x00440820)
char* gCampaignSideNames[4] = {
    localization::Tr("table.gCampaignSideNames.0"),
    localization::Tr("table.gCampaignSideNames.1"),
    localization::Tr("table.gCampaignSideNames.2"),
    localization::Tr("table.gCampaignSideNames.3")
};
DATA(0x00440830)
char* gScoreLabels[CONGRATS_SCORE_LABEL_COUNT] = {
    localization::Tr("table.gScoreLabels.0"),
    localization::Tr("table.gScoreLabels.1"),
    localization::Tr("table.gScoreLabels.2"),
    localization::Tr("table.gScoreLabels.3"),
    localization::Tr("table.gScoreLabels.4"),
};
DATA(0x00440844)
char* gHumanPlayerTypeNames[5] = {
    localization::Tr("table.gHumanPlayerTypeNames.0"),
    localization::Tr("table.gHumanPlayerTypeNames.1"),
    localization::Tr("table.gHumanPlayerTypeNames.2"),
    localization::Tr("table.gHumanPlayerTypeNames.3"),
    localization::Tr("table.gHumanPlayerTypeNames.4")
};
DATA(0x00440858)
char* gHandicapNames[5] = {
    localization::Tr("table.gHandicapNames.0"),
    localization::Tr("table.gHandicapNames.1"),
    localization::Tr("table.gHandicapNames.2"),
    localization::Tr("table.gHandicapNames.3"),
    localization::Tr("table.gHandicapNames.4")
};
DATA(0x0044086c)
char* musicQualityText[3] = {
    localization::Tr("table.musicQualityText.0"),
    localization::Tr("table.musicQualityText.1"),
    localization::Tr("table.musicQualityText.2")
};

// The editor's uninitialized data; .bss orders it by name key.
DATA(0x00451b88)
i32 gDebugLevel;
DATA(0x00451b8c)
mouseManager* gMouseManager;
DATA(0x00451b90)
char gText[768];
// No retail code reads this; the game's KB.CPP has it beside gText.
DATA(0x00451e90)
class font* gBigFont;
DATA(0x00451e94)
char* DEFAULT_AGGREGATE_NAME;
// The map header the eventsManager dialogs edit.
DATA(0x00451e98)
SMapHeader* gMapHeader;
DATA(0x00451e9c)
i32 gSelectionWidth;
DATA(0x00451ea0)
heroWindowManager* gWindowManager;
DATA(0x00451ea4)
editManager* gEditManager;
DATA(0x00451ea8)
char gStatusText[EDITOR_STATUS_TEXT_CAPACITY];
DATA(0x00451f70)
i32 gSelectionY;
DATA(0x00451f74)
resourceManager* gResourceManager;
DATA(0x00451f78)
heroWindow* gNormalDialogWindow;
DATA(0x00451f7c)
heroWindow* gEditDialog;
DATA(0x00451f80)
i32 gSelectionHeight;
// No retail code reads this; it holds its retail .bss place.
DATA(0x00451f84)
i32 gUnusedData451f84;
DATA(0x00451f88)
i8 gGroundToTerrain[140];
DATA(0x00452014)
char gLastFilename[FILE_REQUESTER_NAME_SIZE];
DATA(0x00452174)
i32 gStatusTextHoldTime;
DATA(0x00452178)
i16 gMapX;
DATA(0x0045217a)
i16 gMapY;
DATA(0x0045217c)
mapCell* gEditCell;
DATA(0x00452180)
palette* gBufferPalette;
DATA(0x00452184)
palette* gPalette;
DATA(0x00452188)
char gAggPathName[352];
DATA(0x004522e8)
char gRegAppPath[352];
DATA(0x00452448)
i32 gMaxExtentX;
DATA(0x0045244c)
i32 gMaxExtentY;
// No retail code reads these; they hold their retail .bss place.
DATA(0x00452450)
i32 gUnusedCount452450;
DATA(0x00452454)
i32 gUnusedCount452454;
DATA(0x00452458)
inputManager* gInputManager;
DATA(0x0045245c)
i16 gNextObjectId;
DATA(0x00452460)
configStruct gConfig;
// No retail code reads this; the game's KB.CPP has it after gConfig.
DATA(0x0045258c)
i32 gOldKBMask;
DATA(0x00452590)
i32 gMinExtentX;
DATA(0x00452594)
i32 gMinExtentY;
DATA(0x00452598)
executive* gExec;
DATA(0x0045259c)
i32 gLandCellCount;
DATA(0x004525a0)
char gLastMapName[352];
DATA(0x00452700)
i32 gCurWindowsStyleFlags;
DATA(0x00452704)
char gRegCDRomPath[352];
DATA(0x00452864)
i32 gTimers[GLOBAL_TIMER_COUNT];

// Zero-initialized data follows the uninitialized in definition order.
DATA(0x0045287c)
char gCurMapName[16] = "";
DATA(0x0045288c)
i32 gComputeExtent = 0;
DATA(0x00452890)
i32 gCurrArmyDrawn = 0;
DATA(0x00452894)
i8 gIconClipOn = 0;
DATA(0x00452898)
i32 gLimitToExtent = 0;
DATA(0x0045289c)
i32 gLoadingMonoIcon = 0;
DATA(0x004528a0)
i32 gSaveBiggestExtent = 0;
DATA(0x004528a4)
i32 gColorMice = 0;
DATA(0x004528a8)
i32 gSpecialMouseMasks = 0;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004528ac)
i32 gUnusedData4528ac = 0;
DATA(0x004528b0)
i32 gScrollX = 0;
DATA(0x004528b4)
i32 gScrollY = 0;
DATA(0x004528b8)
HMENU gDefaultMenu = NULL;
// No retail code reads these; the game's KB.CPP defines them after gDefaultMenu.
DATA(0x004528bc)
HMENU gCombatMenu = NULL;
DATA(0x004528c0)
HMENU gAdventureMenu = NULL;
DATA(0x004528c4)
HMENU gTownMenu = NULL;
DATA(0x004528c8)
i32 gOverlayCategory = 0;
DATA(0x004528cc)
i32 gOverlayShownCategory = 0;
DATA(0x004528d0)
i32 gStatusTextShown = 0;
DATA(0x004528d4)
i32 gInDialog = 0;
DATA(0x004528d8)
i32 gMinimized = 0;
DATA(0x004528dc)
i32 gInSetupDialog = 0;
DATA(0x004528e0)
i32 gSaveUnseen = 0;
DATA(0x004528e4)
i32 gGeneratingMaps = 0;
DATA(0x004528e8)
i32 gHeroMoving = 0;
DATA(0x004528ec)
i32 gInSmacker = 0;
DATA(0x004528f0)
i32 gStatusTextClearTime = 0;
DATA(0x004528f4)
i8 gFirstTimeThrough = 0;

VA(0x004084a0, 0x5)
void EditorStartupHook(void) {}

VA(0x004084a5, 0x5)
void PollSound() {}

VA(0x004084aa, 0x106)
i32 oldmain(void) {
    palette* editorPalette;

    if (gExec->InitSystem())
        ShutDown(localization::Tr("editor.startup.initialize.failed"));
    KBChangeMenu(gDefaultMenu);
    editorPalette = gResourceManager->GetPalette("kb.pal");
    PostprocessPalette(editorPalette->m_data);
    gMapX = 0;
    gMapY = 0;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_NORMAL, editorPalette);
    if (gExec->AddManager(gEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
        ShutDown(localization::Tr("startup.manager.failed"));
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, editorPalette);
    gExec->MainLoop();
    gExec->RemoveManager(gEditManager);
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, editorPalette);
    gResourceManager->Dispose(editorPalette);
    ShutDown(NULL);
    return 0;
}

VA(0x004085b0, 0xe)
void IncrementArgumentA(i32 value) {
    value++;
}

VA(0x004085be, 0x5)
void EditorIdleHook(void) {}

VA(0x004085c3, 0xe)
void IncrementArgumentB(i32 value) {
    value++;
}

VA(0x004085d1, 0x2e)
void DelayTicks(i32 ticks) {
    i32 unused = 0;

    gTimers[DELAY_TICKS_TIMER_SLOT] = KBTickCount() + ticks * DELAY_TICK_MILLISECONDS;
    DelayTil(gTimers + DELAY_TICKS_TIMER_SLOT);
}

VA(0x004085ff, 0x3b)
#line 89 EDITOR_CPP_PATH
void DelayTil(i32* endTime) {
#line 90
    H1_ASSERT(*endTime > 10000);
    while (*endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x0040863a, 0x16)
void DelayMilli(i32 delay) {
    DelayTilMilli(KBTickCount() + delay);
}

VA(0x00408650, 0x1b)
void DelayTilMilli(i32 endTime) {
    while (endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x0040866b, 0x34)
void FileError(char* filename) {
    char message[200];
    sprintf(message, localization::Tr("file.open.failed"), filename);
    ShutDown(message);
}

VA(0x0040869f, 0xa7)
void ShutDown(char* message) {
    char buffer[300];
    if (message) {
        strcpy(buffer, message);
        SetFullScreenStatus(0);
        MessageBoxA(gAppWindow, buffer, localization::Tr("shutdown.unexpected.title"), MB_ICONHAND);
    }
    gClosingApp = 1;
    gExec->ShutDownSystem();
    if (gEventHandle) {
        CloseHandle(gEventHandle);
        gEventHandle = NULL;
    }
    DeleteMainClasses();
    AppExit();
    exit(EXIT_SUCCESS);
}

// The /D (debug level) and /B (mouse masks) command-line switches.
VA(0x00408746, 0xd9)
i32 InterpretCommandLine(void) {
    i32 size;
    i32 i;

    gSpecialMouseMasks = 1;
    gDebugLevel = DEBUG_LEVEL_NONE;
    size = strlen(gCommandLine);
    for (i = 0; i < size; i++) {
        if (gCommandLine[i] == '/' && i + 1 < size) {
            switch (toupper(gCommandLine[i + 1])) {
                case 'D':
                    if (i + 2 < size)
                        gDebugLevel = gCommandLine[i + 2] - '0';
                    break;
                case 'B':
                    if (i + 2 < size)
                        gSpecialMouseMasks = gCommandLine[i + 2] - '0';
                    break;
            }
        }
    }
    gCommandLineInterpreted = 1;
    return 1;
}

VA(0x0040881f, 0x164)
i32 EarlySetup(void) {
    DATA(0x004528f5)
    static i8 gEarlySetupDone = 0;
    i32 i;

    if (gEarlySetupDone)
        return 0;
    sprintf(gAggPathName, "%s%s", gDataPath, "heroes.agg");
    gDefaultAggregateName = gAggPathName;
    InitMainClasses();
    GetGraphicsInfo();
    ReadPrefs();
    if (!InterpretCommandLine())
        return 1;
    switch (SetupCDDrive()) {
        case CD_SETUP_NO_DRIVE:
            MessageBoxA(
                gAppWindow,
                localization::Tr("startup.cd.inaccessible"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
        case CD_SETUP_NOT_FOUND:
            MessageBoxA(
                gAppWindow,
                localization::Tr("editor.startup.cd.required"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
        case CD_SETUP_NO_APP_PATH:
            MessageBoxA(
                gAppWindow,
                localization::Tr("startup.directory.invalid"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
        case CD_SETUP_NO_DATA:
            MessageBoxA(
                gAppWindow,
                localization::Tr("startup.data.missing"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
    }
    gDefaultMenu = LoadMenuA(gAppInstance, "mnuDflt");
    for (i = 0; i < MAP_CELL_GROUND_TILE_COUNT; i++)
        gGroundToTerrain[i] = i / MAP_CELL_TILES_PER_TERRAIN;
    return 1;
}

VA(0x00408983, 0x12)
void MemError(void) {
    ShutDown("Out of Memory");
}

VA(0x00408995, 0x39)
bool IsCDDrive(i32 driveIndex) {
    sprintf(gText, "A:\\");
    gText[0] += driveIndex;
    return GetDriveTypeA(gText) == DRIVE_CDROM;
}

VA(0x004089ce, 0x20c)
void InitMainClasses(void) {
    gExec = new executive;
    gInputManager = new inputManager;
    gMouseManager = new mouseManager;
    gWindowManager = new heroWindowManager;
    gResourceManager = new resourceManager;
    gBufferPalette = new palette;
    gEditManager = new editManager;
}

VA(0x00408bda, 0x137)
void DeleteMainClasses(void) {
    if (gEditManager)
        delete gEditManager;
    gEditManager = NULL;
    if (gBufferPalette)
        delete gBufferPalette;
    gBufferPalette = NULL;
    if (gResourceManager)
        delete gResourceManager;
    gResourceManager = NULL;
    if (gWindowManager)
        delete gWindowManager;
    gWindowManager = NULL;
    if (gMouseManager)
        delete gMouseManager;
    gMouseManager = NULL;
    if (gInputManager)
        delete gInputManager;
    gInputManager = NULL;
    if (gExec)
        delete gExec;
    gExec = NULL;
}

// The editor's dialogs carry text only: no resource, artifact or creature
// panels, and they open at the left of the map view.
VA(0x00408d11, 0x2b4)
void NormalDialog(
    char* text,
    H1_ENUM_PARAM(NormalDialogType, i32) dialogType,
    i32 x,
    i32 y,
    H1_ENUM_PARAM(NormalDialogResourceType, i32),
    i32,
    H1_ENUM_PARAM(NormalDialogResourceType, i32),
    i32,
    H1_ENUM_PARAM(NormalDialogOrText, i32)
) {
    // The resource-panel locals of the game's dialog survive unused (their
    // frame slots are retail's).
    i32 sizedHeight;
    i32 width;
    tag_message msg;
    i32 rows;
    char iconFile[NORMAL_DIALOG_FILENAME_LENGTH];
    i32 wrappedLines;
    i32 totalHeight;
    i32 iconFrameIndex;
    i16 showMessageText;
    i32 nextId;
    i32 panelHeight;
    i32 frameHeight;
    textWidget* captionText;
    i32 index;
    i32 resourceIconY;
    i32 tallestImage;
    font* bigFont;
    i32 resWidth;
    char* orWord;

    nextId = NORMAL_DIALOG_TEXT_WIDGET_FIRST_ID;
    showMessageText = 1;
    bigFont = gResourceManager->GetFont("bigfont.fnt");
    wrappedLines = bigFont->LineLength(text, NORMAL_DIALOG_TEXT_LINE_WIDTH);
    gResourceManager->Dispose(bigFont);
    totalHeight = wrappedLines * NORMAL_DIALOG_TEXT_LINE_HEIGHT;
    if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        totalHeight += NORMAL_DIALOG_BUTTON_AREA_HEIGHT;
    rows = (totalHeight - 12) / NORMAL_DIALOG_WINDOW_ROW_HEIGHT;
    if (rows > NORMAL_DIALOG_MAX_ROWS)
        rows = NORMAL_DIALOG_MAX_ROWS;
    if (rows <= 0 && dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        rows = 1;
    width = NORMAL_DIALOG_WINDOW_WIDTH;
    panelHeight = rows * NORMAL_DIALOG_WINDOW_ROW_HEIGHT + NORMAL_DIALOG_WINDOW_BASE_HEIGHT;

    if (x == NORMAL_DIALOG_AUTO_POSITION || width + x >= LOGICAL_SCREEN_WIDTH - 1)
        x = NORMAL_DIALOG_ADVENTURE_X;
    if (y == NORMAL_DIALOG_AUTO_POSITION || panelHeight + y >= LOGICAL_SCREEN_HEIGHT - 1) {
        y = (LOGICAL_SCREEN_HEIGHT - panelHeight) / 2;
        if (y > NORMAL_DIALOG_MAX_TOP)
            y = NORMAL_DIALOG_MAX_TOP;
    }

    sprintf(iconFile, "evntwin%d.bin", rows);
    gNormalDialogWindow = new heroWindow(x, y, iconFile);
    if (!gNormalDialogWindow)
        MemError();

    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
    msg.value = NORMAL_DIALOG_BUTTON_FLAGS;
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_CANCEL
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        msg.id = NORMAL_DIALOG_BUTTON_OK;
        gNormalDialogWindow->BroadcastMessage(msg);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_OK && dialogType != NORMAL_DIALOG_TYPE_OK
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        msg.id = NORMAL_DIALOG_BUTTON_CANCEL;
        gNormalDialogWindow->BroadcastMessage(msg);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_YES_NO) {
        msg.id = NORMAL_DIALOG_BUTTON_YES;
        gNormalDialogWindow->BroadcastMessage(msg);
        msg.id = NORMAL_DIALOG_BUTTON_NO;
        gNormalDialogWindow->BroadcastMessage(msg);
    }

    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, NORMAL_DIALOG_TEXT_WIDGET_ID);
    msg.text = text;
    gNormalDialogWindow->BroadcastMessage(msg);

    if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
        gMouseManager->ReallyHidePointer();
        gWindowManager->AddWindow(gNormalDialogWindow, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gWindowManager->RemoveWindow(gNormalDialogWindow);
        gMouseManager->ReallyShowPointer();
    } else {
        gWindowManager->DoDialog(gNormalDialogWindow, EventWindowHandler, 0);
    }
    delete gNormalDialogWindow;
}

VA(0x00408fc5, 0x91)
i16 EventWindowHandler(tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case CAMPAIGN_INFO_RESTART:
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                    case DIALOG_BUTTON_2:
                    case DIALOG_BUTTON_3:
                    case DIALOG_BUTTON_5:
                    case DIALOG_BUTTON_6:
                        FINISH_DIALOG_MESSAGE(message);
                        return MESSAGE_DISPATCH_FORWARD;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x00409056, 0x7a)
void QuickViewWait(void) {
    tag_message event;
    i32 done = 0;
    while (!done) {
        PollSound();
        Process1WindowsMessage();
        event = gInputManager->GetEvent();
        done = event.type == MESSAGE_RIGHT_BUTTON_UP || event.type == MESSAGE_LEFT_BUTTON_DOWN
               || event.type == MESSAGE_LEFT_BUTTON_UP;
    }
}

// Shows `text` (or, while a text is shown, the current one) in the status bar.
VA(0x004090d0, 0xbe)
void ShowStatusText(char* text) {
    if (gStatusTextShown && !text)
        text = gStatusText;
    else
        strcpy(gStatusText, text);
    gStatusTextShown = 1;
    gStatusTextHoldTime = KBTickCount() + EDITOR_STATUS_TEXT_HOLD_MILLISECONDS;
    FillBitmapArea(
        gWindowManager->m_screen,
        EDITOR_STATUS_BAR_X,
        EDITOR_STATUS_BAR_Y,
        EDITOR_STATUS_BAR_WIDTH,
        EDITOR_STATUS_BAR_HEIGHT,
        0
    );
    gEditManager->m_statusFont->DrawBoundedString(
        text,
        EDITOR_STATUS_BAR_X,
        EDITOR_STATUS_BAR_Y,
        EDITOR_STATUS_BAR_WIDTH,
        EDITOR_STATUS_BAR_HEIGHT,
        1,
        FONT_ALIGN_CENTER
    );
    gWindowManager->UpdateScreenRegion(
        EDITOR_STATUS_BAR_X,
        EDITOR_STATUS_BAR_Y,
        EDITOR_STATUS_BAR_WIDTH,
        EDITOR_STATUS_BAR_HEIGHT
    );
}

VA(0x0040918e, 0x4d)
void ClearStatusText(void) {
    gStatusTextClearTime = EDITOR_STATUS_TEXT_KEPT;
    if (gStatusTextShown) {
        gStatusTextShown = 0;
        gEditManager->m_window->DrawWindow(0);
        gWindowManager->UpdateScreenRegion(
            EDITOR_STATUS_BAR_X,
            EDITOR_STATUS_BAR_Y,
            EDITOR_STATUS_BAR_WIDTH,
            EDITOR_STATUS_BAR_HEIGHT
        );
    }
}

VA(0x004091db, 0x5)
void UpdateAppSpecificMenus(void*) {}

VA(0x004091e0, 0x3c)
void CleanUpMenus(void) {
    if (gAppMenu) {
        SetMenu(gAppWindow, NULL);
        if (gDefaultMenu)
            DestroyMenu(gDefaultMenu);
    }
    gAppMenu = NULL;
}

VA(0x0040921c, 0x1b)
void EarlyShutDownSystem(void) {
    if (gEditManager)
        gEditManager->SelectTool(EDIT_MANAGER_NO_TOOL);
}

// The editor always asks before quitting.
VA(0x00409237, 0xa)
i32 GameUnsaved(void) {
    return 1;
}

VA(0x00409241, 0x37)
i32 HandleAppSpecificMenuCommands(i32 command) {
    switch (command) {
        case APP_MENU_QUIT:
            PostMessageA(gAppWindow, WM_CLOSE, 0, 0);
            break;
        default:
            return 1;
    }
    return 0;
}

VA(0x00409278, 0x5)
void EarlyResizeWindow(i32, i32, i32, i32) {}

VA(0x0040927d, 0x5)
void UpdateSystemOptionsMenu(void) {}
