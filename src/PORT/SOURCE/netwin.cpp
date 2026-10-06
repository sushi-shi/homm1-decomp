// The native counterpart of src/SOURCE/netwin.cpp: the NetBIOS calls the game
// makes (nb_init, nb_sess, nb_snd, nb_rcv, nb_stat, nb_term), over TCP.
//
// The original asked the operating system's NetBIOS for named sessions on the
// local network. The host registered HHOST0, broadcast "Empire Too " and its
// name as a datagram and listened for any caller; a guest registered
// HGUEST1, waited for that datagram and called the name it carried. Each
// session then carried the game's packets as NetBIOS messages, which keep
// their boundaries.
//
// Here a session is a TCP connection carrying length-prefixed frames, and a
// name is a TCP endpoint:
//
//   - the host listens on the port (--port, default 1995) and broadcasts the
//     original's datagram to that UDP port on the local networks;
//   - a guest given --join ADDRESS[:PORT] calls that address; without it, it
//     waits for the host's datagram and calls the address it came from;
//   - the call opens with a frame naming both sides ("H1NB", version, kind,
//     called name, calling name) and the listener answers with its own name;
//     every later frame is one NetBIOS message.
//
// The session table, the status bits the game polls (name registered,
// active, connected), MOVE, the receive queue and the two send queues keep
// the original's behaviour, as do its retries: a call that fails is retried
// 20 times, 100 ms apart. Everything happens on the game's thread: each call
// pumps the sockets, and the game's PollRemote calls nb_thr_ctl, which was the
// original's completion loop.

#include <H1/Ints.h>

#include <SOURCE/netwin.h>

#include <PLATFORM/Net.h>
#include <PLATFORM/Platform.h>
#include <PLATFORM/Records.h>
#include <SOURCE/remoteRecords.h>

#include <stdarg.h>
#include <string.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

u8 gNetbiosLana = 0;
u8 gNetbiosAvail = 0;

namespace {

using platform::net::Address;
using platform::net::Datagram;
using platform::net::Listener;
using platform::net::Stream;

// The NetBIOS return codes the game sees.
enum NetbiosReturnCode {
    NRC_GOODRET = 0x00,
    NRC_SNUMOUT = 0x08,
    NRC_NOCALL = 0x14,
    NRC_NORESOURCES = 0x38
};

// The session frames that open a TCP session.
enum SessionFrame {
    SESSION_VERSION = 1,
    SESSION_CALL = 1,
    SESSION_ACCEPT = 2,
    SESSION_REFUSE = 3,
    SESSION_MAGIC_SIZE = 4,
    CALL_RETRY_LIMIT = NETBIOS_CALL_RETRY_LIMIT,
    CALL_RETRY_DELAY = NETBIOS_CALL_RETRY_DELAY
};

const char kSessionMagic[SESSION_MAGIC_SIZE + 1] = "H1NB";

enum Pending {
    PENDING_NONE,
    PENDING_CALL,
    PENDING_LISTEN,
    PENDING_RECEIVE_ANY
};

struct Slot {
    std::unique_ptr<Stream> stream;
    Pending pending = PENDING_NONE;
    // The session is open (the original held an LSN for it).
    bool established = false;
    // The local name for the name slot, the peer's name for a session.
    u8 name[NETBIOS_NAME_RECORD_SIZE] = {};
    // A listen's or call's name: "*" is any.
    u8 wanted[NETBIOS_NAME_RECORD_SIZE] = {};
    Address callAddress;
    i32 callRetries = 0;
    u32 nextCall = 0;
};

struct Packet {
    u8 session = 0;
    std::vector<u8> data;
};

struct Netbios {
    bool open = false;
    u8 nameSlot = 0xff;
    u8 status[NETBIOS_SESSION_COUNT] = {};
    Slot slots[NETBIOS_SESSION_COUNT];
    std::unique_ptr<Listener> listener;
    bool listenFailureLogged = false;
    std::unique_ptr<Datagram> announcer;
    std::unique_ptr<Datagram> discovery;
    std::deque<Packet> received;
    std::deque<Packet> priority;
    std::deque<Packet> normal;
};

Netbios& State() {
    static Netbios state;
    return state;
}

std::string NameText(const u8* name) {
    std::string text(reinterpret_cast<const char*>(name), NETBIOS_NAME_RECORD_SIZE - 1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\0'))
        text.pop_back();
    return text;
}

// As the original: up to 15 characters, padded with spaces, then a zero.
void FormatName(const char* source, u8* destination) {
    u32 i;
    memset(destination, 0, NETBIOS_NAME_RECORD_SIZE);
    for (i = 0; i < NETBIOS_NAME_RECORD_SIZE - 1 && *source != '\0'; i++, source++)
        destination[i] = static_cast<u8>(*source);
    for (; i < NETBIOS_NAME_RECORD_SIZE - 1; i++)
        destination[i] = ' ';
}

bool IsAnyName(const u8* name) {
    return name[0] == '*';
}

void WriteSessionFrame(Stream& stream, u8 kind, const u8* first, const u8* second) {
    RecordWriter frame;
    frame.Put(kSessionMagic, SESSION_MAGIC_SIZE);
    frame.Put(static_cast<u8>(SESSION_VERSION));
    frame.Put(kind);
    frame.Put(first, NETBIOS_NAME_RECORD_SIZE);
    frame.Put(second, NETBIOS_NAME_RECORD_SIZE);
    stream.WriteFrame(frame.Data(), static_cast<size_t>(frame.Size()));
}

bool ReadSessionFrame(const std::vector<u8>& bytes, u8& kind, u8* first, u8* second) {
    RecordReader frame(bytes.data(), static_cast<i32>(bytes.size()));
    char magic[SESSION_MAGIC_SIZE];
    u8 version = 0;
    frame.Get(magic, SESSION_MAGIC_SIZE);
    frame.Get(version);
    frame.Get(kind);
    frame.Get(first, NETBIOS_NAME_RECORD_SIZE);
    frame.Get(second, NETBIOS_NAME_RECORD_SIZE);
    return frame.Ok() && frame.Remaining() == 0
           && memcmp(magic, kSessionMagic, SESSION_MAGIC_SIZE) == 0 && version == SESSION_VERSION;
}

void CloseSlot(Slot& slot) {
    slot.stream.reset();
    slot.established = false;
    slot.pending = PENDING_NONE;
}

void StartCall(Slot& slot, const Address& address) {
    slot.callAddress = address;
    slot.callRetries = 0;
    slot.nextCall = platform::Ticks();
    slot.pending = PENDING_CALL;
    slot.stream.reset();
    slot.established = false;
}

void Announce() {
    Netbios& state = State();
    if (state.nameSlot >= NETBIOS_SESSION_COUNT)
        return;
    if (!state.announcer)
        state.announcer = Datagram::Open(0);
    if (!state.announcer)
        return;
    RecordWriter announce;
    WriteNetbiosAnnounce(announce, state.slots[state.nameSlot].name);
    state.announcer->Broadcast(platform::net::CurrentSettings().port, announce.Data(),
                               static_cast<size_t>(announce.Size()));
}

bool OpenListener() {
    Netbios& state = State();
    if (state.listener)
        return true;
    u16 port = platform::net::CurrentSettings().port;
    state.listener = Listener::Open(port);
    if (!state.listener) {
        if (!state.listenFailureLogged)
            platform::Log("network: cannot listen on port %u", port);
        state.listenFailureLogged = true;
        return false;
    }
    platform::Log("network: hosting on port %u", port);
    return true;
}

void Established(i32 index, const u8* peerName) {
    Netbios& state = State();
    Slot& slot = state.slots[index];
    memcpy(slot.name, peerName, NETBIOS_NAME_RECORD_SIZE);
    slot.established = true;
    slot.pending = PENDING_NONE;
    state.status[index] |= NETBIOS_SESSION_ACTIVE | NETBIOS_SESSION_CONNECTED;
    platform::Log("network: session %d with %s at %s", index, NameText(peerName).c_str(),
                  platform::net::ToString(slot.stream->Peer()).c_str());
}

void PumpSlot(i32 index) {
    Netbios& state = State();
    Slot& slot = state.slots[index];
    const u8* localName = state.nameSlot < NETBIOS_SESSION_COUNT
                              ? state.slots[state.nameSlot].name
                              : slot.name;
    if (slot.pending == PENDING_CALL && !slot.stream) {
        if (static_cast<i32>(platform::Ticks() - slot.nextCall) < 0)
            return;
        slot.stream = Stream::Connect(slot.callAddress);
        if (slot.stream)
            WriteSessionFrame(*slot.stream, SESSION_CALL, slot.wanted, localName);
    }
    if (!slot.stream)
        return;
    bool alive = slot.stream->Pump();
    std::vector<u8> frame;
    if (!slot.established) {
        u8 kind = 0;
        u8 first[NETBIOS_NAME_RECORD_SIZE];
        u8 second[NETBIOS_NAME_RECORD_SIZE];
        if (slot.stream->ReadFrame(frame)) {
            if (!ReadSessionFrame(frame, kind, first, second)) {
                alive = false;
            } else if (slot.pending == PENDING_CALL && kind == SESSION_ACCEPT) {
                // first: the listener's name.
                Established(index, first);
            } else if (slot.pending == PENDING_LISTEN && kind == SESSION_CALL) {
                // first: the name called, second: the caller's.
                if (IsAnyName(first) || memcmp(first, localName, NETBIOS_NAME_RECORD_SIZE) == 0) {
                    WriteSessionFrame(*slot.stream, SESSION_ACCEPT, localName, second);
                    slot.stream->Pump();
                    Established(index, second);
                } else {
                    WriteSessionFrame(*slot.stream, SESSION_REFUSE, localName, second);
                    slot.stream->Pump();
                    alive = false;
                }
            } else {
                alive = false;
            }
        }
        if (!slot.established && (!alive || slot.stream->Failed())) {
            slot.stream.reset();
            if (slot.pending == PENDING_CALL) {
                slot.callRetries++;
                if (slot.callRetries < CALL_RETRY_LIMIT) {
                    slot.nextCall = platform::Ticks() + CALL_RETRY_DELAY;
                } else {
                    platform::Log("network: cannot reach %s",
                                  platform::net::ToString(slot.callAddress).c_str());
                    slot.pending = PENDING_NONE;
                }
            }
            return;
        }
        if (!slot.established)
            return;
    }
    while (slot.stream->ReadFrame(frame)) {
        Packet packet;
        packet.session = static_cast<u8>(index);
        packet.data.swap(frame);
        state.received.push_back(std::move(packet));
    }
    if (!alive || slot.stream->Failed()) {
        platform::Log("network: session %d closed", index);
        state.status[index] &= static_cast<u8>(~NETBIOS_SESSION_ACTIVE);
        slot.stream.reset();
        slot.established = false;
    }
}

void Send(std::deque<Packet>& queue) {
    Netbios& state = State();
    while (!queue.empty()) {
        Packet& packet = queue.front();
        Slot& slot = state.slots[packet.session];
        // A packet for a session that is not open is dropped, as the
        // original's worker did when the session had no LSN.
        if (slot.established && slot.stream)
            slot.stream->WriteFrame(packet.data.data(), packet.data.size());
        queue.pop_front();
    }
}

void Pump() {
    Netbios& state = State();
    if (!state.open)
        return;
    if (state.listener) {
        while (std::unique_ptr<Stream> caller = state.listener->Accept()) {
            i32 index;
            for (index = 0; index < NETBIOS_SESSION_COUNT; index++) {
                Slot& slot = state.slots[index];
                if (slot.pending == PENDING_LISTEN && !slot.stream)
                    break;
            }
            if (index < NETBIOS_SESSION_COUNT)
                state.slots[index].stream = std::move(caller);
        }
    }
    if (state.discovery) {
        u8 datagram[64];
        Address from;
        i32 length;
        while ((length = state.discovery->Receive(datagram, sizeof(datagram), &from)) >= 0) {
            RecordReader announce(datagram, length);
            u8 hostName[NETBIOS_NAME_RECORD_SIZE];
            if (!ReadNetbiosAnnounce(announce, hostName))
                continue;
            for (i32 index = 0; index < NETBIOS_SESSION_COUNT; index++) {
                Slot& slot = state.slots[index];
                if (slot.pending != PENDING_RECEIVE_ANY)
                    continue;
                platform::Log("network: found host %s at %s", NameText(hostName).c_str(),
                              platform::net::ToString(from).c_str());
                memcpy(slot.wanted, hostName, NETBIOS_NAME_RECORD_SIZE);
                StartCall(slot,
                          platform::net::WithPort(from, platform::net::CurrentSettings().port));
            }
        }
    }
    for (i32 index = 0; index < NETBIOS_SESSION_COUNT; index++)
        PumpSlot(index);
    Send(state.priority);
    Send(state.normal);
    for (i32 index = 0; index < NETBIOS_SESSION_COUNT; index++) {
        Slot& slot = state.slots[index];
        if (slot.established && slot.stream && slot.stream->PendingOutput())
            slot.stream->Pump();
    }
}

bool ValidSession(i32 session) {
    return session >= 0 && session < NETBIOS_SESSION_COUNT;
}

} // namespace

void nb_thr_ctl(void) {
    Pump();
}

extern "C" u16 nb_init(u16 maxSessions) {
    Netbios& state = State();
    nb_term(0);
    state.open = true;
    state.nameSlot = static_cast<u8>(maxSessions);
    for (i32 index = 0; index < NETBIOS_SESSION_COUNT; index++) {
        state.status[index] = 0;
        state.slots[index] = Slot();
    }
    state.listenFailureLogged = false;
    gNetbiosAvail = 1;
    return 0;
}

extern "C" void nb_term(i32) {
    Netbios& state = State();
    for (i32 index = 0; index < NETBIOS_SESSION_COUNT; index++) {
        Slot& slot = state.slots[index];
        // What is queued for the peer still goes out (a player's exit).
        if (slot.stream && slot.established)
            slot.stream->Pump();
        CloseSlot(slot);
        state.status[index] &= static_cast<u8>(~NETBIOS_SESSION_ACTIVE);
    }
    state.listener.reset();
    state.announcer.reset();
    state.discovery.reset();
    state.received.clear();
    state.priority.clear();
    state.normal.clear();
    state.open = false;
}

extern "C" i16 nb_rcv(i32, u16 maxLength, void* buffer) {
    Netbios& state = State();
    Pump();
    if (state.received.empty())
        return 0;
    Packet packet = std::move(state.received.front());
    state.received.pop_front();
    size_t size = packet.data.size() < maxLength ? packet.data.size() : maxLength;
    memcpy(buffer, packet.data.data(), size);
    return static_cast<i16>(size);
}

extern "C" i16 nb_snd(i32, u16 session, u16 length, void* data, i32 priority) {
    Netbios& state = State();
    if (session == state.nameSlot && length == 0) {
        Announce();
        return NRC_GOODRET;
    }
    if (!ValidSession(session) || !(state.status[session] & NETBIOS_SESSION_ACTIVE))
        return NRC_SNUMOUT;
    Packet packet;
    packet.session = static_cast<u8>(session);
    packet.data.assign(static_cast<u8*>(data), static_cast<u8*>(data) + length);
    if (priority)
        state.priority.push_back(std::move(packet));
    else
        state.normal.push_back(std::move(packet));
    Pump();
    return NRC_GOODRET;
}

extern "C" i16 nb_sess(i32, i32 operation, ...) {
    Netbios& state = State();
    const platform::net::Settings& settings = platform::net::CurrentSettings();
    va_list args;
    i32 session;
    i32 destination;
    i32 release;
    char* name;
    i16 result = NRC_GOODRET;

    va_start(args, operation);
    switch (operation) {
        case NETBIOS_SESSION_REGISTER: {
            // Names are local: registering one succeeds at once.
            name = va_arg(args, char*);
            if (!ValidSession(state.nameSlot)) {
                result = NRC_NORESOURCES;
                break;
            }
            state.status[state.nameSlot] &= static_cast<u8>(~NETBIOS_SESSION_ERROR);
            FormatName(name, state.slots[state.nameSlot].name);
            state.status[state.nameSlot] |= NETBIOS_SESSION_NAME_REGISTERED;
            break;
        }
        case NETBIOS_SESSION_RECEIVE_ANY: {
            // The guest waits for the host's broadcast, then calls it.
            session = va_arg(args, i32);
            if (!ValidSession(session)) {
                result = NRC_SNUMOUT;
                break;
            }
            Slot& slot = state.slots[session];
            if (slot.pending == PENDING_CALL || slot.pending == PENDING_RECEIVE_ANY)
                break;
            Address address;
            if (!settings.join.empty()) {
                if (!platform::net::Resolve(settings.join, settings.port, address)) {
                    platform::Log("network: cannot resolve %s", settings.join.c_str());
                    result = NRC_NOCALL;
                    break;
                }
                FormatName("*", slot.wanted);
                platform::Log("network: joining %s", platform::net::ToString(address).c_str());
                StartCall(slot, address);
            } else {
                if (!state.discovery)
                    state.discovery = Datagram::Open(settings.port);
                if (!state.discovery) {
                    platform::Log("network: cannot listen for hosts on UDP port %u", settings.port);
                    result = NRC_NORESOURCES;
                    break;
                }
                platform::Log("network: looking for a host on UDP port %u", settings.port);
                slot.pending = PENDING_RECEIVE_ANY;
            }
            break;
        }
        case NETBIOS_SESSION_CALL: {
            session = va_arg(args, i32);
            name = va_arg(args, char*);
            Address address;
            if (!ValidSession(session) || settings.join.empty()
                || !platform::net::Resolve(settings.join, settings.port, address)) {
                result = NRC_NOCALL;
                break;
            }
            FormatName(name, state.slots[session].wanted);
            StartCall(state.slots[session], address);
            break;
        }
        case NETBIOS_SESSION_LISTEN_ANY:
        case NETBIOS_SESSION_LISTEN: {
            session = va_arg(args, i32);
            name = operation == NETBIOS_SESSION_LISTEN ? va_arg(args, char*) : NULL;
            if (!ValidSession(session)) {
                result = NRC_SNUMOUT;
                break;
            }
            if (operation == NETBIOS_SESSION_LISTEN_ANY)
                Announce();
            if (!OpenListener()) {
                result = NRC_NORESOURCES;
                break;
            }
            Slot& slot = state.slots[session];
            CloseSlot(slot);
            FormatName(name ? name : "*", slot.wanted);
            slot.pending = PENDING_LISTEN;
            break;
        }
        case NETBIOS_SESSION_MOVE: {
            session = va_arg(args, i32);
            destination = va_arg(args, i32);
            release = va_arg(args, i32);
            if (session == state.nameSlot)
                state.nameSlot = static_cast<u8>(destination);
            if (!ValidSession(session) || !ValidSession(destination)
                || !state.slots[session].established)
                break;
            Slot& from = state.slots[session];
            Slot& to = state.slots[destination];
            // One connection cannot serve two slots: the session moves.
            to.stream = std::move(from.stream);
            to.established = true;
            to.pending = PENDING_NONE;
            memcpy(to.name, from.name, NETBIOS_NAME_RECORD_SIZE);
            state.status[destination] = state.status[session];
            from.established = false;
            if (release) {
                from.pending = PENDING_NONE;
                state.status[session] = 0;
                memset(from.name, 0, NETBIOS_NAME_RECORD_SIZE);
            }
            for (Packet& packet : state.received) {
                if (packet.session == session)
                    packet.session = static_cast<u8>(destination);
            }
            break;
        }
        case NETBIOS_SESSION_CLOSE:
            session = va_arg(args, i32);
            if (ValidSession(session)) {
                if (state.slots[session].established)
                    state.status[session] &= static_cast<u8>(~NETBIOS_SESSION_ACTIVE);
                CloseSlot(state.slots[session]);
            }
            break;
        case NETBIOS_SESSION_CLEAR_CONNECTED:
            session = va_arg(args, i32);
            if (ValidSession(session))
                state.status[session] &= static_cast<u8>(~NETBIOS_SESSION_CONNECTED);
            break;
        case NETBIOS_SESSION_GET_NAME:
            session = va_arg(args, i32);
            name = va_arg(args, char*);
            if (ValidSession(session))
                memcpy(name, state.slots[session].name, NETBIOS_NAME_RECORD_SIZE);
            break;
        default:
            result = 1;
            break;
    }
    va_end(args);
    Pump();
    return result;
}

extern "C" u8 nb_stat(i32, u16 session) {
    Pump();
    if (!ValidSession(session))
        return 0;
    return State().status[session];
}
