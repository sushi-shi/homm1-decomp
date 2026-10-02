// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/Modem.h>
#include <SOURCE/comwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>

#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>
#include <string.h>

// donor PoL RVA 0x0000cb3e; preferred Buka symbol ?Dial@@YIJXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.658408;margin=0.279474;shape=0.349;size=0.861;calls=1.000;strings=%s %s|ATDT%s|CONNECT;alternate=pol20:long int Dial(void)@0x0000cb3e
VA(0x00459627, 0xa5)
long int Dial(void) {
    char dialCommand[40];
    iLastDialPos = 0;
    sprintf(dialCommand, "ATDT%s", numbuf);
    sprintf(gText, "%s %s", "Dialing...", numbuf);
    GUIModemCommand(gText, dialCommand);
    sprintf(gText, "%s %s", "Dialing...", numbuf);
    if (GUIModemResponse(gText, "CONNECT"))
        return 1;
    return 0;
}

// donor PoL RVA 0x0000cbdc; preferred Buka symbol ?Wait@@YIJXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.520648;margin=0.735557;shape=0.143;size=0.710;calls=1.000;strings=CONNECT|RING;alternate=pol20:long int Wait(void)@0x0000cbdc
VA(0x004596cc, 0x5d)
long int Wait(void) {
    GUIModemResponse("Waiting for ring...", "RING");
    GUIModemCommand("Initializing modem...", "ATA");
    if (GUIModemResponse("Establishing connection...", "CONNECT"))
        return 1;
    return 0;
}

// donor PoL RVA 0x0000cc30; preferred Buka symbol ?GUIModemCommand@@YIXPAD0@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.504929;margin=0.542655;shape=0.294;size=0.956;calls=1.000;alternate=pol20:void GUIModemCommand(char *, char *)@0x0000cc30
VA(0x00459729, 0x71)
void GUIModemCommand(char* message, char* command) {
    iLastActionTime = 0;
    iModemCommandPos = 0;
    giWaitType = 5;
    strcpy(cModemCommand, command);
    NormalDialog(message, 6, -1, -1, -1, 0, -1, 0, -1);
    if (!gbFunctionComplete)
        ShutDown(0);
}

// donor PoL RVA 0x0000cca9; preferred Buka symbol ?GUIModemCommandExec@@YICXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.478276;margin=0.078716;shape=0.283;size=0.851;calls=1.000;alternate=pol20:signed char GUIModemCommandExec(void)@0x0000cca9
VA(0x0045979a, 0x94)
signed char GUIModemCommandExec(void) {
    int commandLength;
    if (KBTickCount() < iLastActionTime + 250)
        return 0;

    iLastActionTime = KBTickCount();
    commandLength = strlen(cModemCommand);
    if (iModemCommandPos < commandLength) {
        write_buffer(cModemCommand + iModemCommandPos, 1);
        ++iModemCommandPos;
        return 0;
    } else {
        write_buffer("\r", 1);
        return 1;
    }
}

// Buka 2.1 ModemCommand; HoMM1 writes one command byte at a time.
VA(0x0045982e, 0x6c)
void ModemCommand(char* command) {
    int pos;
    int len = strlen(command);
    for (pos = 0; pos < len; ++pos) {
        write_buffer(command + pos, 1);
        DelayMilli(100);
    }
    write_buffer("\r", 1);
}

// donor PoL RVA 0x0000cdcc; preferred Buka symbol ?GUIModemResponse@@YICPAD0@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.487980;margin=0.528115;shape=0.250;size=0.959;calls=1.000;alternate=pol20:signed char GUIModemResponse(char *, char *)@0x0000cdcc
VA(0x0045989a, 0x7a)
signed char GUIModemResponse(char* message, char* response) {
    memset(GUIMRresponse, 0, 80);
    GUIMRrespptr = 0;
    strcpy(GUIMRresp, response);
    giWaitType = 6;
    NormalDialog(message, 6, -1, -1, -1, 0, -1, 0, -1);
    if (!gbFunctionComplete)
        ShutDown(0);
    return 0;
}

// donor PoL RVA 0x0000ce4e; preferred Buka symbol ?GUIModemResponseExec@@YICXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.623720;margin=0.638042;shape=0.611;size=0.792;calls=1.000;alternate=pol20:signed char GUIModemResponseExec(void)@0x0000ce4e
VA(0x00459914, 0xe2)
signed char GUIModemResponseExec(void) {
    GUIMRc = read_byte();
    if (GUIMRc == -1)
        return 0;
    if (GUIMRc == '\n' || GUIMRrespptr == MODEM_RESPONSE_LAST) {
        GUIMRresponse[GUIMRrespptr] = 0;
        if (GUIMRrespptr > 17)
            GUIMRresponse[17] = 0;
        goto compareResponse;
    }
    if (GUIMRc >= ' ') {
        GUIMRresponse[GUIMRrespptr] = static_cast<char>(GUIMRc);
        ++GUIMRrespptr;
    }
    return 0;
compareResponse:
    if (strncmp(GUIMRresponse, GUIMRresp, strlen(GUIMRresp)) != 0) {
        GUIMRrespptr = 0;
        return 0;
    } else {
        return 1;
    }
}

// Buka 2.1 serial queue helpers; HoMM1 has no outgoing-queue guard.
VA(0x004599f6, 0x2b)
int write_buffer(char* buffer, int length) {
    com_snd(0, 0, length, buffer, 0);
    return 1;
}

VA(0x00459a21, 0x47)
int read_byte(void) {
    unsigned char ch;
    int received = com_rcv(0, 1, &ch);
    if (received == 1)
        return ch;
    else
        return -1;
}

VA(0x00459a68, 0x24)
void write_byte(int value) {
    com_snd(0, 0, 1, &value, 0);
}

// donor PoL RVA 0x0000cfec; preferred Buka symbol ?Connect@@YIXXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.591174;margin=0.244003;shape=0.392;size=0.585;calls=0.933;strings=ID%s_%i;alternate=pol20:void Connect(void)@0x0000cfec
VA(0x00459a8c, 0x2c0)
void Connect(void) {
    int result;
    char msg[20];
    unsigned long seed = KBTickCount();
    seed %= 1000000;
    idstr[0] = seed / 100000 + '0';
    seed -= (idstr[0] - '0') * 100000;
    idstr[1] = seed / 10000 + '0';
    seed -= (idstr[1] - '0') * 10000;
    idstr[2] = seed / 1000 + '0';
    seed -= (idstr[2] - '0') * 1000;
    idstr[3] = seed / 100 + '0';
    seed -= (idstr[3] - '0') * 100;
    idstr[4] = seed / 10 + '0';
    seed -= (idstr[4] - '0') * 10;
    idstr[5] = seed + '0';
    idstr[6] = 0;
    oldsec = -1;
    remotestage = 0;
    localstage = remotestage;
    do {
        if (ReadPacket()) {
            packet[packetlen] = 0;
            if (packetlen != 10)
                continue;
            if (strncmp(packet, "ID", 2))
                continue;
            if (!strncmp(packet + 2, idstr, 6)) {
                sprintf(gText, "Duplicate ID Strings!\nSorry Please Try Again\n");
                GOut(gText);
                RemoteCleanup();
            }
            strncpy(remoteidstr, packet + 2, 6);
            remotestage = packet[9] - '0';
            localstage = remotestage + 1;
            oldsec = -1;
        }
        stime = KBTickCount();
        if (oldsec / 1000 != stime / 1000) {
            oldsec = stime;
            sprintf(msg, "ID%s_%i", idstr, localstage);
            WriteModemPacket(msg, strlen(msg));
        }
        PollSound();
    } while (localstage < 2);
    while (ReadPacket()) {
    }
}

// donor PoL RVA 0x0000d1a7; preferred Buka symbol ?WaitForDirectConnect@@YIHXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.592752;margin=0.888123;shape=0.373;size=0.613;calls=0.929;strings=ID%s_%i;alternate=pol20:int WaitForDirectConnect(void)@0x0000d1a7
VA(0x00459d4c, 0x316)
int WaitForDirectConnect(void) {
    char idMessage[20];
    unsigned long seed;
    switch (WFDCStage) {
        case 0:
            seed = KBTickCount();
            seed %= 1000000;
            idstr[0] = seed / 100000 + '0';
            seed -= (idstr[0] - '0') * 100000;
            idstr[1] = seed / 10000 + '0';
            seed -= (idstr[1] - '0') * 10000;
            idstr[2] = seed / 1000 + '0';
            seed -= (idstr[2] - '0') * 1000;
            idstr[3] = seed / 100 + '0';
            seed -= (idstr[3] - '0') * 100;
            idstr[4] = seed / 10 + '0';
            seed -= (idstr[4] - '0') * 10;
            idstr[5] = seed + '0';
            idstr[6] = 0;
            oldsec = -1;
            remotestage = 0;
            localstage = remotestage;
            WFDCStage++;
            break;
        case 1:
            if (ReadPacket()) {
                packet[packetlen] = 0;
                if (packetlen != 10)
                    return 0;
                if (strncmp(packet, "ID", 2))
                    return 0;
                if (!strncmp(packet + 2, idstr, 6)) {
                    sprintf(gText, "Duplicate ID Strings!\nSorry Please Try Again\n");
                    GOut(gText);
                    RemoteCleanup();
                }
                strncpy(remoteidstr, packet + 2, 6);
                remotestage = packet[9] - '0';
                localstage = remotestage + 1;
                oldsec = -1;
            }
            stime = KBTickCount();
            if (oldsec / 1000 != stime / 1000) {
                oldsec = stime;
                sprintf(idMessage, "ID%s_%i", idstr, localstage);
                WriteModemPacket(idMessage, strlen(idMessage));
            }
            if (localstage >= 2)
                WFDCStage++;
            break;
        case 2:
            if (!ReadPacket())
                return 1;
            break;
    }
    return 0;
}

// donor PoL RVA 0x0000d3b8; preferred Buka symbol ?ReadPacket@@YIDXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.530036;margin=0.483413;shape=0.466;size=0.966;calls=0.333;alternate=pol20:char ReadPacket(void)@0x0000d3b8
VA(0x0045a062, 0x109)
char ReadPacket(void) {
    int input;
    // Unused; retail reserves 0x20 bytes with the input below it.
    char buffer[28];
    if (inque.writePosition > 4092) {
        inque.writePosition = 0;
        newpacket = 1;
    }
readPacketStart:
    if (newpacket) {
        packetlen = 0;
        newpacket = 0;
    }
    do {
    readNextByte:
        input = read_byte();
        if (input < 0)
            return 0;
        if (inescape) {
            inescape = 0;
            if (input == 1) {
                newpacket = 1;
                return 1;
            } else if (input == 0) {
                newpacket = 1;
                goto readPacketStart;
            }
        } else if (input == MODEM_PACKET_ESCAPE) {
            inescape = 1;
            goto readNextByte;
        }
        if (packetlen >= 256)
            goto readPacketStart;
        packet[packetlen] = static_cast<char>(input);
        ++packetlen;
    } while (1);
}

// Modem's data shares the retail object that starts at 0x00458520 (see SETUP).
DATA(0x0049f828)
int packetlen = 0;
DATA(0x0049f82c)
int inescape = 0;
DATA(0x0049f830)
int newpacket = 0;
DATA(0x004c7e70)
char idstr[8];
DATA(0x004c7f78)
int GUIMRc;
DATA(0x004c7f7c)
int iModemCommandPos;
DATA(0x004c7f80)
int GUIMRrespptr;
DATA(0x004c7f84)
int localstage;
DATA(0x004c7f88)
char numbuf[40];
DATA(0x004c8028)
int WFDCStage;
DATA(0x004c8030)
char remoteidstr[8];
DATA(0x004c8138)
int stime;
DATA(0x004c8260)
char cModemCommand[40];
DATA(0x004c8288)
int iLastDialPos;
DATA(0x004c828c)
int remotestage;
DATA(0x004c8298)
char GUIMRresp[40];
DATA(0x004c82c0)
int oldsec;
DATA(0x004c82c8)
inque_t inque;
DATA(0x004c92d8)
char packet[256];
DATA(0x004c93d8)
int iLastActionTime;
DATA(0x004c94e0)
char GUIMRresponse[80];
