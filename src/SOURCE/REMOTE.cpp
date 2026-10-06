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
    FILE* f;
    f = fopen(filename, "r+b");
    if (f == NULL) {
        if (f == NULL)
            FileError(filename);
    }
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    fclose(f);
    return length;
}

VA(0x00451b2f, 0x221)
void RemoteMain(H1_ENUM_PARAM(RemoteGameMode, i32) gameMode) {
    char directConnectMessage[164];

    gInNetSetup = true;
    memset(rcvBuf, 0, sizeof(rcvBuf));
    memset(gLastIds, 0, 30);
    GameMode = gameMode;
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
            inque.writePosition = 0;
            inque.readPosition = 0;
            outque.writePosition = 0;
            outque.readPosition = 0;
            gBaudBits =
                CBR_115200 / gConfig.baudRate[H1_ENUM_DECODE(ConfigConnection, gDirectConnect)];
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
    i16 theCarry;
    i16 mask;
    while (length--) {
        for (mask = REMOTE_CRC_BYTE_TOP_BIT; mask; mask >>= 1) {
            theCarry = *crc & REMOTE_CRC_TOP_BIT;
            *crc <<= 1;
            *crc |= (mask & *data) != 0;
            if (theCarry)
                *crc ^= REMOTE_CRC_POLYNOMIAL;
        }
        data++;
    }
}

VA(0x00451e2e, 0x7b)
i32 EncodePacket(RemoteMessage* data, i8 source, i8 destination, i32 length) {
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

VA(0x00451ea9, 0xb2)
b32 DecodePacket(RemoteMessage* data, i32 source) {
    u16 computedCrc;
    u16 crc;
    i32 i;
    u32 theSize;

    computedCrc = 0;
    if (REMOTE_PACKET(packet)->source != source && source != REMOTE_BROADCAST_PLAYER) {
        return false;
    }
    if (REMOTE_PACKET(packet)->destination != gThisNetPos
        && REMOTE_PACKET(packet)->destination != REMOTE_BROADCAST_PLAYER) {
        return false;
    }
    theSize = REMOTE_PACKET(packet)->payloadSize;
    crc = REMOTE_PACKET(packet)->crc;
    REMOTE_PACKET(packet)->crc = 0;
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&computedCrc, reinterpret_cast<u8*>(packet), theSize + sizeof(RemotePacketHeader));
    if (crc != computedCrc) {
        return false;
    }
    memcpy(data, packet + sizeof(RemotePacketHeader), theSize);
    return true;
}

VA(0x00451f5b, 0x10f)
b32 SendRemoteData(RemoteMessage* dataToSend, u8*, i32 destination, i32 length) {
    i32 size;
    b32 out;
    i32 retry;
    i32 sendStatus;
    u8 remotePacket[REMOTE_MESSAGE_SIZE];

    out = true;
    if (gMapBaseType == MULTIPLAYER_BASE_NETWORK) {
        if (GameMode == REMOTE_GAME_NETWORK_HOST)
            destination = gNetNameIndex + 1;
        else
            destination = 0;
    } else if (destination == REMOTE_BROADCAST_PLAYER) {
        destination = 1 - gThisNetPos;
    }
    size = EncodePacket(dataToSend, gThisNetPos, destination, length);
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            do {
                sendStatus = nb_snd(0, destination, size, PacketSend, 0);
                if (sendStatus) {
                    out = false;
                    goto finished;
                }
            } while (sendStatus);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            WriteModemPacket(PacketSend, size);
            out = true;
            break;
    }
finished:
    return out;
}

VA(0x0045206a, 0xcd)
b32 ReceiveRemoteData(u8*, RemoteMessage* data, i32 decodeType) {
    i32 receiveResult;
    b32 result;

    result = true;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            if (GameMode == REMOTE_GAME_NETWORK_HOST)
                decodeType = gNetNameIndex + 1;
            else
                decodeType = 0;
            receiveResult = nb_rcv(0, 0x100, packet);
            if (receiveResult == 0)
                return false;
            result = DecodePacket(data, decodeType);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            receiveResult = ReadPacket();
            if (receiveResult == 0)
                return false;
            result = DecodePacket(data, decodeType);
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
i32 packetlen = 0;
DATA(0x004cc804)
b32 inescape = false;
DATA(0x004cc808)
b32 newpacket = false;
DATA(0x004cc80c)
i32 gInOrderCtr = 0;
DATA(0x004cc810)
i32 gCurLastID = 0;
DATA(0x004cc814)
H1_ENUM_STORAGE(RemoteGameMode, u8) GameMode = REMOTE_GAME_NONE;
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
    switch (GameMode) {
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
    sprintf(dialCommand, "ATDT%s", numbuf);
    sprintf(gText, "%s %s", localization::Tr("modem.dialing"), numbuf);
    GUIModemCommand(gText, dialCommand);
    sprintf(gText, "%s %s", localization::Tr("modem.dialing"), numbuf);
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
    memset(GUIMRresponse, 0, 80);
    GUIMRrespptr = 0;
    strcpy(GUIMRresp, response);
    gWaitType = DIALOG_WAIT_MODEM_RESPONSE;
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
    if (!gFunctionComplete)
        ShutDown(NULL);
    return 0;
}

VA(0x00452ac9, 0xb3)
b8 GUIModemResponseExec(void) {
    GUIMRc = read_byte();
    if (GUIMRc == -1)
        return false;
    if (GUIMRc == '\n' || GUIMRrespptr == MODEM_RESPONSE_LAST) {
        GUIMRresponse[GUIMRrespptr] = 0;
        if (GUIMRrespptr > 17)
            GUIMRresponse[17] = 0;
        goto compareResponse;
    }
    if (GUIMRc >= ' ') {
        GUIMRresponse[GUIMRrespptr] = GUIMRc;
        ++GUIMRrespptr;
    }
    return false;
compareResponse:
    if (strncmp(GUIMRresponse, GUIMRresp, strlen(GUIMRresp)) != 0) {
        GUIMRrespptr = 0;
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
    char msg[20];
    u32 randSeed = KBTickCount();
    randSeed %= 1000000;
    idstr[0] = randSeed / 100000 + '0';
    randSeed -= (idstr[0] - '0') * 100000;
    idstr[1] = randSeed / 10000 + '0';
    randSeed -= (idstr[1] - '0') * 10000;
    idstr[2] = randSeed / 1000 + '0';
    randSeed -= (idstr[2] - '0') * 1000;
    idstr[3] = randSeed / 100 + '0';
    randSeed -= (idstr[3] - '0') * 100;
    idstr[4] = randSeed / 10 + '0';
    randSeed -= (idstr[4] - '0') * 10;
    idstr[5] = randSeed + '0';
    idstr[6] = 0;
    oldsec = -1;
    remotestage = 0;
    localstage = remotestage;
    do {
        if (ReadPacket()) {
            packet[packetlen] = 0;
            if (packetlen != DIRECT_CONNECT_ID_PACKET_LENGTH)
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
        if (stime / 1000 != oldsec / 1000) {
            oldsec = stime;
            sprintf(msg, "ID%s_%i", idstr, localstage);
            WriteModemPacket(msg, strlen(msg));
        }
        PollSound();
    } while (localstage < 2);
    while (ReadPacket()) {
    }
}

VA(0x00452e5f, 0x2c3)
b32 WaitForDirectConnect(void) {
    char idMessage[20];
    u32 rng;
    switch (WFDCStage) {
        case DIRECT_CONNECT_MAKE_ID:
            rng = KBTickCount();
            rng %= 1000000;
            idstr[0] = rng / 100000 + '0';
            rng -= (idstr[0] - '0') * 100000;
            idstr[1] = rng / 10000 + '0';
            rng -= (idstr[1] - '0') * 10000;
            idstr[2] = rng / 1000 + '0';
            rng -= (idstr[2] - '0') * 1000;
            idstr[3] = rng / 100 + '0';
            rng -= (idstr[3] - '0') * 100;
            idstr[4] = rng / 10 + '0';
            rng -= (idstr[4] - '0') * 10;
            idstr[5] = rng + '0';
            idstr[6] = 0;
            oldsec = -1;
            remotestage = 0;
            localstage = remotestage;
            WFDCStage++;
            break;
        case DIRECT_CONNECT_EXCHANGE_ID:
            if (ReadPacket()) {
                packet[packetlen] = 0;
                if (packetlen != DIRECT_CONNECT_ID_PACKET_LENGTH)
                    return false;
                if (strncmp(packet, "ID", 2))
                    return false;
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
            if (stime / 1000 != oldsec / 1000) {
                oldsec = stime;
                sprintf(idMessage, "ID%s_%i", idstr, localstage);
                WriteModemPacket(idMessage, strlen(idMessage));
            }
            if (localstage >= 2)
                WFDCStage++;
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
    if (inque.writePosition > 4092) {
        inque.writePosition = 0;
        newpacket = true;
    }
readPacketStart:
    if (newpacket) {
        packetlen = 0;
        newpacket = false;
    }
    do {
    readNextByte:
        input = read_byte();
        if (input < 0)
            return 0;
        if (inescape) {
            inescape = false;
            if (H1_ENUM_DECODE(ModemPacketControl, input) == MODEM_PACKET_END) {
                newpacket = true;
                return 1;
            } else if (H1_ENUM_DECODE(ModemPacketControl, input) == MODEM_PACKET_START) {
                newpacket = true;
                goto readPacketStart;
            }
        } else if (H1_ENUM_DECODE(ModemPacketControl, input) == MODEM_PACKET_ESCAPE) {
            inescape = true;
            goto readNextByte;
        }
        if (packetlen >= MODEM_PACKET_MAX_LENGTH)
            goto readPacketStart;
        packet[packetlen] = input;
        ++packetlen;
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
// HoMM1 callers pass an eighth flag that maps a game position to its net position.
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
        if (H1_ENUM_ENCODE(RemoteMessageType, rcvBuf[queueIndex].type)
            && gInOrder[queueIndex] < oldestOrder) {
            oldestOrder = gInOrder[queueIndex];
            selected = queueIndex;
        }
    }
    if (selected >= 0) {
        memcpy(&rcvBufOut, &rcvBuf[selected], REMOTE_MESSAGE_SIZE);
        if (remove)
            rcvBuf[selected].type = REMOTE_MESSAGE_NONE;
        rcvBuf[selected].sender = NetPosToGamePos(rcvBuf[selected].sender);
        return &rcvBufOut;
    }
    return NULL;
}

VA(0x004535af, 0x46c)
void PollRemote(void) {
    DATA(0x004cc828)
    static b8 gInTimeoutFail = false;
    i8 newControl;
    b8 newFull;
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
    newFull = false;
    if (KBTickCount() - gLastHeartbeatSend > 5000) {
        sndBuf.sender = gThisNetPos;
        sndBuf.type = REMOTE_MESSAGE_HEARTBEAT;
        sndBuf.payloadSize = 1;
        sndBuf.command = (gCurPlayer << 4) + gCurHourGlassPhase;
        sndBuf.payload.data[0] = 1;
        SendRemoteData(&sndBuf, NULL, 1 - gThisNetPos, REMOTE_MESSAGE_HEADER_SIZE + 1);
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
                    newControl = 0;
                else
                    newControl = 1;
            } else {
                if (gThisGamePos == gHostGamePos)
                    newControl = 0;
                else
                    newControl = 1;
            }
            ReceiveRemotePlayerExit(1 - gThisGamePos, newControl, false, true);
        }
    }
    for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
        if (H1_ENUM_ENCODE(RemoteMessageType, rcvBuf[i].type))
            numQueued++;
    }
    if (numQueued == REMOTE_QUEUE_CAPACITY)
        newFull = true;
    result = true;
    while (result) {
    nextIncoming:
        result = ReceiveRemoteData(NULL, REMOTE_MESSAGE(rcvBufIn), REMOTE_BROADCAST_PLAYER);
        if (result && REMOTE_MESSAGE(rcvBufIn)->sender != gThisNetPos) {
            if (REMOTE_MESSAGE(rcvBufIn)->type == REMOTE_MESSAGE_CONFIRM) {
                gLastConfirm = REMOTE_MESSAGE(rcvBufIn)->id;
                goto done;
            } else if (REMOTE_MESSAGE(rcvBufIn)->type == REMOTE_MESSAGE_HEARTBEAT) {
                if (REMOTE_MESSAGE(rcvBufIn)->payloadSize == 1
                    && REMOTE_MESSAGE(rcvBufIn)->payload.data[0] == 1)
                    gRemoteReady = true;
                gLastHeartbeatReceive = KBTickCount();
                gHeartbeatSeen = true;
                if (gThisGamePos != gHostGamePos && gCurPlayer != gThisGamePos
                    && gAdvManager->m_active == 1
                    && REMOTE_MESSAGE(rcvBufIn)->command / 16 != gThisGamePos) {
                    gCurPlayer = REMOTE_MESSAGE(rcvBufIn)->command / 16;
                    gCurHourGlassPhase = REMOTE_MESSAGE(rcvBufIn)->command - gCurPlayer * 16;
                }
                goto done;
            } else if (newFull) {
                goto done;
            }
            if (REMOTE_MESSAGE(rcvBufIn)->type == REMOTE_MESSAGE_RELIABLE) {
                sndBuf.sender = gThisNetPos;
                sndBuf.id = REMOTE_MESSAGE(rcvBufIn)->id;
                sndBuf.type = REMOTE_MESSAGE_CONFIRM;
                sndBuf.payloadSize = 0;
                SendRemoteData(
                    &sndBuf,
                    NULL,
                    REMOTE_MESSAGE(rcvBufIn)->sender,
                    REMOTE_MESSAGE_HEADER_SIZE
                );
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (H1_ENUM_ENCODE(RemoteMessageType, rcvBuf[i].type)
                    && rcvBuf[i].id == REMOTE_MESSAGE(rcvBufIn)->id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_RECENT_ID_COUNT; i++) {
                if (gLastIds[i] == REMOTE_MESSAGE(rcvBufIn)->id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (!H1_ENUM_ENCODE(RemoteMessageType, rcvBuf[i].type)) {
                    gInOrder[i] = gInOrderCtr++;
                    memcpy(&rcvBuf[i], rcvBufIn, REMOTE_MESSAGE_SIZE);
                    numQueued++;
                    gLastIds[gCurLastID] = REMOTE_MESSAGE(rcvBufIn)->id;
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
char idstr[8];
DATA(0x004cc6e8)
RemoteMessage rcvBufOut;
DATA(0x004cb40c)
i32 GUIMRc;
DATA(0x004cb3e0)
i32 gModemCommandPos;
DATA(0x004cb3d8)
i32 GUIMRrespptr;
DATA(0x004ca28c)
i32 localstage;
DATA(0x004ca1a0)
char numbuf[40];
// No retail code reads this; it holds its retail .bss place.
DATA(0x004ca1c8)
u8 gRemoteOldName[156];
DATA(0x004cc670)
i32 gLastIds[REMOTE_RECENT_ID_COUNT];
DATA(0x004cb510)
i32 WFDCStage;
DATA(0x004ca29c)
char remoteidstr[8];
// No retail code reads this; it holds its retail .bss place.
DATA(0x004ca2a4)
i32 gRemoteOldLong;
DATA(0x004cb564)
char PacketSend[256];
DATA(0x004ca298)
i32 stime;
DATA(0x004cb3f0)
i32 gInOrder[REMOTE_QUEUE_CAPACITY];
DATA(0x004cb410)
RemoteMessage sndBuf;
DATA(0x004ca264)
char gModemCommand[40];
DATA(0x004ca294)
i32 gLastDialPos;
DATA(0x004cb3dc)
i32 remotestage;
DATA(0x004cb3e4)
i32 gNumNetGuests;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004cb3e8)
i32 gOldRemoteIdBits;
DATA(0x004cb2b0)
char GUIMRresp[40];
DATA(0x004ca290)
i32 oldsec;
DATA(0x004ca2a8)
inque_t inque;
DATA(0x004cb2d8)
char packet[256];
DATA(0x004cb3ec)
i32 gLastActionTime;
DATA(0x004cb664)
char rcvBufIn[REMOTE_MESSAGE_SIZE];
DATA(0x004cb514)
char GUIMRresponse[80];
DATA(0x004cbf70)
RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
DATA(0x004cb768)
outque_t outque;
