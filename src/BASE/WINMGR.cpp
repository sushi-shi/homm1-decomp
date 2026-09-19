// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x000cac40; preferred Buka symbol ?BroadcastMessage@heroWindowManager@@QAEHHHHH@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.484375;margin=1.382188;shape=0.250;size=0.844;calls=1.000;alternate=pol20:int heroWindowManager::BroadcastMessage(int, int, int, int)@0x000cac40
VA(0x00474130, 0x40)
short heroWindowManager::BroadcastMessage(short, short, short, short) { return 0; }

// donor PoL RVA 0x000cad40; preferred Buka symbol ?RemoveWindow@heroWindowManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.383824;margin=0.385483;shape=0.180;size=0.618;calls=1.000;alternate=pol20:void heroWindowManager::RemoveWindow(class heroWindow *)@0x000cad40
VA(0x00474240, 0x90)
void heroWindowManager::RemoveWindow(class heroWindow *) {}

// donor PoL RVA 0x000cb1e0; preferred Buka symbol ?FizzleForward@heroWindowManager@@QAEXHHHHHPAC0@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.536514;margin=0.381106;shape=0.221;size=0.706;calls=0.783;strings=CCYCLE%02d.BIN;alternate=pol20:void heroWindowManager::FizzleForward(int, int, int, int, int, signed char *, signed char *)@0x000cb1e0
VA(0x00474740, 0x320)
void heroWindowManager::FizzleForward(int, int, int, int, int, signed char *, signed char *) {}
