#ifndef HOMM1_BASE_FONT_H
#define HOMM1_BASE_FONT_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 0 own-virtual, 0 static data.

#include <BASE/resource.h>

class icon;

H1_ENUM_BEGIN(FontAlignment)
    FONT_ALIGN_LEFT = 0,
    FONT_ALIGN_CENTER = 1,
    FONT_ALIGN_RIGHT = 2
H1_ENUM_END(FontAlignment)

H1_ENUM_CONST_BEGIN(FontGlyphConstant)
    FONT_GLYPH_INDEX_LAST = 95,
    FONT_GLYPH_ADVANCE_SPACING = 1
H1_ENUM_CONST_END(FontGlyphConstant)

#pragma pack(push, 1)
class font : public resource {
public:
    i16 m_height;
    i16 m_headerWord;
    icon* m_glyphIcon;
    // --- constructors ---
    font(i16);
    virtual ~font();
    // --- methods ---
protected:
    void
    DrawStringExecute(char* text, i32 x, i32 y, i32 mode, i32 clipL, i32 clipT, i32 clipR, i32 clipB); // ?...@font@@IAE... (protected)
public:
    void DrawString(char*, i16, i16, i16);
    i32 GetCharacterWidth(u8 character);
    void DrawBoundedString(char*, i16, i16, i16, i16, i16, i16);
    i32 LineLength(char*, i16);
    i32 LineWidth(char*);
};
#pragma pack(pop)
#endif // HOMM1_BASE_FONT_H
