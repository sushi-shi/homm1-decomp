#include <H1/Ints.h>

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

char gDynamicConstruct[] = "Dynamic Construct";

enum WindowWidgetRecordType {
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
};

heroWindow::heroWindow(i16 x, i16 y, i16 width, i16 height, i16 flags) {
    strcpy(m_name, gDynamicConstruct);
    m_prevWindow = NULL;
    m_nextWindow = m_prevWindow;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_posX = x;
    m_posY = y;
    m_winWidth = width;
    m_winHeight = height;
    m_winFlags = static_cast<i16>(flags);
    m_winState = WINDOW_STATE_CLOSED;
    m_widgetListHead = NULL;
    m_widgetListTail = m_widgetListHead;
    m_savedBackground = NULL;
}

heroWindow::heroWindow(i16 x, i16 y, char* resourceName) {
    i16 jb;
    i16 i;
    dimmerWidget* pDimmer;
    border* pBorder;
    widget* pWidget;
    iconWidget* pic;
    button* but;
    textWidget* ptw;
    i16 rec;
    textEntryWidget* pEntry;
    backdropWidget* pBack;

    strcpy(m_name, resourceName);
    jb = gpResourceManager->MakeId(resourceName);
    gpResourceManager->PointToFile(jb);
    m_savedBackground = NULL;
    m_prevWindow = NULL;
    m_nextWindow = m_prevWindow;
    m_winState = WINDOW_STATE_CLOSED;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_posX = x;
    m_posY = y;
    m_winWidth = gpResourceManager->ReadWord();
    m_winHeight = gpResourceManager->ReadWord();
    m_winFlags = static_cast<i16>(gpResourceManager->ReadWord());
    m_winFlags = static_cast<i16>(m_winFlags | WINDOW_FLAG_OWNS_WIDGETS);
    m_widgetListHead = NULL;
    m_widgetListTail = m_widgetListHead;
    i = 0;
    while (i == 0) {
        PollSound();
        rec = static_cast<i16>(gpResourceManager->ReadWord());
        pWidget = NULL;
        switch (rec) {
            case WIDGET_RECORD_END:
                i++;
                break;
            case WIDGET_RECORD_BORDER:
                pBorder = new border();
                pBorder->Read();
                pWidget = pBorder;
                break;
            case WIDGET_RECORD_BUTTON:
                but = new button();
                but->Read();
                pWidget = but;
                break;
            case WIDGET_RECORD_ICON:
                pic = new iconWidget();
                pic->Read();
                pWidget = pic;
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
                ptw = new textWidget();
                ptw->Read();
                pWidget = ptw;
                break;
            case WIDGET_RECORD_TEXT_ENTRY:
                pEntry = new textEntryWidget();
                pEntry->Read(TEXT_ENTRY_READ_DEFAULT);
                pWidget = pEntry;
                break;
            case WIDGET_RECORD_TEXT_ENTRY_RECT:
                pEntry = new textEntryWidget();
                pEntry->Read(TEXT_ENTRY_READ_RECT);
                pWidget = pEntry;
                break;
            case WIDGET_RECORD_TEXT_ENTRY_MULTILINE:
                pEntry = new textEntryWidget();
                pEntry->Read(TEXT_ENTRY_READ_MULTILINE);
                pWidget = pEntry;
                break;
        }
        if (i == 0 && pWidget != NULL)
            AddWidget(pWidget, WINDOW_Z_ORDER_APPEND);
    }
}

i16 heroWindow::Open(i16 zOrder, i8 flags) {
    if ((m_winState & WINDOW_STATE_OPEN) != 0)
        return WINDOW_OPEN_FAILURE;
    gpMouseManager->ReallyHidePointer();
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != 0 && SaveBackground() != 0)
        return WINDOW_OPEN_FAILURE;
    m_zOrder = zOrder;
    DrawWindow(flags);
    gpMouseManager->ReallyShowPointer();
    m_winState = static_cast<i16>(m_winState | WINDOW_STATE_OPEN);
    return WINDOW_OPEN_SUCCESS;
}

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
        m_widgetListTail = m_widgetListHead;
    } else {
        nextWidget->m_prev = w->m_prev;
        if (nextWidget->m_prev != NULL)
            nextWidget->m_prev->m_next = nextWidget;
    }
}

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

void heroWindow::DrawWindow(void) {
    DrawWindow(1);
}

void heroWindow::DrawWindow(i16 flags) {
    DrawWindow(flags, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

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

i16 heroWindow::SaveBackground(void) {
    m_savedBackground = new bitmap(BITMAP_TYPE_MEMORY, m_winWidth, m_winHeight);
    PollSound();
    m_savedBackground->GrabScreen(m_posX, m_posY);
    PollSound();
    return 0;
}

void heroWindow::RestoreBackground(void) {
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    gpWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
    delete m_savedBackground;
    m_savedBackground = NULL;
}

void heroWindow::MoveWindow(i16 dx, i16 dy) {
    i16 x = m_posX;
    i16 y = m_posY;
    i16 initialWidth = m_winWidth;
    i16 startHeight = m_winHeight;
    i16 targetX = m_posX + dx;
    i16 targetY = m_posY + dy;
    if (targetX < 0)
        targetX = 0;
    if (targetY < 0)
        targetY = 0;
    if (targetX + m_winWidth > LOGICAL_SCREEN_WIDTH)
        targetX = LOGICAL_SCREEN_WIDTH - m_winWidth;
    if (targetY + m_winHeight > LOGICAL_SCREEN_HEIGHT)
        targetY = LOGICAL_SCREEN_HEIGHT - m_winHeight;
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    m_posX = targetX;
    m_posY = targetY;
    m_savedBackground->GrabBitmap(gpWindowManager->m_screen, m_posX, m_posY);
    DrawWindow(0);
    initialWidth += abs(m_posX - x);
    startHeight += abs(m_posY - y);
    if (m_posX < x)
        x = m_posX;
    if (m_posY < y)
        y = m_posY;
    gpWindowManager->UpdateScreenRegion(x, y, initialWidth, startHeight);
}
