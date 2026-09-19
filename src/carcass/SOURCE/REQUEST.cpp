// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H2/_all.h>

// donor PoL RVA 0x0008d5e1; preferred Buka symbol ?Open@fileRequester@@UAEHH@Z
// donor Buka TU SOURCE/REQUEST; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.718088;margin=0.176393;shape=0.403;size=0.976;calls=0.963;strings=fileRequester|request.bin;alternate=pol20:int fileRequester::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x0008d5e1
VA(0x004489c4, 0x431)
int fileRequester::Open(int) { return 0; }

// donor PoL RVA 0x0008daec; preferred Buka symbol ?Main@fileRequester@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/REQUEST; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.521487;margin=0.114044;shape=0.300;size=0.615;calls=0.600;strings=$%'-_@~`!(){}^#&+,;=[].;alternate=pol20:int fileRequester::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x0008daec
VA(0x00448e7f, 0xa8c)
int fileRequester::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x0008ec9a; preferred Buka symbol ?DoKnob@fileRequester@@QAEXXZ
// donor Buka TU SOURCE/REQUEST; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.459180;margin=0.170271;shape=0.236;size=0.874;calls=0.875;alternate=pol20:void fileRequester::DoKnob(void)@0x0008ec9a
VA(0x004499a8, 0x2b2)
void fileRequester::DoKnob(void) {}

// donor PoL RVA 0x0008ef82; preferred Buka symbol ?Update@fileRequester@@QAEXH@Z
// donor Buka TU SOURCE/REQUEST; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.311908;margin=0.264606;shape=0.243;size=0.506;calls=0.526;alternate=pol20:void fileRequester::Update(int)@0x0008ef82
VA(0x00449c5a, 0x55e)
void fileRequester::Update(int) {}
