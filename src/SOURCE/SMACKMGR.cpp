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

// RAD library allocation callbacks; HoMM1 links SMACKW32.DLL, so neither is
// reached.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0045abe0, 0x4c)
H1_C_LINKAGE void *radmalloc(unsigned long numbytes) {
    void *mem;
    if (numbytes == 0)
        return 0;
    if (numbytes != 0xffffffff)
        mem = malloc(numbytes);
    else
        mem = 0;
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
        FillBitmapArea(gpWindowManager->m_screen, 0, 0, 640, 480, 0);
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, 640, 480, 0, 0);
        SmackSoundOnOff(smk, gConfig.musicVolume);
        if (gbSkipIntro && (bSmackNum == 2 || bSmackNum == 3)) {
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
            gpWindowManager->FadeScreen(1, 8, 0);
        SmackToBuffer(smk, 0, 0, 640, 480, gpWindowManager->m_screen->m_pixels, 0);
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
                if (bSmackNum == 6 && currentFrame >= 23)
                    bigFont->DrawBoundedString(gcCongratsText, 29, 338, 325, 115, 1, 1);
                if (bSmackNum == 0) {
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
            if (!gbFirstTimeThrough || bSmackNum > 3) {
                Process1WindowsMessage();
                switch (gpInputManager->GetEvent().type) {
                case MESSAGE_KEY_DOWN:
                case MESSAGE_LEFT_BUTTON_DOWN:
                case MESSAGE_RIGHT_BUTTON_DOWN:
                    if (bSmackNum >= 2)
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
        if (bSmackNum <= 1) {
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
        if (bSmackNum == 1)
            gbSkipIntro = 1;
        if (bSmackNum == 6) {
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
            FillBitmapArea(gpWindowManager->m_screen, 0, 0, 640, 480, 0);
            BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, 640, 480, 0, 0);
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
