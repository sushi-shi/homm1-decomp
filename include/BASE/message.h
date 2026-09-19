#ifndef HOMM1_BASE_MESSAGE_H
#define HOMM1_BASE_MESSAGE_H

#include <Domains.h>

H1_ENUM_BEGIN(MessageType)
    MESSAGE_NONE = 0,
    MESSAGE_MOUSE_MOVE = 4,
    MESSAGE_LEFT_BUTTON_DOWN = 8,
    MESSAGE_LEFT_BUTTON_UP = 0x10,
    MESSAGE_RIGHT_BUTTON_DOWN = 0x20,
    MESSAGE_RIGHT_BUTTON_UP = 0x40
H1_ENUM_END(MessageType)

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

union tag_messagePayload {
    tag_messageMousePayload mouse;
    char unknown[14];
};

struct tag_message {
    H1_ENUM_STORAGE(MessageType, short) type;
    tag_messagePayload payload;
};
#pragma pack(pop)

#endif
