// Non-blocking sockets for the network and serial transports (Net.h): BSD
// sockets, or Winsock on Windows.

#include <PLATFORM/Net.h>

#include <PLATFORM/Platform.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET NativeSocket;
typedef int SocketLength;
#define H1_SOCKET_WOULD_BLOCK(error) ((error) == WSAEWOULDBLOCK || (error) == WSAEINPROGRESS)
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int NativeSocket;
typedef socklen_t SocketLength;
#define H1_SOCKET_WOULD_BLOCK(error)                                                               \
    ((error) == EAGAIN || (error) == EWOULDBLOCK || (error) == EINPROGRESS || (error) == EINTR)
#endif

namespace platform {
namespace net {

namespace {

const std::intptr_t kNoSocket = -1;

NativeSocket Native(std::intptr_t socket) {
    return static_cast<NativeSocket>(socket);
}

std::intptr_t Handle(NativeSocket socket) {
#if defined(_WIN32)
    if (socket == INVALID_SOCKET)
        return kNoSocket;
#else
    if (socket < 0)
        return kNoSocket;
#endif
    return static_cast<std::intptr_t>(socket);
}

int LastError() {
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

void CloseSocket(std::intptr_t socket) {
    if (socket == kNoSocket)
        return;
#if defined(_WIN32)
    closesocket(Native(socket));
#else
    close(Native(socket));
#endif
}

bool SetNonBlocking(NativeSocket socket) {
#if defined(_WIN32)
    u_long enabled = 1;
    return ioctlsocket(socket, FIONBIO, &enabled) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

void SetOption(NativeSocket socket, int level, int name, int value) {
    setsockopt(socket, level, name, reinterpret_cast<const char*>(&value),
               static_cast<SocketLength>(sizeof(value)));
}

bool StartNetwork() {
#if defined(_WIN32)
    static bool started = false;
    if (!started) {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            return false;
        started = true;
    }
#endif
    return true;
}

sockaddr_storage Storage(const Address& address) {
    sockaddr_storage storage;
    std::memset(&storage, 0, sizeof(storage));
    std::memcpy(&storage, address.storage, address.size);
    return storage;
}

Address FromStorage(const sockaddr_storage& storage, SocketLength size) {
    Address address;
    size_t length = static_cast<size_t>(size);
    if (length > sizeof(address.storage))
        length = sizeof(address.storage);
    std::memcpy(address.storage, &storage, length);
    address.size = static_cast<u32>(length);
    return address;
}

// Whether the socket is ready for the events, without waiting.
bool Ready(NativeSocket socket, short events, short* returned) {
#if defined(_WIN32)
    WSAPOLLFD entry;
    entry.fd = socket;
    entry.events = events;
    entry.revents = 0;
    int result = WSAPoll(&entry, 1, 0);
#else
    pollfd entry;
    entry.fd = socket;
    entry.events = events;
    entry.revents = 0;
    int result = poll(&entry, 1, 0);
#endif
    if (returned)
        *returned = entry.revents;
    return result > 0;
}

} // namespace

Settings& CurrentSettings() {
    static Settings settings;
    return settings;
}

void LoadSettingsFromEnvironment() {
    Settings& settings = CurrentSettings();
    std::string port = Environment("HOMM1_NET_PORT");
    if (!port.empty()) {
        long value = std::strtol(port.c_str(), nullptr, 10);
        if (value > 0 && value < 65536)
            settings.port = static_cast<u16>(value);
    }
    if (settings.join.empty())
        settings.join = Environment("HOMM1_NET_JOIN");
}

bool Resolve(const std::string& text, u16 defaultPort, Address& address) {
    if (!StartNetwork())
        return false;
    std::string host = text;
    std::string port = std::to_string(defaultPort);
    if (!host.empty() && host[0] == '[') {
        size_t close = host.find(']');
        if (close == std::string::npos)
            return false;
        if (close + 1 < host.size() && host[close + 1] == ':')
            port = host.substr(close + 2);
        host = host.substr(1, close - 1);
    } else {
        size_t colon = host.find(':');
        if (colon != std::string::npos && host.find(':', colon + 1) == std::string::npos) {
            port = host.substr(colon + 1);
            host = host.substr(0, colon);
        }
    }
    if (host.empty())
        return false;
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* found = nullptr;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &found) != 0 || found == nullptr)
        return false;
    // Prefer IPv4: the original's peers were on one local network.
    addrinfo* chosen = found;
    for (addrinfo* entry = found; entry != nullptr; entry = entry->ai_next) {
        if (entry->ai_family == AF_INET) {
            chosen = entry;
            break;
        }
    }
    std::memset(address.storage, 0, sizeof(address.storage));
    size_t size = static_cast<size_t>(chosen->ai_addrlen);
    if (size > sizeof(address.storage))
        size = sizeof(address.storage);
    std::memcpy(address.storage, chosen->ai_addr, size);
    address.size = static_cast<u32>(size);
    freeaddrinfo(found);
    return true;
}

Address WithPort(const Address& address, u16 port) {
    sockaddr_storage storage = Storage(address);
    if (storage.ss_family == AF_INET) {
        sockaddr_in in;
        std::memcpy(&in, &storage, sizeof(in));
        in.sin_port = htons(port);
        std::memcpy(&storage, &in, sizeof(in));
    } else if (storage.ss_family == AF_INET6) {
        sockaddr_in6 in6;
        std::memcpy(&in6, &storage, sizeof(in6));
        in6.sin6_port = htons(port);
        std::memcpy(&storage, &in6, sizeof(in6));
    }
    return FromStorage(storage, static_cast<SocketLength>(address.size));
}

std::string ToString(const Address& address) {
    if (!address.Valid())
        return "(none)";
    sockaddr_storage storage = Storage(address);
    char host[128] = {};
    char port[16] = {};
    if (getnameinfo(reinterpret_cast<const sockaddr*>(&storage),
                    static_cast<SocketLength>(address.size), host, sizeof(host), port,
                    sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
        return "(unknown)";
    if (storage.ss_family == AF_INET6)
        return std::string("[") + host + "]:" + port;
    return std::string(host) + ":" + port;
}

// ---------------------------------------------------------------- Stream

Stream::Stream(std::intptr_t socket, State state, const Address& peer)
    : m_socket(socket), m_state(state), m_peer(peer) {}

Stream::~Stream() {
    CloseSocket(m_socket);
}

std::unique_ptr<Stream> Stream::Connect(const Address& address) {
    if (!StartNetwork() || !address.Valid())
        return nullptr;
    sockaddr_storage storage = Storage(address);
    NativeSocket socket = ::socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
    std::intptr_t handle = Handle(socket);
    if (handle == kNoSocket)
        return nullptr;
    SetNonBlocking(socket);
    SetOption(socket, IPPROTO_TCP, TCP_NODELAY, 1);
    State state = STATE_CONNECTING;
    if (connect(socket, reinterpret_cast<const sockaddr*>(&storage),
                static_cast<SocketLength>(address.size))
        == 0) {
        state = STATE_OPEN;
    } else if (!H1_SOCKET_WOULD_BLOCK(LastError())) {
        state = STATE_FAILED;
    }
    return std::unique_ptr<Stream>(new Stream(handle, state, address));
}

void Stream::Fail() {
    if (m_state != STATE_CLOSED)
        m_state = STATE_FAILED;
}

bool Stream::Pump() {
    if (m_state == STATE_CONNECTING) {
        short events = 0;
        if (!Ready(Native(m_socket), POLLOUT, &events))
            return true;
        int error = 0;
        SocketLength size = static_cast<SocketLength>(sizeof(error));
        getsockopt(Native(m_socket), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &size);
        if (error != 0 || (events & (POLLERR | POLLHUP))) {
            m_state = STATE_FAILED;
            return false;
        }
        m_state = STATE_OPEN;
    }
    if (m_state != STATE_OPEN)
        return false;
    // Output.
    while (!m_output.empty()) {
        u8 chunk[4096];
        size_t size = 0;
        for (auto it = m_output.begin(); it != m_output.end() && size < sizeof(chunk); ++it)
            chunk[size++] = *it;
#if defined(_WIN32)
        int sent = send(Native(m_socket), reinterpret_cast<const char*>(chunk),
                        static_cast<int>(size), 0);
#elif defined(MSG_NOSIGNAL)
        ssize_t sent = send(Native(m_socket), chunk, size, MSG_NOSIGNAL);
#else
        ssize_t sent = send(Native(m_socket), chunk, size, 0);
#endif
        if (sent < 0) {
            if (H1_SOCKET_WOULD_BLOCK(LastError()))
                break;
            Fail();
            return false;
        }
        m_output.erase(m_output.begin(),
                       m_output.begin() + static_cast<std::ptrdiff_t>(sent));
        if (static_cast<size_t>(sent) < size)
            break;
    }
    // Input.
    for (;;) {
        u8 chunk[4096];
#if defined(_WIN32)
        int received = recv(Native(m_socket), reinterpret_cast<char*>(chunk),
                            static_cast<int>(sizeof(chunk)), 0);
#else
        ssize_t received = recv(Native(m_socket), chunk, sizeof(chunk), 0);
#endif
        if (received > 0) {
            m_input.insert(m_input.end(), chunk, chunk + received);
            continue;
        }
        if (received == 0) {
            m_state = STATE_CLOSED;
            return false;
        }
        if (H1_SOCKET_WOULD_BLOCK(LastError()))
            break;
        Fail();
        return false;
    }
    return true;
}

void Stream::Write(const void* data, size_t size) {
    const u8* bytes = static_cast<const u8*>(data);
    m_output.insert(m_output.end(), bytes, bytes + size);
}

size_t Stream::Read(void* data, size_t size) {
    u8* bytes = static_cast<u8*>(data);
    size_t count = size < m_input.size() ? size : m_input.size();
    for (size_t i = 0; i < count; i++)
        bytes[i] = m_input[i];
    m_input.erase(m_input.begin(), m_input.begin() + static_cast<std::ptrdiff_t>(count));
    return count;
}

void Stream::WriteFrame(const void* data, size_t size) {
    if (size > FRAME_MAX_SIZE)
        return;
    u8 length[2] = {static_cast<u8>(size & 0xff), static_cast<u8>(size >> 8)};
    Write(length, sizeof(length));
    Write(data, size);
}

bool Stream::ReadFrame(std::vector<u8>& frame) {
    if (m_input.size() < 2)
        return false;
    size_t size = static_cast<size_t>(m_input[0]) | static_cast<size_t>(m_input[1]) << 8;
    if (m_input.size() < 2 + size)
        return false;
    frame.assign(m_input.begin() + 2, m_input.begin() + 2 + static_cast<std::ptrdiff_t>(size));
    m_input.erase(m_input.begin(), m_input.begin() + 2 + static_cast<std::ptrdiff_t>(size));
    return true;
}

// ---------------------------------------------------------------- Listener

Listener::Listener(std::intptr_t socket) : m_socket(socket) {}

Listener::~Listener() {
    CloseSocket(m_socket);
}

std::unique_ptr<Listener> Listener::Open(u16 port) {
    if (!StartNetwork())
        return nullptr;
    // Both IPv6 and IPv4 where the host allows one socket for both.
    NativeSocket socket = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    std::intptr_t handle = Handle(socket);
    if (handle != kNoSocket) {
        SetOption(socket, IPPROTO_IPV6, IPV6_V6ONLY, 0);
#if !defined(_WIN32)
        SetOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#endif
        sockaddr_in6 any;
        std::memset(&any, 0, sizeof(any));
        any.sin6_family = AF_INET6;
        any.sin6_addr = in6addr_any;
        any.sin6_port = htons(port);
        if (bind(socket, reinterpret_cast<const sockaddr*>(&any),
                 static_cast<SocketLength>(sizeof(any)))
                != 0
            || listen(socket, 4) != 0) {
            CloseSocket(handle);
            handle = kNoSocket;
        }
    }
    if (handle == kNoSocket) {
        socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        handle = Handle(socket);
        if (handle == kNoSocket)
            return nullptr;
#if !defined(_WIN32)
        SetOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#endif
        sockaddr_in any;
        std::memset(&any, 0, sizeof(any));
        any.sin_family = AF_INET;
        any.sin_addr.s_addr = htonl(INADDR_ANY);
        any.sin_port = htons(port);
        if (bind(socket, reinterpret_cast<const sockaddr*>(&any),
                 static_cast<SocketLength>(sizeof(any)))
                != 0
            || listen(socket, 4) != 0) {
            CloseSocket(handle);
            return nullptr;
        }
    }
    SetNonBlocking(socket);
    return std::unique_ptr<Listener>(new Listener(handle));
}

std::unique_ptr<Stream> Listener::Accept() {
    sockaddr_storage peer;
    SocketLength size = static_cast<SocketLength>(sizeof(peer));
    NativeSocket socket = accept(Native(m_socket), reinterpret_cast<sockaddr*>(&peer), &size);
    std::intptr_t handle = Handle(socket);
    if (handle == kNoSocket)
        return nullptr;
    SetNonBlocking(socket);
    SetOption(socket, IPPROTO_TCP, TCP_NODELAY, 1);
    return std::unique_ptr<Stream>(new Stream(handle, Stream::STATE_OPEN, FromStorage(peer, size)));
}

// ---------------------------------------------------------------- Datagram

Datagram::Datagram(std::intptr_t socket) : m_socket(socket) {}

Datagram::~Datagram() {
    CloseSocket(m_socket);
}

std::unique_ptr<Datagram> Datagram::Open(u16 port) {
    if (!StartNetwork())
        return nullptr;
    NativeSocket socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    std::intptr_t handle = Handle(socket);
    if (handle == kNoSocket)
        return nullptr;
    SetOption(socket, SOL_SOCKET, SO_BROADCAST, 1);
    SetOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#if defined(SO_REUSEPORT)
    SetOption(socket, SOL_SOCKET, SO_REUSEPORT, 1);
#endif
    sockaddr_in any;
    std::memset(&any, 0, sizeof(any));
    any.sin_family = AF_INET;
    any.sin_addr.s_addr = htonl(INADDR_ANY);
    any.sin_port = htons(port);
    if (port != 0
        && bind(socket, reinterpret_cast<const sockaddr*>(&any),
                static_cast<SocketLength>(sizeof(any)))
               != 0) {
        CloseSocket(handle);
        return nullptr;
    }
    SetNonBlocking(socket);
    return std::unique_ptr<Datagram>(new Datagram(handle));
}

void Datagram::Broadcast(u16 port, const void* data, size_t size) {
    const u32 targets[2] = {INADDR_BROADCAST, INADDR_LOOPBACK};
    for (u32 target : targets) {
        sockaddr_in to;
        std::memset(&to, 0, sizeof(to));
        to.sin_family = AF_INET;
        to.sin_addr.s_addr = htonl(target);
        to.sin_port = htons(port);
#if defined(_WIN32)
        sendto(Native(m_socket), static_cast<const char*>(data), static_cast<int>(size), 0,
               reinterpret_cast<const sockaddr*>(&to), static_cast<SocketLength>(sizeof(to)));
#else
        sendto(Native(m_socket), data, size, 0, reinterpret_cast<const sockaddr*>(&to),
               static_cast<SocketLength>(sizeof(to)));
#endif
    }
}

i32 Datagram::Receive(void* data, size_t capacity, Address* from) {
    sockaddr_storage peer;
    SocketLength size = static_cast<SocketLength>(sizeof(peer));
#if defined(_WIN32)
    int received = recvfrom(Native(m_socket), static_cast<char*>(data), static_cast<int>(capacity),
                            0, reinterpret_cast<sockaddr*>(&peer), &size);
#else
    ssize_t received =
        recvfrom(Native(m_socket), data, capacity, 0, reinterpret_cast<sockaddr*>(&peer), &size);
#endif
    if (received < 0)
        return -1;
    if (from)
        *from = FromStorage(peer, size);
    return static_cast<i32>(received);
}

} // namespace net
} // namespace platform
