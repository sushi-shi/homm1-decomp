// Retail-backed dimmer widget resource reader.

#include <match.h>

#include <BASE/dimmerWidget.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

VA_COMPGEN(0x00476c60, 0x1c, "??1dimmerWidget@@UAE@XZ", 0x00476ae0)
VA(0x00476ae0, 0x2b)
dimmerWidget::dimmerWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

VA_COMPGEN(0x00476c30, 0x2e, "??_GdimmerWidget@@UAEPAXI@Z", 0x00476ae0)
dimmerWidget::~dimmerWidget(void) {}

VA(0x00476b4a, 0x77)
void dimmerWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

VA(0x00476bc1, 0x19)
i16 dimmerWidget::Main(tag_message& message) {
    return widget::Main(message);
}

VA(0x00476bda, 0x13)
void dimmerWidget::Draw(void) {
    Dim();
}
