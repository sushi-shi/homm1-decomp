// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x000bf340; preferred Buka symbol ?DoTradingPost@@YIXHM@Z
// donor Buka TU SOURCE/tradpost; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.511421;margin=0.057302;shape=0.391;size=0.883;calls=0.600;alternate=pol20:void DoTradingPost(int, float)@0x000bf340
VA(0x004567f0, 0x164)
void DoTradingPost(int, float) {}

// donor PoL RVA 0x00010ebf; preferred Buka symbol ?SetupBaud@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.648115;margin=0.123105;shape=0.395;size=0.777;calls=0.833;strings=stpbaud.bin;alternate=pol20:int game::SetupBaud(void)@0x00010ebf
VA(0x00456954, 0x190)
int game::SetupBaud(void) { return 0; }

// donor PoL RVA 0x00011000; preferred Buka symbol ?SetupComPort@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.646978;margin=0.131942;shape=0.348;size=0.872;calls=0.750;strings=stpcom.bin;alternate=pol20:int game::SetupComPort(void)@0x00011000
VA(0x00456ae4, 0x222)
int game::SetupComPort(void) { return 0; }

// donor PoL RVA 0x00011200; preferred Buka symbol ?SetupHotSeatGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.510359;margin=0.105262;shape=0.279;size=0.627;calls=0.545;strings=stphotst.bin;alternate=pol20:int game::SetupHotSeatGame(void)@0x00011200
VA(0x00456d06, 0x15d)
int game::SetupHotSeatGame(void) { return 0; }

// donor PoL RVA 0x00011438; preferred Buka symbol ?SetupNetworkGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.778953;margin=0.116684;shape=0.550;size=0.938;calls=1.000;strings=stpnet.bin;alternate=pol20:int game::SetupNetworkGame(void)@0x00011438
VA(0x00456e63, 0x133)
int game::SetupNetworkGame(void) { return 0; }

// donor PoL RVA 0x00011795; preferred Buka symbol ?SetupModemGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.697331;margin=0.184673;shape=0.396;size=0.996;calls=0.720;strings=stpdc.bin|stpdccfg.bin|stpmcfg.bin;alternate=pol20:int game::SetupModemGame(void)@0x00011795
VA(0x00456f96, 0x333)
int game::SetupModemGame(void) { return 0; }

// donor PoL RVA 0x00011aac; preferred Buka symbol ?SetupMultiPlayerGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.675100;margin=0.160042;shape=0.444;size=0.973;calls=0.529;strings=stpmp.bin;alternate=pol20:int game::SetupMultiPlayerGame(void)@0x00011aac
VA(0x004572c9, 0x218)
int game::SetupMultiPlayerGame(void) { return 0; }

// donor PoL RVA 0x000123cc; preferred Buka symbol ?PickLoadGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.594268;margin=0.566491;shape=0.333;size=0.748;calls=0.722;strings=.\GAMES\;alternate=pol20:int game::PickLoadGame(void)@0x000123cc
VA(0x00457967, 0x1e7)
int game::PickLoadGame(void) { return 0; }

// donor PoL RVA 0x0000c8f0; preferred Buka symbol ?ModemSetup@@YIXH@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.387701;margin=0.229361;shape=0.196;size=0.795;calls=0.591;alternate=pol20:void ModemSetup(int)@0x0000c8f0
VA(0x0045869c, 0x27a)
void ModemSetup(int) {}

// donor PoL RVA 0x000a3aa7; preferred Buka symbol ?DecodePacket@@YIHPAEH@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.496845;margin=0.356674;shape=0.447;size=0.760;calls=0.750;alternate=pol20:int DecodePacket(unsigned char *, int)@0x000a3aa7
VA(0x00458aad, 0x162)
int DecodePacket(unsigned char *, int) { return 0; }

// donor PoL RVA 0x000a3be1; preferred Buka symbol ?SendRemoteData@@YIHPAE0HH@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.468075;margin=0.614352;shape=0.312;size=0.933;calls=0.500;alternate=pol20:int SendRemoteData(unsigned char *, unsigned char *, int, int)@0x000a3be1
VA(0x00458c0f, 0x141)
int SendRemoteData(unsigned char *, unsigned char *, int, int) { return 0; }

// donor PoL RVA 0x000a3d6f; preferred Buka symbol ?ReceiveRemoteData@@YIHPAE0H@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.404111;margin=0.668725;shape=0.214;size=0.850;calls=0.500;alternate=pol20:int ReceiveRemoteData(unsigned char *, unsigned char *, int)@0x000a3d6f
VA(0x00458d50, 0xf4)
int ReceiveRemoteData(unsigned char *, unsigned char *, int) { return 0; }

// donor PoL RVA 0x000132f0; preferred Buka symbol ?InitNetHost@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.405636;margin=0.349549;shape=0.179;size=0.703;calls=1.000;alternate=pol20:signed char InitNetHost(void)@0x000132f0
VA(0x00458e44, 0x194)
signed char InitNetHost(void) { return 0; }

// donor PoL RVA 0x00013445; preferred Buka symbol ?InitNetGuest@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.423322;margin=0.095753;shape=0.173;size=0.829;calls=0.833;alternate=pol20:signed char InitNetGuest(void)@0x00013445
VA(0x00458fd8, 0x1f1)
signed char InitNetGuest(void) { return 0; }

// donor PoL RVA 0x0001364f; preferred Buka symbol ?WaitForGuest@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.465490;margin=0.416814;shape=0.321;size=0.696;calls=1.000;alternate=pol20:signed char WaitForGuest(void)@0x0001364f
VA(0x00459267, 0x101)
signed char WaitForGuest(void) { return 0; }
