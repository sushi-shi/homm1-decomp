// Replays files through a fuzz harness without libFuzzer: the checked-in
// regression inputs in the default ctest run, and (with --shipped) the
// shipped game files, which the harness must accept.
//
//   <harness>_replay [PATH...] [--shipped PATTERN[@FIRST-LAST]...]
//
// A PATH that is a folder replays every file in it, in name order. Each
// --shipped PATTERN is a game path with wildcards (MAPS\*.MAP) resolved under
// $HOMM1_DATA; every match must pass without the game refusing it. Without
// $HOMM1_DATA the shipped patterns are skipped. With @FIRST-LAST (hex) each
// match is replayed once for every byte value from FIRST to LAST appended to
// it, for the harnesses whose input ends with a byte of choices.
//
// A regression input is either the input itself or, named *.patch, a recipe
// that builds it, so that a reproducer needs no game data in the repository:
//
//   # comment
//   base GAMEPATH        start from a shipped file (skipped without HOMM1_DATA)
//   bytes HEX...         append bytes
//   fill COUNT HEX       append COUNT copies of a byte
//   repeat COUNT HEX...  append COUNT copies of a byte sequence
//   set OFFSET HEX...    overwrite bytes at OFFSET (decimal or 0x hex)
//   truncate LENGTH      cut the input to LENGTH bytes
//   expect refused|accepted
//
// The input must then end as expected: refused through the game's error path
// or accepted; without an expect line either is fine.

#include "FuzzSupport.h"

#include <PLATFORM/File.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <dirent.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv);

namespace {

std::vector<uint8_t> ReadHostFile(const std::string& path, bool& ok) {
    std::vector<uint8_t> bytes;
    std::FILE* file = std::fopen(path.c_str(), "rb");
    ok = file != nullptr;
    if (!ok)
        return bytes;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        bytes.push_back(static_cast<uint8_t>(c));
    std::fclose(file);
    return bytes;
}

void AddPath(const std::string& path, std::vector<std::string>& files) {
    struct stat status;
    // A harness without regression inputs has no folder for them.
    if (stat(path.c_str(), &status) != 0)
        return;
    if (!S_ISDIR(status.st_mode)) {
        files.push_back(path);
        return;
    }
    std::vector<std::string> entries;
    if (DIR* directory = opendir(path.c_str())) {
        while (dirent* entry = readdir(directory))
            if (entry->d_name[0] != '.')
                entries.push_back(path + "/" + entry->d_name);
        closedir(directory);
    }
    std::sort(entries.begin(), entries.end());
    for (const std::string& entry : entries)
        AddPath(entry, files);
}

struct Shipped {
    std::string path;
    int first;  // the appended byte values, or -1 for none
    int last;
};

void AddShipped(const char* argument, std::vector<Shipped>& files) {
    std::string pattern = argument;
    int first = -1;
    int last = -1;
    size_t at = pattern.find('@');
    if (at != std::string::npos) {
        std::string range = pattern.substr(at + 1);
        pattern.erase(at);
        size_t dash = range.find('-');
        first = static_cast<int>(std::strtoul(range.substr(0, dash).c_str(), nullptr, 16));
        last = dash == std::string::npos
                   ? first
                   : static_cast<int>(std::strtoul(range.substr(dash + 1).c_str(), nullptr, 16));
    }
    std::string root = FuzzDataRoot();
    std::string saved = FileRoot();
    FileSetRoot(root.c_str());
    std::string folder = pattern;
    size_t separator = folder.find_last_of("\\/");
    folder = separator == std::string::npos ? std::string() : folder.substr(0, separator);
    FileFindData found;
    i32 find = FileFindFirst(pattern.c_str(), &found);
    while (find != FILE_INVALID) {
        std::string game = folder.empty() ? std::string(found.name) : folder + "\\" + found.name;
        char host[FILE_PATH_CAPACITY];
        if (FileResolve(game.c_str(), FILE_OPEN_READ, host, sizeof(host)))
            files.push_back(Shipped{host, first, last});
        if (!FileFindNext(find, &found))
            break;
    }
    FileFindClose(find);
    FileSetRoot(saved.c_str());
}

// An input that runs longer than the limit is a hang: report where.
const unsigned kInputSeconds = 10;
const char* gCurrentInput = "";

void OnAlarm(int) {
    const char* what = "fuzz replay: input timed out: ";
    if (write(2, what, std::strlen(what)) < 0 || write(2, gCurrentInput, std::strlen(gCurrentInput)) < 0
        || write(2, "\n", 1) < 0)
        _exit(1);
    void* frames[64];
    int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, 2);
    _exit(1);
}

bool EndsWith(const std::string& text, const char* suffix) {
    size_t length = std::strlen(suffix);
    return text.size() >= length && text.compare(text.size() - length, length, suffix) == 0;
}

// A recipe's input. Returns false with a reason when it cannot be built.
struct Recipe {
    std::vector<uint8_t> bytes;
    std::string expect;
    bool skipped = false;
};

bool BuildRecipe(const std::string& path, Recipe& recipe, std::string& error) {
    bool ok = false;
    std::vector<uint8_t> text = ReadHostFile(path, ok);
    if (!ok) {
        error = "cannot read";
        return false;
    }
    std::string all(text.begin(), text.end());
    size_t start = 0;
    while (start < all.size()) {
        size_t end = all.find('\n', start);
        if (end == std::string::npos)
            end = all.size();
        std::string line = all.substr(start, end - start);
        start = end + 1;
        size_t hash = line.find('#');
        if (hash != std::string::npos)
            line.erase(hash);
        std::vector<std::string> words;
        size_t at = 0;
        while (at < line.size()) {
            while (at < line.size() && (line[at] == ' ' || line[at] == '\t' || line[at] == '\r'))
                at++;
            size_t wordEnd = at;
            while (wordEnd < line.size() && line[wordEnd] != ' ' && line[wordEnd] != '\t'
                   && line[wordEnd] != '\r')
                wordEnd++;
            if (wordEnd > at)
                words.push_back(line.substr(at, wordEnd - at));
            at = wordEnd;
        }
        if (words.empty())
            continue;
        auto number = [&](const std::string& word, int base) {
            return std::strtoul(word.c_str(), nullptr, base);
        };
        const std::string& command = words[0];
        if (command == "base" && words.size() == 2) {
            if (FuzzDataRoot().empty()) {
                recipe.skipped = true;
                return true;
            }
            std::string saved = FileRoot();
            FileSetRoot(FuzzDataRoot().c_str());
            char host[FILE_PATH_CAPACITY];
            bool found = FileResolve(words[1].c_str(), FILE_OPEN_READ, host, sizeof(host));
            FileSetRoot(saved.c_str());
            if (!found) {
                error = "no " + words[1] + " in the game data";
                return false;
            }
            recipe.bytes = ReadHostFile(host, ok);
        } else if (command == "bytes") {
            for (size_t i = 1; i < words.size(); i++)
                recipe.bytes.push_back(static_cast<uint8_t>(number(words[i], 16)));
        } else if (command == "fill" && words.size() == 3) {
            recipe.bytes.insert(recipe.bytes.end(), number(words[1], 0),
                                static_cast<uint8_t>(number(words[2], 16)));
        } else if (command == "repeat" && words.size() >= 3) {
            for (unsigned long copy = 0; copy < number(words[1], 0); copy++)
                for (size_t i = 2; i < words.size(); i++)
                    recipe.bytes.push_back(static_cast<uint8_t>(number(words[i], 16)));
        } else if (command == "set" && words.size() >= 3) {
            size_t offset = number(words[1], 0);
            if (offset + words.size() - 2 > recipe.bytes.size()) {
                error = "set past the end: " + line;
                return false;
            }
            for (size_t i = 2; i < words.size(); i++)
                recipe.bytes[offset + i - 2] = static_cast<uint8_t>(number(words[i], 16));
        } else if (command == "truncate" && words.size() == 2) {
            recipe.bytes.resize(std::min<size_t>(recipe.bytes.size(), number(words[1], 0)));
        } else if (command == "expect" && words.size() == 2
                   && (words[1] == "refused" || words[1] == "accepted")) {
            recipe.expect = words[1];
        } else {
            error = "bad line: " + line;
            return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> regressions;
    std::vector<Shipped> shipped;
    bool shippedMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--shipped") == 0)
            shippedMode = true;
        else if (shippedMode)
            AddShipped(argv[i], shipped);
        else
            AddPath(argv[i], regressions);
    }
    if (shippedMode && !FuzzDataRoot().empty() && shipped.empty()) {
        std::fprintf(stderr, "no shipped files found under %s\n", FuzzDataRoot().c_str());
        return 1;
    }
    LLVMFuzzerInitialize(&argc, &argv);
    signal(SIGALRM, OnAlarm);

    int failures = 0;
    auto run = [&](const std::string& path, bool mustAccept, int appended) {
        bool ok = false;
        std::vector<uint8_t> bytes;
        std::string expect = mustAccept ? "accepted" : "";
        if (EndsWith(path, ".patch")) {
            Recipe recipe;
            std::string error;
            if (!BuildRecipe(path, recipe, error)) {
                std::fprintf(stderr, "FAIL: %s: %s\n", path.c_str(), error.c_str());
                failures++;
                return;
            }
            if (recipe.skipped) {
                std::printf("skipped %s (needs HOMM1_DATA)\n", path.c_str());
                return;
            }
            bytes = recipe.bytes;
            expect = recipe.expect;
        } else {
            bytes = ReadHostFile(path, ok);
            if (!ok) {
                std::fprintf(stderr, "cannot read %s\n", path.c_str());
                failures++;
                return;
            }
            if (appended >= 0)
                bytes.push_back(static_cast<uint8_t>(appended));
        }
        gFuzzRejected = false;
        gCurrentInput = path.c_str();
        alarm(kInputSeconds);
        LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
        alarm(0);
        const char* outcome = gFuzzRejected ? "refused" : "accepted";
        if (!expect.empty() && expect != outcome) {
            std::string name = path;
            if (appended >= 0) {
                char suffix[8];
                std::snprintf(suffix, sizeof(suffix), "@%02x", appended);
                name += suffix;
            }
            std::fprintf(stderr, "FAIL: %s: %s, expected %s%s%s\n", name.c_str(), outcome,
                         expect.c_str(), gFuzzRejected ? ": " : "",
                         gFuzzRejected ? gFuzzRejection.c_str() : "");
            failures++;
        }
        if (appended >= 0)
            std::printf("ok %s@%02x (%zu bytes, %s)\n", path.c_str(), appended, bytes.size(), outcome);
        else
            std::printf("ok %s (%zu bytes, %s)\n", path.c_str(), bytes.size(), outcome);
    };
    for (const std::string& path : regressions)
        run(path, false, -1);
    for (const Shipped& file : shipped) {
        if (file.first < 0)
            run(file.path, true, -1);
        for (int value = file.first; file.first >= 0 && value <= file.last; value++)
            run(file.path, true, value);
    }
    std::printf("%zu regression input(s), %zu shipped file(s), %d failure(s)\n", regressions.size(),
                shipped.size(), failures);
    std::fflush(stdout);
    FuzzCleanup();
    // The game's own shutdown path is not run; end here.
    std::_Exit(failures == 0 ? 0 : 1);
}
