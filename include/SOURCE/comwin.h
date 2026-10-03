#ifndef HOMM1_SOURCE_COMWIN_H
#define HOMM1_SOURCE_COMWIN_H

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
    tag_Node *prev;
    tag_Node *next;
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
    tag_Node *head;
    tag_Node *tail;
};

void init_anchor(tag_Anchor *, i32, i32);
void add_node(tag_Anchor *, tag_Node *);
tag_Node *pop_node(tag_Anchor *);

i16 com_init(u8, i32, i32);
void com_term(i16);
i16 com_rcv(i16, u16, void *);
i16 com_snd(i16, u16, u16, void *, i32);
i16 __cdecl com_sess(i32, i32, ...);
u8 com_stat(i16, u16);
void comm_wrt_task(void);

#endif
