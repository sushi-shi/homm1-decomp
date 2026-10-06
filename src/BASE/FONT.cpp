#include <H1/Ints.h>

#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <string.h>

font::font(i16 id) : resource(RESOURCE_CATEGORY_FONT, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    char name[RESOURCE_NAME_CAPACITY];
    gResourceManager->PointToFile(id);
    m_height = gResourceManager->ReadWord();
    m_glyphOffsetY = gResourceManager->ReadWord();
    gResourceManager->Read13(name);
    gLoadingMonoIcon = true;
    m_glyphIcon = gResourceManager->GetIcon(name);
    gLoadingMonoIcon = false;
}

font::~font(void) {
    gResourceManager->Dispose(m_glyphIcon);
}

i32 RemapCyrillicCharacter(i32 character) {
    if (character == CYRILLIC_CAPITAL_YO)
        return FONT_CODE_CAPITAL_YO;
    if (character == CYRILLIC_SMALL_YO)
        return FONT_CODE_SMALL_YO;
    if (character < CYRILLIC_CAPITAL_A)
        return FONT_CODE_SMALL_A;
    if (character < CYRILLIC_SMALL_A)
        return character - (CYRILLIC_CAPITAL_A - FONT_CODE_CAPITAL_A);
    return character - (CYRILLIC_SMALL_A - FONT_CODE_SMALL_A);
}

void font::DrawString(char* text, i16 x, i16 y, i16 color) {
    i16* entries = m_glyphIcon->m_frameWords;
    i32 glyph = 0;
    i16 drawX = x;
    i16 index = 0;
    while (text[index] != '\0') {
        glyph = static_cast<u8>(text[index]);
        if (glyph < ' '
            || (glyph > FONT_CODE_ASCII_LAST && glyph < CYRILLIC_CAPITAL_A
                && glyph != CYRILLIC_SMALL_YO && glyph != CYRILLIC_CAPITAL_YO))
            glyph = FONT_CODE_UNKNOWN;
        else if (glyph > FONT_CODE_ASCII_LAST)
            glyph = RemapCyrillicCharacter(glyph);
        glyph -= ' ';
        if (glyph != FONT_GLYPH_SPACE)
            m_glyphIcon->FillToBuffer(
                drawX,
                y + m_glyphOffsetY,
                glyph,
                color,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        drawX += entries[glyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                 + FONT_GLYPH_ADVANCE_SPACING;
        index++;
    }
}

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
    i16 overflow;
    i16 fittingWidth;
    i16 hardBreak;

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
    while (position < textLen && textCopy[position] != '\0' && lineTop + m_height <= height) {
        while (textCopy[position] != '\0' && textCopy[position] != '\n' && widthUsed <= width) {
            baseGlyph = static_cast<u8>(textCopy[position]);
            if (baseGlyph < ' '
                || (baseGlyph > FONT_CODE_ASCII_LAST && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = FONT_CODE_UNKNOWN;
            else if (baseGlyph > FONT_CODE_ASCII_LAST)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            widthUsed += frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
        hardBreak = 0;
        if (widthUsed > width) {
            position--;
            overflow = position;
            fittingWidth = widthUsed;
            // A word wider than the line has no space to break at: the walk
            // back ends before the line (at -1 on the first line, where the
            // original read and wrote the byte before the copy, and then
            // drew the rest of the text on one line past the box, again on
            // every line). The word is broken where it overflows, as
            // LineLength counts it.
            while (position >= lineStart && textCopy[position] != ' ') {
                baseGlyph = static_cast<u8>(textCopy[position]);
                if (baseGlyph < ' '
                    || (baseGlyph > FONT_CODE_ASCII_LAST && baseGlyph < CYRILLIC_CAPITAL_A
                        && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                    baseGlyph = FONT_CODE_UNKNOWN;
                else if (baseGlyph > FONT_CODE_ASCII_LAST)
                    baseGlyph = RemapCyrillicCharacter(baseGlyph);
                baseGlyph -= ' ';
                widthUsed -=
                    frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                    + FONT_GLYPH_ADVANCE_SPACING;
                if (position == overflow && overflow > lineStart)
                    fittingWidth = widthUsed;
                position--;
            }
            if (position >= lineStart && textCopy[position] == ' ')
                widthUsed -= frameDirectory[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
            if (position < lineStart) {
                // Up to the overflowing character, or that one alone.
                hardBreak = 1;
                position = overflow > lineStart ? overflow : overflow + 1;
                widthUsed = fittingWidth;
            }
        }
        lineEnd = position;
        breakChar = lineEnd >= 0 ? textCopy[lineEnd] : '\0';
        if (lineEnd >= 0)
            textCopy[lineEnd] = '\0';
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
        if (lineEnd >= 0)
            textCopy[lineEnd] = breakChar;
        lineTop += m_height;
        lineStart = hardBreak ? lineEnd : lineEnd + 1;
        position = lineStart;
        widthUsed = 0;
    }
    delete[] textCopy;
}

i32 font::LineLength(char* text, i16 maxWidth) {
    i16 overflow;
    i16 widthUsed;
    i16 position;
    i16 textLen = strlen(text);
    i32 baseGlyph;
    i16* widths = m_glyphIcon->m_frameWords;
    char charVal = ' ';
    i32 lines = 0;
    i16 t = 0;
    i16 lineEnd;
    i16 lineStart;
    char* chars;
    char breakChar;

    lineStart = 0;
    lineEnd = 0;
    position = 0;
    widthUsed = 0;
    chars = text;
    while (position < textLen && chars[position] != '\0') {
        while (chars[position] != '\0' && chars[position] != '\n' && widthUsed <= maxWidth) {
            baseGlyph = static_cast<u8>(chars[position]);
            if (baseGlyph < ' '
                || (baseGlyph > FONT_CODE_ASCII_LAST && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = FONT_CODE_UNKNOWN;
            else if (baseGlyph > FONT_CODE_ASCII_LAST)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            widthUsed += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
        if (widthUsed > maxWidth) {
            position--;
            overflow = position;
            while (position >= lineStart && chars[position] != ' ') {
                baseGlyph = static_cast<u8>(chars[position]);
                if (baseGlyph < ' '
                    || (baseGlyph > FONT_CODE_ASCII_LAST && baseGlyph < CYRILLIC_CAPITAL_A
                        && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                    baseGlyph = FONT_CODE_UNKNOWN;
                else if (baseGlyph > FONT_CODE_ASCII_LAST)
                    baseGlyph = RemapCyrillicCharacter(baseGlyph);
                baseGlyph -= ' ';
                widthUsed -= widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                             + FONT_GLYPH_ADVANCE_SPACING;
                position--;
            }
            if (position >= lineStart && chars[position] == ' ')
                widthUsed -= widths[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
            // A word wider than the line has no space to break at. The
            // original then started the next line where this one started
            // and counted lines without end (typing a long word into a
            // multi-line field hung the program), reading the byte before
            // the text on the first line. The word is broken where it
            // overflows instead, at least one character per line.
            if (position < lineStart)
                position = overflow > lineStart ? overflow - 1 : lineStart;
        }
        lineEnd = position;
        lines++;
        lineStart = lineEnd + 1;
        position = lineStart;
        widthUsed = 0;
    }
    return lines;
}

i32 font::LineWidth(char* text) {
    i32 spare;
    i32 baseGlyph;
    i16* widths;
    i16 textLen;
    b32 oldSpare;
    i16 newSpare, mySpare, savedSpare, position, widthUsed;
    char* chars;

    textLen = strlen(text);
    widths = m_glyphIcon->m_frameWords;
    oldSpare = false;
    newSpare = 0;
    mySpare = 0;
    savedSpare = 0;
    position = 0;
    widthUsed = 0;
    chars = text;
    while (position < textLen && chars[position] != '\0') {
        while (chars[position] != '\0' && chars[position] != '\n') {
            baseGlyph = static_cast<u8>(chars[position]);
            if (baseGlyph < ' '
                || (baseGlyph > FONT_CODE_ASCII_LAST && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = FONT_CODE_UNKNOWN;
            else if (baseGlyph > FONT_CODE_ASCII_LAST)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            widthUsed += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
    }
    return widthUsed;
}
