// Retail-backed text-entry widget resource reader.

#include <match.h>

#include <BASE/display.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/textEntryWidget.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

#include <stdlib.h>
#include <string.h>

VA(0x00475830, 0x56)
textEntryWidget::textEntryWidget(void) : textWidget() {
    m_cursorPosition = 0;
    m_maxLength = 0;
    m_icon = NULL;
    m_iconFrame = 0;
    m_displayOffset = 0;
    m_kind = WIDGET_KIND_TEXT_ENTRY;
}

textEntryWidget::~textEntryWidget(void) {
    gpResourceManager->Dispose(m_icon);
}

// The parameterized constructor; no retail caller survives (HoMM1 has no
// inset layout arguments).
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00475886, 0xbf)
textEntryWidget::textEntryWidget(
    i16 x,
    i16 y,
    i16 width,
    i16 height,
    i16 maxLength,
    char* text,
    char* fontName,
    i16 color,
    char* iconName,
    i16 iconFrame,
    i16 id,
    i16 kind
)
    : textWidget(x, y, width, height, text, fontName, color, id, kind) {
    m_cursorPosition = 0;
    m_maxLength = maxLength;
    m_icon = gpResourceManager->GetIcon(iconName);
    m_iconFrame = iconFrame;
    m_displayOffset = 0;
    m_kind = WIDGET_KIND_TEXT_ENTRY;
}

VA_COMPGEN(0x00475945, 0x5b, "??1textEntryWidget@@UAE@XZ", 0x00475830)
VA(0x004759a0, 0x261)
void textEntryWidget::Read(H1_ENUM_PARAM(TextEntryReadMode, i32) type) {
    char name[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    m_maxLength = gpResourceManager->ReadWord();
    m_text = static_cast<char*>(malloc(m_maxLength + 5));
    gpResourceManager->ReadBlock(m_text, m_maxLength);
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_font = gpResourceManager->GetFont(name);
    gpResourceManager->RestorePosition();
    m_color = gpResourceManager->ReadWord() & COLOR_INDEX_MASK;
    m_alignment = static_cast<char>(gpResourceManager->ReadWord() & COLOR_INDEX_MASK);
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(name);
    gpResourceManager->RestorePosition();
    m_entryType = type;
    if (type == TEXT_ENTRY_READ_RECT) {
        m_rectX = gpResourceManager->ReadWord();
        m_rectY = gpResourceManager->ReadWord();
        m_rectW = gpResourceManager->ReadWord();
        m_rectH = gpResourceManager->ReadWord();
        m_maxLines = gpResourceManager->ReadWord();
        m_preserveTextOnFocus = gpResourceManager->ReadWord();
    } else {
        m_rectX = m_x;
        m_rectY = m_y;
        m_rectW = m_width;
        m_rectH = m_height;
        m_maxLines = 1;
        if (type == TEXT_ENTRY_READ_MULTILINE)
            m_preserveTextOnFocus = 1;
        else
            m_preserveTextOnFocus = 0;
    }
    m_iconFrame = gpResourceManager->ReadWord();
    m_id = gpResourceManager->ReadWord();
    m_kind = gpResourceManager->ReadWord();
    m_kind = WIDGET_KIND_TEXT_ENTRY;
}

VA(0x00475c01, 0xa70)
i16 textEntryWidget::Main(tag_message& message) {
    i16 done;
    i16 x;
    i16 y;
    tag_message event;
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_SET_MAX_LENGTH:
                    if (message.id == m_id) {
                        m_maxLength = message.value;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_SET_TEXT:
                    if (message.id == m_id) {
                        SetText(message.text);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_COMMAND_GET_TEXT:
                    if (message.id == m_id) {
                        message.text = m_text;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
            }
            break;
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
            if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                char edit[TEXT_ENTRY_DISPLAY_CAPACITY];
                char swap[TEXT_ENTRY_DISPLAY_CAPACITY];
                char copy[TEXT_ENTRY_DISPLAY_CAPACITY];
                char original[TEXT_ENTRY_DISPLAY_CAPACITY];

                gpMouseManager->ReallyHidePointer();
                x = m_owner->m_posX + m_x;
                y = m_owner->m_posY + m_y;
                strcpy(original, m_text);
                if (m_preserveTextOnFocus & TEXT_ENTRY_PRESERVE_TEXT) {
                    m_cursorPosition = strlen(m_text);
                } else {
                    m_cursorPosition = 0;
                    m_text[0] = 0;
                }
                strcpy(edit, m_text);
                SetupDisplayString(edit, m_cursorPosition);
                Draw();
                gpWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
                done = 0;
                while (done == 0) {
                    Process1WindowsMessage();
                    event = gpInputManager->GetEvent();
                    if (event.type == MESSAGE_KEY_DOWN) {
                        switch (event.keyCode) {
                            case TEXT_ENTRY_KEY_ESCAPE:
                                strcpy(edit, original);
                                done++;
                                break;
                            case TEXT_ENTRY_KEY_DELETE:
                                if (m_cursorPosition < strlen(edit)) {
                                    strcpy(swap, edit + m_cursorPosition + 1);
                                    strcpy(edit + m_cursorPosition, swap);
                                }
                                break;
                            case TEXT_ENTRY_KEY_LEFT:
                                if (m_cursorPosition > 0) {
                                    m_cursorPosition--;
                                    if (m_cursorPosition < m_displayOffset)
                                        m_displayOffset = m_cursorPosition;
                                }
                                break;
                            case TEXT_ENTRY_KEY_RIGHT:
                                if (m_cursorPosition < strlen(edit))
                                    m_cursorPosition++;
                                break;
                            default:
                                gpInputManager->AsciiConvert(event);
                                if (event.keyCode == TEXT_ENTRY_KEY_ACCEPT) {
                                    done++;
                                } else if (event.keyCode == TEXT_ENTRY_KEY_BACKSPACE) {
                                    if (m_cursorPosition > 0) {
                                        strcpy(swap, edit + m_cursorPosition);
                                        strcpy(edit + m_cursorPosition - 1, swap);
                                        m_cursorPosition--;
                                        if (m_cursorPosition < m_displayOffset)
                                            m_displayOffset = m_cursorPosition;
                                    }
                                } else if (strlen(edit) + 1 < m_maxLength && event.keyCode != 0) {
                                    char typed;
                                    strcpy(copy, edit);
                                    typed = 0;
                                    if (event.keyCode >= TEXT_ENTRY_EXTENDED_KEY_BASE) {
                                        i32 key = (event.keyCode
                                                   & (INPUT_SCAN_CODE_MASK << INPUT_KEY_SCAN_SHIFT))
                                                  >> INPUT_KEY_SCAN_SHIFT;
                                        switch (key) {
                                            case TEXT_ENTRY_KEYPAD_0:
                                                typed = '0';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_1:
                                                typed = '1';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_2:
                                                typed = '2';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_3:
                                                typed = '3';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_4:
                                                typed = '4';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_5:
                                                typed = '5';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_6:
                                                typed = '6';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_7:
                                                typed = '7';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_8:
                                                typed = '8';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_9:
                                                typed = '9';
                                                break;
                                        }
                                    } else {
                                        typed = event.keyCode & INPUT_SCAN_CODE_MASK;
                                    }
                                    if (typed != 0) {
                                        strcpy(swap, m_text);
                                        free(m_text);
                                        m_text = static_cast<char*>(
                                            malloc(strlen(edit) + 1 + TEXT_ENTRY_ALLOCATION_PADDING)
                                        );
                                        strcpy(swap, edit);
                                        swap[m_cursorPosition] = typed;
                                        swap[m_cursorPosition + 1] = 0;
                                        strcat(swap, edit + m_cursorPosition);
                                        strcpy(edit, swap);
                                        m_cursorPosition++;
                                        SetupDisplayString(edit, m_cursorPosition);
                                        if (m_entryType != TEXT_ENTRY_READ_MULTILINE) {
                                            i32 lineLength = m_font->LineLength(m_text, m_width);
                                            if (lineLength > m_maxLines) {
                                                strcpy(edit, copy);
                                                m_cursorPosition--;
                                            }
                                        }
                                    }
                                }
                                break;
                        }
                        SetupDisplayString(edit, m_cursorPosition);
                        Draw();
                        gpWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
                    }
                }
                strcpy(m_text, edit);
                m_displayOffset = 0;
                Draw();
                gpWindowManager->UpdateScreenRegion(x, y, m_width, m_height);
                gpMouseManager->ReallyShowPointer();
                SET_WIDGET_MESSAGE(message, WIDGET_NOTIFY_SELECT, m_id);
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
    }
    return widget::Main(message);
}

VA(0x00476671, 0x1c7)
void textEntryWidget::Draw(void) {
    if (m_entryType == TEXT_ENTRY_READ_MULTILINE) {
        char display[TEXT_ENTRY_DISPLAY_CAPACITY];
        strcpy(display, m_text + m_displayOffset);
        u32 len = strlen(display);
        while (m_font->LineWidth(display) > m_width)
            display[--len] = 0;
        m_icon->DrawToBuffer(
            m_owner->m_posX + m_rectX,
            m_owner->m_posY + m_rectY,
            m_iconFrame,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        m_font->DrawBoundedString(
            display,
            m_owner->m_posX + m_x,
            m_owner->m_posY + m_y,
            m_width,
            m_height,
            m_color,
            m_alignment
        );
    } else {
        m_icon->DrawToBuffer(
            m_owner->m_posX + m_rectX,
            m_owner->m_posY + m_rectY,
            m_iconFrame,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        textWidget::Draw();
    }
}

VA(0x00476838, 0x23d)
void textEntryWidget::SetupDisplayString(char* source, u16 cursor) {
    i32 changed;
    char display[TEXT_ENTRY_DISPLAY_CAPACITY];
    if (cursor > 0)
        strncpy(m_text, source, cursor);
    m_text[cursor] = '_';
    if (strlen(source) > cursor)
        strcpy(m_text + cursor + 1, source + cursor);
    else
        m_text[cursor + 1] = 0;
    if (m_entryType == TEXT_ENTRY_READ_MULTILINE) {
        changed = 1;
        while (changed) {
            changed = 0;
            strcpy(display, m_text + m_displayOffset);
            if (m_font->LineWidth(display) > m_width) {
                display[cursor - m_displayOffset + 1] = 0;
                if (m_font->LineWidth(display) > m_width) {
                    m_displayOffset++;
                    changed = 1;
                }
            }
        }
        if (m_displayOffset > 0) {
            changed = 1;
            while (changed) {
                changed = 0;
                strcpy(display, m_text + m_displayOffset - 1);
                if (m_font->LineWidth(display) <= m_width)
                    m_displayOffset--;
                else
                    changed = 0;
                if (m_displayOffset == 0)
                    changed = 0;
            }
        }
    }
}

VA_COMPGEN(0x00476ab0, 0x2e, "??_GtextEntryWidget@@UAEPAXI@Z", 0x00475830)
