// Retail-backed text widget resource reader.

#include <match.h>

#include <BASE/font.h>
#include <BASE/resourceManager.h>
#include <BASE/textWidget.h>
#include <H1/KB.h>

#include <stdlib.h>
#include <string.h>

VA(0x0047add0, 0x3e)
VA_COMPGEN(0x0047ae10, 0x42, "??_GtextWidget@@UAEPAXI@Z", 0x0047add0)
textWidget::textWidget(void)
    : widget(0, 0, 0, 0, 0, 0)
{
    m_font = 0;
    m_text = 0;
    m_color = 1;
    m_alignment = 1;
    m_kind = 0x200;
}

VA(0x0047aed0, 0xeb)
void textWidget::Read(void)
{
    signed char name[13];
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    short length = gpResourceManager->ReadWord();
    m_text = static_cast<char *>(malloc(length));
    gpResourceManager->ReadBlock(reinterpret_cast<signed char *>(m_text), length); // byte-evidenced: ReadBlock accepts signed bytes for stored text.
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_font = gpResourceManager->GetFont(reinterpret_cast<char *>(name)); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_color = gpResourceManager->ReadWord() & 0xff;
    m_alignment = static_cast<char>(gpResourceManager->ReadWord());
    m_id = gpResourceManager->ReadWord();
    gpResourceManager->ReadWord();
    m_kind = 0x200;
}

textWidget::~textWidget(void)
{
    gpResourceManager->Dispose(m_font);
    free(m_text);
}

VA(0x0047b220, 0x96)
void textWidget::SetText(char *text)
{
    if (m_kind == WIDGET_KIND_TEXT || m_kind == WIDGET_KIND_TEXT_ENTRY) {
        unsigned short newLength = strlen(text);
        if (newLength > strlen(m_text)) {
            free(m_text);
            m_text = static_cast<char *>(malloc(newLength + 5));
        }
        strcpy(m_text, text);
    } else {
        m_text = text;
    }
}
