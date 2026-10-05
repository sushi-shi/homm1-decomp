// HoMM1 font loading, source-correspondent to the Buka 2.1 resource family.

#include <match.h>

#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/IconEntry.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <string.h>

VA(0x00471dd0, 0xc7)
font::font(i16 id) : resource(RESOURCE_CATEGORY_FONT, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    i8 name[RESOURCE_NAME_CAPACITY];
    gpResourceManager->PointToFile(id);
    m_height = gpResourceManager->ReadWord();
    m_headerWord = gpResourceManager->ReadWord();
    gpResourceManager->Read13(name);
    gLoadingMonoIcon = 1;
    m_glyphIcon = gpResourceManager->GetIcon(
        reinterpret_cast<char*>(name)
    ); // byte-evidenced: Read13 and GetIcon use differently signed byte names.
    gLoadingMonoIcon = 0;
}

VA_COMPGEN(0x00472760, 0x2e, "??_Gfont@@UAEPAXI@Z", 0x00471dd0)
VA(0x00471e97, 0x5b)
font::~font(void) {
    gpResourceManager->Dispose(m_glyphIcon);
}

// Map CP1251 codes to Buka's font character order.
VA(0x00471ef2, 0x4c)
i32 RemapCyrillicCharacter(i32 character) {
    if (character == 0xa8)
        return 0xa0;
    if (character == 0xb8)
        return 0xc1;
    if (character < 0xc0)
        return 0xa1;
    if (character < 0xe0)
        return character - 0x40;
    return character - 0x3f;
}

VA(0x00471f3e, 0xff)
void font::DrawString(char* text, i16 x, i16 y, i16 color) {
    IconEntry* entries = reinterpret_cast<IconEntry*>(
        m_glyphIcon->m_data
    ); // byte-evidenced: packed frame directory decoded from resource bytes.
    i32 glyph = 0;
    i16 drawX = x;
    i16 index = 0;
    while (text[index] != 0) {
        glyph = static_cast<u8>(text[index]);
        if (glyph < ' ' || (glyph > 0x7f && glyph < 0xc0 && glyph != 0xb8 && glyph != 0xa8))
            glyph = 0x7f;
        else if (glyph > 0x7f)
            glyph = RemapCyrillicCharacter(glyph);
        glyph -= ' ';
        if (glyph != 0)
            m_glyphIcon->FillToBuffer(
                drawX,
                y + m_headerWord,
                glyph,
                color,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        drawX += entries[glyph].w + FONT_GLYPH_ADVANCE_SPACING;
        index++;
    }
}

VA(0x0047203d, 0x34f)
void font::DrawBoundedString(char* str, i16 x, i16 y, i16 width, i16 height, i16 color, i16 align) {
    i16 s;
    i32 q;
    IconEntry* widths;
    char spaceChar;
    // Names place the frame slots; the order gives the operand sort keys of
    // p < s, p >= r, lw <= width, x + t and y + u.
    i16 r;
    i16 lineEnd;
    i16 drawColor;
    i16 t;
    i16 p;
    i16 lw;
    i16 u;
    char* w;
    char v;

    s = strlen(str);
    widths = reinterpret_cast<IconEntry*>(
        m_glyphIcon->m_data
    ); // byte-evidenced: packed frame directory decoded from resource bytes.
    spaceChar = ' ';
    t = 0;
    u = 0;
    r = 0;
    lineEnd = 0;
    p = 0;
    lw = 0;
    w = new char[s + 1];
    strcpy(w, str);
    drawColor = color;
    while (p < s && w[p] != 0 && u + m_height <= height) {
        while (w[p] != 0 && w[p] != '\n' && lw <= width) {
            q = static_cast<u8>(w[p]);
            if (q < ' ' || (q > 0x7f && q < 0xc0 && q != 0xb8 && q != 0xa8))
                q = 0x7f;
            else if (q > 0x7f)
                q = RemapCyrillicCharacter(q);
            q -= ' ';
            lw += widths[q].w + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
        if (lw > width) {
            p--;
            while (w[p] != ' ' && p >= r) {
                q = static_cast<u8>(w[p]);
                if (q < ' ' || (q > 0x7f && q < 0xc0 && q != 0xb8 && q != 0xa8))
                    q = 0x7f;
                else if (q > 0x7f)
                    q = RemapCyrillicCharacter(q);
                q -= ' ';
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
            case FONT_ALIGN_LEFT:
                t = 0;
                break;
            case FONT_ALIGN_CENTER:
                t = (width - lw) / 2;
                break;
            case FONT_ALIGN_RIGHT:
                t = width - lw;
                break;
        }
        DrawString(w + r, t + x, u + y, drawColor);
        w[lineEnd] = v;
        u += m_height;
        r = lineEnd + 1;
        p = r;
        lw = 0;
    }
    delete[] w;
}

VA(0x0047238c, 0x25f)
i32 font::LineLength(char* str, i16 maxW) {
    i16 lw;
    i16 p;
    i16 s = strlen(str);
    i32 q;
    IconEntry* widths = reinterpret_cast<IconEntry*>(
        m_glyphIcon->m_data
    ); // byte-evidenced: packed frame directory decoded from resource bytes.
    char spaceChar = ' ';
    i32 z = 0;
    i16 t = 0;
    i16 y;
    i16 r;
    char* w;
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
            q = static_cast<u8>(w[p]);
            if (q < ' ' || (q > 0x7f && q < 0xc0 && q != 0xb8 && q != 0xa8))
                q = 0x7f;
            else if (q > 0x7f)
                q = RemapCyrillicCharacter(q);
            q -= ' ';
            lw += widths[q].w + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
        if (lw > maxW) {
            p--;
            while (w[p] != ' ' && p >= r) {
                q = static_cast<u8>(w[p]);
                if (q < ' ' || (q > 0x7f && q < 0xc0 && q != 0xb8 && q != 0xa8))
                    q = 0x7f;
                else if (q > 0x7f)
                    q = RemapCyrillicCharacter(q);
                q -= ' ';
                lw -= widths[q].w + FONT_GLYPH_ADVANCE_SPACING;
                p--;
            }
            if (w[p] == ' ')
                lw -= widths[0].w + FONT_GLYPH_ADVANCE_SPACING;
        }
        y = p;
        z++;
        r = y + 1;
        p = r;
        lw = 0;
    }
    return z;
}

VA(0x004725eb, 0x133)
i32 font::LineWidth(char* text) {
    i32 q;
    i32 u;
    IconEntry* table;
    // PoL 2.0 retains this shared line-layout local census; HoMM1's /Od
    // retail body proves y's dword store and the five word stores below
    // (u is the census's unused slot). s follows y for the operand sort key.
    i32 y;
    i16 s;
    i16 t, r, x, p, w;
    char* v;

    s = strlen(text);
    table = reinterpret_cast<IconEntry*>(
        m_glyphIcon->m_data
    ); // byte-evidenced: packed frame directory decoded from resource bytes.
    y = 0;
    t = 0;
    r = 0;
    x = 0;
    p = 0;
    w = 0;
    v = text;
    while (p < s && v[p] != 0) {
        while (v[p] != 0 && v[p] != '\n') {
            q = static_cast<u8>(v[p]);
            if (q < ' ' || (q > 0x7f && q < 0xc0 && q != 0xb8 && q != 0xa8))
                q = 0x7f;
            else if (q > 0x7f)
                q = RemapCyrillicCharacter(q);
            q -= ' ';
            w += table[q].w + FONT_GLYPH_ADVANCE_SPACING;
            p++;
        }
    }
    return w;
}
