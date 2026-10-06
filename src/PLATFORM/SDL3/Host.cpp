#include "Internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The document is in the program's in-memory file system, which the browser
// cannot open by URL: its bytes go to a Blob shown in a new tab. The page
// (src/PLATFORM/Web/homm1.js) does it, and leaves a link on the page when
// the browser blocks the new tab.
EM_JS(int, OpenDocumentInBrowser, (const char* path), {
    try {
        const bytes = FS.readFile(UTF8ToString(path));
        if (Module['homm1OpenDocument']) {
            Module['homm1OpenDocument'](bytes, 'text/html');
            return 1;
        }
        const url = URL.createObjectURL(new Blob([bytes], {type: 'text/html'}));
        return window.open(url, '_blank') ? 1 : 0;
    } catch (e) {
        console.error('open document', e);
        return 0;
    }
});
#endif

namespace platform {

namespace {

bool gStarted = false;
Uint64 gTickBase = 0;

#if defined(_WIN32)
// The programs are Windows GUI programs, which start without a console. The
// log goes where the standard error already points (a redirection), else to
// the console of the command prompt that started the program, else to
// homm1.log in the settings folder.
void OpenLog() {
    HANDLE error = GetStdHandle(STD_ERROR_HANDLE);
    if (error != nullptr && error != INVALID_HANDLE_VALUE)
        return;
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (std::freopen("CONOUT$", "w", stderr) != nullptr)
            return;
    }
    std::string path = ConfigDirectory() + "homm1.log";
    int length = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wide(static_cast<size_t>(length > 0 ? length : 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide.data(), length);
    if (_wfreopen(wide.c_str(), L"w", stderr) != nullptr)
        std::setvbuf(stderr, nullptr, _IONBF, 0);
}
#endif

}  // namespace

bool Startup(const StartupOptions& options) {
    (void)options;
    if (gStarted)
        return true;
#if defined(_WIN32)
    OpenLog();
#endif
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    gTickBase = SDL_GetTicks();
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

namespace {

// $HOMM1_CONFIG (for tests and portable installs), with a trailing separator
// and created, or empty.
std::string OverriddenConfigDirectory() {
    std::string directory = Environment("HOMM1_CONFIG");
    if (directory.empty())
        return directory;
    if (directory.back() != '/' && directory.back() != '\\')
        directory += '/';
    SDL_CreateDirectory(directory.c_str());
    return directory;
}

}  // namespace

#if defined(_WIN32)

// %APPDATA%\homm1\, created by SDL.
std::string ConfigDirectory() {
    std::string overridden = OverriddenConfigDirectory();
    if (!overridden.empty())
        return overridden;
    char* path = SDL_GetPrefPath(nullptr, "homm1");
    if (path == nullptr)
        return std::string(".\\");
    std::string directory = path;
    SDL_free(path);
    return directory;
}

#else

std::string ConfigDirectory() {
    std::string directory = OverriddenConfigDirectory();
    if (!directory.empty())
        return directory;
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

#endif

std::string Environment(const char* name) {
#if defined(_WIN32)
    // SDL keeps a UTF-8 copy of Windows' UTF-16 environment.
    const char* value = SDL_getenv(name);
#else
    const char* value = std::getenv(name);
#endif
    return value != nullptr ? std::string(value) : std::string();
}

u32 Ticks() {
    Uint64 elapsed = SDL_GetTicks() - gTickBase;
    return static_cast<u32>(1000000u + elapsed);
}

void Sleep(u32 milliseconds) {
    // SDL_Delay returns to the browser while it waits (ASYNCIFY).
    SDL_Delay(milliseconds);
}

namespace sdl {

void YieldToBrowser() {
#ifdef __EMSCRIPTEN__
    // A frame at 60 Hz.
    constexpr u32 kYieldInterval = 16;
    static u32 gLastYield = 0;
    u32 now = Ticks();
    if (now - gLastYield < kYieldInterval)
        return;
    Present(true);
    emscripten_sleep(0);
    gLastYield = Ticks();
#endif
}

}  // namespace sdl

void ShowMessage(const char* title, const char* text) {
    std::string utf8Title = ToUtf8(title);
    std::string utf8Text = ToUtf8(text);
    Log("%s: %s", utf8Title.c_str(), utf8Text.c_str());
    if (Environment("HOMM1_NO_DIALOGS").empty())
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, utf8Title.c_str(), utf8Text.c_str(), sdl::Window());
}

bool OpenDocument(const std::string& hostPath) {
#ifdef __EMSCRIPTEN__
    return OpenDocumentInBrowser(hostPath.c_str()) != 0;
#else
    // A file URL of the absolute path, with the bytes outside the unreserved
    // set percent-encoded (the path is UTF-8).
    std::error_code error;
    std::filesystem::path absolute = std::filesystem::absolute(hostPath, error);
    std::string path = error ? hostPath : absolute.generic_string();
    std::string url = "file://";
    if (!path.empty() && path[0] != '/')
        url += '/';  // a Windows drive letter
    static const char kHex[] = "0123456789ABCDEF";
    for (char c : path) {
        unsigned char byte = static_cast<unsigned char>(c);
        if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')
            || (byte >= '0' && byte <= '9') || c == '/' || c == '-' || c == '_' || c == '.'
            || c == '~' || c == ':') {
            url += c;
        } else {
            url += '%';
            url += kHex[byte >> 4];
            url += kHex[byte & 15];
        }
    }
    if (!SDL_OpenURL(url.c_str())) {
        Log("cannot open %s: %s", url.c_str(), SDL_GetError());
        return false;
    }
    return true;
#endif
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
