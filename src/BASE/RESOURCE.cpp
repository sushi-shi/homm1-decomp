// Located from HoMM2 Buka 2.1; HoMM1 stores all resource identifiers as
// signed words.

#include <match.h>

#include <BASE/resource.h>

#include <stddef.h>

// HoMM2 Buka's default resource: an empty, unlisted bitmap record.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00477650, 0x3c)
resource::resource(void) {
    m_resourceType = RESOURCE_CATEGORY_BITMAP;
    m_refCount = RESOURCE_REFERENCE_EMPTY;
    m_id = 0;
    m_next = NULL;
}

VA(0x0047768c, 0x43)
resource::resource(
    i16 category,
    i16 id,
    H1_ENUM_PARAM(ResourceReferenceCount, i16) refCount,
    resource* next
) {
    m_resourceType = H1_ENUM_CAST(ResourceCategory, i16, category);
    m_id = id;
    m_refCount = refCount;
    m_next = next;
}

VA(0x004776cf, 0x14)
resource::~resource(void) {}
