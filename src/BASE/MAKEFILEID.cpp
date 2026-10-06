// Buka resource-name hash.

#include <match.h>

#include <BASE/MAKEFILEID.h>

#include <string.h>

VA(0x00473610, 0x12b)
u32 MAKEFILEID(char* name) {
    u16 fileId = 0;
    u16 highByte = 0;
    i32 activeSize = strlen(name);
    char* line = new char[activeSize + 1];
    strcpy(line, name);
    for (i32 i = 0; i < activeSize; i++) {
        if (line[i] >= 'a' && line[i] <= 'z')
            line[i] &= ~('a' - 'A');
        highByte = fileId >> 8;
        fileId <<= 8;
        fileId |= highByte;
        if (fileId & 0x8000) {
            fileId <<= 1;
            fileId |= 1;
        } else {
            fileId <<= 1;
        }
        fileId -= line[i];
    }
    delete[] line;
    return fileId;
}
