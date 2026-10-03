#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

#include <Domains.h>

class bitmap;

// Map-grid (taxicab) distance of an offset (Buka 2.1 Misc.h).
#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

void SetPalette(signed char*, int);
void LogTruncate();
void LogInt(char*, int);
void LogStr(char*);
void LogStr(char*, long, long);
void LogStr(char*, long, long, long, long, long);
void LogStr(char*, long, long, long, long, long, long, long);

H1_ENUM_CONST_BEGIN(MiscLogConstant)
    MISC_FILE_DEBUG_BEGIN = 2,
    MISC_DEBUGGER_OUTPUT_LEVEL = 3,
    MISC_LOG_TEXT_CAPACITY = 500,
    MISC_LOG_VALUE_TEXT_CAPACITY = 100,
    MISC_LOG_VALUES_TEXT_CAPACITY = 130,
    MISC_FORCED_DEBUG_LEVEL = 9
H1_ENUM_CONST_END(MiscLogConstant)

#endif
