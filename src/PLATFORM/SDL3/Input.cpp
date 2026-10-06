#include "Internal.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

namespace platform {

namespace {

// SDL's physical key positions as PC Set 1 scan codes. Extended keys share
// the code of their numeric keypad or left-hand twin, as the Windows original
// read them (it masks the extended flag away).
int SetOneScanCode(SDL_Scancode code) {
    switch (code) {
        case SDL_SCANCODE_ESCAPE: return 0x01;
        case SDL_SCANCODE_1: return 0x02;
        case SDL_SCANCODE_2: return 0x03;
        case SDL_SCANCODE_3: return 0x04;
        case SDL_SCANCODE_4: return 0x05;
        case SDL_SCANCODE_5: return 0x06;
        case SDL_SCANCODE_6: return 0x07;
        case SDL_SCANCODE_7: return 0x08;
        case SDL_SCANCODE_8: return 0x09;
        case SDL_SCANCODE_9: return 0x0a;
        case SDL_SCANCODE_0: return 0x0b;
        case SDL_SCANCODE_MINUS: return 0x0c;
        case SDL_SCANCODE_EQUALS: return 0x0d;
        case SDL_SCANCODE_BACKSPACE: return 0x0e;
        case SDL_SCANCODE_TAB: return 0x0f;
        case SDL_SCANCODE_Q: return 0x10;
        case SDL_SCANCODE_W: return 0x11;
        case SDL_SCANCODE_E: return 0x12;
        case SDL_SCANCODE_R: return 0x13;
        case SDL_SCANCODE_T: return 0x14;
        case SDL_SCANCODE_Y: return 0x15;
        case SDL_SCANCODE_U: return 0x16;
        case SDL_SCANCODE_I: return 0x17;
        case SDL_SCANCODE_O: return 0x18;
        case SDL_SCANCODE_P: return 0x19;
        case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
        case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
        case SDL_SCANCODE_RETURN: return 0x1c;
        case SDL_SCANCODE_KP_ENTER: return 0x1c;
        case SDL_SCANCODE_LCTRL: return 0x1d;
        case SDL_SCANCODE_RCTRL: return 0x1d;
        case SDL_SCANCODE_A: return 0x1e;
        case SDL_SCANCODE_S: return 0x1f;
        case SDL_SCANCODE_D: return 0x20;
        case SDL_SCANCODE_F: return 0x21;
        case SDL_SCANCODE_G: return 0x22;
        case SDL_SCANCODE_H: return 0x23;
        case SDL_SCANCODE_J: return 0x24;
        case SDL_SCANCODE_K: return 0x25;
        case SDL_SCANCODE_L: return 0x26;
        case SDL_SCANCODE_SEMICOLON: return 0x27;
        case SDL_SCANCODE_APOSTROPHE: return 0x28;
        case SDL_SCANCODE_GRAVE: return 0x29;
        case SDL_SCANCODE_LSHIFT: return 0x2a;
        case SDL_SCANCODE_BACKSLASH: return 0x2b;
        case SDL_SCANCODE_Z: return 0x2c;
        case SDL_SCANCODE_X: return 0x2d;
        case SDL_SCANCODE_C: return 0x2e;
        case SDL_SCANCODE_V: return 0x2f;
        case SDL_SCANCODE_B: return 0x30;
        case SDL_SCANCODE_N: return 0x31;
        case SDL_SCANCODE_M: return 0x32;
        case SDL_SCANCODE_COMMA: return 0x33;
        case SDL_SCANCODE_PERIOD: return 0x34;
        case SDL_SCANCODE_SLASH: return 0x35;
        case SDL_SCANCODE_KP_DIVIDE: return 0x35;
        case SDL_SCANCODE_RSHIFT: return 0x36;
        case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
        case SDL_SCANCODE_PRINTSCREEN: return 0x37;
        case SDL_SCANCODE_LALT: return 0x38;
        case SDL_SCANCODE_RALT: return 0x38;
        case SDL_SCANCODE_SPACE: return 0x39;
        case SDL_SCANCODE_CAPSLOCK: return 0x3a;
        case SDL_SCANCODE_F1: return 0x3b;
        case SDL_SCANCODE_F2: return 0x3c;
        case SDL_SCANCODE_F3: return 0x3d;
        case SDL_SCANCODE_F4: return 0x3e;
        case SDL_SCANCODE_F5: return 0x3f;
        case SDL_SCANCODE_F6: return 0x40;
        case SDL_SCANCODE_F7: return 0x41;
        case SDL_SCANCODE_F8: return 0x42;
        case SDL_SCANCODE_F9: return 0x43;
        case SDL_SCANCODE_F10: return 0x44;
        case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
        case SDL_SCANCODE_SCROLLLOCK: return 0x46;
        case SDL_SCANCODE_KP_7: return 0x47;
        case SDL_SCANCODE_HOME: return 0x47;
        case SDL_SCANCODE_KP_8: return 0x48;
        case SDL_SCANCODE_UP: return 0x48;
        case SDL_SCANCODE_KP_9: return 0x49;
        case SDL_SCANCODE_PAGEUP: return 0x49;
        case SDL_SCANCODE_KP_MINUS: return 0x4a;
        case SDL_SCANCODE_KP_4: return 0x4b;
        case SDL_SCANCODE_LEFT: return 0x4b;
        case SDL_SCANCODE_KP_5: return 0x4c;
        case SDL_SCANCODE_KP_6: return 0x4d;
        case SDL_SCANCODE_RIGHT: return 0x4d;
        case SDL_SCANCODE_KP_PLUS: return 0x4e;
        case SDL_SCANCODE_KP_1: return 0x4f;
        case SDL_SCANCODE_END: return 0x4f;
        case SDL_SCANCODE_KP_2: return 0x50;
        case SDL_SCANCODE_DOWN: return 0x50;
        case SDL_SCANCODE_KP_3: return 0x51;
        case SDL_SCANCODE_PAGEDOWN: return 0x51;
        case SDL_SCANCODE_KP_0: return 0x52;
        case SDL_SCANCODE_INSERT: return 0x52;
        case SDL_SCANCODE_KP_PERIOD: return 0x53;
        case SDL_SCANCODE_DELETE: return 0x53;
        case SDL_SCANCODE_NONUSBACKSLASH: return 0x56;
        case SDL_SCANCODE_F11: return 0x57;
        case SDL_SCANCODE_F12: return 0x58;
        default: return 0;
    }
}

// ---------------------------------------------------------------- replay

struct ReplayAction {
    u32 time = 0;
    std::string verb;
    std::vector<std::string> arguments;
    int line = 0;
};

std::deque<ReplayAction> gReplay;
bool gReplayActive = false;
bool gReplayStarted = false;
u32 gReplayStart = 0;
std::deque<Event> gPending;
int gReplayX = DISPLAY_WIDTH / 2;
int gReplayY = DISPLAY_HEIGHT / 2;

int NamedScanCode(const std::string& name) {
    static const struct {
        const char* name;
        int code;
    } kNames[] = {
        {"escape", 0x01}, {"return", 0x1c}, {"enter", 0x1c}, {"space", 0x39},
        {"tab", 0x0f}, {"backspace", 0x0e}, {"up", 0x48}, {"down", 0x50},
        {"left", 0x4b}, {"right", 0x4d}, {"home", 0x47}, {"end", 0x4f},
        {"pageup", 0x49}, {"pagedown", 0x51}, {"lshift", 0x2a}, {"ctrl", 0x1d},
        {"alt", 0x38}, {"f1", 0x3b}, {"f2", 0x3c}, {"f3", 0x3d}, {"f4", 0x3e},
        {"f5", 0x3f}, {"f6", 0x40}, {"f7", 0x41}, {"f8", 0x42}, {"f9", 0x43},
        {"f10", 0x44}, {"f11", 0x57}, {"f12", 0x58},
    };
    std::string lower;
    for (char c : name)
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (const auto& entry : kNames) {
        if (lower == entry.name)
            return entry.code;
    }
    if (lower.size() == 1) {
        static const char kLetters[] = "qwertyuiopasdfghjklzxcvbnm";
        static const int kLetterCodes[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                                           0x19, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                           0x26, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32};
        const char* found = std::strchr(kLetters, lower[0]);
        if (found != nullptr && lower[0] != '\0')
            return kLetterCodes[found - kLetters];
        if (lower[0] >= '1' && lower[0] <= '9')
            return lower[0] - '1' + 0x02;
        if (lower[0] == '0')
            return 0x0b;
    }
    return static_cast<int>(std::strtol(name.c_str(), nullptr, 0));
}

void QueueMouse(Event::Type type, Event::Button button, int x, int y) {
    gReplayX = std::clamp(x, 0, DISPLAY_WIDTH - 1);
    gReplayY = std::clamp(y, -ChromeBar(), DISPLAY_HEIGHT - 1);
    sdl::SetPointer(gReplayX, gReplayY, true);
    Event event;
    event.type = type;
    event.button = button;
    event.x = gReplayX;
    event.y = gReplayY;
    gPending.push_back(event);
}

void QueueKey(Event::Type type, int code) {
    Event event;
    event.type = type;
    event.scanCode = code;
    event.returnKey = code == 0x1c;
    gPending.push_back(event);
}

int Argument(const ReplayAction& action, size_t index) {
    return index < action.arguments.size() ? std::atoi(action.arguments[index].c_str()) : 0;
}

void RunReplayAction(const ReplayAction& action) {
    Log("replay %u: %s", action.time, action.verb.c_str());
    int x = Argument(action, 0);
    int y = Argument(action, 1);
    if (action.verb == "move") {
        QueueMouse(Event::MOUSE_MOVE, Event::BUTTON_LEFT, x, y);
    } else if (action.verb == "left-down") {
        QueueMouse(Event::MOUSE_DOWN, Event::BUTTON_LEFT, x, y);
    } else if (action.verb == "left-up") {
        QueueMouse(Event::MOUSE_UP, Event::BUTTON_LEFT, x, y);
    } else if (action.verb == "right-down") {
        QueueMouse(Event::MOUSE_DOWN, Event::BUTTON_RIGHT, x, y);
    } else if (action.verb == "right-up") {
        QueueMouse(Event::MOUSE_UP, Event::BUTTON_RIGHT, x, y);
    } else if (action.verb == "click") {
        QueueMouse(Event::MOUSE_MOVE, Event::BUTTON_LEFT, x, y);
        QueueMouse(Event::MOUSE_DOWN, Event::BUTTON_LEFT, x, y);
        QueueMouse(Event::MOUSE_UP, Event::BUTTON_LEFT, x, y);
    } else if (action.verb == "key-down" || action.verb == "key-up" || action.verb == "key") {
        int code = action.arguments.empty() ? 0 : NamedScanCode(action.arguments[0]);
        if (action.verb != "key-up")
            QueueKey(Event::KEY_DOWN, code);
        if (action.verb != "key-down")
            QueueKey(Event::KEY_UP, code);
    } else if (action.verb == "shot") {
        Present(true);
        std::string path = action.arguments.empty() ? "screen.bmp" : action.arguments[0];
        if (!SaveDisplayBmp(path.c_str()))
            Log("replay: cannot write %s", path.c_str());
    } else if (action.verb == "check") {
        // The display against the game's own picture; with a path, a
        // difference also saves a screenshot there.
        int box[4];
        int differing = CompareWithReference(box);
        if (differing == 0) {
            Log("replay check: display matches the game image");
        } else {
            Log("replay check: %d pixels differ in %d,%d-%d,%d", differing, box[0], box[1], box[2],
                box[3]);
            if (!action.arguments.empty()) {
                SaveDisplayBmp(action.arguments[0].c_str());
                SaveReferenceBmp((action.arguments[0] + ".game.bmp").c_str());
            }
        }
    } else if (action.verb == "quit") {
        Event event;
        event.type = Event::QUIT;
        gPending.push_back(event);
    } else if (action.verb == "exit") {
        Log("replay: exit");
        std::exit(0);
    } else {
        Log("replay line %d: unknown action %s", action.line, action.verb.c_str());
    }
}

void PumpReplay() {
    if (!gReplayActive)
        return;
    u32 now = Ticks();
    if (!gReplayStarted) {
        gReplayStarted = true;
        gReplayStart = now;
    }
    while (!gReplay.empty() && now - gReplayStart >= gReplay.front().time) {
        ReplayAction action = gReplay.front();
        gReplay.pop_front();
        RunReplayAction(action);
    }
}

bool Translate(const SDL_Event& source, Event& event) {
    switch (source.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            event.type = Event::QUIT;
            return true;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            event.type = Event::FOCUS_GAINED;
            return true;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            event.type = Event::FOCUS_LOST;
            return true;
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            sdl::MarkDisplayDirty();
            Present(true);
            return false;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            if (gReplayActive)
                return false;
            int code = SetOneScanCode(source.key.scancode);
            if (code == 0)
                return false;
            event.type = source.type == SDL_EVENT_KEY_DOWN ? Event::KEY_DOWN : Event::KEY_UP;
            event.scanCode = code;
            event.returnKey = source.key.scancode == SDL_SCANCODE_RETURN
                              || source.key.scancode == SDL_SCANCODE_KP_ENTER;
            return true;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            if (gReplayActive)
                return false;
            bool inside;
            sdl::WindowToDisplay(source.motion.x, source.motion.y, event.x, event.y, inside);
            sdl::SetPointer(event.x, event.y, inside);
            event.type = Event::MOUSE_MOVE;
            return true;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (gReplayActive)
                return false;
            if (source.button.button != SDL_BUTTON_LEFT && source.button.button != SDL_BUTTON_RIGHT)
                return false;
            bool inside;
            sdl::WindowToDisplay(source.button.x, source.button.y, event.x, event.y, inside);
            sdl::SetPointer(event.x, event.y, inside);
            event.type =
                source.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? Event::MOUSE_DOWN : Event::MOUSE_UP;
            event.button =
                source.button.button == SDL_BUTTON_LEFT ? Event::BUTTON_LEFT : Event::BUTTON_RIGHT;
            event.doubleClick = source.type == SDL_EVENT_MOUSE_BUTTON_DOWN && source.button.clicks == 2;
            return true;
        }
        case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
            int x;
            int y;
            bool inside;
            sdl::PointerPosition(x, y, inside);
            if (!gReplayActive)
                sdl::SetPointer(x, y, false);
            return false;
        }
        default:
            return false;
    }
}

}  // namespace

bool PollEvent(Event& event, u32 timeoutMilliseconds) {
    sdl::YieldToBrowser();
    PumpReplay();
    if (!gPending.empty()) {
        event = gPending.front();
        gPending.pop_front();
        return true;
    }
    SDL_Event source;
    bool waited = false;
    for (;;) {
        bool got = timeoutMilliseconds > 0 && !waited
                       ? SDL_WaitEventTimeout(&source, static_cast<Sint32>(timeoutMilliseconds))
                       : SDL_PollEvent(&source);
        waited = true;
        if (!got)
            break;
        event = Event();
        if (Translate(source, event))
            return true;
    }
    Present(false);
    return false;
}

void MousePosition(int& x, int& y) {
    bool inside;
    if (gReplayActive) {
        x = gReplayX;
        y = gReplayY;
        return;
    }
    sdl::PointerPosition(x, y, inside);
}

void CaptureMouse(bool capture) {
    if (!gReplayActive)
        SDL_CaptureMouse(capture);
}

// Each line: a millisecond offset from the first event poll, an action and
// its arguments. Blank lines and lines starting with # are skipped.
void LoadInputReplay(const char* hostPath) {
    // SDL opens UTF-8 host paths on every host.
    size_t size = 0;
    void* contents = SDL_LoadFile(hostPath, &size);
    if (contents == nullptr) {
        Log("cannot read input replay %s", hostPath);
        return;
    }
    std::istringstream file(std::string(static_cast<const char*>(contents), size));
    SDL_free(contents);
    std::string text;
    int number = 0;
    u32 previous = 0;
    while (std::getline(file, text)) {
        number++;
        if (!text.empty() && text.back() == '\r')
            text.pop_back();
        std::istringstream line(text);
        ReplayAction action;
        action.line = number;
        std::string time;
        if (!(line >> time) || time[0] == '#')
            continue;
        // "+N" is relative to the previous action.
        if (time[0] == '+')
            action.time = previous + static_cast<u32>(std::stoul(time.substr(1)));
        else
            action.time = static_cast<u32>(std::stoul(time));
        previous = action.time;
        line >> action.verb;
        std::string argument;
        while (line >> argument)
            action.arguments.push_back(argument);
        gReplay.push_back(action);
    }
    std::stable_sort(gReplay.begin(), gReplay.end(), [](const ReplayAction& a, const ReplayAction& b) {
        return a.time < b.time;
    });
    gReplayActive = true;
    Log("input replay: %zu actions from %s", gReplay.size(), hostPath);
}

}  // namespace platform
