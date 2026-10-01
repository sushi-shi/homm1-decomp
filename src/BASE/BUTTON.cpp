// Retail-backed button widget resource reader.

#include <match.h>

#include <BASE/button.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

long gButtonRepeatTimer;
int iLeftRightSave;

VA(0x0047eef0, 0x31)
button::button(void) : widget(0, 0, 0, 0, 0, 0) {
    m_normalFrame = 0;
    m_pressedFrame = 0;
    m_selectMode = 0;
    m_icon = 0;
    m_hotkey = -1;
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

VA(0x0047f580, 0x92)
short button::Select(tag_message& message) {
    short x = m_owner->m_posX + m_x;
    short y = m_owner->m_posY + m_y;
    m_icon->DrawToBuffer(x, y, m_pressedFrame, 0, 0);
    gpWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
    m_flags |= WIDGET_FLAG_SELECTED;
    message.type = MESSAGE_WIDGET;
    message.payload.widget.id = m_id;
    if (m_selectMode == BUTTON_SELECT_DIALOG_RESULT)
        message.payload.widget.command = WIDGET_COMMAND_DIALOG_SELECT;
    else
        message.payload.widget.command = WIDGET_NOTIFY_SELECT;
    gButtonRepeatTimer = KBTickCount() + BUTTON_REPEAT_DELAY_TICKS;
    iLeftRightSave = message.payload.mouse.modifiers & MESSAGE_MODIFIER_BUTTON_MASK;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x0047f620, 0x4d)
void button::Draw(void) {
    if (m_flags & WIDGET_FLAG_SELECTED) {
        m_icon->DrawToBuffer(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_pressedFrame, 0, 0);
        return;
    }
    m_icon->DrawToBuffer(m_owner->m_posX + m_x, m_owner->m_posY + m_y, m_normalFrame, 0, 0);
}
