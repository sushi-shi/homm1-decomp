// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x0008b310; preferred Buka symbol ?SetupRecruitWin@@YIXPAVheroWindow@@HHHHH@Z
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.642179;margin=0.286053;shape=0.397;size=0.715;calls=0.929;strings=%s %s|%s%d;alternate=pol20:void SetupRecruitWin(class heroWindow *, int, int, int, int, int)@0x0008b310
VA(0x00401b60, 0x164)
void SetupRecruitWin(class heroWindow *, int, int, int, int, int) {}

// donor PoL RVA 0x0008b49c; preferred Buka symbol ?Open@recruitUnit@@UAEHH@Z
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.766621;margin=0.519238;shape=0.510;size=0.964;calls=1.000;strings=recruit0.bin|recruit1.bin|recruitManager;alternate=pol20:int recruitUnit::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x0008b49c
VA(0x00401cc4, 0x282)
int recruitUnit::Open(int) { return 0; }

// donor PoL RVA 0x0008b6e7; preferred Buka symbol ?Close@recruitUnit@@UAEXXZ
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.515555;margin=0.462411;shape=0.327;size=0.957;calls=0.857;alternate=pol20:void recruitUnit::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x0008b6e7
VA(0x00401f46, 0xd1)
void recruitUnit::Close(void) {}

// donor PoL RVA 0x0008b7ce; preferred Buka symbol ?Update@recruitUnit@@QAEXXZ
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.730023;margin=0.237862;shape=0.449;size=0.939;calls=1.000;strings=%s%d;alternate=pol20:void recruitUnit::Update(void)@0x0008b7ce
VA(0x00402017, 0x127)
void recruitUnit::Update(void) {}

// donor PoL RVA 0x0008b8f0; preferred Buka symbol ?Main@recruitUnit@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.367477;margin=0.265938;shape=0.265;size=0.928;calls=0.231;alternate=pol20:int recruitUnit::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x0008b8f0
VA(0x0040213e, 0x3e5)
int recruitUnit::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x0008bd0b; preferred Buka symbol ??0recruitUnit@@QAE@PAVarmyGroup@@HPAF@Z
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.525485;margin=0.166304;shape=0.333;size=0.916;calls=1.000;alternate=pol20:void recruitUnit::constructor(class armyGroup *, int, short int *)@0x0008bd0b
VA(0x00402523, 0xd6)
recruitUnit::recruitUnit(class armyGroup *, int, short int *) {}

// donor PoL RVA 0x0008bdea; preferred Buka symbol ??0recruitUnit@@QAE@PAVtown@@HH@Z
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.543110;margin=0.262899;shape=0.371;size=0.914;calls=1.000;alternate=pol20:void recruitUnit::constructor(class town *, int, int)@0x0008bdea
VA(0x004025f9, 0xf4)
recruitUnit::recruitUnit(class town *, int, int) {}

// donor PoL RVA 0x0008bee5; preferred Buka symbol ?QuickViewRecruit@@YIXPAVtown@@H@Z
// donor Buka TU SOURCE/RECRUIT; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.627669;margin=0.094785;shape=0.388;size=0.783;calls=0.727;strings=recruiq0.bin|recruiq1.bin;alternate=pol20:void QuickViewRecruit(class town *, int)@0x0008bee5
VA(0x004026ed, 0x1c3)
void QuickViewRecruit(class town *, int) {}
