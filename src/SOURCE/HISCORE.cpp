// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x00089a96; preferred Buka symbol ?Open@highScoreManager@@UAEHH@Z
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.779726;margin=0.242262;shape=0.537;size=0.989;calls=0.923;strings=highScoreManager|hiscore.bin;alternate=pol20:int highScoreManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00089a96
VA(0x004010bf, 0x169)
int highScoreManager::Open(int) { return 0; }

// donor PoL RVA 0x00089c40; preferred Buka symbol ?Main@highScoreManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479652;margin=0.177846;shape=0.336;size=0.742;calls=1.000;alternate=pol20:int highScoreManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00089c40
VA(0x00401285, 0x269)
int highScoreManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x00089e6a; preferred Buka symbol ?Update@highScoreManager@@QAEXXZ
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.683474;margin=0.421632;shape=0.404;size=0.957;calls=0.730;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:void highScoreManager::Update(void)@0x00089e6a
VA(0x004014ee, 0x672)
void highScoreManager::Update(void) {}
