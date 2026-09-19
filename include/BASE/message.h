#ifndef HOMM1_BASE_MESSAGE_H
#define HOMM1_BASE_MESSAGE_H

#include <Domains.h>

H1_ENUM_BEGIN(MessageType)
    MESSAGE_NONE = 0,
    MESSAGE_MOUSE_MOVE = 4,
    MESSAGE_LEFT_BUTTON_DOWN = 8,
    MESSAGE_LEFT_BUTTON_UP = 0x10,
    MESSAGE_RIGHT_BUTTON_DOWN = 0x20,
    MESSAGE_RIGHT_BUTTON_UP = 0x40,
    MESSAGE_WIDGET = 0x200
H1_ENUM_END(MessageType)

H1_ENUM_BEGIN(MessageDispatchResult)
    MESSAGE_DISPATCH_CONTINUE = 0,
    MESSAGE_DISPATCH_CONSUME = 1,
    MESSAGE_DISPATCH_FORWARD = 2
H1_ENUM_END(MessageDispatchResult)

H1_ENUM_BEGIN(BaseWidgetCommand)
    WIDGET_COMMAND_DRAW = 2
H1_ENUM_END(BaseWidgetCommand)

H1_ENUM_BEGIN(MessageModifier)
    MESSAGE_MODIFIER_NONE = 0
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
    char *text;
};

struct tag_messageWidgetPayload {
    H1_ENUM_STORAGE(BaseWidgetCommand, short) command;
    short id;
    char unknown[6];
    tag_messageWidgetData data;
};

union tag_messagePayload {
    tag_messageMousePayload mouse;
    tag_messageWidgetPayload widget;
    char unknown[14];
};

struct tag_message {
    H1_ENUM_STORAGE(MessageType, short) type;
    tag_messagePayload payload;
};
#pragma pack(pop)

#endif
