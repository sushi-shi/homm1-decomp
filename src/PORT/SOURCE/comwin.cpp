// The native counterpart of src/SOURCE/comwin.cpp: the serial port the game's
// modem and direct-cable play write to and read from (com_init, com_snd,
// com_rcv, com_stat, comm_wrt_task, com_term), carried over a TCP stream.
//
// The game's serial code stays as it is: it frames its packets with escape
// bytes, exchanges identification packets and, for a modem, sends Hayes
// commands and waits for the modem's answers. Here the "cable" is a TCP
// connection and the bytes pass through unchanged.
//
//   - Direct connection: the host listens on the port (--port, default
//     1995), the guest connects to --join ADDRESS[:PORT] and keeps retrying
//     every second, as a cable plugged in later would start working. Bytes
//     written while nothing is connected are lost, as on an unplugged cable.
//   - Modem: a small Hayes modem answers the game's commands. AT commands get
//     OK; the guest's modem listens on the port and reports RING for an
//     incoming call, which ATA answers; the host's ATDT dials the number the
//     player typed, read as ADDRESS[:PORT] (a number of digits only means
//     --join). CONNECT, NO CARRIER and RING are sent as a modem sends them,
//     and a dropped connection is a lost carrier.
//
// The queues keep the original's order: com_snd queues, comm_wrt_task (from
// the game's PollRemote) writes, priority data first.

#include <H1/Ints.h>

#include <SOURCE/comwin.h>

#include <PLATFORM/Net.h>
#include <PLATFORM/Platform.h>
#include <SOURCE/KB.h>
#include <SOURCE/REMOTE.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

void add_node(tag_Anchor* anchor, tag_Node* node) {
    node->prev = node->next = NULL;
    if (anchor->tail) {
        anchor->tail->next = node;
        node->prev = anchor->tail;
        anchor->tail = node;
    } else {
        anchor->tail = node;
        anchor->head = node;
    }
}

tag_Node* pop_node(tag_Anchor* anchor) {
    tag_Node* node = anchor->head;
    if (node)
        anchor->head = node->next;
    if (!anchor->head)
        anchor->tail = NULL;
    return node;
}

void init_anchor(tag_Anchor* anchor, i32, i32) {
    anchor->head = NULL;
    anchor->tail = NULL;
}

namespace {

using platform::net::Address;
using platform::net::Listener;
using platform::net::Stream;

enum SerialConstant {
    DIRECT_RETRY_DELAY = 1000,
    RING_INTERVAL = 2000,
    DIAL_TIMEOUT = 30000
};

enum LineMode {
    LINE_COMMAND,
    LINE_DATA
};

struct SerialLine {
    bool open = false;
    bool modem = false;
    // Listens for the peer (the direct host, the modem guest).
    bool answers = false;
    LineMode mode = LINE_DATA;
    i32 baud = COM_RATE_19200;
    std::unique_ptr<Listener> listener;
    std::unique_ptr<Stream> stream;
    // An incoming call the modem has not answered yet.
    std::unique_ptr<Stream> ringing;
    u32 nextRing = 0;
    // Direct guest: where to connect and when to try again.
    Address peer;
    u32 nextConnect = 0;
    bool reported = false;
    // Modem: the command line being typed, a dial in progress.
    std::string command;
    bool dialing = false;
    u32 dialDeadline = 0;
    // What the game reads: modem answers and line data, in order.
    std::deque<u8> incoming;
    std::deque<std::vector<u8> > priority;
    std::deque<std::vector<u8> > normal;
};

SerialLine& Line() {
    static SerialLine line;
    return line;
}

std::string gComFailure;

void Respond(const char* text) {
    SerialLine& line = Line();
    std::string response = std::string("\r\n") + text + "\r\n";
    line.incoming.insert(line.incoming.end(), response.begin(), response.end());
}

void Connected() {
    SerialLine& line = Line();
    char text[32];
    line.mode = LINE_DATA;
    line.dialing = false;
    snprintf(text, sizeof(text), "CONNECT %d", line.baud);
    Respond(text);
    platform::Log("serial: connected to %s", platform::net::ToString(line.stream->Peer()).c_str());
}

void LostCarrier() {
    SerialLine& line = Line();
    platform::Log("serial: connection closed");
    line.stream.reset();
    if (line.modem) {
        line.mode = LINE_COMMAND;
        line.dialing = false;
        Respond("NO CARRIER");
    }
}

bool PhoneNumber(const std::string& number) {
    for (char c : number) {
        if (!strchr("0123456789-()+ *#,WTP!@;wtp", c))
            return false;
    }
    return true;
}

void Dial(std::string number) {
    SerialLine& line = Line();
    const platform::net::Settings& settings = platform::net::CurrentSettings();
    while (!number.empty() && (number[0] == 'T' || number[0] == 'P' || number[0] == ' '))
        number.erase(0, 1);
    while (!number.empty() && number.back() == ' ')
        number.pop_back();
    std::string address = PhoneNumber(number) ? settings.join : number;
    Address target;
    if (address.empty() || !platform::net::Resolve(address, settings.port, target)) {
        platform::Log("serial: cannot dial \"%s\" (type an ADDRESS[:PORT] or pass --join)",
                      number.c_str());
        Respond("NO CARRIER");
        return;
    }
    platform::Log("serial: dialing %s", platform::net::ToString(target).c_str());
    line.stream = Stream::Connect(target);
    if (!line.stream) {
        Respond("NO CARRIER");
        return;
    }
    line.dialing = true;
    line.dialDeadline = platform::Ticks() + DIAL_TIMEOUT;
}

// A Hayes command line: AT, then commands; D dials (the rest of the line is
// the number), A answers, H hangs up; everything else is accepted.
void Command(const std::string& text) {
    SerialLine& line = Line();
    std::string upper;
    for (char c : text)
        upper += static_cast<char>(toupper(static_cast<unsigned char>(c)));
    size_t start = upper.find_first_not_of(' ');
    if (start == std::string::npos || upper.compare(start, 2, "AT") != 0)
        return;
    for (size_t i = start + 2; i < upper.size(); i++) {
        char c = upper[i];
        if (c == '&' || c == '\\' || c == '%') {
            i++;
            while (i + 1 < upper.size() && isdigit(static_cast<unsigned char>(upper[i + 1])))
                i++;
        } else if (c == 'D') {
            Dial(text.substr(i + 1));
            return;
        } else if (c == 'A') {
            if (line.ringing) {
                line.stream = std::move(line.ringing);
                Connected();
            } else {
                Respond("NO CARRIER");
            }
            return;
        } else if (c == 'H') {
            line.stream.reset();
            line.dialing = false;
        }
    }
    Respond("OK");
}

void TypeCommand(const u8* data, u16 length) {
    SerialLine& line = Line();
    for (u16 i = 0; i < length; i++) {
        char c = static_cast<char>(data[i]);
        if (c == '\r') {
            std::string text = line.command;
            line.command.clear();
            Command(text);
            if (line.mode == LINE_DATA)
                return;
        } else if (c != '\n' && line.command.size() < 80) {
            line.command += c;
        }
    }
}

void Pump() {
    SerialLine& line = Line();
    if (!line.open)
        return;
    u32 now = platform::Ticks();
    // Incoming calls.
    if (line.listener) {
        while (std::unique_ptr<Stream> caller = line.listener->Accept()) {
            if (line.modem && line.mode == LINE_COMMAND && !line.ringing && !line.stream) {
                platform::Log("serial: incoming call from %s",
                              platform::net::ToString(caller->Peer()).c_str());
                line.ringing = std::move(caller);
                line.nextRing = now;
            } else if (!line.modem && !line.stream) {
                line.stream = std::move(caller);
                platform::Log("serial: connected to %s",
                              platform::net::ToString(line.stream->Peer()).c_str());
            }
        }
    }
    if (line.ringing) {
        if (!line.ringing->Pump()) {
            line.ringing.reset();
        } else if (static_cast<i32>(now - line.nextRing) >= 0) {
            Respond("RING");
            line.nextRing = now + RING_INTERVAL;
        }
    }
    // The direct guest's cable.
    if (!line.modem && !line.answers && !line.stream && static_cast<i32>(now - line.nextConnect) >= 0) {
        line.stream = Stream::Connect(line.peer);
        line.nextConnect = now + DIRECT_RETRY_DELAY;
    }
    if (!line.stream)
        return;
    bool alive = line.stream->Pump();
    if (line.dialing) {
        if (line.stream->Connected()) {
            Connected();
        } else if (!alive || line.stream->Failed() || static_cast<i32>(now - line.dialDeadline) >= 0) {
            line.stream.reset();
            line.dialing = false;
            Respond("NO CARRIER");
            return;
        } else {
            return;
        }
    }
    if (!line.stream->Connected() && !line.stream->Failed())
        return;
    if (!line.modem && !line.answers && line.stream->Connected() && !line.reported) {
        line.reported = true;
        platform::Log("serial: connected to %s",
                      platform::net::ToString(line.stream->Peer()).c_str());
    }
    if (line.mode == LINE_DATA) {
        u8 chunk[1024];
        size_t size;
        while ((size = line.stream->Read(chunk, sizeof(chunk))) > 0)
            line.incoming.insert(line.incoming.end(), chunk, chunk + size);
    }
    if (!alive || line.stream->Failed()) {
        if (line.modem || line.answers) {
            LostCarrier();
        } else {
            if (line.reported)
                platform::Log("serial: connection closed");
            line.reported = false;
            line.stream.reset();
        }
    }
}

i32 BaudRate(i32 code) {
    switch (code) {
        case COM_BAUD_2400:
            return COM_RATE_2400;
        case COM_BAUD_4800:
            return COM_RATE_4800;
        case COM_BAUD_9600:
            return COM_RATE_9600;
        case COM_BAUD_19200:
            return COM_RATE_19200;
        case COM_BAUD_38400:
            return COM_RATE_38400;
        default:
            return code;
    }
}

} // namespace

void ShutdownComError(char* function) {
    char message[COM_ERROR_MESSAGE_SIZE];
    snprintf(message, sizeof(message),
             "Communications error on function '%s'\n\n%s\n\nThe serial connection is a TCP "
             "connection: the host listens on port %u (--port), the guest connects to "
             "--join ADDRESS[:PORT].",
             function, gComFailure.c_str(), platform::net::CurrentSettings().port);
    ShutDown(message);
}

i16 com_init(u8 portNumber, i32 baudRate, i32) {
    SerialLine& line = Line();
    const platform::net::Settings& settings = platform::net::CurrentSettings();
    char function[40];
    com_term(0);
    line.open = true;
    line.modem = !gDirectConnect;
    line.answers = line.modem ? gRemoteGameMode == REMOTE_GAME_MODEM_GUEST
                              : gRemoteGameMode == REMOTE_GAME_MODEM_HOST;
    line.mode = line.modem ? LINE_COMMAND : LINE_DATA;
    line.baud = BaudRate(baudRate);
    line.nextConnect = platform::Ticks();
    snprintf(function, sizeof(function), "Opening COM%d", portNumber);
    if (line.answers) {
        line.listener = Listener::Open(settings.port);
        if (!line.listener) {
            gComFailure = "Port " + std::to_string(settings.port) + " is in use.";
            ShutdownComError(function);
            return -1;
        }
        platform::Log("serial: listening on port %u", settings.port);
    } else if (!line.modem) {
        if (settings.join.empty() || !platform::net::Resolve(settings.join, settings.port, line.peer)) {
            gComFailure = settings.join.empty()
                              ? std::string("No address to connect to (--join).")
                              : "Cannot resolve " + settings.join + ".";
            ShutdownComError(function);
            return -1;
        }
        platform::Log("serial: connecting to %s", platform::net::ToString(line.peer).c_str());
    }
    return 0;
}

void com_term(i16) {
    SerialLine& line = Line();
    if (line.stream)
        line.stream->Pump();
    line = SerialLine();
}

i16 com_rcv(i16, u16 requested, void* buffer) {
    SerialLine& line = Line();
    Pump();
    u16 count = 0;
    u8* bytes = static_cast<u8*>(buffer);
    while (count < requested && !line.incoming.empty()) {
        bytes[count++] = line.incoming.front();
        line.incoming.pop_front();
    }
    return static_cast<i16>(count);
}

i16 com_snd(i16, u16, u16 length, void* data, i32 priority) {
    SerialLine& line = Line();
    if (!line.open)
        return 1;
    // A break (no data) has no TCP counterpart.
    if (!length)
        return 0;
    const u8* bytes = static_cast<const u8*>(data);
    if (line.modem && line.mode == LINE_COMMAND) {
        TypeCommand(bytes, length);
        return 0;
    }
    std::vector<u8> node(bytes, bytes + length);
    if (priority)
        line.priority.push_back(node);
    else
        line.normal.push_back(node);
    return 0;
}

i16 com_sess(i32, i32, ...) {
    return 0;
}

u8 com_stat(i16, u16) {
    SerialLine& line = Line();
    Pump();
    return line.open && line.mode == LINE_DATA && line.stream && line.stream->Connected() ? 1 : 0;
}

void comm_wrt_task(void) {
    SerialLine& line = Line();
    if (!line.open)
        return;
    Pump();
    bool connected = line.mode == LINE_DATA && line.stream && line.stream->Connected();
    while (!line.priority.empty() || !line.normal.empty()) {
        std::deque<std::vector<u8> >& queue = line.priority.empty() ? line.normal : line.priority;
        if (connected)
            line.stream->Write(queue.front().data(), queue.front().size());
        queue.pop_front();
    }
    if (connected)
        line.stream->Pump();
}
