// Retail-backed icon widget resource reader.

#include <match.h>

#include <BASE/heroWindow.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/message.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

VA(0x0047a9f0, 0x2a)
iconWidget::iconWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_frame = 0;
    m_icon = 0;
    m_fillColor = 0;
    m_orientation = ICON_DRAW_NORMAL;
}

VA_COMPGEN(0x0047aa20, 0x36, "??_GiconWidget@@UAEPAXI@Z", 0x0047a9f0)
iconWidget::~iconWidget(void) {
    gpResourceManager->Dispose(m_icon);
}

// Retail reads the frame argument as a signed byte before widening it.
VA(0x0047aa60, 0x61)
iconWidget::iconWidget(
    short x,
    short y,
    short width,
    short height,
    char* name,
    signed char frame,
    signed char orientation,
    short id,
    short kind,
    short fillColor
)
    : widget(x, y, width, height, id, kind) {
    m_icon = gpResourceManager->GetIcon(name);
    m_frame = frame;
    m_fillColor = fillColor;
    m_orientation = orientation;
}

VA(0x0047aad0, 0xce)
void iconWidget::Read(void) {
    signed char name[RESOURCE_NAME_CAPACITY];
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(
        reinterpret_cast<char*>(name)
    ); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_frame = gpResourceManager->ReadWord();
    m_orientation = static_cast<signed char>(gpResourceManager->ReadWord());
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
    m_fillColor = gpResourceManager->ReadWord() & 0xff;
}

VA(0x0047aba0, 0x1bc)
short iconWidget::Main(tag_message& message) {
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN: {
            short x = message.x - m_owner->m_posX;
            short y = message.y - m_owner->m_posY;
            if (x >= m_x && y >= m_y && x < m_x + m_width && y < m_y + m_height) {
                m_flags |= WIDGET_FLAG_SELECTED;
                if (message.type == MESSAGE_RIGHT_BUTTON_DOWN)
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                message.type = MESSAGE_WIDGET;
                message.command = WIDGET_NOTIFY_SELECT;
                message.id = m_id;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
        case MESSAGE_LEFT_BUTTON_UP:
        case MESSAGE_RIGHT_BUTTON_UP:
            if (m_flags & WIDGET_FLAG_SELECTED) {
                m_flags &= ~WIDGET_FLAG_SELECTED;
                message.type = MESSAGE_WIDGET;
                message.command = WIDGET_NOTIFY_DESELECT;
                message.id = m_id;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_SET_FRAME:
                    if (m_id == message.id) {
                        m_frame = message.value;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_SET_COLOR:
                    if (m_id == message.id) {
                        m_fillColor = message.value & 0xff;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_SET_ICON:
                    if (m_id == message.id) {
                        if (m_icon != 0) {
                            gpResourceManager->Dispose(m_icon);
                            m_icon = gpResourceManager->GetIcon(message.text);
                        }
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
            }
            break;
    }
    return widget::Main(message);
}

VA(0x0047ad60, 0x68)
void iconWidget::Draw(void) {
    short x = m_owner->m_posX + m_x;
    short y = m_owner->m_posY + m_y;
    switch (m_kind) {
        case ICON_WIDGET_DRAW:
            PollSound();
            m_icon->DrawToBuffer(x, y, m_frame, m_orientation, 0);
            break;
        case ICON_WIDGET_FILL:
            m_icon->FillToBuffer(x, y, m_frame, m_fillColor, m_orientation, 0);
            break;
    }
}
