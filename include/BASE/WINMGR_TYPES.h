#ifndef HOMM1_BASE_WINMGR_TYPES_H
#define HOMM1_BASE_WINMGR_TYPES_H

#include <Domains.h>

H1_ENUM_BEGIN(WindowFadeMode)
    WINDOW_FADE_IN = 0,
    WINDOW_FADE_OUT = 1
H1_ENUM_END(WindowFadeMode)

H1_ENUM_CONST_BEGIN(WindowManagerConstant)
    WINDOW_MANAGER_NO_DIALOG_RESULT = -1,
    WINDOW_MANAGER_NO_HOVER_WIDGET = -1,
    WINDOW_MANAGER_INITIAL_FADE_STEP = 128,
    WINDOW_MANAGER_DIALOG_FADE_STEP = 8
H1_ENUM_CONST_END(WindowManagerConstant)

class palette;
extern palette *gPalette;
extern int gbInDialog;
extern int iDialogNestCount;

extern short gWindowFadeAssertLine;
extern char gWindowFadeAssertFile[];
extern signed char gWindowFadeSavedUpdate;

#endif
