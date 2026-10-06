#ifndef HOMM1_BASE_FONT_H
#define HOMM1_BASE_FONT_H

#include <BASE/resource.h>

class icon;

i32 RemapCyrillicCharacter(i32 character);

H1_ENUM_BEGIN(FontAlignment)
    FONT_ALIGN_LEFT = 0,
    FONT_ALIGN_CENTER = 1,
    FONT_ALIGN_RIGHT = 2
H1_ENUM_END(FontAlignment)

H1_ENUM_CONST_BEGIN(FontGlyphConstant)
    FONT_GLYPH_INDEX_LAST = 161,
    FONT_GLYPH_ADVANCE_SPACING = 1,
    // Buka reads glyph widths through a word view of the icon directory:
    // each glyph is 6 words (one 12-byte IconEntry) and its width is word 2
    // (IconEntry::w).
    FONT_GLYPH_ENTRY_WORDS = 6,
    FONT_GLYPH_WIDTH_WORD = 2,
    // Glyph indices count from the space character; its glyph is blank and
    // DrawString only advances past it.
    FONT_GLYPH_SPACE = 0
H1_ENUM_CONST_END(FontGlyphConstant)

// Character codes in Buka's font order: ASCII through 0x7f, then the CP1251
// capitals A..Ya, capital Yo, the small letters a..ya and small Yo.
// RemapCyrillicCharacter moves CP1251 letters to these codes; any other code
// above ASCII draws as the last ASCII glyph.
H1_ENUM_CONST_BEGIN(FontCharacterCode)
    FONT_CODE_ASCII_LAST = 0x7f,
    FONT_CODE_UNKNOWN = FONT_CODE_ASCII_LAST,
    FONT_CODE_CAPITAL_A = 0x80,
    FONT_CODE_CAPITAL_YO = 0xa0,
    FONT_CODE_SMALL_A = 0xa1,
    FONT_CODE_SMALL_YO = 0xc1
H1_ENUM_CONST_END(FontCharacterCode)

#pragma pack(push, 1)
class font : public resource {
public:
    i16 m_height;
    i16 m_glyphOffsetY;
    icon* m_glyphIcon;
    // --- constructors ---
    font(i16 id);
    virtual ~font();
    // --- methods ---
    void DrawString(char* text, i16 x, i16 y, i16 color);
    void DrawBoundedString(
        char* text,
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        i16 color,
        H1_ENUM_PARAM(FontAlignment, i16) align
    );
    i32 LineLength(char* text, i16 maxWidth);
    i32 LineWidth(char* text);
};
#pragma pack(pop)
#endif // HOMM1_BASE_FONT_H
