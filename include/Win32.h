#ifndef HOMM1_WIN32_H
#define HOMM1_WIN32_H

#include "Domains.h"

// Win32 ABI used by the recovered dialog fragment.
typedef void *HWND;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef int BOOL;
typedef unsigned short WORD;

H1_ENUM_BEGIN(WindowMessage)
    WM_INITDIALOG = 0x110,
    WM_COMMAND = 0x111
H1_ENUM_END(WindowMessage)
H1_ENUM_BEGIN(DialogControl)
    IDOK = 1
H1_ENUM_END(DialogControl)

extern "C" __declspec(dllimport) BOOL __stdcall EndDialog(HWND, int);

#endif
