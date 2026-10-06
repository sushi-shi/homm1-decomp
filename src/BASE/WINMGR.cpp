#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/display.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum WindowFizzleConstant {
    CYCLE_FRAME_COUNT = 8,
    FIZZLE_DEFAULT_DELAY = 150,
    FIZZLE_CYCLE_TABLE_BYTES = 0x10000,
    FIZZLE_LOOKUP_HIGH_BYTE_SHIFT = 8,
    SCREENSHOT_FILENAME_CAPACITY = 16
};

void CycleColors(void) {
    i8 savedColor[PALETTE_GRAPHICS_CHANNELS];

    if (gpWindowManager == NULL)
        return;
    if (gpBufferPalette == NULL)
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

heroWindowManager::heroWindowManager(void) : baseManager() {
    m_active = 0;
    m_activeWindow = NULL;
    m_focusWindow = NULL;
    m_windowListTail = NULL;
    m_windowListHead = NULL;
    m_unknown40 = 0;
    m_unknown41 = 0;
    m_screenshotIndex = 0;
    m_screen = NULL;
    m_updateFlags = 0;
    m_fizzleSource = NULL;
    m_fizzleWork = NULL;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    m_dialogResult = WINDOW_MANAGER_NO_DIALOG_RESULT;
}

i16 heroWindowManager::Open(i16 managerOrder) {
    FadeOut(WINDOW_FADE_STEPS_NORMAL);
    m_screen = new bitmap();
    if (m_screen == NULL)
        MemError();
    m_screen->m_bitmapType = BITMAP_TYPE_MEMORY;
    m_screen->m_width = SCREEN_BLIT_WIDTH;
    m_screen->m_height = SCREEN_BLIT_HEIGHT;
    m_screen->m_pixels = static_cast<i8*>(gInitWin);
    if (m_screen != NULL) {
        m_priority = managerOrder;
        m_messageMask = BASE_MANAGER_ACCEPT_RIGHT_BUTTON_DOWN;
        m_active = 1;
        strcpy(m_name, "heroWindowManager");
        return BASE_MANAGER_SUCCESS;
    }
    return WINDOW_MANAGER_OPEN_FAILURE;
}

void heroWindowManager::Close(void) {
    if (m_active != 1)
        return;
    heroWindow* window = m_windowListTail;
    while (window != NULL) {
        heroWindow* previous = window->m_prevWindow;
        RemoveWindow(window);
        window = previous;
    }
    m_screen->m_pixels = NULL;
    if (m_screen != NULL)
        delete m_screen;
    m_active = 0;
}

i16 heroWindowManager::Main(tag_message& message) {
    i16 result = MESSAGE_DISPATCH_CONTINUE;
    heroWindow* window = m_windowListTail;
    while (window != NULL) {
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

i16 heroWindowManager::BroadcastMessage(i16 type, i16 command, i16 widgetId, i16 value) {
    tag_message message;
    message.type = type;
    message.command = command;
    message.id = widgetId;
    message.value = value;
    return Main(message);
}

void heroWindowManager::AddWindow(heroWindow* window, i16 zOrder, i8 openFlags) {
    heroWindow* currentWindow = m_windowListTail;
    if (window->m_winFlags & WINDOW_FLAG_FIXED_LAYER)
        zOrder = 0;
    if (zOrder == WINDOW_Z_ORDER_APPEND) {
        if (currentWindow == NULL)
            zOrder = 0;
        else
            zOrder = currentWindow->m_zOrder + 1;
    }
    if (zOrder == 0 && m_windowListHead != NULL)
        return;
    if (zOrder != 0 && m_windowListHead == NULL)
        return;
    if (window->Open(zOrder, openFlags) != WINDOW_OPEN_SUCCESS)
        return;
    while (currentWindow != NULL && currentWindow->m_zOrder > zOrder)
        currentWindow = currentWindow->m_prevWindow;
    if (currentWindow == NULL) {
        window->m_nextWindow = m_windowListHead;
        window->m_prevWindow = NULL;
        m_windowListHead = window;
        if (m_windowListTail == NULL)
            m_windowListTail = window;
    } else if (currentWindow->m_nextWindow == NULL) {
        window->m_prevWindow = m_windowListTail;
        window->m_nextWindow = NULL;
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

void heroWindowManager::RemoveWindow(heroWindow* window) {
    if (window != NULL) {
        window->Close();
        if (m_windowListHead == window) {
            heroWindow* next = window->m_nextWindow;
            m_windowListHead = next;
            if (next == NULL)
                m_windowListTail = NULL;
            else
                next->m_prevWindow = NULL;
        } else {
            if (m_windowListTail == window) {
                heroWindow* previous = window->m_prevWindow;
                m_windowListTail = previous;
                previous->m_nextWindow = NULL;
            } else {
                heroWindow* previous = window->m_prevWindow;
                if (previous != NULL)
                    previous->m_nextWindow = window->m_nextWindow;
                if (window->m_nextWindow != NULL)
                    window->m_nextWindow->m_prevWindow = window->m_prevWindow;
            }
        }
        if (m_activeWindow == window)
            m_activeWindow = NULL;
        if (m_activeWindow == NULL) {
            m_focusWindow = m_windowListTail;
            return;
        }
        m_focusWindow = m_activeWindow;
    }
}

i16 heroWindowManager::DoDialog(heroWindow* window, i16 (*handler)(tag_message&), i32 fade) {
    static i32 gDialogNestCount = 0;
    tag_message message;
    i16 done;
    i32 result;

    gInDialog = 1;
    if (gDialogNestCount == 0)
        SetNoDialogMenus(0);
    gDialogNestCount++;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    if (window != NULL)
        AddWindow(window, WINDOW_Z_ORDER_APPEND, 1);
    if (fade != 0)
        gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
    gpInputManager->Flush();
    m_dialogResult = WINDOW_MANAGER_NO_DIALOG_RESULT;
    done = 0;
    while (done == 0) {
        PollSound();
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        gpMouseManager->Main(message);
        if (window != NULL) {
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
        if (window != NULL)
            RemoveWindow(window);
        gpInputManager->Flush();
    }
    gInDialog = 0;
    gDialogNestCount--;
    if (gDialogNestCount == 0)
        SetNoDialogMenus(1);
    return 0;
}

void heroWindowManager::UpdateScreenRegion(i16 x, i16 y, i16 width, i16 height) {
    i16 top, left, bottom, right;
    i16 pointerHidden;
    i16 mouseX, mouseY;

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

void heroWindowManager::FadeScreen(i16 direction, i16 steps, palette* currentPalette) {
    H1_ASSERT(direction == WINDOW_FADE_IN || direction == WINDOW_FADE_OUT);
    if (currentPalette != NULL)
        SetPalette(currentPalette->m_data, 0);
    PollSound();
    switch (direction) {
        case WINDOW_FADE_IN: {
            i8 saved = m_updateFlags;
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

void heroWindowManager::ScreenShot(void) {
    char filename[SCREENSHOT_FILENAME_CAPACITY];
    sprintf(filename, "shot%04d.raw", m_screenshotIndex);
    GrabScreenBitmap(m_screen, 0, 0);
    m_screen->Write(filename);
    m_screenshotIndex++;
    gpInputManager->Flush();
}

void heroWindowManager::SaveFizzleSource(i16 x, i16 y, i16 width, i16 height) {
    if (bShowIt == 0)
        return;
    if (m_fizzleSource != NULL)
        delete m_fizzleSource;
    m_fizzleSource = new bitmap(BITMAP_TYPE_NONE, width, height);
    BlitBitmap(gpWindowManager->m_screen, x, y, width, height, m_fizzleSource, 0, 0);
}

void heroWindowManager::FizzleForward(i16 x, i16 y, i16 width, i16 height, i32 delay) {
    i32 sourceX;
    i32 sourceY;
    i32 tickStart;
    u8* screenPixel;
    u8* workPixel;
    i32 frame;
    i32 saveFlags;
    u8* savePixel;
    i8* ccycleBuf;
    if (bShowIt != 0) {
        gEnlargeScreenBlit = 0;
        tickStart = 0;
        saveFlags = gpWindowManager->m_updateFlags;
        gpWindowManager->m_updateFlags = 0;
        if (delay == FIZZLE_USE_DEFAULT_DELAY)
            delay = FIZZLE_DEFAULT_DELAY;
        m_fizzleWork = new bitmap(BITMAP_TYPE_NONE, width, height);
        ccycleBuf = static_cast<i8*>(malloc(FIZZLE_CYCLE_TABLE_BYTES));
        BlitBitmap(gpWindowManager->m_screen, x, y, width, height, m_fizzleWork, 0, 0);

        for (frame = 0; frame < CYCLE_FRAME_COUNT; frame++) {
            sprintf(gText, "CCYCLE%02d.BIN", frame);
            gpResourceManager->PointToFile(gpResourceManager->MakeId(gText));
            gpResourceManager->ReadBlock(ccycleBuf, FIZZLE_CYCLE_TABLE_BYTES);
            for (sourceY = y; sourceY < y + height; sourceY++) {
                savePixel = reinterpret_cast<u8*>(m_fizzleSource->m_pixels)
                            + m_fizzleSource->m_width * (sourceY - y);
                workPixel = reinterpret_cast<u8*>(m_fizzleWork->m_pixels)
                            + (sourceY - y) * width;
                screenPixel = reinterpret_cast<u8*>(m_screen->m_pixels)
                              + sourceY * LOGICAL_SCREEN_WIDTH + x;
                for (sourceX = x; sourceX < x + width; sourceX++) {
                    u16 lookup = *workPixel++ | (*savePixel++ << FIZZLE_LOOKUP_HIGH_BYTE_SHIFT);
                    *screenPixel++ = ccycleBuf[lookup];
                }
            }
            PollSound();
            DelayTilMilli(delay + tickStart);
            tickStart = KBTickCount();
            BlitBitmapToScreen(m_screen, x, y, width, height, x, y);
            PollSound();
        }
        DelayTilMilli(delay + tickStart);
        BlitBitmapToScreen(m_fizzleWork, 0, 0, width, height, x, y);
        gEnlargeScreenBlit = 1;
        gpWindowManager->m_updateFlags = saveFlags;
        if (m_fizzleSource != NULL)
            delete m_fizzleSource;
        m_fizzleSource = NULL;
        if (m_fizzleWork != NULL)
            delete m_fizzleWork;
        m_fizzleWork = NULL;
        free(ccycleBuf);
    }
}

void heroWindowManager::ReleaseFizzleSource(void) {
    if (m_fizzleSource != NULL)
        delete m_fizzleSource;
    m_fizzleSource = NULL;
}

i8 gWindowFadeSavedUpdate;
i8 gCyclePal[PALETTE_CYCLE_BYTES];
