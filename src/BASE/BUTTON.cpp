// Retail-backed button widget resource reader.

#include <match.h>

#include <BASE/button.h>
#include <BASE/icon.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

VA(0x0047eef0, 0x31)
button::button(void)
    : widget(0, 0, 0, 0, 0, 0)
{
    m_normalFrame = 0;
    m_pressedFrame = 0;
    m_selectMode = 0;
    m_icon = 0;
    m_hotkey = -1;
}

VA_COMPGEN(0x0047ef30, 0x36, "??_Gbutton@@UAEPAXI@Z", 0x0047eef0)
button::~button(void)
{
    gpResourceManager->Dispose(m_icon);
}

VA(0x0047ef70, 0xda)
void button::Read(void)
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
    m_normalFrame = gpResourceManager->ReadWord();
    m_pressedFrame = gpResourceManager->ReadWord();
    m_selectMode = gpResourceManager->ReadWord();
    m_hotkey = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}
