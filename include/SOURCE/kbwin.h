#ifndef HOMM1_SOURCE_KBWIN_H
#define HOMM1_SOURCE_KBWIN_H

#include <Domains.h>

H1_ENUM_BEGIN(WindowTextConstant)
    WINDOW_TEXT_ENTRY_COUNT = 68
H1_ENUM_END(WindowTextConstant)

#pragma pack(push, 1)
struct WindowTextEntry {
    short widgetId;
    short windowId;
};
#pragma pack(pop)

extern WindowTextEntry gWinSetup[];
extern char *gWinSetupText[];

extern void *hmnuCurrent;
extern long giCurWindowsStyleFlags;
void KBChangeMenu(void *);
void ResizeWindow(int, int, int, int);
void SetMenuStatus(int);
void UpdateDfltMenu(void *);
void UpdateAppSpecificMenus(void *);

#endif
