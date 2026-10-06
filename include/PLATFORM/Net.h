#ifndef HOMM1_PLATFORM_NET_H
#define HOMM1_PLATFORM_NET_H

// Sockets for the native port's network and serial transports: non-blocking
// TCP streams with optional length-prefixed framing, a listener, and UDP
// datagrams for finding a host on the local network. Everything is polled
// from the game's thread (the transports pump from the calls the game makes
// and from its PollRemote loop); nothing blocks except name resolution.
//
// The game never includes this header: src/PORT/SOURCE/netwin.cpp and
// comwin.cpp implement the game's NetBIOS and serial calls on it.

#include <H1/Ints.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace platform {
namespace net {

enum NetConstant {
    // The TCP and UDP port the transports use unless told otherwise.
    DEFAULT_PORT = 1995,
    // A frame's body is at most this long (its length is a 16-bit field).
    FRAME_MAX_SIZE = 0xffff
};

// Where the transports listen and whom they join: --port and --join on the
// command line, else $HOMM1_NET_PORT and $HOMM1_NET_JOIN.
struct Settings {
    u16 port = DEFAULT_PORT;
    // "host", "host:port", "a.b.c.d:port" or "[v6]:port"; empty: none.
    std::string join;
};
Settings& CurrentSettings();
// Fills unset values from the environment; called once at start-up.
void LoadSettingsFromEnvironment();

// An IPv4 or IPv6 endpoint.
struct Address {
    alignas(8) u8 storage[128] = {};
    u32 size = 0;
    bool Valid() const {
        return size != 0;
    }
};

// Resolves "host[:port]" (the port defaults to defaultPort). Blocks for a
// name lookup.
bool Resolve(const std::string& text, u16 defaultPort, Address& address);
// The same host on another port.
Address WithPort(const Address& address, u16 port);
std::string ToString(const Address& address);

// A TCP connection. Output is queued and written as the socket accepts it;
// input collects in a buffer. Pump() moves bytes both ways.
class Stream {
public:
    // Starts connecting; Connected() or Failed() tells the outcome.
    static std::unique_ptr<Stream> Connect(const Address& address);
    ~Stream();
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    // Moves queued output to the socket and arrived input into the buffer.
    // Returns false once the connection has failed or the peer closed it.
    bool Pump();
    bool Connected() const {
        return m_state == STATE_OPEN;
    }
    bool Failed() const {
        return m_state == STATE_FAILED || m_state == STATE_CLOSED;
    }
    const Address& Peer() const {
        return m_peer;
    }

    // Raw bytes.
    void Write(const void* data, size_t size);
    size_t Available() const {
        return m_input.size();
    }
    size_t Read(void* data, size_t size);
    size_t PendingOutput() const {
        return m_output.size();
    }

    // Frames: a 16-bit little-endian length, then the body.
    void WriteFrame(const void* data, size_t size);
    // Takes the next whole frame from the input; false if none has arrived.
    bool ReadFrame(std::vector<u8>& frame);

private:
    friend class Listener;
    enum State {
        STATE_CONNECTING,
        STATE_OPEN,
        STATE_CLOSED,
        STATE_FAILED
    };
    Stream(std::intptr_t socket, State state, const Address& peer);
    void Fail();

    std::intptr_t m_socket;
    State m_state;
    Address m_peer;
    std::deque<u8> m_input;
    std::deque<u8> m_output;
};

// A TCP listening socket on every interface.
class Listener {
public:
    static std::unique_ptr<Listener> Open(u16 port);
    ~Listener();
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;
    // The next waiting connection, or none.
    std::unique_ptr<Stream> Accept();

private:
    explicit Listener(std::intptr_t socket);
    std::intptr_t m_socket;
};

// A UDP socket bound to a port, able to broadcast.
class Datagram {
public:
    static std::unique_ptr<Datagram> Open(u16 port);
    ~Datagram();
    Datagram(const Datagram&) = delete;
    Datagram& operator=(const Datagram&) = delete;
    // To every host on the local networks and to this host, at port.
    void Broadcast(u16 port, const void* data, size_t size);
    // The next waiting datagram's length (and sender), or -1.
    i32 Receive(void* data, size_t capacity, Address* from);

private:
    explicit Datagram(std::intptr_t socket);
    std::intptr_t m_socket;
};

} // namespace net
} // namespace platform

#endif
