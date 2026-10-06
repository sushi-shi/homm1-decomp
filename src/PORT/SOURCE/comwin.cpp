// The native counterpart of src/SOURCE/comwin.cpp, the serial (modem and
// direct cable) transport. Serial play is not available in the port yet;
// opening a port ends the game with an error, as the original did when it
// could not open one.

#include <H1/Ints.h>

#include <SOURCE/comwin.h>

#include <SOURCE/KB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void add_node(tag_Anchor* anchor, tag_Node* node) {
    node->prev = node->next = NULL;
    if (anchor->tail) {
        anchor->tail->next = node;
        node->prev = anchor->tail;
        anchor->tail = node;
    } else {
        anchor->tail = node;
        anchor->head = node;
    }
}

tag_Node* pop_node(tag_Anchor* anchor) {
    tag_Node* node = anchor->head;
    if (node)
        anchor->head = node->next;
    if (!anchor->head)
        anchor->tail = NULL;
    return node;
}

void init_anchor(tag_Anchor* anchor, i32, i32) {
    anchor->head = NULL;
    anchor->tail = NULL;
}

void ShutdownComError(char* function) {
    char message[COM_ERROR_MESSAGE_SIZE];
    snprintf(message, sizeof(message), "%s: serial connections are not supported by this port.",
             function);
    ShutDown(message);
}

i16 com_init(u8, i32, i32) {
    ShutdownComError(const_cast<char*>("com_init"));
    return -1;
}

void com_term(i16) {}

i16 com_rcv(i16, u16, void*) {
    return 0;
}

i16 com_snd(i16, u16, u16, void*, i32) {
    return -1;
}

i16 com_sess(i32, i32, ...) {
    return -1;
}

u8 com_stat(i16, u16) {
    return 0;
}

void comm_wrt_task(void) {}
