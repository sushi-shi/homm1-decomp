// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <stdlib.h>

// donor PoL RVA 0x000cac40; preferred Buka symbol ?BroadcastMessage@heroWindowManager@@QAEHHHHH@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.484375;margin=1.382188;shape=0.250;size=0.844;calls=1.000;alternate=pol20:int heroWindowManager::BroadcastMessage(int, int, int, int)@0x000cac40
VA(0x00474130, 0x3c)
short heroWindowManager::BroadcastMessage(short type, short command, short widgetId, short value)
{
    tag_message message;
    message.type = type;
    message.payload.widget.command = command;
    message.payload.widget.id = widgetId;
    message.payload.widget.data.value = value;
    return Main(message);
}

// donor PoL RVA 0x000cad40; preferred Buka symbol ?RemoveWindow@heroWindowManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU BASE/WINMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.383824;margin=0.385483;shape=0.180;size=0.618;calls=1.000;alternate=pol20:void heroWindowManager::RemoveWindow(class heroWindow *)@0x000cad40
VA(0x00474240, 0x87)
void heroWindowManager::RemoveWindow(heroWindow *window)
{
    if (window != 0) {
        window->Close();
        if (m_windowListHead == window) {
            heroWindow *next = window->m_nextWindow;
            m_windowListHead = next;
            if (next == 0)
                m_windowListTail = 0;
            else
                next->m_prevWindow = 0;
        } else {
            if (m_windowListTail == window) {
                heroWindow *previous = window->m_prevWindow;
                m_windowListTail = previous;
                previous->m_nextWindow = 0;
            } else {
                heroWindow *previous = window->m_prevWindow;
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

// donor PoL RVA 0x000cb1e0; HoMM1 removes the later palette-fade arguments
// donor Buka TU BASE/WINMGR; five arguments proven by stack use and ret 0x14
// evidence: same cycle-table loop and CCYCLE%02d.BIN resource sequence in both donors
VA(0x00474740, 0x320)
void heroWindowManager::FizzleForward(
    int x,
    int y,
    int width,
    int height,
    int delay)
{
    if (bShowIt != 0) {
        gbEnlargeScreenBlit = 0;
        int tickStart = 0;
        int saveFlags = gpWindowManager->m_updateFlags;
        gpWindowManager->m_updateFlags = 0;
        if (delay == -1)
            delay = 150;
        m_fizzleWork =
            new bitmap(0, static_cast<short>(width), static_cast<short>(height));
        signed char *ccycleBuf = static_cast<signed char *>(malloc(0x10000));
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
                    unsigned char *savePixel =
                        reinterpret_cast<unsigned char *>(m_fizzleSource->m_pixels) // byte-evidenced
                        + m_fizzleSource->m_width * (sourceY - y);
                    unsigned char *workPixel =
                        reinterpret_cast<unsigned char *>(m_fizzleWork->m_pixels) // byte-evidenced
                        + workOffset;
                    // Byte access is proven by the retail framebuffer stores.
                    unsigned char *screenPixel =
                        reinterpret_cast<unsigned char *>(m_screen->m_pixels) // byte-evidenced
                        + x + screenOffset;
                    if (x < x + width) {
                        int remaining = width;
                        do {
                            unsigned short lookup =
                                *workPixel++ | (*savePixel++ << 8);
                            *screenPixel++ = ccycleBuf[lookup];
                            remaining--;
                        } while (remaining != 0);
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
