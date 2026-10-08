#include <H1/Ints.h>

#include <BASE/dimmerWidget.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

dimmerWidget::dimmerWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

dimmerWidget::~dimmerWidget(void) {}

dimmerWidget::dimmerWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind)
    : widget(x, y, width, height, id, kind) {}

void dimmerWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gResourceManager);
    m_id = gResourceManager->ReadWord();
    m_kind = gResourceManager->ReadWord();
}

i16 dimmerWidget::Main(tag_message& message) {
    return widget::Main(message);
}

void dimmerWidget::Draw(void) {
    Dim();
}
