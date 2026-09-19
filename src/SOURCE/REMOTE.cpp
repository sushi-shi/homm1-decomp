// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>

// donor PoL RVA 0x0000d4df; preferred Buka symbol ?WriteModemPacket@@YIXPADH@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.395642;margin=0.361596;shape=0.175;size=0.824;calls=0.667;alternate=pol20:void WriteModemPacket(char *, int)@0x0000d4df
VA(0x0045a16b, 0xdc)
void WriteModemPacket(char *, int) {}

// donor PoL RVA 0x000a3ec7; preferred Buka symbol ?TransmitRemoteData@@YIHPADHHCCCC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.560856;margin=1.192457;shape=0.450;size=0.831;calls=1.000;alternate=pol20:int TransmitRemoteData(char *, int, int, signed char, signed char, signed char, signed char)@0x000a3ec7
VA(0x0045a247, 0x231)
int TransmitRemoteData(char *, int, int, signed char, signed char, signed char, signed char) { return 0; }

// donor PoL RVA 0x000a40e1; preferred Buka symbol ?GetRemoteData@@YIPADC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.517569;margin=0.974708;shape=0.366;size=0.825;calls=1.000;alternate=pol20:char * GetRemoteData(signed char)@0x000a40e1
VA(0x0045a478, 0x10c)
char * GetRemoteData(signed char) { return 0; }

// donor PoL RVA 0x000a41ec; preferred Buka symbol ?PollRemote@@YIXXZ
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void PollRemote(void)@0x000a41ec
VA(0x0045a584, 0x4fe)
void PollRemote(void) {}

// donor PoL RVA 0x000a48e0; preferred Buka symbol ?TransmitAndWait@@YIHPADHHCCPAPAD@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.561659;margin=0.362301;shape=0.477;size=0.829;calls=1.000;alternate=pol20:int TransmitAndWait(char *, int, int, signed char, signed char, char * *)@0x000a48e0
VA(0x0045aa82, 0x15e)
int TransmitAndWait(char *, int, int, signed char, signed char, char * *) { return 0; }
