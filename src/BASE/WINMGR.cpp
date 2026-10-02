// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/WINMGR_TYPES.h>
#include <H1/KB.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/wingraph.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma intrinsic(memcpy, strcpy)

// Buka WINMGR correspondence; retail has no force-update argument or later cycle masks.
VA(0x00473de0, 0x1b0)
void CycleColors(void) {
    signed char savedColor[PALETTE_GRAPHICS_CHANNELS];

    if (gpWindowManager == 0)
        return;
    if (gpBufferPalette == 0)
        return;
    if (gpWindowManager->m_active != 1)
        return;
    if (gpWindowManager->m_updateFlags == 0)
        return;

    memcpy(savedColor, gCyclePal + 0, PALETTE_GRAPHICS_CHANNELS);
    memmove(
        gCyclePal + 0,
        gCyclePal + 1 * PALETTE_GRAPHICS_CHANNELS,
        3 * PALETTE_GRAPHICS_CHANNELS
    );
    memcpy(gCyclePal + 3 * PALETTE_GRAPHICS_CHANNELS, savedColor, PALETTE_GRAPHICS_CHANNELS);

    memcpy(savedColor, gCyclePal + 4 * PALETTE_GRAPHICS_CHANNELS, PALETTE_GRAPHICS_CHANNELS);
    memmove(
        gCyclePal + 4 * PALETTE_GRAPHICS_CHANNELS,
        gCyclePal + 5 * PALETTE_GRAPHICS_CHANNELS,
        3 * PALETTE_GRAPHICS_CHANNELS
    );
    memcpy(gCyclePal + 7 * PALETTE_GRAPHICS_CHANNELS, savedColor, PALETTE_GRAPHICS_CHANNELS);

    memcpy(savedColor, gCyclePal + 16 * PALETTE_GRAPHICS_CHANNELS, PALETTE_GRAPHICS_CHANNELS);
    memmove(
        gCyclePal + 16 * PALETTE_GRAPHICS_CHANNELS,
        gCyclePal + 17 * PALETTE_GRAPHICS_CHANNELS,
        4 * PALETTE_GRAPHICS_CHANNELS
    );
    memcpy(gCyclePal + 20 * PALETTE_GRAPHICS_CHANNELS, savedColor, PALETTE_GRAPHICS_CHANNELS);

    memcpy(savedColor, gCyclePal + 21 * PALETTE_GRAPHICS_CHANNELS, PALETTE_GRAPHICS_CHANNELS);
    memcpy(
        gCyclePal + 21 * PALETTE_GRAPHICS_CHANNELS,
        gCyclePal + 22 * PALETTE_GRAPHICS_CHANNELS,
        1 * PALETTE_GRAPHICS_CHANNELS
    );
    memcpy(gCyclePal + 22 * PALETTE_GRAPHICS_CHANNELS, savedColor, PALETTE_GRAPHICS_CHANNELS);

    memcpy(savedColor, gCyclePal + 23 * PALETTE_GRAPHICS_CHANNELS, PALETTE_GRAPHICS_CHANNELS);
    memmove(
        gCyclePal + 23 * PALETTE_GRAPHICS_CHANNELS,
        gCyclePal + 24 * PALETTE_GRAPHICS_CHANNELS,
        3 * PALETTE_GRAPHICS_CHANNELS
    );
    memcpy(gCyclePal + 26 * PALETTE_GRAPHICS_CHANNELS, savedColor, PALETTE_GRAPHICS_CHANNELS);

    memcpy(
        gpBufferPalette->m_data + PALETTE_CYCLE_FIRST * PALETTE_GRAPHICS_CHANNELS,
        gCyclePal,
        PALETTE_CYCLE_BYTES
    );
    UpdatePalette(gpBufferPalette->m_data);
}

// Retail constructor initializes the recovered HoMM1 manager layout.
VA(0x00473f90, 0x46)
heroWindowManager::heroWindowManager(void) : baseManager() {
    m_active = 0;
    m_activeWindow = 0;
    m_focusWindow = 0;
    m_windowListTail = 0;
    m_windowListHead = 0;
    m_unknown40 = 0;
    m_unknown41 = 0;
    m_screenshotIndex = 0;
    m_screen = 0;
    m_updateFlags = 0;
    m_fizzleSource = 0;
    m_fizzleWork = 0;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    m_dialogResult = WINDOW_MANAGER_NO_DIALOG_RESULT;
}

VA(0x00473fe0, 0xba)
short heroWindowManager::Open(short managerOrder) {
    FadeOut(WINDOW_MANAGER_INITIAL_FADE_STEP);
    m_screen = new bitmap();
    if (m_screen == 0)
        MemError();
    m_screen->m_bitmapType = WINDOW_MANAGER_SCREEN_BITMAP_TYPE;
    m_screen->m_width = SCREEN_BLIT_WIDTH;
    m_screen->m_height = SCREEN_BLIT_HEIGHT;
    m_screen->m_pixels = static_cast<signed char*>(lpInitWin);
    if (m_screen != 0) {
        m_priority = managerOrder;
        m_messageMask = BASE_MANAGER_ACCEPT_RIGHT_BUTTON_DOWN;
        m_active = 1;
        strcpy(m_name, "heroWindowManager");
        return 0;
    }
    return 1;
}

VA(0x004740a0, 0x43)
void heroWindowManager::Close(void) {
    if (m_active != 1)
        return;
    heroWindow* window = m_windowListTail;
    while (window != 0) {
        heroWindow* previous = window->m_prevWindow;
        RemoveWindow(window);
        window = previous;
    }
    m_screen->m_pixels = 0;
    if (m_screen != 0)
        delete m_screen;
    m_active = 0;
}

VA(0x004740f0, 0x31)
short heroWindowManager::Main(tag_message& message) {
    short result = MESSAGE_DISPATCH_CONTINUE;
    heroWindow* window = m_windowListTail;
    while (window != 0) {
        switch (result = window->BroadcastMessage(message)) {
            case MESSAGE_DISPATCH_CONTINUE:
                break;
            case MESSAGE_DISPATCH_CONSUME:
            case MESSAGE_DISPATCH_FORWARD:
                return result;
        }
        window = window->m_prevWindow;
    }
    return result;
}

// donor PoL RVA 0x000cac40; preferred Buka symbol ?BroadcastMessage@heroWindowManager@@QAEHHHHH@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.484375;margin=1.382188;shape=0.250;size=0.844;calls=1.000;alternate=pol20:int heroWindowManager::BroadcastMessage(int, int, int, int)@0x000cac40
VA(0x00474130, 0x3c)
short heroWindowManager::BroadcastMessage(short type, short command, short widgetId, short value) {
    tag_message message;
    message.type = type;
    message.command = command;
    message.id = widgetId;
    message.value = value;
    return Main(message);
}

// Buka list insertion correspondence; retail keeps the requested layer as a short.
VA(0x00474170, 0xce)
void heroWindowManager::AddWindow(heroWindow* window, short zOrder, int openFlags) {
    heroWindow* currentWindow = m_windowListTail;
    if (window->m_winFlags & WINDOW_FLAG_FIXED_LAYER)
        zOrder = 0;
    if (zOrder == -1) {
        if (currentWindow == 0)
            zOrder = 0;
        else
            zOrder = currentWindow->m_zOrder + 1;
    }
    if (zOrder == 0 && m_windowListHead != 0)
        return;
    if (zOrder != 0 && m_windowListHead == 0)
        return;
    if (window->Open(zOrder, openFlags) != 0)
        return;
    while (currentWindow != 0 && currentWindow->m_zOrder > zOrder)
        currentWindow = currentWindow->m_prevWindow;
    if (currentWindow == 0) {
        window->m_nextWindow = m_windowListHead;
        window->m_prevWindow = 0;
        m_windowListHead = window;
        if (m_windowListTail == 0)
            m_windowListTail = window;
    } else if (currentWindow->m_nextWindow == 0) {
        window->m_prevWindow = m_windowListTail;
        window->m_nextWindow = 0;
        m_windowListTail->m_nextWindow = window;
        m_windowListTail = window;
    } else {
        window->m_prevWindow = currentWindow;
        window->m_nextWindow = currentWindow->m_nextWindow;
        currentWindow->m_nextWindow->m_prevWindow = window;
        currentWindow->m_nextWindow = window;
    }
    m_activeWindow = m_focusWindow;
    m_focusWindow = window;
}

// donor PoL RVA 0x000cad40; preferred Buka symbol ?RemoveWindow@heroWindowManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.383824;margin=0.385483;shape=0.180;size=0.618;calls=1.000;alternate=pol20:void heroWindowManager::RemoveWindow(class heroWindow *)@0x000cad40
VA(0x00474240, 0x87)
void heroWindowManager::RemoveWindow(heroWindow* window) {
    if (window != 0) {
        window->Close();
        if (m_windowListHead == window) {
            heroWindow* next = window->m_nextWindow;
            m_windowListHead = next;
            if (next == 0)
                m_windowListTail = 0;
            else
                next->m_prevWindow = 0;
        } else {
            if (m_windowListTail == window) {
                heroWindow* previous = window->m_prevWindow;
                m_windowListTail = previous;
                previous->m_nextWindow = 0;
            } else {
                heroWindow* previous = window->m_prevWindow;
                if (previous != 0)
                    previous->m_nextWindow = window->m_nextWindow;
                if (window->m_nextWindow != 0)
                    window->m_nextWindow->m_prevWindow = window->m_prevWindow;
            }
        }
        if (m_activeWindow == window)
            m_activeWindow = 0;
        if (m_activeWindow == 0) {
            m_focusWindow = m_windowListTail;
            return;
        }
        m_focusWindow = m_activeWindow;
    }
}

VA(0x004742d0, 0x1e0)
short heroWindowManager::DoDialog(heroWindow* window, short (*handler)(tag_message&), int fade) {
    tag_message message;
    short done;
    int result;

    gbInDialog = 1;
    if (iDialogNestCount == 0)
        SetNoDialogMenus(0);
    iDialogNestCount++;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    if (window != 0)
        AddWindow(window, -1, 1);
    if (fade != 0)
        gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_MANAGER_DIALOG_FADE_STEP, gPalette);
    gpInputManager->Flush();
    m_dialogResult = WINDOW_MANAGER_NO_DIALOG_RESULT;
    done = 0;
    while (done == 0) {
        PollSound();
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        gpMouseManager->Main(message);
        if (window != 0) {
            result = window->BroadcastMessage(message);
            if (result == MESSAGE_DISPATCH_FORWARD && message.type == MESSAGE_WIDGET
                && message.command == WIDGET_COMMAND_DIALOG_SELECT) {
                m_dialogResult = message.id;
                done = 1;
            }
        }
        result = handler(message);
        if (result == MESSAGE_DISPATCH_FORWARD && message.type == MESSAGE_WIDGET
            && message.command == WIDGET_COMMAND_DIALOG_SELECT)
            done = 1;
    }
    if (done != 0) {
        if (window != 0)
            RemoveWindow(window);
        gpInputManager->Flush();
    }
    gbInDialog = 0;
    iDialogNestCount--;
    if (iDialogNestCount == 0)
        SetNoDialogMenus(1);
    return 0;
}

// HoMM1 hides the software pointer only when it overlaps the updated region.
// Declaring top before left and bottom before right reproduces retail's VC4
// colouring: equal-cost ranges are coloured, and spilled, in declaration order.
VA(0x004744b0, 0xed)
void heroWindowManager::UpdateScreenRegion(short x, short y, short width, short height) {
    short top, left, bottom, right;
    short pointerHidden;
    short mouseX, mouseY;

    left = x - gpMouseManager->m_savedUnderlying->m_width;
    top = y - gpMouseManager->m_savedUnderlying->m_height;
    right = x + width;
    bottom = y + height;
    pointerHidden = 0;
    mouseX = gpMouseManager->m_mouseX;
    mouseY = gpMouseManager->m_mouseY;
    if (gpMouseManager->IsVis()) {
        if (left > mouseX || right < mouseX || top > mouseY || bottom < mouseY)
            pointerHidden = 0;
        else if (left <= mouseX && top <= mouseY && right >= mouseX && bottom >= mouseY)
            pointerHidden = 1;
    }
    PollSound();
    if (pointerHidden)
        gpMouseManager->HideColorPointer();
    BlitBitmapToScreen(m_screen, x, y, width, height, x, y);
    if (pointerHidden)
        gpMouseManager->ShowColorPointer();
    PollSound();
}

// Retail byte saved-update state and word arguments precede the later donor widening.
VA(0x004745a0, 0xbf)
void heroWindowManager::FadeScreen(short direction, short steps, palette* currentPalette) {
    ProcessAssert(
        direction == WINDOW_FADE_IN || direction == WINDOW_FADE_OUT,
        gWindowFadeAssertFile,
        gWindowFadeAssertLine + 1
    );
    if (currentPalette != 0)
        SetPalette(currentPalette->m_data, 0);
    PollSound();
    switch (direction) {
        case WINDOW_FADE_IN: {
            signed char saved = m_updateFlags;
            m_updateFlags = 0;
            FadeIn(steps);
            saved |= gWindowFadeSavedUpdate;
            m_updateFlags = saved;
            break;
        }
        case WINDOW_FADE_OUT:
            gWindowFadeSavedUpdate = m_updateFlags;
            m_updateFlags = 0;
            FadeOut(steps);
            break;
    }
    PollSound();
}

VA(0x00474660, 0x4e)
void heroWindowManager::ScreenShot(void) {
    char filename[16];
    sprintf(filename, "shot%04d.raw", m_screenshotIndex);
    GrabScreenBitmap(m_screen, 0, 0);
    m_screen->Write(filename);
    m_screenshotIndex++;
    gpInputManager->Flush();
}

// Retail omits the later donor coordinate-clamping checks.
VA(0x004746b0, 0x88)
void heroWindowManager::SaveFizzleSource(short x, short y, short width, short height) {
    if (bShowIt == 0)
        return;
    if (m_fizzleSource != 0)
        delete m_fizzleSource;
    m_fizzleSource = new bitmap(0, width, height);
    BlitBitmap(gpWindowManager->m_screen, x, y, width, height, m_fizzleSource, 0, 0);
}

// donor PoL RVA 0x000cb1e0; HoMM1 removes the later palette-fade arguments
// donor Buka TU BASE/WINMGR; five arguments proven by stack use and ret 0x14
// evidence: same cycle-table loop and CCYCLE%02d.BIN resource sequence in both donors
VA(0x00474740, 0x320)
void heroWindowManager::FizzleForward(short x, short y, short width, short height, int delay) {
    unsigned char* workPixel;
    if (bShowIt != 0) {
        gbEnlargeScreenBlit = 0;
        long tickStart = 0;
        int saveFlags = gpWindowManager->m_updateFlags;
        gpWindowManager->m_updateFlags = 0;
        if (delay == -1)
            delay = 150;
        m_fizzleWork = new bitmap(0, width, height);
        signed char* ccycleBuf = static_cast<signed char*>(malloc(0x10000));
        BlitBitmap(gpWindowManager->m_screen, x, y, width, height, m_fizzleWork, 0, 0);

        for (int frame = 0; frame < 8; frame++) {
            sprintf(gText, "CCYCLE%02d.BIN", frame);
            short id = gpResourceManager->MakeId(gText);
            gpResourceManager->PointToFile(id);
            gpResourceManager->ReadBlock(ccycleBuf, 0x10000);
            int sourceY = y;
            if (sourceY < y + height) {
                int screenOffset = y * 640;
                int workOffset = 0;
                do {
                    // Byte access is proven by the retail load/shift sequence.
                    unsigned char* savePixel =
                        reinterpret_cast<unsigned char*>(m_fizzleSource->m_pixels) // byte-evidenced
                        + m_fizzleSource->m_width * (sourceY - y);
                    workPixel =
                        reinterpret_cast<unsigned char*>(m_fizzleWork->m_pixels) // byte-evidenced
                        + workOffset;
                    // Byte access is proven by the retail framebuffer stores.
                    unsigned char* screenPixel =
                        reinterpret_cast<unsigned char*>(m_screen->m_pixels) // byte-evidenced
                        + x + screenOffset;
                    if (x < x + width) {
                        for (int sourceX = x; sourceX < x + width; sourceX++) {
                            unsigned short lookup = *workPixel++ | (*savePixel++ << 8);
                            *screenPixel++ = ccycleBuf[lookup];
                        }
                    }
                    screenOffset += 640;
                    workOffset += width;
                    sourceY++;
                } while (sourceY < y + height);
            }
            PollSound();
            DelayTilMilli(delay + tickStart);
            tickStart = KBTickCount();
            BlitBitmapToScreen(m_screen, x, y, width, height, x, y);
            PollSound();
        }
        DelayTilMilli(delay + tickStart);
        BlitBitmapToScreen(m_fizzleWork, 0, 0, width, height, x, y);
        gbEnlargeScreenBlit = 1;
        gpWindowManager->m_updateFlags = saveFlags;
        if (m_fizzleSource != 0)
            delete m_fizzleSource;
        m_fizzleSource = 0;
        if (m_fizzleWork != 0)
            delete m_fizzleWork;
        m_fizzleWork = 0;
        free(ccycleBuf);
    }
}

// Donor WINMGR ownership; seven trailing padding bytes are excluded.
VA(0x00474a60, 0x19)
void heroWindowManager::ReleaseFizzleSource(void) {
    if (m_fizzleSource != 0)
        delete m_fizzleSource;
    m_fizzleSource = 0;
}
