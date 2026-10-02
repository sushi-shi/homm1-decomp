// Remote-game transport (network and modem sessions). Buka 2.1 REMOTE
// correspondence. Retail starts this object at 0x00458520 on a 16-byte
// boundary after int3 fill following SETUP's BaseSetupHandler.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <SOURCE/Modem.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/comwin.h>

#include <stdio.h>
#include <string.h>

H1_ENUM_BEGIN(NetbiosSessionStatus)
    NETBIOS_SESSION_ACTIVE_FLAG = 1,
    NETBIOS_SESSION_NAME_REGISTERED = 2,
    NETBIOS_SESSION_NAME_ERROR = 0x80
H1_ENUM_END(NetbiosSessionStatus)

#define NETBIOS_SESSION_ACTIVE NETBIOS_SESSION_ACTIVE_FLAG

extern signed char iInitNetHostStatus;
extern signed char iInitNetGuestStatus;
extern signed char iWaitForHostStatus;
extern signed char iWaitForGuestStatus;
extern long iLastBroadcastTime;
extern signed char gbDirectConnect;

extern int giNumHumanPlayers;
int nbnet_init(void);
void RemoteMain(int);
extern int iLastIds[];

// Buka 2.1 RemoteCleanup without the HoMM2 logging and DirectPlay modes.
VA(0x00458520, 0x8d)
void RemoteCleanup(void) {
    if (!gbRemoteOn)
        return;
    switch (GameMode) {
        case REMOTE_GAME_NETWORK_HOST:
        case REMOTE_GAME_NETWORK_GUEST:
            UnloadRemoteDriver(1);
            break;
        case REMOTE_GAME_MODEM_HOST:
        case REMOTE_GAME_MODEM_GUEST:
            UnloadRemoteDriver(0);
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
            iBaudBits = 115200 / gConfig.baudRate[gbDirectConnect];
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
                giWaitType = 7;
                strcpy(directConnectMessage,
                       "Waiting for other computer to log in to direct connection.");
                NormalDialog(directConnectMessage, 6, -1, -1, -1, 0, -1, 0, -1);
                if (!gbFunctionComplete)
                    ShutDown(0);
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
void UnloadRemoteDriver(short networkDriver) {
    switch (networkDriver) {
        case 0:
            com_term(0);
            break;
        case 1:
            nb_term(0);
            break;
    }
}

// CRC-16/CCITT over the packet bytes, most significant bit first.
VA(0x00458971, 0xb6)
void calc_crc(unsigned short* crc, unsigned char* data, int length) {
    int unused = 0;
    short carry;
    short mask;
    while (length--) {
        for (mask = 0x80; mask; mask >>= 1) {
            carry = *crc & 0x8000;
            *crc <<= 1;
            if (*data & mask)
                *crc |= 1;
            else
                ;
            if (carry)
                *crc ^= 0x1021;
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
    calc_crc(&crc, (unsigned char*)PacketSend, length + sizeof(RemotePacketHeader));
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
        sprintf(gText, "I want packet from %d not %d", source, REMOTE_PACKET(packet)->source);
        LogStr(gText);
        return 0;
    }
    if (REMOTE_PACKET(packet)->destination != giThisNetPos
        && REMOTE_PACKET(packet)->destination != REMOTE_BROADCAST_PLAYER) {
        sprintf(gText, "not mine %d", REMOTE_PACKET(packet)->destination);
        LogStr(gText);
        return 0;
    }
    size = REMOTE_PACKET(packet)->payloadSize;
    crc = REMOTE_PACKET(packet)->crc;
    REMOTE_PACKET(packet)->crc = 0;
    calc_crc(&computedCrc, (unsigned char*)packet, size + sizeof(RemotePacketHeader));
    if (crc != computedCrc) {
        sprintf(
            gText,
            "CRC Check Failed on Packet %d  CRC 1 %d CRC 2 %d",
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
    int unused;
    int needName;

    switch (iInitNetHostStatus) {
        case 0:
            if ((short)nb_init(0) == 1) {
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
            if (nb_sess(0, 0, gText) == 0)
                iInitNetHostStatus++;
            else
                ShutDown("Network initialization failed");
            break;
        case 3:
            needName = nb_stat(0, 0);
            if (needName & NETBIOS_SESSION_NAME_REGISTERED) {
                return 1;
            } else if (needName & NETBIOS_SESSION_NAME_ERROR) {
                iNetNameIndex++;
                if (iNetNameIndex > 10)
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
            if ((short)nb_init(6) == 1) {
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
            if (nb_sess(0, 0, gText) == 0)
                iInitNetGuestStatus++;
            else
                ShutDown("Network initialization failed");
            break;
        case 3:
            status = nb_stat(0, 6);
            unregistered = !(status & NETBIOS_SESSION_NAME_REGISTERED);
            if (unregistered) {
                if (status & NETBIOS_SESSION_NAME_ERROR) {
                    giThisNetPos++;
                    if (giThisNetPos > 10) {
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
            if (nb_sess(0, 1, 0) != 0) {
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
    char buffer[80];
    int status;

    switch (iWaitForGuestStatus) {
        case 0:
            status = nb_sess(0, 3, 6);
            if (status == 0)
                iWaitForGuestStatus++;
            return 0;
        case 1:
            status = !(nb_stat(0, 6) & NETBIOS_SESSION_ACTIVE);
            if (status) {
                if (KBTickCount() > iLastBroadcastTime + 500) {
                    iLastBroadcastTime = KBTickCount();
                    nb_snd(0, 0, 0, 0, 0);
                }
            } else {
                giNumNetGuests++;
                nb_sess(0, 5, 6, iNetNameIndex + 1, 1);
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
            giWaitType = 4;
            sprintf(gText, "Initializing network.");
            NormalDialog(gText, 6, -1, -1, -1, 0, -1, 0, -1);
            if (!gbFunctionComplete)
                ShutDown(0);
            giWaitType = 1;
            sprintf(gText, "Waiting On Guest.");
            NormalDialog(gText, 6, -1, -1, -1, 0, -1, 0, -1);
            if (!gbFunctionComplete)
                ShutDown(0);
            buffer[0] = giNumNetGuests;
            while (nb_snd(0, iNetNameIndex + 1, 3, buffer, 0))
                PollSound();
            break;
        case REMOTE_GAME_NETWORK_GUEST:
            giWaitType = 3;
            sprintf(gText, "Initializing network.");
            NormalDialog(gText, 6, -1, -1, -1, 0, -1, 0, -1);
            if (!gbFunctionComplete)
                ShutDown(0);
            giWaitType = 2;
            sprintf(gText, "Waiting On Host.");
            NormalDialog(gText, 6, -1, -1, -1, 0, -1, 0, -1);
            if (!gbFunctionComplete)
                ShutDown(0);
            break;
    }
    return 0;
}
