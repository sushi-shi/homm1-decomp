#ifndef HOMM1_SOURCE_STRIP_H
#define HOMM1_SOURCE_STRIP_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

// forward declarations:
class armyGroup;

// strip::strip and townManager::ResetStrips fix this packed prefix.
#pragma pack(push, 1)
class strip {
public:
    char m_unknown00[0x16];
    short m_x;
    short m_y;
    signed char m_unknown1a;
    signed char m_selectedSlot;
    char m_unknown1c[0x29];
    armyGroup *m_army;
    // --- constructors ---
    strip(int, int, int, unsigned long int, int, class armyGroup *, int, int, int);
    ~strip();
    // --- methods ---
    void Draw(void);
    void DrawIcons(int);
    void DrawFrame(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_STRIP_H
