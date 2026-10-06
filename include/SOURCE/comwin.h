#ifndef HOMM1_SOURCE_COMWIN_H
#define HOMM1_SOURCE_COMWIN_H

#include <windows.h>

#include <Domains.h>

// com_init's baudRate: codes 1..5 select CBR_2400..CBR_38400; any other value
// is used as the rate itself.
H1_ENUM_BEGIN(ComBaudCode)
    COM_BAUD_2400 = 1,
    COM_BAUD_4800 = 2,
    COM_BAUD_9600 = 3,
    COM_BAUD_19200 = 4,
    COM_BAUD_38400 = 5
H1_ENUM_END(ComBaudCode)

// Serial packets start their payload at +0xa (com_snd's malloc(len + 10));
// NetBIOS packets keep a session byte there and their payload at +0xb.
struct tag_Node {
    tag_Node* prev;
    tag_Node* next;
    u16 len;
    union {
        u8 comData[1];
        struct {
            u8 sessionIndex;
            u8 data[1];
        };
    };
};
struct tag_Anchor {
    tag_Node* head;
    tag_Node* tail;
};

// com_init strides ports by 0x60 bytes: handle, saved DCB at +8, saved
// timeouts at +0x24 and the two send queues at +0x50/+0x58.
struct ComPortState {
    HANDLE handle;
    char reserved04[4];
    DCB savedState;
    COMMTIMEOUTS savedTimeouts;
    char reserved38[0x18];
    tag_Anchor normalQueue;
    tag_Anchor priorityQueue;
};

H1_ENUM_CONST_BEGIN(ComConstant)
    COM_PORT_COUNT = 7,
    COM_ERROR_NAME_SIZE = 100,
    COM_ERROR_MESSAGE_SIZE = 500,
    COM_RECEIVE_BUFFER_SIZE = 0x2000,
    COM_TRANSMIT_BUFFER_SIZE = 0x1000,
    COM_BREAK_DELAY = 500,
    COM_NODE_HEADER_SIZE = 10
H1_ENUM_CONST_END(ComConstant)

extern ComPortState gComPorts[];

void init_anchor(tag_Anchor* anchor, i32, i32);
void add_node(tag_Anchor* anchor, tag_Node* node);
tag_Node* pop_node(tag_Anchor* anchor);
// Pop and free every node of a queue, leaving node NULL.
// One while statement, so it is safe as a conditional body.
#define FREE_NODE_QUEUE(node, anchor)                                                              \
    while (((node) = pop_node(anchor)) != NULL)                                                    \
    free(node)

void ShutdownComError(char* function);

i16 com_init(u8 portNumber, i32 baudRate, i32 useDtr);
void com_term(i16 port);
i16 com_rcv(i16 port, u16 requested, void* buffer);
i16 com_snd(i16 port, u16 session, u16 length, void* data, i32 priority);
i16 __cdecl com_sess(i32 port, i32 operation, ...);
u8 com_stat(i16 port, u16 session);
void comm_wrt_task(void);

#endif
