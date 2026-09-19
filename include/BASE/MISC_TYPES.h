#ifndef HOMM1_BASE_MISC_TYPES_H
#define HOMM1_BASE_MISC_TYPES_H

#include <Domains.h>

H1_ENUM_BEGIN(MiscLogConstant)
    MISC_FILE_DEBUG_BEGIN = 2,
    MISC_DEBUGGER_OUTPUT_LEVEL = 3,
    MISC_LOG_TEXT_CAPACITY = 500
H1_ENUM_END(MiscLogConstant)

extern int giDebugLevel;

#endif
