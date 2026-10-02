// Serial transport; Buka 2.1 SOURCE/comwin correspondence. HoMM1 keeps seven
// port slots and does not report Win32 failures.

#include <match.h>

#include <SOURCE/comwin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string.h>

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

enum ComConstant {
    COM_PORT_COUNT = 7,
    COM_RECEIVE_BUFFER_SIZE = 0x2000,
    COM_TRANSMIT_BUFFER_SIZE = 0x1000,
    COM_BREAK_DELAY = 500,
    COM_NODE_HEADER_SIZE = 10
};

extern ComPortState gComPorts[];

VA(0x00472c60, 0x6b)
void add_node(tag_Anchor *anchor, tag_Node *node)
{
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

VA(0x00472ccb, 0x49)
tag_Node *pop_node(tag_Anchor *anchor)
{
    tag_Node *node = anchor->head;

    if (node)
        anchor->head = node->next;
    if (!anchor->head)
        anchor->tail = NULL;
    return node;
}

VA(0x00472d14, 0x23)
void init_anchor(tag_Anchor *anchor, int, int)
{
    anchor->head = NULL;
    anchor->tail = NULL;
}

VA(0x00472d37, 0x2e5)
short com_init(unsigned char portNumber, int baudRate, int useDtr)
{
    int error; // Unused, as in Buka; retail still reserves its slot.
    int slot;
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
    case 1:
        state.BaudRate = 2400;
        break;
    case 2:
        state.BaudRate = 4800;
        break;
    case 3:
        state.BaudRate = 9600;
        break;
    case 4:
        state.BaudRate = 19200;
        break;
    case 5:
        state.BaudRate = 38400;
        break;
    default:
        state.BaudRate = baudRate;
        break;
    }
    state.fParity = 0;
    state.fOutxCtsFlow = 1;
    if (useDtr)
        state.fOutxDsrFlow = 1;
    else
        state.fOutxDsrFlow = 0;
    state.fDtrControl = DTR_CONTROL_ENABLE;
    state.fInX = 0;
    state.fOutX = 0;
    state.fNull = 0;
    state.fRtsControl = RTS_CONTROL_HANDSHAKE;
    state.fAbortOnError = 1;
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

VA(0x0047301c, 0x111)
void com_term(short port)
{
    tag_Node *node;

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

VA(0x0047312d, 0xad)
short com_rcv(short port, unsigned short requested, void *buffer)
{
    DWORD currentError;
    COMSTAT status;
    unsigned long currentBytesRead;
    DWORD nRead;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        ClearCommError(gComPorts[port].handle, &currentError, &status);
        if (status.cbInQue <= requested)
            currentBytesRead = status.cbInQue;
        else
            currentBytesRead = requested;
        if (currentBytesRead) {
            if (ReadFile(gComPorts[port].handle, buffer, currentBytesRead, &nRead, NULL))
                return (short)nRead;
        }
    }
    return 0;
}

VA(0x004731da, 0x113)
short com_snd(short port, unsigned short, unsigned short length, void *data, int priority)
{
    tag_Node *node;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        if (!length) {
            SetCommBreak(gComPorts[port].handle);
            Sleep(COM_BREAK_DELAY);
            ClearCommBreak(gComPorts[port].handle);
            return 0;
        }
        node = (tag_Node *)malloc(length + COM_NODE_HEADER_SIZE);
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

VA(0x004732ed, 0x13)
short __cdecl com_sess(int, int, ...)
{
    return 0;
}

VA(0x00473300, 0x66)
unsigned char com_stat(short port, unsigned short)
{
    DWORD modemStatus;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE
        && GetCommModemStatus(gComPorts[port].handle, &modemStatus)
        && (modemStatus & MS_CTS_ON) && (modemStatus & MS_RLSD_ON))
        return 1;
    return 0;
}

VA(0x00473366, 0xd5)
void comm_wrt_task(void)
{
    ComPortState *portState;
    tag_Node *node;
    unsigned long totalWritten;
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
            if (WriteFile(portState->handle, &node->comData[totalWritten], node->len - totalWritten,
                          &sizeWritten, NULL))
                totalWritten += sizeWritten;
        }
        free(node);
    }
}

// comwin owns retail .bss 0x004ca918-0x004cabb7.
DATA(0x004ca918)
ComPortState gComPorts[COM_PORT_COUNT];
