// The native TCP transport refuses a peer of another protocol version: the
// edition's program (REMOTE_PROTOCOL_VERSION 1) against a native program of
// the original game, whose session frames carry no protocol byte. Real
// sockets on localhost drive the game's NetBIOS calls (src/PORT/SOURCE/
// netwin.cpp):
//
//   - hosting, it refuses a caller of the original game and stays listening,
//     then accepts a caller of its own version;
//   - calling, it gives up on a listener of the original game at once
//     instead of retrying, and connects to a listener of its own version.

#include <H1/Ints.h>

#include <PLATFORM/Net.h>
#include <PLATFORM/Platform.h>
#include <PLATFORM/Records.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/netwin.h>
#include <SOURCE/remoteRecords.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// The record classes' constructors live in game units this test does not
// link; the codecs need only the storage.
armyGroup::armyGroup(void) {}
hero::hero(void) {}
town::town(void) {}

namespace {

using platform::net::Address;
using platform::net::Listener;
using platform::net::Stream;

int gFailures = 0;

void Expect(bool condition, const std::string& what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what.c_str());
        gFailures++;
    }
}

const u8 kAnyName[NETBIOS_NAME_RECORD_SIZE] = {'*'};
const u8 kGuestName[NETBIOS_NAME_RECORD_SIZE] = {'H', 'G', 'U', 'E', 'S', 'T', '1', ' ', ' ',
                                                  ' ', ' ', ' ', ' ', ' ', ' ', 0};
const u8 kHostName[NETBIOS_NAME_RECORD_SIZE] = {'H', 'H', 'O', 'S', 'T', '0', ' ', ' ',
                                                 ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0};

// A session frame as a native program of the original game writes it:
// "H1NB", frame version 1, kind, two names.
std::vector<u8> OriginalFrame(u8 kind, const u8* first, const u8* second) {
    std::vector<u8> frame = {'H', '1', 'N', 'B', 1, kind};
    frame.insert(frame.end(), first, first + NETBIOS_NAME_RECORD_SIZE);
    frame.insert(frame.end(), second, second + NETBIOS_NAME_RECORD_SIZE);
    return frame;
}

std::vector<u8> EditionFrame(u8 kind, const u8* first, const u8* second) {
    RecordWriter writer;
    WriteNetbiosSession(writer, kind, first, second);
    return std::vector<u8>(writer.Data(), writer.Data() + writer.Size());
}

bool Connected(i32 session) {
    return (nb_stat(0, static_cast<u16>(session)) & NETBIOS_SESSION_CONNECTED) != 0;
}

// Pumps the game's side and the test's stream until the stream has a frame
// or has closed, or two seconds pass.
bool AwaitFrame(Stream& stream, std::vector<u8>& frame) {
    u32 deadline = platform::Ticks() + 2000;
    while (static_cast<i32>(platform::Ticks() - deadline) < 0) {
        nb_thr_ctl();
        if (!stream.Pump() && stream.Available() == 0)
            return false;
        if (stream.ReadFrame(frame))
            return true;
        platform::Sleep(5);
    }
    return false;
}

std::unique_ptr<Stream> ConnectTo(u16 port) {
    Address address;
    if (!platform::net::Resolve("127.0.0.1", port, address))
        return nullptr;
    std::unique_ptr<Stream> stream = Stream::Connect(address);
    u32 deadline = platform::Ticks() + 2000;
    while (stream && !stream->Connected() && !stream->Failed()
           && static_cast<i32>(platform::Ticks() - deadline) < 0) {
        nb_thr_ctl();
        stream->Pump();
        platform::Sleep(5);
    }
    return stream && stream->Connected() ? std::move(stream) : nullptr;
}

// Hosting: the game listens on session 0.
void CheckHost(u16 port) {
    platform::net::CurrentSettings().port = port;
    platform::net::CurrentSettings().join.clear();
    nb_init(NETBIOS_SESSION_COUNT - 1);
    Expect(nb_sess(0, NETBIOS_SESSION_REGISTER, const_cast<char*>("HHOST0")) == 0, "host: name");
    Expect(nb_sess(0, NETBIOS_SESSION_LISTEN_ANY, 0) == 0, "host: listening");

    std::unique_ptr<Stream> original = ConnectTo(port);
    Expect(original != nullptr, "host: the original game's caller connects to the port");
    if (original) {
        std::vector<u8> call = OriginalFrame(NETBIOS_FRAME_CALL, kAnyName, kGuestName);
        original->WriteFrame(call.data(), call.size());
        std::vector<u8> answer;
        bool answered = AwaitFrame(*original, answer);
        u8 kind = 0;
        u8 first[NETBIOS_NAME_RECORD_SIZE];
        u8 second[NETBIOS_NAME_RECORD_SIZE];
        RecordReader in(answer.data(), static_cast<i32>(answer.size()));
        Expect(answered && ReadNetbiosSession(in, kind, first, second) == NETBIOS_SESSION_VALID
                   && kind == NETBIOS_FRAME_REFUSE,
               "host: the original game's caller is refused");
        for (int i = 0; i < 20; i++) {
            nb_thr_ctl();
            original->Pump();
            platform::Sleep(5);
        }
        Expect(original->Failed(), "host: the refused connection is closed");
        Expect(!Connected(0), "host: no session with the original game");
    }

    std::unique_ptr<Stream> edition = ConnectTo(port);
    Expect(edition != nullptr, "host: the edition's caller connects to the port");
    if (edition) {
        std::vector<u8> call = EditionFrame(NETBIOS_FRAME_CALL, kAnyName, kGuestName);
        edition->WriteFrame(call.data(), call.size());
        std::vector<u8> answer;
        bool answered = AwaitFrame(*edition, answer);
        u8 kind = 0;
        u8 first[NETBIOS_NAME_RECORD_SIZE];
        u8 second[NETBIOS_NAME_RECORD_SIZE];
        RecordReader in(answer.data(), static_cast<i32>(answer.size()));
        Expect(answered && ReadNetbiosSession(in, kind, first, second) == NETBIOS_SESSION_VALID
                   && kind == NETBIOS_FRAME_ACCEPT,
               "host: the edition's caller is accepted");
        Expect(Connected(0), "host: a session with the edition's caller");
    }
    nb_term(0);
}

// A listener the test plays: answers the game's call with the given frame.
bool Answer(Listener& listener, const std::vector<u8>& reply, std::unique_ptr<Stream>& peer) {
    u32 deadline = platform::Ticks() + 2000;
    while (static_cast<i32>(platform::Ticks() - deadline) < 0) {
        nb_thr_ctl();
        if (!peer)
            peer = listener.Accept();
        if (peer) {
            peer->Pump();
            std::vector<u8> call;
            if (peer->ReadFrame(call)) {
                peer->WriteFrame(reply.data(), reply.size());
                peer->Pump();
                return true;
            }
        }
        platform::Sleep(5);
    }
    return false;
}

// Calling: the game calls the address on session 0.
void CheckGuest(u16 port) {
    std::unique_ptr<Listener> listener = Listener::Open(port);
    Expect(listener != nullptr, "guest: the test listens");
    if (!listener)
        return;
    platform::net::CurrentSettings().port = port;
    platform::net::CurrentSettings().join = "127.0.0.1:" + std::to_string(port);
    nb_init(NETBIOS_SESSION_COUNT - 1);
    Expect(nb_sess(0, NETBIOS_SESSION_REGISTER, const_cast<char*>("HGUEST1")) == 0, "guest: name");
    Expect(nb_sess(0, NETBIOS_SESSION_RECEIVE_ANY, 0) == 0, "guest: calling");

    std::unique_ptr<Stream> peer;
    Expect(Answer(*listener, OriginalFrame(NETBIOS_FRAME_ACCEPT, kHostName, kGuestName), peer),
           "guest: the original game's listener is called");
    // A refused call is not retried: no second connection arrives.
    std::unique_ptr<Stream> retry;
    for (int i = 0; i < 60 && !retry; i++) {
        nb_thr_ctl();
        retry = listener->Accept();
        platform::Sleep(5);
    }
    Expect(!Connected(0), "guest: no session with the original game");
    Expect(retry == nullptr, "guest: the original game's listener is not called again");

    peer.reset();
    retry.reset();
    Expect(nb_sess(0, NETBIOS_SESSION_RECEIVE_ANY, 0) == 0, "guest: calling again");
    Expect(Answer(*listener, EditionFrame(NETBIOS_FRAME_ACCEPT, kHostName, kGuestName), peer),
           "guest: the edition's listener is called");
    for (int i = 0; i < 40 && !Connected(0); i++) {
        peer->Pump();
        platform::Sleep(5);
    }
    Expect(Connected(0), "guest: a session with the edition's listener");
    nb_term(0);
}

// A free TCP port near the given one.
u16 FreePort(u16 from) {
    for (u16 port = from; port < from + 200; port++) {
        if (Listener::Open(port))
            return port;
    }
    return 0;
}

}  // namespace

int main() {
    u16 base = static_cast<u16>(47000 + platform::Ticks() % 1000);
    u16 hostPort = FreePort(base);
    u16 guestPort = FreePort(static_cast<u16>(hostPort + 1));
    if (hostPort == 0 || guestPort == 0) {
        std::printf("skipped: no free TCP port on localhost\n");
        return 77;
    }
    CheckHost(hostPort);
    CheckGuest(guestPort);
    if (gFailures != 0) {
        std::fprintf(stderr, "%d failure(s)\n", gFailures);
        return 1;
    }
    std::printf("ok: the edition refuses the original game's native peers and plays its own\n");
    return 0;
}
