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
    : resource(RESOURCE_CATEGORY_FONT, id, RESOURCE_REFERENCE_INITIAL, NULL)
{
    signed char name[RESOURCE_NAME_CAPACITY];
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
    IconEntry *entries = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    signed char glyph = 0;
    short drawX = x;
    short index = 0;
    while (text[index] != 0) {
        glyph = text[index] - ' ';
        if (glyph < 0 || glyph > FONT_GLYPH_INDEX_LAST)
            glyph = FONT_GLYPH_INDEX_LAST;
        if (glyph != 0)
            m_glyphIcon->FillToBuffer(drawX, y + m_headerWord, glyph, color, ICON_DRAW_NORMAL, 0);
        drawX += entries[glyph].w + FONT_GLYPH_ADVANCE_SPACING;
        index++;
    }
}

VA(0x0047b4d0, 0x2d0)
void font::DrawBoundedString(char *str, short x, short y, short width, short height, short color, short align)
{
    short s;
    signed char q;
    IconEntry *widths;
    char spaceChar;
    // Names place the frame slots; the order gives the operand sort keys of
    // p < s, p >= r, lw <= width, x + t and y + u.
    short r;
    short lineEnd;
    short drawColor;
    short t;
    short p;
    short lw;
    short u;
    char *w;
    char v;

    s = strlen(str);
    widths = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    spaceChar = ' ';
    t = 0;
    u = 0;
    r = 0;
    lineEnd = 0;
    p = 0;
    lw = 0;
    w = str;
    drawColor = color;
    while (p < s && w[p] != 0 && m_height + u <= height) {
        while (w[p] != 0 && w[p] != '\n' && lw <= width) {
            q = w[p] - ' ';
            if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                q = FONT_GLYPH_INDEX_LAST;
            lw = widths[q].w + lw + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
        if (lw > width) {
            p--;
            while (w[p] != ' ' && p >= r) {
                q = w[p] - ' ';
                if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                    q = FONT_GLYPH_INDEX_LAST;
                lw -= widths[q].w + FONT_GLYPH_ADVANCE_SPACING;
                p--;
            }
            if (w[p] == ' ')
                lw -= widths[0].w + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = p;
        v = w[lineEnd];
        w[lineEnd] = 0;
        switch (align) {
        case FONT_ALIGN_LEFT: t = 0; break;
        case FONT_ALIGN_CENTER: t = (width - lw) / 2; break;
        case FONT_ALIGN_RIGHT: t = width - lw; break;
        }
        DrawString(w + r, x + t, y + u, drawColor);
        w[lineEnd] = v;
        u += m_height;
        r = lineEnd + 1;
        p = r;
        lw = 0;
    }
}

VA(0x0047b7a0, 0x211)
int font::LineLength(char *str, short maxW)
{
    short lw;
    short p;
    short s = strlen(str);
    signed char q;
    IconEntry *widths = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    char spaceChar = ' ';
    int z = 0;
    short t = 0;
    short y;
    short r;
    char *w;
    char v;

    // lw, then p, lead the declarations for the operand sort keys of lw <= maxW
    // and p < s; r follows p for p >= r. Stores keep retail order.
    r = 0;
    y = 0;
    p = 0;
    lw = 0;
    w = str;
    while (p < s && w[p] != 0) {
        while (w[p] != 0 && w[p] != '\n' && lw <= maxW) {
            q = w[p] - ' ';
            if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                q = FONT_GLYPH_INDEX_LAST;
            lw = widths[q].w + lw + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
        if (lw > maxW) {
            p--;
            while (w[p] != ' ' && p >= r) {
                q = w[p] - ' ';
                if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                    q = FONT_GLYPH_INDEX_LAST;
                lw -= widths[q].w + FONT_GLYPH_ADVANCE_SPACING;
                p--;
            }
            if (w[p] == ' ')
                lw -= widths[0].w + FONT_GLYPH_ADVANCE_SPACING;
        }
        y = p;
        v = w[y];
        w[y] = 0;
        w[y] = v;
        z++;
        r = y + 1;
        p = r;
        lw = 0;
    }
    return z;
}

VA(0x0047b9c0, 0x108)
int font::LineWidth(char *text)
{
    signed char q;
    int u;
    IconEntry *table;
    // PoL 2.0 retains this shared line-layout local census; HoMM1's /Od
    // retail body proves y's dword store and the five word stores below
    // (u is the census's unused slot). s follows y for the operand sort key.
    int y;
    short s;
    short t, r, x, p, w;
    char *v;

    s = strlen(text);
    table = reinterpret_cast<IconEntry *>(m_glyphIcon->m_data); // byte-evidenced: packed frame directory decoded from resource bytes.
    y = 0;
    t = 0;
    r = 0;
    x = 0;
    p = 0;
    w = 0;
    v = text;
    while (p < s && v[p] != 0) {
        while (v[p] != 0 && v[p] != '\n') {
            q = v[p] - ' ';
            if (q < 0 || q > FONT_GLYPH_INDEX_LAST)
                q = FONT_GLYPH_INDEX_LAST;
            w += table[q].w + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
    }
    return w;
}
