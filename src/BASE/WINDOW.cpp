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
    i16 finished;
    i16 resourceFile;
    dimmerWidget* pDimmer;
    border* pBorder;
    widget* pWidget;
    textEntryWidget* pTextEnt;
    iconWidget* pIcon;
    button* pButton;
    textWidget* pText;
    H1_ENUM_STORAGE(WindowWidgetRecordType, i16) recordKind;
    backdropWidget* pBack;

    strcpy(m_name, resourceName);
    resourceFile = gResourceManager->MakeId(resourceName);
    gResourceManager->PointToFile(resourceFile);
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
    finished = 0;
    while (finished == 0) {
        PollSound();
        recordKind = H1_ENUM_CAST(WindowWidgetRecordType, i16, gResourceManager->ReadWord());
        pWidget = NULL;
        switch (recordKind) {
            case WIDGET_RECORD_END:
                finished++;
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
            case WIDGET_RECORD_TEXT_ENTRY_SCROLLING:
                pTextEnt = new textEntryWidget();
                pTextEnt->Read(TEXT_ENTRY_READ_SCROLLING);
                pWidget = pTextEnt;
                break;
        }
        if (finished == 0 && pWidget != NULL)
            AddWidget(pWidget, WINDOW_Z_ORDER_APPEND);
    }
}

VA(0x0046d6f0, 0x85)
H1_ENUM_RETURN(WindowOpenStatus, i16) heroWindow::Open(i16 zOrder, i8 updateScreen) {
    if ((m_winState & WINDOW_STATE_OPEN) != WINDOW_STATE_CLOSED)
        return WINDOW_OPEN_FAILURE;
    gMouseManager->ReallyHidePointer();
    if ((m_winFlags & WINDOW_FLAG_SAVE_BACKGROUND) != WINDOW_FLAG_NONE
        && SaveBackground() != WINDOW_OPEN_SUCCESS)
        return WINDOW_OPEN_FAILURE;
    m_zOrder = zOrder;
    DrawWindow(updateScreen);
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
            zOrder = WINDOW_Z_ORDER_BOTTOM;
        else
            zOrder = currentWidget->m_zOrder + 1;
    }
    if (newWidget->Open(zOrder, this) != WINDOW_OPEN_SUCCESS)
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
void heroWindow::RemoveWidget(widget* removedWidget) {
    if (removedWidget == NULL)
        return;
    removedWidget->Close();
    if (removedWidget == m_widgetListTail) {
        m_widgetListTail = removedWidget->m_prev;
        if (m_widgetListTail == NULL)
            m_widgetListHead = NULL;
        else
            m_widgetListTail->m_next = NULL;
    } else if (removedWidget == m_widgetListHead) {
        m_widgetListHead = removedWidget->m_next;
        m_widgetListHead->m_prev = NULL;
    } else {
        removedWidget->m_next->m_prev = removedWidget->m_prev;
        removedWidget->m_prev->m_next = removedWidget->m_next;
    }
    widget* nextWidget = removedWidget->m_next;
    if (nextWidget == NULL) {
        m_widgetListHead = NULL;
        m_widgetListTail = NULL;
    } else {
        nextWidget->m_prev = removedWidget->m_prev;
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
void heroWindow::DrawWindow(i16 updateScreen) {
    DrawWindow(updateScreen, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x0046daab, 0xd3)
void heroWindow::DrawWindow(i16 updateScreen, i32 firstId, i32 lastId) {
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
    if (updateScreen != 0
        && (m_winFlags & WINDOW_UPDATE_SUPPRESS_MASK) != WINDOW_FLAG_FIXED_LAYER) {
        gWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
        PollSound();
    }
}

VA(0x0046db7e, 0xaa)
H1_ENUM_RETURN(WindowOpenStatus, i16) heroWindow::SaveBackground(void) {
    m_savedBackground = new bitmap(BITMAP_TYPE_MEMORY, m_winWidth, m_winHeight);
    PollSound();
    m_savedBackground->GrabScreen(m_posX, m_posY);
    PollSound();
    return WINDOW_OPEN_SUCCESS;
}

VA(0x0046dc28, 0x8a)
void heroWindow::RestoreBackground(void) {
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    gWindowManager->UpdateScreenRegion(m_posX, m_posY, m_winWidth, m_winHeight);
    delete m_savedBackground;
    m_savedBackground = NULL;
}

#define oldX x           // frame-slot spelling
#define oldY yPrev       // frame-slot spelling
#define oldHeight oldHgt // frame-slot spelling
#define newX toX         // frame-slot spelling
#define newY toY         // frame-slot spelling
VA(0x0046dcb2, 0x1bc)
void heroWindow::MoveWindow(i16 dx, i16 dy) {
    i16 oldX = m_posX;
    i16 oldY = m_posY;
    i16 oldWidth = m_winWidth;
    i16 oldHeight = m_winHeight;
    i16 newX = m_posX + dx;
    i16 newY = m_posY + dy;
    if (newX < 0)
        newX = 0;
    if (newY < 0)
        newY = 0;
    if (newX + m_winWidth > LOGICAL_SCREEN_WIDTH)
        newX = LOGICAL_SCREEN_WIDTH - m_winWidth;
    if (newY + m_winHeight > LOGICAL_SCREEN_HEIGHT)
        newY = LOGICAL_SCREEN_HEIGHT - m_winHeight;
    m_savedBackground->DrawToBuffer(m_posX, m_posY);
    m_posX = newX;
    m_posY = newY;
    m_savedBackground->GrabBitmap(gWindowManager->m_screen, m_posX, m_posY);
    DrawWindow(0);
    oldWidth += abs(m_posX - oldX);
    oldHeight += abs(m_posY - oldY);
    if (m_posX < oldX)
        oldX = m_posX;
    if (m_posY < oldY)
        oldY = m_posY;
    gWindowManager->UpdateScreenRegion(oldX, oldY, oldWidth, oldHeight);
}
#undef oldX
#undef oldY
#undef oldHeight
#undef newX
#undef newY
