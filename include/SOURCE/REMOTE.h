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

H1_ENUM_CONST_BEGIN(RemoteConstant)
    REMOTE_BROADCAST_PLAYER = 0x7f,
    REMOTE_MESSAGE_HEADER_SIZE = 9,
    REMOTE_MESSAGE_SIZE = 0x100,
    REMOTE_QUEUE_CAPACITY = 7,
    REMOTE_RECENT_ID_COUNT = 30,
    REMOTE_RETRY_COUNT = 7,
    REMOTE_CONFIRM_POLL_COUNT = 200,
    // InitNetHost/InitNetGuest try the NetBIOS names HHOST0../HGUEST1.. up to
    // this suffix before reporting every game slot used.
    REMOTE_NET_NAME_LAST = 10,
    // HoMM1's modem and network games connect two human players.
    REMOTE_PLAYER_COUNT = 2
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

// UnloadRemoteDriver's driver: the serial (com_*) driver for modem and direct
// connect games, NetBIOS (nb_*) for network games (RemoteCleanup).
H1_ENUM_BEGIN(RemoteDriverType)
    REMOTE_DRIVER_SERIAL = 0,
    REMOTE_DRIVER_NETBIOS = 1
H1_ENUM_END(RemoteDriverType)

// DecodePacket/EncodePacket frame every wire packet with this six-byte header.
#pragma pack(push, 1)
struct RemotePacketHeader {
    i8 source;
    i8 destination;
    u8 sequence;
    u8 payloadSize;
    u16 crc;
};
#pragma pack(pop)

// API-forced: PacketSend/packet are byte buffers framed by this header (Buka
// keeps the same accessor).
#define REMOTE_PACKET(buffer) (reinterpret_cast<RemotePacketHeader*>(buffer))

// A remote message's payload. TransmitRemoteData copies the caller's buffer to
// +9 of the record; the save-game transfer builds its packets in this layout.
#pragma pack(push, 1)
union RemotePayload {
    char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE];
    // WaitForOtherPlayer passes the first payload dword to ReceiveSaveGame;
    // CheckHandleNet also reads the sender's exit flag after it.
    struct {
        i32 saveSize;
        i32 playerExited;
    };
    // Save-game transfer segments: segment index, then segment bytes.
    struct {
        i16 index;
        char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE - 2];
    } segment;
};
#pragma pack(pop)

// Retail TransmitRemoteData fills sender/id/type/command/size at +0/+1/+5/+6/+7
// and copies the payload to +9; GetRemoteData copies 0x100-byte records.
#pragma pack(push, 1)
struct RemoteMessage {
    i8 sender;
    i32 id;
    i8 type;
    i8 command;
    i16 payloadSize;
    RemotePayload payload;
};
#pragma pack(pop)

extern i8 gInNetSetup;
extern i32 gIDCtr;
extern u8 GameMode;
extern u8 gPacketSequence;
extern i32 gNetNameIndex;
extern char PacketSend[];
extern i32 iNetGuests;
extern i32 gLastConfirm;
extern i32 iInOrder[REMOTE_QUEUE_CAPACITY];
extern RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
extern char rcvBufOut[REMOTE_MESSAGE_SIZE];

i32 SendRemoteData(u8* dataToSend, u8*, i32 destination, i32 length);
i32 ReceiveRemoteData(u8*, u8* data, i32 decodeType);
// Buka 2.1 REMOTE.h defaults the retry dialog and message type; HoMM1's
// trailing destination flag defaults to its usual game-position addressing.
i32 TransmitRemoteData(
    char* data,
    i32 destination,
    i32 length,
    i8 command,
    i8 reliable,
    i8 allowRetryDialog = 1,
    i8 messageType = REMOTE_MESSAGE_DEFAULT,
    i8 gamePosDestination = 1
);
char* GetRemoteData(i8 remove);
i32 TransmitAndWait(
    char* bytes,
    i32 destination,
    i32 length,
    i8 command,
    i8 responseCommand,
    char** response
);
void RemoteCleanup(void);
void UnloadRemoteDriver(H1_ENUM_PARAM(RemoteDriverType, i16) networkDriver);
i32 FileSize(char* filename);
void WriteModemPacket(char* buffer, i32 length);
char ReadPacket(void);
void calc_crc(u16* crc, u8* data, i32 length);
i32 EncodePacket(u8* data, i8 source, i8 destination, i32 length);
i32 DecodePacket(u8* data, i32 source);
i8 InitNetHost(void);
i8 InitNetGuest(void);
i8 WaitForHost(void);
i8 WaitForGuest(void);

// PollRemote's heartbeat clocks, timeout latch, recent-id ring and the
// incoming/outgoing message buffers (Buka REMOTE.h).
extern i32 gLastHeartbeatSend;
extern i32 gLastHeartbeatReceive;
extern RemoteMessage sndBuf;
extern RemoteMessage rcvBufIn;
extern i32 iLastIds[REMOTE_RECENT_ID_COUNT];
extern i32 gInOrderCtr;
extern i32 gCurLastID;
// The network setup's host/guest handshake states and broadcast clock (Buka
// Netbios.h; retail places them inside REMOTE's data, 0x004a2d4c-0x004a2e68).
extern i8 gInitNetGuestStatus;
extern i8 gWaitForHostStatus;
void PollRemote();
// HoMM1 REMOTE.cpp defines the transport bring-up (Buka REMOTE and Netbios).
void RemoteMain(i32 gameMode);
i32 nbnet_init(void);

H1_ENUM_BEGIN(ModemResponseLimit)
    MODEM_RESPONSE_LAST = 79
H1_ENUM_END(ModemResponseLimit)

// WriteModemPacket frames a packet as ESCAPE START ... ESCAPE END, doubling
// an ESCAPE byte inside it; ReadPacket undoes it.
H1_ENUM_BEGIN(ModemPacketControl)
    MODEM_PACKET_START = 0,
    MODEM_PACKET_END = 1,
    MODEM_PACKET_ESCAPE = 0x70
H1_ENUM_END(ModemPacketControl)

H1_ENUM_CONST_BEGIN(ModemPacketConstant)
    MODEM_PACKET_MAX_LENGTH = 0x100,
    // Worst case: every payload byte escaped, plus both frame markers.
    MODEM_ENCODED_PACKET_SIZE = 516
H1_ENUM_CONST_END(ModemPacketConstant)

extern i32 iLastActionTime;
extern i32 iModemCommandPos;
extern char cModemCommand[];
extern char GUIMRresponse[];
extern char GUIMRresp[];
extern i32 GUIMRrespptr;
extern i32 GUIMRc;
extern i32 iLastDialPos;
extern char numbuf[];
struct inque_t {
    i32 readPosition;
    i32 writePosition;
    char data[4096];
};
extern inque_t inque;
// The transmit queue holds 2K (retail 0x004c9c80-0x004ca487); SETUP.cpp
// completes its type.
extern struct outque_t outque;
extern i32 gBaudBits;
extern i32 inescape;
extern i32 newpacket;
extern i32 packetlen;
extern char packet[];
extern char idstr[];
extern char remoteidstr[];
extern i32 oldsec;
extern i32 stime;
extern i32 remotestage;
extern i32 localstage;
extern i32 WFDCStage;

void GUIModemCommand(char* message, char* command);
void ModemCommand(char* command);
void ModemSetup(void);
i32 Dial(void);
i32 Wait(void);
void Connect(void);
i8 GUIModemResponse(char* message, char* response);
i32 write_buffer(char* buffer, i32 length);
i32 read_byte(void);
// Modem.cpp's wait-loop steps that KB's WaitHandler drives.
i8 GUIModemCommandExec(void);
i8 GUIModemResponseExec(void);
i32 WaitForDirectConnect(void);

// Moved from REMOTE.cpp.
// CRC-16/CCITT over the packet bytes, most significant bit first.
H1_ENUM_CONST_BEGIN(RemoteCrcConstant)
    REMOTE_CRC_BYTE_TOP_BIT = 0x80,
    REMOTE_CRC_TOP_BIT = 0x8000,
    REMOTE_CRC_POLYNOMIAL = 0x1021
H1_ENUM_CONST_END(RemoteCrcConstant)

// Modem's 2K transmit queue (retail 0x004c9c80-0x004ca487), defined below.
struct outque_t {
    i32 readPosition;
    i32 writePosition;
    char data[2048];
};

// InitNetHost's polling stages (its static gInitNetHostStatus): start
// NetBIOS, check for an existing name, register HHOST<n>, then wait for the
// registration (trying the next suffix on a NetBIOS error).
H1_ENUM_CONST_BEGIN(NetHostInitStage)
    NET_HOST_INIT_START = 0,
    NET_HOST_INIT_CHECK_NAME = 1,
    NET_HOST_INIT_REGISTER_NAME = 2,
    NET_HOST_INIT_WAIT_NAME = 3
H1_ENUM_CONST_END(NetHostInitStage)

// InitNetGuest's polling stages (gInitNetGuestStatus): as the host's, then a
// session receive; an already registered name skips straight to RECEIVE.
H1_ENUM_CONST_BEGIN(NetGuestInitStage)
    NET_GUEST_INIT_START = 0,
    NET_GUEST_INIT_CHECK_NAME = 1,
    NET_GUEST_INIT_REGISTER_NAME = 2,
    NET_GUEST_INIT_WAIT_NAME = 3,
    NET_GUEST_INIT_RECEIVE = 4
H1_ENUM_CONST_END(NetGuestInitStage)

// WaitForHost/WaitForGuest stages: wait for (or listen for) the session, then
// read the host's guest count or watch the session become active.
H1_ENUM_CONST_BEGIN(NetSessionWaitStage)
    NET_WAIT_SESSION = 0,
    NET_WAIT_CONNECTED = 1
H1_ENUM_CONST_END(NetSessionWaitStage)

// WaitForDirectConnect's stages (WFDCStage): make the six-digit ID, exchange
// "ID<id>_<stage>" packets until both sides reach stage 2, then drain.
H1_ENUM_CONST_BEGIN(DirectConnectStage)
    DIRECT_CONNECT_MAKE_ID = 0,
    DIRECT_CONNECT_EXCHANGE_ID = 1,
    DIRECT_CONNECT_DRAIN = 2,
    // "ID", six digits, '_' and the stage digit.
    DIRECT_CONNECT_ID_PACKET_LENGTH = 10
H1_ENUM_CONST_END(DirectConnectStage)

#endif
