// Located from HoMM2 Buka 2.1; HoMM1 stores all resource identifiers as
// signed words.

#include <match.h>

#include <BASE/resource.h>

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
