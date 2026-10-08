#ifndef HOMM1_SOURCE_KBWINHOST_H
#define HOMM1_SOURCE_KBWINHOST_H

// The Windows host of the game: window class, message loop and menus. Only
// the Windows units include this header; the game sees kbwin.h.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <SOURCE/kbwin.h>

enum KbwinWindowStyle {
    KBWIN_CLASS_STYLE = CS_BYTEALIGNCLIENT | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
    KBWIN_WINDOWED_STYLE = WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW,
    KBWIN_FULLSCREEN_STYLE = WS_VISIBLE | WS_CLIPSIBLINGS
};

extern HINSTANCE gAppInstance;
extern HANDLE gEventHandle;
extern u8 gProcessMessage[];
extern struct tagRECT gTempRect;
extern HWND gAppWindow;

i32 AppCommand(HWND window, u32 message, u32 messageParam, i32 messageData);
BOOL AppInit(HINSTANCE instance, HINSTANCE previousInstance, i32 showCommand, char* commandLine);
LRESULT CALLBACK AppWndProc(HWND window, UINT message, WPARAM messageParam, LPARAM messageData);
extern "C" BOOL __stdcall
AppAbout(HWND dialog, UINT message, WPARAM messageParam, LPARAM messageData);

inline HMENU MenuHandle(KBMenu menu) {
    return reinterpret_cast<HMENU>(menu);
}

inline KBMenu MenuFromHandle(HMENU menu) {
    return reinterpret_cast<KBMenu>(menu);
}

#endif
