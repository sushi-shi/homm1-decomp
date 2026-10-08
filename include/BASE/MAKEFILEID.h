#ifndef HOMM1_BASE_MAKEFILEID_H
#define HOMM1_BASE_MAKEFILEID_H

#include <Domains.h>

// MAKEFILEID rotates its 16-bit hash left by a byte and then by one bit,
// carrying the top bit around.
H1_ENUM_CONST_BEGIN(FileIdHashConstant)
    FILE_ID_HASH_BYTE_BITS = 8,
    FILE_ID_HASH_TOP_BIT = 0x8000
H1_ENUM_CONST_END(FileIdHashConstant)

u32 MAKEFILEID(char* name);

#endif // HOMM1_BASE_MAKEFILEID_H
