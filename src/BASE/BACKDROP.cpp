// Retail-backed backdrop widget resource reader.

#include <match.h>

#include <BASE/backdropWidget.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

VA(0x0047cfe0, 0x1e)
backdropWidget::backdropWidget(void) : widget(0, 0, 0, 0, 0, 0) {}

VA_COMPGEN(0x0047d000, 0x25, "??_GbackdropWidget@@UAEPAXI@Z", 0x0047cfe0)
backdropWidget::~backdropWidget(void) {}

VA(0x0047d030, 0x34)
backdropWidget::backdropWidget(short x, short y, short width, short height, short id, short kind)
    : widget(x, y, width, height, id, kind) {}

VA(0x0047d070, 0x5f)
void backdropWidget::Read(void) {
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

VA(0x0047d0d0, 0xd)
short backdropWidget::Main(tag_message& message) {
    return widget::Main(message);
}

VA(0x0047d0e0, 0x2d)
void backdropWidget::Draw(void) {
    gpWindowManager
        ->UpdateScreenRegion(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_width, m_height);
}
