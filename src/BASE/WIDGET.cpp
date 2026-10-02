// Located from HoMM2 Buka 2.1; HoMM1 retains the 16-bit widget layout.

#include <match.h>

#include <BASE/BMAP2.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/widget.h>
#include <H1/KB.h>

VA(0x0047f670, 0x5a)
widget::widget(short x, short y, short width, short height, short id, short kind) {
    m_owner = 0;
    m_next = 0;
    m_prev = 0;
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    m_id = id;
    m_flags = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_zOrder = WINDOW_Z_ORDER_APPEND;
    m_kind = kind;
}

VA(0x0047f6d0, 0x7)
widget::~widget(void) {}

VA(0x0047f6e0, 0x16)
short widget::Open(short zOrder, heroWindow* owner) {
    m_zOrder = zOrder;
    m_owner = owner;
    return 0;
}

VA(0x0047f700, 0x1)
void widget::Close(void) {}

VA(0x0047f710, 0x216)
short widget::Main(tag_message& message) {
    short x;
    short y;
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
                        if (message.value == WIDGET_COMMAND_DIMMED) {
                            m_flags |= WIDGET_FLAG_DIMMED;
                            return MESSAGE_DISPATCH_CONSUME;
                        }
                        m_flags |= message.value;
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
                        short flags = message.value;
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
            if (x >= m_x && y >= m_y && x < m_x + m_width && y < m_y + m_height) {
                message.type = MESSAGE_WIDGET;
                message.command = WIDGET_COMMAND_HOVER;
                message.id = m_id;
                return MESSAGE_DISPATCH_FORWARD;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONTINUE;
}

VA(0x0047f930, 0x3a)
void widget::Dim(void) {
    short x = m_owner->m_posX + m_x;
    short y = m_owner->m_posY + m_y;
    DimBitmapArea(gpWindowManager->m_screen, x, y, m_width, m_height);
}
