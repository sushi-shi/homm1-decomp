// Located from HoMM2 Buka 2.1; HoMM1 retains the 16-bit widget layout.

#include <match.h>

#include <BASE/heroWindow.h>
#include <BASE/widget.h>

VA(0x0047f670, 0x5a)
widget::widget(
    short x,
    short y,
    short width,
    short height,
    short id,
    short kind)
{
    m_owner = 0;
    m_next = 0;
    m_prev = 0;
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    m_id = id;
    m_flags = 6;
    m_zOrder = -1;
    m_kind = kind;
}

VA(0x0047f6d0, 0x7)
widget::~widget(void)
{
}

VA(0x0047f6e0, 0x16)
short widget::Open(short zOrder, heroWindow *owner)
{
    m_zOrder = zOrder;
    m_owner = owner;
    return 0;
}

VA(0x0047f700, 0x1)
void widget::Close(void)
{
}
