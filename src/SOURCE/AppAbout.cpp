#include "match.h"

// Minimal Win32 ABI declarations; no SDK headers needed by this fragment.
typedef void *HWND;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef int BOOL;
typedef unsigned short WORD;

enum { WM_INITDIALOG = 0x110, WM_COMMAND = 0x111, IDOK = 1 };

extern "C" __declspec(dllimport) BOOL __stdcall EndDialog(HWND, int);
// Provisional address-based name: this service's full behavior is not recovered.
extern "C" void __cdecl RetailService_0044F640();

// Identity: PE export AppAbout, ordinal 1. Original TU ownership is unknown.
// Extent: entry through ret 16 at 0x45C1E9; next function starts at 0x45C1EC.
RVA(0x0005C15C, 0x90)
extern "C" BOOL __stdcall AppAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;
    WORD codeNotify;
    HWND hwndCtl;
    switch (message) {
    case WM_INITDIALOG:
        return 1;
    case WM_COMMAND:
        wmId = wParam & 0xffff;
        hwndCtl = (HWND)lParam;
        codeNotify = (wParam >> 16) & 0xffff;
        if (wmId == IDOK)
            EndDialog(hDlg, 1);
        break;
    }
    RetailService_0044F640();
    return 0;
}
