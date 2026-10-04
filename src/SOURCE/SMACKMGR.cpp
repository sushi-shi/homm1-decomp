// Buka movie playback, reconstructed from retail instructions and CFGs.
#include <match.h>
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
#include <mss.h>
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)
struct SmackSoundFormat {
    DWORD format;
    WORD channels;
    DWORD samplesPerSecond;
    WORD bitsPerSample;
};
#pragma pack(pop)

// Buka retail VA 0x0049f7c0: twelve packed WAVE_FORMAT capability choices.
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
// Buka retail VA 0x0049f850.
SSmackOptions SmackOptions[6] = {
    {"BUKA", "", 1, 1, 1, 0, 0, 0, 0},
    {"NWCLOGO", "", 1, 1, 1, 0, 0, 0, 0},
    {"INTRO", "", 1, 1, 1, 0, 0, 0, 0},
    {"LOSE", "", 1, 1, 0, 0, 0, 0, 0},
    {"WIN1", "", 1, 1, 0, 0, 0, 0, 0},
    {"WIN2", "", 1, 1, 0, 0, 0, 0, 0}
};
// Buka retail VA 0x0049f8f4.
static i32 gSmackVolumes[11] = {0, 127, 97, 75, 52, 40, 30, 20, 15, 10, 5};
// Buka retail VA 0x004cc8d0.
i8 gSmackNum;
// Buka retail VA 0x004cc8d4. Set on natural completion; a user skip leaves zero.
static i32 gSmackCompleted;
// Buka retail VA 0x004cc8d8.
static WAVEOUTCAPS gSmackWaveCaps;
// Buka retail VA 0x004cc910.
static SmackSum gSmackSummary;
// Buka retail VA 0x004cc964.
static i8 gSmackSavedPalette[PALETTE_DATA_SIZE];
// Buka retail VA 0x004ccc68.
static i8 gSmackMainDone;
// Buka retail VA 0x004ccc70.
static SmackSoundFormat gSmackSoundFormat;
// Buka retail VA 0x004ccc80.
static PCMWAVEFORMAT gSmackWaveFormat;
// Buka retail VA 0x004ccc90.
static i32 gSmackLastFramePlayed;
// Buka retail VA 0x004ccc94.
static i32 gSmackSound;
// Buka retail VA 0x004ccc98. The only surviving users dispose this resource.
static resource* gSmackResource;
// Buka retail VA 0x004ccc9c.
static font* gSmackFont;
// Buka retail VA 0x004ccca0.
static HDIGDRIVER gSmackDriver;
// Buka retail VA 0x004ccca4.
static i32 gSmackCollectSummary;
// Buka retail VA 0x004ccca8.
static Smack* gSmackPrimary;
// Buka retail VA 0x004cccac.
static Smack* gSmackCompanion;

// Buka retail VA 0x00458240, size 0x18f.
void InitSmackSound() {
    if (gSmackDriver)
        return;
    if (!waveOutGetNumDevs())
        return;
    if (waveOutGetDevCaps(0, &gSmackWaveCaps, sizeof(gSmackWaveCaps)))
        return;
    gSmackSoundFormat.format = 0;
    for (u32 i = 0; i < 12; ++i) {
        if (gSmackWaveCaps.dwFormats & gSmackSoundFormats[i].format) {
            gSmackSoundFormat.format = gSmackSoundFormats[i].format;
            gSmackSoundFormat.channels = gSmackSoundFormats[i].channels;
            gSmackSoundFormat.samplesPerSecond = gSmackSoundFormats[i].samplesPerSecond;
            gSmackSoundFormat.bitsPerSample = gSmackSoundFormats[i].bitsPerSample;
            break;
        }
    }
    if (!gSmackSoundFormat.format) {
        gSmackSoundFormat.channels = 1;
        gSmackSoundFormat.samplesPerSecond = 22050;
        gSmackSoundFormat.bitsPerSample = 8;
    }
    AIL_startup();
    gSmackWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
    gSmackWaveFormat.wf.nChannels = gSmackSoundFormat.channels;
    gSmackWaveFormat.wf.nSamplesPerSec = gSmackSoundFormat.samplesPerSecond;
    gSmackWaveFormat.wf.nAvgBytesPerSec = gSmackSoundFormat.samplesPerSecond
                                          * (gSmackSoundFormat.bitsPerSample / 8)
                                          * gSmackSoundFormat.channels;
    gSmackWaveFormat.wf.nBlockAlign =
        (gSmackSoundFormat.bitsPerSample / 8) * gSmackSoundFormat.channels;
    gSmackWaveFormat.wBitsPerSample = gSmackSoundFormat.bitsPerSample;
    if (AIL_waveOutOpen(&gSmackDriver, NULL, 0, &gSmackWaveFormat.wf))
        gSmackDriver = NULL;
}

// Buka retail VA 0x004583cf, size 0x2a.
void ShutdownSmackSound() {
    if (gSmackDriver) {
        AIL_waveOutClose(gSmackDriver);
        gSmackDriver = NULL;
        AIL_shutdown();
    }
}

// Buka retail VA 0x004583f9, size 0x3a.
void ConvertSmackerPalette(u8* paletteData) {
    for (i32 i = 0; i < PALETTE_DATA_SIZE; ++i)
        paletteData[i] = paletteData[i] >> WINGRAPH_PALETTE_VALUE_SHIFT;
}

// Buka retail VA 0x00458433, size 0x13c.
void DoAdvance(Smack* smack, i32 drawFrame, i32 advanceFrame, i32 updatePalette, i32 skipPalette) {
    if (drawFrame && smack->NewPalette && !skipPalette) {
        memcpy(gPalette->m_data, smack->Palette, PALETTE_DATA_SIZE);
        ConvertSmackerPalette(reinterpret_cast<u8*>(gPalette->m_data));
        if (updatePalette)
            UpdatePalette(gPalette->m_data);
    }
    SmackDoFrame(smack);
    if (drawFrame) {
        while (SmackToBufferRect(smack, SMACK_SURFACE_SLOW)) {
            if (gSmackNum == SMACK_WIN2 && smack->FrameNum >= 22) {
#line 178 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\SMACKMGR.CPP"
                H1_ASSERT(reinterpret_cast<i32>(gcWinText));
                gSmackFont->DrawBoundedString(gcWinText, 32, 342, 320, 106, 4, FONT_ALIGN_CENTER);
            }
            BlitBitmapToScreen(
                gpWindowManager->m_screen,
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

// Buka retail VA 0x0045856f, size 0x73c.
void SmackMain() {
    i32 soundFlags;
    i32 preloadFlags;
    i32 playing;
    i32 primaryStarted;
    i32 companionStarted;
    i32 unusedOne = 1;
    gSmackLastFramePlayed = 0;
    i32 unusedPlaybackState = 0;
    gSmackFont = gpResourceManager->GetFont("bigfont.fnt");
    KBChangeMenu(hmnuDflt);
    gpMouseManager->HideColorPointer();
    gSmackMainDone = 1;
    memcpy(gSmackSavedPalette, gPalette->m_data, PALETTE_DATA_SIZE);
    ShutdownAudio();
    InitSmackSound();
    if (gbNoSound || !gSmackDriver || !gConfig.soundVolume) {
        gSmackSound = 0;
    } else {
        gSmackSound = 1;
        AIL_set_digital_master_volume(gSmackDriver, gSmackVolumes[gConfig.soundVolume]);
        SmackSoundUseMSS(gSmackDriver);
    }
    sprintf(gText, "%s%s.SMK", gAnimPath, SmackOptions[gSmackNum].fileName);
    soundFlags = gSmackSound ? SMACK_TRACKS : 0;
    preloadFlags = SmackOptions[gSmackNum].preload ? SMACK_PRELOAD_ALL : 0;
    gSmackPrimary = SmackOpen(gText, soundFlags + preloadFlags, SMACK_AUTO_EXTRA);
    SmackToBuffer(
        gSmackPrimary,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        gpWindowManager->m_screen->m_pixels,
        0
    );
    if (strlen(SmackOptions[gSmackNum].companionFileName) > 1) {
        sprintf(gText, "%s%s.SMK", gAnimPath, SmackOptions[gSmackNum].companionFileName);
        gSmackCompanion = SmackOpen(gText, soundFlags, SMACK_AUTO_EXTRA);
        if (SmackOptions[gSmackNum].drawCompanion)
            SmackToBuffer(
                gSmackCompanion,
                SmackOptions[gSmackNum].companionX,
                SmackOptions[gSmackNum].companionY,
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT,
                gpWindowManager->m_screen->m_pixels,
                0
            );
    }
    FillBitmapArea(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0);
    BlitBitmapToScreen(
        gpWindowManager->m_screen,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        0,
        0
    );
    if (SmackOptions[gSmackNum].fadeIn)
        gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_NORMAL, NULL);
    playing = 1;
    primaryStarted = 0;
    companionStarted = 0;
    while (playing) {
        if (!SmackWait(gSmackPrimary)) {
            if (!primaryStarted || gSmackPrimary->Frames > 1)
                DoAdvance(
                    gSmackPrimary,
                    1,
                    1,
                    primaryStarted || !SmackOptions[gSmackNum].fadeIn,
                    0
                );
            if (gSmackPrimary->FrameNum > 0 || gSmackPrimary->Frames <= 1) {
                if (!primaryStarted && SmackOptions[gSmackNum].fadeIn) {
                    memcpy(gpBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
                    gpWindowManager->FadeScreen(WINDOW_FADE_IN, 4, NULL);
                }
                primaryStarted = 1;
            }
        }
        if (gSmackCompanion && primaryStarted && !SmackWait(gSmackCompanion)) {
            if (companionStarted && gSmackCompanion->FrameNum == gSmackCompanion->Frames - 1) {
                i32 drawLastFrame;
                if (SmackOptions[gSmackNum].drawCompanion)
                    drawLastFrame = 1;
                else
                    drawLastFrame = 0;
                DoAdvance(gSmackCompanion, drawLastFrame, 0, 0, 1);
                gSmackLastFramePlayed = 1;
                while (SmackWait(gSmackCompanion))
                    Process1WindowsMessage();
            } else {
                DoAdvance(gSmackCompanion, SmackOptions[gSmackNum].drawCompanion, 1, 0, 1);
            }
            if (gSmackCompanion && gSmackCompanion->FrameNum > 0)
                companionStarted = 1;
        }
        Process1WindowsMessage();
        tag_message message;
        message = gpInputManager->GetEvent();
        switch (message.type) {
            case MESSAGE_KEY_DOWN:
                if (message.keyCode == INPUT_SCAN_F4)
                    break;
            case MESSAGE_LEFT_BUTTON_DOWN:
            case MESSAGE_RIGHT_BUTTON_DOWN:
                playing = 0;
                continue;
        }
        if (!SmackOptions[gSmackNum].waitForInput
            && (gSmackLastFramePlayed
                || (gSmackCompanion
                    && (gSmackCompanion->FrameNum >= gSmackCompanion->Frames
                        || (gSmackCompanion->FrameNum <= 0 && companionStarted)))
                || (!gSmackCompanion
                    && (gSmackPrimary->FrameNum >= gSmackPrimary->Frames
                        || (gSmackPrimary->FrameNum <= 0 && primaryStarted))))) {
            playing = 0;
            gSmackCompleted = 1;
        }
    }
    if (SmackOptions[gSmackNum].fadeOut) {
        memcpy(gpBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
        gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
        FillBitmapArea(
            gpWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0
        );
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0,
            0
        );
    } else if (!gSmackCompleted) {
        memcpy(gpBufferPalette->m_data, gPalette->m_data, PALETTE_DATA_SIZE);
        gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_NORMAL, NULL);
        FillBitmapArea(
            gpWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0
        );
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0,
            0
        );
    }
    if (gSmackCollectSummary)
        SmackSummary(gSmackPrimary, &gSmackSummary);
    CloseSmackers();
    InitAudio();
    memcpy(gPalette->m_data, gSmackSavedPalette, PALETTE_DATA_SIZE);
    UpdatePalette(gPalette->m_data);
    gpMouseManager->ShowColorPointer();
    if (gSmackResource)
        gpResourceManager->Dispose(gSmackResource);
    gSmackResource = NULL;
    if (gSmackFont) {
        gpResourceManager->Dispose(gSmackFont);
        gSmackFont = NULL;
    }
}

// Buka retail VA 0x00458cab, size 0x49.
void CloseSmackers() {
    if (gSmackPrimary)
        SmackClose(gSmackPrimary);
    gSmackPrimary = NULL;
    if (gSmackCompanion)
        SmackClose(gSmackCompanion);
    gSmackCompanion = NULL;
    ShutdownSmackSound();
}

// Buka retail VA 0x00458cf4, size 0xa9.
i32 PlaySmacker(H1_ENUM_PARAM(SmackVideo, i8) smackNumber) {
    i8 savedPalette[PALETTE_DATA_SIZE];
    i32 savedUpdateFlags;
    gInSmacker = 1;
    gSmackCompleted = 0;
    memcpy(savedPalette, gpBufferPalette->m_data, PALETTE_DATA_SIZE);
    savedUpdateFlags = gpWindowManager->m_updateFlags;
    gpWindowManager->m_updateFlags = 0;
    StopMusic();
    gSmackNum = smackNumber;
    SmackMain();
    memcpy(gpBufferPalette->m_data, savedPalette, PALETTE_DATA_SIZE);
    gpWindowManager->m_updateFlags = savedUpdateFlags;
    gInSmacker = 0;
    return gSmackCompleted;
}
