#ifndef HOMM1_EDITOR_CLEARMANAGER_H
#define HOMM1_EDITOR_CLEARMANAGER_H

// The eraser tool (src/EDITOR/CLEARMGR.cpp): editManager::SelectTool runs
// it as the tool manager. The unit name CLEARMGR is descriptive; Open stores
// the class name "clearManager".

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>

class button;
class iconWidget;
struct tag_message;

// Main tests message.type against the dispatch mask the managers share.
H1_ENUM_CONST_BEGIN(ClearManagerConstant)
    CLEAR_MANAGER_DISPATCH_MASK = 0x32f,
    // m_lastX/m_lastY before the first drag step.
    CLEAR_MANAGER_NO_CELL = -1
H1_ENUM_CONST_END(ClearManagerConstant)

// The tool panel (a buttons.icn frame) and its options button.
H1_ENUM_CONST_BEGIN(ClearManagerLayout)
    CLEAR_PANEL_X = 480,
    CLEAR_PANEL_Y = 197,
    CLEAR_PANEL_WIDTH = 144,
    CLEAR_PANEL_HEIGHT = 139,
    CLEAR_PANEL_FRAME = 36,
    CLEAR_OPTIONS_BUTTON_X = 504,
    CLEAR_OPTIONS_BUTTON_Y = 268,
    CLEAR_OPTIONS_BUTTON_WIDTH = 95,
    CLEAR_OPTIONS_BUTTON_HEIGHT = 25,
    CLEAR_OPTIONS_BUTTON_FRAME = 34,
    CLEAR_OPTIONS_BUTTON_PRESSED_FRAME = 35
H1_ENUM_CONST_END(ClearManagerLayout)

// Drag modes: shift erases the cells the cursor crosses, a plain drag erases
// the rectangle it spans.
H1_ENUM_BEGIN(ClearDragMode)
    CLEAR_DRAG_CELLS = 0,
    CLEAR_DRAG_RECTANGLE = 1
H1_ENUM_END(ClearDragMode)

#pragma pack(push, 1)
class clearManager : public baseManager {
public:
    // Opens the eraser options window.
    button* m_optionsButton;
    // The last map cell a drag step visited.
    i16 m_lastX;
    i16 m_lastY;
    H1_ENUM_STORAGE(BaseManagerMessageMask, i16) m_dispatchMask;
    // The tool panel's backdrop.
    iconWidget* m_panel;

    clearManager(void);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(tag_message& message) OVERRIDE;
};
#pragma pack(pop)

#endif // HOMM1_EDITOR_CLEARMANAGER_H
