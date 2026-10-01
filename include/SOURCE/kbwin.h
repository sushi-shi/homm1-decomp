#ifndef HOMM1_SOURCE_KBWIN_H
#define HOMM1_SOURCE_KBWIN_H

#include <Domains.h>

H1_ENUM_BEGIN(WindowTextConstant)
    WINDOW_TEXT_ENTRY_COUNT = 68
H1_ENUM_END(WindowTextConstant)

H1_ENUM_BEGIN(KbwinMenuConstant)
    KBWIN_WIDTH_640 = 640,
    KBWIN_WIDTH_800 = 800,
    KBWIN_WIDTH_1024 = 1024,
    KBWIN_WIDTH_1280 = 1280,
    KBWIN_MENU_SIZE_640_480 = 0x9c45,
    KBWIN_MENU_SIZE_800_600 = 0x9c46,
    KBWIN_MENU_SIZE_1024_768 = 0x9c47,
    KBWIN_MENU_SIZE_1280_1024 = 0x9c48,
    KBWIN_MENU_FULLSCREEN = 0x9c49,
    KBWIN_MENU_ENTRY_COUNT = 70
H1_ENUM_END(KbwinMenuConstant)

#pragma pack(push, 1)
struct SMenuEnableStatus {
    unsigned int command;
    unsigned char normalEnabled;
    unsigned char setupEnabled;
    unsigned char reserved;
};
#pragma pack(pop)

extern SMenuEnableStatus gsMenuEnableStatus[KBWIN_MENU_ENTRY_COUNT];
extern int gbInSetupDialog;

#pragma pack(push, 1)
struct WindowTextEntry {
    short widgetId;
    short windowId;
};
#pragma pack(pop)

extern WindowTextEntry gWinSetup[];
extern char* gWinSetupText[];

extern void* hmnuCurrent;
extern long giCurWindowsStyleFlags;
long AppCommand(void*, unsigned int, unsigned int, long);
int AppIdle(void);
void AppExit(void);
void CleanUpMenus(void);
void KBChangeMenu(void*);
void ResizeWindow(int, int, int, int);
void SetMenuStatus(int);

void UpdateDfltMenu(void*);
void UpdateAppSpecificMenus(void*);

#endif
