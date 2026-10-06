// Network and serial message codecs: every message and payload encodes to the
// original's wire layout (checked against bytes computed independently of the
// codecs), decodes back to the same values, and refuses truncated, oversized
// or corrupted input. The edition's protocol (REMOTE_PROTOCOL_VERSION 1)
// differs from the original game's in the packet checksum's seed, the serial
// identification tag, the NetBIOS group name and the native session frame;
// each refuses the original game's.

#include <H1/Ints.h>

#include <PLATFORM/Records.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/game.h>
#include <SOURCE/remoteRecords.h>
#include <SOURCE/saveRecords.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// The record classes' constructors live in game units this test does not
// link; the codecs need only the storage.
armyGroup::armyGroup(void) {}
hero::hero(void) {}
town::town(void) {}

namespace {

int gFailures = 0;

void Expect(bool condition, const std::string& what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what.c_str());
        gFailures++;
    }
}

std::vector<u8> Bytes(const RecordWriter& writer) {
    return std::vector<u8>(writer.Data(), writer.Data() + writer.Size());
}

std::vector<u8> Hex(const char* text) {
    std::vector<u8> bytes;
    for (size_t i = 0; text[i] && text[i + 1]; i += 2) {
        unsigned value = 0;
        std::sscanf(text + i, "%2x", &value);
        bytes.push_back(static_cast<u8>(value));
    }
    return bytes;
}

template <class Record>
void Fill(Record& record, u8 seed) {
    u8* bytes = reinterpret_cast<u8*>(&record);
    for (size_t i = 0; i < sizeof(Record); i++)
        bytes[i] = static_cast<u8>(seed + i * 7);
}

// A payload codec: its size, a round trip to the same bytes, and refusal of
// one byte less.
template <class Record>
void CheckPayload(
    const char* name,
    int size,
    void (*write)(RecordWriter&, const Record&),
    void (*read)(RecordReader&, Record&),
    const Record& original
) {
    RecordWriter first;
    write(first, original);
    Expect(first.Size() == size, std::string(name) + ": encoded size");
    Record decoded;
    Fill(decoded, 0x5a);
    RecordReader in(first.Data(), first.Size());
    read(in, decoded);
    Expect(in.Ok() && in.Remaining() == 0, std::string(name) + ": decodes exactly");
    RecordWriter second;
    write(second, decoded);
    Expect(Bytes(first) == Bytes(second), std::string(name) + ": round trip");
    RecordReader truncated(first.Data(), first.Size() - 1);
    read(truncated, decoded);
    Expect(!truncated.Ok(), std::string(name) + ": truncated input is refused");
}

void WriteAction(RecordWriter& out, const CombatRemoteAction& action) {
    WriteCombatRemoteAction(out, action);
}
void ReadAction(RecordReader& in, CombatRemoteAction& action) {
    ReadCombatRemoteAction(in, action);
}

void CheckPayloads() {
    RemoteSaveHeader saveHeader = {77663, 1};
    CheckPayload("save header", REMOTE_SAVE_HEADER_RECORD_SIZE, WriteRemoteSaveHeader,
                 ReadRemoteSaveHeader, saveHeader);
    RemotePlayerExit playerExit = {1, 1, 0};
    CheckPayload("player exit", REMOTE_PLAYER_EXIT_RECORD_SIZE, WriteRemotePlayerExit,
                 ReadRemotePlayerExit, playerExit);
    CombatRemoteAction action = {3, -1, 45, 0x12345678};
    CheckPayload("combat action", COMBAT_ACTION_RECORD_SIZE, WriteAction, ReadAction, action);

    // The action's layout: four little-endian 32-bit values.
    RecordWriter actionBytes;
    WriteCombatRemoteAction(actionBytes, action);
    Expect(Bytes(actionBytes) == Hex("03000000ffffffff2d00000078563412"), "combat action bytes");

    // The battle hand-off: 25 bytes of scalars, the two armies and the town
    // in their file record layout.
    combatRemoteData combat;
    Fill(combat, 0x13);
    combat.fragment = COMBAT_REMOTE_FRAGMENT_COMBAT;
    combat.randomSeed = 0x01020304;
    combat.firstGold = 7500;
    combat.secondGold = -1;
    CheckPayload("combat hand-off", COMBAT_REMOTE_RECORD_SIZE, WriteCombatRemoteData,
                 ReadCombatRemoteData, combat);
    RecordWriter combatBytes;
    WriteCombatRemoteData(combatBytes, combat);
    std::vector<u8> bytes = Bytes(combatBytes);
    Expect(bytes[0] == COMBAT_REMOTE_FRAGMENT_COMBAT && bytes[8] == 4 && bytes[11] == 1,
           "combat hand-off: seed at offset 8");
    Expect(bytes[16] == (7500 & 0xff) && bytes[17] == (7500 >> 8), "combat hand-off: gold at 16");
    Expect(bytes[21] == 0xff && bytes[24] == 0xff, "combat hand-off: second gold at 21");
    RecordWriter army;
    WriteArmyGroup(army, combat.firstArmy);
    Expect(std::memcmp(&bytes[25], army.Data(), ARMY_GROUP_RECORD_SIZE) == 0,
           "combat hand-off: first army at 25");
    RecordWriter secondArmy;
    WriteArmyGroup(secondArmy, combat.secondArmy);
    Expect(std::memcmp(&bytes[40], secondArmy.Data(), ARMY_GROUP_RECORD_SIZE) == 0,
           "combat hand-off: second army at 40");
    RecordWriter townRecord;
    WriteTown(townRecord, combat.combatTown);
    Expect(std::memcmp(&bytes[55], townRecord.Data(), TOWN_RECORD_SIZE) == 0,
           "combat hand-off: town at 55");

    // A hero fragment: the fragment byte and the hero's file record.
    hero heroRecord;
    Fill(heroRecord, 0x29);
    RecordWriter heroBytes;
    WriteCombatRemoteHero(heroBytes, COMBAT_REMOTE_FRAGMENT_SECOND_HERO, heroRecord);
    Expect(heroBytes.Size() == COMBAT_REMOTE_HERO_RECORD_SIZE, "hero hand-off: size");
    RecordWriter heroOnly;
    WriteHero(heroOnly, heroRecord);
    Expect(heroBytes.Data()[0] == COMBAT_REMOTE_FRAGMENT_SECOND_HERO
               && std::memcmp(heroBytes.Data() + 1, heroOnly.Data(), HERO_RECORD_SIZE) == 0,
           "hero hand-off: layout");
    RecordReader heroIn(heroBytes.Data(), heroBytes.Size());
    hero decodedHero;
    Expect(ReadCombatRemoteFragment(heroIn) == COMBAT_REMOTE_FRAGMENT_SECOND_HERO,
           "hero hand-off: fragment");
    ReadHero(heroIn, decodedHero);
    RecordWriter heroAgain;
    WriteHero(heroAgain, decodedHero);
    Expect(heroIn.Ok() && heroIn.Remaining() == 0, "hero hand-off: decodes exactly");
    int differences = 0;
    for (int i = 0; i < HERO_RECORD_SIZE; i++)
        differences += heroAgain.Data()[i] != heroOnly.Data()[i];
    Expect(differences <= 2, "hero hand-off: round trip (names are terminated)");
    RecordReader heroShort(heroBytes.Data(), heroBytes.Size() - 1);
    ReadCombatRemoteFragment(heroShort);
    ReadHero(heroShort, decodedHero);
    Expect(!heroShort.Ok(), "hero hand-off: truncated input is refused");

    // The setup table, segment index and acknowledgement map.
    i8 setup[GAME_PLAYER_COUNT] = {0, -1, 1, -1};
    RecordWriter setupBytes;
    WriteRemoteSetup(setupBytes, setup);
    Expect(Bytes(setupBytes) == Hex("00ff01ff"), "setup bytes");
    i8 setupDecoded[GAME_PLAYER_COUNT];
    RecordReader setupIn(setupBytes.Data(), setupBytes.Size());
    ReadRemoteSetup(setupIn, setupDecoded);
    Expect(setupIn.Ok() && std::memcmp(setup, setupDecoded, sizeof(setup)) == 0, "setup round trip");
    RecordReader setupShort(setupBytes.Data(), 3);
    ReadRemoteSetup(setupShort, setupDecoded);
    Expect(!setupShort.Ok(), "setup: truncated input is refused");

    RecordWriter segment;
    WriteRemoteSaveIndex(segment, 388);
    Expect(Bytes(segment) == Hex("8401"), "segment index bytes");
    RecordReader segmentIn(segment.Data(), segment.Size());
    Expect(ReadRemoteSaveIndex(segmentIn) == 388 && segmentIn.Ok(), "segment index round trip");
    RecordReader segmentShort(segment.Data(), 1);
    ReadRemoteSaveIndex(segmentShort);
    Expect(!segmentShort.Ok(), "segment index: truncated input is refused");

    char flags[REMOTE_SAVE_BATCH_SIZE];
    for (int i = 0; i < REMOTE_SAVE_BATCH_SIZE; i++)
        flags[i] = static_cast<char>(i % 3 == 0);
    RecordWriter ack;
    WriteRemoteSaveAck(ack, flags);
    Expect(ack.Size() == REMOTE_SAVE_ACK_RECORD_SIZE, "acknowledgement size");
    char flagsDecoded[REMOTE_SAVE_BATCH_SIZE];
    RecordReader ackIn(ack.Data(), ack.Size());
    ReadRemoteSaveAck(ackIn, flagsDecoded);
    Expect(ackIn.Ok() && ackIn.Remaining() == 0
               && std::memcmp(flags, flagsDecoded, sizeof(flags)) == 0,
           "acknowledgement round trip");
    RecordReader ackShort(ack.Data(), ack.Size() - 1);
    ReadRemoteSaveAck(ackShort, flagsDecoded);
    Expect(!ackShort.Ok(), "acknowledgement: truncated input is refused");
}

RemoteMessage Message(i8 sender, i32 id, i8 type, i8 command, const char* payload, int size) {
    RemoteMessage message;
    std::memset(&message, 0x6b, sizeof(message));
    message.sender = sender;
    message.id = id;
    message.type = type;
    message.command = command;
    message.payloadSize = static_cast<i16>(size);
    std::memcpy(message.payload.data, payload, static_cast<size_t>(size));
    return message;
}

void CheckMessages() {
    // Golden packets: the bytes the original's packed structures put on the
    // wire, with the CRC computed by an independent implementation from the
    // edition's seed (0x5401).
    Expect(REMOTE_PROTOCOL_VERSION == 1 && REMOTE_PROTOCOL_CRC_SEED == 0x5401,
           "the edition's protocol version and checksum seed");
    const char heartbeatPayload[1] = {1};
    RemoteMessage heartbeat = Message(0, 0, REMOTE_MESSAGE_HEARTBEAT, 0x12, heartbeatPayload, 1);
    u8 packet[REMOTE_PACKET_MAX_SIZE];
    i32 length = EncodeRemotePacket(packet, sizeof(packet), 0, 1, 0, heartbeat);
    Expect(std::vector<u8>(packet, packet + length) == Hex("0001000aa36f00000000000412010001"),
           "heartbeat packet bytes");

    RemoteMessage chat =
        Message(1, 200000001, REMOTE_MESSAGE_RELIABLE, REMOTE_COMMAND_CHAT, "hello", 6);
    length = EncodeRemotePacket(packet, sizeof(packet), 1, 0, 0, chat);
    Expect(std::vector<u8>(packet, packet + length)
               == Hex("0100000fff720101c2eb0b020b060068656c6c6f00"),
           "chat packet bytes");

    RemotePacketHeader header;
    RemoteMessage decoded;

    // The same packets as the original game sends them (checksum seed 0)
    // are refused.
    for (const char* original :
         {"0001000aafee00000000000412010001", "0100000f4cb50101c2eb0b020b060068656c6c6f00"}) {
        std::vector<u8> bytes = Hex(original);
        RemotePacketHeader originalHeader;
        i32 size = static_cast<i32>(bytes.size());
        Expect(ReadRemotePacketHeader(bytes.data(), size, originalHeader)
                   && !DecodeRemotePacket(bytes.data(), size, originalHeader, decoded),
               std::string("the original game's packet ") + original + " is refused");
    }
    Expect(ReadRemotePacketHeader(packet, length, header) && header.source == 1
               && header.destination == 0 && header.payloadSize == 15,
           "packet header decodes");
    Expect(DecodeRemotePacket(packet, length, header, decoded), "chat packet decodes");
    Expect(decoded.sender == 1 && decoded.id == 200000001 && decoded.type == REMOTE_MESSAGE_RELIABLE
               && decoded.command == REMOTE_COMMAND_CHAT && decoded.payloadSize == 6
               && std::strcmp(decoded.payload.data, "hello") == 0,
           "chat message round trip");

    // Damage: a flipped bit, a short packet, a length byte beyond the data,
    // a message whose own size disagrees with the packet's.
    for (int i = 0; i < length; i++) {
        u8 damaged[REMOTE_PACKET_MAX_SIZE];
        std::memcpy(damaged, packet, static_cast<size_t>(length));
        damaged[i] ^= 0x10;
        RemotePacketHeader damagedHeader;
        bool accepted = ReadRemotePacketHeader(damaged, length, damagedHeader)
                        && DecodeRemotePacket(damaged, length, damagedHeader, decoded);
        Expect(!accepted, "a damaged byte " + std::to_string(i) + " is refused");
    }
    Expect(!ReadRemotePacketHeader(packet, length - 1, header), "a short packet is refused");
    Expect(!ReadRemotePacketHeader(packet, 5, header), "a short header is refused");

    RemoteMessage inconsistent = chat;
    inconsistent.payloadSize = 5;
    RecordWriter body;
    WriteRemoteMessage(body, inconsistent);
    body.Put('!');
    RecordReader bodyIn(body.Data(), body.Size());
    Expect(!ReadRemoteMessage(bodyIn, decoded), "a payload size that disagrees is refused");

    // The largest message fits the length byte; one byte more is refused.
    char big[REMOTE_PAYLOAD_MAX_SIZE + 1];
    std::memset(big, 'x', sizeof(big));
    RemoteMessage largest =
        Message(0, 7, REMOTE_MESSAGE_RELIABLE, REMOTE_COMMAND_CHAT, big, REMOTE_PAYLOAD_MAX_SIZE);
    length = EncodeRemotePacket(packet, sizeof(packet), 0, 1, 0, largest);
    Expect(length == REMOTE_PACKET_MAX_SIZE, "the largest message is encoded");
    Expect(ReadRemotePacketHeader(packet, length, header)
               && DecodeRemotePacket(packet, length, header, decoded)
               && decoded.payload.data[REMOTE_PAYLOAD_MAX_SIZE] == 0,
           "the largest message decodes, terminated");
    RemoteMessage tooBig = largest;
    tooBig.payloadSize = REMOTE_PAYLOAD_MAX_SIZE + 1;
    Expect(EncodeRemotePacket(packet, sizeof(packet), 0, 1, 0, tooBig) == 0,
           "an oversized message is refused");
    Expect(EncodeRemotePacket(packet, 20, 0, 1, 0, chat) == 0, "a small buffer is refused");

    // A received payload is readable through its reader, bounded by its size.
    RecordReader payload = RemotePayloadReader(chat);
    Expect(payload.Remaining() == 6, "payload reader size");
}

void CheckHandshakes() {
    RecordWriter guests;
    WriteRemoteGuestCount(guests, 1);
    Expect(Bytes(guests) == Hex("010000"), "guest count bytes");
    RecordReader guestsIn(guests.Data(), guests.Size());
    Expect(ReadRemoteGuestCount(guestsIn) == 1 && guestsIn.Ok(), "guest count round trip");
    RecordReader guestsShort(guests.Data(), 2);
    ReadRemoteGuestCount(guestsShort);
    Expect(!guestsShort.Ok(), "guest count: truncated input is refused");

    u8 name[NETBIOS_NAME_RECORD_SIZE] = {'H', 'H', 'O', 'S', 'T', '0', ' ', ' ',
                                         ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0};
    RecordWriter announce;
    WriteNetbiosAnnounce(announce, name);
    Expect(announce.Size() == NETBIOS_ANNOUNCE_RECORD_SIZE
               && std::memcmp(announce.Data(), "Empire TE1 HHOST0", 17) == 0,
           "announcement bytes");
    u8 decodedName[NETBIOS_NAME_RECORD_SIZE];
    RecordReader announceIn(announce.Data(), announce.Size());
    Expect(ReadNetbiosAnnounce(announceIn, decodedName)
               && std::memcmp(name, decodedName, sizeof(name)) == 0,
           "announcement round trip");
    RecordReader announceShort(announce.Data(), announce.Size() - 1);
    Expect(!ReadNetbiosAnnounce(announceShort, decodedName), "announcement: truncated is refused");
    std::vector<u8> other = Bytes(announce);
    other[0] = 'e';
    RecordReader otherIn(other.data(), static_cast<i32>(other.size()));
    Expect(!ReadNetbiosAnnounce(otherIn, decodedName), "another group's datagram is refused");
    std::vector<u8> originalGroup = Bytes(announce);
    std::memcpy(originalGroup.data(), "Empire Too ", NETBIOS_GROUP_NAME_SIZE);
    RecordReader originalGroupIn(originalGroup.data(), static_cast<i32>(originalGroup.size()));
    Expect(!ReadNetbiosAnnounce(originalGroupIn, decodedName),
           "the original game's host announcement is refused");

    RecordWriter modemId;
    WriteModemId(modemId, "123456", 1);
    Expect(Bytes(modemId) == std::vector<u8>({'T', 'E', '1', '2', '3', '4', '5', '6', '_', '1'}),
           "modem identification bytes");
    char id[MODEM_ID_DIGITS];
    i32 stage = -1;
    RecordReader modemIn(modemId.Data(), modemId.Size());
    Expect(ReadModemId(modemIn, id, stage) && std::memcmp(id, "123456", 6) == 0 && stage == 1,
           "modem identification round trip");
    RecordReader modemShort(modemId.Data(), modemId.Size() - 1);
    Expect(!ReadModemId(modemShort, id, stage), "modem identification: truncated is refused");
    const u8 originalId[] = {'I', 'D', '1', '2', '3', '4', '5', '6', '_', '1'};
    RecordReader originalIdIn(originalId, sizeof(originalId));
    Expect(!ReadModemId(originalIdIn, id, stage),
           "the original game's modem identification is refused");
}

// The native TCP transport's session frame carries the protocol version; a
// program of another version (the frames before the version byte were the
// original game's protocol) is refused.
void CheckSessionFrames() {
    u8 called[NETBIOS_NAME_RECORD_SIZE] = {'*'};
    u8 calling[NETBIOS_NAME_RECORD_SIZE] = {'H', 'G', 'U', 'E', 'S', 'T', '1'};
    RecordWriter call;
    WriteNetbiosSession(call, NETBIOS_FRAME_CALL, called, calling);
    std::vector<u8> bytes = Bytes(call);
    Expect(bytes.size() == static_cast<size_t>(NETBIOS_SESSION_RECORD_SIZE)
               && std::memcmp(bytes.data(), "H1NB\x02\x01\x01*", 8) == 0,
           "session frame bytes");
    u8 kind = 0;
    u8 first[NETBIOS_NAME_RECORD_SIZE];
    u8 second[NETBIOS_NAME_RECORD_SIZE];
    RecordReader callIn(bytes.data(), static_cast<i32>(bytes.size()));
    Expect(ReadNetbiosSession(callIn, kind, first, second) == NETBIOS_SESSION_VALID
               && kind == NETBIOS_FRAME_CALL && std::memcmp(first, called, sizeof(called)) == 0
               && std::memcmp(second, calling, sizeof(calling)) == 0,
           "session frame round trip");

    // A native program without the edition: "H1NB", frame version 1, kind,
    // two names.
    std::vector<u8> original = {'H', '1', 'N', 'B', 1, NETBIOS_FRAME_CALL};
    original.insert(original.end(), called, called + sizeof(called));
    original.insert(original.end(), calling, calling + sizeof(calling));
    RecordReader originalIn(original.data(), static_cast<i32>(original.size()));
    Expect(ReadNetbiosSession(originalIn, kind, first, second) == NETBIOS_SESSION_OTHER_PROTOCOL,
           "the original game's session frame is another protocol");

    std::vector<u8> otherProtocol = bytes;
    otherProtocol[5] = 0;
    RecordReader otherIn(otherProtocol.data(), static_cast<i32>(otherProtocol.size()));
    Expect(ReadNetbiosSession(otherIn, kind, first, second) == NETBIOS_SESSION_OTHER_PROTOCOL,
           "a session frame of protocol 0 is another protocol");

    std::vector<u8> badMagic = bytes;
    badMagic[0] = 'X';
    RecordReader badMagicIn(badMagic.data(), static_cast<i32>(badMagic.size()));
    Expect(ReadNetbiosSession(badMagicIn, kind, first, second) == NETBIOS_SESSION_MALFORMED,
           "a frame that is no session frame is refused");
    RecordReader shortIn(bytes.data(), static_cast<i32>(bytes.size()) - 1);
    Expect(ReadNetbiosSession(shortIn, kind, first, second) == NETBIOS_SESSION_MALFORMED,
           "a truncated session frame is refused");
}

} // namespace

int main() {
    CheckPayloads();
    CheckMessages();
    CheckHandshakes();
    CheckSessionFrames();
    if (gFailures) {
        std::fprintf(stderr, "%d failures\n", gFailures);
        return 1;
    }
    std::printf("ok: network message codecs\n");
    return 0;
}
