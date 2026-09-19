// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/widget.h>
#include <H1/KB.h>

#include <stdlib.h>
#include <string.h>

extern heroWindowManager *gpWindowManager;
extern mouseManager *gpMouseManager;

char gDynamicConstruct[] = "Dynamic Construct";

// donor PoL RVA 0x000cec20; preferred Buka symbol ??0heroWindow@@QAE@HHHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651321;margin=0.357646;shape=0.346;size=0.852;calls=1.000;strings=Dynamic Construct;alternate=pol20:void heroWindow::constructor(int, int, int, int, int)@0x000cec20
VA(0x00474a80, 0xad)
heroWindow::heroWindow(
    short x, short y, short width, short height, short flags)
{
    strcpy(m_name, gDynamicConstruct);
    m_prevWindow = 0;
    m_nextWindow = m_prevWindow;
    m_zOrder = -1;
    m_posX = x;
    m_posY = y;
    m_winWidth = width;
    m_winHeight = height;
    m_winFlags = H1_ENUM_CAST(WindowFlag, short, flags);
    m_winState = WINDOW_STATE_CLOSED;
    m_widgetListHead = 0;
    m_widgetListTail = m_widgetListHead;
    m_savedBackground = 0;
}

// donor PoL RVA 0x000cecd0; preferred Buka symbol ??0heroWindow@@QAE@HHPAD@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.375549;margin=0.549564;shape=0.254;size=0.606;calls=0.800;alternate=pol20:void heroWindow::constructor(int, int, char *)@0x000cecd0
VA(0x00474b30, 0x450)
heroWindow::heroWindow(int, int, char *) {}

// donor PoL RVA 0x000cf200; preferred Buka symbol ?Open@heroWindow@@QAEHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.353009;margin=0.367120;shape=0.216;size=0.638;calls=0.500;alternate=pol20:int heroWindow::Open(int, int)@0x000cf200
VA(0x00474f80, 0x9d)
short heroWindow::Open(short zOrder, signed char flags)
{
    if ((m_winState & WINDOW_STATE_OPEN) != 0)
        return 3;
    gpMouseManager->ReallyHidePointer();
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != 0
        && SaveBackground() != 0)
        return 3;
    m_zOrder = zOrder;
    DrawWindow(flags);
    gpMouseManager->ReallyShowPointer();
    m_winState = H1_ENUM_CAST(WindowState, short, m_winState | WINDOW_STATE_OPEN);
    return 0;
}

// donor PoL RVA 0x000cf310; preferred Buka symbol ?Close@heroWindow@@QAEXXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.586039;margin=0.311864;shape=0.455;size=0.909;calls=1.000;alternate=pol20:void heroWindow::Close(void)@0x000cf310
VA(0x00475020, 0xb0)
void heroWindow::Close(void)
{
    widget *w, *next;
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != 0
        && (m_winState & WINDOW_STATE_OPEN) != 0)
        RestoreBackground();
    w = m_widgetListHead;
    while (w != 0) {
        next = w->m_next;
        RemoveWidget(w);
        if ((m_winFlags & WINDOW_FLAG_OWNS_WIDGETS) != 0)
            delete w;
        w = next;
    }
    m_winState = WINDOW_STATE_CLOSED;
}

// donor PoL RVA 0x000cf3c0; preferred Buka symbol ?AddWidget@heroWindow@@QAEXPAVwidget@@H@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.553676;margin=0.412288;shape=0.475;size=0.789;calls=1.000;alternate=pol20:void heroWindow::AddWidget(class widget *, int)@0x000cf3c0
VA(0x004750d0, 0x145)
void heroWindow::AddWidget(widget *newWidget, short zOrder)
{
    widget *currentWidget = m_widgetListHead;
    if (zOrder == -1) {
        if (currentWidget == 0)
            zOrder = 0;
        else
            zOrder = currentWidget->m_zOrder + 1;
    }
    if (newWidget->Open(zOrder, this) != 0)
        return;
    while (currentWidget != 0 && currentWidget->m_zOrder > zOrder)
        currentWidget = currentWidget->m_next;
    if (currentWidget == 0) {
        newWidget->m_prev = m_widgetListTail;
        newWidget->m_next = 0;
        m_widgetListTail = newWidget;
        if (m_widgetListHead == 0)
            m_widgetListHead = newWidget;
    } else if (currentWidget->m_prev == 0) {
        newWidget->m_next = m_widgetListHead;
        newWidget->m_prev = 0;
        m_widgetListHead->m_prev = newWidget;
        m_widgetListHead = newWidget;
    } else {
        newWidget->m_next = currentWidget;
        newWidget->m_prev = currentWidget->m_prev;
        currentWidget->m_prev->m_next = newWidget;
        currentWidget->m_prev = newWidget;
    }
}

// donor PoL RVA 0x000cf500; preferred Buka symbol ?RemoveWidget@heroWindow@@QAEXPAVwidget@@@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.565639;margin=0.542930;shape=0.500;size=0.802;calls=1.000;alternate=pol20:void heroWindow::RemoveWidget(class widget *)@0x000cf500
VA(0x00475220, 0x116)
void heroWindow::RemoveWidget(widget *w)
{
    if (w == 0)
        return;
    w->Close();
    if (w == m_widgetListTail) {
        m_widgetListTail = w->m_prev;
        if (m_widgetListTail == 0)
            m_widgetListHead = 0;
        else
            m_widgetListTail->m_next = 0;
    } else if (w == m_widgetListHead) {
        m_widgetListHead = w->m_next;
        m_widgetListHead->m_prev = 0;
    } else {
        w->m_next->m_prev = w->m_prev;
        w->m_prev->m_next = w->m_next;
    }
    widget *nextWidget = w->m_next;
    if (nextWidget == 0) {
        m_widgetListHead = 0;
        m_widgetListTail = m_widgetListHead;
    } else {
        nextWidget->m_prev = w->m_prev;
        if (nextWidget->m_prev != 0)
            nextWidget->m_prev->m_next = nextWidget;
    }
}

// donor PoL RVA 0x000cf620; preferred Buka symbol ?BroadcastMessage@heroWindow@@QAEHAAUtag_message@@@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.435923;margin=0.333583;shape=0.340;size=0.588;calls=1.000;alternate=pol20:int heroWindow::BroadcastMessage(struct tag_message &)@0x000cf620
VA(0x00475340, 0x98)
short heroWindow::BroadcastMessage(tag_message &message)
{
    short dispatchResult = MESSAGE_DISPATCH_CONTINUE;
    widget *currentWidget = m_widgetListHead;
    while (currentWidget != 0) {
        switch (dispatchResult = currentWidget->Main(message)) {
        case MESSAGE_DISPATCH_CONTINUE:
            break;
        case MESSAGE_DISPATCH_CONSUME:
        case MESSAGE_DISPATCH_FORWARD:
            return dispatchResult;
        }
        currentWidget = currentWidget->m_next;
    }
    return dispatchResult;
}

VA(0x004753e0, 0x20)
void heroWindow::DrawWindow(void)
{
    DrawWindow(1);
}

// donor PoL RVA 0x000cf6e0; preferred Buka symbol ?DrawWindow@heroWindow@@QAEXH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.418036;margin=0.971201;shape=0.250;size=0.729;calls=1.000;alternate=pol20:void heroWindow::DrawWindow(int)@0x000cf6e0
VA(0x00475400, 0x2e)
void heroWindow::DrawWindow(short flags)
{
    DrawWindow(flags, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// donor PoL RVA 0x000cf710; preferred Buka symbol ?DrawWindow@heroWindow@@QAEXHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.624064;margin=0.444596;shape=0.548;size=0.926;calls=1.000;alternate=pol20:void heroWindow::DrawWindow(int, int, int)@0x000cf710
VA(0x00475430, 0xfd)
void heroWindow::DrawWindow(short update, int firstId, int lastId)
{
    tag_message windowWidgetMessage;
    widget *current = m_widgetListTail;
    windowWidgetMessage.type = MESSAGE_WIDGET;
    windowWidgetMessage.payload.widget.command = WIDGET_COMMAND_DRAW;
    while (current != 0) {
        PollSound();
        if (firstId != WINDOW_ALL_WIDGETS_LOW
            || lastId != WINDOW_ALL_WIDGETS_HIGH) {
            if (current->m_id >= firstId
                && current->m_id <= lastId)
                current->Main(windowWidgetMessage);
        } else {
            current->Main(windowWidgetMessage);
        }
        current = current->m_prev;
    }
    PollSound();
    if (update != 0
        && (m_winFlags & WINDOW_UPDATE_SUPPRESS_MASK)
            != WINDOW_FLAG_FIXED_LAYER) {
        gpWindowManager->UpdateScreenRegion(
            m_posX, m_posY, m_winWidth, m_winHeight);
        PollSound();
    }
}

// donor PoL RVA 0x000cf830; preferred Buka symbol ?SaveBackground@heroWindow@@QAEHXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570741;margin=0.294707;shape=0.467;size=0.852;calls=1.000;alternate=pol20:int heroWindow::SaveBackground(void)@0x000cf830
VA(0x00475530, 0x84)
short heroWindow::SaveBackground(void)
{
    m_savedBackground = new bitmap(33, m_winWidth, m_winHeight);
    PollSound();
    m_savedBackground->GrabScreen(m_posX, m_posY);
    PollSound();
    return 0;
}

// donor PoL RVA 0x000cf8b0; preferred Buka symbol ?RestoreBackground@heroWindow@@QAEXXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.579338;margin=0.220950;shape=0.412;size=0.993;calls=1.000;alternate=pol20:void heroWindow::RestoreBackground(void)@0x000cf8b0
VA(0x004755c0, 0x90)
void heroWindow::RestoreBackground(void)
{
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    gpWindowManager->UpdateScreenRegion(
        m_posX, m_posY, m_winWidth, m_winHeight);
    delete m_savedBackground;
    m_savedBackground = 0;
}

// donor PoL RVA 0x000cf950; preferred Buka symbol ?MoveWindow@heroWindow@@QAEXHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.486346;margin=0.742274;shape=0.333;size=0.777;calls=1.000;alternate=pol20:void heroWindow::MoveWindow(int, int)@0x000cf950
VA(0x00475650, 0x1d4)
void heroWindow::MoveWindow(short dx, short dy)
{
    short x = m_posX;
    short yPrev = m_posY;
    short oldWidth = m_winWidth;
    short oldHgt = m_winHeight;
    short toX = m_posX + dx;
    short toY = m_posY + dy;
    if (toX < 0)
        toX = 0;
    if (toY < 0)
        toY = 0;
    if (toX + m_winWidth > 640)
        toX = 640 - m_winWidth;
    if (toY + m_winHeight > 480)
        toY = 480 - m_winHeight;
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    m_posX = toX;
    m_posY = toY;
    m_savedBackground->GrabBitmap(
        gpWindowManager->m_screen, m_posX, m_posY);
    DrawWindow(0);
    oldWidth += abs(m_posX - x);
    oldHgt += abs(m_posY - yPrev);
    if (m_posX < x)
        x = m_posX;
    if (m_posY < yPrev)
        yPrev = m_posY;
    gpWindowManager->UpdateScreenRegion(
        x, yPrev, oldWidth, oldHgt);
}
