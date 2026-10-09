#ifndef HOMM1_EDITOR_CLEARMANAGER_H
#define HOMM1_EDITOR_CLEARMANAGER_H

#include <BASE/baseManager.h>

class button;
class iconWidget;
struct tag_message;

enum ClearManagerLayout {
    CLEAR_OPTIONS_BUTTON_X = 504,
    CLEAR_OPTIONS_BUTTON_Y = 268,
    CLEAR_OPTIONS_BUTTON_WIDTH = 95,
    CLEAR_OPTIONS_BUTTON_HEIGHT = 25
};

enum ClearDragMode {
    CLEAR_DRAG_CELLS = 0,
    CLEAR_DRAG_RECTANGLE = 1
};

#pragma pack(push, 1)
class clearManager : public baseManager {
public:
    button* m_optionsButton;
    i16 m_lastX;
    i16 m_lastY;
    i16 m_dispatchMask;
    iconWidget* m_panel;

    clearManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(tag_message& message) ;
};
#pragma pack(pop)

#endif
