// Retail-backed button widget resource reader.

#include <match.h>

#include <BASE/button.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

DATA(0x004d7f60)
// The button modifiers of the last press, kept in a 32-bit global.
i32 gLeftRightSave = MESSAGE_MODIFIER_NONE;

VA(0x00476c80, 0x59)
button::button(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_icon = NULL;
    m_normalFrame = 0;
    m_pressedFrame = 0;
    m_selectMode = BUTTON_SELECT_NOTIFY;
    m_hotkey = BUTTON_NO_HOTKEY;
}

// The icon-file-id and icon-name overloads; no retail caller survives.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00476cd9, 0xae)
button::button(
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    i16 iconId,
    i16 normalFrame,
    i16 pressedFrame,
    i16 selectMode,
    i16 hotkey,
    i16 id,
    i16 kind
)
    : widget(x, y, width, height, id, kind) {
    m_icon = gResourceManager->GetIcon(iconId);
    m_normalFrame = normalFrame;
    m_pressedFrame = pressedFrame;
    m_selectMode = selectMode;
    m_hotkey = hotkey;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00476d87, 0xad)
button::button(
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    char* iconName,
    i16 normalFrame,
    i16 pressedFrame,
    i16 selectMode,
    i16 hotkey,
    i16 id,
    i16 kind
)
    : widget(x, y, width, height, id, kind) {
    m_icon = gResourceManager->GetIcon(iconName);
    m_normalFrame = normalFrame;
    m_pressedFrame = pressedFrame;
    m_selectMode = selectMode;
    m_hotkey = hotkey;
}

VA(0x00476e34, 0xfb)
void button::Read(void) {
    char iconName[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gResourceManager);
    gResourceManager->Read13(iconName);
    gResourceManager->SavePosition();
    m_icon = gResourceManager->GetIcon(iconName);
    gResourceManager->RestorePosition();
    m_normalFrame = gResourceManager->ReadWord();
    m_pressedFrame = gResourceManager->ReadWord();
    m_selectMode = gResourceManager->ReadWord();
    m_hotkey = gResourceManager->ReadWord();
    m_id = gResourceManager->ReadWord();
    m_kind = gResourceManager->ReadWord();
}

VA(0x00476f2f, 0x5b)
button::~button(void) {
    gResourceManager->Dispose(m_icon);
}

VA(0x00476f8a, 0x415)
H1_ENUM_RETURN(MessageDispatchResult, i16) button::Main(tag_message& message) {
    i16 x;
    i16 y;
    if (m_kind == WIDGET_KIND_AUTO_REPEAT && (m_flags & WIDGET_FLAG_SELECTED)
        && gTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] < KBTickCount())
        return Deselect(message);
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_KEY_DOWN:
            if (m_hotkey != BUTTON_NO_HOTKEY && m_hotkey == message.keyCode)
                return Select(message);
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_KEY_UP:
            if (m_hotkey != BUTTON_NO_HOTKEY && m_hotkey == message.keyCode)
                return Deselect(message);
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN: {
            x = message.x - m_owner->m_posX;
            y = message.y - m_owner->m_posY;
            if (message.type == MESSAGE_RIGHT_BUTTON_DOWN) {
                if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                    SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_RIGHT_CLICK, m_id);
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                    return MESSAGE_DISPATCH_FORWARD;
                }
                return MESSAGE_DISPATCH_CONTINUE;
            }
            if (!(m_flags & WIDGET_FLAG_DIMMED) && WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                if (m_kind != WIDGET_KIND_TRACK_PRESS)
                    return Select(message);
                Select(message);
                while (!IS_BUTTON_RELEASE_MESSAGE(message.type)) {
                    gMouseManager->Main(message);
                    if (message.type == MESSAGE_MOUSE_MOVE) {
                        x = message.x - m_owner->m_posX;
                        y = message.y - m_owner->m_posY;
                        if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                            if (!(m_flags & WIDGET_FLAG_SELECTED))
                                Select(message);
                        } else if (m_flags & WIDGET_FLAG_SELECTED) {
                            Deselect(message);
                        }
                    }
                    Process1WindowsMessage();
                    message = gInputManager->GetEvent();
                }
                return Deselect(message);
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
        case MESSAGE_LEFT_BUTTON_UP:
            if (m_flags & WIDGET_FLAG_SELECTED)
                return Deselect(message);
            break;
        default:
        normalEvent:
            return widget::Main(message);
    }
    goto normalEvent;
}

VA(0x0047739f, 0xec)
H1_ENUM_RETURN(MessageDispatchResult, i16) button::Select(tag_message& message) {
    i16 x = m_owner->m_posX + m_x;
    i16 y = m_owner->m_posY + m_y;
    m_icon->DrawToBuffer(x, y, m_pressedFrame, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
    m_flags |= WIDGET_FLAG_SELECTED;
    message.type = MESSAGE_WIDGET;
    message.id = m_id;
    if (m_selectMode == BUTTON_SELECT_DIALOG_RESULT)
        message.command = WIDGET_COMMAND_DIALOG_SELECT;
    else
        message.command = WIDGET_NOTIFY_SELECT;
    gTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = KBTickCount() + BUTTON_REPEAT_DELAY_TICKS;
    gLeftRightSave =
        H1_ENUM_ENCODE(MessageModifier, message.modifiers & MESSAGE_MODIFIER_BUTTON_MASK);
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x0047748b, 0xba)
H1_ENUM_RETURN(MessageDispatchResult, i16) button::Deselect(tag_message& message) {
    if (!(m_flags & WIDGET_FLAG_SELECTED))
        return MESSAGE_DISPATCH_CONTINUE;
    m_flags &= ~WIDGET_FLAG_SELECTED;
    Draw();
    gWindowManager
        ->UpdateScreenRegion(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_width, m_height);
    SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_DESELECT, m_id);
    message.modifiers = H1_ENUM_DECODE(MessageModifier, gLeftRightSave);
    gLeftRightSave = MESSAGE_MODIFIER_NONE;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x00477545, 0x99)
void button::Draw(void) {
    if (m_flags & WIDGET_FLAG_SELECTED) {
        m_icon->DrawToBuffer(
            m_owner->m_posX + m_x,
            m_owner->m_posY + m_y,
            m_pressedFrame,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        return;
    }
    m_icon->DrawToBuffer(
        m_owner->m_posX + m_x,
        m_owner->m_posY + m_y,
        m_normalFrame,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL
    );
}

VA_COMPGEN(0x00477620, 0x2e, "??_Gbutton@@UAEPAXI@Z", 0x00476c80)
