// Finding the game data the way a player sets it up: the program copied into
// the game folder and started from anywhere, HOMM1_DATA typed into a shell
// (cmd keeps the quotes), a trailing separator, a folder with a non-ASCII
// name, and file names in another case than the game asks for. Runs natively
// and, for the Windows build, under Wine (the windows-tests flake check).

#include <H1/Ints.h>

#include <PLATFORM/File.h>
#include <PLATFORM/GameData.h>
#include <PLATFORM/Platform.h>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace fs = std::filesystem;

int gFailures = 0;

void Expect(bool condition, const std::string& what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what.c_str());
        gFailures++;
    }
}

void ExpectEqual(const std::string& got, const std::string& expected, const std::string& what) {
    Expect(got == expected, what + ": got '" + got + "', expected '" + expected + "'");
}

// Host paths are UTF-8 strings in the platform layer.
fs::path HostPath(const std::string& text) {
    return fs::path(std::u8string(text.begin(), text.end()));
}

std::string HostString(const fs::path& path) {
    std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

void Touch(const fs::path& path) {
    std::error_code error;
    fs::create_directories(path.parent_path(), error);
    std::ofstream(path, std::ios::binary) << "x";
}

void SetVariable(const char* name, const std::string* value) {
#if defined(_WIN32)
    // The platform layer reads SDL's copy of the environment on Windows.
    SDL_Environment* environment = SDL_GetEnvironment();
    if (value != nullptr)
        SDL_SetEnvironmentVariable(environment, name, value->c_str(), true);
    else
        SDL_UnsetEnvironmentVariable(environment, name);
#else
    if (value != nullptr)
        setenv(name, value->c_str(), 1);
    else
        unsetenv(name);
#endif
}

#if defined(_WIN32)
const char kSeparator = '\\';
#else
const char kSeparator = '/';
#endif

void TestConfiguredDirectory() {
    using platform::ConfiguredDirectory;
    ExpectEqual(ConfiguredDirectory(""), "", "empty");
    ExpectEqual(ConfiguredDirectory("   "), "", "blanks only");
    ExpectEqual(ConfiguredDirectory("/games/heroes"), "/games/heroes", "plain");
    ExpectEqual(ConfiguredDirectory("  /games/heroes \t"), "/games/heroes", "surrounding blanks");
    ExpectEqual(ConfiguredDirectory("\"/games/my heroes\""), "/games/my heroes", "quotes");
    ExpectEqual(ConfiguredDirectory(" \" /games/heroes/ \" "), "/games/heroes",
                "quotes, blanks inside them and a trailing separator");
    ExpectEqual(ConfiguredDirectory("\"\"/games\"\""), "\"/games\"", "only one pair of quotes");
    ExpectEqual(ConfiguredDirectory("\"/games"), "\"/games", "an unmatched quote stays");
    ExpectEqual(ConfiguredDirectory("/games/heroes//"), "/games/heroes", "trailing separators");
    ExpectEqual(ConfiguredDirectory("/"), "/", "the root keeps its separator");
    ExpectEqual(ConfiguredDirectory("\"/\""), "/", "a quoted root");
#if defined(_WIN32)
    ExpectEqual(ConfiguredDirectory("\"C:\\Games\\Heroes\\\""), "C:\\Games\\Heroes",
                "cmd's set HOMM1_DATA=\"C:\\...\\\"");
    ExpectEqual(ConfiguredDirectory("C:\\"), "C:\\", "a drive root keeps its separator");
    ExpectEqual(ConfiguredDirectory("\"C:\\\""), "C:\\", "a quoted drive root");
    ExpectEqual(ConfiguredDirectory("C:/Games/"), "C:/Games", "forward separators");
#else
    ExpectEqual(ConfiguredDirectory("/games/back\\"), "/games/back\\",
                "a backslash is a file name character here");
#endif
}

}  // namespace

int main() {
    TestConfiguredDirectory();

    std::error_code error;
    const std::string program = platform::ExecutableDirectory();
    const fs::path programPath = HostPath(program);
    const fs::path scratch =
        fs::temp_directory_path(error)
        / HostPath("homm1-data-root-тест-" + std::to_string(std::random_device()()));
    const fs::path elsewhere = scratch / "elsewhere";
    // An installation below a non-ASCII name, its files in lowercase.
    const fs::path other = scratch / HostPath("Герои Меча и Магии");
    Touch(other / "data" / "heroes.agg");
    fs::create_directories(elsewhere, error);
    const std::string otherText = HostString(other);

    // Nothing from the real user folders.
    const std::string state = HostString(scratch / "state");
    SetVariable("XDG_DATA_HOME", &state);
    SetVariable("LOCALAPPDATA", &state);
    SetVariable("HOME", &state);
    SetVariable("HOMM1_DATA", nullptr);

    std::vector<std::string> searched;

    // No data anywhere: every folder tried is listed.
    fs::current_path(elsewhere, error);
    ExpectEqual(platform::FindGameRoot("", searched), "", "no data");
    Expect(searched.size() >= 3 && searched[0] == program && searched[1] == ".",
           "the folders searched start with the program's and the current folder");

    // A player's folder: the program beside DATA, the archive in uppercase,
    // started from another folder and from the game folder.
    Touch(programPath / "DATA" / "HEROES.AGG");
    ExpectEqual(platform::FindGameRoot("", searched), program, "started from another folder");
    fs::current_path(programPath, error);
    ExpectEqual(platform::FindGameRoot("", searched), program, "started from the game folder");
    fs::current_path(elsewhere, error);

    // HOMM1_DATA as cmd keeps it, with quotes; then with a trailing separator
    // and a blank.
    const std::string quoted = "\"" + otherText + "\"";
    SetVariable("HOMM1_DATA", &quoted);
    ExpectEqual(platform::FindGameRoot("", searched), otherText, "quoted HOMM1_DATA");
    const std::string trailing = otherText + kSeparator + " ";
    SetVariable("HOMM1_DATA", &trailing);
    ExpectEqual(platform::FindGameRoot("", searched), otherText,
                "HOMM1_DATA with a trailing separator");
    // --data comes first, taken the same way.
    ExpectEqual(platform::FindGameRoot(" " + quoted + " ", searched), otherText, "quoted --data");
    SetVariable("HOMM1_DATA", nullptr);

    // A file the game asks for in another case than it is stored in, below a
    // non-ASCII folder name.
    FileSetRoot(otherText.c_str());
    char resolved[FILE_PATH_CAPACITY];
    Expect(FileResolve(".\\DATA\\HEROES.AGG", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "an uppercase request finds the lowercase file");
    i32 file = FileOpen(".\\DATA\\HEROES.AGG", FILE_OPEN_READ);
    Expect(file != FILE_INVALID, "and opens it");
    if (file != FILE_INVALID)
        FileClose(file);
    FileSetRoot(program.c_str());
    file = FileOpen(".\\data\\heroes.agg", FILE_OPEN_READ);
    Expect(file != FILE_INVALID, "a lowercase request opens the uppercase archive");
    if (file != FILE_INVALID)
        FileClose(file);

    fs::current_path(programPath, error);
    fs::remove_all(scratch, error);
    fs::remove_all(programPath / "DATA", error);
    if (gFailures == 0)
        std::printf("data root: all checks passed\n");
    return gFailures == 0 ? 0 : 1;
}
