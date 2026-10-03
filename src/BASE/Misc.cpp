// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <BASE/MISC_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>
#include <stdlib.h>
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
    char logText[MISC_LOG_VALUE_TEXT_CAPACITY];
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
    char logText[MISC_LOG_VALUE_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(logText, "%s : % 8d  % 8d\n", label, value1, value2);
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// HoMM1's five-value logging form; DDAppPaint logs its blit rectangles here.
VA(0x00419bd4, 0xab)
void LogStr(char* label, long value1, long value2, long value3, long value4, long value5) {
    char logText[MISC_LOG_VALUES_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(
        logText,
        "%s : % 8d  % 8d  % 8d  % 8d  % 8d\n",
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
    char logText[MISC_LOG_VALUES_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(
        logText,
        "%s: % 6d % 6d % 6d % 6d % 6d % 6d % 6d\n",
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
    gpPhilAI->ShowDebugText(text);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00419d51, 0x4e)
void AbsAiPrint(char* text) {
    int saved;

    if (giDebugLevel == 0)
        return;
    saved = giDebugLevel;
    giDebugLevel = MISC_FORCED_DEBUG_LEVEL;
    gpPhilAI->ShowDebugText(text);
    giDebugLevel = saved;
}

// philAI.h: the AI strategic-value maps philAI::DoAI resets through ResetHeroRVs.

// Buka ResetHeroRVs; HoMM1 has no off-map guard and indexes [x][y].
VA(0x00419d9f, 0x177)
void ResetHeroRVs(int resetAll, int x, int y) {
    int i;
    int j;

    for (i = 0; i < MAP_CELL_GRID_SIZE; i++) {
        for (j = 0; j < MAP_CELL_GRID_SIZE; j++) {
            if (resetAll) {
                if (abs(x - i) + abs(y - j) < 10)
                    gaiHeroStrategicRVOfPos[i][j] = RV_UNSET;
            } else {
                gaiHeroStrategicRVOfPos[i][j] = RV_UNSET;
                gaiHeroEventStratRVOfPos[i][j] = RV_UNSET;
            }
        }
    }
    gaiHeroEventStratRVOfPos[x][y] = RV_UNSET;
    for (i = 0; i < GAME_HERO_COUNT; i++) {
        if (!resetAll
            || abs(y - gpGame->m_heroRecs[i].m_x) + abs(x - gpGame->m_heroRecs[i].m_x) < 10)
            gaiHeroLiveChance[i] = RV_UNSET;
    }
}
