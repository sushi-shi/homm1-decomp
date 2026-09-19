// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <Win32.h>

#include <H1/KB.h>
#include <H2/_all.h>

// donor PoL RVA 0x0001bce0; preferred Buka symbol _WinMain@16
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651055;margin=0.328202;shape=0.658;size=0.820;calls=1.000;alternate=pol20:_WinMain@16@0x0001bce0
VA(0x0045b6f0, 0x14e)
H2_C_LINKAGE int __stdcall WinMain(void *, void *, char *, int) { return 0; }

// donor PoL RVA 0x0001be26; preferred Buka symbol ?AppInit@@YIHPAX0HPAD@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.682496;margin=0.205177;shape=0.345;size=0.971;calls=0.867;strings=Heroes|hInstApp;alternate=pol20:int AppInit(void *, void *, int, char *)@0x0001be26
VA(0x0045b83e, 0x2d6)
int AppInit(void *, void *, int, char *) { return 0; }

// donor PoL RVA 0x0001c190; preferred Buka symbol ?AppWndProc@@YGJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.508573;margin=0.535153;shape=0.364;size=0.977;calls=0.857;alternate=pol20:long int AppWndProc(void *, unsigned int, unsigned int, long int)@0x0001c190
VA(0x0045bb45, 0x617)
long int __stdcall AppWndProc(void *, unsigned int, unsigned int, long int) { return 0; }

// Identity: PE export AppAbout, ordinal 1.
// Extent: entry through ret 16 at 0x45c1e9; next function starts at 0x45c1ec.
extern "C" VA(0x0045c15c, 0x90)
BOOL __stdcall AppAbout(HWND hDlg, H1_ENUM_PARAM(WindowMessage, UINT) message,
                       WPARAM wParam, LPARAM lParam)
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
    PollSound();
    return 0;
}

// donor PoL RVA 0x0001c7b8; preferred Buka symbol ?Process1WindowsMessage@@YIXXZ
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.631126;margin=0.664983;shape=0.634;size=0.797;calls=1.000;alternate=pol20:void Process1WindowsMessage(void)@0x0001c7b8
VA(0x0045c206, 0xca)
void Process1WindowsMessage(void) {}

// donor PoL RVA 0x0001c880; preferred Buka symbol ?ResizeWindow@@YIXHHHH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.562416;margin=0.918799;shape=0.364;size=0.993;calls=1.000;alternate=pol20:void ResizeWindow(int, int, int, int)@0x0001c880
VA(0x0045c2d0, 0x127)
void ResizeWindow(int, int, int, int) {}

// donor PoL RVA 0x0001c9c7; preferred Buka symbol ?AppCommand@@YIJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.642433;margin=0.651384;shape=0.267;size=0.907;calls=1.000;strings=HEROES;alternate=pol20:long int AppCommand(void *, unsigned int, unsigned int, long int)@0x0001c9c7
VA(0x0045c3f7, 0x185)
long int AppCommand(void *, unsigned int, unsigned int, long int) { return 0; }

// donor PoL RVA 0x0001cc35; preferred Buka symbol ?KBChangeMenu@@YIXPAX@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.545069;margin=0.304682;shape=0.429;size=0.841;calls=1.000;alternate=pol20:void KBChangeMenu(void *)@0x0001cc35
VA(0x0045c64c, 0xaa)
void KBChangeMenu(void *) {}

// donor PoL RVA 0x0001cce1; preferred Buka symbol ?SetMenuStatus@@YIXH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.517140;margin=0.517010;shape=0.323;size=0.903;calls=1.000;alternate=pol20:void SetMenuStatus(int)@0x0001cce1
VA(0x0045c6f6, 0x135)
void SetMenuStatus(int) {}

// donor PoL RVA 0x0001ce3d; preferred Buka symbol ?SetNoDialogMenus@@YIXH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.510874;margin=0.487078;shape=0.429;size=0.686;calls=1.000;alternate=pol20:void SetNoDialogMenus(int)@0x0001ce3d
VA(0x0045c82b, 0x79)
void SetNoDialogMenus(int) {}

// donor PoL RVA 0x000a0c76; preferred Buka symbol ?SetWinText@@YIXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.447557;margin=0.235076;shape=0.180;size=0.912;calls=1.000;alternate=pol20:void SetWinText(class heroWindow *, int)@0x000a0c76
VA(0x0045dc1f, 0x7c)
void SetWinText(heroWindow *j, int id)
{}

// donor PoL RVA 0x0001d011; preferred Buka symbol ?KBTickCount@@YIJXZ
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:long int KBTickCount(void)@0x0001d011
VA(0x0045dc9b, 0x16)
long int KBTickCount(void) { return 0; }

// donor PoL RVA 0x000c47f0; preferred Buka symbol ?ProcessAssert@@YIXHPADH@Z
// donor Buka TU BASE/Misc; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.598916;margin=0.432613;shape=0.279;size=0.853;calls=0.600;strings=Assert Failure;alternate=pol20:void ProcessAssert(int, char *, int)@0x000c47f0
VA(0x0045dcb1, 0x63)
void ProcessAssert(int condition, char *file, int line)
{}
