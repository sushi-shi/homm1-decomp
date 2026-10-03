// HoMM1 smackManager: an executive-driven manager, unlike HoMM2's
// SmackManagerMain loop. The TU begins after int3 alignment at 0x45abe0.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/soundManager.h>
#include <BASE/soundmgr.h>
#include <BASE/WINMGR_TYPES.h>
#include <H1/Macros.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/smack.h>
#include <SOURCE/smackManager.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// RAD library allocation callbacks; HoMM1 links SMACKW32.DLL, so neither is
// reached.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0045abe0, 0x4c)
H1_C_LINKAGE void* radmalloc(unsigned long numbytes) {
    void* mem;
    if (numbytes == 0)
        return NULL;
    if (numbytes != 0xffffffff)
        mem = malloc(numbytes);
    else
        mem = NULL;
    return mem;
}

// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0045ac2c, 0x1c)
H1_C_LINKAGE void radfree(void* ptr) {
    free(ptr);
}

// smackManager::Main's playback (HoMM1 movies): a skipped intro restarts at
// frame 125; the publisher logo freezes on frame 101 under its "Presents..."
// caption (bigfont, colour 255, an 80x20 update box); win02 draws the win
// text box from frame 23 and waits 4.5 s before the white "Press a Key"
// prompt (a 220x20 box); each frame waits at most 300 ms, at least 25 ms,
// 50 ms when the music is off, pumping messages every 25 ms.
H1_ENUM_CONST_BEGIN(SmackManagerConstant)
    SMACK_INTRO_SKIP_FRAME = 125,
    SMACK_LOGO_FINAL_FRAME = 101,
    SMACK_WIN_TEXT_FRAME = 23,
    SMACK_WIN_TEXT_X = 29,
    SMACK_WIN_TEXT_Y = 338,
    SMACK_WIN_TEXT_WIDTH = 325,
    SMACK_WIN_TEXT_HEIGHT = 115,
    SMACK_PRESENTS_X = 280,
    SMACK_PRESENTS_Y = 440,
    SMACK_PRESENTS_WIDTH = 80,
    SMACK_PRESENTS_COLOR = 255,
    SMACK_PROMPT_X = 420,
    SMACK_PROMPT_Y = 460,
    SMACK_PROMPT_WIDTH = 220,
    SMACK_CAPTION_HEIGHT = 20,
    SMACK_WIN_PROMPT_DELAY = 4500,
    SMACK_FRAME_MAX_WAIT = 300,
    SMACK_FRAME_MIN_WAIT = 25,
    SMACK_FRAME_SILENT_WAIT = 50,
    SMACK_MESSAGE_PUMP_INTERVAL = 25
H1_ENUM_CONST_END(SmackManagerConstant)

VA(0x0045ac48, 0x2a)
smackManager::smackManager(void) : baseManager() {}

VA(0x0045ac72, 0x53)
short smackManager::Open(short priority) {
    gbSmackAborted = 0;
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "smackManager");
    return BASE_MANAGER_SUCCESS;
}

VA(0x0045acc5, 0x1f)
void smackManager::Close(void) {
    m_active = 0;
}

VA(0x0045ace4, 0x975)
short smackManager::Main(struct tag_message& msg) {
    char savedUpdateFlags;
    int startFrame;
    int currentFrame;
    palette* pPalette;
    signed char* savedPal;
    font* bigFont;
    Smack* smk;
    long frameStartTick;
    long lastTick;

    startFrame = 1;
    KBChangeMenu(hmnuDflt);
    if (gbSmackAborted)
        return MESSAGE_DISPATCH_CONSUME;
    gbSmackAborted = 1;
    LogStr("SmackM1");
    pPalette = new palette;
    if (!pPalette)
        MemError();
    savedPal = pPalette->m_data;
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    SmackSoundUseMSS(gpSoundManager->m_digitalDriver);
    LogStr("SmackM2");
    savedUpdateFlags = gpWindowManager->m_updateFlags;
    sprintf(gText, "%s%s", gcAnimPath, SmackOptions[bSmackNum].fileName);
    LogStr("SmackM2b");
    smk = SmackOpen(
        gText,
        SmackOptions[bSmackNum].openFlags | (gpSoundManager->m_digitalDriver ? SMACK_TRACKS : 0),
        SMACK_AUTO_EXTRA
    );
    LogStr("SmackM3b");
    if (smk) {
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
        SmackSoundOnOff(smk, gConfig.musicVolume);
        if (gbSkipIntro && (bSmackNum == SMACK_INTRO02C || bSmackNum == SMACK_INTRO02U)) {
            startFrame = SMACK_INTRO_SKIP_FRAME;
            SmackVolumePan(smk, SMACK_TRACK_1, 0, 0);
            SmackSoundOnOff(smk, 0);
            SmackGoto(smk, startFrame);
            SmackSoundOnOff(smk, gConfig.musicVolume);
            if (smk->paletteSelector == 1)
                pPalette->m_data = reinterpret_cast<signed char*>(
                    smk->Palette
                ); // API-forced: Smacker palettes are unsigned bytes.
            else
                pPalette->m_data = reinterpret_cast<signed char*>(
                    smk->alternatePalette
                ); // API-forced: Smacker palettes are unsigned bytes.
            SetPalette(pPalette->m_data, 1);
            gbFirstTimeThrough = 1;
        }
        gpWindowManager->m_updateFlags = SmackOptions[bSmackNum].updateFlags;
        if (SmackOptions[bSmackNum].fadeIn)
            gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
        SmackToBuffer(
            smk,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            gpWindowManager->m_screen->m_pixels,
            0
        );
        for (currentFrame = startFrame; currentFrame <= smk->Frames; currentFrame++) {
            SmackDoFrame(smk);
            if (SmackOptions[bSmackNum].fadeIn && currentFrame == startFrame) {
                if (smk->paletteSelector == 1)
                    pPalette->m_data = reinterpret_cast<signed char*>(
                        smk->Palette
                    ); // API-forced: Smacker palettes are unsigned bytes.
                else
                    pPalette->m_data = reinterpret_cast<signed char*>(
                        smk->alternatePalette
                    ); // API-forced: Smacker palettes are unsigned bytes.
                if (giMainVideoModeColorDepth == 8 || gConfig.gfx[giCurExe].fullScreen) {
                    while (SmackToBufferRect(smk, SMACK_SURFACE_SLOW))
                        BlitBitmapToScreen(
                            gpWindowManager->m_screen,
                            smk->LastRectx,
                            smk->LastRecty,
                            smk->LastRectw,
                            smk->LastRecth,
                            smk->LastRectx,
                            smk->LastRecty
                        );
                }
                gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, pPalette);
            } else {
                if (bSmackNum == SMACK_WIN02 && currentFrame >= SMACK_WIN_TEXT_FRAME)
                    bigFont->DrawBoundedString(
                        gcWinText,
                        SMACK_WIN_TEXT_X,
                        SMACK_WIN_TEXT_Y,
                        SMACK_WIN_TEXT_WIDTH,
                        SMACK_WIN_TEXT_HEIGHT,
                        1,
                        FONT_ALIGN_CENTER
                    );
                if (bSmackNum == SMACK_NWCLOGO) {
                    bigFont->DrawString(
                        "Presents...",
                        SMACK_PRESENTS_X,
                        SMACK_PRESENTS_Y,
                        SMACK_PRESENTS_COLOR
                    );
                    gpWindowManager->UpdateScreenRegion(
                        SMACK_PRESENTS_X,
                        SMACK_PRESENTS_Y,
                        SMACK_PRESENTS_WIDTH,
                        SMACK_CAPTION_HEIGHT
                    );
                }
                while (SmackToBufferRect(smk, SMACK_SURFACE_SLOW))
                    BlitBitmapToScreen(
                        gpWindowManager->m_screen,
                        smk->LastRectx,
                        smk->LastRecty,
                        smk->LastRectw,
                        smk->LastRecth,
                        smk->LastRectx,
                        smk->LastRecty
                    );
            }
            Process1WindowsMessage();
            if (smk->Frames != currentFrame)
                SmackNextFrame(smk);
            frameStartTick = KBTickCount();
            lastTick = frameStartTick;
            while (KBTickCount() < frameStartTick + SMACK_FRAME_MAX_WAIT
                   && (SmackWait(smk) || KBTickCount() < frameStartTick + SMACK_FRAME_MIN_WAIT
                       || (!gConfig.musicVolume
                           && KBTickCount() < frameStartTick + SMACK_FRAME_SILENT_WAIT))) {
                if (KBTickCount() > lastTick + SMACK_MESSAGE_PUMP_INTERVAL) {
                    Process1WindowsMessage();
                    lastTick = KBTickCount();
                }
            }
            if (!gbFirstTimeThrough || bSmackNum > SMACK_INTRO_LAST) {
                Process1WindowsMessage();
                switch (gpInputManager->GetEvent().type) {
                    case MESSAGE_KEY_DOWN:
                    case MESSAGE_LEFT_BUTTON_DOWN:
                    case MESSAGE_RIGHT_BUTTON_DOWN:
                        if (bSmackNum >= SMACK_INTRO_FIRST)
                            break;
                        else {
                            currentFrame = smk->Frames;
                            gbSkipIntro = 1;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        if (bSmackNum <= SMACK_LOGO_LAST) {
            SmackVolumePan(smk, SMACK_TRACK_1, 0, 0);
            SmackSoundOnOff(smk, 0);
            SmackGoto(smk, SMACK_LOGO_FINAL_FRAME);
            SmackDoFrame(smk);
            while (SmackToBufferRect(smk, SMACK_SURFACE_SLOW))
                BlitBitmapToScreen(
                    gpWindowManager->m_screen,
                    smk->LastRectx,
                    smk->LastRecty,
                    smk->LastRectw,
                    smk->LastRecth,
                    smk->LastRectx,
                    smk->LastRecty
                );
            bigFont->DrawString(
                "Presents...",
                SMACK_PRESENTS_X,
                SMACK_PRESENTS_Y,
                SMACK_PRESENTS_COLOR
            );
            gpWindowManager->UpdateScreenRegion(
                SMACK_PRESENTS_X,
                SMACK_PRESENTS_Y,
                SMACK_PRESENTS_WIDTH,
                SMACK_CAPTION_HEIGHT
            );
        }
        if (bSmackNum == SMACK_NWCLOGO1)
            gbSkipIntro = 1;
        if (bSmackNum == SMACK_WIN02) {
            DelayMilli(SMACK_WIN_PROMPT_DELAY);
            bigFont->DrawString("Press a Key to Continue...", SMACK_PROMPT_X, SMACK_PROMPT_Y, 1);
            gpWindowManager->UpdateScreenRegion(
                SMACK_PROMPT_X,
                SMACK_PROMPT_Y,
                SMACK_PROMPT_WIDTH,
                SMACK_CAPTION_HEIGHT
            );
            for (;;) {
                Process1WindowsMessage();
                switch (gpInputManager->GetEvent().type) {
                    case MESSAGE_KEY_DOWN:
                    case MESSAGE_LEFT_BUTTON_DOWN:
                    case MESSAGE_RIGHT_BUTTON_DOWN:
                        goto pressed;
                }
            }
        pressed:;
        }
        if (SmackOptions[bSmackNum].fadeOut) {
            if (smk->paletteSelector == 1)
                pPalette->m_data = reinterpret_cast<signed char*>(
                    smk->Palette
                ); // API-forced: Smacker palettes are unsigned bytes.
            else
                pPalette->m_data = reinterpret_cast<signed char*>(
                    smk->alternatePalette
                ); // API-forced: Smacker palettes are unsigned bytes.
            gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, pPalette);
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
        SmackClose(smk);
    }
    gpResourceManager->Dispose(bigFont);
    pPalette->m_data = savedPal;
    delete pPalette;
    if (bSmackNum)
        gpWindowManager->m_updateFlags = savedUpdateFlags;
    msg.type = MESSAGE_EXECUTIVE;
    msg.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x0045b659, 0x93)
void PlaySmacker(H1_ENUM_PARAM(SmackVideo, signed char) smackNumber) {
    gbInSmacker = 1;
    gpSoundManager->m_musicReady = 1;
    gpSoundManager->PlayAmbientMusic(MUSIC_TRACK_NONE, 0, SOUND_VOLUME_FROM_CONFIG);
    bSmackNum = smackNumber;
    if (gpExec->AddManager(gpSmackManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
        ShutDown("Can't add manager!");
    gpExec->MainLoop();
    gpExec->RemoveManager(gpSmackManager);
    gbInSmacker = 0;
}

// SMACKMGR owns retail .data 0x0049fd08-0x0049fe4f and .bss 0x004ca488-0x004ca48f.
DATA(0x0049fd08)
SSmackOptions SmackOptions[8] = {
    {"nwclogo.smk", 0, 1, 0, 32},
    {"nwclogo1.smk", 0, 1, 0, 32},
    {"intro02c.smk", 1, 1, 0, 32},
    {"intro02u.smk", 1, 1, 0, 32},
    {"win01c.smk", 1, 1, 1, 0},
    {"win01u.smk", 1, 1, 1, 0},
    {"win02.smk", 0, 1, 1, 0},
    {"lose1.smk", 0, 1, 1, 0},
};
DATA(0x004ca488)
signed char bSmackNum;
DATA(0x004ca48c)
signed char gbSmackAborted;
