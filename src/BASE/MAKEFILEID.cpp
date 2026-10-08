#include <H1/Ints.h>

#include <BASE/MAKEFILEID.h>

#include <string.h>

u32 MAKEFILEID(char* name) {
    u16 fileId = 0;
    u16 highByte = 0;
    i32 length = strlen(name);
    char* upperName = new char[length + 1];
    strcpy(upperName, name);
    for (i32 i = 0; i < length; i++) {
        if (upperName[i] >= 'a' && upperName[i] <= 'z')
            upperName[i] &= ~('a' - 'A');
        highByte = fileId >> FILE_ID_HASH_BYTE_BITS;
        fileId <<= FILE_ID_HASH_BYTE_BITS;
        fileId |= highByte;
        if (fileId & FILE_ID_HASH_TOP_BIT) {
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
