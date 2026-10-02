#ifndef HOMM1_SOURCE_REMOTE_H
#define HOMM1_SOURCE_REMOTE_H

#include <Domains.h>
#include <H1/Macros.h>

H1_ENUM_BEGIN(RemoteMessageType)
    REMOTE_MESSAGE_DEFAULT = -1,
    REMOTE_MESSAGE_NONE = 0,
    REMOTE_MESSAGE_CONFIRM = 1,
    REMOTE_MESSAGE_RELIABLE = 2,
    REMOTE_MESSAGE_UNRELIABLE = 3,
    REMOTE_MESSAGE_HEARTBEAT = 4
H1_ENUM_END(RemoteMessageType)

H1_ENUM_BEGIN(RemoteBoxCommand)
    BOX_REMOTE_SAVE = 1,
    BOX_REMOTE_SETUP = 0x1f
H1_ENUM_END(RemoteBoxCommand)

// clang-format off
// RemoteMessage::command values (TransmitRemoteData's command argument and
// the receivers' switches). The save-game transfer (TransmitSaveGame /
// ReceiveSaveGame) and the hero/town exchange before a networked battle
// follow Buka 2.1's GAME.cpp RemoteSaveConstant and EVENTS.cpp
// CombatRemoteCommand numbering; chat text (PopNetBox), combat actions
// (ProcessNextAction) and the exit notice (HandleRemote*Exit) complete it.
// SAVE_INIT and SETUP are the RemoteBoxCommand values.
H1_ENUM_BEGIN(RemoteCommand)
    REMOTE_COMMAND_SAVE_INIT_RESPONSE = 2,
    REMOTE_COMMAND_SAVE_DATA = 3,
    REMOTE_COMMAND_SAVE_ACK_REQUEST = 4,
    REMOTE_COMMAND_SAVE_ACK_RESPONSE = 5,
    REMOTE_COMMAND_SAVE_FINISH = 6,
    REMOTE_COMMAND_CHAT = 11,
    REMOTE_COMMAND_HERO_TOWN_DATA = 0x15,
    REMOTE_COMMAND_HERO_TOWN_CONFIRM = 0x16,
    REMOTE_COMMAND_COMBAT_ACTION = 0x17,
    REMOTE_COMMAND_PLAYER_EXIT = 30
H1_ENUM_END(RemoteCommand)
// clang-format on

H1_ENUM_CONST_BEGIN(RemoteConstant)
    REMOTE_BROADCAST_PLAYER = 0x7f,
    REMOTE_MESSAGE_HEADER_SIZE = 9,
    REMOTE_MESSAGE_SIZE = 0x100,
    REMOTE_QUEUE_CAPACITY = 7,
    REMOTE_RECENT_ID_COUNT = 30,
    REMOTE_RETRY_COUNT = 7,
    REMOTE_CONFIRM_POLL_COUNT = 200
H1_ENUM_CONST_END(RemoteConstant)

H1_ENUM_BEGIN(RemoteGameMode)
    REMOTE_GAME_NONE = 0,
    REMOTE_GAME_NETWORK_HOST = 1,
    REMOTE_GAME_NETWORK_GUEST = 2,
    REMOTE_GAME_MODEM_HOST = 3,
    REMOTE_GAME_MODEM_GUEST = 4,
    REMOTE_GAME_UNSET = 10
H1_ENUM_END(RemoteGameMode)

H1_ENUM_BEGIN(MultiplayerBaseType)
    MULTIPLAYER_BASE_MODEM = 0,
    MULTIPLAYER_BASE_NETWORK = 1,
    MULTIPLAYER_BASE_HOT_SEAT = 2,
    MULTIPLAYER_BASE_UNSET = 10
H1_ENUM_END(MultiplayerBaseType)

// DecodePacket/EncodePacket frame every wire packet with this six-byte header.
#pragma pack(push, 1)
struct RemotePacketHeader {
    signed char source;
    signed char destination;
    unsigned char sequence;
    unsigned char payloadSize;
    unsigned short crc;
};
#pragma pack(pop)

// API-forced: PacketSend/packet are byte buffers framed by this header (Buka
// keeps the same accessor).
#define REMOTE_PACKET(buffer) (reinterpret_cast<RemotePacketHeader*>(buffer))

// Retail TransmitRemoteData fills sender/id/type/command/size at +0/+1/+5/+6/+7
// and copies the payload to +9; GetRemoteData copies 0x100-byte records.
#pragma pack(push, 1)
struct RemoteMessage {
    signed char sender;
    int id;
    signed char type;
    signed char command;
    short payloadSize;
    union {
        char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE];
        // WaitForOtherPlayer passes the first payload dword to ReceiveSaveGame;
        // CheckHandleNet also reads the sender's exit flag after it.
        struct {
            int saveSize;
            int playerExited;
        };
        // Save-game transfer segments: segment index, then segment bytes.
        struct {
            short index;
            char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE - 2];
        } segment;
    } payload;
};
#pragma pack(pop)

extern signed char gbInNetSetup;
extern int giThisNetPos;
// WaitForOtherPlayer stores the game position of net position zero here
// (0x004c6710). Declared ahead of giThisGamePos (0x004c74a0): only this order
// gives the host/this compares in advManager::Main, game::NextPlayer,
// PollRemote and HandleRemoteSuddenExit retail's load order.
extern int giHostGamePos;
extern int iIDCtr;
extern unsigned char GameMode;
extern signed char iMPBaseType;
extern unsigned char gPacketSequence;
extern int iNetNameIndex;
extern char PacketSend[];
extern int giNumNetGuests;
extern int giLastConfirm;
extern signed char gbGamePosToNetPos[];
extern int giThisGamePos;
extern int iInOrder[REMOTE_QUEUE_CAPACITY];
extern RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
extern char rcvBufOut[REMOTE_MESSAGE_SIZE];

int SendRemoteData(unsigned char*, unsigned char*, int, int);
int ReceiveRemoteData(unsigned char*, unsigned char*, int);
int TransmitRemoteData(
    char*,
    int,
    int,
    signed char,
    signed char,
    signed char,
    signed char,
    signed char
);
char* GetRemoteData(signed char);
int TransmitAndWait(char*, int, int, signed char, signed char, char**);
signed char NetPosToGamePos(int);
signed char WaitForOtherPlayer(void);
void RemoteCleanup(void);
void UnloadRemoteDriver(short);
long FileSize(char*);
void WriteModemPacket(char*, int);
char ReadPacket(void);
void calc_crc(unsigned short*, unsigned char*, int);
int EncodePacket(unsigned char*, char, char, int);
int DecodePacket(unsigned char*, int);
signed char InitNetHost(void);
signed char InitNetGuest(void);
signed char WaitForHost(void);
signed char WaitForGuest(void);

// HoMM1 transport entry points as the remote layer calls them: each carries a
// leading unused selector, unlike HoMM2's narrower netwin signatures.
void com_term(short);
H1_C_LINKAGE unsigned short __cdecl nb_init(unsigned short);
H1_C_LINKAGE void __cdecl nb_term(int);
H1_C_LINKAGE short __cdecl nb_rcv(int, unsigned short, void*);
H1_C_LINKAGE short __cdecl nb_snd(int, unsigned short, unsigned short, void*, int);
H1_C_LINKAGE short __cdecl nb_sess(int, int, ...);
H1_C_LINKAGE unsigned char __cdecl nb_stat(int, unsigned short);
// PollRemote's heartbeat clocks, timeout latch, recent-id ring and the
// incoming/outgoing message buffers (Buka REMOTE.h).
extern long lLastHeartbeatSend;
extern long lLastHeartbeatReceive;
extern signed char bInTimeoutFail;
extern RemoteMessage sndBuf;
extern RemoteMessage rcvBufIn;
extern int iLastIds[REMOTE_RECENT_ID_COUNT];
extern int iInOrderCtr;
extern int iCurLastID;
// The network setup's host/guest handshake states and broadcast clock (Buka
// Netbios.h; retail places them inside REMOTE's data, 0x0049f954-0x0049fa70).
extern signed char iInitNetHostStatus;
extern signed char iInitNetGuestStatus;
extern signed char iWaitForHostStatus;
extern signed char iWaitForGuestStatus;
extern long iLastBroadcastTime;

#endif
