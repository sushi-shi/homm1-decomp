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

H1_ENUM_ID_BEGIN(RemoteBoxCommand)
BOX_REMOTE_SAVE = 1,
    BOX_REMOTE_SETUP = 0x1f H1_ENUM_ID_END(RemoteBoxCommand)

    // RemoteMessage::command values (TransmitRemoteData's command argument and
    // the receivers' switches): the save-game transfer (TransmitSaveGame /
    // ReceiveSaveGame), the hero/town exchange before a networked battle, chat
    // text (PopNetBox), combat actions (ProcessNextAction) and the exit notice
    // (HandleRemote*Exit). SAVE_INIT and SETUP are the RemoteBoxCommand values.
    H1_ENUM_ID_BEGIN(RemoteCommand) REMOTE_COMMAND_SAVE_INIT_RESPONSE = 2,
    REMOTE_COMMAND_SAVE_DATA = 3, REMOTE_COMMAND_SAVE_ACK_REQUEST = 4,
    REMOTE_COMMAND_SAVE_ACK_RESPONSE = 5, REMOTE_COMMAND_SAVE_FINISH = 6, REMOTE_COMMAND_CHAT = 11,
    REMOTE_COMMAND_HERO_TOWN_DATA = 0x15, REMOTE_COMMAND_HERO_TOWN_CONFIRM = 0x16,
    REMOTE_COMMAND_COMBAT_ACTION = 0x17,
    REMOTE_COMMAND_PLAYER_EXIT = 30 H1_ENUM_ID_END(RemoteCommand)

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
    // Modem and network games connect two human players.
    REMOTE_PLAYER_COUNT = 2,
    // Milliseconds a confirmed send, the combat setup exchange or a saved-game
    // transfer waits for the other side before NormalDialog asks whether to
    // keep waiting.
    REMOTE_WAIT_TIMEOUT = 20000
H1_ENUM_CONST_END(RemoteConstant)

// REMOTE_COMMAND_PLAYER_EXIT's payload bytes: the leaving player's game
// position, whether it held the turn and, when it did, the next human player
// to take it.
H1_ENUM_CONST_BEGIN(RemotePlayerExitField)
    REMOTE_PLAYER_EXIT_POSITION = 0,
    REMOTE_PLAYER_EXIT_HAD_CONTROL = 1,
    REMOTE_PLAYER_EXIT_NEXT_PLAYER = 2,
    REMOTE_PLAYER_EXIT_PAYLOAD_SIZE = 3
H1_ENUM_CONST_END(RemotePlayerExitField)

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

// TransmitSaveGame and ReceiveSaveGame move the saved game LZHUF-encoded in
// modem games, and in network games once the other side is ready.
#define REMOTE_SAVE_ENCODED()                                                                      \
    (!H1_ENUM_ENCODE(MultiplayerBaseType, gMapBaseType)                                            \
     || (gMapBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))

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

// API-forced: gPacketSend/gPacket are byte buffers framed by this header.
#define REMOTE_PACKET(buffer) (reinterpret_cast<RemotePacketHeader*>(buffer))
// API-forced: gReceiveIn is a byte buffer read as a message record (retail
// keeps it 4-byte aligned, as an array, not 8-byte aligned like a record).
#define REMOTE_MESSAGE(buffer) (reinterpret_cast<RemoteMessage*>(buffer))

// The combat action ProcessNextAction relays to the other player and
// combatManager::Main replays (REMOTE_COMMAND_COMBAT_ACTION).
#pragma pack(push, 1)
struct CombatRemoteAction {
    i32 nextAction;
    i32 nextActionExtra;
    i32 nextActionGridIndex;
    i32 nextActionGridIndex2;
};
#pragma pack(pop)

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
    CombatRemoteAction combatAction;
};
#pragma pack(pop)

// Retail TransmitRemoteData fills sender/id/type/command/size at +0/+1/+5/+6/+7
// and copies the payload to +9; GetRemoteData copies 0x100-byte records.
#pragma pack(push, 1)
struct RemoteMessage {
    i8 sender;
    i32 id;
    H1_ENUM_STORAGE(RemoteMessageType, i8) type;
    i8 command;
    i16 payloadSize;
    RemotePayload payload;
};
#pragma pack(pop)

extern b8 gInNetSetup;
extern i32 gIDCtr;
extern H1_ENUM_STORAGE(RemoteGameMode, u8) gRemoteGameMode;
extern u8 gPacketSequence;
extern i32 gNetNameIndex;
#define gPacketSend PacketSend // spelling fixes .bss order
extern char gPacketSend[];
#define gNumNetGuests iNetGuests // spelling fixes .bss order
extern i32 gNumNetGuests;
extern i32 gLastConfirm;
// GetRemoteData returns the queued message with the lowest gInOrder stamp:
// the search starts above any stamp and with no slot selected.
H1_ENUM_CONST_BEGIN(RemoteReceiveOrderConstant)
    REMOTE_RECEIVE_ORDER_UNSET = 999999999,
    REMOTE_QUEUE_SLOT_NONE = -1
H1_ENUM_CONST_END(RemoteReceiveOrderConstant)
#define gInOrder iInOrder // spelling fixes .bss order
extern i32 gInOrder[REMOTE_QUEUE_CAPACITY];
#define gReceiveQueue rcvBuf // spelling fixes .bss order
extern RemoteMessage gReceiveQueue[REMOTE_QUEUE_CAPACITY];
#define gReceiveOut rcvBufOut // spelling fixes .bss order
extern RemoteMessage gReceiveOut;

b32 SendRemoteData(RemoteMessage* dataToSend, u8*, i32 destination, i32 length);
b32 ReceiveRemoteData(u8*, RemoteMessage* data, i32 source);
// The trailing destination flag defaults to game-position addressing.
b32 TransmitRemoteData(
    void* data,
    i32 destination,
    i32 length,
    i8 command,
    b8 reliable,
    b8 allowRetryDialog = true,
    H1_ENUM_PARAM(RemoteMessageType, i8) messageType = REMOTE_MESSAGE_DEFAULT,
    b8 gamePosDestination = true
);
RemoteMessage* GetRemoteData(b8 remove);
b32 TransmitAndWait(
    void* bytes,
    i32 destination,
    i32 length,
    i8 command,
    i8 responseCommand,
    RemoteMessage** response
);
void RemoteCleanup(void);
void UnloadRemoteDriver(H1_ENUM_PARAM(RemoteDriverType, i16) networkDriver);
i32 FileSize(char* filename);
void WriteModemPacket(char* buffer, i32 length);
char ReadPacket(void);
void calc_crc(u16* crc, u8* data, i32 length);
i32 EncodePacket(RemoteMessage* data, i8 source, i8 destination, i32 length);
b32 DecodePacket(RemoteMessage* data, i32 source);
b8 InitNetHost(void);
b8 InitNetGuest(void);
b8 WaitForHost(void);
b8 WaitForGuest(void);

// PollRemote's heartbeat clocks, timeout latch, recent-id ring and the
// incoming/outgoing message buffers.
extern i32 gLastHeartbeatSend;
extern i32 gLastHeartbeatReceive;
#define gSendMessage sndBuf // spelling fixes .bss order
extern RemoteMessage gSendMessage;
#define gReceiveIn rcvBufIn // spelling fixes .bss order
extern char gReceiveIn[REMOTE_MESSAGE_SIZE];
#define gLastIds iLastIds // spelling fixes .bss order
extern i32 gLastIds[REMOTE_RECENT_ID_COUNT];
extern i32 gInOrderCtr;
extern i32 gCurLastID;
// The network setup's host/guest handshake states.
extern i8 gInitNetGuestStatus;
extern i8 gWaitForHostStatus;
void PollRemote();
// REMOTE.cpp defines the transport bring-up.
void RemoteMain(H1_ENUM_PARAM(RemoteGameMode, i32) gameMode);
i32 nbnet_init(void);

// GUIModemResponseExec stops collecting a response line at this buffer
// position: an extent of GUIMRresponse, not a value domain.
H1_ENUM_CONST_BEGIN(ModemResponseLimit)
    MODEM_RESPONSE_LAST = 79
H1_ENUM_CONST_END(ModemResponseLimit)

// WriteModemPacket frames a packet as ESCAPE START ... ESCAPE END, doubling
// an ESCAPE byte inside it; ReadPacket undoes it.
// The bytes travel in the raw modem stream, so both ends encode and decode
// them at the buffer.
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

#define gLastActionTime iLastActionTime // spelling fixes .bss order
extern i32 gLastActionTime;
#define gModemCommandPos iModemCommandPos // spelling fixes .bss order
extern i32 gModemCommandPos;
extern char gModemCommand[];
#define gModemResponseLine GUIMRresponse // spelling fixes .bss order
extern char gModemResponseLine[];
#define gModemExpectedResponse GUIMRresp // spelling fixes .bss order
extern char gModemExpectedResponse[];
#define gModemResponseLength GUIMRrespptr // spelling fixes .bss order
extern i32 gModemResponseLength;
#define gModemResponseChar GUIMRc // spelling fixes .bss order
extern i32 gModemResponseChar;
#define gLastDialPos iLastDialPos // spelling fixes .bss order
extern i32 gLastDialPos;
#define gPhoneNumber numbuf // spelling fixes .bss order
extern char gPhoneNumber[];
struct inque_t {
    i32 readPosition;
    i32 writePosition;
    char data[4096];
};
#define gModemInQueue inque // spelling fixes .bss order
extern inque_t gModemInQueue;
// The 2K transmit queue.
#define gModemOutQueue outque // spelling fixes .bss order
extern struct outque_t gModemOutQueue;
extern i32 gBaudBits;
extern b32 gModemInEscape;
extern b32 gModemNewPacket;
extern i32 gModemPacketLength;
#define gPacket packet // spelling fixes .bss order
extern char gPacket[];
#define gModemIdString idstr // spelling fixes .bss order
extern char gModemIdString[];
#define gRemoteModemIdString remoteidstr // spelling fixes .bss order
extern char gRemoteModemIdString[];
#define gLastIdSendTime oldsec // spelling fixes .bss order
extern i32 gLastIdSendTime;
#define gConnectTick stime // spelling fixes .bss order
extern i32 gConnectTick;
#define gRemoteConnectStage remotestage // spelling fixes .bss order
extern i32 gRemoteConnectStage;
#define gLocalConnectStage localstage // spelling fixes .bss order
extern i32 gLocalConnectStage;
#define gDirectConnectStage WFDCStage // spelling fixes .bss order
extern i32 gDirectConnectStage;

void GUIModemCommand(char* message, char* command);
void ModemCommand(char* command);
void ModemSetup(void);
i32 Dial(void);
i32 Wait(void);
void Connect(void);
i8 GUIModemResponse(char* message, char* response);
i32 write_buffer(char* buffer, i32 length);
i32 read_byte(void);
// The modem wait-loop steps that KB's WaitHandler drives.
b8 GUIModemCommandExec(void);
b8 GUIModemResponseExec(void);
b32 WaitForDirectConnect(void);

// CRC-16/CCITT over the packet bytes, most significant bit first.
H1_ENUM_CONST_BEGIN(RemoteCrcConstant)
    REMOTE_CRC_BYTE_TOP_BIT = 0x80,
    REMOTE_CRC_TOP_BIT = 0x8000,
    REMOTE_CRC_POLYNOMIAL = 0x1021
H1_ENUM_CONST_END(RemoteCrcConstant)

// The modem's 2K transmit queue.
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

// WaitForDirectConnect's stages (gDirectConnectStage): make the six-digit ID, exchange
// "ID<id>_<stage>" packets until both sides reach stage 2, then drain.
H1_ENUM_CONST_BEGIN(DirectConnectStage)
    DIRECT_CONNECT_MAKE_ID = 0,
    DIRECT_CONNECT_EXCHANGE_ID = 1,
    DIRECT_CONNECT_DRAIN = 2,
    // "ID", six digits, '_' and the stage digit.
    DIRECT_CONNECT_ID_PACKET_LENGTH = 10
H1_ENUM_CONST_END(DirectConnectStage)

#endif
