// Buka movie playback.

#include <match.h>

#include <mss.h>

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

#include <stdio.h>
#include <string.h>

DATA(0x0049f7c0)
static SmackSoundFormat gSmackSoundFormats[12] = {
    {WAVE_FORMAT_4S16, 2, 44100, 16},
    {WAVE_FORMAT_4S08, 2, 44100, 8},
    {WAVE_FORMAT_4M16, 1, 44100, 16},
    {WAVE_FORMAT_4M08, 1, 44100, 8},
    {WAVE_FORMAT_2S16, 2, 22050, 16},
    {WAVE_FORMAT_2S08, 2, 22050, 8},
    {WAVE_FORMAT_2M16, 1, 22050, 16},
    {WAVE_FORMAT_2M08, 1, 22050, 8},
    {WAVE_FORMAT_1S16, 2, 11025, 16},
    {WAVE_FORMAT_1S08, 2, 11025, 8},
    {WAVE_FORMAT_1M16, 1, 11025, 16},
    {WAVE_FORMAT_1M08, 1, 11025, 8}
};
DATA(0x0049f850)
SSmackOptions SmackOptions[6] = {
    {"BUKA", "", 1, 1, 1, 0, 0, 0, 0},
    {"NWCLOGO", "", 1, 1, 1, 0, 0, 0, 0},
    {"INTRO", "", 1, 1, 1, 0, 0, 0, 0},
    {"LOSE", "", 1, 1, 0, 0, 0, 0, 0},
    {"WIN1", "", 1, 1, 0, 0, 0, 0, 0},
    {"WIN2", "", 1, 1, 0, 0, 0, 0, 0}
};
DATA(0x0049f8f4)
static i32 gSmackVolumes[11] = {0, 127, 97, 75, 52, 40, 30, 20, 15, 10, 5};
DATA(0x004cc8d0)
i8 gMovieId;
DATA(0x004cc8d4)
static i32 gSmackEnded;
DATA(0x004cc8d8)
static WAVEOUTCAPS gSmackWaveCaps;
DATA(0x004cc910)
static SmackSum gSmackSummary;
DATA(0x004cc964)
static i8 gSmackSavedPalette[PALETTE_DATA_SIZE];
DATA(0x004ccc68)
static i8 gSmackStop;
DATA(0x004ccc70)
static SmackSoundFormat gSmackAudioFormat;
DATA(0x004ccc80)
static PCMWAVEFORMAT gSmackPcmFormat;
DATA(0x004ccc90)
static i32 gSmackPrevFrame;
#define gSmackSound gSmkSounds // spelling fixes .bss order
DATA(0x004ccc94)
static i32 gSmackSound;
#define gSmackResource gSmkResource // spelling fixes .bss order
DATA(0x004ccc98)
static resource* gSmackResource;
DATA(0x004ccc9c)
static font* gSmackFont;
DATA(0x004ccca0)
static HDIGDRIVER gSmackDigDriver;
DATA(0x004ccca4)
static i32 gSmackTrackSummary;
DATA(0x004ccca8)
static Smack* gSmackPrimary;
#define gSmackCompanion gSmackSecond // spelling fixes .bss order
DATA(0x004cccac)
static Smack* gSmackCompanion;

VA(0x00458240, 0x18f)
void InitSmackSound() {
    if (gSmackDigDriver)
        return;
    if (!waveOutGetNumDevs())
        return;
    if (waveOutGetDevCaps(0, &gSmackWaveCaps, sizeof(gSmackWaveCaps)))
        return;
    gSmackAudioFormat.format = 0;
    for (u32 i = 0; i < 12; ++i) {
        if (gSmackWaveCaps.dwFormats & gSmackSoundFormats[i].format) {
            gSmackAudioFormat.format = gSmackSoundFormats[i].format;
            gSmackAudioFormat.channels = gSmackSoundFormats[i].channels;
            gSmackAudioFormat.samplesPerSecond = gSmackSoundFormats[i].samplesPerSecond;
            gSmackAudioFormat.bitsPerSample = gSmackSoundFormats[i].bitsPerSample;
            break;
        }
    }
    if (!gSmackAudioFormat.format) {
        gSmackAudioFormat.channels = 1;
        gSmackAudioFormat.samplesPerSecond = 22050;
        gSmackAudioFormat.bitsPerSample = 8;
    }
    AIL_startup();
    gSmackPcmFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
    gSmackPcmFormat.wf.nChannels = gSmackAudioFormat.channels;
    gSmackPcmFormat.wf.nSamplesPerSec = gSmackAudioFormat.samplesPerSecond;
    gSmackPcmFormat.wf.nAvgBytesPerSec = gSmackAudioFormat.samplesPerSecond
                                         * (gSmackAudioFormat.bitsPerSample / 8)
                                         * gSmackAudioFormat.channels;
    gSmackPcmFormat.wf.nBlockAlign =
        (gSmackAudioFormat.bitsPerSample / 8) * gSmackAudioFormat.channels;
    gSmackPcmFormat.wBitsPerSample = gSmackAudioFormat.bitsPerSample;
    if (AIL_waveOutOpen(&gSmackDigDriver, NULL, 0, &gSmackPcmFormat.wf))
        gSmackDigDriver = NULL;
}

VA(0x004583cf, 0x2a)
void ShutdownSmackSound() {
    if (gSmackDigDriver) {
        AIL_waveOutClose(gSmackDigDriver);
        gSmackDigDriver = NULL;
        AIL_shutdown();
    }
}

VA(0x004583f9, 0x3a)
void ConvertSmackerPalette(u8* paletteData) {
    for (i32 i = 0; i < PALETTE_DATA_SIZE; ++i)
        paletteData[i] = paletteData[i] >> WINGRAPH_PALETTE_VALUE_SHIFT;
}

VA(0x00458433, 0x13c)
void DoAdvance(Smack* smack, i32 drawFrame, i32 advanceFrame, i32 updatePalette, i32 skipPalette) {
    if (drawFrame && smack->NewPalette && !skipPalette) {
        memcpy(gPalette->m_data, smack->Palette, PALETTE_DATA_SIZE);
        // API-forced: ConvertSmackerPalette takes u8*.
        ConvertSmackerPalette(reinterpret_cast<u8*>(gPalette->m_data));
        if (updatePalette)
            UpdatePalette(gPalette->m_data);
    }
    SmackDoFrame(smack);
    if (drawFrame) {
        while (SmackToBufferRect(smack, SMACK_SURFACE_SLOW)) {
            if (gMovieId == SMACK_WIN2 && smack->FrameNum >= 22) {
                // language-forced: the assertion tests the pointer as an integer.
#line 178 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\SMACKMGR.CPP"
                H1_ASSERT(reinterpret_cast<i32>(gWinText));
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

VA(0x0045856f, 0x73c)
void SmackMain() {
    i32 soundFlags;
    i32 preloadFlags;
    i32 active;
    i32 primaryOn;
    i32 companionOn;
    i32 unusedTrue = 1;
    gSmackPrevFrame = 0;
    i32 unusedPlaybackState = 0;
    i32 unusedTimer;
    i32 unusedKey;
    gSmackFont = gResourceManager->GetFont("bigfont.fnt");
    KBChangeMenu(gDefaultMenu);
    gMouseManager->ReallyHidePointer();
    gSmackStop = 1;
    memcpy(gSmackSavedPalette, gPalette->m_data, PALETTE_DATA_SIZE);
    ShutdownAudio();
    InitSmackSound();
    if (gNoSound || !gSmackDigDriver || !gConfig.soundVolume) {
        gSmackSound = 0;
    } else {
        gSmackSound = 1;
        AIL_set_digital_master_volume(gSmackDigDriver, gSmackVolumes[gConfig.soundVolume]);
        SmackSoundUseMSS(gSmackDigDriver);
    }
    sprintf(gText, "%s%s.SMK", gAnimPath, SmackOptions[gMovieId].fileName);
    soundFlags = gSmackSound ? SMACK_TRACKS : 0;
    preloadFlags = SmackOptions[gMovieId].preload ? SMACK_PRELOAD_ALL : 0;
    gSmackPrimary = SmackOpen(gText, soundFlags + preloadFlags, SMACK_AUTO_EXTRA);
    SmackToBuffer(
        gSmackPrimary,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        gWindowManager->m_screen->m_pixels,
        0
    );
    if (strlen(SmackOptions[gMovieId].companionFileName) > 1) {
        sprintf(gText, "%s%s.SMK", gAnimPath, SmackOptions[gMovieId].companionFileName);
        gSmackCompanion = SmackOpen(gText, soundFlags, SMACK_AUTO_EXTRA);
        if (SmackOptions[gMovieId].drawCompanion)
            SmackToBuffer(
                gSmackCompanion,
                SmackOptions[gMovieId].companionX,
                SmackOptions[gMovieId].companionY,
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
    if (SmackOptions[gMovieId].fadeIn)
        gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_NORMAL, NULL);
    active = 1;
    primaryOn = 0;
    companionOn = 0;
    while (active) {
        if (!SmackWait(gSmackPrimary)) {
            if (!primaryOn || gSmackPrimary->Frames > 1)
                DoAdvance(gSmackPrimary, 1, 1, primaryOn || !SmackOptions[gMovieId].fadeIn, 0);
            if (gSmackPrimary->FrameNum > 0 || gSmackPrimary->Frames <= 1) {
                if (!primaryOn && SmackOptions[gMovieId].fadeIn) {
                    memcpy(gBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
                    gWindowManager->FadeScreen(WINDOW_FADE_IN, 4, NULL);
                }
                primaryOn = 1;
            }
        }
        if (gSmackCompanion && primaryOn && !SmackWait(gSmackCompanion)) {
            if (companionOn && gSmackCompanion->FrameNum == gSmackCompanion->Frames - 1) {
                i32 drawLastFrame;
                if (SmackOptions[gMovieId].drawCompanion)
                    drawLastFrame = 1;
                else
                    drawLastFrame = 0;
                DoAdvance(gSmackCompanion, drawLastFrame, 0, 0, 1);
                gSmackPrevFrame = 1;
                while (SmackWait(gSmackCompanion))
                    Process1WindowsMessage();
            } else {
                DoAdvance(gSmackCompanion, SmackOptions[gMovieId].drawCompanion, 1, 0, 1);
            }
            if (gSmackCompanion && gSmackCompanion->FrameNum > 0)
                companionOn = 1;
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
                active = 0;
                continue;
        }
        if (!SmackOptions[gMovieId].waitForInput
            && (gSmackPrevFrame
                || (gSmackCompanion
                    && (gSmackCompanion->FrameNum >= gSmackCompanion->Frames
                        || (gSmackCompanion->FrameNum <= 0 && companionOn)))
                || (!gSmackCompanion
                    && (gSmackPrimary->FrameNum >= gSmackPrimary->Frames
                        || (gSmackPrimary->FrameNum <= 0 && primaryOn))))) {
            active = 0;
            gSmackEnded = 1;
        }
    }
    if (SmackOptions[gMovieId].fadeOut) {
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

VA(0x00458cab, 0x49)
void CloseSmackers() {
    if (gSmackPrimary)
        SmackClose(gSmackPrimary);
    gSmackPrimary = NULL;
    if (gSmackCompanion)
        SmackClose(gSmackCompanion);
    gSmackCompanion = NULL;
    ShutdownSmackSound();
}

VA(0x00458cf4, 0xa9)
i32 PlaySmacker(H1_ENUM_PARAM(SmackVideo, i32) smackNumber) {
    i8 savedPalette[PALETTE_DATA_SIZE];
    i32 savedUpdateFlags;
    gInSmacker = 1;
    gSmackEnded = 0;
    memcpy(savedPalette, gBufferPalette->m_data, PALETTE_DATA_SIZE);
    savedUpdateFlags = gWindowManager->m_updateFlags;
    gWindowManager->m_updateFlags = 0;
    StopMusic();
    gMovieId = smackNumber;
    SmackMain();
    memcpy(gBufferPalette->m_data, savedPalette, PALETTE_DATA_SIZE);
    gWindowManager->m_updateFlags = savedUpdateFlags;
    gInSmacker = 0;
    return gSmackEnded;
}
