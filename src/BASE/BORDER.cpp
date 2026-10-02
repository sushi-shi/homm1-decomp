// Retail-backed border widget resource reader.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/border.h>
#include <BASE/display.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

VA(0x00479780, 0x2b)
border::border(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE), m_background(0), m_fillColor(0) {}

VA_COMPGEN(0x004797b0, 0x3a, "??_Gborder@@UAEPAXI@Z", 0x00479780)
border::~border(void) {
    if (m_background)
        gpResourceManager->Dispose(m_background);
}

VA(0x004797f0, 0x5d)
border::border(
    short x,
    short y,
    short width,
    short height,
    short id,
    short kind,
    short fillColor,
    char* name
)
    : widget(x, y, width, height, id, kind) {
    if (name != 0)
        m_background = gpResourceManager->GetBitmap(name);
    else
        m_background = 0;
    m_fillColor = fillColor;
}

VA(0x00479850, 0xc3)
void border::Read(void) {
    signed char name[RESOURCE_NAME_CAPACITY];
    m_x = gpResourceManager->ReadWord();
    m_y = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
    if (m_kind == BORDER_BACKGROUND_BITMAP) {
        gpResourceManager->Read13(name);
        gpResourceManager->SavePosition();
        m_background = gpResourceManager->GetBitmap(
            reinterpret_cast<char*>(name)
        ); // byte-evidenced: resource name APIs use differently signed bytes.
        gpResourceManager->RestorePosition();
        return;
    }
    short color = gpResourceManager->ReadWord();
    m_background = 0;
    m_fillColor = color & COLOR_INDEX_MASK;
}

VA(0x00479920, 0x15d)
short border::Main(tag_message& message) {
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
    }
    return widget::Main(message);
}

VA(0x00479a80, 0x9d)
void border::Draw(void) {
    short x = m_owner->m_posX + m_x;
    short y = m_owner->m_posY + m_y;
    switch (m_kind) {
        case BORDER_BACKGROUND_SOLID:
            FillBitmapArea(
                gpWindowManager->m_screen,
                x,
                y,
                m_width,
                m_height,
                gMonoColorMap[m_fillColor]
            );
            break;
        case BORDER_BACKGROUND_BITMAP:
            PollSound();
            BlitBitmap(m_background, 0, 0, m_width, m_height, gpWindowManager->m_screen, x, y);
            PollSound();
            break;
    }
}
