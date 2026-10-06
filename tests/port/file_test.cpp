// Game paths resolve case-insensitively under the game folder, never leave
// it, and directory listings match wildcards and come back sorted.

#include <H1/Ints.h>

#include <PLATFORM/File.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

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
    const char* temporary = std::getenv("TMPDIR");
    std::string pattern = std::string(temporary != nullptr && temporary[0] != '\0' ? temporary : "/tmp")
                          + "/homm1-file-test-XXXXXX";
    const char* root = mkdtemp(pattern.data());
    Expect(root != nullptr, "temporary folder");
    if (root == nullptr)
        return 1;
    std::string base = root;
    mkdir((base + "/Data").c_str(), 0755);
    mkdir((base + "/Maps").c_str(), 0755);
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
    Expect(!FileResolve("C:\\WINDOWS\\WIN.INI", FILE_OPEN_READ, resolved, sizeof(resolved)),
           "refuses drive paths");
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

    std::string cleanup = "rm -r '" + base + "'";
    if (std::system(cleanup.c_str()) != 0)
        std::fprintf(stderr, "could not remove %s\n", root);
    if (gFailures != 0)
        return 1;
    std::printf("file: all checks passed\n");
    return 0;
}
