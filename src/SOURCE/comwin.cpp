#include <H1/Ints.h>

#include <SOURCE/comwin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string.h>

struct ComPortState {
    HANDLE handle;
    char reserved04[4];
    DCB savedState;
    COMMTIMEOUTS savedTimeouts;
    char reserved38[0x18];
    tag_Anchor normalQueue;
    tag_Anchor priorityQueue;
};

enum ComConstant {
    COM_PORT_COUNT = 7,
    COM_RECEIVE_BUFFER_SIZE = 0x2000,
    COM_TRANSMIT_BUFFER_SIZE = 0x1000,
    COM_BREAK_DELAY = 500,
    COM_NODE_HEADER_SIZE = 10
};

extern ComPortState gComPorts[];

void add_node(tag_Anchor* anchor, tag_Node* node) {
    node->prev = node->next = NULL;
    if (anchor->tail) {
        anchor->tail->next = node;
        node->prev = anchor->tail;
        anchor->tail = node;
    } else {
        anchor->tail = node;
        anchor->head = anchor->tail;
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

i16 com_init(u8 portNumber, i32 baudRate, i32 useDtr) {
    i32 error;
    i32 slot;
    DCB state;
    char portName[12];
    COMMTIMEOUTS commTimeouts;

    for (slot = 0; slot < COM_PORT_COUNT; slot++)
        gComPorts[slot].handle = INVALID_HANDLE_VALUE;
    for (slot = 0; slot < COM_PORT_COUNT; slot++) {
        if (gComPorts[slot].handle == INVALID_HANDLE_VALUE)
            break;
    }
    if (slot >= COM_PORT_COUNT)
        return -1;
    wsprintfA(portName, "COM%d", portNumber);
    gComPorts[slot].handle =
        CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (gComPorts[slot].handle == INVALID_HANDLE_VALUE)
        return -1;
    state.DCBlength = sizeof(DCB);
    GetCommState(gComPorts[slot].handle, &state);
    gComPorts[slot].savedState = state;
    GetCommTimeouts(gComPorts[slot].handle, &gComPorts[slot].savedTimeouts);
    switch (baudRate) {
        case COM_BAUD_2400:
            state.BaudRate = CBR_2400;
            break;
        case COM_BAUD_4800:
            state.BaudRate = CBR_4800;
            break;
        case COM_BAUD_9600:
            state.BaudRate = CBR_9600;
            break;
        case COM_BAUD_19200:
            state.BaudRate = CBR_19200;
            break;
        case COM_BAUD_38400:
            state.BaudRate = CBR_38400;
            break;
        default:
            state.BaudRate = baudRate;
            break;
    }
    state.fParity = FALSE;
    state.fOutxCtsFlow = TRUE;
    if (useDtr)
        state.fOutxDsrFlow = TRUE;
    else
        state.fOutxDsrFlow = FALSE;
    state.fDtrControl = DTR_CONTROL_ENABLE;
    state.fInX = FALSE;
    state.fOutX = FALSE;
    state.fNull = FALSE;
    state.fRtsControl = RTS_CONTROL_HANDSHAKE;
    state.fAbortOnError = TRUE;
    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    SetupComm(gComPorts[slot].handle, COM_RECEIVE_BUFFER_SIZE, COM_TRANSMIT_BUFFER_SIZE);
    SetCommState(gComPorts[slot].handle, &state);
    commTimeouts.ReadIntervalTimeout = MAXDWORD;
    commTimeouts.ReadTotalTimeoutMultiplier = commTimeouts.ReadTotalTimeoutConstant = 0;
    commTimeouts.WriteTotalTimeoutMultiplier = commTimeouts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(gComPorts[slot].handle, &commTimeouts);
    init_anchor(&gComPorts[slot].normalQueue, 1, 0);
    init_anchor(&gComPorts[slot].priorityQueue, 1, 0);
    return slot;
}

void com_term(i16 port) {
    tag_Node* node;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        SetCommState(gComPorts[port].handle, &gComPorts[port].savedState);
        SetCommTimeouts(gComPorts[port].handle, &gComPorts[port].savedTimeouts);
        CloseHandle(gComPorts[port].handle);
        gComPorts[port].handle = INVALID_HANDLE_VALUE;
        while ((node = pop_node(&gComPorts[port].normalQueue)) != NULL)
            free(node);
        while ((node = pop_node(&gComPorts[port].priorityQueue)) != NULL)
            free(node);
    }
}

i16 com_rcv(i16 port, u16 requested, void* buffer) {
    DWORD currentError;
    COMSTAT status;
    u32 currentBytesRead;
    DWORD nRead;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        ClearCommError(gComPorts[port].handle, &currentError, &status);
        if (status.cbInQue <= requested)
            currentBytesRead = status.cbInQue;
        else
            currentBytesRead = requested;
        if (currentBytesRead) {
            if (ReadFile(gComPorts[port].handle, buffer, currentBytesRead, &nRead, NULL))
                return static_cast<i16>(nRead);
        }
    }
    return 0;
}

i16 com_snd(i16 port, u16, u16 length, void* data, i32 priority) {
    tag_Node* node;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        if (!length) {
            SetCommBreak(gComPorts[port].handle);
            Sleep(COM_BREAK_DELAY);
            ClearCommBreak(gComPorts[port].handle);
            return 0;
        }
        node = static_cast<tag_Node*>(malloc(length + COM_NODE_HEADER_SIZE));
        if (node) {
            node->len = length;
            memcpy(node->comData, data, length);
            if (priority)
                add_node(&gComPorts[port].priorityQueue, node);
            else
                add_node(&gComPorts[port].normalQueue, node);
            return 0;
        }
    }
    return 1;
}

i16 __cdecl com_sess(i32, i32, ...) {
    return 0;
}

u8 com_stat(i16 port, u16) {
    DWORD modemStatus;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE
        && GetCommModemStatus(gComPorts[port].handle, &modemStatus) && (modemStatus & MS_CTS_ON)
        && (modemStatus & MS_RLSD_ON))
        return 1;
    return 0;
}

void comm_wrt_task(void) {
    ComPortState* portState;
    tag_Node* node;
    u32 totalWritten;
    DWORD sizeWritten;

    portState = gComPorts;
    while (portState->handle != INVALID_HANDLE_VALUE) {
        node = pop_node(&portState->priorityQueue);
        if (!node)
            node = pop_node(&portState->normalQueue);
        if (!node)
            return;
        totalWritten = 0;
        while (portState->handle != INVALID_HANDLE_VALUE && node->len > totalWritten) {
            if (WriteFile(
                    portState->handle,
                    &node->comData[totalWritten],
                    node->len - totalWritten,
                    &sizeWritten,
                    NULL
                ))
                totalWritten += sizeWritten;
        }
        free(node);
    }
}

ComPortState gComPorts[COM_PORT_COUNT];
