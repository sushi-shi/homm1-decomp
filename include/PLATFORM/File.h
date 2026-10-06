#ifndef HOMM1_PLATFORM_FILE_H
#define HOMM1_PLATFORM_FILE_H

// Game file access. The game names its files the way the Windows original did:
// relative to the game folder, separated by backslashes, in any letter case
// ("DATA\\HEROES.AGG", "MAPS\\W95A1234.MAP"). These functions resolve such a
// name on the host: on Windows directly, elsewhere by matching each path
// component case-insensitively under the game folder.
//
// This header is plain C++98: the Visual C++ 6 build compiles it too.

#include <H1/Ints.h>

#include <stddef.h>

enum FileOpenMode {
    FILE_OPEN_READ = 0,
    // Creates the file or truncates an existing one.
    FILE_OPEN_WRITE = 1,
    // Opens an existing file for reading and writing in place.
    FILE_OPEN_UPDATE = 2
};

enum FileSeekOrigin {
    FILE_SEEK_SET = 0,
    FILE_SEEK_CUR = 1,
    FILE_SEEK_END = 2
};

enum FileConstant {
    FILE_INVALID = -1,
    FILE_PATH_CAPACITY = 1024,
    FILE_FIND_NAME_CAPACITY = 260
};

// The folder game paths are relative to (default: the current directory).
void FileSetRoot(const char* directory);
const char* FileRoot(void);

// Returns a handle, or FILE_INVALID when the file cannot be opened.
i32 FileOpen(const char* path, FileOpenMode mode);
void FileClose(i32 file);
// Transfers at most count bytes and returns how many moved (0 at the end of
// the file, -1 on error), like the C runtime's read and write.
i32 FileRead(i32 file, void* buffer, i32 count);
i32 FileWrite(i32 file, const void* buffer, i32 count);
// True only when exactly count bytes moved.
bool FileReadExact(i32 file, void* buffer, i32 count);
bool FileWriteExact(i32 file, const void* buffer, i32 count);
// Returns the new position, or -1.
i32 FileSeek(i32 file, i32 offset, FileSeekOrigin origin);
i32 FileTell(i32 file);
i32 FileLength(i32 file);
bool FileExists(const char* path);

// Writes a whole file. Where the host allows it, the previous file is
// replaced only after the new contents are completely written and flushed,
// so a failed save never destroys the old one.
bool FileReplace(const char* path, const void* data, i32 count);

// Writes the host path for a game path into buffer (capacity bytes, always
// terminated), for the code that hands paths to other libraries. Returns false
// when the result does not fit.
bool FileResolve(const char* path, FileOpenMode mode, char* buffer, size_t capacity);

// Directory listing with a wildcard pattern in the last component
// ("MAPS\\*.MAP", "GAMES\\*.GM?"); names match case-insensitively. Names come
// back as they are stored on the host, in the host's order sorted by name.
struct FileFindData {
    char name[FILE_FIND_NAME_CAPACITY];
};
// Returns a handle and fills data with the first match, or FILE_INVALID.
i32 FileFindFirst(const char* pattern, FileFindData* data);
// Fills data with the next match; false when there is none.
bool FileFindNext(i32 find, FileFindData* data);
void FileFindClose(i32 find);

// True when the pattern (with * and ?) matches name, ignoring ASCII case.
bool FileNameMatches(const char* pattern, const char* name);

#endif
