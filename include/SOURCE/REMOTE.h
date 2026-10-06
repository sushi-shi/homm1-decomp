#ifndef HOMM1_SOURCE_REMOTE_H
#define HOMM1_SOURCE_REMOTE_H

enum RemoteMessageType {
    REMOTE_MESSAGE_DEFAULT = -1,
    REMOTE_MESSAGE_NONE = 0,
    REMOTE_MESSAGE_CONFIRM = 1,
    REMOTE_MESSAGE_RELIABLE = 2,
    REMOTE_MESSAGE_UNRELIABLE = 3,
    REMOTE_MESSAGE_HEARTBEAT = 4
};

enum RemoteBoxCommand {
BOX_REMOTE_SAVE = 1,
    BOX_REMOTE_SETUP = 0x1f };

    enum RemoteCommand { REMOTE_COMMAND_SAVE_INIT_RESPONSE = 2,
    REMOTE_COMMAND_SAVE_DATA = 3, REMOTE_COMMAND_SAVE_ACK_REQUEST = 4,
    REMOTE_COMMAND_SAVE_ACK_RESPONSE = 5, REMOTE_COMMAND_SAVE_FINISH = 6, REMOTE_COMMAND_CHAT = 11,
    REMOTE_COMMAND_HERO_TOWN_DATA = 0x15, REMOTE_COMMAND_HERO_TOWN_CONFIRM = 0x16,
    REMOTE_COMMAND_COMBAT_ACTION = 0x17,
    REMOTE_COMMAND_PLAYER_EXIT = 30 };

        enum RemoteConstant {
    REMOTE_BROADCAST_PLAYER = 0x7f,
    REMOTE_MESSAGE_HEADER_SIZE = 9,
    REMOTE_MESSAGE_SIZE = 0x100,
    REMOTE_QUEUE_CAPACITY = 7,
    REMOTE_RECENT_ID_COUNT = 30,
    REMOTE_RETRY_COUNT = 7,
    REMOTE_CONFIRM_POLL_COUNT = 200,
    REMOTE_NET_NAME_LAST = 10,
    REMOTE_PLAYER_COUNT = 2,
    REMOTE_WAIT_TIMEOUT = 20000
};

enum RemotePlayerExitField {
    REMOTE_PLAYER_EXIT_POSITION = 0,
    REMOTE_PLAYER_EXIT_HAD_CONTROL = 1,
    REMOTE_PLAYER_EXIT_NEXT_PLAYER = 2,
    REMOTE_PLAYER_EXIT_PAYLOAD_SIZE = 3
};

enum RemoteGameMode {
    REMOTE_GAME_NONE = 0,
    REMOTE_GAME_NETWORK_HOST = 1,
    REMOTE_GAME_NETWORK_GUEST = 2,
    REMOTE_GAME_MODEM_HOST = 3,
    REMOTE_GAME_MODEM_GUEST = 4,
    REMOTE_GAME_UNSET = 10
};

enum MultiplayerBaseType {
    MULTIPLAYER_BASE_MODEM = 0,
    MULTIPLAYER_BASE_NETWORK = 1,
    MULTIPLAYER_BASE_HOT_SEAT = 2,
    MULTIPLAYER_BASE_UNSET = 10
};

#define REMOTE_SAVE_ENCODED()                                                                      \
    (!gMapBaseType                                            \
     || (gMapBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))

enum RemoteDriverType {
    REMOTE_DRIVER_SERIAL = 0,
    REMOTE_DRIVER_NETBIOS = 1
};

#pragma pack(push, 1)
struct RemotePacketHeader {
    i8 source;
    i8 destination;
    u8 sequence;
    u8 payloadSize;
    u16 crc;
};
#pragma pack(pop)

#define REMOTE_PACKET(buffer) (reinterpret_cast<RemotePacketHeader*>(buffer))
#define REMOTE_MESSAGE(buffer) (reinterpret_cast<RemoteMessage*>(buffer))

#pragma pack(push, 1)
struct CombatRemoteAction {
    i32 nextAction;
    i32 nextActionExtra;
    i32 nextActionGridIndex;
    i32 nextActionGridIndex2;
};
#pragma pack(pop)

#pragma pack(push, 1)
union RemotePayload {
    char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE];
    struct {
        i32 saveSize;
        i32 playerExited;
    };
    struct {
        i16 index;
        char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE - 2];
    } segment;
    CombatRemoteAction combatAction;
};
#pragma pack(pop)

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

extern b8 gInNetSetup;
extern i32 gIDCtr;
extern u8 gRemoteGameMode;
extern u8 gPacketSequence;
extern i32 gNetNameIndex;
extern char gPacketSend[];
extern i32 gNumNetGuests;
extern i32 gLastConfirm;
enum RemoteReceiveOrderConstant {
    REMOTE_RECEIVE_ORDER_UNSET = 999999999,
    REMOTE_QUEUE_SLOT_NONE = -1
};
extern i32 gInOrder[REMOTE_QUEUE_CAPACITY];
extern RemoteMessage gReceiveQueue[REMOTE_QUEUE_CAPACITY];
extern RemoteMessage gReceiveOut;

b32 SendRemoteData(RemoteMessage* dataToSend, u8*, i32 destination, i32 length);
b32 ReceiveRemoteData(u8*, RemoteMessage* data, i32 source);
b32 TransmitRemoteData(
    void* data,
    i32 destination,
    i32 length,
    i8 command,
    b8 reliable,
    b8 allowRetryDialog = true,
    i8 messageType = REMOTE_MESSAGE_DEFAULT,
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
void UnloadRemoteDriver(i16 networkDriver);
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

extern i32 gLastHeartbeatSend;
extern i32 gLastHeartbeatReceive;
extern RemoteMessage gSendMessage;
extern char gReceiveIn[REMOTE_MESSAGE_SIZE];
extern i32 gLastIds[REMOTE_RECENT_ID_COUNT];
extern i32 gInOrderCtr;
extern i32 gCurLastID;
extern i8 gInitNetGuestStatus;
extern i8 gWaitForHostStatus;
void PollRemote();
void RemoteMain(i32 gameMode);
i32 nbnet_init(void);

enum ModemResponseLimit {
    MODEM_RESPONSE_LAST = 79
};

enum ModemPacketControl {
    MODEM_PACKET_START = 0,
    MODEM_PACKET_END = 1,
    MODEM_PACKET_ESCAPE = 0x70
};

enum ModemPacketConstant {
    MODEM_PACKET_MAX_LENGTH = 0x100,
    MODEM_ENCODED_PACKET_SIZE = 516
};

extern i32 gLastActionTime;
extern i32 gModemCommandPos;
extern char gModemCommand[];
extern char gModemResponseLine[];
extern char gModemExpectedResponse[];
extern i32 gModemResponseLength;
extern i32 gModemResponseChar;
extern i32 gLastDialPos;
extern char gPhoneNumber[];
struct inque_t {
    i32 readPosition;
    i32 writePosition;
    char data[4096];
};
extern inque_t gModemInQueue;
extern struct outque_t gModemOutQueue;
extern i32 gBaudBits;
extern b32 gModemInEscape;
extern b32 gModemNewPacket;
extern i32 gModemPacketLength;
extern char gPacket[];
extern char gModemIdString[];
extern char gRemoteModemIdString[];
extern i32 gLastIdSendTime;
extern i32 gConnectTick;
extern i32 gRemoteConnectStage;
extern i32 gLocalConnectStage;
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
b8 GUIModemCommandExec(void);
b8 GUIModemResponseExec(void);
b32 WaitForDirectConnect(void);

enum RemoteCrcConstant {
    REMOTE_CRC_BYTE_TOP_BIT = 0x80,
    REMOTE_CRC_TOP_BIT = 0x8000,
    REMOTE_CRC_POLYNOMIAL = 0x1021
};

struct outque_t {
    i32 readPosition;
    i32 writePosition;
    char data[2048];
};

enum NetHostInitStage {
    NET_HOST_INIT_START = 0,
    NET_HOST_INIT_CHECK_NAME = 1,
    NET_HOST_INIT_REGISTER_NAME = 2,
    NET_HOST_INIT_WAIT_NAME = 3
};

enum NetGuestInitStage {
    NET_GUEST_INIT_START = 0,
    NET_GUEST_INIT_CHECK_NAME = 1,
    NET_GUEST_INIT_REGISTER_NAME = 2,
    NET_GUEST_INIT_WAIT_NAME = 3,
    NET_GUEST_INIT_RECEIVE = 4
};

enum NetSessionWaitStage {
    NET_WAIT_SESSION = 0,
    NET_WAIT_CONNECTED = 1
};

enum DirectConnectStage {
    DIRECT_CONNECT_MAKE_ID = 0,
    DIRECT_CONNECT_EXCHANGE_ID = 1,
    DIRECT_CONNECT_DRAIN = 2,
    DIRECT_CONNECT_ID_PACKET_LENGTH = 10
};

#endif
