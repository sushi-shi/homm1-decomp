// Modem transport. Buka 2.1 Modem correspondence: ModemSetup, the dial and
// packet helpers, then the packet writers, in Buka's order. Dial and
// WriteModemPacket start at odd addresses directly after the previous
// function, so retail compiled them in one object with ModemSetup. ModemSetup
// starts at 0x00459530, a 16-byte boundary that REMOTE's nbnet_init ends on.

#include <match.h>

#include <BASE/Misc.h>
#include <SOURCE/Modem.h>
#include <SOURCE/comwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/netwinRuntime.h>
#include <SOURCE/dialogTypes.h>

#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>
#include <string.h>

extern signed char gbDirectConnect;

// Buka 2.1 ModemSetup reset loop: open the port and reset a dial-up modem.
VA(0x00459530, 0xf7)
void ModemSetup(void) {
    char command[104];
    int resetAttempt;
    int i;

    com_init(gConfig.comPort[gbDirectConnect], 4, 0);
    if (!gbDirectConnect) {
        for (resetAttempt = 0; resetAttempt < 2; resetAttempt++) {
            if (gConfig.comPort[gbDirectConnect] >= 1)
                sprintf(command, gConfig.modemInitString);
            else
                sprintf(command, "ATZ");
            PollSound();
            ModemCommand(command);
            for (i = 0; i < 12; i++) {
                DelayMilli(10);
                PollSound();
            }
            ModemCommand("\r");
            DelayMilli(80);
            PollSound();
        }
    }
}

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

// donor PoL RVA 0x0000d4df; preferred Buka symbol ?WriteModemPacket@@YIXPADH@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.395642;margin=0.361596;shape=0.175;size=0.824;calls=0.667;alternate=pol20:void WriteModemPacket(char *, int)@0x0000d4df
VA(0x0045a16b, 0xdc)
void WriteModemPacket(char* buffer, int length) {
    char buf[544];
    int pos = 0;
    if (length > 256)
        return;

    buf[pos] = MODEM_PACKET_ESCAPE;
    ++pos;
    buf[pos] = 0;
    ++pos;
    while (length--) {
        if (*buffer == MODEM_PACKET_ESCAPE) {
            buf[pos] = MODEM_PACKET_ESCAPE;
            ++pos;
        }
        buf[pos] = *buffer;
        ++buffer;
        ++pos;
    }
    buf[pos] = MODEM_PACKET_ESCAPE;
    ++pos;
    buf[pos] = 1;
    ++pos;
    while (write_buffer(buf, pos) == 0)
        ForcePollSound();
}

// donor PoL RVA 0x000a3ec7; preferred Buka symbol ?TransmitRemoteData@@YIHPADHHCCCC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.560856;margin=1.192457;shape=0.450;size=0.831;calls=1.000;alternate=pol20:int TransmitRemoteData(char *, int, int, signed char, signed char, signed char, signed char)@0x000a3ec7
VA(0x0045a247, 0x231)
// HoMM1 callers pass an eighth flag that maps a game position to its net position.
int TransmitRemoteData(
    char* data,
    int destination,
    int length,
    signed char command,
    signed char reliable,
    signed char allowRetryDialog,
    signed char messageType,
    signed char gamePosDestination
) {
    int k;
    int retval;
    int j;
    RemoteMessage msg;
    int tries;

    if (!gbRemoteOn || gbInNetSetup)
        return 1;
    if (gamePosDestination && destination != REMOTE_BROADCAST_PLAYER)
        destination = gbGamePosToNetPos[destination];
    retval = 0;
    tries = 0;
    iIDCtr++;
    msg.sender = giThisNetPos;
    msg.id = iIDCtr;
    if (messageType != REMOTE_MESSAGE_DEFAULT)
        msg.type = messageType;
    else if (reliable)
        msg.type = REMOTE_MESSAGE_RELIABLE;
    else
        msg.type = REMOTE_MESSAGE_UNRELIABLE;
    msg.payloadSize = length;
    msg.command = command;
    if (length > 0)
        memcpy(msg.payload.data, data, length);
    while (retval == 0 && tries <= REMOTE_RETRY_COUNT) {
        retval = SendRemoteData(
            (unsigned char*)&msg,
            0,
            destination,
            length + REMOTE_MESSAGE_HEADER_SIZE
        );
        if (!reliable && retval) {
            return 1;
        } else if (retval) {
            k = 0;
            while (k < REMOTE_CONFIRM_POLL_COUNT) {
                ForcePollSound();
                if (giLastConfirm == iIDCtr)
                    return 1;
                retval = 0;
                DelayMilli(10);
                k++;
            }
        } else {
            DelayMilli(1000);
        }
        if (allowRetryDialog && tries == REMOTE_RETRY_COUNT && retval == 0) {
            NormalDialog("Error sending data.  Keep trying??", 2, -1, -1, -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                tries = -1;
        }
        tries++;
    }
    return retval;
}

// donor PoL RVA 0x000a40e1; preferred Buka symbol ?GetRemoteData@@YIPADC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.517569;margin=0.974708;shape=0.366;size=0.825;calls=1.000;alternate=pol20:char * GetRemoteData(signed char)@0x000a40e1
VA(0x0045a478, 0x10c)
char* GetRemoteData(signed char remove) {
    int oldest;
    int i;
    int index;

    if (!gbRemoteOn || gbInNetSetup)
        return 0;
    oldest = 999999999;
    index = -1;
    for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
        if (rcvBuf[i].type && iInOrder[i] < oldest) {
            oldest = iInOrder[i];
            index = i;
        }
    }
    if (index >= 0) {
        memcpy(rcvBufOut, &rcvBuf[index], REMOTE_MESSAGE_SIZE);
        if (remove)
            rcvBuf[index].type = REMOTE_MESSAGE_NONE;
        rcvBuf[index].sender = NetPosToGamePos(rcvBuf[index].sender);
        return rcvBufOut;
    }
    return 0;
}

// donor PoL RVA 0x000a41ec; preferred Buka symbol ?PollRemote@@YIXXZ
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void PollRemote(void)@0x000a41ec
// PollRemote's heartbeat clocks, timeout latch, recent-id ring and the
// incoming/outgoing message buffers.
extern long lLastHeartbeatSend;
extern long lLastHeartbeatReceive;
extern signed char bInTimeoutFail;
extern RemoteMessage sndBuf;
extern RemoteMessage rcvBufIn;
extern int iLastIds[REMOTE_RECENT_ID_COUNT];
extern int iInOrderCtr;
extern int iCurLastID;
// The other side's ready flag and the heartbeat-seen flag.
extern int gbRemoteReady;
extern int gbHeartbeatSeen;

VA(0x0045a584, 0x4fe)
void PollRemote(void) {
    signed char newControl;
    signed char queueFull;
    int i;
    int numQueued;
    int result;

    if (!gbRemoteOn)
        return;
    if (iMPBaseType == MULTIPLAYER_BASE_MODEM)
        comm_wrt_task();
    else if (iMPBaseType == MULTIPLAYER_BASE_NETWORK)
        nb_thr_ctl();
    if (gbInNetSetup)
        return;
    numQueued = 0;
    queueFull = 0;
    if (KBTickCount() - lLastHeartbeatSend > 5000) {
        sndBuf.sender = giThisNetPos;
        sndBuf.type = REMOTE_MESSAGE_HEARTBEAT;
        sndBuf.payloadSize = 1;
        sndBuf.command = (giCurPlayer << 4) + iCurHourGlassPhase;
        sndBuf.payload.data[0] = 1;
        SendRemoteData((unsigned char*)&sndBuf, 0, 1 - giThisNetPos, 10);
        lLastHeartbeatSend = KBTickCount();
    }
    if (KBTickCount() > lLastHeartbeatReceive + 60000 && !bInTimeoutFail) {
        NormalDialog(
            "The other player's computer is not responding.  Do you wish to wait longer?",
            2, -1, -1, -1, 0, -1, 0, -1);
        if (gpWindowManager->m_dialogResult == 0x7805) {
            lLastHeartbeatReceive = KBTickCount();
        } else {
            bInTimeoutFail = 1;
            if (gbHumanPlayer[giCurPlayer]) {
                if (giCurPlayer == giThisGamePos)
                    newControl = 0;
                else
                    newControl = 1;
            } else {
                if (giHostGamePos == giThisGamePos)
                    newControl = 0;
                else
                    newControl = 1;
            }
            ReceiveRemotePlayerExit(1 - giThisGamePos, newControl, 0, 1);
        }
    }
    for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
        if (rcvBuf[i].type)
            numQueued++;
    }
    if (numQueued == REMOTE_QUEUE_CAPACITY)
        queueFull = 1;
    result = 1;
    while (result) {
    nextIncoming:
        result = ReceiveRemoteData(0, (unsigned char*)&rcvBufIn, REMOTE_BROADCAST_PLAYER);
        if (result && rcvBufIn.sender != giThisNetPos) {
            if (rcvBufIn.type == REMOTE_MESSAGE_CONFIRM) {
                giLastConfirm = rcvBufIn.id;
                goto done;
            } else if (rcvBufIn.type == REMOTE_MESSAGE_HEARTBEAT) {
                if (rcvBufIn.payloadSize == 1 && rcvBufIn.payload.data[0] == 1)
                    gbRemoteReady = 1;
                lLastHeartbeatReceive = KBTickCount();
                gbHeartbeatSeen = 1;
                if (giHostGamePos != giThisGamePos && giCurPlayer != giThisGamePos
                    && gpAdvManager->m_active == 1 && rcvBufIn.command / 16 != giThisGamePos) {
                    giCurPlayer = rcvBufIn.command / 16;
                    iCurHourGlassPhase = rcvBufIn.command - giCurPlayer * 16;
                }
                goto done;
            } else if (queueFull) {
                goto done;
            }
            if (rcvBufIn.type == REMOTE_MESSAGE_RELIABLE) {
                sndBuf.sender = giThisNetPos;
                sndBuf.id = rcvBufIn.id;
                sndBuf.type = REMOTE_MESSAGE_CONFIRM;
                sndBuf.payloadSize = 0;
                SendRemoteData((unsigned char*)&sndBuf, 0, rcvBufIn.sender, REMOTE_MESSAGE_HEADER_SIZE);
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (rcvBuf[i].type && rcvBuf[i].id == rcvBufIn.id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_RECENT_ID_COUNT; i++) {
                if (iLastIds[i] == rcvBufIn.id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (!rcvBuf[i].type) {
                    iInOrder[i] = iInOrderCtr++;
                    memcpy(&rcvBuf[i], &rcvBufIn, REMOTE_MESSAGE_SIZE);
                    numQueued++;
                    iLastIds[iCurLastID] = rcvBufIn.id;
                    iCurLastID = (iCurLastID + 1) % REMOTE_RECENT_ID_COUNT;
                    if (numQueued == REMOTE_QUEUE_CAPACITY)
                        goto done;
                    goto nextIncoming;
                }
            }
        }
    }
done:;
}

// donor PoL RVA 0x000a48e0; preferred Buka symbol ?TransmitAndWait@@YIHPADHHCCPAPAD@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.561659;margin=0.362301;shape=0.477;size=0.829;calls=1.000;alternate=pol20:int TransmitAndWait(char *, int, int, signed char, signed char, char * *)@0x000a48e0
VA(0x0045aa82, 0x14f)
int TransmitAndWait(
    char* bytes,
    int destination,
    int length,
    signed char command,
    signed char responseCommand,
    char** response
) {
    int start;
    int result;
    RemoteMessage* received;
    char complete;

    if (!gbRemoteOn || gbInNetSetup)
        return 1;
    received = 0;
    result =
        TransmitRemoteData(bytes, destination, length, command, 1, 1, REMOTE_MESSAGE_DEFAULT, 1);
    if (result == 0)
        goto transmitComplete;
    start = KBTickCount();
    complete = 0;
    while (!complete) {
        if (KBTickCount() > start + 20000) {
            NormalDialog("Error sending data.  Keep trying??", 2, -1, -1, -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                start = KBTickCount();
            } else {
                result = 0;
                goto transmitComplete;
            }
        }
        ForcePollSound();
        received = (RemoteMessage*)GetRemoteData(1);
        if (received && received->type == REMOTE_MESSAGE_RELIABLE
            && received->command == responseCommand)
            complete = 1;
    }
    *response = (char*)received;
transmitComplete:
    return result;
}
