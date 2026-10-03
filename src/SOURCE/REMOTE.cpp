// Remote play: the network and modem transports and their packet layer.
// Buka 2.1 REMOTE, Netbios and Modem correspondence. Retail compiled these as
// one object: RemoteCleanup starts it at 0x00458520 after SETUP's int3 fill,
// Dial (0x00459627) and WriteModemPacket (0x0045a16b) start at odd addresses
// directly after their predecessors, and the object's .data
// (0x0049f808-0x0049fdb7) and .bss (0x004c7e70-0x004ca487) interleave the
// network, modem and packet-layer variables.

#include <match.h>

#include <SOURCE/REMOTE.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/comwin.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/Modem.h>
#include <SOURCE/netwin.h>
#include <SOURCE/netwinRuntime.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/SETUP.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <string.h>

// Buka 2.1 RemoteCleanup without the HoMM2 logging and DirectPlay modes.
VA(0x00458520, 0x8d)
void RemoteCleanup(void) {
    if (!gbRemoteOn)
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
    gbRemoteOn = 0;
}

// @dead-code
// Zero-ref: reads one block from a file offset into the caller buffer.
VA(0x004585ad, 0x74)
void* ReadFileBlock(char* filename, void* buffer, int size, long offset) {
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
VA(0x00458621, 0x7b)
long FileSize(char* filename) {
    long length;
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
    int readPosition;
    int writePosition;
    char data[2048];
};

// Buka 2.1 RemoteMain merged with the HoMM2 ModemSetup mode switch; HoMM1
// keeps the modem reset sequence in ModemSetup (0x459530).
VA(0x0045869c, 0x27a)
void RemoteMain(int gameMode) {
    char directConnectMessage[164];

    gbInNetSetup = 1;
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
            gbRemoteOn = 1;
            giNumNetGuests = 1;
            inque.writePosition = 0;
            inque.readPosition = 0;
            outque.writePosition = 0;
            outque.readPosition = 0;
            iBaudBits = CBR_115200 / gConfig.baudRate[gbDirectConnect];
            ModemSetup();
            switch (gameMode) {
                case REMOTE_GAME_MODEM_HOST:
                    if (!gbDirectConnect && Dial()) {
                        RemoteCleanup();
                        GameMode = REMOTE_GAME_NONE;
                    }
                    break;
                case REMOTE_GAME_MODEM_GUEST:
                    if (!gbDirectConnect && Wait()) {
                        RemoteCleanup();
                        GameMode = REMOTE_GAME_NONE;
                    }
                    break;
                default:
                    return;
            }
            if (gbDirectConnect) {
                WFDCStage = 0;
                giWaitType = DIALOG_WAIT_DIRECT_CONNECT;
                strcpy(directConnectMessage,
                       "Waiting for other computer to log in to direct connection.\n\nPress 'CANCEL' to abort.");
                NormalDialog(directConnectMessage, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                if (!gbFunctionComplete)
                    ShutDown(NULL);
            } else {
                Connect();
            }
            break;
    }
    gbRemoteOn = 1;
    giNumHumanPlayers = giNumNetGuests + 1;
    iIDCtr = (iNetNameIndex * 400 + giThisNetPos + 1) * 100000000;
    gbInNetSetup = 0;
}

VA(0x00458916, 0x5b)
void UnloadRemoteDriver(H1_ENUM_PARAM(RemoteDriverType, short) networkDriver) {
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

VA(0x00458971, 0xb6)
void calc_crc(unsigned short* crc, unsigned char* data, int length) {
    int unused = 0;
    short carry;
    short mask;
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

VA(0x00458a27, 0x86)
int EncodePacket(unsigned char* data, char source, char destination, int length) {
    unsigned short crc;

    REMOTE_PACKET(PacketSend)->source = source;
    REMOTE_PACKET(PacketSend)->destination = destination;
    REMOTE_PACKET(PacketSend)->sequence = gPacketSequence;
    REMOTE_PACKET(PacketSend)->payloadSize = length;
    crc = 0;
    REMOTE_PACKET(PacketSend)->crc = crc;
    memcpy(PacketSend + sizeof(RemotePacketHeader), data, length);
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&crc, reinterpret_cast<unsigned char*>(PacketSend), length + sizeof(RemotePacketHeader));
    REMOTE_PACKET(PacketSend)->crc = crc;
    return length + sizeof(RemotePacketHeader);
}

// donor PoL RVA 0x000a3aa7; preferred Buka symbol ?DecodePacket@@YIHPAEH@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.496845;margin=0.356674;shape=0.447;size=0.760;calls=0.750;alternate=pol20:int DecodePacket(unsigned char *, int)@0x000a3aa7
VA(0x00458aad, 0x162)
int DecodePacket(unsigned char* data, int source) {
    unsigned short computedCrc;
    unsigned short crc;
    int rv;
    unsigned long size;

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
    calc_crc(&computedCrc, reinterpret_cast<unsigned char*>(packet), size + sizeof(RemotePacketHeader));
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
VA(0x00458c0f, 0x141)
int SendRemoteData(unsigned char* dataToSend, unsigned char*, int destination, int length) {
    int len;
    int result;
    int tries;
    int sendStatus;
    unsigned char buf[REMOTE_MESSAGE_SIZE];

    result = 1;
    if (iMPBaseType == MULTIPLAYER_BASE_NETWORK) {
        if (GameMode == REMOTE_GAME_NETWORK_HOST)
            destination = iNetNameIndex + 1;
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
VA(0x00458d50, 0xf4)
int ReceiveRemoteData(unsigned char*, unsigned char* data, int decodeType) {
    int receiveResult;
    int result;

    result = 1;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            if (GameMode == REMOTE_GAME_NETWORK_HOST)
                decodeType = iNetNameIndex + 1;
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
VA(0x00458e44, 0x194)
signed char InitNetHost(void) {
    DATA(0x0049f954) static signed char iInitNetHostStatus = 0;
    int unused;
    int needName;

    switch (iInitNetHostStatus) {
        case 0:
            if (static_cast<short>(nb_init(0)) == 1) {
                ShutDown("NETBIOS is not loaded.");
            } else {
                iInitNetHostStatus++;
                gbRemoteOn = 1;
                giThisNetPos = 0;
            }
            break;
        case 1:
            needName = !(nb_stat(0, 0) & NETBIOS_SESSION_NAME_REGISTERED);
            if (needName)
                iInitNetHostStatus++;
            else
                return 1;
            break;
        case 2:
            iNetNameIndex = 0;
            sprintf(gText, "HHOST%d", iNetNameIndex);
            if (nb_sess(0, NETBIOS_SESSION_REGISTER, gText) == 0)
                iInitNetHostStatus++;
            else
                ShutDown("Network initialization failed");
            break;
        case 3:
            needName = nb_stat(0, 0);
            if (needName & NETBIOS_SESSION_NAME_REGISTERED) {
                return 1;
            } else if (needName & NETBIOS_SESSION_ERROR) {
                iNetNameIndex++;
                if (iNetNameIndex > REMOTE_NET_NAME_LAST)
                    ShutDown("Network initialization failed, all game slots used!");
            }
            break;
    }
    return 0;
}

// donor PoL RVA 0x00013445; preferred Buka symbol ?InitNetGuest@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.423322;margin=0.095753;shape=0.173;size=0.829;calls=0.833;alternate=pol20:signed char InitNetGuest(void)@0x00013445
VA(0x00458fd8, 0x1f1)
signed char InitNetGuest(void) {
    int status;
    int unregistered;

    switch (iInitNetGuestStatus) {
        case 0:
            if (static_cast<short>(nb_init(6)) == 1) {
                ShutDown("NETBIOS is not loaded.");
            } else {
                gbRemoteOn = 1;
                giThisNetPos = 1;
                iInitNetGuestStatus++;
            }
            break;
        case 1:
            if (nb_stat(0, 6) & NETBIOS_SESSION_NAME_REGISTERED)
                iInitNetGuestStatus += 3;
            else
                iInitNetGuestStatus++;
            break;
        case 2:
            sprintf(gText, "HGUEST%d", giThisNetPos);
            if (nb_sess(0, NETBIOS_SESSION_REGISTER, gText) == 0)
                iInitNetGuestStatus++;
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
                        iInitNetGuestStatus--;
                    }
                }
            } else {
                iInitNetGuestStatus++;
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

VA(0x004591c9, 0x9e)
signed char WaitForHost(void) {
    char buffer[80];
    int status;

    switch (iWaitForHostStatus) {
        case 0:
            status = nb_stat(0, 0) & NETBIOS_SESSION_ACTIVE;
            if (status != 0)
                iWaitForHostStatus++;
            break;
        case 1:
            if (nb_rcv(0, 3, buffer)) {
                giNumNetGuests = buffer[0];
                return 1;
            }
            break;
    }
    return 0;
}

// donor PoL RVA 0x0001364f; preferred Buka symbol ?WaitForGuest@@YICXZ
// donor Buka TU SOURCE/Netbios; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.465490;margin=0.416814;shape=0.321;size=0.696;calls=1.000;alternate=pol20:signed char WaitForGuest(void)@0x0001364f
VA(0x00459267, 0x101)
signed char WaitForGuest(void) {
    DATA(0x0049fa6c) static signed char iWaitForGuestStatus = 0;
    DATA(0x0049fa70) static long iLastBroadcastTime = 0;
    char buffer[80];
    int status;

    switch (iWaitForGuestStatus) {
        case 0:
            status = nb_sess(0, NETBIOS_SESSION_LISTEN_ANY, 6);
            if (status == 0)
                iWaitForGuestStatus++;
            return 0;
        case 1:
            status = !(nb_stat(0, 6) & NETBIOS_SESSION_ACTIVE);
            if (status) {
                if (KBTickCount() > iLastBroadcastTime + 500) {
                    iLastBroadcastTime = KBTickCount();
                    nb_snd(0, 0, 0, NULL, 0);
                }
            } else {
                giNumNetGuests++;
                nb_sess(0, NETBIOS_SESSION_MOVE, 6, iNetNameIndex + 1, 1);
                return 1;
            }
    }
    return 0;
}

// Buka 2.1 Netbios nbnet_init; the host also sends the guest count.
VA(0x00459368, 0x1c8)
int nbnet_init(void) {
    char buffer[80];
    int status;

    giNumNetGuests = 0;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
            giWaitType = DIALOG_WAIT_NETBIOS_INIT_HOST;
            sprintf(gText, "Initializing network.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            giWaitType = DIALOG_WAIT_NETBIOS_GUEST;
            sprintf(gText, "Waiting On Guest.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            buffer[0] = giNumNetGuests;
            while (nb_snd(0, iNetNameIndex + 1, 3, buffer, 0))
                PollSound();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            giWaitType = DIALOG_WAIT_NETBIOS_INIT_GUEST;
            sprintf(gText, "Initializing network.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            giWaitType = DIALOG_WAIT_NETBIOS_HOST;
            sprintf(gText, "Waiting On Host.\n\n  Press 'CANCEL' to abort.");
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            break;
    }
    return 0;
}

// Buka 2.1 ModemSetup reset loop: open the port and reset a dial-up modem.
VA(0x00459530, 0xf7)
void ModemSetup(void) {
    char command[104];
    int resetAttempt;
    int i;

    com_init(gConfig.comPort[gbDirectConnect], COM_BAUD_19200, 0);
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
    giWaitType = DIALOG_WAIT_MODEM_COMMAND;
    strcpy(cModemCommand, command);
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    if (!gbFunctionComplete)
        ShutDown(NULL);
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
    giWaitType = DIALOG_WAIT_MODEM_RESPONSE;
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    if (!gbFunctionComplete)
        ShutDown(NULL);
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

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
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
VA(0x0045a16b, 0xdc)
void WriteModemPacket(char* buffer, int length) {
    char buf[544];
    int pos = 0;
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
            reinterpret_cast<unsigned char*>(&msg), // API-forced: SendRemoteData takes wire bytes.
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
            NormalDialog("Error sending data.  Keep trying??", NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
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

VA(0x0045a584, 0x4fe)
void PollRemote(void) {
    DATA(0x0049fc80) static signed char bInTimeoutFail = 0;
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
        SendRemoteData(reinterpret_cast<unsigned char*>(&sndBuf), NULL, 1 - giThisNetPos, REMOTE_MESSAGE_HEADER_SIZE + 1); // API-forced: wire bytes.
        lLastHeartbeatSend = KBTickCount();
    }
    if (KBTickCount() > lLastHeartbeatReceive + 60000 && !bInTimeoutFail) {
        NormalDialog(
            "The other player's computer is not responding.  Do you wish to keep waiting for a response?",
            NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
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
        result = ReceiveRemoteData(NULL, reinterpret_cast<unsigned char*>(&rcvBufIn), REMOTE_BROADCAST_PLAYER); // API-forced: wire bytes.
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
                SendRemoteData(reinterpret_cast<unsigned char*>(&sndBuf), NULL, rcvBufIn.sender, REMOTE_MESSAGE_HEADER_SIZE); // API-forced: wire bytes.
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
    received = NULL;
    result =
        TransmitRemoteData(bytes, destination, length, command, 1, 1, REMOTE_MESSAGE_DEFAULT, 1);
    if (result == 0)
        goto transmitComplete;
    start = KBTickCount();
    complete = 0;
    while (!complete) {
        if (KBTickCount() > start + 20000) {
            NormalDialog("Error sending data.  Keep trying??", NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
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
DATA(0x0049f808)
int iNetNameIndex = -1;
DATA(0x0049f80c)
int iIDCtr = 0;
DATA(0x0049f824)
int iBaudBits = 8;
DATA(0x0049f828)
int packetlen = 0;
DATA(0x0049f82c)
int inescape = 0;
DATA(0x0049f830)
int newpacket = 0;
DATA(0x0049f834)
int iInOrderCtr = 0;
DATA(0x0049f838)
int giLastConfirm = -1;
DATA(0x0049f83c)
int iCurLastID = 0;
DATA(0x0049f840)
unsigned char GameMode = 0;
DATA(0x0049f844)
unsigned char gPacketSequence = 0;
DATA(0x0049f848)
long lLastHeartbeatSend = 0;
DATA(0x0049f84c)
long lLastHeartbeatReceive = 1999999999;
DATA(0x0049f850)
signed char gbInNetSetup = 0;
DATA(0x0049f958)
signed char iInitNetGuestStatus = 0;
DATA(0x0049f9d0)
signed char iWaitForHostStatus = 0;
DATA(0x004c7e70)
char idstr[8];
DATA(0x004c7e78)
char rcvBufOut[REMOTE_MESSAGE_SIZE];
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
DATA(0x004c7fb0)
int iLastIds[REMOTE_RECENT_ID_COUNT];
DATA(0x004c8028)
int WFDCStage;
DATA(0x004c8030)
char remoteidstr[8];
DATA(0x004c8038)
char PacketSend[256];
DATA(0x004c8138)
int stime;
DATA(0x004c8140)
int iInOrder[REMOTE_QUEUE_CAPACITY];
DATA(0x004c8160)
RemoteMessage sndBuf;
DATA(0x004c8260)
char cModemCommand[40];
DATA(0x004c8288)
int iLastDialPos;
DATA(0x004c828c)
int remotestage;
DATA(0x004c8290)
int giNumNetGuests;
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
DATA(0x004c93e0)
RemoteMessage rcvBufIn;
DATA(0x004c94e0)
char GUIMRresponse[80];
DATA(0x004c9530)
RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
DATA(0x004c9c80)
outque_t outque;
