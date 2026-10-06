#include <PLATFORM/GameData.h>

#include <PLATFORM/File.h>
#include <PLATFORM/Platform.h>

namespace platform {

namespace {

bool ResolvesUnder(const std::string& directory, const char* path) {
    char resolved[FILE_PATH_CAPACITY];
    std::string saved = FileRoot();
    FileSetRoot(directory.c_str());
    bool found = FileResolve(path, FILE_OPEN_READ, resolved, sizeof(resolved));
    FileSetRoot(saved.c_str());
    return found;
}

}  // namespace

std::string FindGameRoot(const std::string& requested, std::vector<std::string>& searched) {
    searched.clear();
    std::string argument = ConfiguredDirectory(requested);
    if (!argument.empty())
        searched.push_back(argument);
    std::string environment = EnvironmentDirectory("HOMM1_DATA");
    if (!environment.empty())
        searched.push_back(environment);
    searched.push_back(ExecutableDirectory());
    searched.push_back(".");
#if defined(_WIN32)
    std::string local = EnvironmentDirectory("LOCALAPPDATA");
    if (!local.empty())
        searched.push_back(local + "\\homm1-buka\\game");
#else
    std::string xdg = EnvironmentDirectory("XDG_DATA_HOME");
    std::string home = EnvironmentDirectory("HOME");
    if (!xdg.empty())
        searched.push_back(xdg + "/homm1-buka/game");
    if (!home.empty())
        searched.push_back(home + "/.local/share/homm1-buka/game");
#endif
    for (const std::string& candidate : searched) {
        if (ResolvesUnder(candidate, "DATA\\HEROES.AGG"))
            return candidate;
    }
    return std::string();
}

std::string FindCdRoot(const std::string& gameRoot) {
    std::vector<std::string> candidates;
    std::string environment = EnvironmentDirectory("HOMM1_CD");
    if (!environment.empty())
        candidates.push_back(environment);
    candidates.push_back(gameRoot + "/../cd");
    candidates.push_back(gameRoot);
    for (const std::string& candidate : candidates) {
        if (ResolvesUnder(candidate, "TRACKS\\02-AudioTrack 02.ogg"))
            return candidate;
    }
    return std::string();
}

}  // namespace platform
