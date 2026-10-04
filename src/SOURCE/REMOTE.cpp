// Remote play: the network and modem transports and their packet layer.
// Buka 2.1 REMOTE, Netbios and Modem correspondence. Retail compiled these as
// one object: RemoteCleanup starts it at 0x00458520 after SETUP's int3 fill,
// Dial (0x00459627) and WriteModemPacket (0x0045a16b) start at odd addresses
// directly after their predecessors, and the object's .data
// (0x004a2c00-0x0049fdb7) and .bss (0x004c7e70-0x004ca487) interleave the
// network, modem and packet-layer variables.

#include <match.h>

#include <SOURCE/REMOTE.h>

#include <BASE/heroWindowManager.h>
#include <BASE/Misc.h>
#include <SOURCE/advManager.h>
#include <SOURCE/comwin.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/netwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/SETUP.h>

#include <stdio.h>
#include <string.h>

// Buka 2.1 RemoteCleanup without the HoMM2 logging and DirectPlay modes.
VA(0x004519f0, 0x66)
void RemoteCleanup(void) {
    if (!gRemoteOn)
        return;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            UnloadRemoteDriver(REMOTE_DRIVER_NETBIOS);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            UnloadRemoteDriver(REMOTE_DRIVER_SERIAL);
            break;
        default:
            break;
    }
    gRemoteOn = 0;
}

// @dead-code
// Zero-ref: reads one block from a file offset into the caller buffer.
VA(0x00451a56, 0x65)
void* ReadFileBlock(char* filename, void* buffer, i32 size, i32 offset) {
    FILE* fp;
    fp = fopen(filename, "r+b");
    if (!fp)
        FileError(filename);
    fseek(fp, offset, SEEK_SET);
    fread(buffer, size, 1, fp);
    fclose(fp);
    return buffer;
}

// Buka 2.1 MiscRuntime FileSize.
VA(0x00470c21, 0x7b)
i32 FileSize(char* filename) {
    i32 length;
    FILE* f;
    f = fopen(filename, "r+b");
    if (!f)
        FileError(filename);
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    fclose(f);
    return length;
}

// Modem's 2K transmit queue (retail 0x004c9c80-0x004ca487), defined below.
struct outque_t {
    i32 readPosition;
    i32 writePosition;
    char data[2048];
};

// Buka 2.1 RemoteMain merged with the HoMM2 ModemSetup mode switch; HoMM1
// keeps the modem reset sequence in ModemSetup (0x459530).
VA(0x00470c9c, 0x27a)
void RemoteMain(i32 gameMode) {
    char directConnectMessage[164];

    gInNetSetup = 1;
    memset(rcvBuf, 0, sizeof(rcvBuf));
    memset(iLastIds, 0, 30);
    GameMode = gameMode;
    switch (gameMode) {
        case REMOTE_GAME_NETWORK_HOST:
            nbnet_init();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            nbnet_init();
            break;
        case REMOTE_GAME_MODEM_HOST:
            giThisNetPos = 0;
            goto modemStart;
        case REMOTE_GAME_MODEM_GUEST:
            giThisNetPos = 1;
        modemStart:
            gRemoteOn = 1;
            gNumNetGuests = 1;
            inque.writePosition = 0;
            inque.readPosition = 0;
            outque.writePosition = 0;
            outque.readPosition = 0;
            gBaudBits = CBR_115200 / gConfig.baudRate[gDirectConnect];
            ModemSetup();
            switch (gameMode) {
                case REMOTE_GAME_MODEM_HOST:
                    if (!gDirectConnect && Dial()) {
                        RemoteCleanup();
                        GameMode = REMOTE_GAME_NONE;
                    }
                    break;
                case REMOTE_GAME_MODEM_GUEST:
                    if (!gDirectConnect && Wait()) {
                        RemoteCleanup();
                        GameMode = REMOTE_GAME_NONE;
                    }
                    break;
                default:
                    return;
            }
            if (gDirectConnect) {
                WFDCStage = 0;
                giWaitType = DIALOG_WAIT_DIRECT_CONNECT;
                strcpy(
                    directConnectMessage,
                    "Waiting for other computer to log in to direct connection.\n\nPress 'CANCEL' "
                    "to abort."
                );
                NormalDialog(
                    directConnectMessage,
                    NORMAL_DIALOG_TYPE_WAIT_CANCEL,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                if (!gbFunctionComplete)
                    ShutDown(NULL);
            } else {
                Connect();
            }
            break;
    }
    gRemoteOn = 1;
    giNumHumanPlayers = gNumNetGuests + 1;
    gIDCtr = (gNetNameIndex * 400 + giThisNetPos + 1) * 100000000;
    gInNetSetup = 0;
}

VA(0x00451d50, 0x33)
void UnloadRemoteDriver(H1_ENUM_PARAM(RemoteDriverType, i16) networkDriver) {
    switch (networkDriver) {
        case REMOTE_DRIVER_SERIAL:
            com_term(0);
            break;
        case REMOTE_DRIVER_NETBIOS:
            nb_term(0);
            break;
    }
}

// CRC-16/CCITT over the packet bytes, most significant bit first.
H1_ENUM_CONST_BEGIN(RemoteCrcConstant)
    REMOTE_CRC_BYTE_TOP_BIT = 0x80,
    REMOTE_CRC_TOP_BIT = 0x8000,
    REMOTE_CRC_POLYNOMIAL = 0x1021
H1_ENUM_CONST_END(RemoteCrcConstant)

VA(0x00470f71, 0xb6)
void calc_crc(u16* crc, u8* data, i32 length) {
    i32 unused = 0;
    i16 carry;
    i16 mask;
    while (length--) {
        for (mask = REMOTE_CRC_BYTE_TOP_BIT; mask; mask >>= 1) {
            carry = *crc & REMOTE_CRC_TOP_BIT;
            *crc <<= 1;
            if (*data & mask)
                *crc |= 1;
            else
                ;
            if (carry)
                *crc ^= REMOTE_CRC_POLYNOMIAL;
        }
        data++;
    }
}

VA(0x00451e2e, 0x7b)
i32 EncodePacket(u8* data, i8 source, i8 destination, i32 length) {
    u16 crc;

    REMOTE_PACKET(PacketSend)->source = source;
    REMOTE_PACKET(PacketSend)->destination = destination;
    REMOTE_PACKET(PacketSend)->sequence = gPacketSequence;
    REMOTE_PACKET(PacketSend)->payloadSize = length;
    crc = 0;
    REMOTE_PACKET(PacketSend)->crc = crc;
    memcpy(PacketSend + sizeof(RemotePacketHeader), data, length);
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&crc, reinterpret_cast<u8*>(PacketSend), length + sizeof(RemotePacketHeader));
    REMOTE_PACKET(PacketSend)->crc = crc;
    return length + sizeof(RemotePacketHeader);
}

// donor PoL RVA 0x000a3aa7; preferred Buka symbol ?DecodePacket@@YIHPAEH@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.496845;margin=0.356674;shape=0.447;size=0.760;calls=0.750;alternate=pol20:int DecodePacket(unsigned char *, int)@0x000a3aa7
VA(0x004710ad, 0x162)
i32 DecodePacket(u8* data, i32 source) {
    u16 computedCrc;
    u16 crc;
    i32 rv;
    u32 size;

    computedCrc = 0;
    if (REMOTE_PACKET(packet)->source != source && source != REMOTE_BROADCAST_PLAYER) {
        sprintf(gText, "I want packet from %d not %d\n", source, REMOTE_PACKET(packet)->source);
        LogStr(gText);
        return 0;
    }
    if (REMOTE_PACKET(packet)->destination != giThisNetPos
        && REMOTE_PACKET(packet)->destination != REMOTE_BROADCAST_PLAYER) {
        sprintf(gText, "not mine %d\n", REMOTE_PACKET(packet)->destination);
        LogStr(gText);
        return 0;
    }
    size = REMOTE_PACKET(packet)->payloadSize;
    crc = REMOTE_PACKET(packet)->crc;
    REMOTE_PACKET(packet)->crc = 0;
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&computedCrc, reinterpret_cast<u8*>(packet), size + sizeof(RemotePacketHeader));
    if (crc != computedCrc) {
        sprintf(
            gText,
            "CRC Check Failed on Packet %d  CRC 1 %d CRC 2 %d\n",
            gPacketSequence,
            crc,
            computedCrc
        );
        LogStr(gText);
        return 0;
    }
    memcpy(data, packet + sizeof(RemotePacketHeader), size);
    return 1;
}

// donor PoL RVA 0x000a3be1; preferred Buka symbol ?SendRemoteData@@YIHPAE0HH@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.468075;margin=0.614352;shape=0.312;size=0.933;calls=0.500;alternate=pol20:int SendRemoteData(unsigned char *, unsigned char *, int, int)@0x000a3be1
VA(0x0047120f, 0x141)
i32 SendRemoteData(u8* dataToSend, u8*, i32 destination, i32 length) {
    i32 len;
    i32 result;
    i32 tries;
    i32 sendStatus;
    u8 buf[REMOTE_MESSAGE_SIZE];

    result = 1;
    if (iMPBaseType == MULTIPLAYER_BASE_NETWORK) {
        if (GameMode == REMOTE_GAME_NETWORK_HOST)
            destination = gNetNameIndex + 1;
        else
            destination = 0;
    } else if (destination == REMOTE_BROADCAST_PLAYER) {
        destination = 1 - giThisNetPos;
    }
    len = EncodePacket(dataToSend, giThisNetPos, destination, length);
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            do {
                sendStatus = nb_snd(0, destination, len, PacketSend, 0);
                if (sendStatus) {
                    result = 0;
                    goto finished;
                }
            } while (sendStatus);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            WriteModemPacket(PacketSend, len);
            result = 1;
            break;
    }
finished:
    return result;
}

// donor PoL RVA 0x000a3d6f; preferred Buka symbol ?ReceiveRemoteData@@YIHPAE0H@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.404111;margin=0.668725;shape=0.214;size=0.850;calls=0.500;alternate=pol20:int ReceiveRemoteData(unsigned char *, unsigned char *, int)@0x000a3d6f
VA(0x0045206a, 0xcd)
i32 ReceiveRemoteData(u8*, u8* data, i32 decodeType) {
    i32 receiveResult;
    i32 result;

    result = 1;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            if (GameMode == REMOTE_GAME_NETWORK_HOST)
                decodeType = gNetNameIndex + 1;
            else
                decodeType = 0;
            receiveResult = nb_rcv(0, 0x100, packet);
            if (receiveResult == 0)
                return 0;
            result = DecodePacket(data, decodeType);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            receiveResult = ReadPacket();
            if (receiveResult == 0)
                return 0;
            result = DecodePacket(data, decodeType);
            break;
    }
    return result;
}

// donor PoL RVA 0x000132f0; preferred Buka symbol ?InitNetHost@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.405636;margin=0.349549;shape=0.179;size=0.703;calls=1.000;alternate=pol20:signed char InitNetHost(void)@0x000132f0
VA(0x00471444, 0x194)
i8 InitNetHost(void) {
    DATA(0x004a2d4c)
    static i8 gInitNetHostStatus = 0;
    i32 unused;
    i32 needName;

    switch (gInitNetHostStatus) {
        case 0:
            if (static_cast<i16>(nb_init(0)) == 1) {
                ShutDown("NETBIOS is not loaded.");
            } else {
                gInitNetHostStatus++;
                gRemoteOn = 1;
                giThisNetPos = 0;
            }
            break;
        case 1:
            needName = !(nb_stat(0, 0) & NETBIOS_SESSION_NAME_REGISTERED);
            if (needName)
                gInitNetHostStatus++;
            else
                return 1;
            break;
        case 2:
            gNetNameIndex = 0;
            sprintf(gText, "HHOST%d", gNetNameIndex);
            if (nb_sess(0, NETBIOS_SESSION_REGISTER, gText) == 0)
                gInitNetHostStatus++;
            else
                ShutDown("Network initialization failed");
            break;
        case 3:
            needName = nb_stat(0, 0);
            if (needName & NETBIOS_SESSION_NAME_REGISTERED) {
                return 1;
            } else if (needName & NETBIOS_SESSION_ERROR) {
                gNetNameIndex++;
                if (gNetNameIndex > REMOTE_NET_NAME_LAST)
                    ShutDown("Network initialization failed, all game slots used!");
            }
            break;
    }
    return 0;
}

// donor PoL RVA 0x00013445; preferred Buka symbol ?InitNetGuest@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.423322;margin=0.095753;shape=0.173;size=0.829;calls=0.833;alternate=pol20:signed char InitNetGuest(void)@0x00013445
VA(0x004522a4, 0x1d9)
i8 InitNetGuest(void) {
    i32 status;
    i32 unregistered;

    switch (gInitNetGuestStatus) {
        case 0:
            if (static_cast<i16>(nb_init(6)) == 1) {
                ShutDown("NETBIOS is not loaded.");
            } else {
                gRemoteOn = 1;
                giThisNetPos = 1;
                gInitNetGuestStatus++;
            }
            break;
        case 1:
            if (nb_stat(0, 6) & NETBIOS_SESSION_NAME_REGISTERED)
                gInitNetGuestStatus += 3;
            else
                gInitNetGuestStatus++;
            break;
        case 2:
            sprintf(gText, "HGUEST%d", giThisNetPos);
            if (nb_sess(0, NETBIOS_SESSION_REGISTER, gText) == 0)
                gInitNetGuestStatus++;
            else
                ShutDown("Network initialization failed");
            break;
        case 3:
            status = nb_stat(0, 6);
            unregistered = !(status & NETBIOS_SESSION_NAME_REGISTERED);
            if (unregistered) {
                if (status & NETBIOS_SESSION_ERROR) {
                    giThisNetPos++;
                    if (giThisNetPos > REMOTE_NET_NAME_LAST) {
                        sprintf(gText, "Network initialization failed, all game slots used!");
                        ShutDown(gText);
                    } else {
                        gInitNetGuestStatus--;
                    }
                }
            } else {
                gInitNetGuestStatus++;
            }
            break;
        case 4:
            if (nb_sess(0, NETBIOS_SESSION_RECEIVE_ANY, 0) != 0) {
                sprintf(gText, "Network initialization failed");
                ShutDown(gText);
            }
            return 1;
    }
    return 0;
}

VA(0x0045247d, 0x75)
i8 WaitForHost(void) {
    char buffer[80];
    i32 status;

    switch (gWaitForHostStatus) {
        case 0:
            status = nb_stat(0, 0) & NETBIOS_SESSION_ACTIVE;
            if (status != 0)
                gWaitForHostStatus++;
            break;
        case 1:
            if (nb_rcv(0, 3, buffer)) {
                gNumNetGuests = buffer[0];
                return 1;
            }
            break;
    }
    return 0;
}

// donor PoL RVA 0x0001364f; preferred Buka symbol ?WaitForGuest@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.465490;margin=0.416814;shape=0.321;size=0.696;calls=1.000;alternate=pol20:signed char WaitForGuest(void)@0x0001364f
VA(0x004524f2, 0xd6)
i8 WaitForGuest(void) {
    DATA(0x004cc820)
    static i8 gWaitForGuestStatus = 0;
    DATA(0x004cc824)
    static i32 gLastBroadcastTime = 0;
    char buffer[80];
    i32 status;

    switch (gWaitForGuestStatus) {
        case 0:
            status = nb_sess(0, NETBIOS_SESSION_LISTEN_ANY, 6);
            if (status == 0)
                gWaitForGuestStatus++;
            return 0;
        case 1:
            status = !(nb_stat(0, 6) & NETBIOS_SESSION_ACTIVE);
            if (status) {
                if (KBTickCount() > gLastBroadcastTime + 500) {
                    gLastBroadcastTime = KBTickCount();
                    nb_snd(0, 0, 0, NULL, 0);
                }
            } else {
                gNumNetGuests++;
                nb_sess(0, NETBIOS_SESSION_MOVE, 6, gNetNameIndex + 1, 1);
                return 1;
            }
    }
    return 0;
}

// Buka 2.1 Netbios nbnet_init; the host also sends the guest count.
VA(0x004525c8, 0x196)
i32 nbnet_init(void) {
    char buffer[80];
    i32 status;

    gNumNetGuests = 0;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
            giWaitType = DIALOG_WAIT_NETBIOS_INIT_HOST;
            sprintf(gText, "Initializing network.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_WAIT_CANCEL,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (!gbFunctionComplete)
                ShutDown(NULL);
            giWaitType = DIALOG_WAIT_NETBIOS_GUEST;
            sprintf(gText, "Waiting On Guest.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_WAIT_CANCEL,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (!gbFunctionComplete)
                ShutDown(NULL);
            buffer[0] = gNumNetGuests;
            while (nb_snd(0, gNetNameIndex + 1, 3, buffer, 0))
                PollSound();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            giWaitType = DIALOG_WAIT_NETBIOS_INIT_GUEST;
            sprintf(gText, "Initializing network.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_WAIT_CANCEL,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (!gbFunctionComplete)
                ShutDown(NULL);
            giWaitType = DIALOG_WAIT_NETBIOS_HOST;
            sprintf(gText, "Waiting On Host.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_WAIT_CANCEL,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (!gbFunctionComplete)
                ShutDown(NULL);
            break;
    }
    return 0;
}

// Buka 2.1 ModemSetup reset loop: open the port and reset a dial-up modem.
VA(0x00471b30, 0xf7)
void ModemSetup(void) {
    char command[104];
    i32 resetAttempt;
    i32 i;

    com_init(gConfig.comPort[gDirectConnect], COM_BAUD_19200, 0);
    if (!gDirectConnect) {
        for (resetAttempt = 0; resetAttempt < 2; resetAttempt++) {
            if (gConfig.comPort[gDirectConnect] >= 1)
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
VA(0x0045283e, 0x95)
i32 Dial(void) {
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
VA(0x004528d3, 0x4b)
i32 Wait(void) {
    GUIModemResponse("Waiting for ring...", "RING");
    GUIModemCommand("Initializing modem...", "ATA");
    if (GUIModemResponse("Establishing connection...", "CONNECT"))
        return 1;
    return 0;
}

// donor PoL RVA 0x0000cc30; preferred Buka symbol ?GUIModemCommand@@YIXPAD0@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.504929;margin=0.542655;shape=0.294;size=0.956;calls=1.000;alternate=pol20:void GUIModemCommand(char *, char *)@0x0000cc30
VA(0x0045291e, 0x62)
void GUIModemCommand(char* message, char* command) {
    iLastActionTime = 0;
    iModemCommandPos = 0;
    giWaitType = DIALOG_WAIT_MODEM_COMMAND;
    strcpy(cModemCommand, command);
    NormalDialog(
        message,
        NORMAL_DIALOG_TYPE_WAIT_CANCEL,
        -1,
        -1,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_OR_TEXT
    );
    if (!gbFunctionComplete)
        ShutDown(NULL);
}

// donor PoL RVA 0x0000cca9; preferred Buka symbol ?GUIModemCommandExec@@YICXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.478276;margin=0.078716;shape=0.283;size=0.851;calls=1.000;alternate=pol20:signed char GUIModemCommandExec(void)@0x0000cca9
VA(0x00452980, 0x7f)
i8 GUIModemCommandExec(void) {
    i32 commandLength;
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
VA(0x00471e2e, 0x6c)
void ModemCommand(char* command) {
    i32 pos;
    i32 len = strlen(command);
    for (pos = 0; pos < len; ++pos) {
        write_buffer(command + pos, 1);
        DelayMilli(100);
    }
    write_buffer("\r", 1);
}

// donor PoL RVA 0x0000cdcc; preferred Buka symbol ?GUIModemResponse@@YICPAD0@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.487980;margin=0.528115;shape=0.250;size=0.959;calls=1.000;alternate=pol20:signed char GUIModemResponse(char *, char *)@0x0000cdcc
VA(0x00452a5e, 0x6b)
i8 GUIModemResponse(char* message, char* response) {
    memset(GUIMRresponse, 0, 80);
    GUIMRrespptr = 0;
    strcpy(GUIMRresp, response);
    giWaitType = DIALOG_WAIT_MODEM_RESPONSE;
    NormalDialog(
        message,
        NORMAL_DIALOG_TYPE_WAIT_CANCEL,
        -1,
        -1,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_OR_TEXT
    );
    if (!gbFunctionComplete)
        ShutDown(NULL);
    return 0;
}

// donor PoL RVA 0x0000ce4e; preferred Buka symbol ?GUIModemResponseExec@@YICXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.623720;margin=0.638042;shape=0.611;size=0.792;calls=1.000;alternate=pol20:signed char GUIModemResponseExec(void)@0x0000ce4e
VA(0x00452ac9, 0xb3)
i8 GUIModemResponseExec(void) {
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
VA(0x00471ff6, 0x2b)
i32 write_buffer(char* buffer, i32 length) {
    com_snd(0, 0, length, buffer, 0);
    return 1;
}

VA(0x00472021, 0x47)
i32 read_byte(void) {
    u8 ch;
    i32 received = com_rcv(0, 1, &ch);
    if (received == 1)
        return ch;
    else
        return -1;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00472068, 0x24)
void write_byte(i32 value) {
    com_snd(0, 0, 1, &value, 0);
}

// donor PoL RVA 0x0000cfec; preferred Buka symbol ?Connect@@YIXXZ
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.591174;margin=0.244003;shape=0.392;size=0.585;calls=0.933;strings=ID%s_%i;alternate=pol20:void Connect(void)@0x0000cfec
VA(0x0047208c, 0x2c0)
void Connect(void) {
    i32 result;
    char msg[20];
    u32 seed = KBTickCount();
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
VA(0x0047234c, 0x316)
i32 WaitForDirectConnect(void) {
    char idMessage[20];
    u32 seed;
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
VA(0x00472662, 0x109)
char ReadPacket(void) {
    i32 input;
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
            if (input == MODEM_PACKET_END) {
                newpacket = 1;
                return 1;
            } else if (input == MODEM_PACKET_START) {
                newpacket = 1;
                goto readPacketStart;
            }
        } else if (input == MODEM_PACKET_ESCAPE) {
            inescape = 1;
            goto readNextByte;
        }
        if (packetlen >= MODEM_PACKET_MAX_LENGTH)
            goto readPacketStart;
        packet[packetlen] = static_cast<char>(input);
        ++packetlen;
    } while (1);
}

// donor PoL RVA 0x0000d4df; preferred Buka symbol ?WriteModemPacket@@YIXPADH@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.395642;margin=0.361596;shape=0.175;size=0.824;calls=0.667;alternate=pol20:void WriteModemPacket(char *, int)@0x0000d4df
VA(0x0047276b, 0xdc)
void WriteModemPacket(char* buffer, i32 length) {
    char buf[544];
    i32 pos = 0;
    if (length > MODEM_PACKET_MAX_LENGTH)
        return;

    buf[pos] = MODEM_PACKET_ESCAPE;
    ++pos;
    buf[pos] = MODEM_PACKET_START;
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
    buf[pos] = MODEM_PACKET_END;
    ++pos;
    while (write_buffer(buf, pos) == 0)
        ForcePollSound();
}

// donor PoL RVA 0x000a3ec7; preferred Buka symbol ?TransmitRemoteData@@YIHPADHHCCCC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.560856;margin=1.192457;shape=0.450;size=0.831;calls=1.000;alternate=pol20:int TransmitRemoteData(char *, int, int, signed char, signed char, signed char, signed char)@0x000a3ec7
VA(0x00472847, 0x228)
// HoMM1 callers pass an eighth flag that maps a game position to its net position.
i32 TransmitRemoteData(
    char* data,
    i32 destination,
    i32 length,
    i8 command,
    i8 reliable,
    i8 allowRetryDialog,
    i8 messageType,
    i8 gamePosDestination
) {
    i32 k;
    i32 retval;
    i32 j;
    RemoteMessage msg;
    i32 tries;

    if (!gRemoteOn || gInNetSetup)
        return 1;
    if (gamePosDestination && destination != REMOTE_BROADCAST_PLAYER)
        destination = gbGamePosToNetPos[destination];
    retval = 0;
    tries = 0;
    gIDCtr++;
    msg.sender = giThisNetPos;
    msg.id = gIDCtr;
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
            reinterpret_cast<u8*>(&msg), // API-forced: SendRemoteData takes wire bytes.
            NULL,
            destination,
            length + REMOTE_MESSAGE_HEADER_SIZE
        );
        if (!reliable && retval) {
            return 1;
        } else if (retval) {
            k = 0;
            while (k < REMOTE_CONFIRM_POLL_COUNT) {
                ForcePollSound();
                if (gLastConfirm == gIDCtr)
                    return 1;
                retval = 0;
                DelayMilli(10);
                k++;
            }
        } else {
            DelayMilli(1000);
        }
        if (allowRetryDialog && tries == REMOTE_RETRY_COUNT && retval == 0) {
            NormalDialog(
                "Error sending data.  Keep trying??",
                NORMAL_DIALOG_TYPE_YES_NO,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
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
VA(0x00472a6f, 0x10c)
char* GetRemoteData(i8 remove) {
    i32 oldest;
    i32 i;
    i32 index;

    if (!gRemoteOn || gInNetSetup)
        return NULL;
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
    return NULL;
}

// donor PoL RVA 0x000a41ec; preferred Buka symbol ?PollRemote@@YIXXZ
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void PollRemote(void)@0x000a41ec

VA(0x00472b7b, 0x4fe)
void PollRemote(void) {
    DATA(0x004a3078)
    static i8 gInTimeoutFail = 0;
    i8 newControl;
    i8 queueFull;
    i32 i;
    i32 numQueued;
    i32 result;

    if (!gRemoteOn)
        return;
    if (iMPBaseType == MULTIPLAYER_BASE_MODEM)
        comm_wrt_task();
    else if (iMPBaseType == MULTIPLAYER_BASE_NETWORK)
        nb_thr_ctl();
    if (gInNetSetup)
        return;
    numQueued = 0;
    queueFull = 0;
    if (KBTickCount() - gLastHeartbeatSend > 5000) {
        sndBuf.sender = giThisNetPos;
        sndBuf.type = REMOTE_MESSAGE_HEARTBEAT;
        sndBuf.payloadSize = 1;
        sndBuf.command = (giCurPlayer << 4) + gCurHourGlassPhase;
        sndBuf.payload.data[0] = 1;
        SendRemoteData(
            reinterpret_cast<u8*>(&sndBuf),
            NULL,
            1 - giThisNetPos,
            REMOTE_MESSAGE_HEADER_SIZE + 1
        ); // API-forced: wire bytes.
        gLastHeartbeatSend = KBTickCount();
    }
    if (KBTickCount() > gLastHeartbeatReceive + 60000 && !gInTimeoutFail) {
        NormalDialog(
            "The other player's computer is not responding.  Do you wish to keep waiting for a "
            "response?",
            NORMAL_DIALOG_TYPE_YES_NO,
            -1,
            -1,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            gLastHeartbeatReceive = KBTickCount();
        } else {
            gInTimeoutFail = 1;
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
        result = ReceiveRemoteData(
            NULL,
            reinterpret_cast<u8*>(&rcvBufIn),
            REMOTE_BROADCAST_PLAYER
        ); // API-forced: wire bytes.
        if (result && rcvBufIn.sender != giThisNetPos) {
            if (rcvBufIn.type == REMOTE_MESSAGE_CONFIRM) {
                gLastConfirm = rcvBufIn.id;
                goto done;
            } else if (rcvBufIn.type == REMOTE_MESSAGE_HEARTBEAT) {
                if (rcvBufIn.payloadSize == 1 && rcvBufIn.payload.data[0] == 1)
                    gRemoteReady = 1;
                gLastHeartbeatReceive = KBTickCount();
                gHeartbeatSeen = 1;
                if (giHostGamePos != giThisGamePos && giCurPlayer != giThisGamePos
                    && gpAdvManager->m_active == 1 && rcvBufIn.command / 16 != giThisGamePos) {
                    giCurPlayer = rcvBufIn.command / 16;
                    gCurHourGlassPhase = rcvBufIn.command - giCurPlayer * 16;
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
                SendRemoteData(
                    reinterpret_cast<u8*>(&sndBuf),
                    NULL,
                    rcvBufIn.sender,
                    REMOTE_MESSAGE_HEADER_SIZE
                ); // API-forced: wire bytes.
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
                    iInOrder[i] = gInOrderCtr++;
                    memcpy(&rcvBuf[i], &rcvBufIn, REMOTE_MESSAGE_SIZE);
                    numQueued++;
                    iLastIds[gCurLastID] = rcvBufIn.id;
                    gCurLastID = (gCurLastID + 1) % REMOTE_RECENT_ID_COUNT;
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
VA(0x00473079, 0x14f)
i32 TransmitAndWait(
    char* bytes,
    i32 destination,
    i32 length,
    i8 command,
    i8 responseCommand,
    char** response
) {
    i32 start;
    i32 result;
    RemoteMessage* received;
    i8 complete;

    if (!gRemoteOn || gInNetSetup)
        return 1;
    received = NULL;
    result =
        TransmitRemoteData(bytes, destination, length, command, 1, 1, REMOTE_MESSAGE_DEFAULT, 1);
    if (result == 0)
        goto transmitComplete;
    start = KBTickCount();
    complete = 0;
    while (!complete) {
        if (KBTickCount() > start + 20000) {
            NormalDialog(
                "Error sending data.  Keep trying??",
                NORMAL_DIALOG_TYPE_YES_NO,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                start = KBTickCount();
            } else {
                result = 0;
                goto transmitComplete;
            }
        }
        ForcePollSound();
        received = reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
        if (received && received->type == REMOTE_MESSAGE_RELIABLE
            && received->command == responseCommand)
            complete = 1;
    }
    *response = reinterpret_cast<char*>(received); // API-forced: char* record.
transmitComplete:
    return result;
}

// The object's .data and .bss, in retail address order.
DATA(0x0049f048)
i32 gNetNameIndex = -1;
DATA(0x004a2c04)
i32 gIDCtr = 0;
DATA(0x004a2c1c)
i32 gBaudBits = 8;
DATA(0x004a2c20)
i32 packetlen = 0;
DATA(0x004a2c24)
i32 inescape = 0;
DATA(0x004a2c28)
i32 newpacket = 0;
DATA(0x004a2c2c)
i32 gInOrderCtr = 0;
DATA(0x004a2c30)
i32 gLastConfirm = -1;
DATA(0x004a2c34)
i32 gCurLastID = 0;
DATA(0x004cc814)
u8 GameMode = 0;
DATA(0x004cc815)
u8 gPacketSequence = 0;
DATA(0x004a2c40)
i32 gLastHeartbeatSend = 0;
DATA(0x004a2c44)
i32 gLastHeartbeatReceive = 1999999999;
DATA(0x004a2c48)
i8 gInNetSetup = 0;
DATA(0x004cc81e)
i8 gInitNetGuestStatus = 0;
DATA(0x004cc81f)
i8 gWaitForHostStatus = 0;
DATA(0x004cd858)
char idstr[8];
DATA(0x004cd758)
char rcvBufOut[REMOTE_MESSAGE_SIZE];
DATA(0x004cb40c)
i32 GUIMRc;
DATA(0x004cb3e0)
i32 iModemCommandPos;
DATA(0x004cb3d8)
i32 GUIMRrespptr;
DATA(0x004cb300)
i32 localstage;
DATA(0x004ca1a0)
char numbuf[40];
DATA(0x004cd6e0)
i32 iLastIds[REMOTE_RECENT_ID_COUNT];
DATA(0x004cc580)
i32 WFDCStage;
DATA(0x004cb310)
char remoteidstr[8];
DATA(0x004cb564)
char PacketSend[256];
DATA(0x004cb30c)
i32 stime;
DATA(0x004cc460)
i32 iInOrder[REMOTE_QUEUE_CAPACITY];
DATA(0x004cc480)
RemoteMessage sndBuf;
DATA(0x004ca264)
char cModemCommand[40];
DATA(0x004ca294)
i32 iLastDialPos;
DATA(0x004cc44c)
i32 remotestage;
DATA(0x004cb3e4)
i32 gNumNetGuests;
DATA(0x004cb2b0)
char GUIMRresp[40];
DATA(0x004cb304)
i32 oldsec;
DATA(0x004cb318)
inque_t inque;
DATA(0x004cb2d8)
char packet[256];
DATA(0x004cb3ec)
i32 iLastActionTime;
DATA(0x004cc6d8)
RemoteMessage rcvBufIn;
DATA(0x004cb514)
char GUIMRresponse[80];
DATA(0x004ccfe0)
RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
DATA(0x004cc7d8)
outque_t outque;
