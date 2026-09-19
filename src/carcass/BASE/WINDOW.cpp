// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x000cec20; preferred Buka symbol ??0heroWindow@@QAE@HHHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651321;margin=0.357646;shape=0.346;size=0.852;calls=1.000;strings=Dynamic Construct;alternate=pol20:void heroWindow::constructor(int, int, int, int, int)@0x000cec20
VA(0x00474a80, 0xb0)
heroWindow::heroWindow(int, int, int, int, int) {}

// donor PoL RVA 0x000cecd0; preferred Buka symbol ??0heroWindow@@QAE@HHPAD@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.375549;margin=0.549564;shape=0.254;size=0.606;calls=0.800;alternate=pol20:void heroWindow::constructor(int, int, char *)@0x000cecd0
VA(0x00474b30, 0x450)
heroWindow::heroWindow(int, int, char *) {}

// donor PoL RVA 0x000cf200; preferred Buka symbol ?Open@heroWindow@@QAEHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.353009;margin=0.367120;shape=0.216;size=0.638;calls=0.500;alternate=pol20:int heroWindow::Open(int, int)@0x000cf200
VA(0x00474f80, 0xa0)
int heroWindow::Open(int, int) { return 0; }

// donor PoL RVA 0x000cf310; preferred Buka symbol ?Close@heroWindow@@QAEXXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.586039;margin=0.311864;shape=0.455;size=0.909;calls=1.000;alternate=pol20:void heroWindow::Close(void)@0x000cf310
VA(0x00475020, 0xb0)
void heroWindow::Close(void) {}

// donor PoL RVA 0x000cf3c0; preferred Buka symbol ?AddWidget@heroWindow@@QAEXPAVwidget@@H@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.553676;margin=0.412288;shape=0.475;size=0.789;calls=1.000;alternate=pol20:void heroWindow::AddWidget(class widget *, int)@0x000cf3c0
VA(0x004750d0, 0x150)
void heroWindow::AddWidget(class widget *, int) {}

// donor PoL RVA 0x000cf500; preferred Buka symbol ?RemoveWidget@heroWindow@@QAEXPAVwidget@@@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.565639;margin=0.542930;shape=0.500;size=0.802;calls=1.000;alternate=pol20:void heroWindow::RemoveWidget(class widget *)@0x000cf500
VA(0x00475220, 0x120)
void heroWindow::RemoveWidget(class widget *) {}

// donor PoL RVA 0x000cf620; preferred Buka symbol ?BroadcastMessage@heroWindow@@QAEHAAUtag_message@@@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.435923;margin=0.333583;shape=0.340;size=0.588;calls=1.000;alternate=pol20:int heroWindow::BroadcastMessage(struct tag_message &)@0x000cf620
VA(0x00475340, 0xa0)
int heroWindow::BroadcastMessage(struct tag_message &) { return 0; }

// donor PoL RVA 0x000cf6e0; preferred Buka symbol ?DrawWindow@heroWindow@@QAEXH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.418036;margin=0.971201;shape=0.250;size=0.729;calls=1.000;alternate=pol20:void heroWindow::DrawWindow(int)@0x000cf6e0
VA(0x00475400, 0x30)
void heroWindow::DrawWindow(int) {}

// donor PoL RVA 0x000cf710; preferred Buka symbol ?DrawWindow@heroWindow@@QAEXHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.624064;margin=0.444596;shape=0.548;size=0.926;calls=1.000;alternate=pol20:void heroWindow::DrawWindow(int, int, int)@0x000cf710
VA(0x00475430, 0x100)
void heroWindow::DrawWindow(int, int, int) {}

// donor PoL RVA 0x000cf830; preferred Buka symbol ?SaveBackground@heroWindow@@QAEHXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570741;margin=0.294707;shape=0.467;size=0.852;calls=1.000;alternate=pol20:int heroWindow::SaveBackground(void)@0x000cf830
VA(0x00475530, 0x90)
int heroWindow::SaveBackground(void) { return 0; }

// donor PoL RVA 0x000cf8b0; preferred Buka symbol ?RestoreBackground@heroWindow@@QAEXXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.579338;margin=0.220950;shape=0.412;size=0.993;calls=1.000;alternate=pol20:void heroWindow::RestoreBackground(void)@0x000cf8b0
VA(0x004755c0, 0x90)
void heroWindow::RestoreBackground(void) {}

// donor PoL RVA 0x000cf950; preferred Buka symbol ?MoveWindow@heroWindow@@QAEXHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.486346;margin=0.742274;shape=0.333;size=0.777;calls=1.000;alternate=pol20:void heroWindow::MoveWindow(int, int)@0x000cf950
VA(0x00475650, 0x1e0)
void heroWindow::MoveWindow(int, int) {}
