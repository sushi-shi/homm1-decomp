#ifndef HOMM1_SOURCE_TOWNOBJECT_H
#define HOMM1_SOURCE_TOWNOBJECT_H

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/town.h>

class icon;
class border;

#pragma pack(push, 1)
class townObject {
public:
    i8 m_animationFrameCount;
    i8 m_animationFrame;
    i8 m_visible;
    H1_ENUM_STORAGE(BuildingSlotType, i16) m_buildingId;
    icon* m_icon;
    border* m_border;
    // --- constructors ---
    // Reads the placement from <name>.tod.
    townObject(char* name);
    ~townObject();
    // --- methods ---
    void Draw(b8 advanceAnimation);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_TOWNOBJECT_H
