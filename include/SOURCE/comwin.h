#ifndef HOMM1_SOURCE_COMWIN_H
#define HOMM1_SOURCE_COMWIN_H

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
