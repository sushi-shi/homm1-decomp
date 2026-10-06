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

// Default heroWindow constructor: a full-screen fixed-layer
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
    jb = gResourceManager->MakeId(resourceName);
    gResourceManager->PointToFile(jb);
    m_savedBackground = NULL;
    m_nextWindow = m_prevWindow = NULL;
    m_winState = WINDOW_STATE_CLOSED;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_posX = x;
    m_posY = y;
    m_winWidth = gResourceManager->ReadWord();
    m_winHeight = gResourceManager->ReadWord();
    m_winFlags = H1_ENUM_CAST(WindowFlag, i16, gResourceManager->ReadWord());
    m_winFlags |= WINDOW_FLAG_OWNS_WIDGETS;
    m_widgetListTail = m_widgetListHead = NULL;
    idx = 0;
    while (idx == 0) {
        PollSound();
        rec = H1_ENUM_CAST(WindowWidgetRecordType, i16, gResourceManager->ReadWord());
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

VA(0x0046d6f0, 0x85)
H1_ENUM_RETURN(WindowOpenStatus, i16) heroWindow::Open(i16 zOrder, i8 flags) {
    if ((m_winState & WINDOW_STATE_OPEN) != WINDOW_STATE_CLOSED)
        return WINDOW_OPEN_FAILURE;
    gMouseManager->ReallyHidePointer();
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != WINDOW_FLAG_NONE && SaveBackground() != 0)
        return WINDOW_OPEN_FAILURE;
    m_zOrder = zOrder;
    DrawWindow(flags);
    gMouseManager->ReallyShowPointer();
    m_winState |= WINDOW_STATE_OPEN;
    return WINDOW_OPEN_SUCCESS;
}

VA(0x0046d775, 0xa2)
void heroWindow::Close(void) {
    widget *current, *next;
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != WINDOW_FLAG_NONE
        && (m_winState & WINDOW_STATE_OPEN) != WINDOW_STATE_CLOSED)
        RestoreBackground();
    current = m_widgetListHead;
    while (current != NULL) {
        next = current->m_next;
        RemoveWidget(current);
        if ((m_winFlags & WINDOW_FLAG_OWNS_WIDGETS) != WINDOW_FLAG_NONE)
            delete current;
        current = next;
    }
    m_winState = WINDOW_STATE_CLOSED;
}

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

VA(0x0046da11, 0x61)
H1_ENUM_RETURN(MessageDispatchResult, i16) heroWindow::BroadcastMessage(tag_message& message) {
    H1_ENUM_LOCAL(MessageDispatchResult, i16) dispatchResult = MESSAGE_DISPATCH_CONTINUE;
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

VA(0x0046da87, 0x24)
void heroWindow::DrawWindow(i16 flags) {
    DrawWindow(flags, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

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
        gWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
        PollSound();
    }
}

VA(0x0046db7e, 0xaa)
i16 heroWindow::SaveBackground(void) {
    m_savedBackground = new bitmap(BITMAP_TYPE_MEMORY, m_winWidth, m_winHeight);
    PollSound();
    m_savedBackground->GrabScreen(m_posX, m_posY);
    PollSound();
    return 0;
}

VA(0x0046dc28, 0x8a)
void heroWindow::RestoreBackground(void) {
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    gWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
    delete m_savedBackground;
    m_savedBackground = NULL;
}

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
    m_savedBackground->GrabBitmap(gWindowManager->m_screen, m_posX, m_posY);
    DrawWindow(0);
    oldWidth += abs(m_posX - x);
    oldHgt += abs(m_posY - yPrev);
    if (m_posX < x)
        x = m_posX;
    if (m_posY < yPrev)
        yPrev = m_posY;
    gWindowManager->UpdateScreenRegion(x, yPrev, oldWidth, oldHgt);
}
