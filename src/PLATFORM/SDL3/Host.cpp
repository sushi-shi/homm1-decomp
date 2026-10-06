#include "Internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sys/stat.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The document is in the program's in-memory file system, which the browser
// cannot open by URL: its bytes go to a Blob shown in a new tab.
EM_JS(int, OpenDocumentInBrowser, (const char* path), {
    try {
        const bytes = FS.readFile(UTF8ToString(path));
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
    Uint64 elapsed = SDL_GetTicks() - gTickBase;
    return static_cast<u32>(1000000u + elapsed);
}

void Sleep(u32 milliseconds) {
    SDL_Delay(milliseconds);
}

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
