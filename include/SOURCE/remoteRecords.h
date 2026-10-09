#ifndef HOMM1_SOURCE_REMOTERECORDS_H
#define HOMM1_SOURCE_REMOTERECORDS_H

// The network and serial messages, encoded field by field. The original
// copied its packet header, message header and payload structures (with the
// heroes, towns and armies inside them) to the wire whole, so the wire format
// is the byte layout of its 32-bit, byte-packed structures, little-endian.
// These codecs write and read exactly that layout from structures laid out
// however the compiler likes, so that every build - the Visual C++ 6 one, the
// native ones on any CPU - speaks the original's protocol byte for byte.
// Bytes the original sent without initializing are sent as zero.
//
// Plain C++98: the Visual C++ 6 build compiles it too.

#include <H1/Ints.h>

#include <PLATFORM/Records.h>
#include <SOURCE/REMOTE.h>

class hero;
struct combatRemoteData;

enum RemoteRecordSize {
    // source, destination, sequence, length, CRC-16
    REMOTE_PACKET_HEADER_RECORD_SIZE = 6,
    // sender, id, type, command, payload size
    REMOTE_MESSAGE_HEADER_RECORD_SIZE = REMOTE_MESSAGE_HEADER_SIZE,
    // The packet header's length byte holds the whole message, header and
    // payload, so a message has at most 255 bytes.
    REMOTE_MESSAGE_MAX_SIZE = 0xff,
    REMOTE_PAYLOAD_MAX_SIZE = REMOTE_MESSAGE_MAX_SIZE - REMOTE_MESSAGE_HEADER_SIZE,
    REMOTE_PACKET_MAX_SIZE = REMOTE_PACKET_HEADER_RECORD_SIZE + REMOTE_MESSAGE_MAX_SIZE,

    // BOX_REMOTE_SETUP: the host's game position to network position table.
    REMOTE_SETUP_RECORD_SIZE = 4,
    // BOX_REMOTE_SAVE: the save's size and whether its sender left the game.
    REMOTE_SAVE_HEADER_RECORD_SIZE = 8,
    // REMOTE_COMMAND_SAVE_DATA, _ACK_REQUEST: the segment index (then, for
    // data, up to REMOTE_SAVE_SEGMENT_SIZE bytes of the save).
    REMOTE_SAVE_INDEX_RECORD_SIZE = 2,
    // REMOTE_COMMAND_SAVE_ACK_RESPONSE: one flag per segment of a batch; the
    // original sent twice the batch, the second half uninitialized.
    REMOTE_SAVE_ACK_RECORD_SIZE = 200,
    // REMOTE_COMMAND_PLAYER_EXIT: position, had control, next player.
    REMOTE_PLAYER_EXIT_RECORD_SIZE = 3,
    // REMOTE_MESSAGE_HEARTBEAT: one byte, 1 once the sender is ready.
    REMOTE_HEARTBEAT_RECORD_SIZE = 1,
    // REMOTE_COMMAND_COMBAT_ACTION: the next combat action's four values.
    COMBAT_ACTION_RECORD_SIZE = 16,
    // REMOTE_COMMAND_HERO_TOWN_DATA, fragment 0: the battle and both armies
    // and the town.
    COMBAT_REMOTE_RECORD_SIZE = 110,
    // REMOTE_COMMAND_HERO_TOWN_DATA, fragments 1 and 2: one hero.
    COMBAT_REMOTE_HERO_RECORD_SIZE = 183,

    // The network handshake: the host tells the guest the number of guests.
    REMOTE_GUEST_COUNT_RECORD_SIZE = 3,
    // The NetBIOS host's broadcast: the group name and the host's name.
    NETBIOS_NAME_RECORD_SIZE = 16,
    NETBIOS_GROUP_NAME_SIZE = 11,
    NETBIOS_ANNOUNCE_RECORD_SIZE = NETBIOS_GROUP_NAME_SIZE + NETBIOS_NAME_RECORD_SIZE,
    // The serial handshake: "ID", six digits, "_", the connect stage.
    MODEM_ID_DIGITS = 6,
    MODEM_ID_RECORD_SIZE = 10
};

// The NetBIOS group name the original's broadcasts begin with.
extern const char gNetbiosGroupName[NETBIOS_GROUP_NAME_SIZE + 1];

// ------------------------------------------------------------ framing

// calc_crc (REMOTE.h) is defined with the codecs: the CRC-16 (polynomial
// 0x1021, bits fed most significant first, no final step) of the packets.

// A message: header and payloadSize bytes of payload. The writer refuses
// (writes nothing and returns false) a payload size outside the protocol's.
bool WriteRemoteMessage(RecordWriter& out, const RemoteMessage& message);
// Reads a message of exactly the reader's remaining length; the payload's
// size must match it. The payload buffer past the payload is zeroed, so the
// payload is always followed by a terminating zero byte.
bool ReadRemoteMessage(RecordReader& in, RemoteMessage& message);

// A packet: the packet header and a message, with the CRC computed over both
// (with the CRC field zero). Returns the packet's length, or 0 if the message
// does not fit a packet.
i32 EncodeRemotePacket(
    u8* packet,
    i32 capacity,
    i8 source,
    i8 destination,
    u8 sequence,
    const RemoteMessage& message
);
// Reads the packet header of length bytes; false if they are fewer than the
// header and its length byte say.
bool ReadRemotePacketHeader(const u8* packet, i32 length, RemotePacketHeader& header);
// Checks the CRC and decodes the message of a packet whose header was read.
bool DecodeRemotePacket(
    const u8* packet,
    i32 length,
    const RemotePacketHeader& header,
    RemoteMessage& message
);

// A reader over a received message's payload.
RecordReader RemotePayloadReader(const RemoteMessage& message);

// ------------------------------------------------------------ payloads

struct RemoteSaveHeader {
    i32 saveSize;
    b32 playerExited;
};

struct RemotePlayerExit {
    i8 position;
    i8 hadControl;
    i8 nextPlayer;
};

void WriteRemoteSetup(RecordWriter& out, const i8* gamePosToNetPos);
void ReadRemoteSetup(RecordReader& in, i8* gamePosToNetPos);
void WriteRemoteSaveHeader(RecordWriter& out, const RemoteSaveHeader& header);
void ReadRemoteSaveHeader(RecordReader& in, RemoteSaveHeader& header);
void WriteRemoteSaveIndex(RecordWriter& out, i32 index);
i32 ReadRemoteSaveIndex(RecordReader& in);
// flags: one byte per segment of the batch (REMOTE_SAVE_BATCH_SIZE).
void WriteRemoteSaveAck(RecordWriter& out, const char* flags);
void ReadRemoteSaveAck(RecordReader& in, char* flags);
void WriteRemotePlayerExit(RecordWriter& out, const RemotePlayerExit& record);
void ReadRemotePlayerExit(RecordReader& in, RemotePlayerExit& record);
void WriteCombatRemoteAction(RecordWriter& out, const CombatRemoteAction& action);
void ReadCombatRemoteAction(RecordReader& in, CombatRemoteAction& action);
void WriteCombatRemoteData(RecordWriter& out, const combatRemoteData& record);
void ReadCombatRemoteData(RecordReader& in, combatRemoteData& record);
void WriteCombatRemoteHero(RecordWriter& out, i8 fragment, const hero& record);
// The fragment byte every REMOTE_COMMAND_HERO_TOWN_DATA payload begins with;
// a hero fragment's hero follows it (ReadHero).
i8 ReadCombatRemoteFragment(RecordReader& in);

// ------------------------------------------------------------ handshakes

void WriteRemoteGuestCount(RecordWriter& out, i32 guests);
i32 ReadRemoteGuestCount(RecordReader& in);
// name: NETBIOS_NAME_RECORD_SIZE bytes.
void WriteNetbiosAnnounce(RecordWriter& out, const u8* name);
// False unless the datagram is the group's broadcast.
bool ReadNetbiosAnnounce(RecordReader& in, u8* name);
// id: MODEM_ID_DIGITS digits.
void WriteModemId(RecordWriter& out, const char* id, i32 stage);
// False unless the packet is an identification packet.
bool ReadModemId(RecordReader& in, char* id, i32& stage);

#endif
