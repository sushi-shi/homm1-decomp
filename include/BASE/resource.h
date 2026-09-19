#ifndef HOMM1_BASE_RESOURCE_H
#define HOMM1_BASE_RESOURCE_H

#include <Domains.h>

H1_ENUM_BEGIN(ResourceCategory)
    RESOURCE_CATEGORY_BITMAP = 0,
    RESOURCE_CATEGORY_ICON = 1,
    RESOURCE_CATEGORY_PALETTE = 2,
    RESOURCE_CATEGORY_TILESET = 3,
    RESOURCE_CATEGORY_FONT = 5,
    RESOURCE_CATEGORY_SAMPLE = 6
H1_ENUM_END(ResourceCategory)

#pragma pack(push, 1)
class resource {
public:
    H1_ENUM_STORAGE(ResourceCategory, short) m_resourceType;
    short m_refCount;
    short m_id;
    resource *m_next;

    resource();
    resource(short, short, short, resource *);
    virtual ~resource() = 0;
};
#pragma pack(pop)

#endif
