// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x00032230; preferred Buka symbol ??0strip@@QAE@HHHKHPAVarmyGroup@@HHH@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.710816;margin=0.058489;shape=0.438;size=0.937;calls=0.800;strings=strip.icn;alternate=pol20:void strip::constructor(int, int, int, unsigned long int, int, class armyGroup *, int, int, int)@0x00032230
VA(0x00463630, 0x2de)
strip::strip(int, int, int, unsigned long int, int, class armyGroup *, int, int, int) {}

// donor PoL RVA 0x000324ae; preferred Buka symbol ??1strip@@QAE@XZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.560366;margin=0.278966;shape=0.418;size=0.916;calls=1.000;alternate=pol20:void strip::~destructor(void)@0x000324ae
VA(0x0046390e, 0x112)
strip::~strip() {}

// donor PoL RVA 0x000325f2; preferred Buka symbol ?Draw@strip@@QAEXXZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.508282;margin=0.688808;shape=0.381;size=0.803;calls=1.000;alternate=pol20:void strip::Draw(void)@0x000325f2
VA(0x00463a20, 0x42)
void strip::Draw(void) {}

// donor PoL RVA 0x00032632; preferred Buka symbol ?DrawIcons@strip@@QAEXH@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411442;margin=0.465209;shape=0.280;size=0.697;calls=0.714;alternate=pol20:void strip::DrawIcons(int)@0x00032632
VA(0x00463a62, 0x26e)
void strip::DrawIcons(int) {}

// donor PoL RVA 0x00032a38; preferred Buka symbol ??0bankBox@@QAE@HHPAVplayerData@@@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.721149;margin=0.159729;shape=0.520;size=0.843;calls=0.833;strings=bankbox.bin;alternate=pol20:void bankBox::constructor(int, int, class playerData *)@0x00032a38
VA(0x00463d07, 0xfe)
bankBox::bankBox(int, int, class playerData *) {}

// donor PoL RVA 0x00032aea; preferred Buka symbol ??1bankBox@@QAE@XZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.486146;margin=0.167932;shape=0.360;size=0.776;calls=1.000;alternate=pol20:void bankBox::~destructor(void)@0x00032aea
VA(0x00463e05, 0x43)
bankBox::~bankBox() {}
