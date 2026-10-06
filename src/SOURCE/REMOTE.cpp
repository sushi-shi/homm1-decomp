#include <H1/Ints.h>

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

struct outque_t {
    i32 readPosition;
    i32 writePosition;
    char data[2048];
};

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

void UnloadRemoteDriver(i16 networkDriver) {
    switch (networkDriver) {
        case REMOTE_DRIVER_SERIAL:
            com_term(0);
            break;
        case REMOTE_DRIVER_NETBIOS:
            nb_term(0);
            break;
    }
}

enum RemoteCrcConstant {
    REMOTE_CRC_BYTE_TOP_BIT = 0x80,
    REMOTE_CRC_TOP_BIT = 0x8000,
    REMOTE_CRC_POLYNOMIAL = 0x1021
};

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

i32 EncodePacket(u8* data, i8 source, i8 destination, i32 length) {
    u16 crc;

    REMOTE_PACKET(PacketSend)->source = source;
    REMOTE_PACKET(PacketSend)->destination = destination;
    REMOTE_PACKET(PacketSend)->sequence = gPacketSequence;
    REMOTE_PACKET(PacketSend)->payloadSize = length;
    crc = 0;
    REMOTE_PACKET(PacketSend)->crc = crc;
    memcpy(PacketSend + sizeof(RemotePacketHeader), data, length);
    calc_crc(&crc, reinterpret_cast<u8*>(PacketSend), length + sizeof(RemotePacketHeader));
    REMOTE_PACKET(PacketSend)->crc = crc;
    return length + sizeof(RemotePacketHeader);
}

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

i8 InitNetHost(void) {
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

i8 WaitForGuest(void) {
    static i8 gWaitForGuestStatus = 0;
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

i32 Wait(void) {
    GUIModemResponse("Waiting for ring...", "RING");
    GUIModemCommand("Initializing modem...", "ATA");
    if (GUIModemResponse("Establishing connection...", "CONNECT"))
        return 1;
    return 0;
}

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

void ModemCommand(char* command) {
    i32 pos;
    i32 len = strlen(command);
    for (pos = 0; pos < len; ++pos) {
        write_buffer(command + pos, 1);
        DelayMilli(100);
    }
    write_buffer("\r", 1);
}

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

i32 write_buffer(char* buffer, i32 length) {
    com_snd(0, 0, length, buffer, 0);
    return 1;
}

i32 read_byte(void) {
    u8 ch;
    i32 received = com_rcv(0, 1, &ch);
    if (received == 1)
        return ch;
    else
        return -1;
}

void write_byte(i32 value) {
    com_snd(0, 0, 1, &value, 0);
}

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

char ReadPacket(void) {
    i32 input;
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
            reinterpret_cast<u8*>(&msg),
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

void PollRemote(void) {
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
        );
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
        );
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
                );
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
        received = reinterpret_cast<RemoteMessage*>(GetRemoteData(1));
        if (received && received->type == REMOTE_MESSAGE_RELIABLE
            && received->command == responseCommand)
            complete = 1;
    }
    *response = reinterpret_cast<char*>(received);
transmitComplete:
    return result;
}

i32 gNetNameIndex = -1;
i32 gIDCtr = 0;
i32 gBaudBits = 8;
i32 packetlen = 0;
i32 inescape = 0;
i32 newpacket = 0;
i32 gInOrderCtr = 0;
i32 gLastConfirm = -1;
i32 gCurLastID = 0;
u8 GameMode = 0;
u8 gPacketSequence = 0;
i32 gLastHeartbeatSend = 0;
i32 gLastHeartbeatReceive = 1999999999;
i8 gInNetSetup = 0;
i8 gInitNetGuestStatus = 0;
i8 gWaitForHostStatus = 0;
char idstr[8];
char rcvBufOut[REMOTE_MESSAGE_SIZE];
i32 GUIMRc;
i32 iModemCommandPos;
i32 GUIMRrespptr;
i32 localstage;
char numbuf[40];
i32 iLastIds[REMOTE_RECENT_ID_COUNT];
i32 WFDCStage;
char remoteidstr[8];
char PacketSend[256];
i32 stime;
i32 iInOrder[REMOTE_QUEUE_CAPACITY];
RemoteMessage sndBuf;
char cModemCommand[40];
i32 iLastDialPos;
i32 remotestage;
i32 gNumNetGuests;
char GUIMRresp[40];
i32 oldsec;
inque_t inque;
char packet[256];
i32 iLastActionTime;
RemoteMessage rcvBufIn;
char GUIMRresponse[80];
RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
outque_t outque;
