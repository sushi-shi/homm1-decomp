#include <H1/Ints.h>

#include <PLATFORM/File.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

bool FileNameMatches(const char* pattern, const char* name) {
    // Iterative wildcard match with one backtracking point for '*'.
    const char* star = NULL;
    const char* resume = NULL;
    while (*name != '\0') {
        if (*pattern == '*') {
            star = pattern++;
            resume = name;
        } else if (*pattern == '?'
                   || tolower(static_cast<u8>(*pattern)) == tolower(static_cast<u8>(*name))) {
            pattern++;
            name++;
        } else if (star != NULL) {
            pattern = star + 1;
            name = ++resume;
        } else {
            return false;
        }
    }
    while (*pattern == '*')
        pattern++;
    return *pattern == '\0';
}

bool FileReadExact(i32 file, void* buffer, i32 count) {
    u8* cursor = static_cast<u8*>(buffer);
    while (count > 0) {
        i32 moved = FileRead(file, cursor, count);
        if (moved <= 0)
            return false;
        cursor += moved;
        count -= moved;
    }
    return count == 0;
}

bool FileWriteExact(i32 file, const void* buffer, i32 count) {
    const u8* cursor = static_cast<const u8*>(buffer);
    while (count > 0) {
        i32 moved = FileWrite(file, cursor, count);
        if (moved <= 0)
            return false;
        cursor += moved;
        count -= moved;
    }
    return count == 0;
}

i32 FileLength(i32 file) {
    i32 position = FileTell(file);
    if (position < 0)
        return -1;
    i32 length = FileSeek(file, 0, FILE_SEEK_END);
    FileSeek(file, position, FILE_SEEK_SET);
    return length;
}

bool FileExists(const char* path) {
    i32 file = FileOpen(path, FILE_OPEN_READ);
    if (file == FILE_INVALID)
        return false;
    FileClose(file);
    return true;
}

#if defined(_WIN32)

// Windows resolves case and separators itself; the game folder is the current
// directory unless the port chose another.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

static char gFileRoot[FILE_PATH_CAPACITY] = "";

void FileSetRoot(const char* directory) {
    strncpy(gFileRoot, directory, sizeof(gFileRoot) - 1);
    gFileRoot[sizeof(gFileRoot) - 1] = '\0';
}

const char* FileRoot(void) {
    return gFileRoot;
}

bool FileResolve(const char* path, FileOpenMode, char* buffer, size_t capacity) {
    size_t rootLength;
    size_t pathLength;
    bool relative;

    if (capacity == 0)
        return false;
    relative = gFileRoot[0] != '\0' && path[0] != '\\' && path[0] != '/'
               && !(path[0] != '\0' && path[1] == ':');
    rootLength = relative ? strlen(gFileRoot) : 0;
    pathLength = strlen(path);
    if (rootLength + 1 + pathLength + 1 > capacity) {
        buffer[0] = '\0';
        return false;
    }
    buffer[0] = '\0';
    if (relative) {
        strcpy(buffer, gFileRoot);
        if (rootLength > 0 && buffer[rootLength - 1] != '\\' && buffer[rootLength - 1] != '/')
            strcat(buffer, "\\");
    }
    strcat(buffer, path);
    return true;
}

i32 FileOpen(const char* path, FileOpenMode mode) {
    char resolved[FILE_PATH_CAPACITY];
    if (!FileResolve(path, mode, resolved, sizeof(resolved)))
        return FILE_INVALID;
    switch (mode) {
        case FILE_OPEN_WRITE:
            return _open(resolved, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
        case FILE_OPEN_UPDATE:
            return _open(resolved, _O_RDWR | _O_BINARY);
        default:
            return _open(resolved, _O_RDONLY | _O_BINARY);
    }
}

void FileClose(i32 file) {
    if (file != FILE_INVALID)
        _close(file);
}

i32 FileRead(i32 file, void* buffer, i32 count) {
    return _read(file, buffer, count);
}

i32 FileWrite(i32 file, const void* buffer, i32 count) {
    return _write(file, buffer, count);
}

i32 FileSeek(i32 file, i32 offset, FileSeekOrigin origin) {
    return _lseek(file, offset, origin);
}

i32 FileTell(i32 file) {
    return _lseek(file, 0, SEEK_CUR);
}

bool FileReplace(const char* path, const void* data, i32 count) {
    i32 file = FileOpen(path, FILE_OPEN_WRITE);
    bool ok;
    if (file == FILE_INVALID)
        return false;
    ok = FileWriteExact(file, data, count);
    return _close(file) == 0 && ok;
}

struct FileFindState {
    HANDLE handle;
    char pattern[FILE_FIND_NAME_CAPACITY];
};

enum FileFindConstant {
    FILE_FIND_SLOTS = 8
};

static FileFindState gFileFinds[FILE_FIND_SLOTS];
static bool gFileFindsReady = false;

static bool FileFindCopy(FileFindState* state, const WIN32_FIND_DATAA* found, FileFindData* data) {
    if (!FileNameMatches(state->pattern, found->cFileName))
        return false;
    strncpy(data->name, found->cFileName, sizeof(data->name) - 1);
    data->name[sizeof(data->name) - 1] = '\0';
    return true;
}

i32 FileFindFirst(const char* pattern, FileFindData* data) {
    char resolved[FILE_PATH_CAPACITY];
    WIN32_FIND_DATAA found;
    const char* name;
    i32 slot;

    if (!gFileFindsReady) {
        for (slot = 0; slot < FILE_FIND_SLOTS; slot++)
            gFileFinds[slot].handle = INVALID_HANDLE_VALUE;
        gFileFindsReady = true;
    }
    for (slot = 0; slot < FILE_FIND_SLOTS; slot++) {
        if (gFileFinds[slot].handle == INVALID_HANDLE_VALUE)
            break;
    }
    if (slot == FILE_FIND_SLOTS)
        return FILE_INVALID;
    if (!FileResolve(pattern, FILE_OPEN_READ, resolved, sizeof(resolved)))
        return FILE_INVALID;
    name = strrchr(pattern, '\\');
    name = name != NULL ? name + 1 : pattern;
    strncpy(gFileFinds[slot].pattern, name, sizeof(gFileFinds[slot].pattern) - 1);
    gFileFinds[slot].pattern[sizeof(gFileFinds[slot].pattern) - 1] = '\0';
    gFileFinds[slot].handle = FindFirstFileA(resolved, &found);
    if (gFileFinds[slot].handle == INVALID_HANDLE_VALUE)
        return FILE_INVALID;
    if (FileFindCopy(&gFileFinds[slot], &found, data) || FileFindNext(slot, data))
        return slot;
    FileFindClose(slot);
    return FILE_INVALID;
}

bool FileFindNext(i32 find, FileFindData* data) {
    WIN32_FIND_DATAA found;
    if (find < 0 || find >= FILE_FIND_SLOTS || gFileFinds[find].handle == INVALID_HANDLE_VALUE)
        return false;
    while (FindNextFileA(gFileFinds[find].handle, &found)) {
        if (FileFindCopy(&gFileFinds[find], &found, data))
            return true;
    }
    return false;
}

void FileFindClose(i32 find) {
    if (find < 0 || find >= FILE_FIND_SLOTS || gFileFinds[find].handle == INVALID_HANDLE_VALUE)
        return;
    FindClose(gFileFinds[find].handle);
    gFileFinds[find].handle = INVALID_HANDLE_VALUE;
}

#else

// POSIX hosts: game paths are matched component by component under the game
// folder, ignoring ASCII case. "." components and the leading ".\" of the
// original's path globals are dropped; ".." and absolute game paths are
// refused, so a game path never leaves the game folder.

#include <algorithm>
#include <cerrno>
#include <map>
#include <string>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string gRoot = ".";

bool SameIgnoringCase(const std::string& a, const char* b) {
    size_t length = strlen(b);
    if (a.size() != length)
        return false;
    for (size_t i = 0; i < length; i++) {
        if (tolower(static_cast<u8>(a[i])) != tolower(static_cast<u8>(b[i])))
            return false;
    }
    return true;
}

std::vector<std::string> ListDirectory(const std::string& directory) {
    std::vector<std::string> names;
    DIR* handle = opendir(directory.c_str());
    if (handle == nullptr)
        return names;
    while (dirent* entry = readdir(handle)) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            names.emplace_back(entry->d_name);
    }
    closedir(handle);
    // Directory order is not stable across file systems; the game sees sorted names.
    std::sort(names.begin(), names.end());
    return names;
}

// The host name of component inside directory: the exact spelling when it
// exists, else the first case-insensitive match in sorted order, else empty.
std::string FindComponent(const std::string& directory, const std::string& component) {
    struct stat info;
    std::string exact = directory + "/" + component;
    if (stat(exact.c_str(), &info) == 0)
        return component;
    for (const std::string& name : ListDirectory(directory)) {
        if (SameIgnoringCase(name, component.c_str()))
            return name;
    }
    return std::string();
}

bool SplitGamePath(const char* path, std::vector<std::string>& components) {
    std::string current;
    for (const char* cursor = path;; cursor++) {
        char c = *cursor;
        if (c == '\\' || c == '/' || c == '\0') {
            if (current == "..")
                return false;
            if (!current.empty() && current != ".")
                components.push_back(current);
            current.clear();
            if (c == '\0')
                break;
        } else {
            current += c;
        }
    }
    return true;
}

bool Resolve(const char* path, FileOpenMode mode, std::string& out) {
    if (path == nullptr || path[0] == '\0')
        return false;
    // A host path (from the command line or the environment) is used as given.
    if (path[0] == '/') {
        out = path;
        return true;
    }
    if (isalpha(static_cast<u8>(path[0])) && path[1] == ':')
        return false;
    std::vector<std::string> components;
    if (!SplitGamePath(path, components) || components.empty())
        return false;
    std::string resolved = gRoot;
    for (size_t i = 0; i < components.size(); i++) {
        bool last = i + 1 == components.size();
        std::string name = FindComponent(resolved, components[i]);
        if (name.empty()) {
            if (mode != FILE_OPEN_WRITE)
                return false;
            name = components[i];
            if (!last && mkdir((resolved + "/" + name).c_str(), 0755) != 0 && errno != EEXIST)
                return false;
        }
        resolved += "/";
        resolved += name;
    }
    out = resolved;
    return true;
}

struct FindState {
    std::vector<std::string> names;
    size_t next = 0;
};

std::map<i32, FindState> gFinds;
i32 gNextFind = 1;

}  // namespace

void FileSetRoot(const char* directory) {
    gRoot = directory != nullptr && directory[0] != '\0' ? directory : ".";
    while (gRoot.size() > 1 && gRoot.back() == '/')
        gRoot.pop_back();
}

const char* FileRoot(void) {
    return gRoot.c_str();
}

bool FileResolve(const char* path, FileOpenMode mode, char* buffer, size_t capacity) {
    std::string resolved;
    if (capacity == 0)
        return false;
    buffer[0] = '\0';
    if (!Resolve(path, mode, resolved) || resolved.size() + 1 > capacity)
        return false;
    memcpy(buffer, resolved.c_str(), resolved.size() + 1);
    return true;
}

i32 FileOpen(const char* path, FileOpenMode mode) {
    std::string resolved;
    if (!Resolve(path, mode, resolved))
        return FILE_INVALID;
    int flags = O_RDONLY;
    if (mode == FILE_OPEN_WRITE)
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (mode == FILE_OPEN_UPDATE)
        flags = O_RDWR;
    int file = open(resolved.c_str(), flags | O_CLOEXEC, 0644);
    if (file < 0)
        return FILE_INVALID;
    struct stat info;
    if (fstat(file, &info) != 0 || S_ISDIR(info.st_mode)) {
        close(file);
        return FILE_INVALID;
    }
    return file;
}

void FileClose(i32 file) {
    if (file != FILE_INVALID)
        close(file);
}

i32 FileRead(i32 file, void* buffer, i32 count) {
    if (count < 0)
        return -1;
    for (;;) {
        ssize_t moved = read(file, buffer, static_cast<size_t>(count));
        if (moved < 0 && errno == EINTR)
            continue;
        return static_cast<i32>(moved);
    }
}

i32 FileWrite(i32 file, const void* buffer, i32 count) {
    if (count < 0)
        return -1;
    for (;;) {
        ssize_t moved = write(file, buffer, static_cast<size_t>(count));
        if (moved < 0 && errno == EINTR)
            continue;
        return static_cast<i32>(moved);
    }
}

i32 FileSeek(i32 file, i32 offset, FileSeekOrigin origin) {
    int whence = origin == FILE_SEEK_END ? SEEK_END : origin == FILE_SEEK_CUR ? SEEK_CUR : SEEK_SET;
    off_t position = lseek(file, offset, whence);
    if (position < 0 || position > 0x7fffffff)
        return -1;
    return static_cast<i32>(position);
}

i32 FileTell(i32 file) {
    return FileSeek(file, 0, FILE_SEEK_CUR);
}

bool FileReplace(const char* path, const void* data, i32 count) {
    std::string resolved;
    if (!Resolve(path, FILE_OPEN_WRITE, resolved))
        return false;
    std::string temporary = resolved + ".partial";
    int file = open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (file < 0)
        return false;
    bool ok = FileWriteExact(file, data, count) && fsync(file) == 0;
    ok = close(file) == 0 && ok;
    if (ok)
        ok = rename(temporary.c_str(), resolved.c_str()) == 0;
    if (!ok)
        unlink(temporary.c_str());
    return ok;
}

i32 FileFindFirst(const char* pattern, FileFindData* data) {
    std::string directoryPath = ".";
    std::string namePattern = pattern;
    size_t split = namePattern.find_last_of("\\/");
    if (split != std::string::npos) {
        directoryPath = namePattern.substr(0, split);
        namePattern = namePattern.substr(split + 1);
    }
    std::string directory;
    if (directoryPath == "." || directoryPath.empty())
        directory = gRoot;
    else if (!Resolve(directoryPath.c_str(), FILE_OPEN_READ, directory))
        return FILE_INVALID;
    FindState state;
    for (const std::string& name : ListDirectory(directory)) {
        struct stat info;
        if (stat((directory + "/" + name).c_str(), &info) == 0 && S_ISREG(info.st_mode)
            && name.size() < sizeof(data->name) && FileNameMatches(namePattern.c_str(), name.c_str()))
            state.names.push_back(name);
    }
    if (state.names.empty())
        return FILE_INVALID;
    i32 find = gNextFind++;
    gFinds[find] = state;
    FileFindNext(find, data);
    return find;
}

bool FileFindNext(i32 find, FileFindData* data) {
    std::map<i32, FindState>::iterator found = gFinds.find(find);
    if (found == gFinds.end() || found->second.next >= found->second.names.size())
        return false;
    const std::string& name = found->second.names[found->second.next++];
    memcpy(data->name, name.c_str(), name.size() + 1);
    return true;
}

void FileFindClose(i32 find) {
    gFinds.erase(find);
}

#endif
