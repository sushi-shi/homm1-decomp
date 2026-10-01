#ifndef HOMM1_BASE_MESSAGE_H
#define HOMM1_BASE_MESSAGE_H

#include <Domains.h>

H1_ENUM_BEGIN(MessageType)
    MESSAGE_NONE = 0,
    MESSAGE_KEY_DOWN = 1,
    MESSAGE_KEY_UP = 2,
    MESSAGE_MOUSE_MOVE = 4,
    MESSAGE_LEFT_BUTTON_DOWN = 8,
    MESSAGE_LEFT_BUTTON_UP = 0x10,
    MESSAGE_RIGHT_BUTTON_DOWN = 0x20,
    MESSAGE_RIGHT_BUTTON_UP = 0x40,
    MESSAGE_WIDGET = 0x200,
    MESSAGE_EXECUTIVE = 0x4000
H1_ENUM_END(MessageType)

H1_ENUM_BEGIN(ExecutiveCommand)
    EXECUTIVE_COMMAND_TERMINATE_LOOP = 1,
    EXECUTIVE_COMMAND_REMOVE_MANAGER = 2,
    EXECUTIVE_COMMAND_RETURN_RESULT = 4
H1_ENUM_END(ExecutiveCommand)

H1_ENUM_BEGIN(MessageDispatchResult)
    MESSAGE_DISPATCH_CONTINUE = 0,
    MESSAGE_DISPATCH_CONSUME = 1,
    MESSAGE_DISPATCH_FORWARD = 2
H1_ENUM_END(MessageDispatchResult)

H1_ENUM_BEGIN(BaseWidgetCommand)
    WIDGET_COMMAND_DRAW = 2,
    WIDGET_COMMAND_SET_TEXT = 3,
    WIDGET_COMMAND_SET_FLAGS = 5,
    WIDGET_COMMAND_CLEAR_FLAGS = 6,
    WIDGET_COMMAND_DIALOG_SELECT = 10,
    WIDGET_COMMAND_HOVER = 11,
    WIDGET_COMMAND_DIMMED = 0x1000,
    WIDGET_COMMAND_SET_FRAME = 4,
    WIDGET_COMMAND_SET_COLOR = 8,
    WIDGET_COMMAND_SET_ICON = 9,
    WIDGET_NOTIFY_SELECT = 12,
    WIDGET_NOTIFY_DESELECT = 13,
    WIDGET_NOTIFY_RIGHT_CLICK = 14
H1_ENUM_END(BaseWidgetCommand)

H1_ENUM_BEGIN(MessageModifier)
    MESSAGE_MODIFIER_NONE = 0,
    MESSAGE_MODIFIER_RIGHT_SHIFT = 1,
    MESSAGE_MODIFIER_LEFT_SHIFT = 2,
    MESSAGE_MODIFIER_SHIFT_KEYS = 3,
    MESSAGE_MODIFIER_CONTROL = 4,
    MESSAGE_MODIFIER_ALT = 0x20,
    MESSAGE_MODIFIER_RIGHT_BUTTON = 0x200,
    MESSAGE_MODIFIER_BUTTON_MASK = 0x300
H1_ENUM_END(MessageModifier)

#pragma pack(push, 1)
struct tag_messageMousePayload {
    short x;
    short y;
    H1_ENUM_STORAGE(MessageModifier, short) modifiers;
    char unknown[8];
};

union tag_messageWidgetData {
    long value;
    char* text;
};

struct tag_messageKeyboardPayload {
    short keyCode;
    short unknown;
    short modifiers;
    char unknown6[8];
};

struct tag_messageWidgetPayload {
    H1_ENUM_STORAGE(BaseWidgetCommand, short) command;
    short id;
    short modifiers;
    char unknown[4];
    tag_messageWidgetData data;
};

struct tag_messageExecutivePayload {
    H1_ENUM_STORAGE(ExecutiveCommand, short) command;
    char unknown[8];
    int result;
};

union tag_messagePayload {
    tag_messageMousePayload mouse;
    tag_messageKeyboardPayload keyboard;
    tag_messageWidgetPayload widget;
    tag_messageExecutivePayload executive;
    char unknown[14];
};

struct tag_message {
    H1_ENUM_STORAGE(MessageType, short) type;
    tag_messagePayload payload;
};
#pragma pack(pop)

#define IS_WIDGET_SELECTION_NOTIFICATION(command) \
    ((command) == WIDGET_NOTIFY_SELECT || (command) == WIDGET_NOTIFY_RIGHT_CLICK)

#endif
