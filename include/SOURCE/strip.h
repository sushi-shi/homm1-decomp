#ifndef HOMM1_SOURCE_STRIP_H
#define HOMM1_SOURCE_STRIP_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>

// strip window layout and strip.icn frames (Buka strip.h StripConstant
// names and values; HoMM1 picks the faction background as type / 6 + 3).
H1_ENUM_CONST_BEGIN(StripConstant)
    STRIP_WINDOW_WIDTH = 0x228,
    STRIP_WINDOW_HEIGHT = 0x69,
    STRIP_PORTRAIT_X = 5,
    STRIP_CONTENT_Y = 6,
    STRIP_ARMY_FIRST_X = 0x70,
    STRIP_ARMY_X_STEP = 0x58,
    STRIP_ARMY_BORDER_WIDTH = 0x52,
    STRIP_BORDER_HEIGHT = 0x5d,
    STRIP_PORTRAIT_BORDER_WIDTH = 0x65,
    STRIP_MONSTER_X = 0x77,
    STRIP_MONSTER_Y = 0x13,
    STRIP_QUANTITY_Y = 0x56,
    STRIP_QUANTITY_WIDTH = 0x4d,
    STRIP_QUANTITY_HEIGHT = 0xd,
    STRIP_BACKGROUND_FRAME = 0,
    STRIP_SELECTED_FRAME = 1,
    STRIP_EMPTY_FRAME = 2,
    STRIP_FACTION_FRAME_OFFSET = 3,
    STRIP_CREATURES_PER_FACTION = 6,
    // m_selectedSlot (and the town/swap managers' selected army slots) with
    // no slot picked.
    STRIP_SLOT_NONE = -1
H1_ENUM_CONST_END(StripConstant)

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
    i16 m_x;
    i16 m_y;
    i8 m_stripType;
    i8 m_selectedSlot;
    border* m_borders[6];
    font* m_font;
    icon* m_stripIcon;
    icon* m_monsterIcon;
    icon* m_portraitIcon;
    i8 m_portraitFrame;
    armyGroup* m_army;
    // --- constructors ---
    // HoMM1 retail: eight arguments (ret 0x20).
    strip(i16 x, i16 y, i8 stripType, i16 portraitId, i8 portraitFrame, class armyGroup* army, i16 firstBorderId, i32 drawWindow);
    ~strip();
    // --- methods ---
    void Draw(void);
    void DrawIcons(i8 drawWindow);
    void DrawFrame(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_STRIP_H
