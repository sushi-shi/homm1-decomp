#ifndef HOMM1_BASE_BASEMANAGER_H
#define HOMM1_BASE_BASEMANAGER_H

#include <Domains.h>

struct tag_message;

H1_ENUM_BEGIN(BaseManagerMessageMask)
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
H1_ENUM_END(BaseManagerMessageMask)

H1_ENUM_BEGIN(BaseManagerConstant)
    BASE_MANAGER_NAME_CAPACITY = 30,
    BASE_MANAGER_SIZE = 0x30
H1_ENUM_END(BaseManagerConstant)

#pragma pack(push, 1)
class baseManager {
public:
    baseManager *m_next;
    baseManager *m_prev;
    H1_ENUM_STORAGE(BaseManagerMessageMask, short) m_messageMask;
    short m_priority;
    char m_name[BASE_MANAGER_NAME_CAPACITY];
    short m_active;

    baseManager();
    virtual short Open(short) = 0;
    virtual void Close() = 0;
    virtual short Main(tag_message &) = 0;
};
#pragma pack(pop)

typedef char BaseManagerSizeCheck[
    sizeof(baseManager) == BASE_MANAGER_SIZE ? 1 : -1];

#endif
