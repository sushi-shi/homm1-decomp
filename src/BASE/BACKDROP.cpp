// Retail-backed backdrop widget resource reader.

#include <match.h>

#include <BASE/backdropWidget.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

VA(0x0046ce40, 0x2b)
backdropWidget::backdropWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

backdropWidget::~backdropWidget(void) {}

VA(0x0046ce6b, 0x3f)
backdropWidget::backdropWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind)
    : widget(x, y, width, height, id, kind) {}

VA(0x0046ceaa, 0x77)
void backdropWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gResourceManager);
    m_id = gResourceManager->ReadWord();
    m_kind = gResourceManager->ReadWord();
}

VA(0x0046cf21, 0x19)
i16 backdropWidget::Main(tag_message& message) {
    return widget::Main(message);
}

VA(0x0046cf3a, 0x4e)
void backdropWidget::Draw(void) {
    gWindowManager
        ->UpdateScreenRegion(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_width, m_height);
}

VA_COMPGEN(0x0046cfd0, 0x2e, "??_GbackdropWidget@@UAEPAXI@Z", 0x0046ce40)
VA_COMPGEN(0x0046d000, 0x1c, "??1backdropWidget@@UAE@XZ", 0x0046ce40)
