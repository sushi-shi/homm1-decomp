#include "Internal.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

namespace platform {

namespace {

bool gStarted = false;
Uint64 gTickBase = 0;
// Test controls (docs/port/README.md, unattended runs): the clock can run
// faster than real time and start at another count.
u32 gTickStart = 1000000u;
u32 gTimeScale = 1u;

}  // namespace

bool Startup(const StartupOptions& options) {
    (void)options;
    if (gStarted)
        return true;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    gTickBase = SDL_GetTicks();
    std::string tickStart = Environment("HOMM1_TICK_START");
    if (!tickStart.empty())
        gTickStart = static_cast<u32>(std::strtoul(tickStart.c_str(), nullptr, 0));
    std::string timeScale = Environment("HOMM1_TIME_SCALE");
    if (!timeScale.empty())
        gTimeScale = static_cast<u32>(std::max(1ul, std::strtoul(timeScale.c_str(), nullptr, 0)));
    gStarted = true;
    std::string replay = Environment("HOMM1_INPUT_REPLAY");
    if (!replay.empty())
        LoadInputReplay(replay.c_str());
    return true;
}

void Shutdown() {
    if (!gStarted)
        return;
    CloseAudio();
    CloseDisplay();
    SDL_Quit();
    gStarted = false;
}

std::string ExecutableDirectory() {
    const char* base = SDL_GetBasePath();
    return base != nullptr ? std::string(base) : std::string("./");
}

std::string ConfigDirectory() {
    std::string directory;
    std::string xdg = Environment("XDG_CONFIG_HOME");
    std::string home = Environment("HOME");
    if (!xdg.empty())
        directory = xdg + "/homm1/";
    else if (!home.empty())
        directory = home + "/.config/homm1/";
    else
        directory = "./";
    std::string partial;
    for (char c : directory) {
        partial += c;
        if (c == '/')
            mkdir(partial.c_str(), 0755);
    }
    return directory;
}

std::string Environment(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : std::string();
}

u32 Ticks() {
    Uint64 elapsed = (SDL_GetTicks() - gTickBase) * gTimeScale;
    return static_cast<u32>(gTickStart + elapsed);
}

void Sleep(u32 milliseconds) {
    SDL_Delay(milliseconds / gTimeScale);
}

void ShowMessage(const char* title, const char* text) {
    std::string utf8Title = ToUtf8(title);
    std::string utf8Text = ToUtf8(text);
    Log("%s: %s", utf8Title.c_str(), utf8Text.c_str());
    if (Environment("HOMM1_NO_DIALOGS").empty())
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, utf8Title.c_str(), utf8Text.c_str(), sdl::Window());
}

void Log(const char* format, ...) {
    std::va_list arguments;
    va_start(arguments, format);
    std::fputs("[homm1] ", stderr);
    std::vfprintf(stderr, format, arguments);
    std::fputc('\n', stderr);
    va_end(arguments);
}

}  // namespace platform
