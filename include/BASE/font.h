#ifndef HOMM1_BASE_FONT_H
#define HOMM1_BASE_FONT_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 0 own-virtual, 0 static data.

#include <BASE/resource.h>

class icon;

// clang-format off
H1_ENUM_BEGIN(FontAlignment)
    FONT_ALIGN_LEFT = 0,
    FONT_ALIGN_CENTER = 1,
    FONT_ALIGN_RIGHT = 2
H1_ENUM_END(FontAlignment)

H1_ENUM_CONST_BEGIN(FontGlyphConstant)
    FONT_GLYPH_INDEX_LAST = 95,
    FONT_GLYPH_ADVANCE_SPACING = 1
H1_ENUM_CONST_END(FontGlyphConstant)
// clang-format on

#pragma pack(push, 1)
class font : public resource {
public:
    short m_height;
    short m_headerWord;
    icon *m_glyphIcon;
    // --- constructors ---
    font(short);
    virtual ~font();
    // --- methods ---
protected:
    void DrawStringExecute(char *, int, int, int, int, int, int, int);   // ?...@font@@IAE... (protected)
public:
    void DrawString(char *, short, short, short);
    int GetCharacterWidth(unsigned char);
    void DrawBoundedString(char *, short, short, short, short, short, short);
    int LineLength(char *, short);
    int LineWidth(char *);
};
#pragma pack(pop)
#endif // HOMM1_BASE_FONT_H
