#include <H1/Ints.h>

#include <BASE/dimmerWidget.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

dimmerWidget::dimmerWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

void dimmerWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

i16 dimmerWidget::Main(tag_message& message) {
    return widget::Main(message);
}

void dimmerWidget::Draw(void) {
    Dim();
}
