// Located from HoMM2 Buka 2.1; HoMM1 stores all resource identifiers as
// signed words.

#include <match.h>

#include <BASE/resource.h>

VA(0x0047fc20, 0x2f)
resource::resource(short category, short id, short refCount, resource *next)
{
    m_resourceType = H1_ENUM_CAST(ResourceCategory, short, category);
    m_id = id;
    m_refCount = refCount;
    m_next = next;
}

VA(0x0047fc50, 0x7)
resource::~resource(void)
{
}
