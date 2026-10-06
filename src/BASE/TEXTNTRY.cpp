#include <H1/Ints.h>

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

textEntryWidget::textEntryWidget(void) : textWidget() {
    m_cursorPosition = 0;
    m_icon = NULL;
    m_kind = WIDGET_KIND_TEXT_ENTRY;
    m_maxLength = 0;
    m_iconFrame = 0;
    m_displayOffset = 0;
}

textEntryWidget::~textEntryWidget(void) {
    gpResourceManager->Dispose(m_icon);
}

void textEntryWidget::Read(i32 type) {
    i8 name[RESOURCE_NAME_CAPACITY];
    READ_WIDGET_GEOMETRY(this, gpResourceManager);
    m_maxLength = gpResourceManager->ReadWord();
    m_text = static_cast<char*>(malloc(m_maxLength + 5));
    gpResourceManager->ReadBlock(reinterpret_cast<i8*>(m_text), m_maxLength);
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_font = gpResourceManager->GetFont(
        reinterpret_cast<char*>(name)
    );
    gpResourceManager->RestorePosition();
    m_color = gpResourceManager->ReadWord() & COLOR_INDEX_MASK;
    m_alignment = static_cast<char>(gpResourceManager->ReadWord());
    gpResourceManager->Read13(name);
    gpResourceManager->SavePosition();
    m_icon = gpResourceManager->GetIcon(
        reinterpret_cast<char*>(name)
    );
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
    gpResourceManager->ReadWord();
    m_kind = WIDGET_KIND_TEXT_ENTRY;
}

i16 textEntryWidget::Main(tag_message& message) {
    if (!(m_flags & WIDGET_FLAG_ENABLED)) {
        if (message.type == MESSAGE_WIDGET)
            return widget::Main(message);
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN: {
            i16 x = message.x - m_owner->m_posX;
            i16 y = message.y - m_owner->m_posY;
            if (message.type == MESSAGE_RIGHT_BUTTON_DOWN) {
                if (WIDGET_CONTAINS_LOCAL_POINT(*this, x, y)) {
                    message.command = WIDGET_NOTIFY_RIGHT_CLICK;
                    message.type = MESSAGE_WIDGET;
                    message.id = m_id;
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
                tag_message event;
                i16 done;

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
                            case TEXT_ENTRY_KEY_LEFT:
                                if (m_cursorPosition > 0) {
                                    m_cursorPosition--;
                                    if (m_displayOffset > m_cursorPosition)
                                        m_displayOffset = m_cursorPosition;
                                }
                                break;
                            case TEXT_ENTRY_KEY_RIGHT:
                                if (m_cursorPosition < strlen(edit))
                                    m_cursorPosition++;
                                break;
                            case TEXT_ENTRY_KEY_DELETE:
                                if (m_cursorPosition < strlen(edit)) {
                                    strcpy(swap, edit + m_cursorPosition + 1);
                                    strcpy(edit + m_cursorPosition, swap);
                                }
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
                                        if (m_displayOffset > m_cursorPosition)
                                            m_displayOffset = m_cursorPosition;
                                    }
                                } else if (strlen(edit) + 1 < m_maxLength && event.keyCode != 0) {
                                    char typed;
                                    strcpy(copy, edit);
                                    typed = 0;
                                    if (event.keyCode >= TEXT_ENTRY_EXTENDED_KEY_BASE) {
                                        switch ((event.keyCode >> INPUT_KEY_SCAN_SHIFT)
                                                & INPUT_SCAN_CODE_MASK) {
                                            case TEXT_ENTRY_KEYPAD_7:
                                                typed = '7';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_8:
                                                typed = '8';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_9:
                                                typed = '9';
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
                                            case TEXT_ENTRY_KEYPAD_1:
                                                typed = '1';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_2:
                                                typed = '2';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_3:
                                                typed = '3';
                                                break;
                                            case TEXT_ENTRY_KEYPAD_0:
                                                typed = '0';
                                                break;
                                        }
                                    } else {
                                        typed = static_cast<char>(event.keyCode);
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
                                        if (m_entryType != TEXT_ENTRY_READ_MULTILINE
                                            && m_font->LineLength(m_text, m_width) > m_maxLines) {
                                            strcpy(edit, copy);
                                            m_cursorPosition--;
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
                message.command = WIDGET_NOTIFY_SELECT;
                message.type = MESSAGE_WIDGET;
                message.id = m_id;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return MESSAGE_DISPATCH_CONTINUE;
        }
        case MESSAGE_WIDGET:
            switch (message.command) {
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
                case WIDGET_COMMAND_SET_MAX_LENGTH:
                    if (message.id == m_id) {
                        m_maxLength = message.value;
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
            }
            break;
    }
    return widget::Main(message);
}

void textEntryWidget::Draw(void) {
    if (m_entryType == TEXT_ENTRY_READ_MULTILINE) {
        char display[TEXT_ENTRY_DISPLAY_CAPACITY];
        strcpy(display, m_text + m_displayOffset);
        u32 length = strlen(display);
        while (m_font->LineWidth(display) > m_width)
            display[--length] = 0;
        m_icon->DrawToBuffer(
            m_rectX + m_owner->m_posX,
            m_owner->m_posY + m_rectY,
            m_iconFrame,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        m_font->DrawBoundedString(
            display,
            m_x + m_owner->m_posX,
            m_owner->m_posY + m_y,
            m_width,
            m_height,
            m_color,
            m_alignment
        );
    } else {
        m_icon->DrawToBuffer(
            m_rectX + m_owner->m_posX,
            m_owner->m_posY + m_rectY,
            m_iconFrame,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        textWidget::Draw();
    }
}

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
