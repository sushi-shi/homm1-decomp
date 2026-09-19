#ifndef HOMM1_WIN32_H
#define HOMM1_WIN32_H

// Win32 ABI used by the recovered dialog fragment.
typedef void *HWND;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef int BOOL;
typedef unsigned short WORD;

enum WindowMessage { WM_INITDIALOG = 0x110, WM_COMMAND = 0x111 };
enum DialogControl { IDOK = 1 };

extern "C" __declspec(dllimport) BOOL __stdcall EndDialog(HWND, int);

#endif
