// Retail-backed button widget resource reader.

#include <match.h>

#include <BASE/button.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

long gButtonRepeatTimer;
int iLeftRightSave;

VA(0x0047eef0, 0x31)
button::button(void) : widget(0, 0, 0, 0, 0, WIDGET_KIND_NONE) {
    m_normalFrame = 0;
    m_pressedFrame = 0;
    m_selectMode = 0;
    m_icon = NULL;
    m_hotkey = BUTTON_NO_HOTKEY;
}

VA_COMPGEN(0x0047ef30, 0x36, "??_Gbutton@@UAEPAXI@Z", 0x0047eef0)
button::~button(void) {
    gpResourceManager->Dispose(m_icon);
}

VA(0x0047ef70, 0xda)
void button::Read(void) {
    signed char name[13];
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
    m_normalFrame = gpResourceManager->ReadWord();
    m_pressedFrame = gpResourceManager->ReadWord();
    m_selectMode = gpResourceManager->ReadWord();
    m_hotkey = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
}

inline short button::Deselect(tag_message& message) {
    if (!(m_flags & WIDGET_FLAG_SELECTED))
        return MESSAGE_DISPATCH_CONTINUE;
    m_flags &= ~WIDGET_FLAG_SELECTED;
    Draw();
    gpWindowManager
        ->UpdateScreenRegion(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_width, m_height);
    message.command = WIDGET_NOTIFY_DESELECT;
    message.type = MESSAGE_WIDGET;
    message.id = m_id;
    message.modifiers = iLeftRightSave;
    iLeftRightSave = MESSAGE_MODIFIER_NONE;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x0047f050, 0x528)
short button::Main(tag_message& message) {
    if (m_kind == WIDGET_KIND_AUTO_REPEAT && (m_flags & WIDGET_FLAG_SELECTED)
        && KBTickCount() > gButtonRepeatTimer)
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
            short x = message.x - m_owner->m_posX;
            short y = message.y - m_owner->m_posY;
            if (message.type == MESSAGE_RIGHT_BUTTON_DOWN) {
                if (x >= m_x && y >= m_y && x < m_x + m_width && y < m_y + m_height) {
                    message.type = MESSAGE_WIDGET;
                    message.command = WIDGET_NOTIFY_RIGHT_CLICK;
                    message.id = m_id;
                    message.modifiers = MESSAGE_MODIFIER_RIGHT_BUTTON;
                    return MESSAGE_DISPATCH_FORWARD;
                }
                return MESSAGE_DISPATCH_CONTINUE;
            }
            if (!(m_flags & WIDGET_FLAG_DIMMED) && x >= m_x && y >= m_y && x < m_x + m_width
                && y < m_y + m_height) {
                if (m_kind != WIDGET_KIND_TRACK_PRESS)
                    return Select(message);
                Select(message);
                while (message.type != MESSAGE_LEFT_BUTTON_UP
                       && message.type != MESSAGE_RIGHT_BUTTON_UP) {
                    gpMouseManager->Main(message);
                    if (message.type == MESSAGE_MOUSE_MOVE) {
                        x = message.x - m_owner->m_posX;
                        y = message.y - m_owner->m_posY;
                        if (x >= m_x && y >= m_y && x < m_x + m_width && y < m_y + m_height) {
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

VA(0x0047f580, 0x92)
short button::Select(tag_message& message) {
    short x = m_owner->m_posX + m_x;
    short y = m_owner->m_posY + m_y;
    m_icon->DrawToBuffer(x, y, m_pressedFrame, ICON_DRAW_NORMAL, 0);
    gpWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
    m_flags |= WIDGET_FLAG_SELECTED;
    message.type = MESSAGE_WIDGET;
    message.id = m_id;
    if (m_selectMode == BUTTON_SELECT_DIALOG_RESULT)
        message.command = WIDGET_COMMAND_DIALOG_SELECT;
    else
        message.command = WIDGET_NOTIFY_SELECT;
    gButtonRepeatTimer = KBTickCount() + BUTTON_REPEAT_DELAY_TICKS;
    iLeftRightSave = message.modifiers & MESSAGE_MODIFIER_BUTTON_MASK;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x0047f620, 0x4d)
void button::Draw(void) {
    if (m_flags & WIDGET_FLAG_SELECTED) {
        m_icon->DrawToBuffer(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_pressedFrame, ICON_DRAW_NORMAL, 0);
        return;
    }
    m_icon->DrawToBuffer(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_normalFrame, ICON_DRAW_NORMAL, 0);
}
