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

// Buka 2.1 RemoteMain merged with the HoMM2 ModemSetup mode switch; HoMM1
// keeps the modem reset sequence in ModemSetup (0x459530).
VA(0x00451b2f, 0x221)
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
                strcpy(directConnectMessage, localization::Tr("network.direct.wait"));
                NormalDialog(directConnectMessage, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
                if (!gbFunctionComplete)
                    ShutDown(NULL);
            } else {
                Connect();
            }
            break;
    }
    gRemoteOn = 1;
    giNumHumanPlayers = gNumNetGuests + 1;
    gIDCtr = (giThisNetPos + gNetNameIndex * 400 + 1) * 100000000;
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

VA(0x00451ea9, 0xb2)
i32 DecodePacket(u8* data, i32 source) {
    u16 computedCrc;
    u16 crc;
    i32 i;
    u32 theSize;

    computedCrc = 0;
    if (REMOTE_PACKET(packet)->source != source && source != REMOTE_BROADCAST_PLAYER) {
        return 0;
    }
    if (REMOTE_PACKET(packet)->destination != giThisNetPos
        && REMOTE_PACKET(packet)->destination != REMOTE_BROADCAST_PLAYER) {
        return 0;
    }
    theSize = REMOTE_PACKET(packet)->payloadSize;
    crc = REMOTE_PACKET(packet)->crc;
    REMOTE_PACKET(packet)->crc = 0;
    // API-forced: calc_crc takes unsigned bytes; the wire buffer is char[].
    calc_crc(&computedCrc, reinterpret_cast<u8*>(packet), theSize + sizeof(RemotePacketHeader));
    if (crc != computedCrc) {
        return 0;
    }
    memcpy(data, packet + sizeof(RemotePacketHeader), theSize);
    return 1;
}

VA(0x00451f5b, 0x10f)
i32 SendRemoteData(u8* dataToSend, u8*, i32 destination, i32 length) {
    i32 size;
    i32 out;
    i32 retry;
    i32 sendStatus;
    u8 remotePacket[REMOTE_MESSAGE_SIZE];

    out = 1;
    if (iMPBaseType == MULTIPLAYER_BASE_NETWORK) {
        if (GameMode == REMOTE_GAME_NETWORK_HOST)
            destination = gNetNameIndex + 1;
        else
            destination = 0;
    } else if (destination == REMOTE_BROADCAST_PLAYER) {
        destination = 1 - giThisNetPos;
    }
    size = EncodePacket(dataToSend, giThisNetPos, destination, length);
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            do {
                sendStatus = nb_snd(0, destination, size, PacketSend, 0);
                if (sendStatus) {
                    out = 0;
                    goto finished;
                }
            } while (sendStatus);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            WriteModemPacket(PacketSend, size);
            out = 1;
            break;
    }
finished:
    return out;
}

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

VA(0x00452137, 0x16d)
i8 InitNetHost(void) {
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
                gRemoteOn = 1;
                giThisNetPos = 0;
            }
            break;
        case NET_HOST_INIT_CHECK_NAME:
            needName = !(nb_stat(0, 0) & NETBIOS_SESSION_NAME_REGISTERED);
            if (needName)
                gInitNetHostStatus++;
            else
                return 1;
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
                return 1;
            } else if (needName & NETBIOS_SESSION_ERROR) {
                gNetNameIndex++;
                if (gNetNameIndex > REMOTE_NET_NAME_LAST)
                    ShutDown(localization::Tr("network.initialize.slots_full"));
            }
            break;
    }
    return 0;
}

VA(0x004522a4, 0x1d9)
i8 InitNetGuest(void) {
    i32 status;
    i32 unregistered;

    switch (gInitNetGuestStatus) {
        case NET_GUEST_INIT_START:
            if (static_cast<i16>(nb_init(6)) == 1) {
                ShutDown(localization::Tr("network.netbios.missing"));
            } else {
                gRemoteOn = 1;
                giThisNetPos = 1;
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
            sprintf(gText, "HGUEST%d", giThisNetPos);
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
                    giThisNetPos++;
                    if (giThisNetPos > REMOTE_NET_NAME_LAST) {
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
            return 1;
    }
    return 0;
}

VA(0x0045247d, 0x75)
i8 WaitForHost(void) {
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
                return 1;
            }
            break;
    }
    return 0;
}

VA(0x004524f2, 0xd6)
i8 WaitForGuest(void) {
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
            return 0;
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
            sprintf(gText, localization::Tr("network.initialize.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            giWaitType = DIALOG_WAIT_NETBIOS_GUEST;
            sprintf(gText, localization::Tr("network.guest.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            buffer[0] = gNumNetGuests;
            while (nb_snd(0, gNetNameIndex + 1, 3, buffer, 0))
                PollSound();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            giWaitType = DIALOG_WAIT_NETBIOS_INIT_GUEST;
            sprintf(gText, localization::Tr("network.initialize.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            giWaitType = DIALOG_WAIT_NETBIOS_HOST;
            sprintf(gText, localization::Tr("network.host.wait"));
            NormalDialog(gText, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
            if (!gbFunctionComplete)
                ShutDown(NULL);
            break;
    }
    return 0;
}

// Buka 2.1 ModemSetup reset loop: open the port and reset a dial-up modem.
VA(0x0045275e, 0xe0)
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

VA(0x0045283e, 0x95)
i32 Dial(void) {
    char dialCommand[40];
    iLastDialPos = 0;
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
    iLastActionTime = 0;
    iModemCommandPos = 0;
    giWaitType = DIALOG_WAIT_MODEM_COMMAND;
    strcpy(cModemCommand, command);
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
    if (!gbFunctionComplete)
        ShutDown(NULL);
}

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
    giWaitType = DIALOG_WAIT_MODEM_RESPONSE;
    NormalDialog(message, NORMAL_DIALOG_TYPE_WAIT_CANCEL);
    if (!gbFunctionComplete)
        ShutDown(NULL);
    return 0;
}

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
i32 WaitForDirectConnect(void) {
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
                return 1;
            break;
    }
    return 0;
}

VA(0x00453122, 0xe4)
char ReadPacket(void) {
    i32 input;
    // Unused; retail reserves 0x20 bytes with the input below it.
    char scratch[28];
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

VA(0x00453206, 0xe2)
void WriteModemPacket(char* buffer, i32 length) {
    char unusedText[28]; // dead local: retail's /Od frame holds its unreferenced bytes
    char encoded[MODEM_ENCODED_PACKET_SIZE];
    i32 encodedPosition = 0;
    if (length > MODEM_PACKET_MAX_LENGTH)
        return;

    encoded[encodedPosition] = MODEM_PACKET_ESCAPE;
    ++encodedPosition;
    encoded[encodedPosition] = MODEM_PACKET_START;
    ++encodedPosition;
    while (length--) {
        if (*buffer == MODEM_PACKET_ESCAPE) {
            encoded[encodedPosition] = MODEM_PACKET_ESCAPE;
            ++encodedPosition;
        }
        encoded[encodedPosition] = *buffer;
        ++encodedPosition;
        ++buffer;
    }
    encoded[encodedPosition] = MODEM_PACKET_ESCAPE;
    ++encodedPosition;
    encoded[encodedPosition] = MODEM_PACKET_END;
    ++encodedPosition;
    while (write_buffer(encoded, encodedPosition) == 0)
        ForcePollSound();
}

VA(0x004532e8, 0x1e3)
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
    i32 i;
    i32 result;
    i32 j;
    RemoteMessage msg;
    i32 tries;

    if (!gRemoteOn || gInNetSetup)
        return 1;
    if (gamePosDestination && destination != REMOTE_BROADCAST_PLAYER)
        destination = gbGamePosToNetPos[destination];
    result = 0;
    tries = 0;
    gIDCtr++;
    msg.sender = giThisNetPos;
    msg.id = gIDCtr;
    if (messageType != REMOTE_MESSAGE_DEFAULT)
        msg.type = messageType;
    else
        msg.type = reliable ? REMOTE_MESSAGE_RELIABLE : REMOTE_MESSAGE_UNRELIABLE;
    msg.payloadSize = length;
    msg.command = command;
    if (length > 0)
        memcpy(msg.payload.data, data, length);
    while (result == 0 && tries <= REMOTE_RETRY_COUNT) {
        result = SendRemoteData(
            reinterpret_cast<u8*>(&msg), // API-forced: SendRemoteData takes wire bytes.
            NULL,
            destination,
            length + REMOTE_MESSAGE_HEADER_SIZE
        );
        if (!reliable && result) {
            return 1;
        } else if (result) {
            i = 0;
            while (i < REMOTE_CONFIRM_POLL_COUNT) {
                ForcePollSound();
                if (gLastConfirm == gIDCtr)
                    return 1;
                result = 0;
                DelayMilli(10);
                i++;
            }
        } else {
            DelayMilli(1000);
        }
        if (allowRetryDialog && tries == REMOTE_RETRY_COUNT && result == 0) {
            NormalDialog(localization::Tr("network.send.retry"), NORMAL_DIALOG_TYPE_YES_NO);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                tries = -1;
        }
        tries++;
    }
    return result;
}

VA(0x004534cb, 0xe4)
char* GetRemoteData(i8 remove) {
    i32 oldestOrder;
    i32 queueIndex;
    i32 selected;

    if (!gRemoteOn || gInNetSetup)
        return NULL;
    oldestOrder = 999999999;
    selected = -1;
    for (queueIndex = 0; queueIndex < REMOTE_QUEUE_CAPACITY; queueIndex++) {
        if (rcvBuf[queueIndex].type && iInOrder[queueIndex] < oldestOrder) {
            oldestOrder = iInOrder[queueIndex];
            selected = queueIndex;
        }
    }
    if (selected >= 0) {
        memcpy(rcvBufOut, &rcvBuf[selected], REMOTE_MESSAGE_SIZE);
        if (remove)
            rcvBuf[selected].type = REMOTE_MESSAGE_NONE;
        rcvBuf[selected].sender = NetPosToGamePos(rcvBuf[selected].sender);
        return rcvBufOut;
    }
    return NULL;
}

VA(0x004535af, 0x46c)
void PollRemote(void) {
    DATA(0x004cc828)
    static i8 gInTimeoutFail = 0;
    i8 newControl;
    i8 newFull;
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
    newFull = 0;
    if (KBTickCount() - gLastHeartbeatSend > 5000) {
        sndBuf.sender = giThisNetPos;
        sndBuf.type = REMOTE_MESSAGE_HEARTBEAT;
        sndBuf.payloadSize = 1;
        sndBuf.command = (giCurPlayer << 4) + gCurHourGlassPhase;
        sndBuf.payload.data[0] = 1;
        SendRemoteData(
            reinterpret_cast<u8*>(&sndBuf), // API-forced: SendRemoteData takes wire bytes.
            NULL,
            1 - giThisNetPos,
            REMOTE_MESSAGE_HEADER_SIZE + 1
        ); // API-forced: wire bytes.
        gLastHeartbeatSend = KBTickCount();
    }
    if (KBTickCount() > gLastHeartbeatReceive + 60000 && !gInTimeoutFail) {
        NormalDialog(localization::Tr("network.peer.wait"), NORMAL_DIALOG_TYPE_YES_NO);
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
                if (giThisGamePos == giHostGamePos)
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
        newFull = 1;
    result = 1;
    while (result) {
    nextIncoming:
        result = ReceiveRemoteData(
            NULL,
            reinterpret_cast<u8*>(&rcvBufIn), // API-forced: ReceiveRemoteData takes wire bytes.
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
                if (giThisGamePos != giHostGamePos && giCurPlayer != giThisGamePos
                    && gpAdvManager->m_active == 1 && rcvBufIn.command / 16 != giThisGamePos) {
                    giCurPlayer = rcvBufIn.command / 16;
                    gCurHourGlassPhase = rcvBufIn.command - giCurPlayer * 16;
                }
                goto done;
            } else if (newFull) {
                goto done;
            }
            if (rcvBufIn.type == REMOTE_MESSAGE_RELIABLE) {
                sndBuf.sender = giThisNetPos;
                sndBuf.id = rcvBufIn.id;
                sndBuf.type = REMOTE_MESSAGE_CONFIRM;
                sndBuf.payloadSize = 0;
                SendRemoteData(
                    reinterpret_cast<u8*>(&sndBuf), // API-forced: SendRemoteData takes wire bytes.
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

VA(0x00453a1b, 0x114)
i32 TransmitAndWait(
    char* bytes,
    i32 destination,
    i32 length,
    i8 command,
    i8 responseCommand,
    char** response
) {
    i32 result;
    i32 clock;
    i8 complete;
    RemoteMessage* receivedData;

    if (!gRemoteOn || gInNetSetup)
        return 1;
    receivedData = NULL;
    result = TransmitRemoteData(bytes, destination, length, command, 1);
    if (result == 0)
        goto transmitComplete;
    clock = KBTickCount();
    complete = 0;
    while (!complete) {
        if (clock + 20000 < KBTickCount()) {
            NormalDialog(localization::Tr("network.send.retry"), NORMAL_DIALOG_TYPE_YES_NO);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                clock = KBTickCount();
            } else {
                result = 0;
                goto transmitComplete;
            }
        }
        ForcePollSound();
        receivedData =
            reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
        if (receivedData && receivedData->type == REMOTE_MESSAGE_RELIABLE
            && receivedData->command == responseCommand)
            complete = 1;
    }
    *response = reinterpret_cast<char*>(receivedData); // API-forced: char* record.
transmitComplete:
    return result;
}

// The object's .data and .bss, in retail address order.
DATA(0x0049f048)
i32 gNetNameIndex = -1;
DATA(0x004cc7f0)
i32 gIDCtr = 0;
DATA(0x0049f054)
i32 gBaudBits = 8;
DATA(0x004cc800)
i32 packetlen = 0;
DATA(0x004cc804)
i32 inescape = 0;
DATA(0x004cc808)
i32 newpacket = 0;
DATA(0x004cc80c)
i32 gInOrderCtr = 0;
DATA(0x0049f058)
i32 gLastConfirm = -1;
DATA(0x004cc810)
i32 gCurLastID = 0;
DATA(0x004cc814)
u8 GameMode = 0;
DATA(0x004cc815)
u8 gPacketSequence = 0;
DATA(0x004cc818)
i32 gLastHeartbeatSend = 0;
DATA(0x0049f05c)
i32 gLastHeartbeatReceive = 1999999999;
DATA(0x004cc81c)
i8 gInNetSetup = 0;
DATA(0x004cc81e)
i8 gInitNetGuestStatus = 0;
DATA(0x004cc81f)
i8 gWaitForHostStatus = 0;
DATA(0x004cc7e8)
char idstr[8];
DATA(0x004cc6e8)
char rcvBufOut[REMOTE_MESSAGE_SIZE];
DATA(0x004cb40c)
i32 GUIMRc;
DATA(0x004cb3e0)
i32 iModemCommandPos;
DATA(0x004cb3d8)
i32 GUIMRrespptr;
DATA(0x004ca28c)
i32 localstage;
DATA(0x004ca1a0)
char numbuf[40];
DATA(0x004cc670)
i32 iLastIds[REMOTE_RECENT_ID_COUNT];
DATA(0x004cb510)
i32 WFDCStage;
DATA(0x004ca29c)
char remoteidstr[8];
DATA(0x004cb564)
char PacketSend[256];
DATA(0x004ca298)
i32 stime;
DATA(0x004cb3f0)
i32 iInOrder[REMOTE_QUEUE_CAPACITY];
DATA(0x004cb410)
RemoteMessage sndBuf;
DATA(0x004ca264)
char cModemCommand[40];
DATA(0x004ca294)
i32 iLastDialPos;
DATA(0x004cb3dc)
i32 remotestage;
DATA(0x004cb3e4)
i32 gNumNetGuests;
DATA(0x004cb2b0)
char GUIMRresp[40];
DATA(0x004ca290)
i32 oldsec;
DATA(0x004ca2a8)
inque_t inque;
DATA(0x004cb2d8)
char packet[256];
DATA(0x004cb3ec)
i32 iLastActionTime;
DATA(0x004cb664)
RemoteMessage rcvBufIn;
DATA(0x004cb514)
char GUIMRresponse[80];
DATA(0x004cbf70)
RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
DATA(0x004cb768)
outque_t outque;
