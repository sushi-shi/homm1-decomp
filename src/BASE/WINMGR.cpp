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

#ifdef HOMM1_EDITOR
#define WINMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\WINMGR.CPP"
#else
#define WINMGR_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\WINMGR.CPP"
#endif

void CycleColors(void) {
    i8 savedColor[PALETTE_GRAPHICS_CHANNELS];

    if (gWindowManager == NULL)
        return;
    if (gBufferPalette == NULL)
        return;
    if (gWindowManager->m_active != 1)
        return;
    if (gWindowManager->m_colorCycling == 0)
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
        gBufferPalette->m_data + PALETTE_CYCLE_FIRST * PALETTE_GRAPHICS_CHANNELS,
        gCyclePal,
        PALETTE_CYCLE_BYTES
    );
    UpdatePalette(gBufferPalette->m_data);
}

heroWindowManager::heroWindowManager(void) : baseManager() {
    m_active = 0;
    m_previousFocusWindow = NULL;
    m_focusWindow = NULL;
    m_windowListTail = NULL;
    m_windowListHead = NULL;
    m_unused40 = 0;
    m_unused41 = 0;
    m_screen = NULL;
    m_screenshotIndex = 0;
    m_colorCycling = 0;
    m_fizzleSource = NULL;
    m_fizzleWork = NULL;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    m_dialogResult = WINDOW_MANAGER_NO_DIALOG_RESULT;
}

i16 heroWindowManager::Open(i16 priority) {
    FadeOut(WINDOW_FADE_NORMAL);
    m_screen = new bitmap();
    if (m_screen == NULL)
        MemError();
    m_screen->m_bitmapType = BITMAP_TYPE_MEMORY;
    m_screen->m_width = LOGICAL_SCREEN_WIDTH;
    m_screen->m_height = LOGICAL_SCREEN_HEIGHT;
    m_screen->m_pixels = static_cast<u8*>(gInitWin);
    if (m_screen == NULL) {
        Cleanup();
        return WINDOW_MANAGER_OPEN_FAILURE;
    }
    m_messageMask = BASE_MANAGER_ACCEPT_RIGHT_BUTTON_DOWN;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "heroWindowManager");
    return BASE_MANAGER_SUCCESS;
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
    Cleanup();
    m_screen->m_pixels = NULL;
    if (m_screen != NULL)
        delete m_screen;
    m_active = 0;
}

i16 heroWindowManager::UpdateHoverWindow(i16 x, i16 y) {
    heroWindow* window = m_windowListTail;

    while (window != NULL) {
        if (x >= window->m_posX && y >= window->m_posY && x < window->m_posX + window->m_winWidth
            && y < window->m_posY + window->m_winHeight) {
            if (window != m_focusWindow) {
                m_previousFocusWindow = m_focusWindow;
                m_focusWindow = window;
                return 1;
            }
            return 0;
        }
        window = window->m_prevWindow;
    }
    m_previousFocusWindow = m_focusWindow;
    m_focusWindow = NULL;
    return 1;
}

i16 heroWindowManager::Main(tag_message& message) {
    i16 dispatchResult = MESSAGE_DISPATCH_CONTINUE;
    heroWindow* window = m_windowListTail;
    while (window != NULL) {
        switch (dispatchResult = window->BroadcastMessage(message)) {
            case MESSAGE_DISPATCH_CONTINUE:
                break;
            case MESSAGE_DISPATCH_CONSUME:
            case MESSAGE_DISPATCH_FORWARD:
                return dispatchResult;
        }
        window = window->m_prevWindow;
    }
    return dispatchResult;
}

i16 heroWindowManager::BroadcastMessage(
    i16 type,
    i16 command,
    i16 widgetId,
    i16 value
) {
    tag_message message;
    message.type = type;
    message.command = command;
    message.id = widgetId;
    message.value = value;
    return Main(message);
}

void heroWindowManager::AddWindow(heroWindow* window, i16 zOrder, b8 updateScreen) {
    heroWindow* currentWindow = m_windowListTail;
    if (window->m_winFlags & WINDOW_FLAG_FIXED_LAYER)
        zOrder = WINDOW_Z_ORDER_BOTTOM;
    if (zOrder == WINDOW_Z_ORDER_APPEND) {
        if (currentWindow == NULL)
            zOrder = WINDOW_Z_ORDER_BOTTOM;
        else
            zOrder = currentWindow->m_zOrder + 1;
    }
    if (zOrder == WINDOW_Z_ORDER_BOTTOM && m_windowListHead != NULL)
        return;
    if (zOrder != WINDOW_Z_ORDER_BOTTOM && m_windowListHead == NULL)
        return;
    if (window->Open(zOrder, updateScreen) != WINDOW_OPEN_SUCCESS)
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
    m_previousFocusWindow = m_focusWindow;
    m_focusWindow = window;
}

void heroWindowManager::RemoveWindow(heroWindow* window) {
    if (window == NULL)
        return;
    window->Close();
    if (window == m_windowListHead) {
        m_windowListHead = window->m_nextWindow;
        if (m_windowListHead == NULL)
            m_windowListTail = NULL;
        else
            m_windowListHead->m_prevWindow = NULL;
    } else {
        if (window == m_windowListTail) {
            m_windowListTail = window->m_prevWindow;
            m_windowListTail->m_nextWindow = NULL;
        } else {
            if (window->m_prevWindow != NULL)
                window->m_prevWindow->m_nextWindow = window->m_nextWindow;
            if (window->m_nextWindow != NULL)
                window->m_nextWindow->m_prevWindow = window->m_prevWindow;
        }
    }
    if (m_previousFocusWindow == window)
        m_previousFocusWindow = NULL;
    if (m_previousFocusWindow == NULL) {
        m_focusWindow = m_windowListTail;
        return;
    }
    m_focusWindow = m_previousFocusWindow;
}

i16 heroWindowManager::DoDialog(
    heroWindow* window,
    i16 (*handler)(tag_message&),
    b32 fade
) {
    static i32 gDialogNestCount = 0;
    tag_message message;
    i16 done;
    i32 result;

    gInDialog = true;
    if (gDialogNestCount == 0)
        SetNoDialogMenus(0);
    gDialogNestCount++;
    m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
    if (window != NULL)
        AddWindow(window, WINDOW_Z_ORDER_APPEND, true);
    if (fade)
        gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
    gInputManager->Flush();
    m_dialogResult = WINDOW_MANAGER_NO_DIALOG_RESULT;
    done = 0;
    while (done == 0) {
        PollSound();
        Process1WindowsMessage();
        message = gInputManager->GetEvent();
        gMouseManager->Main(message);
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
        gInputManager->Flush();
    }
    gInDialog = false;
    gDialogNestCount--;
    if (gDialogNestCount == 0)
        SetNoDialogMenus(1);
    return 0;
}

void heroWindowManager::UpdateScreen(void) {
    PollSound();
    BitmapToScreen(m_screen);
    PollSound();
    gMouseManager->ShowColorPointer();
}

void heroWindowManager::UpdateScreenRegion(i16 x, i16 y, i16 width, i16 height) {
    i16 top, left, bottom, right;
    i16 pointerInside;
    i16 mousePosX, mouseY;

    left = x - gMouseManager->m_savedUnderlying->m_width;
    top = y - gMouseManager->m_savedUnderlying->m_height;
    right = x + width;
    bottom = y + height;
    mousePosX = gMouseManager->m_mouseX;
    mouseY = gMouseManager->m_mouseY;
    pointerInside = 0;
    if (gMouseManager->IsVis()) {
        if (mousePosX < left || mousePosX > right)
            pointerInside = 0;
        else if (mouseY < top || mouseY > bottom)
            pointerInside = 0;
        else if (mousePosX >= left && mouseY >= top && mousePosX <= right && mouseY <= bottom)
            pointerInside = 1;
    }
    PollSound();
    if (pointerInside)
        gMouseManager->HideColorPointer();
    BlitBitmapToScreen(m_screen, x, y, width, height, x, y);
    if (pointerInside)
        gMouseManager->ShowColorPointer();
    PollSound();
}

void heroWindowManager::RedrawScreen(void) {
    heroWindow* window = m_windowListHead;

    while (window != NULL) {
        window->DrawWindow();
        window = window->m_nextWindow;
    }
}

void heroWindowManager::FadeScreen(
    i16 direction,
    i16 increment,
    palette* currentPalette
) {
    H1_ASSERT(direction == WINDOW_FADE_IN || direction == WINDOW_FADE_OUT);
    if (currentPalette != NULL)
        SetPalette(currentPalette->m_data, false);
    PollSound();
    switch (direction) {
        case WINDOW_FADE_IN: {
            i8 saved = m_colorCycling;
            m_colorCycling = 0;
            FadeIn(increment);
            m_colorCycling = saved | gFadeSavedColorCycling;
            break;
        }
        case WINDOW_FADE_OUT:
            gFadeSavedColorCycling = m_colorCycling;
            m_colorCycling = 0;
            FadeOut(increment);
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
    gInputManager->Flush();
}

void heroWindowManager::Cleanup(void) {}

void heroWindowManager::SaveFizzleSource(i16 x, i16 y, i16 width, i16 height) {
    if (!gShowIt)
        return;
    if (m_fizzleSource != NULL)
        delete m_fizzleSource;
    m_fizzleSource = new bitmap(BITMAP_TYPE_NONE, width, height);
    BlitBitmap(gWindowManager->m_screen, x, y, width, height, m_fizzleSource, 0, 0);
}

#define FIZZLE_PAIR(values, from, to)                                                              \
    ((values) + (from) * PALETTE_COLOR_COUNT * PALETTE_GRAPHICS_CHANNELS                           \
     + (to) * PALETTE_GRAPHICS_CHANNELS)

void CreateFizzleTables(void) {
    u8(*paletteColors)[PALETTE_GRAPHICS_CHANNELS];
    u8(*table)[PALETTE_COLOR_COUNT];
    float* increment;
    u32 g;
    i32 minDist;
    i32 delta;
    i32 destColor;
    i32 src;
    u8(*rgbCube)[PALETTE_CUBE_LEVELS][PALETTE_CUBE_LEVELS];
    i32 c;
    i32 cycleFrame;
    u32 r;
    u32 b;
    FILE* fp;
    float* blend;

    paletteColors = reinterpret_cast<u8(*)[PALETTE_GRAPHICS_CHANNELS]>(gBufferPalette->m_data);
    rgbCube = static_cast<u8(*)[PALETTE_CUBE_LEVELS][PALETTE_CUBE_LEVELS]>(
        malloc(PALETTE_CUBE_LEVELS * PALETTE_CUBE_LEVELS * PALETTE_CUBE_LEVELS)
    );
    table = static_cast<u8(*)[PALETTE_COLOR_COUNT]>(malloc(FIZZLE_CYCLE_TABLE_BYTES));
    increment = static_cast<float*>(malloc(FIZZLE_COLOR_PAIR_FLOATS * sizeof(float)));
    blend = static_cast<float*>(malloc(FIZZLE_COLOR_PAIR_FLOATS * sizeof(float)));
    memset(rgbCube, 0, PALETTE_CUBE_LEVELS * PALETTE_CUBE_LEVELS * PALETTE_CUBE_LEVELS);
    for (r = 0; r < PALETTE_CUBE_LEVELS; r++) {
        for (g = 0; g < PALETTE_CUBE_LEVELS; g++) {
            for (b = 0; b < PALETTE_CUBE_LEVELS; b++) {
                minDist = PALETTE_NEAREST_DISTANCE_LIMIT;
                for (src = 0; src < PALETTE_COLOR_COUNT; src++) {
                    delta = abs(paletteColors[src][PALETTE_CHANNEL_RED] - r)
                            + abs(paletteColors[src][PALETTE_CHANNEL_GREEN] - g)
                            + abs(paletteColors[src][PALETTE_CHANNEL_BLUE] - b);
                    if (delta < minDist) {
                        minDist = delta;
                        rgbCube[r][g][b] = src;
                    }
                }
            }
        }
    }
    for (src = 0; src < PALETTE_COLOR_COUNT; src++) {
        for (destColor = 0; destColor < PALETTE_COLOR_COUNT; destColor++) {
            for (c = 0; c < PALETTE_GRAPHICS_CHANNELS; c++) {
                FIZZLE_PAIR(increment, src, destColor)
                [c] = (paletteColors[destColor][c] - paletteColors[src][c])
                      / (CYCLE_FRAME_COUNT + 1.0f);
                FIZZLE_PAIR(blend, src, destColor)[c] = paletteColors[src][c];
            }
        }
    }
    for (cycleFrame = 0; cycleFrame < CYCLE_FRAME_COUNT; cycleFrame++) {
        for (src = 0; src < PALETTE_COLOR_COUNT; src++) {
            for (destColor = 0; destColor < PALETTE_COLOR_COUNT; destColor++) {
                for (c = 0; c < PALETTE_GRAPHICS_CHANNELS; c++)
                    FIZZLE_PAIR(blend, src, destColor)
                [c] += FIZZLE_PAIR(increment, src, destColor)[c];
                table[src][destColor] = rgbCube
                    [static_cast<i32>(FIZZLE_PAIR(blend, src, destColor)[PALETTE_CHANNEL_RED])]
                    [static_cast<i32>(FIZZLE_PAIR(blend, src, destColor)[PALETTE_CHANNEL_GREEN])]
                    [static_cast<i32>(FIZZLE_PAIR(blend, src, destColor)[PALETTE_CHANNEL_BLUE])];
            }
        }
        sprintf(gText, "CCYCLE%02d.BIN", cycleFrame);
        fp = fopen(gText, "wb");
        fwrite(table, FIZZLE_CYCLE_TABLE_BYTES, 1, fp);
        fclose(fp);
    }
    free(rgbCube);
    free(table);
    free(increment);
    free(blend);
}

void heroWindowManager::FizzleForward(i16 x, i16 y, i16 width, i16 height, i32 delay) {
    u8* workPixel;
    u8* screenPixel;
    u8* savePixel;
    i32 tickStart;
    i32 frame;
    i32 sourceY;
    i32 sourceX;
    i8* cycleTable;
    i32 savedColorCycling;
    if (!gShowIt)
        return;
    gEnlargeScreenBlit = false;
    tickStart = 0;
    savedColorCycling = gWindowManager->m_colorCycling;
    gWindowManager->m_colorCycling = 0;
    if (delay == FIZZLE_USE_DEFAULT_DELAY)
        delay = FIZZLE_DEFAULT_DELAY;
    m_fizzleWork = new bitmap(BITMAP_TYPE_NONE, width, height);
    cycleTable = static_cast<i8*>(malloc(FIZZLE_CYCLE_TABLE_BYTES));
    BlitBitmap(gWindowManager->m_screen, x, y, width, height, m_fizzleWork, 0, 0);

    for (frame = 0; frame < CYCLE_FRAME_COUNT; frame++) {
        sprintf(gText, "CCYCLE%02d.BIN", frame);
        gResourceManager->PointToFile(gResourceManager->MakeId(gText));
        gResourceManager->ReadBlock(cycleTable, FIZZLE_CYCLE_TABLE_BYTES);
        for (sourceY = y; sourceY < y + height; sourceY++) {
            savePixel = m_fizzleSource->m_pixels + (sourceY - y) * m_fizzleSource->m_width;
            workPixel = m_fizzleWork->m_pixels + (sourceY - y) * width;
            screenPixel = m_screen->m_pixels + sourceY * LOGICAL_SCREEN_WIDTH + x;
            for (sourceX = x; sourceX < x + width; sourceX++) {
                *screenPixel = cycleTable[static_cast<u16>(
                    *workPixel | (*savePixel << FIZZLE_LOOKUP_HIGH_BYTE_SHIFT)
                )];
                savePixel++;
                workPixel++;
                screenPixel++;
            }
        }
        PollSound();
        DelayTilMilli(tickStart + delay);
        tickStart = KBTickCount();
        BlitBitmapToScreen(m_screen, x, y, width, height, x, y);
        PollSound();
    }
    DelayTilMilli(tickStart + delay);
    BlitBitmapToScreen(m_fizzleWork, 0, 0, width, height, x, y);
    gEnlargeScreenBlit = true;
    gWindowManager->m_colorCycling = savedColorCycling;
    delete m_fizzleSource;
    m_fizzleSource = NULL;
    delete m_fizzleWork;
    m_fizzleWork = NULL;
    free(cycleTable);
}

void heroWindowManager::ReleaseFizzleSource(void) {
    if (m_fizzleSource != NULL)
        delete m_fizzleSource;
    m_fizzleSource = NULL;
}

i8 gFadeSavedColorCycling;
i8 gCyclePal[PALETTE_CYCLE_BYTES];
