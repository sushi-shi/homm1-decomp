#ifndef HOMM1_SOURCE_COMWIN_H
#define HOMM1_SOURCE_COMWIN_H

struct tag_Node {
    tag_Node *prev;
    tag_Node *next;
    unsigned short len;
    unsigned char sessionIndex;
    unsigned char data[1];
};
struct tag_Anchor {
    tag_Node *head;
    tag_Node *tail;
};

void init_anchor(tag_Anchor *, int, int);
void add_node(tag_Anchor *, tag_Node *);
short com_rcv(short, unsigned short, void *);
short com_snd(short, unsigned short, unsigned short, void *, int);

#endif
