// Buka resource-name hash.

#include <match.h>

#include <BASE/MAKEFILEID.h>

#include <string.h>

#define upperName line    // frame-slot spelling
#define length activeSize // frame-slot spelling
VA(0x00473610, 0x12b)
u32 MAKEFILEID(char* name) {
    u16 fileId = 0;
    u16 highByte = 0;
    i32 length = strlen(name);
    char* upperName = new char[length + 1];
    strcpy(upperName, name);
    for (i32 i = 0; i < length; i++) {
        if (upperName[i] >= 'a' && upperName[i] <= 'z')
            upperName[i] &= ~('a' - 'A');
        highByte = fileId >> 8;
        fileId <<= 8;
        fileId |= highByte;
        if (fileId & 0x8000) {
            fileId <<= 1;
            fileId |= 1;
        } else {
            fileId <<= 1;
        }
        fileId -= upperName[i];
    }
    delete[] upperName;
    return fileId;
}
#undef upperName
#undef length
