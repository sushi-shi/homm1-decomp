#include "FuzzSupport.h"

#include <PLATFORM/File.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

bool gFuzzRejected = false;
std::string gFuzzRejection;

// The game keeps what it loads for the whole session and leaves partly
// loaded state behind when it refuses a file; leak reports would be noise.
extern "C" int __lsan_is_turned_off() {
    return 1;
}

namespace {

std::string gScratch;

void RemoveScratch() {
    FuzzCleanup();
}

std::string FindEntry(const std::string& directory, const char* name) {
    char resolved[FILE_PATH_CAPACITY];
    std::string saved = FileRoot();
    FileSetRoot(directory.c_str());
    bool found = FileResolve(name, FILE_OPEN_READ, resolved, sizeof(resolved));
    FileSetRoot(saved.c_str());
    return found ? std::string(resolved) : std::string();
}

}  // namespace

void FuzzCleanup() {
    if (gScratch.empty())
        return;
    std::string command = "rm -rf '" + gScratch + "'";
    if (std::system(command.c_str()) != 0)
        std::fprintf(stderr, "could not remove %s\n", gScratch.c_str());
    gScratch.clear();
}

std::string FuzzDataRoot() {
    const char* data = std::getenv("HOMM1_DATA");
    return data != nullptr ? std::string(data) : std::string();
}

void FuzzRequireData(const char* harness) {
    if (FuzzDataRoot().empty()) {
        std::printf("%s: skipped, needs HOMM1_DATA (the game data folder)\n", harness);
        std::fflush(stdout);
        std::_Exit(77);
    }
}

std::string FuzzScratchGame() {
    if (!gScratch.empty())
        return gScratch;
    const char* temporary = std::getenv("TMPDIR");
    std::string pattern = std::string(temporary != nullptr && temporary[0] != '\0' ? temporary : "/tmp")
                          + "/homm1-fuzz-XXXXXX";
    const char* root = mkdtemp(pattern.data());
    if (root == nullptr) {
        std::perror("mkdtemp");
        std::_Exit(1);
    }
    gScratch = root;
    std::atexit(RemoveScratch);
    std::string data = FuzzDataRoot();
    if (!data.empty()) {
        std::string dataDirectory = FindEntry(data, "DATA");
        if (dataDirectory.empty() || symlink(dataDirectory.c_str(), (gScratch + "/DATA").c_str()) != 0) {
            std::fprintf(stderr, "no DATA folder in %s\n", data.c_str());
            std::_Exit(1);
        }
    } else if (mkdir((gScratch + "/DATA").c_str(), 0755) != 0) {
        std::_Exit(1);
    }
    if (mkdir((gScratch + "/MAPS").c_str(), 0755) != 0 || mkdir((gScratch + "/GAMES").c_str(), 0755) != 0)
        std::_Exit(1);
    return gScratch;
}

bool FuzzWriteFile(const std::string& path, const uint8_t* data, size_t size) {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr)
        return false;
    bool ok = size == 0 || std::fwrite(data, 1, size, file) == size;
    return std::fclose(file) == 0 && ok;
}
