#ifndef HOMM1_SOURCE_STRIP_H
#define HOMM1_SOURCE_STRIP_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

// forward declarations:
class armyGroup;
class border;
class font;
class heroWindow;
class icon;

// strip's constructor, destructor and DrawIcons fix this packed layout.
#pragma pack(push, 1)
class strip {
public:
    heroWindow* m_window;
    char m_unknown04[0x12];
    short m_x;
    short m_y;
    signed char m_stripType;
    signed char m_selectedSlot;
    border* m_borders[6];
    font* m_font;
    icon* m_stripIcon;
    icon* m_monsterIcon;
    icon* m_portraitIcon;
    signed char m_portraitFrame;
    armyGroup* m_army;
    // --- constructors ---
    // HoMM1 retail: eight arguments (ret 0x20).
    strip(short, short, signed char, short, signed char, class armyGroup*, short, int);
    ~strip();
    // --- methods ---
    void Draw(void);
    void DrawIcons(signed char);
    void DrawFrame(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_STRIP_H
