// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <BASE/MISC_TYPES.h>
#include <H1/All.h>

#include <stdio.h>
#include <string.h>

// HoMM1's retained logging path opens KB.LOG afresh and writes its banner.
VA(0x004199a5, 0x79)
void LogTruncate() {
    char logText[MISC_LOG_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "wt+");
    strcpy(logText, "===========New Log==========\n");
    fputs(logText, out);
    fclose(out);
}

// donor PoL RVA 0x000c6120; preferred Buka symbol ?LogStr@@YIXPAD@Z
// donor Buka TU BASE/Misc; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.724628;margin=0.262043;shape=0.429;size=0.928;calls=1.000;strings=KB.LOG;alternate=pol20:void LogStr(char *)@0x000c6120
VA(0x00419a1e, 0xa6)
void LogStr(char* text) {
    char logText[MISC_LOG_TEXT_CAPACITY];
    FILE* out;

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

// HoMM1 keeps the two-value logging form used by its AI call sites.
VA(0x00419ac4, 0x86)
void LogInt(char* label, int value) {
    char logText[100];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(logText, "%s : % 8d \n", label, value);
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// Prior LZHUF source uses this two-long LogStr overload (evidence/lzhuf-provenance.md).
VA(0x00419b4a, 0x8a)
void LogStr(char* label, long value1, long value2) {
    char logText[100];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(logText, "%s : % 8d  % 8d", label, value1, value2);
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// HoMM1's five-value logging form; DDAppPaint logs its blit rectangles here.
VA(0x00419bd4, 0xab)
void LogStr(char* label, long value1, long value2, long value3, long value4, long value5) {
    char logText[130];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(
        logText,
        "%s : % 8d  % 8d  % 8d  % 8d  % 8d",
        label,
        value1,
        value2,
        value3,
        value4,
        value5
    );
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// Seven-value form used by combat's action trace.
VA(0x00419c7f, 0xb3)
void LogStr(
    char* label,
    long value1,
    long value2,
    long value3,
    long value4,
    long value5,
    long value6,
    long value7
) {
    char logText[130];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(
        logText,
        "%s: % 6d % 6d % 6d % 6d % 6d % 6d % 6d",
        label,
        value1,
        value2,
        value3,
        value4,
        value5,
        value6,
        value7
    );
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// HoMM1 routes the status-line print through the AI object's debug font.
VA(0x00419d32, 0x1f)
void AiPrint(char* text) {
    gpPhilAI->AiPrint(text);
}

VA(0x00419d51, 0x4e)
void AbsAiPrint(char* text) {
    int saved;

    if (giDebugLevel == 0)
        return;
    saved = giDebugLevel;
    giDebugLevel = MISC_FORCED_DEBUG_LEVEL;
    gpPhilAI->AiPrint(text);
    giDebugLevel = saved;
}
