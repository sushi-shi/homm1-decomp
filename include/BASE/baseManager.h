#ifndef HOMM1_BASE_BASEMANAGER_H
#define HOMM1_BASE_BASEMANAGER_H

#include <BASE/message.h>

struct tag_message;

enum BaseManagerPriority {
    BASE_MANAGER_PRIORITY_UNASSIGNED = -1,
    BASE_MANAGER_PRIORITY_FIRST = 0
};

enum BaseManagerStatus {
    BASE_MANAGER_SUCCESS = 0,
    WINDOW_MANAGER_OPEN_FAILURE = 1,
    BASE_MANAGER_ERROR = 3
};

enum BaseManagerMessageMask {
    BASE_MANAGER_MESSAGE_MASK_ALL = -1,
    BASE_MANAGER_ACCEPT_MOUSE_MOVE = 4,
    BASE_MANAGER_ACCEPT_LEFT_BUTTON_UP = 0x10,
    BASE_MANAGER_ACCEPT_RIGHT_BUTTON_DOWN = 0x20,
    BASE_MANAGER_ACCEPT_RIGHT_BUTTON_UP = 0x40,
    BASE_MANAGER_ACCEPT_RESOURCE = 0x80,
    BASE_MANAGER_ACCEPT_SWAP = 0x100,
    BASE_MANAGER_ACCEPT_WIDGET = 0x200,
    BASE_MANAGER_ACCEPT_ADVENTURE = 0x400,
    BASE_MANAGER_ACCEPT_TOWN_EVENT = 0x800,
    BASE_MANAGER_ACCEPT_EXECUTIVE = 0x4000
};

enum BaseManagerConstant {
    BASE_MANAGER_NAME_CAPACITY = 30
};

enum BaseManagerInfoField {
    BASE_MANAGER_INFO_MESSAGE_MASK = 0,
    BASE_MANAGER_INFO_PRIORITY = 1,
    BASE_MANAGER_INFO_ACTIVE = 2
};

class baseManager {
public:
    baseManager* m_next;
    baseManager* m_prev;
    i16 m_messageMask;
    i16 m_priority;
    char m_name[BASE_MANAGER_NAME_CAPACITY];
    i16 m_active;

    baseManager();
    void Activate(void) {
        m_active = 1;
    }
    i16 GetInfo(i16 field);
    virtual i16 Open(i16 priority) = 0;
    virtual void Close() = 0;
    virtual i16 Main(tag_message& message) = 0;
};

#endif
