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
    gResourceManager->PointToFile(id);
    m_height = gResourceManager->ReadWord();
    m_headerWord = gResourceManager->ReadWord();
    gResourceManager->Read13(name);
    gLoadingMonoIcon = true;
    m_glyphIcon = gResourceManager->GetIcon(name);
    gLoadingMonoIcon = false;
}

VA(0x00471e97, 0x5b)
font::~font(void) {
    gResourceManager->Dispose(m_glyphIcon);
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
    char* str,
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    i16 color,
    H1_ENUM_PARAM(FontAlignment, i16) align
) {
    i16 textLen;
    i32 baseGlyph;
    i16* theWidths;
    char spaceCharValue;
    i16 startIdx;
    i16 lineEnd;
    i16 drawColor;
    i16 bestDrawX;
    i16 curPosIdx;
    i16 tempWidth;
    i16 u;
    char* myText;
    char v;

    textLen = strlen(str);
    theWidths = m_glyphIcon->m_frameWords;
    spaceCharValue = ' ';
    bestDrawX = 0;
    u = 0;
    startIdx = 0;
    lineEnd = 0;
    curPosIdx = 0;
    tempWidth = 0;
    myText = new char[textLen + 1];
    strcpy(myText, str);
    drawColor = color;
    while (curPosIdx < textLen && myText[curPosIdx] != 0 && u + m_height <= height) {
        while (myText[curPosIdx] != 0 && myText[curPosIdx] != '\n' && tempWidth <= width) {
            baseGlyph = static_cast<u8>(myText[curPosIdx]);
            if (baseGlyph < ' '
                || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = 0x7f;
            else if (baseGlyph > 0x7f)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            tempWidth += theWidths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            curPosIdx++;
        }
        if (tempWidth > width) {
            curPosIdx--;
            while (myText[curPosIdx] != ' ' && curPosIdx >= startIdx) {
                baseGlyph = static_cast<u8>(myText[curPosIdx]);
                if (baseGlyph < ' '
                    || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                        && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                    baseGlyph = 0x7f;
                else if (baseGlyph > 0x7f)
                    baseGlyph = RemapCyrillicCharacter(baseGlyph);
                baseGlyph -= ' ';
                tempWidth -= theWidths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                             + FONT_GLYPH_ADVANCE_SPACING;
                curPosIdx--;
            }
            if (myText[curPosIdx] == ' ')
                tempWidth -= theWidths[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
        }
        lineEnd = curPosIdx;
        v = myText[lineEnd];
        myText[lineEnd] = 0;
        switch (align) {
            case FONT_ALIGN_LEFT:
                bestDrawX = 0;
                break;
            case FONT_ALIGN_CENTER:
                bestDrawX = (width - tempWidth) / 2;
                break;
            case FONT_ALIGN_RIGHT:
                bestDrawX = width - tempWidth;
                break;
        }
        DrawString(myText + startIdx, bestDrawX + x, u + y, drawColor);
        myText[lineEnd] = v;
        u += m_height;
        startIdx = lineEnd + 1;
        curPosIdx = startIdx;
        tempWidth = 0;
    }
    delete[] myText;
}

VA(0x0047238c, 0x25f)
i32 font::LineLength(char* str, i16 maxW) {
    i16 lw;
    i16 thePos;
    i16 theLen = strlen(str);
    i32 baseGlyph;
    i16* widths = m_glyphIcon->m_frameWords;
    char charVal = ' ';
    i32 z = 0;
    i16 t = 0;
    i16 curLineEnd;
    i16 mainStart;
    char* cursor;
    char v;

    mainStart = 0;
    curLineEnd = 0;
    thePos = 0;
    lw = 0;
    cursor = str;
    while (thePos < theLen && cursor[thePos] != 0) {
        while (cursor[thePos] != 0 && cursor[thePos] != '\n' && lw <= maxW) {
            baseGlyph = static_cast<u8>(cursor[thePos]);
            if (baseGlyph < ' '
                || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                    && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                baseGlyph = 0x7f;
            else if (baseGlyph > 0x7f)
                baseGlyph = RemapCyrillicCharacter(baseGlyph);
            baseGlyph -= ' ';
            lw += widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                  + FONT_GLYPH_ADVANCE_SPACING;
            thePos++;
        }
        if (lw > maxW) {
            thePos--;
            while (cursor[thePos] != ' ' && thePos >= mainStart) {
                baseGlyph = static_cast<u8>(cursor[thePos]);
                if (baseGlyph < ' '
                    || (baseGlyph > 0x7f && baseGlyph < CYRILLIC_CAPITAL_A
                        && baseGlyph != CYRILLIC_SMALL_YO && baseGlyph != CYRILLIC_CAPITAL_YO))
                    baseGlyph = 0x7f;
                else if (baseGlyph > 0x7f)
                    baseGlyph = RemapCyrillicCharacter(baseGlyph);
                baseGlyph -= ' ';
                lw -= widths[baseGlyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                      + FONT_GLYPH_ADVANCE_SPACING;
                thePos--;
            }
            if (cursor[thePos] == ' ')
                lw -= widths[FONT_GLYPH_WIDTH_WORD] + FONT_GLYPH_ADVANCE_SPACING;
        }
        curLineEnd = thePos;
        z++;
        mainStart = curLineEnd + 1;
        thePos = mainStart;
        lw = 0;
    }
    return z;
}

VA(0x004725eb, 0x133)
i32 font::LineWidth(char* text) {
    i32 curCh;
    i32 spare;
    i16* table;
    b32 oldSpare;
    i16 theLen;
    i16 newSpare, mySpare, savedSpare, position, thisWidth;
    char* p;

    theLen = strlen(text);
    table = m_glyphIcon->m_frameWords;
    oldSpare = false;
    newSpare = 0;
    mySpare = 0;
    savedSpare = 0;
    position = 0;
    thisWidth = 0;
    p = text;
    while (position < theLen && p[position] != 0) {
        while (p[position] != 0 && p[position] != '\n') {
            curCh = static_cast<u8>(p[position]);
            if (curCh < ' '
                || (curCh > 0x7f && curCh < CYRILLIC_CAPITAL_A && curCh != CYRILLIC_SMALL_YO
                    && curCh != CYRILLIC_CAPITAL_YO))
                curCh = 0x7f;
            else if (curCh > 0x7f)
                curCh = RemapCyrillicCharacter(curCh);
            curCh -= ' ';
            thisWidth += table[curCh * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                         + FONT_GLYPH_ADVANCE_SPACING;
            position++;
        }
    }
    return thisWidth;
}

VA_COMPGEN(0x00472760, 0x2e, "??_Gfont@@UAEPAXI@Z", 0x00471dd0)
