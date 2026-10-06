#ifndef HOMM1_SOURCE_TOWNOBJECT_H
#define HOMM1_SOURCE_TOWNOBJECT_H

#include <SOURCE/town.h>

class icon;
class border;

class townObject {
public:
    i8 m_animationFrameCount;
    i8 m_animationFrame;
    i8 m_visible;
    i16 m_buildingId;
    icon* m_icon;
    border* m_border;
    townObject(char* name);
    ~townObject();
    void Draw(b8 advanceAnimation);
};
#endif
