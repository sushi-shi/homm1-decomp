// Retail-backed dimmer widget resource reader.

#include <match.h>

#include <BASE/dimmerWidget.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

VA(0x00476ae0, 0x2b)
dimmerWidget::dimmerWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {}

dimmerWidget::~dimmerWidget(void) {}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00476b0b, 0x3f)
dimmerWidget::dimmerWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind)
    : widget(x, y, width, height, id, kind) {}

VA(0x00476b4a, 0x77)
void dimmerWidget::Read(void) {
    READ_WIDGET_GEOMETRY(this, gResourceManager);
    m_id = gResourceManager->ReadWord();
    m_kind = gResourceManager->ReadWord();
}

VA(0x00476bc1, 0x19)
H1_ENUM_RETURN(MessageDispatchResult, i16) dimmerWidget::Main(tag_message& message) {
    return widget::Main(message);
}

VA(0x00476bda, 0x13)
void dimmerWidget::Draw(void) {
    Dim();
}

VA_COMPGEN(0x00476c30, 0x2e, "??_GdimmerWidget@@UAEPAXI@Z", 0x00476ae0)
VA_COMPGEN(0x00476c60, 0x1c, "??1dimmerWidget@@UAE@XZ", 0x00476ae0)
