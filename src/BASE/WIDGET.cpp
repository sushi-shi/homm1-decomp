#include <match.h>

#include <BASE/BMAP2.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/widget.h>
#include <SOURCE/KB.h>

VA(0x00475370, 0x8b)
widget::widget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind) {
    m_owner = NULL;
    m_next = NULL;
    m_prev = NULL;
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    m_id = id;
    m_flags = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_kind = kind;
}

// The default widget: a 16-pixel enabled, drawn widget.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x004753fb, 0x7d)
widget::widget(void) {
    m_owner = NULL;
    m_next = NULL;
    m_prev = NULL;
    m_id = 0;
    m_flags = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_kind = WIDGET_KIND_DEFAULT;
    m_y = 0;
    m_x = 0;
    m_width = WIDGET_DEFAULT_EXTENT;
    m_height = WIDGET_DEFAULT_EXTENT;
}

VA(0x00475478, 0x14)
widget::~widget(void) {}

VA(0x0047548c, 0x24)
i16 widget::Open(i16 zOrder, heroWindow* owner) {
    m_zOrder = zOrder;
    m_owner = owner;
    return 0;
}

VA(0x004754b0, 0xb)
void widget::Close(void) {}

VA(0x004754bb, 0x2c8)
H1_ENUM_RETURN(MessageDispatchResult, i16) widget::Main(tag_message& message) {
    i16 x;
    i16 y;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_DRAW:
                    if (m_flags & WIDGET_FLAG_DRAW)
                        Draw();
                    if (m_flags & WIDGET_FLAG_DIMMED)
                        Dim();
                    break;
                case WIDGET_COMMAND_SET_FLAGS:
                    if (message.id == m_id) {
                        if (message.value == WIDGET_FLAG_DIM_REQUEST) {
                            m_flags |= WIDGET_FLAG_DIMMED;
                            return MESSAGE_DISPATCH_CONSUME;
                        }
                        m_flags |= message.value & WIDGET_FLAG_MASK;
                        if (m_flags & WIDGET_FLAG_DIMMED) {
                            Draw();
                            Dim();
                        }
                        if (m_flags & WIDGET_FLAG_UPDATE) {
                            gpWindowManager->UpdateScreenRegion(
                                m_owner->m_posX + m_x,
                                m_owner->m_posY + m_y,
                                m_width,
                                m_height
                            );
                            m_flags &= ~WIDGET_FLAG_UPDATE;
                        }
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_CLEAR_FLAGS:
                    if (message.id == m_id) {
                        i16 flags = message.value & WIDGET_FLAG_MASK;
                        m_flags &= ~flags;
                        if (flags & WIDGET_FLAG_DIMMED)
                            Draw();
                        if (flags & WIDGET_FLAG_UPDATE)
                            gpWindowManager->UpdateScreenRegion(
                                m_owner->m_posX + m_x,
                                m_owner->m_posY + m_y,
                                m_width,
                                m_height
                            );
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
            }
            break;
        case MESSAGE_MOUSE_MOVE:
            x = message.x - m_owner->m_posX;
            y = message.y - m_owner->m_posY;
            if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_HOVER, m_id);
                return MESSAGE_DISPATCH_FORWARD;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}

VA(0x00475783, 0x67)
void widget::Dim(void) {
    i16 x = m_owner->m_posX + m_x;
    i16 y = m_owner->m_posY + m_y;
    DimBitmapArea(gpWindowManager->m_screen, x, y, m_width, m_height);
}
