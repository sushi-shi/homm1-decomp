// Retail-backed border widget resource reader.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/border.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

VA(0x00479780, 0x2b)
border::border(void)
    : widget(0, 0, 0, 0, 0, 0), m_background(0), m_fillColor(0)
{
}

VA_COMPGEN(0x004797b0, 0x3a, "??_Gborder@@UAEPAXI@Z", 0x00479780)
border::~border(void)
{
    if (m_background)
        gpResourceManager->Dispose(m_background);
}

VA(0x00479850, 0xc3)
void border::Read(void)
{
    signed char name[13];
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
    if (m_kind == BORDER_BACKGROUND_BITMAP) {
        gpResourceManager->Read13(name);
        gpResourceManager->SavePosition();
        m_background = gpResourceManager->GetBitmap(reinterpret_cast<char *>(name)); // byte-evidenced: resource name APIs use differently signed bytes.
        gpResourceManager->RestorePosition();
        return;
    }
    short color = gpResourceManager->ReadWord();
    m_background = 0;
    m_fillColor = color & 0xff;
}
