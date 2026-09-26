// Retail-backed icon widget resource reader.

#include <match.h>

#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

VA(0x0047a9f0, 0x2a)
iconWidget::iconWidget(void)
    : widget(0, 0, 0, 0, 0, 0)
{
    m_frame = 0;
    m_icon = 0;
    m_fillColor = 0;
    m_orientation = 0;
}

VA_COMPGEN(0x0047aa20, 0x36, "??_GiconWidget@@UAEPAXI@Z", 0x0047a9f0)
iconWidget::~iconWidget(void)
{
    gpResourceManager->Dispose(m_icon);
}

VA(0x0047aad0, 0xce)
void iconWidget::Read(void)
{
    signed char name[13];
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(reinterpret_cast<char *>(name)); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_frame = gpResourceManager->ReadWord();
    m_orientation = static_cast<signed char>(gpResourceManager->ReadWord());
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
    m_fillColor = gpResourceManager->ReadWord() & 0xff;
}
