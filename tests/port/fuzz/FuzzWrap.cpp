// The game's error exits, wrapped for the harnesses that link the game's or
// the editor's units (CMake links them with --wrap for these symbols): the
// original functions clean up and call exit(), which would end the fuzzer.

#include "FuzzSupport.h"

#include <BASE/executive.h>
#include <SOURCE/KB.h>

#include <cstdio>
#include <cstdlib>

bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen);

extern "C" {

// FileError(char*)
[[noreturn]] void __wrap__Z9FileErrorPc(char* filename) {
    throw GameExit{std::string("file error: ") + (filename != nullptr ? filename : "")};
}

// ShutDown(char*), which the asserts and the archive errors end in.
[[noreturn]] void __wrap__Z8ShutDownPc(char* message) {
    throw GameExit{std::string("shut down: ") + (message != nullptr ? message : "")};
}

// MemError()
[[noreturn]] void __wrap__Z8MemErrorv() {
    throw GameExit{"memory error"};
}

}  // extern "C"

void FuzzStartHost(const std::string& root) {
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    setenv("SDL_AUDIO_DRIVER", "dummy", 0);
    setenv("HOMM1_NO_DIALOGS", "1", 1);
    setenv("XDG_CONFIG_HOME", root.c_str(), 1);
    bool started = false;
    RunGame([&] { started = KBStartHost(root.c_str(), "/I0", 0) && gExec->InitSystem() == 0; });
    if (!started) {
        std::fprintf(stderr, "the program did not start on %s\n", root.c_str());
        std::_Exit(1);
    }
    gFuzzRejected = false;
}
