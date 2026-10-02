// HoMM1 smackManager: an executive-driven manager, unlike HoMM2's
// SmackManagerMain loop. The TU begins after int3 alignment at 0x45abe0.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/soundManager.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/smack.h>
#include <SOURCE/smackManager.h>
#include <SOURCE/wingraph.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
// SmackOptions rows (retail 0x0049fd08), named by their movie files. Rows
// 0..1 are the publisher logos that draw "Presents...", 2..3 the intro and
// 4..7 the endings; oldmain and the end sequence pick one of each pair
// from gConfig.slowVideo.
H1_ENUM_BEGIN(SmackVideo)
    SMACK_NWCLOGO = 0,
    SMACK_NWCLOGO1 = 1,
    SMACK_LOGO_LAST = SMACK_NWCLOGO1,
    SMACK_INTRO02C = 2,
    SMACK_INTRO_FIRST = SMACK_INTRO02C,
    SMACK_INTRO02U = 3,
    SMACK_INTRO_LAST = SMACK_INTRO02U,
    SMACK_WIN01C = 4,
    SMACK_WIN01U = 5,
    SMACK_WIN02 = 6,
    SMACK_LOSE1 = 7
H1_ENUM_END(SmackVideo)
// clang-format on

// RAD library allocation callbacks; HoMM1 links SMACKW32.DLL, so neither is
// reached.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0045abe0, 0x4c)
H1_C_LINKAGE void *radmalloc(unsigned long numbytes) {
    void *mem;
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
H1_C_LINKAGE void radfree(void *ptr) {
    free(ptr);
}

VA(0x0045ac48, 0x2a)
smackManager::smackManager(void) : baseManager() {
}

VA(0x0045ac72, 0x53)
short smackManager::Open(short priority) {
    gbSmackAborted = 0;
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "smackManager");
    return 0;
}

VA(0x0045acc5, 0x1f)
void smackManager::Close(void) {
    m_active = 0;
}

VA(0x0045ace4, 0x975)
short smackManager::Main(struct tag_message &msg) {
    char savedUpdateFlags;
    int startFrame;
    int currentFrame;
    palette *pPalette;
    signed char *savedPal;
    font *bigFont;
    Smack *smk;
    long frameStartTick;
    long lastTick;

    startFrame = 1;
    KBChangeMenu(hmnuDflt);
    if (gbSmackAborted)
        return 1;
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
    smk = SmackOpen(gText, SmackOptions[bSmackNum].openFlags | (gpSoundManager->m_digitalDriver ? 0xfe00 : 0), -1);
    LogStr("SmackM3b");
    if (smk) {
        FillBitmapArea(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0);
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0, 0);
        SmackSoundOnOff(smk, gConfig.musicVolume);
        if (gbSkipIntro && (bSmackNum == SMACK_INTRO02C || bSmackNum == SMACK_INTRO02U)) {
            startFrame = 125;
            SmackVolumePan(smk, 0x200, 0, 0);
            SmackSoundOnOff(smk, 0);
            SmackGoto(smk, startFrame);
            SmackSoundOnOff(smk, gConfig.musicVolume);
            if (smk->paletteSelector == 1)
                pPalette->m_data = reinterpret_cast<signed char*>(smk->Palette); // API-forced: Smacker palettes are unsigned bytes.
            else
                pPalette->m_data = reinterpret_cast<signed char*>(smk->alternatePalette); // API-forced: Smacker palettes are unsigned bytes.
            SetPalette(pPalette->m_data, 1);
            gbFirstTimeThrough = 1;
        }
        gpWindowManager->m_updateFlags = SmackOptions[bSmackNum].updateFlags;
        if (SmackOptions[bSmackNum].fadeIn)
            gpWindowManager->FadeScreen(1, 8, NULL);
        SmackToBuffer(smk, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, gpWindowManager->m_screen->m_pixels, 0);
        for (currentFrame = startFrame; currentFrame <= smk->Frames; currentFrame++) {
            SmackDoFrame(smk);
            if (SmackOptions[bSmackNum].fadeIn && currentFrame == startFrame) {
                if (smk->paletteSelector == 1)
                    pPalette->m_data = reinterpret_cast<signed char*>(smk->Palette); // API-forced: Smacker palettes are unsigned bytes.
                else
                    pPalette->m_data = reinterpret_cast<signed char*>(smk->alternatePalette); // API-forced: Smacker palettes are unsigned bytes.
                if (giMainVideoModeColorDepth == 8 || gConfig.gfx[giCurExe].fullScreen) {
                    while (SmackToBufferRect(smk, 1))
                        BlitBitmapToScreen(gpWindowManager->m_screen, smk->LastRectx, smk->LastRecty,
                                           smk->LastRectw, smk->LastRecth, smk->LastRectx, smk->LastRecty);
                }
                gpWindowManager->FadeScreen(0, 8, pPalette);
            } else {
                if (bSmackNum == SMACK_WIN02 && currentFrame >= 23)
                    bigFont->DrawBoundedString(gcCongratsText, 29, 338, 325, 115, 1, 1);
                if (bSmackNum == SMACK_NWCLOGO) {
                    bigFont->DrawString("Presents...", 280, 440, 255);
                    gpWindowManager->UpdateScreenRegion(280, 440, 80, 20);
                }
                while (SmackToBufferRect(smk, 1))
                    BlitBitmapToScreen(gpWindowManager->m_screen, smk->LastRectx, smk->LastRecty,
                                       smk->LastRectw, smk->LastRecth, smk->LastRectx, smk->LastRecty);
            }
            Process1WindowsMessage();
            if (smk->Frames != currentFrame)
                SmackNextFrame(smk);
            frameStartTick = KBTickCount();
            lastTick = frameStartTick;
            while (KBTickCount() < frameStartTick + 300
                   && (SmackWait(smk) || KBTickCount() < frameStartTick + 25
                       || (!gConfig.musicVolume && KBTickCount() < frameStartTick + 50))) {
                if (KBTickCount() > lastTick + 25) {
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
            SmackVolumePan(smk, 0x200, 0, 0);
            SmackSoundOnOff(smk, 0);
            SmackGoto(smk, 101);
            SmackDoFrame(smk);
            while (SmackToBufferRect(smk, 1))
                BlitBitmapToScreen(gpWindowManager->m_screen, smk->LastRectx, smk->LastRecty, smk->LastRectw,
                                   smk->LastRecth, smk->LastRectx, smk->LastRecty);
            bigFont->DrawString("Presents...", 280, 440, 255);
            gpWindowManager->UpdateScreenRegion(280, 440, 80, 20);
        }
        if (bSmackNum == SMACK_NWCLOGO1)
            gbSkipIntro = 1;
        if (bSmackNum == SMACK_WIN02) {
            DelayMilli(4500);
            bigFont->DrawString("Press a Key to Continue...", 420, 460, 1);
            gpWindowManager->UpdateScreenRegion(420, 460, 220, 20);
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
                pPalette->m_data = reinterpret_cast<signed char*>(smk->Palette); // API-forced: Smacker palettes are unsigned bytes.
            else
                pPalette->m_data = reinterpret_cast<signed char*>(smk->alternatePalette); // API-forced: Smacker palettes are unsigned bytes.
            gpWindowManager->FadeScreen(1, 8, pPalette);
            FillBitmapArea(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0);
            BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0, 0);
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
    return 2;
}

VA(0x0045b659, 0x93)
void PlaySmacker(signed char smackNumber) {
    gbInSmacker = 1;
    gpSoundManager->m_musicReady = 1;
    gpSoundManager->PlayAmbientMusic(-1, 0, -1);
    bSmackNum = smackNumber;
    if (gpExec->AddManager(gpSmackManager, -1))
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
