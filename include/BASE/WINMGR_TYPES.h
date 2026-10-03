#ifndef HOMM1_BASE_WINMGR_TYPES_H
#define HOMM1_BASE_WINMGR_TYPES_H

#include <Domains.h>

// clang-format off
H1_ENUM_BEGIN(WindowFadeMode)
    WINDOW_FADE_IN = 0,
    WINDOW_FADE_OUT = 1
H1_ENUM_END(WindowFadeMode)

// Palette fade lengths passed to FadeIn/FadeOut/FadeScreen (Buka SMACKMGR
// SHORT_FADE / NORMAL_FADE): the short fade of dialogs and screen changes and
// the long fade of the window manager's start-up.
H1_ENUM_BEGIN(WindowFadeSteps)
    WINDOW_FADE_STEPS_SHORT = 8,
    WINDOW_FADE_STEPS_NORMAL = 0x80
H1_ENUM_END(WindowFadeSteps)

H1_ENUM_CONST_BEGIN(WindowManagerConstant)
    WINDOW_MANAGER_NO_DIALOG_RESULT = -1,
    WINDOW_MANAGER_NO_HOVER_WIDGET = -1,
    // heroWindowManager::Open when the screen bitmap is missing.
    WINDOW_MANAGER_OPEN_FAILURE = 1
H1_ENUM_CONST_END(WindowManagerConstant)
// clang-format on

class palette;
extern palette* gPalette;
extern int gbInDialog;
extern int iDialogNestCount;

extern signed char gWindowFadeSavedUpdate;

#endif
