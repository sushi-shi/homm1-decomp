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
i32 gLeftRightSave = 0;

VA_COMPGEN(0x00476f2f, 0x5b, "??1button@@UAE@XZ", 0x00476c80)
VA(0x00476c80, 0x59)
button::button(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_normalFrame = 0;
    m_pressedFrame = 0;
    m_selectMode = 0;
    m_icon = NULL;
    m_hotkey = BUTTON_NO_HOTKEY;
}

VA_COMPGEN(0x00477620, 0x2e, "??_Gbutton@@UAEPAXI@Z", 0x00476c80)
button::~button(void) {
    gpResourceManager->Dispose(m_icon);
}

VA(0x00476e34, 0xfb)
void button::Read(void) {
    i8 name[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(
        reinterpret_cast<char*>(name)
    ); // byte-evidenced: resource name APIs use differently signed bytes.
    gpResourceManager->RestorePosition();
    m_normalFrame = gpResourceManager->ReadWord();
    m_pressedFrame = gpResourceManager->ReadWord();
    m_selectMode = gpResourceManager->ReadWord();
    m_hotkey = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

inline i16 button::Deselect(tag_message& message) {
    if (!(m_flags & WIDGET_FLAG_SELECTED))
        return MESSAGE_DISPATCH_CONTINUE;
    m_flags &= ~WIDGET_FLAG_SELECTED;
    Draw();
    gpWindowManager
        ->UpdateScreenRegion(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_width, m_height);
    message.command = WIDGET_NOTIFY_DESELECT;
    message.type = MESSAGE_WIDGET;
    message.id = m_id;
    message.modifiers = gLeftRightSave;
    gLeftRightSave = MESSAGE_MODIFIER_NONE;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x00476f8a, 0x415)
i16 button::Main(tag_message& message) {
    if (m_kind == WIDGET_KIND_AUTO_REPEAT && (m_flags & WIDGET_FLAG_SELECTED)
        && KBTickCount() > glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT])
        return Deselect(message);
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_KEY_DOWN:
            if (m_hotkey != BUTTON_NO_HOTKEY && message.keyCode == m_hotkey)
                return Select(message);
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_KEY_UP:
            if (m_hotkey != BUTTON_NO_HOTKEY && message.keyCode == m_hotkey)
                return Deselect(message);
            return MESSAGE_DISPATCH_CONTINUE;
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN: {
            i16 x = message.x - m_owner->m_posX;
            i16 y = message.y - m_owner->m_posY;
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
                while (message.type != MESSAGE_LEFT_BUTTON_UP
                       && message.type != MESSAGE_RIGHT_BUTTON_UP) {
                    gpMouseManager->Main(message);
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
                    message = gpInputManager->GetEvent();
                }
                return Deselect(message);
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
        case MESSAGE_LEFT_BUTTON_UP:
            if (m_flags & WIDGET_FLAG_SELECTED)
                return Deselect(message);
            break;
    }
    return widget::Main(message);
}

VA(0x0047739f, 0xec)
i16 button::Select(tag_message& message) {
    i16 x = m_owner->m_posX + m_x;
    i16 y = m_owner->m_posY + m_y;
    m_icon->DrawToBuffer(x, y, m_pressedFrame, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gpWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
    m_flags |= WIDGET_FLAG_SELECTED;
    message.type = MESSAGE_WIDGET;
    message.id = m_id;
    if (m_selectMode == BUTTON_SELECT_DIALOG_RESULT)
        message.command = WIDGET_COMMAND_DIALOG_SELECT;
    else
        message.command = WIDGET_NOTIFY_SELECT;
    glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = KBTickCount() + BUTTON_REPEAT_DELAY_TICKS;
    gLeftRightSave = message.modifiers & MESSAGE_MODIFIER_BUTTON_MASK;
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
