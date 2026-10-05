// Base manager construction; Buka BASEMGR correspondence, retail authority.

#include <match.h>

#include <BASE/baseManager.h>

#include <string.h>

VA(0x004688c0, 0x5a)
baseManager::baseManager(void) : m_next(NULL), m_prev(NULL) {
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_messageMask = BASE_MANAGER_MESSAGE_MASK_ALL;
    m_active = 0;
    strcpy(m_name, "Unknown");
}

// Descriptive name: an unreferenced manager-property accessor; the original
// name is unavailable.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046891a, 0x48)
i16 baseManager::GetInfo(H1_ENUM_PARAM(BaseManagerInfoField, i16) field) {
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
