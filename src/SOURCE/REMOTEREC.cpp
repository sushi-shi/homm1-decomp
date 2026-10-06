#include <H1/Ints.h>

#include <SOURCE/remoteRecords.h>

#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/gameTypes.h>
#include <SOURCE/saveRecords.h>

#include <stdio.h>
#include <string.h>

// The records' sizes follow from the original's packed structures; the
// payloads must fit a message, and a message the packet header's length byte.
H1_STATIC_ASSERT(REMOTE_MESSAGE_HEADER_RECORD_SIZE == 1 + 4 + 1 + 1 + 2, "message header");
H1_STATIC_ASSERT(REMOTE_PACKET_HEADER_RECORD_SIZE == 1 + 1 + 1 + 1 + 2, "packet header");
H1_STATIC_ASSERT(REMOTE_MESSAGE_MAX_SIZE < REMOTE_MESSAGE_SIZE, "a message's length fits a byte");
H1_STATIC_ASSERT(sizeof(((RemotePayload*)0)->data) > REMOTE_PAYLOAD_MAX_SIZE, "payload buffer");
H1_STATIC_ASSERT(REMOTE_SETUP_RECORD_SIZE == GAME_PLAYER_COUNT, "setup table");
H1_STATIC_ASSERT(REMOTE_SAVE_HEADER_RECORD_SIZE == REMOTE_SAVE_HEADER_SIZE, "save header");
H1_STATIC_ASSERT(REMOTE_SAVE_INDEX_RECORD_SIZE == REMOTE_SAVE_INDEX_SIZE, "save index");
H1_STATIC_ASSERT(REMOTE_SAVE_ACK_RECORD_SIZE == REMOTE_SAVE_ACK_MAP_SIZE, "save acknowledgement");
H1_STATIC_ASSERT(REMOTE_SAVE_ACK_RECORD_SIZE >= REMOTE_SAVE_BATCH_SIZE, "a flag per segment");
H1_STATIC_ASSERT(COMBAT_ACTION_RECORD_SIZE == 4 * 4, "combat action");
H1_STATIC_ASSERT(
    COMBAT_REMOTE_RECORD_SIZE
        == 8 + 4 + 4 + 4 + 1 + 4 + 2 * ARMY_GROUP_RECORD_SIZE + TOWN_RECORD_SIZE,
    "combat hand-off"
);
H1_STATIC_ASSERT(COMBAT_REMOTE_HERO_RECORD_SIZE == 1 + HERO_RECORD_SIZE, "hero hand-off");
H1_STATIC_ASSERT(COMBAT_REMOTE_HERO_RECORD_SIZE <= COMBAT_REMOTE_BUFFER_SIZE, "hand-off buffer");
H1_STATIC_ASSERT(COMBAT_REMOTE_HERO_RECORD_SIZE <= REMOTE_PAYLOAD_MAX_SIZE, "hero fits");
H1_STATIC_ASSERT(COMBAT_REMOTE_RECORD_SIZE <= REMOTE_PAYLOAD_MAX_SIZE, "battle fits");
H1_STATIC_ASSERT(
    REMOTE_SAVE_INDEX_RECORD_SIZE + REMOTE_SAVE_SEGMENT_SIZE <= REMOTE_PAYLOAD_MAX_SIZE,
    "save segment fits"
);
H1_STATIC_ASSERT(REMOTE_SAVE_ACK_RECORD_SIZE <= REMOTE_PAYLOAD_MAX_SIZE, "acknowledgement fits");
H1_STATIC_ASSERT(MODEM_ID_RECORD_SIZE == 2 + MODEM_ID_DIGITS + 1 + 1, "modem identification");
H1_STATIC_ASSERT(MODEM_ID_RECORD_SIZE == DIRECT_CONNECT_ID_PACKET_LENGTH, "modem id length");

const char gNetbiosGroupName[NETBIOS_GROUP_NAME_SIZE + 1] = "Empire Too ";

// ------------------------------------------------------------ framing

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

bool WriteRemoteMessage(RecordWriter& out, const RemoteMessage& message) {
    if (message.payloadSize < 0 || message.payloadSize > REMOTE_PAYLOAD_MAX_SIZE)
        return false;
    out.Put(message.sender);
    out.Put(message.id);
    out.Put(message.type);
    out.Put(message.command);
    out.Put(message.payloadSize);
    out.Bytes(message.payload.data, message.payloadSize);
    return true;
}

bool ReadRemoteMessage(RecordReader& in, RemoteMessage& message) {
    in.Get(message.sender);
    message.id = in.GetI32();
    in.Get(message.type);
    in.Get(message.command);
    message.payloadSize = in.GetI16();
    memset(message.payload.data, 0, sizeof(message.payload.data));
    if (!in.Ok() || message.payloadSize < 0 || message.payloadSize > REMOTE_PAYLOAD_MAX_SIZE
        || message.payloadSize != in.Remaining()) {
        message.payloadSize = 0;
        return false;
    }
    in.Bytes(message.payload.data, message.payloadSize);
    return in.Ok();
}

i32 EncodeRemotePacket(
    u8* packet,
    i32 capacity,
    i8 source,
    i8 destination,
    u8 sequence,
    const RemoteMessage& message
) {
    RecordWriter body;
    RecordWriter header;
    u16 crc;

    if (!WriteRemoteMessage(body, message))
        return 0;
    if (REMOTE_PACKET_HEADER_RECORD_SIZE + body.Size() > capacity)
        return 0;
    header.Put(source);
    header.Put(destination);
    header.Put(sequence);
    header.Put(static_cast<u8>(body.Size()));
    header.Put(static_cast<u16>(0));
    crc = 0;
    calc_crc(&crc, const_cast<u8*>(header.Data()), header.Size());
    calc_crc(&crc, const_cast<u8*>(body.Data()), body.Size());
    memcpy(packet, header.Data(), REMOTE_PACKET_HEADER_RECORD_SIZE - 2);
    packet[REMOTE_PACKET_HEADER_RECORD_SIZE - 2] = static_cast<u8>(crc & 0xff);
    packet[REMOTE_PACKET_HEADER_RECORD_SIZE - 1] = static_cast<u8>(crc >> 8);
    memcpy(packet + REMOTE_PACKET_HEADER_RECORD_SIZE, body.Data(), body.Size());
    return REMOTE_PACKET_HEADER_RECORD_SIZE + body.Size();
}

bool ReadRemotePacketHeader(const u8* packet, i32 length, RemotePacketHeader& header) {
    RecordReader in(packet, length);
    in.Get(header.source);
    in.Get(header.destination);
    in.Get(header.sequence);
    in.Get(header.payloadSize);
    header.crc = in.GetU16();
    return in.Ok() && in.Remaining() >= header.payloadSize;
}

bool DecodeRemotePacket(
    const u8* packet,
    i32 length,
    const RemotePacketHeader& header,
    RemoteMessage& message
) {
    u8 zeros[2] = {0, 0};
    u16 crc = 0;

    if (length < REMOTE_PACKET_HEADER_RECORD_SIZE + header.payloadSize)
        return false;
    calc_crc(&crc, const_cast<u8*>(packet), REMOTE_PACKET_HEADER_RECORD_SIZE - 2);
    calc_crc(&crc, zeros, 2);
    calc_crc(&crc, const_cast<u8*>(packet + REMOTE_PACKET_HEADER_RECORD_SIZE), header.payloadSize);
    if (crc != header.crc)
        return false;
    RecordReader in(packet + REMOTE_PACKET_HEADER_RECORD_SIZE, header.payloadSize);
    return ReadRemoteMessage(in, message);
}

RecordReader RemotePayloadReader(const RemoteMessage& message) {
    i32 size = message.payloadSize;
    if (size < 0 || size > REMOTE_PAYLOAD_MAX_SIZE)
        size = 0;
    return RecordReader(reinterpret_cast<const u8*>(message.payload.data), size);
}

// ------------------------------------------------------------ payloads

void WriteRemoteSetup(RecordWriter& out, const i8* gamePosToNetPos) {
    out.Put(gamePosToNetPos, REMOTE_SETUP_RECORD_SIZE);
}

void ReadRemoteSetup(RecordReader& in, i8* gamePosToNetPos) {
    in.Get(gamePosToNetPos, REMOTE_SETUP_RECORD_SIZE);
}

void WriteRemoteSaveHeader(RecordWriter& out, const RemoteSaveHeader& header) {
    out.Put(header.saveSize);
    out.Put(header.playerExited);
}

void ReadRemoteSaveHeader(RecordReader& in, RemoteSaveHeader& header) {
    header.saveSize = in.GetI32();
    header.playerExited = in.GetI32();
}

void WriteRemoteSaveIndex(RecordWriter& out, i32 index) {
    out.Put(static_cast<i16>(index));
}

i32 ReadRemoteSaveIndex(RecordReader& in) {
    return in.GetI16();
}

void WriteRemoteSaveAck(RecordWriter& out, const char* flags) {
    out.Put(flags, REMOTE_SAVE_BATCH_SIZE);
    out.Zeros(REMOTE_SAVE_ACK_RECORD_SIZE - REMOTE_SAVE_BATCH_SIZE);
}

void ReadRemoteSaveAck(RecordReader& in, char* flags) {
    in.Get(flags, REMOTE_SAVE_BATCH_SIZE);
    in.Skip(REMOTE_SAVE_ACK_RECORD_SIZE - REMOTE_SAVE_BATCH_SIZE);
}

void WriteRemotePlayerExit(RecordWriter& out, const RemotePlayerExit& record) {
    out.Put(record.position);
    out.Put(record.hadControl);
    out.Put(record.nextPlayer);
}

void ReadRemotePlayerExit(RecordReader& in, RemotePlayerExit& record) {
    in.Get(record.position);
    in.Get(record.hadControl);
    in.Get(record.nextPlayer);
}

void WriteCombatRemoteAction(RecordWriter& out, const CombatRemoteAction& action) {
    out.Put(action.nextAction);
    out.Put(action.nextActionExtra);
    out.Put(action.nextActionGridIndex);
    out.Put(action.nextActionGridIndex2);
}

void ReadCombatRemoteAction(RecordReader& in, CombatRemoteAction& action) {
    action.nextAction = in.GetI32();
    action.nextActionExtra = in.GetI32();
    action.nextActionGridIndex = in.GetI32();
    action.nextActionGridIndex2 = in.GetI32();
}

void WriteCombatRemoteData(RecordWriter& out, const combatRemoteData& record) {
    out.Put(record.fragment);
    out.Put(record.x);
    out.Put(record.y);
    out.Put(record.hasFirstHero);
    out.Put(record.hasTown);
    out.Put(record.hasSecondHero);
    out.Put(record.setupCombatX);
    out.Put(record.setupCombatY);
    out.Put(record.randomSeed);
    out.Put(record.combatResult);
    out.Put(record.retreatWin);
    out.Put(record.combatSurrender);
    out.Put(record.firstOwner);
    out.Put(record.firstGold);
    out.Put(record.secondOwner);
    out.Put(record.secondGold);
    WriteArmyGroup(out, record.firstArmy);
    WriteArmyGroup(out, record.secondArmy);
    WriteTown(out, record.combatTown);
}

void ReadCombatRemoteData(RecordReader& in, combatRemoteData& record) {
    in.Get(record.fragment);
    in.Get(record.x);
    in.Get(record.y);
    in.Get(record.hasFirstHero);
    in.Get(record.hasTown);
    in.Get(record.hasSecondHero);
    in.Get(record.setupCombatX);
    in.Get(record.setupCombatY);
    record.randomSeed = in.GetI32();
    in.Get(record.combatResult);
    in.Get(record.retreatWin);
    in.Get(record.combatSurrender);
    in.Get(record.firstOwner);
    record.firstGold = in.GetI32();
    in.Get(record.secondOwner);
    record.secondGold = in.GetI32();
    ReadArmyGroup(in, record.firstArmy);
    ReadArmyGroup(in, record.secondArmy);
    ReadTown(in, record.combatTown);
}

void WriteCombatRemoteHero(RecordWriter& out, i8 fragment, const hero& record) {
    out.Put(fragment);
    WriteHero(out, record);
}

i8 ReadCombatRemoteFragment(RecordReader& in) {
    i8 fragment;
    in.Get(fragment);
    return fragment;
}

// ------------------------------------------------------------ handshakes

void WriteRemoteGuestCount(RecordWriter& out, i32 guests) {
    out.Put(static_cast<i8>(guests));
    out.Zeros(REMOTE_GUEST_COUNT_RECORD_SIZE - 1);
}

i32 ReadRemoteGuestCount(RecordReader& in) {
    i8 guests;
    in.Get(guests);
    in.Skip(REMOTE_GUEST_COUNT_RECORD_SIZE - 1);
    return guests;
}

void WriteNetbiosAnnounce(RecordWriter& out, const u8* name) {
    out.Put(gNetbiosGroupName, NETBIOS_GROUP_NAME_SIZE);
    out.Put(name, NETBIOS_NAME_RECORD_SIZE);
}

bool ReadNetbiosAnnounce(RecordReader& in, u8* name) {
    char group[NETBIOS_GROUP_NAME_SIZE];
    in.Get(group, NETBIOS_GROUP_NAME_SIZE);
    in.Get(name, NETBIOS_NAME_RECORD_SIZE);
    return in.Ok() && memcmp(group, gNetbiosGroupName, NETBIOS_GROUP_NAME_SIZE) == 0;
}

void WriteModemId(RecordWriter& out, const char* id, i32 stage) {
    char text[32];
    sprintf(text, "ID%.6s_%i", id, stage);
    out.Bytes(text, static_cast<i32>(strlen(text)));
}

bool ReadModemId(RecordReader& in, char* id, i32& stage) {
    char text[MODEM_ID_RECORD_SIZE];
    if (in.Remaining() != MODEM_ID_RECORD_SIZE)
        return false;
    in.Get(text, MODEM_ID_RECORD_SIZE);
    if (!in.Ok() || text[0] != 'I' || text[1] != 'D')
        return false;
    memcpy(id, text + 2, MODEM_ID_DIGITS);
    stage = text[MODEM_ID_RECORD_SIZE - 1] - '0';
    return true;
}
