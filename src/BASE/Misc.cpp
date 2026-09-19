// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <BASE/MISC_TYPES.h>
#include <H1/All.h>

#include <stdio.h>
#include <string.h>

// donor PoL RVA 0x000c6120; preferred Buka symbol ?LogStr@@YIXPAD@Z
// donor Buka TU BASE/Misc; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.724628;margin=0.262043;shape=0.429;size=0.928;calls=1.000;strings=KB.LOG;alternate=pol20:void LogStr(char *)@0x000c6120
VA(0x00419a1e, 0xa6)
void LogStr(char *text)
{
    char logText[MISC_LOG_TEXT_CAPACITY];
    FILE *out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    strcpy(logText, text);
    strcat(logText, "\n");
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}
