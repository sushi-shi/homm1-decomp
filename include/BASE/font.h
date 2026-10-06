#ifndef HOMM1_BASE_FONT_H
#define HOMM1_BASE_FONT_H

#include <BASE/resource.h>

class icon;

enum FontAlignment {
    FONT_ALIGN_LEFT = 0,
    FONT_ALIGN_CENTER = 1,
    FONT_ALIGN_RIGHT = 2
};

enum FontGlyphConstant {
    FONT_GLYPH_INDEX_LAST = 95,
    FONT_GLYPH_ADVANCE_SPACING = 1
};

#pragma pack(push, 1)
class font : public resource {
public:
    i16 m_height;
    i16 m_headerWord;
    icon* m_glyphIcon;
    font(i16 id);
    virtual ~font();
protected:
    void DrawStringExecute(
        char* text,
        i32 x,
        i32 y,
        i32 mode,
        i32 clipL,
        i32 clipT,
        i32 clipR,
        i32 clipB
    );
public:
    void DrawString(char* text, i16 x, i16 y, i16 color);
    i32 GetCharacterWidth(u8 character);
    void DrawBoundedString(char* str, i16 x, i16 y, i16 width, i16 height, i16 color, i16 align);
    i32 LineLength(char* str, i16 maxW);
    i32 LineWidth(char* text);
};
#pragma pack(pop)
#endif
