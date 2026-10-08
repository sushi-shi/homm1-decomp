#ifndef HOMM1_BASE_BASEMANAGER_H
#define HOMM1_BASE_BASEMANAGER_H

#include <BASE/message.h>
#include <Domains.h>

struct tag_message;

// executive::AddManager appends a manager whose priority is unassigned after
// the list tail. A priority is a rank (the tail's plus one); this is its
// sentinel, not a value domain.
H1_ENUM_CONST_BEGIN(BaseManagerPriority)
    BASE_MANAGER_PRIORITY_UNASSIGNED = -1,
    // The rank of the first manager in an empty list.
    BASE_MANAGER_PRIORITY_FIRST = 0
H1_ENUM_CONST_END(BaseManagerPriority)

// Manager Open/AddManager status.
H1_ENUM_BEGIN(BaseManagerStatus)
    BASE_MANAGER_SUCCESS = 0,
    // heroWindowManager::Open when the screen bitmap is missing.
    WINDOW_MANAGER_OPEN_FAILURE = 1,
    BASE_MANAGER_ERROR = 3
H1_ENUM_END(BaseManagerStatus)

H1_ENUM_FLAGS_BEGIN(BaseManagerMessageMask, i16)
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
H1_ENUM_FLAGS_END(BaseManagerMessageMask)

H1_ENUM_CONST_BEGIN(BaseManagerConstant)
    BASE_MANAGER_NAME_CAPACITY = 30
H1_ENUM_CONST_END(BaseManagerConstant)

// baseManager::GetInfo selectors; any other selector reads as zero.
H1_ENUM_BEGIN(BaseManagerInfoField)
    BASE_MANAGER_INFO_MESSAGE_MASK = 0,
    BASE_MANAGER_INFO_PRIORITY = 1,
    BASE_MANAGER_INFO_ACTIVE = 2
H1_ENUM_END(BaseManagerInfoField)

#pragma pack(push, 1)
class baseManager {
public:
    baseManager* m_next;
    baseManager* m_prev;
    H1_ENUM_STORAGE(BaseManagerMessageMask, i16) m_messageMask;
    i16 m_priority;
    char m_name[BASE_MANAGER_NAME_CAPACITY];
    i16 m_active;

    baseManager();
    // swapManager::Close's inline store.
    void Activate(void) {
        m_active = 1;
    }
    i16 GetInfo(H1_ENUM_PARAM(BaseManagerInfoField, i16) field);
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 priority) = 0;
    virtual void Close() = 0;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(tag_message& message) = 0;
};
#pragma pack(pop)

#endif // HOMM1_BASE_BASEMANAGER_H
