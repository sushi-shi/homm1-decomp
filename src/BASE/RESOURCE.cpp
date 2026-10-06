#include <H1/Ints.h>

#include <BASE/resource.h>

resource::resource(
    i16 category,
    i16 id,
    i16 refCount,
    resource* next
) {
    m_resourceType = static_cast<i16>(category);
    m_id = id;
    m_refCount = refCount;
    m_next = next;
}

resource::~resource(void) {}
