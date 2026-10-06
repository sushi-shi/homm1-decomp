// The network and serial message codecs (src/SOURCE/REMOTEREC.cpp) on
// arbitrary bytes: the input is decoded as a received packet (header, CRC,
// message) and, as a message payload, through every payload decoder; each
// decoded record is encoded again and must decode to the same encoding, and a
// decoded message must survive its own packet.

#include "FuzzSupport.h"

#include <PLATFORM/Records.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/remoteRecords.h>
#include <SOURCE/saveRecords.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// The record classes' constructors live in game units this harness does not
// link; the codecs need only the storage.
armyGroup::armyGroup(void) {}
hero::hero(void) {}
town::town(void) {}

namespace {

[[noreturn]] void Fail(const char* what) {
    std::fprintf(stderr, "fuzz_remote: %s\n", what);
    std::abort();
}

std::vector<u8> Bytes(const RecordWriter& out) {
    return std::vector<u8>(out.Data(), out.Data() + out.Size());
}

// Decodes with read, encodes with write, then decodes and encodes again: the
// two encodings must be equal.
template <class Record, class Read, class Write>
void Stable(const u8* data, i32 size, Read read, Write write) {
    Record first;
    std::memset(static_cast<void*>(&first), 0, sizeof(first));
    RecordReader in(data, size);
    read(in, first);
    RecordWriter once;
    write(once, first);
    Record second;
    std::memset(static_cast<void*>(&second), 0, sizeof(second));
    RecordReader again(once.Data(), once.Size());
    read(again, second);
    RecordWriter twice;
    write(twice, second);
    if (!again.Ok() || Bytes(once) != Bytes(twice))
        Fail("re-encoding is not stable");
}

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 4096)
        return 0;
    i32 length = static_cast<i32>(size);

    // As a received packet.
    RemotePacketHeader header;
    RemoteMessage message;
    std::memset(static_cast<void*>(&message), 0, sizeof(message));
    if (ReadRemotePacketHeader(data, length, header) && DecodeRemotePacket(data, length, header, message)) {
        u8 packet[REMOTE_PACKET_MAX_SIZE];
        i32 packetLength = EncodeRemotePacket(packet, sizeof(packet), header.source, header.destination,
                                              header.sequence, message);
        RemotePacketHeader decodedHeader;
        RemoteMessage decoded;
        if (packetLength == 0 || !ReadRemotePacketHeader(packet, packetLength, decodedHeader)
            || !DecodeRemotePacket(packet, packetLength, decodedHeader, decoded)
            || decoded.payloadSize != message.payloadSize
            || std::memcmp(decoded.payload.data, message.payload.data, message.payloadSize) != 0)
            Fail("a decoded message did not survive its own packet");
        if (message.payload.data[message.payloadSize] != 0)
            Fail("the payload is not followed by a zero byte");
        RecordReader payload = RemotePayloadReader(message);
        if (payload.Remaining() != message.payloadSize)
            Fail("the payload reader does not cover the payload");
    }

    // As a message of any length.
    {
        RecordReader in(data, length);
        RemoteMessage any;
        if (ReadRemoteMessage(in, any)) {
            RecordWriter out;
            if (!WriteRemoteMessage(out, any) || out.Size() != length
                || std::memcmp(out.Data(), data, size) != 0)
                Fail("a message did not encode back to its bytes");
        }
    }

    // As each payload.
    Stable<RemoteSaveHeader>(data, length, ReadRemoteSaveHeader, WriteRemoteSaveHeader);
    Stable<RemotePlayerExit>(data, length, ReadRemotePlayerExit, WriteRemotePlayerExit);
    Stable<CombatRemoteAction>(data, length, ReadCombatRemoteAction, WriteCombatRemoteAction);
    Stable<combatRemoteData>(data, length, ReadCombatRemoteData, WriteCombatRemoteData);
    {
        i8 table[REMOTE_SETUP_RECORD_SIZE];
        RecordReader in(data, length);
        ReadRemoteSetup(in, table);
        char flags[REMOTE_SAVE_BATCH_SIZE];
        RecordReader ack(data, length);
        ReadRemoteSaveAck(ack, flags);
        RecordReader index(data, length);
        ReadRemoteSaveIndex(index);
        RecordReader guests(data, length);
        ReadRemoteGuestCount(guests);
        u8 name[NETBIOS_NAME_RECORD_SIZE];
        RecordReader announce(data, length);
        ReadNetbiosAnnounce(announce, name);
        char id[MODEM_ID_DIGITS];
        i32 stage = 0;
        RecordReader modem(data, length);
        if (ReadModemId(modem, id, stage)) {
            RecordWriter out;
            WriteModemId(out, id, stage);
        }
        RecordReader fragment(data, length);
        if (ReadCombatRemoteFragment(fragment) != 0) {
            hero record;
            std::memset(static_cast<void*>(&record), 0, sizeof(record));
            ReadHero(fragment, record);
            RecordWriter out;
            WriteCombatRemoteHero(out, 1, record);
        }
    }
    return 0;
}
