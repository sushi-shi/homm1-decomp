// The eraser tool (clearManager). The unit name CLEARMGR is descriptive: no
// retail assertion names it; Open stores the class name "clearManager".

#include <match.h>

#include <BASE/button.h>
#include <BASE/heroWindow.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <EDITOR/clearManager.h>
#include <EDITOR/editManager.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/eventsManager.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

#include <stdlib.h>
#include <string.h>

VA(0x00401000, 0x44)
clearManager::clearManager(void) {
    m_lastY = CLEAR_MANAGER_NO_CELL;
    m_lastX = CLEAR_MANAGER_NO_CELL;
    m_dispatchMask = CLEAR_MANAGER_DISPATCH_MASK;
    m_panel = NULL;
}

VA(0x00401044, 0x188)
H1_ENUM_RETURN(BaseManagerStatus, i16) clearManager::Open(i16 priority) {
    m_panel = new iconWidget(
        CLEAR_PANEL_X,
        CLEAR_PANEL_Y,
        CLEAR_PANEL_WIDTH,
        CLEAR_PANEL_HEIGHT,
        "buttons.icn",
        CLEAR_PANEL_FRAME,
        ICON_DRAW_NORMAL,
        WIDGET_ID_NONE,
        ICON_WIDGET_DRAW,
        1
    );
    gEditManager->m_window->AddWidget(m_panel, -1);
    m_optionsButton = new button(
        CLEAR_OPTIONS_BUTTON_X,
        CLEAR_OPTIONS_BUTTON_Y,
        CLEAR_OPTIONS_BUTTON_WIDTH,
        CLEAR_OPTIONS_BUTTON_HEIGHT,
        "buttons.icn",
        CLEAR_OPTIONS_BUTTON_FRAME,
        CLEAR_OPTIONS_BUTTON_PRESSED_FRAME,
        0,
        BUTTON_NO_HOTKEY,
        EDIT_CONTROL_TOOL_OPTIONS,
        WIDGET_KIND_DEFAULT
    );
    gEditManager->m_window->AddWidget(m_optionsButton, -1);
    gEditManager->m_window->DrawWindow();
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "clearManager");
    return BASE_MANAGER_SUCCESS;
}

VA(0x004011cc, 0xc5)
void clearManager::Close(void) {
    gEditManager->m_window->RemoveWidget(m_optionsButton);
    delete m_optionsButton;
    if (m_panel) {
        gEditManager->m_window->RemoveWidget(m_panel);
        delete m_panel;
        m_panel = NULL;
    }
    gEditManager->m_window->DrawWindow();
    m_active = 0;
}

VA(0x00401291, 0x501)
H1_ENUM_RETURN(MessageDispatchResult, i16) clearManager::Main(tag_message& message) {
    i16 newX;
    i16 newY;
    i16 anchorX;
    i16 anchorY;
    i16 x;
    i16 y;
    i32 unusedMask;
    H1_ENUM_LOCAL(ClearDragMode, i16) dragMode;
    tag_message event;

    if (!(message.type & m_dispatchMask))
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                        break;
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            if (message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS)
                                dragMode = CLEAR_DRAG_CELLS;
                            else
                                dragMode = CLEAR_DRAG_RECTANGLE;
                            gMouseManager->MouseCoords(anchorX, anchorY);
                            gEditManager->ScreenToCell(anchorX, anchorY);
                            gSelectionX = -1;
                            anchorX += gEditManager->m_viewX;
                            anchorY += gEditManager->m_viewY;
                            gEditManager->SaveUndo();
                            event = gInputManager->GetEvent();
                            while (event.type != MESSAGE_LEFT_BUTTON_UP
                                   && event.type != MESSAGE_RIGHT_BUTTON_UP) {
                                Process1WindowsMessage();
                                gMouseManager->Main(event);
                                if (event.type == MESSAGE_MOUSE_MOVE) {
                                    gMouseManager->MouseCoords(x, y);
                                    gEditManager->ScreenToCell(x, y);
                                    x += gEditManager->m_viewX;
                                    y += gEditManager->m_viewY;
                                    if (x != m_lastX || y != m_lastY) {
                                        m_lastX = x;
                                        m_lastY = y;
                                        switch (dragMode) {
                                            case CLEAR_DRAG_CELLS:
                                                gEditManager
                                                    ->ClearArea(x, y, 1, 1, gClearFlags, false);
                                                break;
                                            case CLEAR_DRAG_RECTANGLE:
                                                gSelectionX = x < anchorX ? x : anchorX;
                                                gSelectionY = y < anchorY ? y : anchorY;
                                                gSelectionWidth = abs(x - anchorX) + 1;
                                                gSelectionHeight = abs(y - anchorY) + 1;
                                                gEditManager->DrawMap();
                                                gEditManager->UpdateMapView();
                                                gEditManager->DrawRadar(1);
                                                break;
                                        }
                                    }
                                }
                                event = gInputManager->GetEvent();
                            }
                            if (dragMode == CLEAR_DRAG_RECTANGLE) {
                                if (gSelectionX < 0)
                                    gEditManager
                                        ->ClearArea(anchorX, anchorY, 1, 1, gClearFlags, false);
                                else
                                    gEditManager->ClearArea(
                                        gSelectionX,
                                        gSelectionY,
                                        gSelectionWidth,
                                        gSelectionHeight,
                                        gClearFlags,
                                        false
                                    );
                            }
                            gSelectionX = gSelectionY = -1;
                            gEditManager->DrawMap();
                            gEditManager->UpdateMapView();
                            gEditManager->DrawRadar(1);
                            m_lastY = CLEAR_MANAGER_NO_CELL;
                            m_lastX = CLEAR_MANAGER_NO_CELL;
                            gEditManager->m_mapChanged = 1;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case EDIT_CONTROL_TOOL_OPTIONS:
                            ClearOptionsDialog();
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_RIGHT_CLICK:
                    if (message.id == EDIT_CONTROL_TOOL_OPTIONS)
                        NormalDialog(
                            gClearToolHelp[CLEAR_TOOL_HELP_OPTIONS],
                            NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                    break;
                case WIDGET_COMMAND_HOVER:
                    if (message.id != EDIT_CONTROL_MAP
                        && message.id == gEditManager->m_lastCommandId)
                        return MESSAGE_DISPATCH_CONSUME;
                    gEditManager->m_lastCommandId = message.id;
                    switch (message.id) {
                        case EDIT_CONTROL_MAP:
                            gMouseManager->MouseCoords(newX, newY);
                            gEditManager->ScreenToCell(newX, newY);
                            if (gEditManager->m_cursorX != newX
                                || gEditManager->m_cursorY != newY) {
                                gEditManager->m_cursorX = newX;
                                gEditManager->m_cursorY = newY;
                                gEditManager->UpdateCursor();
                            }
                            break;
                    }
                    return MESSAGE_DISPATCH_CONSUME;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}
