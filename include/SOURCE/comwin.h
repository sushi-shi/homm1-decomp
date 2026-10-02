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
    unsigned short len;
    union {
        unsigned char comData[1];
        struct {
            unsigned char sessionIndex;
            unsigned char data[1];
        };
    };
};
struct tag_Anchor {
    tag_Node *head;
    tag_Node *tail;
};

void init_anchor(tag_Anchor *, int, int);
void add_node(tag_Anchor *, tag_Node *);
tag_Node *pop_node(tag_Anchor *);

short com_init(unsigned char, int, int);
void com_term(short);
short com_rcv(short, unsigned short, void *);
short com_snd(short, unsigned short, unsigned short, void *, int);
short __cdecl com_sess(int, int, ...);
unsigned char com_stat(short, unsigned short);
void comm_wrt_task(void);

#endif
