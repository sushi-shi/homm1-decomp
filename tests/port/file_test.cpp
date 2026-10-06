// Game paths resolve case-insensitively under the game folder, never leave
// it, and directory listings match wildcards and come back sorted.

#include <H1/Ints.h>

#include <PLATFORM/File.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace {

int gFailures = 0;

void Expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        gFailures++;
    }
}

void Touch(const std::string& path, const char* text) {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    std::fputs(text, file);
    std::fclose(file);
}

}  // namespace

int main() {
    namespace fs = std::filesystem;
    std::error_code error;
    fs::path folder = fs::temp_directory_path(error)
                      / ("homm1-file-test-" + std::to_string(std::random_device()()));
    Expect(!error && fs::create_directories(folder / "Data", error)
               && fs::create_directories(folder / "Maps", error),
           "temporary folder");
    if (error)
        return 1;
    std::string base = folder.string();
    const char* root = base.c_str();
    Touch(base + "/Data/heroes.agg", "agg");
    Touch(base + "/Maps/b.map", "b");
    Touch(base + "/Maps/A.MAP", "a");
    Touch(base + "/Maps/c.txt", "c");
    FileSetRoot(root);

    char resolved[FILE_PATH_CAPACITY];
    Expect(FileResolve(".\\DATA\\HEROES.AGG", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "resolves across case and separators");
    Expect(std::string(resolved) == base + "/Data/heroes.agg", "resolves to the stored name");
    Expect(!FileResolve("..\\outside", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "refuses to leave the game folder");
    Expect(!FileResolve("C:WIN.INI", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "refuses drive paths");
#if !defined(_WIN32)
    Expect(!FileResolve("C:\\WINDOWS\\WIN.INI", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "refuses absolute drive paths");
#endif
    Expect(!FileResolve("DATA\\MISSING.BIN", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "a missing file does not resolve for reading");
    Expect(FileResolve("GAMES\\NEW.GM1", FILE_OPEN_WRITE, resolved, sizeof(resolved)),
           "a new file resolves for writing, creating its folder");

    i32 file = FileOpen("data\\Heroes.Agg", FILE_OPEN_READ);
    char text[4] = {};
    Expect(file != FILE_INVALID && FileReadExact(file, text, 3) && std::strcmp(text, "agg") == 0,
           "opens and reads");
    Expect(FileLength(file) == 3, "length");
    FileClose(file);

    Expect(FileReplace("GAMES\\NEW.GM1", "save", 4), "replaces a file");
    Expect(FileReplace("GAMES\\NEW.GM1", "again", 5), "replaces it again");
    file = FileOpen("GAMES\\new.gm1", FILE_OPEN_READ);
    Expect(file != FILE_INVALID && FileLength(file) == 5, "replacement contents");
    FileClose(file);

    FileFindData found;
    i32 find = FileFindFirst("MAPS\\*.MAP", &found);
    Expect(find != FILE_INVALID && std::strcmp(found.name, "A.MAP") == 0, "first match, sorted");
    Expect(FileFindNext(find, &found) && std::strcmp(found.name, "b.map") == 0, "second match");
    Expect(!FileFindNext(find, &found), "no third match");
    FileFindClose(find);

    Expect(FileNameMatches("*.GM?", "save.gm1"), "wildcards ignore case");
    Expect(!FileNameMatches("*.GM?", "save.gm12"), "? matches one character");

    if (fs::remove_all(folder, error) == 0 || error)
        std::fprintf(stderr, "could not remove %s\n", root);
    if (gFailures != 0)
        return 1;
    std::printf("file: all checks passed\n");
    return 0;
}
