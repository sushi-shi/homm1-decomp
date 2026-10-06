#ifndef HOMM1_EDITOR_EDITOR_H
#define HOMM1_EDITOR_EDITOR_H

// The editor program unit's own interface (src/EDITOR/EDITOR.cpp); the
// functions it shares with the game keep their SOURCE/KB.h declarations.

#include <Domains.h>

struct SMapHeader;

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

// The map header the editor edits (EDITMGR's gEditMapHeader).
extern SMapHeader* gMapHeader;
// Cleared while a map without the editor's format word is loaded: such maps
// keep no object owners, so the eraser clears whole cells.
extern i32 gNewMapFormat;
// The eraser's and terrain tool's drag rectangle in map cells (x -1: none).
extern i32 gSelectionX;
extern i32 gSelectionY;
extern i32 gSelectionWidth;
extern i32 gSelectionHeight;
// Set while the random-map generator runs: the map view draws clouds only.
extern i32 gGeneratingMaps;
// The last object number given to placed cells (editCellOwner).
extern i16 gNextCellOwner;
// The right-click help of editwind.bin's buttons and areas.
extern char* gEditButtonHelp[];
extern char* gEditAreaHelp[];

void ShowStatusText(char* text);
void ClearStatusText(void);

#endif // HOMM1_EDITOR_EDITOR_H
