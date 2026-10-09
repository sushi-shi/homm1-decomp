#ifndef HOMM1_BASE_FONT_H
#define HOMM1_BASE_FONT_H

#include <BASE/resource.h>

class icon;

i32 RemapCyrillicCharacter(i32 character);

enum FontAlignment {
    FONT_ALIGN_LEFT = 0,
    FONT_ALIGN_CENTER = 1,
    FONT_ALIGN_RIGHT = 2
};

enum FontGlyphConstant {
    FONT_GLYPH_INDEX_LAST = 161,
    FONT_GLYPH_ADVANCE_SPACING = 1,
    FONT_GLYPH_ENTRY_WORDS = 6,
    FONT_GLYPH_WIDTH_WORD = 2,
    FONT_GLYPH_SPACE = 0
};

enum FontCharacterCode {
    FONT_CODE_ASCII_LAST = 0x7f,
    FONT_CODE_UNKNOWN = FONT_CODE_ASCII_LAST,
    FONT_CODE_CAPITAL_A = 0x80,
    FONT_CODE_CAPITAL_YO = 0xa0,
    FONT_CODE_SMALL_A = 0xa1,
    FONT_CODE_SMALL_YO = 0xc1
};

#pragma pack(push, 1)
class font : public resource {
public:
    i16 m_height;
    i16 m_glyphOffsetY;
    icon* m_glyphIcon;
    font(i16 id);
    virtual ~font();
    void DrawString(char* text, i16 x, i16 y, i16 color);
    void DrawBoundedString(
        char* text,
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        i16 color,
        i16 align
    );
    i32 LineLength(char* text, i16 maxWidth);
    i32 LineWidth(char* text);
};
#pragma pack(pop)
#endif
