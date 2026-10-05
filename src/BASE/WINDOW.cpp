// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/backdropWidget.h>
#include <BASE/bitmap.h>
#include <BASE/border.h>
#include <BASE/button.h>
#include <BASE/dimmerWidget.h>
#include <BASE/display.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/iconWidget.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/textEntryWidget.h>
#include <BASE/textWidget.h>
#include <BASE/widget.h>
#include <SOURCE/KB.h>

#include <stdlib.h>
#include <string.h>

DATA(0x004a1360)
char gDefaultConstruct[] = "Default Construct";
DATA(0x004a1374)
char gDynamicConstruct[] = "Dynamic Construct";

H1_ENUM_BEGIN(WindowWidgetRecordType)
    WIDGET_RECORD_END = 0,
    WIDGET_RECORD_BORDER = 1,
    WIDGET_RECORD_BUTTON = 2,
    WIDGET_RECORD_TEXT = 8,
    WIDGET_RECORD_ICON = 0x10,
    WIDGET_RECORD_BACKDROP = 0x20,
    WIDGET_RECORD_DIMMER = 0x40,
    WIDGET_RECORD_TEXT_ENTRY = 0x100,
    WIDGET_RECORD_TEXT_ENTRY_RECT = 0x201,
    WIDGET_RECORD_TEXT_ENTRY_MULTILINE = 0x202
H1_ENUM_END(WindowWidgetRecordType)

// HoMM2 Buka's default heroWindow constructor: a full-screen fixed-layer
// window. No retail caller survives.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046d020, 0x92)
heroWindow::heroWindow(void) {
    strcpy(m_name, gDefaultConstruct);
    m_nextWindow = m_prevWindow = NULL;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_posX = m_posY = 0;
    m_winWidth = LOGICAL_SCREEN_WIDTH;
    m_winHeight = LOGICAL_SCREEN_HEIGHT;
    m_winFlags = WINDOW_FLAG_FIXED_LAYER;
    m_winState = WINDOW_STATE_CLOSED;
    m_widgetListTail = m_widgetListHead = NULL;
    m_savedBackground = NULL;
}

// donor PoL RVA 0x000cec20; preferred Buka symbol ??0heroWindow@@QAE@HHHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651321;margin=0.357646;shape=0.346;size=0.852;calls=1.000;strings=Dynamic Construct;alternate=pol20:void heroWindow::constructor(int, int, int, int, int)@0x000cec20
VA(0x0046d0b2, 0x9e)
heroWindow::heroWindow(i16 x, i16 y, i16 width, i16 height, i16 flags) {
    strcpy(m_name, gDynamicConstruct);
    m_nextWindow = m_prevWindow = NULL;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_posX = x;
    m_posY = y;
    m_winWidth = width;
    m_winHeight = height;
    m_winFlags = H1_ENUM_CAST(WindowFlag, i16, flags);
    m_winState = WINDOW_STATE_CLOSED;
    m_widgetListTail = m_widgetListHead = NULL;
    m_savedBackground = NULL;
}

// donor PoL RVA 0x000cecd0; preferred Buka symbol ??0heroWindow@@QAE@HHPAD@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.375549;margin=0.549564;shape=0.254;size=0.606;calls=0.800;alternate=pol20:void heroWindow::constructor(int, int, char *)@0x000cecd0
VA(0x0046d150, 0x5a0)
heroWindow::heroWindow(i16 x, i16 y, char* resourceName) {
    i16 idx;
    i16 jb;
    dimmerWidget* pDimmer;
    border* pBorder;
    widget* pWidget;
    textEntryWidget* pTextEnt;
    iconWidget* pIcon;
    button* pButton;
    textWidget* pText;
    H1_ENUM_STORAGE(WindowWidgetRecordType, i16) rec;
    backdropWidget* pBack;

    strcpy(m_name, resourceName);
    jb = gpResourceManager->MakeId(resourceName);
    gpResourceManager->PointToFile(jb);
    m_savedBackground = NULL;
    m_nextWindow = m_prevWindow = NULL;
    m_winState = WINDOW_STATE_CLOSED;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_posX = x;
    m_posY = y;
    m_winWidth = gpResourceManager->ReadWord();
    m_winHeight = gpResourceManager->ReadWord();
    m_winFlags = H1_ENUM_CAST(WindowFlag, i16, gpResourceManager->ReadWord());
    m_winFlags |= WINDOW_FLAG_OWNS_WIDGETS;
    m_widgetListTail = m_widgetListHead = NULL;
    idx = 0;
    while (idx == 0) {
        PollSound();
        rec = H1_ENUM_CAST(WindowWidgetRecordType, i16, gpResourceManager->ReadWord());
        pWidget = NULL;
        switch (rec) {
            case WIDGET_RECORD_END:
                idx++;
                break;
            case WIDGET_RECORD_BORDER:
                pBorder = new border();
                pBorder->Read();
                pWidget = pBorder;
                break;
            case WIDGET_RECORD_BUTTON:
                pButton = new button();
                pButton->Read();
                pWidget = pButton;
                break;
            case WIDGET_RECORD_ICON:
                pIcon = new iconWidget();
                pIcon->Read();
                pWidget = pIcon;
                break;
            case WIDGET_RECORD_DIMMER:
                pDimmer = new dimmerWidget();
                pDimmer->Read();
                pWidget = pDimmer;
                break;
            case WIDGET_RECORD_BACKDROP:
                pBack = new backdropWidget();
                pBack->Read();
                pWidget = pBack;
                break;
            case WIDGET_RECORD_TEXT:
                pText = new textWidget();
                pText->Read();
                pWidget = pText;
                break;
            case WIDGET_RECORD_TEXT_ENTRY:
                pTextEnt = new textEntryWidget();
                pTextEnt->Read(TEXT_ENTRY_READ_DEFAULT);
                pWidget = pTextEnt;
                break;
            case WIDGET_RECORD_TEXT_ENTRY_RECT:
                pTextEnt = new textEntryWidget();
                pTextEnt->Read(TEXT_ENTRY_READ_RECT);
                pWidget = pTextEnt;
                break;
            case WIDGET_RECORD_TEXT_ENTRY_MULTILINE:
                pTextEnt = new textEntryWidget();
                pTextEnt->Read(TEXT_ENTRY_READ_MULTILINE);
                pWidget = pTextEnt;
                break;
        }
        if (idx == 0 && pWidget != NULL)
            AddWidget(pWidget, WINDOW_Z_ORDER_APPEND);
    }
}

// donor PoL RVA 0x000cf200; preferred Buka symbol ?Open@heroWindow@@QAEHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.353009;margin=0.367120;shape=0.216;size=0.638;calls=0.500;alternate=pol20:int heroWindow::Open(int, int)@0x000cf200
VA(0x0046d6f0, 0x85)
i16 heroWindow::Open(i16 zOrder, i8 flags) {
    if ((m_winState & WINDOW_STATE_OPEN) != 0)
        return WINDOW_OPEN_FAILURE;
    gpMouseManager->ReallyHidePointer();
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != 0 && SaveBackground() != 0)
        return WINDOW_OPEN_FAILURE;
    m_zOrder = zOrder;
    DrawWindow(flags);
    gpMouseManager->ReallyShowPointer();
    m_winState |= WINDOW_STATE_OPEN;
    return WINDOW_OPEN_SUCCESS;
}

// donor PoL RVA 0x000cf310; preferred Buka symbol ?Close@heroWindow@@QAEXXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.586039;margin=0.311864;shape=0.455;size=0.909;calls=1.000;alternate=pol20:void heroWindow::Close(void)@0x000cf310
VA(0x0046d775, 0xa2)
void heroWindow::Close(void) {
    widget *current, *next;
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != 0 && (m_winState & WINDOW_STATE_OPEN) != 0)
        RestoreBackground();
    current = m_widgetListHead;
    while (current != NULL) {
        next = current->m_next;
        RemoveWidget(current);
        if ((m_winFlags & WINDOW_FLAG_OWNS_WIDGETS) != 0)
            delete current;
        current = next;
    }
    m_winState = WINDOW_STATE_CLOSED;
}

// donor PoL RVA 0x000cf3c0; preferred Buka symbol ?AddWidget@heroWindow@@QAEXPAVwidget@@H@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.553676;margin=0.412288;shape=0.475;size=0.789;calls=1.000;alternate=pol20:void heroWindow::AddWidget(class widget *, int)@0x000cf3c0
VA(0x0046d817, 0x113)
void heroWindow::AddWidget(widget* newWidget, i16 zOrder) {
    widget* currentWidget = m_widgetListHead;
    if (zOrder == WINDOW_Z_ORDER_APPEND) {
        if (currentWidget == NULL)
            zOrder = 0;
        else
            zOrder = currentWidget->m_zOrder + 1;
    }
    if (newWidget->Open(zOrder, this) != 0)
        return;
    while (currentWidget != NULL && currentWidget->m_zOrder > zOrder)
        currentWidget = currentWidget->m_next;
    if (currentWidget == NULL) {
        newWidget->m_prev = m_widgetListTail;
        newWidget->m_next = NULL;
        m_widgetListTail = newWidget;
        if (m_widgetListHead == NULL)
            m_widgetListHead = newWidget;
    } else if (currentWidget->m_prev == NULL) {
        newWidget->m_next = m_widgetListHead;
        newWidget->m_prev = NULL;
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
VA(0x0046d92a, 0xe7)
void heroWindow::RemoveWidget(widget* w) {
    if (w == NULL)
        return;
    w->Close();
    if (w == m_widgetListTail) {
        m_widgetListTail = w->m_prev;
        if (m_widgetListTail == NULL)
            m_widgetListHead = NULL;
        else
            m_widgetListTail->m_next = NULL;
    } else if (w == m_widgetListHead) {
        m_widgetListHead = w->m_next;
        m_widgetListHead->m_prev = NULL;
    } else {
        w->m_next->m_prev = w->m_prev;
        w->m_prev->m_next = w->m_next;
    }
    widget* nextWidget = w->m_next;
    if (nextWidget == NULL) {
        m_widgetListHead = NULL;
        m_widgetListTail = NULL;
    } else {
        nextWidget->m_prev = w->m_prev;
        if (nextWidget->m_prev != NULL)
            nextWidget->m_prev->m_next = nextWidget;
    }
}

// donor PoL RVA 0x000cf620; preferred Buka symbol ?BroadcastMessage@heroWindow@@QAEHAAUtag_message@@@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.435923;margin=0.333583;shape=0.340;size=0.588;calls=1.000;alternate=pol20:int heroWindow::BroadcastMessage(struct tag_message &)@0x000cf620
VA(0x0046da11, 0x61)
i16 heroWindow::BroadcastMessage(tag_message& message) {
    i16 dispatchResult = MESSAGE_DISPATCH_CONTINUE;
    widget* currentWidget = m_widgetListHead;
    while (currentWidget != NULL) {
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

VA(0x0046da72, 0x15)
void heroWindow::DrawWindow(void) {
    DrawWindow(1);
}

// donor PoL RVA 0x000cf6e0; preferred Buka symbol ?DrawWindow@heroWindow@@QAEXH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.418036;margin=0.971201;shape=0.250;size=0.729;calls=1.000;alternate=pol20:void heroWindow::DrawWindow(int)@0x000cf6e0
VA(0x0046da87, 0x24)
void heroWindow::DrawWindow(i16 flags) {
    DrawWindow(flags, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// donor PoL RVA 0x000cf710; preferred Buka symbol ?DrawWindow@heroWindow@@QAEXHHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.624064;margin=0.444596;shape=0.548;size=0.926;calls=1.000;alternate=pol20:void heroWindow::DrawWindow(int, int, int)@0x000cf710
VA(0x0046daab, 0xd3)
void heroWindow::DrawWindow(i16 update, i32 firstId, i32 lastId) {
    tag_message windowWidgetMessage;
    widget* current = m_widgetListTail;
    windowWidgetMessage.type = MESSAGE_WIDGET;
    windowWidgetMessage.command = WIDGET_COMMAND_DRAW;
    while (current != NULL) {
        PollSound();
        if (firstId != WINDOW_ALL_WIDGETS_LOW || lastId != WINDOW_ALL_WIDGETS_HIGH) {
            if (current->m_id >= firstId && current->m_id <= lastId)
                current->Main(windowWidgetMessage);
        } else {
            current->Main(windowWidgetMessage);
        }
        current = current->m_prev;
    }
    PollSound();
    if (update != 0 && (m_winFlags & WINDOW_UPDATE_SUPPRESS_MASK) != WINDOW_FLAG_FIXED_LAYER) {
        gpWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
        PollSound();
    }
}

// donor PoL RVA 0x000cf830; preferred Buka symbol ?SaveBackground@heroWindow@@QAEHXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570741;margin=0.294707;shape=0.467;size=0.852;calls=1.000;alternate=pol20:int heroWindow::SaveBackground(void)@0x000cf830
VA(0x0046db7e, 0xaa)
i16 heroWindow::SaveBackground(void) {
    m_savedBackground = new bitmap(BITMAP_TYPE_MEMORY, m_winWidth, m_winHeight);
    PollSound();
    m_savedBackground->GrabScreen(m_posX, m_posY);
    PollSound();
    return 0;
}

// donor PoL RVA 0x000cf8b0; preferred Buka symbol ?RestoreBackground@heroWindow@@QAEXXZ
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.579338;margin=0.220950;shape=0.412;size=0.993;calls=1.000;alternate=pol20:void heroWindow::RestoreBackground(void)@0x000cf8b0
VA(0x0046dc28, 0x8a)
void heroWindow::RestoreBackground(void) {
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    gpWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
    delete m_savedBackground;
    m_savedBackground = NULL;
}

// donor PoL RVA 0x000cf950; preferred Buka symbol ?MoveWindow@heroWindow@@QAEXHH@Z
// donor Buka TU BASE/WINDOW; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.486346;margin=0.742274;shape=0.333;size=0.777;calls=1.000;alternate=pol20:void heroWindow::MoveWindow(int, int)@0x000cf950
VA(0x0046dcb2, 0x1bc)
void heroWindow::MoveWindow(i16 dx, i16 dy) {
    i16 x = m_posX;
    i16 yPrev = m_posY;
    i16 oldWidth = m_winWidth;
    i16 oldHgt = m_winHeight;
    i16 toX = m_posX + dx;
    i16 toY = m_posY + dy;
    if (toX < 0)
        toX = 0;
    if (toY < 0)
        toY = 0;
    if (toX + m_winWidth > LOGICAL_SCREEN_WIDTH)
        toX = LOGICAL_SCREEN_WIDTH - m_winWidth;
    if (toY + m_winHeight > LOGICAL_SCREEN_HEIGHT)
        toY = LOGICAL_SCREEN_HEIGHT - m_winHeight;
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    m_posX = toX;
    m_posY = toY;
    m_savedBackground->GrabBitmap(gpWindowManager->m_screen, m_posX, m_posY);
    DrawWindow(0);
    oldWidth += abs(m_posX - x);
    oldHgt += abs(m_posY - yPrev);
    if (m_posX < x)
        x = m_posX;
    if (m_posY < yPrev)
        yPrev = m_posY;
    gpWindowManager->UpdateScreenRegion(x, yPrev, oldWidth, oldHgt);
}
