// HoMM1 font loading.

#include <match.h>

#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <string.h>

VA(0x00471dd0, 0xc7)
font::font(i16 id) : resource(RESOURCE_CATEGORY_FONT, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    char name[RESOURCE_NAME_CAPACITY];
    gpResourceManager->PointToFile(id);
    m_height = gpResourceManager->ReadWord();
    m_headerWord = gpResourceManager->ReadWord();
    gpResourceManager->Read13(name);
    gLoadingMonoIcon = 1;
    m_glyphIcon = gpResourceManager->GetIcon(name);
    gLoadingMonoIcon = 0;
}

VA(0x00471e97, 0x5b)
font::~font(void) {
    gpResourceManager->Dispose(m_glyphIcon);
}

// Map CP1251 codes to Buka's font character order.
VA(0x00471ef2, 0x4c)
i32 RemapCyrillicCharacter(i32 character) {
    if (character == CYRILLIC_CAPITAL_YO)
        return 0xa0;
    if (character == CYRILLIC_SMALL_YO)
        return 0xc1;
    if (character < CYRILLIC_CAPITAL_A)
        return 0xa1;
    if (character < 0xe0)
        return character - 0x40;
    return character - 0x3f;
}

VA(0x00471f3e, 0xff)
void font::DrawString(char* text, i16 x, i16 y, i16 color) {
    i16* entries = m_glyphIcon->m_frameWords;
    i32 glyph = 0;
    i16 pos = x;
    i16 index = 0;
    while (text[index] != 0) {
        glyph = static_cast<u8>(text[index]);
        if (glyph < ' '
            || (glyph > 0x7f && glyph < CYRILLIC_CAPITAL_A && glyph != CYRILLIC_SMALL_YO
                && glyph != CYRILLIC_CAPITAL_YO))
            glyph = 0x7f;
        else if (glyph > 0x7f)
            glyph = RemapCyrillicCharacter(glyph);
        glyph -= ' ';
        if (glyph != 0)
            m_glyphIcon->FillToBuffer(
                pos,
                y + m_headerWord,
                glyph,
                color,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        pos += entries[glyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
               + FONT_GLYPH_ADVANCE_SPACING;
        index++;
    }
}

VA(0x0047203d, 0x34f)
void font::DrawBoundedString(
    char* text,
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    i16 color,
    i16 align
) {
    i16 textLen;
    i32 baseGlyph;
    i16* frameDirectory;
    char space;
    i16 lineStart;
    i16 lineEnd;
    i16 drawColor;
    i16 position;
    i16 alignIndent;
    i16 widthUsed;
    i16 lineTop;
    char* textCopy;
    char breakChar;

    textLen = strlen(text);
    frameDirectory = m_glyphIcon->m_frameWords;
    space = ' ';
    alignIndent = 0;
    lineTop = 0;
    lineStart = 0;
    lineEnd = 0;
    position = 0;
    widthUsed = 0;
    textCopy = new char[textLen + 1];
    strcpy(textCopy, text);
    drawColor = color;
    while (position < textLen && textCopy[position] != 0 && lineTop + m_height <= height) {
        while (textCopy[position] != 0 && textCopy[position] != '\n' && widthUsed <= width) {
            baseGlyph = static_cast<u8>(textCopy[position]);
            if (baseGlyph < ' '
                || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = 0x7f;
            else if (baseGlyph > 0x7f)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            widthUsed += frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
        if (widthUsed > width) {
            position--;
            while (textCopy[position] != ' ' && position >= lineStart) {
                baseGlyph = static_cast<u8>(textCopy[position]);
                if (baseGlyph < ' '
                    || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                        && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                    baseGlyph = 0x7f;
                else if (baseGlyph > 0x7f)
                    baseGlyph = RemapCyrillicCharacter(baseGlyph);
                baseGlyph -= ' ';
                widthUsed -=
                    frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                    + FONT_GLYPH_ADVANCE_SPACING;
                position--;
            }
            if (textCopy[position] == ' ')
                widthUsed -= frameDirectory[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = position;
        breakChar = textCopy[lineEnd];
        textCopy[lineEnd] = 0;
        switch (align) {
            case FONT_ALIGN_LEFT:
                alignIndent = 0;
                break;
            case FONT_ALIGN_CENTER:
                alignIndent = (width - widthUsed) / 2;
                break;
            case FONT_ALIGN_RIGHT:
                alignIndent = width - widthUsed;
                break;
        }
        DrawString(textCopy + lineStart, alignIndent + x, lineTop + y, drawColor);
        textCopy[lineEnd] = breakChar;
        lineTop += m_height;
        lineStart = lineEnd + 1;
        position = lineStart;
        widthUsed = 0;
    }
    delete[] textCopy;
}

VA(0x0047238c, 0x25f)
i32 font::LineLength(char* text, i16 maxWidth) {
    i16 widthUsed;
    i16 thePos;
    i16 textLen = strlen(text);
    i32 baseGlyph;
    i16* widths = m_glyphIcon->m_frameWords;
    char charVal = ' ';
    i32 lines = 0;
    i16 t = 0;
    i16 lineEnd;
    i16 lineStart;
    char* cursor;
    char breakChar;

    lineStart = 0;
    lineEnd = 0;
    thePos = 0;
    widthUsed = 0;
    cursor = text;
    while (thePos < textLen && cursor[thePos] != 0) {
        while (cursor[thePos] != 0 && cursor[thePos] != '\n' && widthUsed <= maxWidth) {
            baseGlyph = static_cast<u8>(cursor[thePos]);
            if (baseGlyph < ' '
                || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = 0x7f;
            else if (baseGlyph > 0x7f)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            widthUsed += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            thePos++;
        }
        if (widthUsed > maxWidth) {
            thePos--;
            while (cursor[thePos] != ' ' && thePos >= lineStart) {
                baseGlyph = static_cast<u8>(cursor[thePos]);
                if (baseGlyph < ' '
                    || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                        && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                    baseGlyph = 0x7f;
                else if (baseGlyph > 0x7f)
                    baseGlyph = RemapCyrillicCharacter(baseGlyph);
                baseGlyph -= ' ';
                widthUsed -= widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                             + FONT_GLYPH_ADVANCE_SPACING;
                thePos--;
            }
            if (cursor[thePos] == ' ')
                widthUsed -= widths[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = thePos;
        lines++;
        lineStart = lineEnd + 1;
        thePos = lineStart;
        widthUsed = 0;
    }
    return lines;
}

VA(0x004725eb, 0x133)
i32 font::LineWidth(char* text) {
    i32 spare;
    i32 baseGlyph;
    i16* widths;
    i16 textLen;
    i32 oldSpare;
    i16 newSpare, mySpare, savedSpare, position, widthUsed;
    char* p;

    textLen = strlen(text);
    widths = m_glyphIcon->m_frameWords;
    oldSpare = 0;
    newSpare = 0;
    mySpare = 0;
    savedSpare = 0;
    position = 0;
    widthUsed = 0;
    p = text;
    while (position < textLen && p[position] != 0) {
        while (p[position] != 0 && p[position] != '\n') {
            baseGlyph = static_cast<u8>(p[position]);
            if (baseGlyph < ' '
                || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = 0x7f;
            else if (baseGlyph > 0x7f)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            widthUsed += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
    }
    return widthUsed;
}

VA_COMPGEN(0x00472760, 0x2e, "??_Gfont@@UAEPAXI@Z", 0x00471dd0)
