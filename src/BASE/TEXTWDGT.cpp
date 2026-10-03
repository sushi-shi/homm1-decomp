// Retail-backed text widget resource reader.

#include <match.h>

#include <BASE/display.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/message.h>
#include <BASE/resourceManager.h>
#include <BASE/textWidget.h>
#include <H1/KB.h>

#include <stdlib.h>
#include <string.h>

VA(0x0047add0, 0x3e)
VA_COMPGEN(0x0047ae10, 0x42, "??_GtextWidget@@UAEPAXI@Z", 0x0047add0)
textWidget::textWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_font = NULL;
    m_text = NULL;
    m_color = 1;
    m_alignment = FONT_ALIGN_CENTER;
    m_kind = WIDGET_KIND_TEXT;
}

VA(0x0047ae60, 0x61)
textWidget::textWidget(
    short x,
    short y,
    short width,
    short height,
    char* text,
    char* fontName,
    short color,
    short id,
    short kind
)
    : widget(x, y, width, height, id, kind) {
    m_font = gpResourceManager->GetFont(fontName);
    m_text = text;
    m_alignment = FONT_ALIGN_CENTER;
    m_kind = WIDGET_KIND_TEXT;
    m_color = color;
}

VA(0x0047aed0, 0xeb)
void textWidget::Read(void) {
    signed char name[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    short length = gpResourceManager->ReadWord();
    m_text = static_cast<char*>(malloc(length));
    // byte-evidenced: ReadBlock accepts signed bytes for stored text.
    gpResourceManager->ReadBlock(reinterpret_cast<signed char*>(m_text), length);
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_font = gpResourceManager->GetFont(
        reinterpret_cast<char*>(name)
    ); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_color = gpResourceManager->ReadWord() & COLOR_INDEX_MASK;
    m_alignment = static_cast<char>(gpResourceManager->ReadWord());
    m_id = gpResourceManager->ReadWord();
    gpResourceManager->ReadWord();
    m_kind = WIDGET_KIND_TEXT;
}

VA(0x0047afc0, 0x2d)
textWidget::~textWidget(void) {
    gpResourceManager->Dispose(m_font);
    free(m_text);
}

VA(0x0047aff0, 0x1ea)
short textWidget::Main(tag_message& message) {
    // PoL 2.0 textWidget::Main caches the flags word in a local; retail
    // keeps it in dx for the enable test and the select/deselect stores.
    short flags = m_flags;
    short y;
    short x;
    if (!(flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN: {
            x = message.x - m_owner->m_posX;
            y = message.y - m_owner->m_posY;
            if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                m_flags |= WIDGET_FLAG_SELECTED;
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
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_SET_TEXT:
                    if (message.id == m_id) {
                        SetText(message.text);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_SET_COLOR:
                    if (message.id == m_id) {
                        m_color = message.value;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
            }
            break;
    }
    return widget::Main(message);
}

VA(0x0047b1e0, 0x3b)
void textWidget::Draw(void) {
    m_font->DrawBoundedString(
        m_text,
        m_x + m_owner->m_posX,
        m_y + m_owner->m_posY,
        m_width,
        m_height,
        m_color,
        m_alignment
    );
}

VA(0x0047b220, 0x96)
void textWidget::SetText(char* text) {
    if (m_kind == WIDGET_KIND_TEXT || m_kind == WIDGET_KIND_TEXT_ENTRY) {
        unsigned short newLength = strlen(text);
        if (newLength > strlen(m_text)) {
            free(m_text);
            m_text = static_cast<char*>(malloc(newLength + 5));
        }
        strcpy(m_text, text);
    } else {
        m_text = text;
    }
}
