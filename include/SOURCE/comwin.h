#ifndef HOMM1_SOURCE_COMWIN_H
#define HOMM1_SOURCE_COMWIN_H

enum ComBaudCode {
    COM_BAUD_2400 = 1,
    COM_BAUD_4800 = 2,
    COM_BAUD_9600 = 3,
    COM_BAUD_19200 = 4,
    COM_BAUD_38400 = 5
};

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

void init_anchor(tag_Anchor* anchor, i32, i32);
void add_node(tag_Anchor* anchor, tag_Node* node);
tag_Node* pop_node(tag_Anchor* anchor);

i16 com_init(u8 portNumber, i32 baudRate, i32 useDtr);
void com_term(i16 port);
i16 com_rcv(i16 port, u16 requested, void* buffer);
i16 com_snd(i16 port, u16, u16 length, void* data, i32 priority);
i16 __cdecl com_sess(i32, i32, ...);
u8 com_stat(i16 port, u16);
void comm_wrt_task(void);

#endif
