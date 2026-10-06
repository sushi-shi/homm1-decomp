#include <H1/Ints.h>

#include <SOURCE/kbwin.h>

#include <BASE/Misc.h>
#include <BASE/audio.h>
#include <BASE/heroWindow.h>
#include <BASE/message.h>
#include <BASE/miscwin.h>
#include <BASE/soundmgr.h>
#include <SOURCE/KB.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <string.h>

// The host-independent part of kbwin.cpp: settings defaults, window text
// setup, assertions and token search. Both programs and both hosts link it.

void SetGameDefaults(void) {
    i32 i;

    gConfig.musicVolume = SOUND_VOLUME_100;
    gConfig.soundVolume = SOUND_VOLUME_100;
    gConfig.autosave = 1;
    gConfig.showRoute = 1;
    gConfig.blackoutComputer = 0;
    for (i = CONFIG_EXECUTABLE_GAME; i < CONFIG_EXECUTABLE_COUNT; i++) {
        gConfig.gfx[i].showMenu = 1;
        gConfig.gfx[i].x = DEFAULT_WINDOW_ORIGIN;
        gConfig.gfx[i].y = DEFAULT_WINDOW_ORIGIN;
        if (gMainVideoModeWidth <= LOGICAL_SCREEN_WIDTH && gDDrawAttached) {
            gConfig.gfx[i].fullScreen = 0;
            gConfig.gfx[i].width = DEFAULT_SMALL_WINDOW_WIDTH;
            gConfig.gfx[i].height = DEFAULT_SMALL_WINDOW_HEIGHT;
        } else {
            gConfig.gfx[i].fullScreen = 0;
            gConfig.gfx[i].width = LOGICAL_SCREEN_WIDTH;
            gConfig.gfx[i].height = LOGICAL_SCREEN_HEIGHT;
        }
    }
    gConfig.blackoutComputer = 0;
    gConfig.currentMapOffset = 0;
    gConfig.firstMapOffset = Random(0, DEFAULT_MAP_OFFSET_LIMIT);
    gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
    gFirstTimeThrough = true;
    gConfig.walkSpeed = WALK_SPEED_CANTER;
    SetEditionDefaults();
}

void SetEditionDefaults(void) {
    gConfig.showEnemyMobility = 0;
    gConfig.softRetreatSurrender = 0;
    gConfig.slightlyHarderAI = 0;
    gConfig.cheatMode = CHEAT_MODE_EXTENDED;
    gConfig.originalCheatKeys = 0;
    gConfig.losslessAudio = 0;
    gConfig.playVideos = 0;
    gConfig.battleMessageFormat = BATTLE_MESSAGE_FORECAST;
}

void SetWinText(heroWindow* window, i16 id) {
    i32 i;
    tag_message msg;
#ifdef HOMM1_EDITOR
    for (i = 0; i < WINDOW_TEXT_EDITOR_ENTRY_COUNT; i++) {
#else
    for (i = 0; i < WINDOW_TEXT_ENTRY_COUNT; i++) {
#endif
        if (gWinSetup[i].windowId == id) {
            SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, gWinSetup[i].widgetId);
            msg.text = gWinSetupText[i];
            window->BroadcastMessage(msg);
        }
    }
}

void ProcessAssert(i32 condition, char* file, i32 line) {
    i32 unusedAssertWord;
    if (condition == 0) {
        sprintf(gText, "Assert statement failed in module %s, line %d.", file, line);
        KBErrorBox(gText, "Assert Failure");
        unusedAssertWord = 0;
        ShutDown(gText);
    }
}

char* FindToken(char* text, char token) {
    i32 pos;
    i32 len;

    len = strlen(text);
    for (pos = 0; pos < len; pos++) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}

char* FindLastToken(char* text, char token) {
    i32 pos;
    i32 len;

    len = strlen(text);
    for (pos = len - 1; pos >= 0; pos--) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}
