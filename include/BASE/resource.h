#ifndef HOMM1_BASE_RESOURCE_H
#define HOMM1_BASE_RESOURCE_H

enum ResourceCategory {
    RESOURCE_CATEGORY_BITMAP = 0,
    RESOURCE_CATEGORY_ICON = 1,
    RESOURCE_CATEGORY_PALETTE = 2,
    RESOURCE_CATEGORY_TILESET = 3,
    RESOURCE_CATEGORY_FONT = 5,
    RESOURCE_CATEGORY_SAMPLE = 6
};

enum ResourceReferenceCount {
    RESOURCE_REFERENCE_UNMANAGED = -1,
    RESOURCE_REFERENCE_EMPTY = 0,
    RESOURCE_REFERENCE_INITIAL = 1
};

#pragma pack(push, 1)
class resource {
public:
    i16 m_resourceType;
    i16 m_refCount;
    i16 m_id;
    resource* m_next;

    resource();
    resource(
        i16 category,
        i16 id,
        i16 refCount,
        resource* next
    );
    virtual ~resource() = 0;
};
#pragma pack(pop)

#endif
