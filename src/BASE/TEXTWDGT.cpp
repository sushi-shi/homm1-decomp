// Text widget resource reader.

#include <match.h>

#include <BASE/display.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/message.h>
#include <BASE/resourceManager.h>
#include <BASE/textEntryWidget.h>
#include <BASE/textWidget.h>
#include <SOURCE/KB.h>

#include <stdlib.h>
#include <string.h>

VA(0x00471770, 0x58)
textWidget::textWidget(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_font = NULL;
    m_text = NULL;
    m_color = 1;
    m_alignment = FONT_ALIGN_CENTER;
    m_kind = WIDGET_KIND_TEXT;
}

VA(0x004717c8, 0xa5)
textWidget::textWidget(
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    char* text,
    char* fontName,
    i16 color,
    i16 id,
    i16 kind
)
    : widget(x, y, width, height, id, kind) {
    m_font = gResourceManager->GetFont(fontName);
    m_text = text;
    m_color = color;
    m_alignment = FONT_ALIGN_CENTER;
    m_kind = WIDGET_KIND_TEXT;
}

#define fontName name // frame-slot spelling
VA(0x0047186d, 0x12a)
void textWidget::Read(void) {
    char fontName[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gResourceManager);
    i16 length = gResourceManager->ReadWord();
    m_text = static_cast<char*>(malloc(length));
    gResourceManager->ReadBlock(m_text, length);
    gResourceManager->Read13(fontName);
    gResourceManager->SavePosition();
    m_font = gResourceManager->GetFont(fontName);
    gResourceManager->RestorePosition();
    m_color = gResourceManager->ReadWord() & COLOR_INDEX_MASK;
    m_alignment = H1_ENUM_DECODE(
        FontAlignment,
        static_cast<char>(gResourceManager->ReadWord() & COLOR_INDEX_MASK)
    );
    m_id = gResourceManager->ReadWord();
    m_kind = gResourceManager->ReadWord();
    m_kind = WIDGET_KIND_TEXT;
}
#undef fontName

VA(0x00471997, 0x6a)
textWidget::~textWidget(void) {
    gResourceManager->Dispose(m_font);
    free(m_text);
}

VA(0x00471a01, 0x238)
H1_ENUM_RETURN(MessageDispatchResult, i16) textWidget::Main(tag_message& message) {
    i16 y;
    i16 x;
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
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
                        SetColorIndex(message.value);
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
                SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_SELECT, m_id);
                // SET_WIDGET_MESSAGE has already replaced the type, so the right-button
                // modifier is never set here.
                if (message.type == MESSAGE_RIGHT_BUTTON_DOWN)
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
        case MESSAGE_LEFT_BUTTON_UP:
        case MESSAGE_RIGHT_BUTTON_UP:
            if (m_flags & WIDGET_FLAG_SELECTED) {
                m_flags &= ~WIDGET_FLAG_SELECTED;
                SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_DESELECT, m_id);
                // SET_WIDGET_MESSAGE has already replaced the type, so the right-button
                // modifier is never set here.
                if (message.type == MESSAGE_RIGHT_BUTTON_UP)
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
    }
    return widget::Main(message);
}

VA(0x00471c39, 0x66)
void textWidget::Draw(void) {
    m_font->DrawBoundedString(
        m_text,
        m_owner->m_posX + m_x,
        m_owner->m_posY + m_y,
        m_width,
        m_height,
        m_color,
        m_alignment
    );
}

VA(0x00471c9f, 0x18)
void textWidget::SetColorIndex(i16 color) {
    m_color = color;
}

VA(0x00471cb7, 0xa1)
void textWidget::SetText(char* text) {
    if (m_kind == WIDGET_KIND_TEXT || m_kind == WIDGET_KIND_TEXT_ENTRY) {
        u16 newLength = strlen(text);
        if (newLength > strlen(m_text)) {
            free(m_text);
            m_text = static_cast<char*>(malloc(newLength + TEXT_ENTRY_ALLOCATION_PADDING));
        }
        strcpy(m_text, text);
    } else {
        m_text = text;
    }
}

VA_COMPGEN(0x00471da0, 0x2e, "??_GtextWidget@@UAEPAXI@Z", 0x00471770)
