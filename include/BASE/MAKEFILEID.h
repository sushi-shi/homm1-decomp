#ifndef HOMM1_BASE_MAKEFILEID_H
#define HOMM1_BASE_MAKEFILEID_H

enum FileIdHashConstant {
    FILE_ID_HASH_BYTE_BITS = 8,
    FILE_ID_HASH_TOP_BIT = 0x8000
};

u32 MAKEFILEID(char* name);

#endif
