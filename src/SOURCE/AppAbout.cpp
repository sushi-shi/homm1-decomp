#include "match.h"
#include "RetailServices.h"
#include "Win32.h"

// Identity: PE export AppAbout, ordinal 1. Original TU ownership is unknown.
// Extent: entry through ret 16 at 0x45C1E9; next function starts at 0x45C1EC.
extern "C" RVA(0x0005C15C, 0x90)
BOOL __stdcall AppAbout(HWND hDlg, H1_ENUM_PARAM(WindowMessage, UINT) message, WPARAM wParam, LPARAM lParam)
{
    H1_ENUM_PARAM(DialogControl, int) wmId;
    WORD codeNotify;
    HWND hwndCtl;
    switch (message) {
    case WM_INITDIALOG:
        return 1;
    case WM_COMMAND:
        wmId = H1_ENUM_CAST(DialogControl, int, wParam & 0xffff);
        hwndCtl = reinterpret_cast<HWND>(lParam); // WM_COMMAND passes HWND in LPARAM.
        codeNotify = (wParam >> 16) & 0xffff;
        if (wmId == IDOK)
            EndDialog(hDlg, 1);
        break;
    }
    RetailService_0044F640();
    return 0;
}
