// Base manager construction; Buka BASEMGR correspondence, retail authority.

#include <match.h>

#include <BASE/baseManager.h>

#include <string.h>

VA(0x0047dbe0, 0x4a)
baseManager::baseManager(void) : m_next(NULL), m_prev(NULL) {
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_messageMask = BASE_MANAGER_MESSAGE_MASK_ALL;
    m_active = 0;
    strcpy(m_name, "Unknown");
}
