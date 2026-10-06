// Remote play: the network and modem transports and their packet layer.
// Retail compiled these as one object: RemoteCleanup starts it at 0x00458520
// after SETUP's int3 fill, Dial (0x00459627) and WriteModemPacket (0x0045a16b)
// start at odd addresses directly after their predecessors, and the object's
// .data (0x004a2c00-0x0049fdb7) and .bss (0x004c7e70-0x004ca487) interleave
// the network, modem and packet-layer variables.

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

VA(0x004519f0, 0x66)
void RemoteCleanup(void) {
    if (!gRemoteOn)
        return;
    switch (gRemoteGameMode) {
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
    gRemoteOn = false;
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

VA(0x00451abb, 0x74)
i32 FileSize(char* filename) {
    i32 length;
    FILE* stream;
    stream = fopen(filename, "r+b");
    if (stream == NULL) {
        if (stream == NULL)
            FileError(filename);
    }
    fseek(stream, 0, SEEK_END);
    length = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    fclose(stream);
    return length;
}

VA(0x00451b2f, 0x221)
void RemoteMain(H1_ENUM_PARAM(RemoteGameMode, i32) gameMode) {
    char directConnectMessage[164];

    gInNetSetup = true;
    memset(gReceiveQueue, 0, sizeof(gReceiveQueue));
    memset(gLastIds, 0, 30);
    gRemoteGameMode = gameMode;
    switch (gameMode) {
        case REMOTE_GAME_NETWORK_HOST:
            nbnet_init();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            nbnet_init();
            break;
        case REMOTE_GAME_MODEM_HOST:
            gThisNetPos = 0;
            goto modemStart;
        case REMOTE_GAME_MODEM_GUEST:
            gThisNetPos = 1;
        modemStart:
            gRemoteOn = true;
            gNumNetGuests = 1;
            gModemInQueue.writePosition = 0;
            gModemInQueue.readPosition = 0;
            gModemOutQueue.writePosition = 0;
            gModemOutQueue.readPosition = 0;
            gBaudBits =
                CBR_115200 / gConfig.baudRate[H1_ENUM_DECODE(ConfigConnection, gDirectConnect)];
            ModemSetup();
            switch (gameMode) {
                case REMOTE_GAME_MODEM_HOST:
                    if (!gDirectConnect && Dial()) {
                        RemoteCleanup();
                        gRemoteGameMode = REMOTE_GAME_NONE;
                    }
                    break;
                case REMOTE_GAME_MODEM_GUEST:
                    if (!gDirectConnect && Wait()) {
                        RemoteCleanup();
                        gRemoteGameMode = REMOTE_GAME_NONE;
                    }
                    break;
                default:
                    return;
            }
            if (gDirectConnect) {
                gDirectConnectStage = 0;
                gWaitType = DIALOG_WAIT_DIRECT_CONNECT;
                strcpy(directConnectMessage, localization::Tr("network.direct.wait"));
                NormalDialog(directConnectMessage, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
                if (!gFunctionComplete)
                    ShutDown(NULL);
            } else {
                Connect();
            }
            break;
    }
    gRemoteOn = true;
    gNumHumanPlayers = gNumNetGuests + 1;
    gIDCtr = (gThisNetPos + gNetNameIndex * 400 + 1) * 100000000;
    gInNetSetup = false;
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

VA(0x00451d83, 0xab)
void calc_crc(u16* crc, u8* data, i32 length) {
    i32 unused = 0;
    i16 overflow;
    i16 mask;
    while (length--) {
        for (mask = REMOTE_CRC_BYTE_TOP_BIT; mask; mask >>= 1) {
            overflow = *crc & REMOTE_CRC_TOP_BIT;
            *crc <<= 1;
            *crc |= (mask & *data) != 0;
            if (overflow)
                *crc ^= REMOTE_CRC_POLYNOMIAL;
        }
        data++;
    }
}

VA(0x00451e2e, 0x7b)
i32 EncodePacket(RemoteMessage* data, i8 source, i8 destination, i32 length) {
    u16 crc;

    REMOTE_PACKET(gPacketSend)->source = source;
    REMOTE_PACKET(gPacketSend)->destination = destination;
    REMOTE_PACKET(gPacketSend)->sequence = gPacketSequence;
    REMOTE_PACKET(gPacketSend)->payloadSize = length;
    crc = 0;
    REMOTE_PACKET(gPacketSend)->crc = crc;
    memcpy(gPacketSend + sizeof(RemotePacketHeader), data, length);
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&crc, reinterpret_cast<u8*>(gPacketSend), length + sizeof(RemotePacketHeader));
    REMOTE_PACKET(gPacketSend)->crc = crc;
    return length + sizeof(RemotePacketHeader);
}

#define dataSize theSize // frame-slot spelling
VA(0x00451ea9, 0xb2)
b32 DecodePacket(RemoteMessage* data, i32 source) {
    u16 computedCrc;
    u16 crc;
    i32 i;
    u32 dataSize;

    computedCrc = 0;
    if (REMOTE_PACKET(gPacket)->source != source && source != REMOTE_BROADCAST_PLAYER) {
        return false;
    }
    if (REMOTE_PACKET(gPacket)->destination != gThisNetPos
        && REMOTE_PACKET(gPacket)->destination != REMOTE_BROADCAST_PLAYER) {
        return false;
    }
    dataSize = REMOTE_PACKET(gPacket)->payloadSize;
    crc = REMOTE_PACKET(gPacket)->crc;
    REMOTE_PACKET(gPacket)->crc = 0;
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&computedCrc, reinterpret_cast<u8*>(gPacket), dataSize + sizeof(RemotePacketHeader));
    if (crc != computedCrc) {
        return false;
    }
    memcpy(data, gPacket + sizeof(RemotePacketHeader), dataSize);
    return true;
}
#undef dataSize

VA(0x00451f5b, 0x10f)
b32 SendRemoteData(RemoteMessage* dataToSend, u8*, i32 destination, i32 length) {
    i32 size;
    b32 out;
    i32 retry;
    i32 sendStatus;
    u8 remotePacket[REMOTE_MESSAGE_SIZE];

    out = true;
    if (gMapBaseType == MULTIPLAYER_BASE_NETWORK) {
        if (gRemoteGameMode == REMOTE_GAME_NETWORK_HOST)
            destination = gNetNameIndex + 1;
        else
            destination = 0;
    } else if (destination == REMOTE_BROADCAST_PLAYER) {
        destination = 1 - gThisNetPos;
    }
    size = EncodePacket(dataToSend, gThisNetPos, destination, length);
    switch (gRemoteGameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            do {
                sendStatus = nb_snd(0, destination, size, gPacketSend, 0);
                if (sendStatus) {
                    out = false;
                    goto finished;
                }
            } while (sendStatus);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            WriteModemPacket(gPacketSend, size);
            out = true;
            break;
    }
finished:
    return out;
}

VA(0x0045206a, 0xcd)
b32 ReceiveRemoteData(u8*, RemoteMessage* data, i32 source) {
    i32 receiveResult;
    b32 result;

    result = true;
    switch (gRemoteGameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            if (gRemoteGameMode == REMOTE_GAME_NETWORK_HOST)
                source = gNetNameIndex + 1;
            else
                source = 0;
            receiveResult = nb_rcv(0, 0x100, gPacket);
            if (receiveResult == 0)
                return false;
            result = DecodePacket(data, source);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            receiveResult = ReadPacket();
            if (receiveResult == 0)
                return false;
            result = DecodePacket(data, source);
            break;
    }
    return result;
}

DATA(0x004cc7f0)
i32 gIDCtr = 0;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004cc7f4)
i32 gUnusedRemoteWords[3] = {0, 0, 0};
DATA(0x004cc800)
i32 gModemPacketLength = 0;
DATA(0x004cc804)
b32 gModemInEscape = false;
DATA(0x004cc808)
b32 gModemNewPacket = false;
DATA(0x004cc80c)
i32 gInOrderCtr = 0;
DATA(0x004cc810)
i32 gCurLastID = 0;
DATA(0x004cc814)
H1_ENUM_STORAGE(RemoteGameMode, u8) gRemoteGameMode = REMOTE_GAME_NONE;
DATA(0x004cc815)
u8 gPacketSequence = 0;
DATA(0x004cc818)
i32 gLastHeartbeatSend = 0;
DATA(0x004cc81c)
b8 gInNetSetup = false;

VA(0x00452137, 0x16d)
b8 InitNetHost(void) {
    DATA(0x004cc81d)
    static i8 gInitNetHostStatus = 0;
    i32 reserved;
    i32 needName;

    switch (gInitNetHostStatus) {
        case NET_HOST_INIT_START:
            if (static_cast<i16>(nb_init(0)) == 1) {
                ShutDown(localization::Tr("network.netbios.missing"));
            } else {
                gInitNetHostStatus++;
                gRemoteOn = true;
                gThisNetPos = 0;
            }
            break;
        case NET_HOST_INIT_CHECK_NAME:
            needName = !(nb_stat(0, 0) & NETBIOS_SESSION_NAME_REGISTERED);
            if (needName)
                gInitNetHostStatus++;
            else
                return true;
            break;
        case NET_HOST_INIT_REGISTER_NAME:
            gNetNameIndex = 0;
            sprintf(gText, "HHOST%d", gNetNameIndex);
            if (nb_sess(0, NETBIOS_SESSION_REGISTER, gText) == 0)
                gInitNetHostStatus++;
            else
                ShutDown(localization::Tr("network.initialize.failed"));
            break;
        case NET_HOST_INIT_WAIT_NAME:
            needName = nb_stat(0, 0);
            if (needName & NETBIOS_SESSION_NAME_REGISTERED) {
                return true;
            } else if (needName & NETBIOS_SESSION_ERROR) {
                gNetNameIndex++;
                if (gNetNameIndex > REMOTE_NET_NAME_LAST)
                    ShutDown(localization::Tr("network.initialize.slots_full"));
            }
            break;
    }
    return false;
}

VA(0x004522a4, 0x1d9)
b8 InitNetGuest(void) {
    i32 status;
    b32 unregistered;

    switch (gInitNetGuestStatus) {
        case NET_GUEST_INIT_START:
            if (static_cast<i16>(nb_init(6)) == 1) {
                ShutDown(localization::Tr("network.netbios.missing"));
            } else {
                gRemoteOn = true;
                gThisNetPos = 1;
                gInitNetGuestStatus++;
            }
            break;
        case NET_GUEST_INIT_CHECK_NAME:
            if (nb_stat(0, 6) & NETBIOS_SESSION_NAME_REGISTERED)
                gInitNetGuestStatus += 3;
            else
                gInitNetGuestStatus++;
            break;
        case NET_GUEST_INIT_REGISTER_NAME:
            sprintf(gText, "HGUEST%d", gThisNetPos);
            if (nb_sess(0, NETBIOS_SESSION_REGISTER, gText) == 0)
                gInitNetGuestStatus++;
            else
                ShutDown(localization::Tr("network.initialize.failed"));
            break;
        case NET_GUEST_INIT_WAIT_NAME:
            status = nb_stat(0, 6);
            unregistered = !(status & NETBIOS_SESSION_NAME_REGISTERED);
            if (unregistered) {
                if (status & NETBIOS_SESSION_ERROR) {
                    gThisNetPos++;
                    if (gThisNetPos > REMOTE_NET_NAME_LAST) {
                        sprintf(gText, localization::Tr("network.initialize.slots_full"));
                        ShutDown(gText);
                    } else {
                        gInitNetGuestStatus--;
                    }
                }
            } else {
                gInitNetGuestStatus++;
            }
            break;
        case NET_GUEST_INIT_RECEIVE:
            if (nb_sess(0, NETBIOS_SESSION_RECEIVE_ANY, 0) != 0) {
                sprintf(gText, localization::Tr("network.initialize.failed"));
                ShutDown(gText);
            }
            return true;
    }
    return false;
}

DATA(0x004cc81e)
i8 gInitNetGuestStatus = 0;
DATA(0x004cc81f)
i8 gWaitForHostStatus = 0;

VA(0x0045247d, 0x75)
b8 WaitForHost(void) {
    char buffer[80];
    i32 status;

    switch (gWaitForHostStatus) {
        case NET_WAIT_SESSION:
            status = nb_stat(0, 0) & NETBIOS_SESSION_ACTIVE;
            if (status != 0)
                gWaitForHostStatus++;
            break;
        case NET_WAIT_CONNECTED:
            if (nb_rcv(0, 3, buffer)) {
                gNumNetGuests = buffer[0];
                return true;
            }
            break;
    }
    return false;
}

VA(0x004524f2, 0xd6)
b8 WaitForGuest(void) {
    DATA(0x004cc820)
    static i8 gWaitForGuestStatus = 0;
    DATA(0x004cc824)
    static i32 gLastBroadcastTime = 0;
    char buffer[80];
    i32 status;

    switch (gWaitForGuestStatus) {
        case NET_WAIT_SESSION:
            status = nb_sess(0, NETBIOS_SESSION_LISTEN_ANY, 6);
            if (status == 0)
                gWaitForGuestStatus++;
            return false;
        case NET_WAIT_CONNECTED:
            status = !(nb_stat(0, 6) & NETBIOS_SESSION_ACTIVE);
            if (status) {
                if (KBTickCount() > gLastBroadcastTime + 500) {
                    gLastBroadcastTime = KBTickCount();
                    nb_snd(0, 0, 0, NULL, 0);
                }
            } else {
                gNumNetGuests++;
                nb_sess(0, NETBIOS_SESSION_MOVE, 6, gNetNameIndex + 1, 1);
                return true;
            }
    }
    return false;
}

// The host also sends the guest count.
VA(0x004525c8, 0x196)
i32 nbnet_init(void) {
    char buffer[80];
    i32 status;

    gNumNetGuests = 0;
    switch (gRemoteGameMode) {
        case REMOTE_GAME_NETWORK_HOST:
            gWaitType = DIALOG_WAIT_NETBIOS_INIT_HOST;
            sprintf(gText, localization::Tr("network.initialize.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gFunctionComplete)
                ShutDown(NULL);
            gWaitType = DIALOG_WAIT_NETBIOS_GUEST;
            sprintf(gText, localization::Tr("network.guest.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gFunctionComplete)
                ShutDown(NULL);
            buffer[0] = gNumNetGuests;
            while (nb_snd(0, gNetNameIndex + 1, 3, buffer, 0))
                PollSound();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            gWaitType = DIALOG_WAIT_NETBIOS_INIT_GUEST;
            sprintf(gText, localization::Tr("network.initialize.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gFunctionComplete)
                ShutDown(NULL);
            gWaitType = DIALOG_WAIT_NETBIOS_HOST;
            sprintf(gText, localization::Tr("network.host.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gFunctionComplete)
                ShutDown(NULL);
            break;
    }
    return 0;
}

// Open the port and reset a dial-up modem.
VA(0x0045275e, 0xe0)
void ModemSetup(void) {
    char command[104];
    i32 resetAttempt;
    i32 i;

    com_init(gConfig.comPort[H1_ENUM_DECODE(ConfigConnection, gDirectConnect)], COM_BAUD_19200, 0);
    if (!gDirectConnect) {
        for (resetAttempt = 0; resetAttempt < 2; resetAttempt++) {
            if (gConfig.comPort[H1_ENUM_DECODE(ConfigConnection, gDirectConnect)] >= 1)
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

VA(0x0045283e, 0x95)
i32 Dial(void) {
    char dialCommand[40];
    gLastDialPos = 0;
    sprintf(dialCommand, "ATDT%s", gPhoneNumber);
    sprintf(gText, "%s %s", localization::Tr("modem.dialing"), gPhoneNumber);
    GUIModemCommand(gText, dialCommand);
    sprintf(gText, "%s %s", localization::Tr("modem.dialing"), gPhoneNumber);
    if (GUIModemResponse(gText, "CONNECT"))
        return 1;
    return 0;
}

VA(0x004528d3, 0x4b)
i32 Wait(void) {
    GUIModemResponse(localization::Tr("modem.ring.wait"), "RING");
    GUIModemCommand(localization::Tr("modem.initializing"), "ATA");
    if (GUIModemResponse(localization::Tr("modem.connecting"), "CONNECT"))
        return 1;
    return 0;
}

VA(0x0045291e, 0x62)
void GUIModemCommand(char* message, char* command) {
    gLastActionTime = 0;
    gModemCommandPos = 0;
    gWaitType = DIALOG_WAIT_MODEM_COMMAND;
    strcpy(gModemCommand, command);
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
    if (!gFunctionComplete)
        ShutDown(NULL);
}

VA(0x00452980, 0x7f)
b8 GUIModemCommandExec(void) {
    i32 commandLength;
    if (KBTickCount() < gLastActionTime + 250)
        return false;

    gLastActionTime = KBTickCount();
    commandLength = strlen(gModemCommand);
    if (gModemCommandPos < commandLength) {
        write_buffer(gModemCommand + gModemCommandPos, 1);
        ++gModemCommandPos;
        return false;
    } else {
        write_buffer("\r", 1);
        return true;
    }
}

// Writes one command byte at a time.
VA(0x004529ff, 0x5f)
void ModemCommand(char* command) {
    i32 curPos;
    i32 len = strlen(command);
    for (curPos = 0; curPos < len; ++curPos) {
        write_buffer(command + curPos, 1);
        DelayMilli(100);
    }
    write_buffer("\r", 1);
}

VA(0x00452a5e, 0x6b)
i8 GUIModemResponse(char* message, char* response) {
    memset(gModemResponseLine, 0, 80);
    gModemResponseLength = 0;
    strcpy(gModemExpectedResponse, response);
    gWaitType = DIALOG_WAIT_MODEM_RESPONSE;
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
    if (!gFunctionComplete)
        ShutDown(NULL);
    return 0;
}

VA(0x00452ac9, 0xb3)
b8 GUIModemResponseExec(void) {
    gModemResponseChar = read_byte();
    if (gModemResponseChar == -1)
        return false;
    if (gModemResponseChar == '\n' || gModemResponseLength == MODEM_RESPONSE_LAST) {
        gModemResponseLine[gModemResponseLength] = 0;
        if (gModemResponseLength > 17)
            gModemResponseLine[17] = 0;
        goto compareResponse;
    }
    if (gModemResponseChar >= ' ') {
        gModemResponseLine[gModemResponseLength] = gModemResponseChar;
        ++gModemResponseLength;
    }
    return false;
compareResponse:
    if (strncmp(gModemResponseLine, gModemExpectedResponse, strlen(gModemExpectedResponse)) != 0) {
        gModemResponseLength = 0;
        return false;
    } else {
        return true;
    }
}

// Serial queue helpers.
VA(0x00452b7c, 0x21)
i32 write_buffer(char* buffer, i32 length) {
    com_snd(0, 0, length, buffer, 0);
    return 1;
}

VA(0x00452b9d, 0x33)
i32 read_byte(void) {
    u8 value;
    i32 received = com_rcv(0, 1, &value);
    if (received == 1)
        return value;
    else
        return -1;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00452bd0, 0x19)
void write_byte(i32 value) {
    com_snd(0, 0, 1, &value, 0);
}

VA(0x00452be9, 0x276)
void Connect(void) {
    i32 code;
    char idMessage[20];
    u32 randSeed = KBTickCount();
    randSeed %= 1000000;
    gModemIdString[0] = randSeed / 100000 + '0';
    randSeed -= (gModemIdString[0] - '0') * 100000;
    gModemIdString[1] = randSeed / 10000 + '0';
    randSeed -= (gModemIdString[1] - '0') * 10000;
    gModemIdString[2] = randSeed / 1000 + '0';
    randSeed -= (gModemIdString[2] - '0') * 1000;
    gModemIdString[3] = randSeed / 100 + '0';
    randSeed -= (gModemIdString[3] - '0') * 100;
    gModemIdString[4] = randSeed / 10 + '0';
    randSeed -= (gModemIdString[4] - '0') * 10;
    gModemIdString[5] = randSeed + '0';
    gModemIdString[6] = 0;
    gLastIdSendTime = -1;
    gRemoteConnectStage = 0;
    gLocalConnectStage = gRemoteConnectStage;
    do {
        if (ReadPacket()) {
            gPacket[gModemPacketLength] = 0;
            if (gModemPacketLength != DIRECT_CONNECT_ID_PACKET_LENGTH)
                continue;
            if (strncmp(gPacket, "ID", 2))
                continue;
            if (!strncmp(gPacket + 2, gModemIdString, 6)) {
                sprintf(gText, "Duplicate ID Strings!\nSorry Please Try Again\n");
                GOut(gText);
                RemoteCleanup();
            }
            strncpy(gRemoteModemIdString, gPacket + 2, 6);
            gRemoteConnectStage = gPacket[9] - '0';
            gLocalConnectStage = gRemoteConnectStage + 1;
            gLastIdSendTime = -1;
        }
        gConnectTick = KBTickCount();
        if (gConnectTick / 1000 != gLastIdSendTime / 1000) {
            gLastIdSendTime = gConnectTick;
            sprintf(idMessage, "ID%s_%i", gModemIdString, gLocalConnectStage);
            WriteModemPacket(idMessage, strlen(idMessage));
        }
        PollSound();
    } while (gLocalConnectStage < 2);
    while (ReadPacket()) {
    }
}

VA(0x00452e5f, 0x2c3)
b32 WaitForDirectConnect(void) {
    char idMessage[20];
    u32 idSeed;
    switch (gDirectConnectStage) {
        case DIRECT_CONNECT_MAKE_ID:
            idSeed = KBTickCount();
            idSeed %= 1000000;
            gModemIdString[0] = idSeed / 100000 + '0';
            idSeed -= (gModemIdString[0] - '0') * 100000;
            gModemIdString[1] = idSeed / 10000 + '0';
            idSeed -= (gModemIdString[1] - '0') * 10000;
            gModemIdString[2] = idSeed / 1000 + '0';
            idSeed -= (gModemIdString[2] - '0') * 1000;
            gModemIdString[3] = idSeed / 100 + '0';
            idSeed -= (gModemIdString[3] - '0') * 100;
            gModemIdString[4] = idSeed / 10 + '0';
            idSeed -= (gModemIdString[4] - '0') * 10;
            gModemIdString[5] = idSeed + '0';
            gModemIdString[6] = 0;
            gLastIdSendTime = -1;
            gRemoteConnectStage = 0;
            gLocalConnectStage = gRemoteConnectStage;
            gDirectConnectStage++;
            break;
        case DIRECT_CONNECT_EXCHANGE_ID:
            if (ReadPacket()) {
                gPacket[gModemPacketLength] = 0;
                if (gModemPacketLength != DIRECT_CONNECT_ID_PACKET_LENGTH)
                    return false;
                if (strncmp(gPacket, "ID", 2))
                    return false;
                if (!strncmp(gPacket + 2, gModemIdString, 6)) {
                    sprintf(gText, "Duplicate ID Strings!\nSorry Please Try Again\n");
                    GOut(gText);
                    RemoteCleanup();
                }
                strncpy(gRemoteModemIdString, gPacket + 2, 6);
                gRemoteConnectStage = gPacket[9] - '0';
                gLocalConnectStage = gRemoteConnectStage + 1;
                gLastIdSendTime = -1;
            }
            gConnectTick = KBTickCount();
            if (gConnectTick / 1000 != gLastIdSendTime / 1000) {
                gLastIdSendTime = gConnectTick;
                sprintf(idMessage, "ID%s_%i", gModemIdString, gLocalConnectStage);
                WriteModemPacket(idMessage, strlen(idMessage));
            }
            if (gLocalConnectStage >= 2)
                gDirectConnectStage++;
            break;
        case DIRECT_CONNECT_DRAIN:
            if (!ReadPacket())
                return true;
            break;
    }
    return false;
}

VA(0x00453122, 0xe4)
char ReadPacket(void) {
    i32 input;
    char scratch[28];
    if (gModemInQueue.writePosition > 4092) {
        gModemInQueue.writePosition = 0;
        gModemNewPacket = true;
    }
readPacketStart:
    if (gModemNewPacket) {
        gModemPacketLength = 0;
        gModemNewPacket = false;
    }
    do {
    readNextByte:
        input = read_byte();
        if (input < 0)
            return 0;
        if (gModemInEscape) {
            gModemInEscape = false;
            if (H1_ENUM_DECODE(ModemPacketControl, input) == MODEM_PACKET_END) {
                gModemNewPacket = true;
                return 1;
            } else if (H1_ENUM_DECODE(ModemPacketControl, input) == MODEM_PACKET_START) {
                gModemNewPacket = true;
                goto readPacketStart;
            }
        } else if (H1_ENUM_DECODE(ModemPacketControl, input) == MODEM_PACKET_ESCAPE) {
            gModemInEscape = true;
            goto readNextByte;
        }
        if (gModemPacketLength >= MODEM_PACKET_MAX_LENGTH)
            goto readPacketStart;
        gPacket[gModemPacketLength] = input;
        ++gModemPacketLength;
    } while (1);
}

VA(0x00453206, 0xe2)
void WriteModemPacket(char* buffer, i32 length) {
    char unusedText[28];
    char encoded[MODEM_ENCODED_PACKET_SIZE];
    i32 encodedPosition = 0;
    if (length > MODEM_PACKET_MAX_LENGTH)
        return;

    encoded[encodedPosition] = H1_ENUM_ENCODE(ModemPacketControl, MODEM_PACKET_ESCAPE);
    ++encodedPosition;
    encoded[encodedPosition] = H1_ENUM_ENCODE(ModemPacketControl, MODEM_PACKET_START);
    ++encodedPosition;
    while (length--) {
        if (H1_ENUM_DECODE(ModemPacketControl, *buffer) == MODEM_PACKET_ESCAPE) {
            encoded[encodedPosition] = H1_ENUM_ENCODE(ModemPacketControl, MODEM_PACKET_ESCAPE);
            ++encodedPosition;
        }
        encoded[encodedPosition] = *buffer;
        ++encodedPosition;
        ++buffer;
    }
    encoded[encodedPosition] = H1_ENUM_ENCODE(ModemPacketControl, MODEM_PACKET_ESCAPE);
    ++encodedPosition;
    encoded[encodedPosition] = H1_ENUM_ENCODE(ModemPacketControl, MODEM_PACKET_END);
    ++encodedPosition;
    while (write_buffer(encoded, encodedPosition) == 0)
        ForcePollSound();
}

VA(0x004532e8, 0x1e3)
// The eighth flag maps a game position to its net position.
b32 TransmitRemoteData(
    void* data,
    i32 destination,
    i32 length,
    i8 command,
    b8 reliable,
    i8 allowRetryDialog,
    H1_ENUM_PARAM(RemoteMessageType, i8) messageType,
    i8 gamePosDestination
) {
    i32 i;
    b32 result;
    i32 j;
    RemoteMessage msg;
    i32 tries;

    if (!gRemoteOn || gInNetSetup)
        return true;
    if (gamePosDestination && destination != REMOTE_BROADCAST_PLAYER)
        destination = gGamePosToNetPos[destination];
    result = false;
    tries = 0;
    gIDCtr++;
    msg.sender = gThisNetPos;
    msg.id = gIDCtr;
    if (messageType != REMOTE_MESSAGE_DEFAULT)
        msg.type = messageType;
    else
        msg.type = reliable ? REMOTE_MESSAGE_RELIABLE : REMOTE_MESSAGE_UNRELIABLE;
    msg.payloadSize = length;
    msg.command = command;
    if (length > 0)
        memcpy(msg.payload.data, data, length);
    while (result == false && tries <= REMOTE_RETRY_COUNT) {
        result = SendRemoteData(&msg, NULL, destination, length + REMOTE_MESSAGE_HEADER_SIZE);
        if (!reliable && result) {
            return true;
        } else if (result) {
            i = 0;
            while (i < REMOTE_CONFIRM_POLL_COUNT) {
                ForcePollSound();
                if (gLastConfirm == gIDCtr)
                    return true;
                result = false;
                DelayMilli(10);
                i++;
            }
        } else {
            DelayMilli(1000);
        }
        if (allowRetryDialog && tries == REMOTE_RETRY_COUNT && result == false) {
            NormalDialog(localization::Tr("network.send.retry"), NORMAL_DIALOG_TYPE_YES_NO);
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                tries = -1;
        }
        tries++;
    }
    return result;
}

VA(0x004534cb, 0xe4)
RemoteMessage* GetRemoteData(b8 remove) {
    i32 oldestOrder;
    i32 queueIndex;
    i32 selected;

    if (!gRemoteOn || gInNetSetup)
        return NULL;
    oldestOrder = 999999999;
    selected = -1;
    for (queueIndex = 0; queueIndex < REMOTE_QUEUE_CAPACITY; queueIndex++) {
        if (H1_ENUM_ENCODE(RemoteMessageType, gReceiveQueue[queueIndex].type)
            && gInOrder[queueIndex] < oldestOrder) {
            oldestOrder = gInOrder[queueIndex];
            selected = queueIndex;
        }
    }
    if (selected >= 0) {
        memcpy(&gReceiveOut, &gReceiveQueue[selected], REMOTE_MESSAGE_SIZE);
        if (remove)
            gReceiveQueue[selected].type = REMOTE_MESSAGE_NONE;
        gReceiveQueue[selected].sender = NetPosToGamePos(gReceiveQueue[selected].sender);
        return &gReceiveOut;
    }
    return NULL;
}

#define queueFull newFull // frame-slot spelling
VA(0x004535af, 0x46c)
void PollRemote(void) {
    DATA(0x004cc828)
    static b8 gInTimeoutFail = false;
    i8 peerHadControl;
    b8 queueFull;
    i32 i;
    i32 numQueued;
    b32 result;

    if (!gRemoteOn)
        return;
    if (gMapBaseType == MULTIPLAYER_BASE_MODEM)
        comm_wrt_task();
    else if (gMapBaseType == MULTIPLAYER_BASE_NETWORK)
        nb_thr_ctl();
    if (gInNetSetup)
        return;
    numQueued = 0;
    queueFull = false;
    if (KBTickCount() - gLastHeartbeatSend > 5000) {
        gSendMessage.sender = gThisNetPos;
        gSendMessage.type = REMOTE_MESSAGE_HEARTBEAT;
        gSendMessage.payloadSize = 1;
        gSendMessage.command = (gCurPlayer << 4) + gCurHourGlassPhase;
        gSendMessage.payload.data[0] = 1;
        SendRemoteData(&gSendMessage, NULL, 1 - gThisNetPos, REMOTE_MESSAGE_HEADER_SIZE + 1);
        gLastHeartbeatSend = KBTickCount();
    }
    if (KBTickCount() > gLastHeartbeatReceive + 60000 && !gInTimeoutFail) {
        NormalDialog(localization::Tr("network.peer.wait"), NORMAL_DIALOG_TYPE_YES_NO);
        if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            gLastHeartbeatReceive = KBTickCount();
        } else {
            gInTimeoutFail = true;
            if (gHumanPlayer[gCurPlayer]) {
                if (gCurPlayer == gThisGamePos)
                    peerHadControl = 0;
                else
                    peerHadControl = 1;
            } else {
                if (gThisGamePos == gHostGamePos)
                    peerHadControl = 0;
                else
                    peerHadControl = 1;
            }
            ReceiveRemotePlayerExit(1 - gThisGamePos, peerHadControl, false, true);
        }
    }
    for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
        if (H1_ENUM_ENCODE(RemoteMessageType, gReceiveQueue[i].type))
            numQueued++;
    }
    if (numQueued == REMOTE_QUEUE_CAPACITY)
        queueFull = true;
    result = true;
    while (result) {
    nextIncoming:
        result = ReceiveRemoteData(NULL, REMOTE_MESSAGE(gReceiveIn), REMOTE_BROADCAST_PLAYER);
        if (result && REMOTE_MESSAGE(gReceiveIn)->sender != gThisNetPos) {
            if (REMOTE_MESSAGE(gReceiveIn)->type == REMOTE_MESSAGE_CONFIRM) {
                gLastConfirm = REMOTE_MESSAGE(gReceiveIn)->id;
                goto done;
            } else if (REMOTE_MESSAGE(gReceiveIn)->type == REMOTE_MESSAGE_HEARTBEAT) {
                if (REMOTE_MESSAGE(gReceiveIn)->payloadSize == 1
                    && REMOTE_MESSAGE(gReceiveIn)->payload.data[0] == 1)
                    gRemoteReady = true;
                gLastHeartbeatReceive = KBTickCount();
                gHeartbeatSeen = true;
                if (gThisGamePos != gHostGamePos && gCurPlayer != gThisGamePos
                    && gAdvManager->m_active == 1
                    && REMOTE_MESSAGE(gReceiveIn)->command / 16 != gThisGamePos) {
                    gCurPlayer = REMOTE_MESSAGE(gReceiveIn)->command / 16;
                    gCurHourGlassPhase = REMOTE_MESSAGE(gReceiveIn)->command - gCurPlayer * 16;
                }
                goto done;
            } else if (queueFull) {
                goto done;
            }
            if (REMOTE_MESSAGE(gReceiveIn)->type == REMOTE_MESSAGE_RELIABLE) {
                gSendMessage.sender = gThisNetPos;
                gSendMessage.id = REMOTE_MESSAGE(gReceiveIn)->id;
                gSendMessage.type = REMOTE_MESSAGE_CONFIRM;
                gSendMessage.payloadSize = 0;
                SendRemoteData(
                    &gSendMessage,
                    NULL,
                    REMOTE_MESSAGE(gReceiveIn)->sender,
                    REMOTE_MESSAGE_HEADER_SIZE
                );
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (H1_ENUM_ENCODE(RemoteMessageType, gReceiveQueue[i].type)
                    && gReceiveQueue[i].id == REMOTE_MESSAGE(gReceiveIn)->id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_RECENT_ID_COUNT; i++) {
                if (gLastIds[i] == REMOTE_MESSAGE(gReceiveIn)->id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (!H1_ENUM_ENCODE(RemoteMessageType, gReceiveQueue[i].type)) {
                    gInOrder[i] = gInOrderCtr++;
                    memcpy(&gReceiveQueue[i], gReceiveIn, REMOTE_MESSAGE_SIZE);
                    numQueued++;
                    gLastIds[gCurLastID] = REMOTE_MESSAGE(gReceiveIn)->id;
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
#undef queueFull

VA(0x00453a1b, 0x114)
b32 TransmitAndWait(
    void* bytes,
    i32 destination,
    i32 length,
    i8 command,
    i8 responseCommand,
    RemoteMessage** response
) {
    b32 result;
    i32 clock;
    b8 complete;
    RemoteMessage* receivedData;

    if (!gRemoteOn || gInNetSetup)
        return true;
    receivedData = NULL;
    result = TransmitRemoteData(bytes, destination, length, command, true);
    if (result == false)
        goto transmitComplete;
    clock = KBTickCount();
    complete = false;
    while (!complete) {
        if (clock + REMOTE_WAIT_TIMEOUT < KBTickCount()) {
            NormalDialog(localization::Tr("network.send.retry"), NORMAL_DIALOG_TYPE_YES_NO);
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                clock = KBTickCount();
            } else {
                result = false;
                goto transmitComplete;
            }
        }
        ForcePollSound();
        receivedData = GetRemoteData(true);
        if (receivedData && receivedData->type == REMOTE_MESSAGE_RELIABLE
            && receivedData->command == responseCommand)
            complete = true;
    }
    *response = receivedData;
transmitComplete:
    return result;
}

// The object's .data and .bss, in retail address order.
DATA(0x0049f048)
i32 gNetNameIndex = -1;
// No retail code reads these two values.
DATA(0x0049f04c)
i32 gUnusedRemoteValue1 = -1;
DATA(0x0049f050)
i32 gUnusedRemoteValue2 = -1;
DATA(0x0049f054)
i32 gBaudBits = 8;
DATA(0x0049f058)
i32 gLastConfirm = -1;
DATA(0x0049f05c)
i32 gLastHeartbeatReceive = 1999999999;
// Serial-link choices kept from the modem setup; no retail code reads them.
DATA(0x0049f060)
i32 gBaudRates[7] = {300, 1200, 2400, 9600, 19200, 38400, 57600};
DATA(0x0049f07c)
i32 gComIrqs[7] = {1, 2, 3, 4, 5, 7, 9};
// No retail code reads this value.
DATA(0x0049f098)
i32 gUnusedRemoteValue3 = -1;
DATA(0x004cc7e8)
char gModemIdString[8];
DATA(0x004cc6e8)
RemoteMessage gReceiveOut;
DATA(0x004cb40c)
i32 gModemResponseChar;
DATA(0x004cb3e0)
i32 gModemCommandPos;
DATA(0x004cb3d8)
i32 gModemResponseLength;
DATA(0x004ca28c)
i32 gLocalConnectStage;
DATA(0x004ca1a0)
char gPhoneNumber[40];
// No retail code reads this; it holds its retail .bss place.
DATA(0x004ca1c8)
u8 gRemoteOldName[156];
DATA(0x004cc670)
i32 gLastIds[REMOTE_RECENT_ID_COUNT];
DATA(0x004cb510)
i32 gDirectConnectStage;
DATA(0x004ca29c)
char gRemoteModemIdString[8];
// No retail code reads this; it holds its retail .bss place.
DATA(0x004ca2a4)
i32 gRemoteOldLong;
DATA(0x004cb564)
char gPacketSend[256];
DATA(0x004ca298)
i32 gConnectTick;
DATA(0x004cb3f0)
i32 gInOrder[REMOTE_QUEUE_CAPACITY];
DATA(0x004cb410)
RemoteMessage gSendMessage;
DATA(0x004ca264)
char gModemCommand[40];
DATA(0x004ca294)
i32 gLastDialPos;
DATA(0x004cb3dc)
i32 gRemoteConnectStage;
DATA(0x004cb3e4)
i32 gNumNetGuests;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004cb3e8)
i32 gOldRemoteIdBits;
DATA(0x004cb2b0)
char gModemExpectedResponse[40];
DATA(0x004ca290)
i32 gLastIdSendTime;
DATA(0x004ca2a8)
inque_t gModemInQueue;
DATA(0x004cb2d8)
char gPacket[256];
DATA(0x004cb3ec)
i32 gLastActionTime;
DATA(0x004cb664)
char gReceiveIn[REMOTE_MESSAGE_SIZE];
DATA(0x004cb514)
char gModemResponseLine[80];
DATA(0x004cbf70)
RemoteMessage gReceiveQueue[REMOTE_QUEUE_CAPACITY];
DATA(0x004cb768)
outque_t gModemOutQueue;
