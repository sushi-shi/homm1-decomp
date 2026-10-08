#include <H1/Ints.h>

#include <BASE/audio.h>
#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/smackManager.h>
#include <SOURCE/wingraph.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

SSmackOptions gSmackOptions[SMACK_COUNT] = {
    {"BUKA", "", true, true, true, false, false, 0, 0},
    {"NWCLOGO", "", true, true, true, false, false, 0, 0},
    {"INTRO", "", true, true, true, false, false, 0, 0},
    {"LOSE", "", true, true, false, false, false, 0, 0},
    {"WIN1", "", true, true, false, false, false, 0, 0},
    {"WIN2", "", true, true, false, false, false, 0, 0}
};
static i32 gSmackVolumes[11] = {0, 127, 97, 75, 52, 40, 30, 20, 15, 10, 5};
i8 gMovieId;
static b32 gSmackEnded;
static SmackSum gSmackSummary;
static i8 gSmackSavedPalette[PALETTE_DATA_SIZE];
static i32 gOldSmackPad;
static b8 gSmackStop;
static b32 gSmackPrevFrame;
static b32 gSmackSound;
static resource* gSmackResource;
static font* gSmackFont;
static i32 gSmackTrackSummary;
static Smack* gSmackPrimary;
static Smack* gSmackCompanion;

void ConvertSmackerPalette(u8* paletteData) {
    for (i32 i = 0; i < PALETTE_DATA_SIZE; ++i)
        paletteData[i] = paletteData[i] >> WINGRAPH_PALETTE_VALUE_SHIFT;
}

void DoAdvance(Smack* smack, b32 drawFrame, b32 advanceFrame, b32 updatePalette, b32 skipPalette) {
    if (drawFrame && smack->NewPalette && !skipPalette) {
        memcpy(gPalette->m_data, smack->Palette, PALETTE_DATA_SIZE);
        ConvertSmackerPalette(reinterpret_cast<u8*>(gPalette->m_data));
        if (updatePalette)
            UpdatePalette(gPalette->m_data);
    }
    SmackDoFrame(smack);
    if (drawFrame) {
        while (SmackToBufferRect(smack, SMACK_SURFACE_SLOW)) {
            if (gMovieId == SMACK_WIN2 && smack->FrameNum >= SMACK_WIN2_TEXT_FIRST_FRAME) {
                H1_ASSERT(gWinText != NULL);
                gSmackFont->DrawBoundedString(gWinText, 32, 342, 320, 106, 4, FONT_ALIGN_CENTER);
            }
            BlitBitmapToScreen(
                gWindowManager->m_screen,
                smack->LastRectx,
                smack->LastRecty,
                smack->LastRectw,
                smack->LastRecth,
                smack->LastRectx,
                smack->LastRecty
            );
        }
    }
    if (advanceFrame)
        SmackNextFrame(smack);
}

void SmackMain() {
    i32 soundFlags;
    i32 preloadFlags;
    b32 active;
    b32 primaryOn;
    b32 companionOn;
    b32 unusedTrue = true;
    gSmackPrevFrame = false;
    i32 unusedPlaybackState = 0;
    i32 unusedTimer;
    i32 unusedKey;
    gSmackFont = gResourceManager->GetFont("bigfont.fnt");
    KBChangeMenu(gDefaultMenu);
    gMouseManager->ReallyHidePointer();
    gSmackStop = true;
    memcpy(gSmackSavedPalette, gPalette->m_data, PALETTE_DATA_SIZE);
    ShutdownAudio();
    InitSmackSound();
    if (gNoSound || !SmackSoundReady() || !gConfig.soundVolume) {
        gSmackSound = false;
    } else {
        gSmackSound = true;
        UseSmackSound(gSmackVolumes[gConfig.soundVolume]);
    }
    sprintf(gText, "%s%s.SMK", gAnimPath, gSmackOptions[gMovieId].fileName);
    soundFlags = gSmackSound ? SMACK_TRACKS : 0;
    preloadFlags = gSmackOptions[gMovieId].preload ? SMACK_PRELOAD_ALL : 0;
    gSmackPrimary = SmackOpen(gText, soundFlags + preloadFlags, SMACK_AUTO_EXTRA);
    // A movie missing from ANIM (a game folder copied without it) is skipped
    // as if it had played to its end, so what follows it still runs; the
    // original went on with no movie and crashed.
    if (gSmackPrimary == NULL) {
        gSmackEnded = true;
        goto closeMovies;
    }
    SmackToBuffer(
        gSmackPrimary,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        gWindowManager->m_screen->m_pixels,
        0
    );
    if (strlen(gSmackOptions[gMovieId].companionFileName) > 1) {
        sprintf(gText, "%s%s.SMK", gAnimPath, gSmackOptions[gMovieId].companionFileName);
        gSmackCompanion = SmackOpen(gText, soundFlags, SMACK_AUTO_EXTRA);
        if (gSmackOptions[gMovieId].drawCompanion && gSmackCompanion)
            SmackToBuffer(
                gSmackCompanion,
                gSmackOptions[gMovieId].companionX,
                gSmackOptions[gMovieId].companionY,
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT,
                gWindowManager->m_screen->m_pixels,
                0
            );
    }
    FillBitmapArea(gWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0);
    BlitBitmapToScreen(
        gWindowManager->m_screen,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        0,
        0
    );
    if (gSmackOptions[gMovieId].fadeIn)
        gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_NORMAL, NULL);
    active = true;
    primaryOn = false;
    companionOn = false;
    while (active) {
        if (!SmackWait(gSmackPrimary)) {
            if (!primaryOn || gSmackPrimary->Frames > 1)
                DoAdvance(
                    gSmackPrimary,
                    true,
                    true,
                    primaryOn || !gSmackOptions[gMovieId].fadeIn,
                    false
                );
            if (gSmackPrimary->FrameNum > 0 || gSmackPrimary->Frames <= 1) {
                if (!primaryOn && gSmackOptions[gMovieId].fadeIn) {
                    memcpy(gBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
                    gWindowManager->FadeScreen(WINDOW_FADE_IN, 4, NULL);
                }
                primaryOn = true;
            }
        }
        if (gSmackCompanion && primaryOn && !SmackWait(gSmackCompanion)) {
            if (companionOn && gSmackCompanion->FrameNum == gSmackCompanion->Frames - 1) {
                b32 drawLastFrame;
                if (gSmackOptions[gMovieId].drawCompanion)
                    drawLastFrame = true;
                else
                    drawLastFrame = false;
                DoAdvance(gSmackCompanion, drawLastFrame, false, false, true);
                gSmackPrevFrame = true;
                while (SmackWait(gSmackCompanion))
                    Process1WindowsMessage();
            } else {
                DoAdvance(
                    gSmackCompanion,
                    gSmackOptions[gMovieId].drawCompanion,
                    true,
                    false,
                    true
                );
            }
            if (gSmackCompanion && gSmackCompanion->FrameNum > 0)
                companionOn = true;
        }
        Process1WindowsMessage();
        tag_message message;
        message = gInputManager->GetEvent();
        switch (message.type) {
            case MESSAGE_KEY_DOWN:
                if (message.keyCode == INPUT_SCAN_F4)
                    break;
            case MESSAGE_LEFT_BUTTON_DOWN:
            case MESSAGE_RIGHT_BUTTON_DOWN:
                active = false;
                continue;
        }
        if (!gSmackOptions[gMovieId].waitForInput
            && (gSmackPrevFrame
                || (gSmackCompanion
                    && (gSmackCompanion->FrameNum >= gSmackCompanion->Frames
                        || (gSmackCompanion->FrameNum <= 0 && companionOn)))
                || (!gSmackCompanion
                    && (gSmackPrimary->FrameNum >= gSmackPrimary->Frames
                        || (gSmackPrimary->FrameNum <= 0 && primaryOn))))) {
            active = false;
            gSmackEnded = true;
        }
    }
    if (gSmackOptions[gMovieId].fadeOut) {
        memcpy(gBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
        gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
        FillBitmapArea(
            gWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0
        );
        BlitBitmapToScreen(
            gWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0,
            0
        );
    } else if (!gSmackEnded) {
        memcpy(gBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
        gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_NORMAL, NULL);
        FillBitmapArea(
            gWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0
        );
        BlitBitmapToScreen(
            gWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0,
            0
        );
    }
    if (gSmackTrackSummary)
        SmackSummary(gSmackPrimary, &gSmackSummary);
closeMovies:
    CloseSmackers();
    InitAudio();
    memcpy(gPalette->m_data, gSmackSavedPalette, PALETTE_DATA_SIZE);
    UpdatePalette(gPalette->m_data);
    gMouseManager->ReallyShowPointer();
    if (gSmackResource)
        gResourceManager->Dispose(gSmackResource);
    gSmackResource = NULL;
    if (gSmackFont) {
        gResourceManager->Dispose(gSmackFont);
        gSmackFont = NULL;
    }
}

void CloseSmackers() {
    if (gSmackPrimary)
        SmackClose(gSmackPrimary);
    gSmackPrimary = NULL;
    if (gSmackCompanion)
        SmackClose(gSmackCompanion);
    gSmackCompanion = NULL;
    ShutdownSmackSound();
}

i32 PlaySmacker(i32 smackNumber) {
    i8 savedPalette[PALETTE_DATA_SIZE];
    i32 savedColorCycling;
    gInSmacker = true;
    gSmackEnded = false;
    memcpy(savedPalette, gBufferPalette->m_data, PALETTE_DATA_SIZE);
    savedColorCycling = gWindowManager->m_colorCycling;
    gWindowManager->m_colorCycling = 0;
    StopMusic();
    gMovieId = smackNumber;
    // Videos play only when the player enabled them.
    if (gConfig.playVideos)
        SmackMain();
    memcpy(gBufferPalette->m_data, savedPalette, PALETTE_DATA_SIZE);
    gWindowManager->m_colorCycling = savedColorCycling;
    gInSmacker = false;
    return gSmackEnded;
}
