#include <H1/Ints.h>

#include <BASE/baseManager.h>

#include <string.h>

baseManager::baseManager(void) : m_next(NULL), m_prev(NULL) {
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_messageMask = BASE_MANAGER_MESSAGE_MASK_ALL;
    m_active = 0;
    strcpy(m_name, "Unknown");
}

i16 baseManager::GetInfo(i16 field) {
    switch (field) {
        case BASE_MANAGER_INFO_MESSAGE_MASK:
            return m_messageMask;
        case BASE_MANAGER_INFO_PRIORITY:
            return m_priority;
        case BASE_MANAGER_INFO_ACTIVE:
            return m_active;
        default:
            return 0;
    }
}
