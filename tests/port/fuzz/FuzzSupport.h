// Shared pieces of the file parser fuzz harnesses (tests/port/fuzz).
//
// Each harness defines LLVMFuzzerTestOneInput (and LLVMFuzzerInitialize) and
// is built twice: as a libFuzzer program with -DHOMM1_FUZZERS=ON, and always
// as a replay program (ReplayMain.cpp) that feeds it the checked-in
// regression inputs and, with $HOMM1_DATA, the shipped files.

#ifndef HOMM1_TESTS_FUZZ_SUPPORT_H
#define HOMM1_TESTS_FUZZ_SUPPORT_H

#include <H1/Ints.h>

#include <cstddef>
#include <cstdint>
#include <string>

// The game's error exits (FileError, ShutDown and the asserts that end in
// ShutDown) clean up and end the process. The harnesses that link the game's
// units wrap them (FuzzWrap.cpp, linked with --wrap) to throw this instead,
// so that refusing a file through the game's own error path is an ordinary
// outcome of one input.
struct GameExit {
    std::string message;
};

// Set when the game refused the current input through its error path; the
// replay program checks it for inputs that must be accepted.
extern bool gFuzzRejected;
// The game's message for the last refusal.
extern std::string gFuzzRejection;

// Runs `body`; returns false (and sets gFuzzRejected) when the game left it
// through its error path.
template <class Body>
bool RunGame(Body body) {
    try {
        body();
        return true;
    } catch (const GameExit& exit) {
        gFuzzRejected = true;
        gFuzzRejection = exit.message;
        return false;
    }
}

// $HOMM1_DATA, or empty.
std::string FuzzDataRoot();

// Exits with the ctest skip code 77 when $HOMM1_DATA is not set.
void FuzzRequireData(const char* harness);

// A scratch game folder in $TMPDIR: DATA linked to the game data's, empty
// writable MAPS and GAMES. Removed at exit.
std::string FuzzScratchGame();

// Removes the scratch folder (also done at exit()).
void FuzzCleanup();

// Writes a host file.
bool FuzzWriteFile(const std::string& path, const uint8_t* data, size_t size);

// Starts the program (game or editor, whichever units are linked) headless on
// `root` with SDL's dummy drivers, as editor_maps_test does, and initializes
// its managers. Messages that would be dialog boxes go to the log.
void FuzzStartHost(const std::string& root);

#endif
