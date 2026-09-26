// Retail-backed text-entry widget resource reader.

#include <match.h>

#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/resourceManager.h>
#include <BASE/textEntryWidget.h>
#include <H1/KB.h>

#include <stdlib.h>
#include <string.h>

VA(0x0047e100, 0x2d)
textEntryWidget::textEntryWidget(void)
    : textWidget()
{
    m_cursorPosition = 0;
    m_icon = 0;
    m_kind = 0x4000;
    m_maxLength = 0;
    m_iconFrame = 0;
    m_displayOffset = 0;
}

VA_COMPGEN(0x0047e130, 0x36, "??_GtextEntryWidget@@UAEPAXI@Z", 0x0047e100)
textEntryWidget::~textEntryWidget(void)
{
    gpResourceManager->Dispose(m_icon);
}

VA(0x0047e170, 0x1e8)
void textEntryWidget::Read(int type)
{
    signed char name[13];
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    m_maxLength = gpResourceManager->ReadWord();
    m_text = static_cast<char *>(malloc(m_maxLength + 5));
    gpResourceManager->ReadBlock(reinterpret_cast<signed char *>(m_text), m_maxLength); // byte-evidenced: ReadBlock accepts signed bytes for text storage.
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_font = gpResourceManager->GetFont(reinterpret_cast<char *>(name)); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_color = gpResourceManager->ReadWord() & 0xff;
    m_alignment = static_cast<char>(gpResourceManager->ReadWord());
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(reinterpret_cast<char *>(name)); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_entryType = type;
    if (type == TEXT_ENTRY_READ_RECT) {
        m_rectX = gpResourceManager->ReadWord();
        m_rectY = gpResourceManager->ReadWord();
        m_rectW = gpResourceManager->ReadWord();
        m_rectH = gpResourceManager->ReadWord();
        m_maxLines = gpResourceManager->ReadWord();
        m_preserveTextOnFocus = gpResourceManager->ReadWord();
    } else {
        m_rectX = m_x;
        m_rectY = m_y;
        m_rectW = m_width;
        m_rectH = m_height;
        m_maxLines = 1;
        if (type == TEXT_ENTRY_READ_MULTILINE)
            m_preserveTextOnFocus = 1;
        else
            m_preserveTextOnFocus = 0;
    }
    m_iconFrame = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    gpResourceManager->ReadWord();
    m_kind = 0x4000;
}

VA(0x0047ec90, 0x182)
void textEntryWidget::SetupDisplayString(char *source, unsigned short cursor)
{
    int changed;
    char display[TEXT_ENTRY_DISPLAY_CAPACITY];
    if (cursor > 0)
        strncpy(m_text, source, cursor);
    m_text[cursor] = '_';
    if (strlen(source) > cursor)
        strcpy(m_text + cursor + 1, source + cursor);
    else
        m_text[cursor + 1] = 0;
    if (m_entryType == TEXT_ENTRY_READ_MULTILINE) {
        changed = 1;
        while (changed) {
            changed = 0;
            strcpy(display, m_text + m_displayOffset);
            if (m_font->LineWidth(display) > m_width) {
                display[cursor - m_displayOffset + 1] = 0;
                if (m_font->LineWidth(display) > m_width) {
                    m_displayOffset++;
                    changed = 1;
                }
            }
        }
        if (m_displayOffset > 0) {
            changed = 1;
            while (changed) {
                changed = 0;
                strcpy(display, m_text + m_displayOffset - 1);
                if (m_font->LineWidth(display) <= m_width)
                    m_displayOffset--;
                else
                    changed = 0;
                if (m_displayOffset == 0)
                    changed = 0;
            }
        }
    }
}
