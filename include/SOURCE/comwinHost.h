#ifndef HOMM1_SOURCE_COMWINHOST_H
#define HOMM1_SOURCE_COMWINHOST_H

// The Windows serial transport's port state. Only comwin.cpp includes it.

#include <windows.h>

#include <SOURCE/comwin.h>

struct ComPortState {
    HANDLE handle;
    char reserved04[4];
    DCB savedState;
    COMMTIMEOUTS savedTimeouts;
    char reserved38[0x18];
    tag_Anchor normalQueue;
    tag_Anchor priorityQueue;
};

extern ComPortState gComPorts[];

#endif
