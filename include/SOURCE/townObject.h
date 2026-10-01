#ifndef HOMM1_SOURCE_TOWNOBJECT_H
#define HOMM1_SOURCE_TOWNOBJECT_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 3 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

class icon;
class border;

#pragma pack(push, 1)
class townObject {
public:
    signed char m_animationFrameCount;
    signed char m_animationFrame;
    signed char m_visible;
    short m_buildingId;
    icon *m_icon;
    border *m_border;
    // --- constructors ---
    // HoMM1 reads the placement from <name>.tod (retail ret 4).
    townObject(char *);
    ~townObject();
    // --- methods ---
    // HoMM1 passes the animation-advance flag as a byte (retail ret 4, movsx).
    void Draw(signed char);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_TOWNOBJECT_H
