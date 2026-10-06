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
    // The glyph icon's name fills its field; the game treats it as a C
    // string. Every character is drawn as a frame of the icon.
    name[RESOURCE_NAME_CAPACITY - 1] = '\0';
    gLoadingMonoIcon = true;
    m_glyphIcon = gResourceManager->GetIcon(name);
    gLoadingMonoIcon = false;
    if (m_glyphIcon->m_frameCount <= FONT_GLYPH_INDEX_LAST)
        gResourceManager->InvalidResource(id);
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

// The glyph frame of a Windows-1251 character. Guillemets, the em dash and
// the numero sign are drawn only by fonts that carry their glyphs; other
// fonts show an ASCII look-alike. Characters without a glyph show the 0x7f glyph.
i32 font::GlyphFrame(i32 character) {
    i32 frame;
    i32 lookAlike;

    switch (character) {
        case FONT_CHAR_LEFT_GUILLEMET:
            frame = FONT_FRAME_LEFT_GUILLEMET;
            lookAlike = '"';
            break;
        case FONT_CHAR_RIGHT_GUILLEMET:
            frame = FONT_FRAME_RIGHT_GUILLEMET;
            lookAlike = '"';
            break;
        case FONT_CHAR_EM_DASH:
            frame = FONT_FRAME_EM_DASH;
            lookAlike = '-';
            break;
        case FONT_CHAR_NUMERO:
            frame = FONT_FRAME_NUMERO;
            lookAlike = 'N';
            break;
        default:
            frame = -1;
            lookAlike = character;
            break;
    }
    if (frame >= 0 && frame < m_glyphIcon->m_frameCount)
        return frame;
    character = lookAlike;
    if (character < ' '
        || (character > FONT_CODE_ASCII_LAST && character < CYRILLIC_CAPITAL_A
            && character != CYRILLIC_SMALL_YO && character != CYRILLIC_CAPITAL_YO))
        character = FONT_CODE_UNKNOWN;
    else if (character > FONT_CODE_ASCII_LAST)
        character = RemapCyrillicCharacter(character);
    return character - ' ';
}

// Whether a line that overflows at `position` has a space to break at,
// searching back from it to the line's start.
static i32 BreakAtSpace(const char* text, i16 lineStart, i16 position) {
    for (; position >= lineStart && position >= 0; position--) {
        if (text[position] == ' ')
            return 1;
    }
    return 0;
}

void font::DrawString(char* text, i16 x, i16 y, i16 color) {
    i16* entries = m_glyphIcon->m_frameWords;
    i32 glyph = 0;
    i16 drawX = x;
    i16 index = 0;
    while (text[index] != '\0') {
        glyph = GlyphFrame(static_cast<u8>(text[index]));
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
    i16 nextStart;

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
            baseGlyph = GlyphFrame(static_cast<u8>(textCopy[position]));
            widthUsed += frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
        nextStart = -1;
        if (widthUsed > width) {
            position--;
            if (!BreakAtSpace(textCopy, lineStart, position)) {
                // No space on the line: the word is broken before the glyph
                // that overflows, or after its first glyph. The original
                // searched before the line for a space and, on the first
                // line, wrote before the text; it drew the line again and
                // again.
                if (position > lineStart) {
                    // The overflowing glyph goes to the next line: the
                    // alignment is that of the glyphs before it.
                    baseGlyph = GlyphFrame(static_cast<u8>(textCopy[position]));
                    widthUsed -=
                        frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                        + FONT_GLYPH_ADVANCE_SPACING;
                }
                position = position > lineStart ? position : lineStart + 1;
                nextStart = position;
            }
            while (nextStart < 0 && textCopy[position] != ' ' && position >= lineStart) {
                baseGlyph = GlyphFrame(static_cast<u8>(textCopy[position]));
                widthUsed -=
                    frameDirectory[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                    + FONT_GLYPH_ADVANCE_SPACING;
                position--;
            }
            if (nextStart < 0 && textCopy[position] == ' ')
                widthUsed -= frameDirectory[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = position;
        breakChar = textCopy[lineEnd];
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
        textCopy[lineEnd] = breakChar;
        lineTop += m_height;
        lineStart = nextStart >= 0 ? nextStart : lineEnd + 1;
        position = lineStart;
        widthUsed = 0;
    }
    delete[] textCopy;
}

i32 font::LineLength(char* text, i16 maxWidth) {
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
    i16 nextStart;

    lineStart = 0;
    lineEnd = 0;
    position = 0;
    widthUsed = 0;
    chars = text;
    while (position < textLen && chars[position] != '\0') {
        while (chars[position] != '\0' && chars[position] != '\n' && widthUsed <= maxWidth) {
            baseGlyph = GlyphFrame(static_cast<u8>(chars[position]));
            widthUsed += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
        nextStart = -1;
        if (widthUsed > maxWidth) {
            position--;
            if (!BreakAtSpace(chars, lineStart, position)) {
                // As in DrawBoundedString; the original counted lines here
                // forever.
                position = position > lineStart ? position : lineStart + 1;
                nextStart = position;
            }
            while (nextStart < 0 && chars[position] != ' ' && position >= lineStart) {
                baseGlyph = GlyphFrame(static_cast<u8>(chars[position]));
                widthUsed -= widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                             + FONT_GLYPH_ADVANCE_SPACING;
                position--;
            }
            if (nextStart < 0 && chars[position] == ' ')
                widthUsed -= widths[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = position;
        lines++;
        lineStart = nextStart >= 0 ? nextStart : lineEnd + 1;
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
            baseGlyph = GlyphFrame(static_cast<u8>(chars[position]));
            widthUsed += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
        // A line break ends the inner scan; the original stopped on it
        // forever. The widths of all lines are summed, as before.
        if (chars[position] == '\n')
            position++;
    }
    return widthUsed;
}
