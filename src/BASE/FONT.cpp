// HoMM1 font loading, source-correspondent to the Buka 2.1 resource family.

#include <match.h>

#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/IconEntry.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

#include <string.h>

#pragma intrinsic(strlen)

VA(0x0047b2c0, 0xa1)
font::font(short id)
    : resource(RESOURCE_CATEGORY_FONT, id, 1, 0)
{
    signed char name[13];
    gpResourceManager->PointToFile(id);
    m_height = gpResourceManager->ReadWord();
    m_headerWord = gpResourceManager->ReadWord();
    gpResourceManager->Read13(name);
    gbLoadingMonoIcon = 1;
    m_glyphIcon = gpResourceManager->GetIcon(reinterpret_cast<char *>(name)); // byte-evidenced: Read13 and GetIcon use differently signed byte names.
    gbLoadingMonoIcon = 0;
}

VA_COMPGEN(0x0047b370, 0x39, "??_Gfont@@UAEPAXI@Z", 0x0047b2c0)
VA(0x0047b3b0, 0x39)
font::~font(void)
{
    gpResourceManager->Dispose(m_glyphIcon);
}

VA(0x0047b3f0, 0xd1)
void font::DrawString(char *text, short x, short y, short color)
{
    IconEntry *glyphs = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    signed char glyph = 0;
    short position = x;
    short index = 0;
    while (text[index] != 0) {
        glyph = text[index] - ' ';
        if (glyph < 0 || glyph > FONT_GLYPH_INDEX_LAST)
            glyph = FONT_GLYPH_INDEX_LAST;
        if (glyph != 0)
            m_glyphIcon->FillToBuffer(position, y + m_headerWord, glyph, color, 0, 0);
        position += glyphs[glyph].w + FONT_GLYPH_ADVANCE_SPACING;
        index++;
    }
}

VA(0x0047b4d0, 0x2d0)
void font::DrawBoundedString(char *str, short x, short y, short w, short h, short color, short align)
{
    short s = strlen(str);
    signed char q;
    IconEntry *u = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    char aa = ' ';
    short t = 0, yOff = 0, r = 0, lineEnd = 0, p = 0, width = 0;
    char *line = str;
    short drawColor = color;
    char v;
    while (p < s && line[p] != 0 && m_height + yOff <= h) {
        while (line[p] != 0 && line[p] != '\n' && width <= w) {
            q = line[p] - ' ';
            if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                q = FONT_GLYPH_INDEX_LAST;
            width = u[q].w + width + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
        if (width > w) {
            p--;
            while (line[p] != ' ' && p >= r) {
                q = line[p] - ' ';
                if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                    q = FONT_GLYPH_INDEX_LAST;
                width -= u[q].w + FONT_GLYPH_ADVANCE_SPACING;
                p--;
            }
            if (line[p] == ' ')
                width -= u[0].w + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = p;
        v = line[lineEnd];
        line[lineEnd] = 0;
        switch (align) {
        case FONT_ALIGN_LEFT: t = 0; break;
        case FONT_ALIGN_CENTER: t = (w - width) / 2; break;
        case FONT_ALIGN_RIGHT: t = w - width; break;
        }
        DrawString(line + r, x + t, y + yOff, drawColor);
        line[lineEnd] = v;
        yOff += m_height;
        r = lineEnd + 1;
        p = r;
        width = 0;
    }
}

VA(0x0047b7a0, 0x211)
int font::LineLength(char *str, short maxW)
{
    short s = strlen(str);
    signed char q;
    IconEntry *u = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    char aa = ' ';
    int z = 0;
    short t = 0, r = 0, y = 0, p = 0, x = 0;
    char *w = str;
    char v;
    while (p < s && w[p] != 0) {
        while (w[p] != 0 && w[p] != '\n' && x <= maxW) {
            q = w[p] - ' ';
            if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                q = FONT_GLYPH_INDEX_LAST;
            x = u[q].w + x + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
        if (x > maxW) {
            p--;
            while (w[p] != ' ' && p >= r) {
                q = w[p] - ' ';
                if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                    q = FONT_GLYPH_INDEX_LAST;
                x -= u[q].w + FONT_GLYPH_ADVANCE_SPACING;
                p--;
            }
            if (w[p] == ' ')
                x -= u[0].w + FONT_GLYPH_ADVANCE_SPACING;
        }
        y = p;
        v = w[y];
        w[y] = 0;
        w[y] = v;
        z++;
        r = y + 1;
        p = r;
        x = 0;
    }
    return z;
}

VA(0x0047b9c0, 0x108)
int font::LineWidth(char *text)
{
    short s = strlen(text);
    signed char q;
    IconEntry *u = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    // PoL 2.0 retains this shared line-layout local census; HoMM1's /Od
    // retail body proves y's dword store and the five word stores below.
    int y = 0;
    short t = 0, r = 0, x = 0, p = 0, w = 0;
    char *v = text;
    while (p < s && v[p] != 0) {
        while (v[p] != 0 && v[p] != '\n') {
            q = v[p] - ' ';
            if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                q = FONT_GLYPH_INDEX_LAST;
            w += u[q].w + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
    }
    return w;
}
