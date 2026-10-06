#ifndef HOMM1_BASE_MESSAGE_H
#define HOMM1_BASE_MESSAGE_H

enum MessageType {
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
};

enum ExecutiveCommand {
    EXECUTIVE_COMMAND_TERMINATE_LOOP = 1,
    EXECUTIVE_COMMAND_REMOVE_MANAGER = 2,
    EXECUTIVE_COMMAND_RETURN_RESULT = 4
};

enum MessageDispatchResult {
    MESSAGE_DISPATCH_CONTINUE = 0,
    MESSAGE_DISPATCH_CONSUME = 1,
    MESSAGE_DISPATCH_FORWARD = 2
};

enum BaseWidgetCommand {
    WIDGET_COMMAND_DRAW = 2,
    WIDGET_COMMAND_SET_TEXT = 3,
    WIDGET_COMMAND_SET_FLAGS = 5,
    WIDGET_COMMAND_CLEAR_FLAGS = 6,
    WIDGET_COMMAND_DIALOG_SELECT = 10,
    WIDGET_COMMAND_HOVER = 11,
    WIDGET_COMMAND_SET_FRAME = 4,
    WIDGET_COMMAND_SET_COLOR = 8,
    WIDGET_COMMAND_SET_ICON = 9,
    WIDGET_COMMAND_GET_TEXT = 7,
    WIDGET_COMMAND_SET_MAX_LENGTH = 0x33,
    WIDGET_NOTIFY_SELECT = 12,
    WIDGET_NOTIFY_DESELECT = 13,
    WIDGET_NOTIFY_RIGHT_CLICK = 14
};

enum MessageModifier {
    MESSAGE_MODIFIER_NONE = 0,
    MESSAGE_MODIFIER_RIGHT_SHIFT = 1,
    MESSAGE_MODIFIER_LEFT_SHIFT = 2,
    MESSAGE_MODIFIER_SHIFT_KEYS = 3,
    MESSAGE_MODIFIER_CONTROL = 4,
    MESSAGE_MODIFIER_CONTROL_KEYS = 0xc,
    MESSAGE_MODIFIER_ALT = 0x20,
    MESSAGE_MODIFIER_RIGHT_BUTTON = 0x200,
    MESSAGE_MODIFIER_BUTTON_MASK = 0x300
};

struct tag_message {
    i16 type;
    union {
        i16 command;
        i16 executiveCommand;
        i16 keyCode;
        i16 x;
    };
    union {
        i16 id;
        i16 y;
    };
    i16 modifiers;
    char unknown8[4];
    union {
        i32 value;
        char* text;
        i32 result;
    };
};

#define SET_WIDGET_MESSAGE(messageValue, commandValue, idValue)                                    \
    ((messageValue).type = MESSAGE_WIDGET,                                                         \
     (messageValue).command = (commandValue),                                                      \
     (messageValue).id = (idValue))

#define IS_WIDGET_SELECTION_NOTIFICATION(command)                                                  \
    ((command) == WIDGET_NOTIFY_SELECT || (command) == WIDGET_NOTIFY_RIGHT_CLICK)

#define IS_BUTTON_RELEASE_MESSAGE(type)                                                            \
    ((type) == MESSAGE_LEFT_BUTTON_UP || (type) == MESSAGE_RIGHT_BUTTON_UP)

#endif
