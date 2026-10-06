#include <H1/Ints.h>

#include <BASE/backdropWidget.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

backdropWidget::backdropWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

backdropWidget::~backdropWidget(void) {}

backdropWidget::backdropWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind)
    : widget(x, y, width, height, id, kind) {}

void backdropWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

i16 backdropWidget::Main(tag_message& message) {
    return widget::Main(message);
}

void backdropWidget::Draw(void) {
    gpWindowManager
        ->UpdateScreenRegion(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_width, m_height);
}
