// Retail-backed icon widget resource reader.

#include <match.h>

#include <BASE/display.h>
#include <BASE/heroWindow.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/message.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

VA(0x0046deb0, 0x4e)
iconWidget::iconWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_icon = NULL;
    m_frame = 0;
    m_fillColor = 0;
    m_orientation = ICON_DRAW_NORMAL;
}

// The icon-file-id overload; no retail caller survives.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046defe, 0xa2)
iconWidget::iconWidget(
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    i16 iconId,
    i8 frame,
    i8 orientation,
    i16 id,
    i16 kind,
    i16 fillColor
)
    : widget(x, y, width, height, id, kind) {
    m_icon = gpResourceManager->GetIcon(iconId);
    m_frame = frame;
    m_fillColor = fillColor;
    m_orientation = orientation;
}

// Retail reads the frame argument as a signed byte before widening it.
VA(0x0046dfa0, 0xa1)
iconWidget::iconWidget(
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    char* iconName,
    i8 frame,
    i8 orientation,
    i16 id,
    i16 kind,
    i16 fillColor
)
    : widget(x, y, width, height, id, kind) {
    m_icon = gpResourceManager->GetIcon(iconName);
    m_frame = frame;
    m_fillColor = fillColor;
    m_orientation = orientation;
}

VA(0x0046e041, 0xf9)
void iconWidget::Read(void) {
    char iconName[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    gpResourceManager->Read13(iconName);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(iconName);
    gpResourceManager->RestorePosition();
    m_frame = gpResourceManager->ReadWord();
    m_orientation = gpResourceManager->ReadWord() & ICON_WIDGET_ORIENTATION_MASK;
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
    m_fillColor = gpResourceManager->ReadWord() & COLOR_INDEX_MASK;
}

VA(0x0046e13a, 0x5b)
iconWidget::~iconWidget(void) {
    gpResourceManager->Dispose(m_icon);
}

VA(0x0046e195, 0x2a4)
i16 iconWidget::Main(tag_message& message) {
    i16 x;
    i16 y;
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_SET_ICON:
                    if (message.id == m_id) {
                        if (m_icon != NULL) {
                            gpResourceManager->Dispose(m_icon);
                            m_icon = gpResourceManager->GetIcon(message.text);
                        }
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_SET_FRAME:
                    if (message.id == m_id) {
                        i16 frame = static_cast<i16>(message.value & 0xffff);
                        m_frame = frame;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_SET_COLOR:
                    if (message.id == m_id) {
                        m_fillColor = message.value & COLOR_INDEX_MASK;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
            }
            break;
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN: {
            x = message.x - m_owner->m_posX;
            y = message.y - m_owner->m_posY;
            if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                m_flags |= WIDGET_FLAG_SELECTED;
                if (message.type == MESSAGE_RIGHT_BUTTON_DOWN)
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_SELECT, m_id);
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
        case MESSAGE_LEFT_BUTTON_UP:
        case MESSAGE_RIGHT_BUTTON_UP:
            if (m_flags & WIDGET_FLAG_SELECTED) {
                m_flags &= ~WIDGET_FLAG_SELECTED;
                SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_DESELECT, m_id);
                // Retail tests the type after SET_WIDGET_MESSAGE replaces it.
                if (message.type == MESSAGE_RIGHT_BUTTON_UP)
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
    }
    return widget::Main(message);
}

VA(0x0046e439, 0xb1)
void iconWidget::Draw(void) {
    i16 x = m_owner->m_posX + m_x;
    i16 y = m_owner->m_posY + m_y;
    switch (m_kind) {
        case ICON_WIDGET_DRAW:
            PollSound();
            m_icon->DrawToBuffer(x, y, m_frame, m_orientation, ICON_DRAW_OFFSET_FULL);
            break;
        case ICON_WIDGET_FILL:
            m_icon->FillToBuffer(x, y, m_frame, m_fillColor, m_orientation, ICON_DRAW_OFFSET_FULL);
            break;
    }
}

VA_COMPGEN(0x0046e530, 0x2e, "??_GiconWidget@@UAEPAXI@Z", 0x0046deb0)
