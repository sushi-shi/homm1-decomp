// Buka resource-name hash, recovered from the retail instruction flow.
#include <match.h>
#include <BASE/MAKEFILEID.h>
#include <string.h>

VA(0x00473610, 0x12b)
u32 MAKEFILEID(char* text) {
    u16 fileId = 0;
    u16 highByte = 0;
    i32 size = strlen(text);
    char* buffer = new char[size + 1];
    strcpy(buffer, text);
    for (i32 i = 0; i < size; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z')
            buffer[i] &= ~('a' - 'A');
        highByte = fileId >> 8;
        fileId <<= 8;
        fileId |= highByte;
        if (fileId & 0x8000) {
            fileId <<= 1;
            fileId |= 1;
        } else {
            fileId <<= 1;
        }
        fileId -= buffer[i];
    }
    delete[] buffer;
    return fileId;
}
