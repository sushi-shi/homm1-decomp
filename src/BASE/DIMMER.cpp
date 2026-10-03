// Retail-backed dimmer widget resource reader.

#include <match.h>

#include <BASE/dimmerWidget.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

VA(0x0047ea80, 0x1e)
dimmerWidget::dimmerWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

VA_COMPGEN(0x0047eaa0, 0x25, "??_GdimmerWidget@@UAEPAXI@Z", 0x0047ea80)
dimmerWidget::~dimmerWidget(void) {}

VA(0x0047ead0, 0x5f)
void dimmerWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

VA(0x0047eb30, 0xd)
i16 dimmerWidget::Main(tag_message& message) {
    return widget::Main(message);
}

VA(0x0047eb40, 0x5)
void dimmerWidget::Draw(void) {
    Dim();
}
