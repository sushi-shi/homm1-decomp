// Serial transport; Buka 2.1 SOURCE/comwin correspondence. HoMM1 keeps seven
// port slots; Windows 95 1.1 adds ShutdownComError at each serial failure.

#include <match.h>

#include <SOURCE/comwin.h>
#include <SOURCE/KB.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
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
    COM_ERROR_NAME_SIZE = 100,
    COM_ERROR_MESSAGE_SIZE = 500,
    COM_RECEIVE_BUFFER_SIZE = 0x2000,
    COM_TRANSMIT_BUFFER_SIZE = 0x1000,
    COM_BREAK_DELAY = 500,
    COM_NODE_HEADER_SIZE = 10
};

extern ComPortState gComPorts[];

VA(0x00437270, 0x6b)
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

VA(0x004372db, 0x49)
tag_Node* pop_node(tag_Anchor* anchor) {
    tag_Node* node = anchor->head;

    if (node)
        anchor->head = node->next;
    if (!anchor->head)
        anchor->tail = NULL;
    return node;
}

VA(0x00437324, 0x23)
void init_anchor(tag_Anchor* anchor, i32, i32) {
    anchor->head = NULL;
    anchor->tail = NULL;
}

// HoMM2 Buka 2.1 ShutdownComError; literals and error cases verified in 1.1.
VA(0x00437347, 0x395)
void ShutdownComError(char* function) {
    DWORD error;
    char errorName[COM_ERROR_NAME_SIZE];
    char message[COM_ERROR_MESSAGE_SIZE];

    error = GetLastError();

    switch (error) {
        case ERROR_INVALID_FUNCTION:
            strcpy(errorName, "ERROR_INVALID_FUNCTION    ");
            break;
        case ERROR_FILE_NOT_FOUND:
            strcpy(errorName, "ERROR_FILE_NOT_FOUND      ");
            break;
        case ERROR_PATH_NOT_FOUND:
            strcpy(errorName, "ERROR_PATH_NOT_FOUND      ");
            break;
        case ERROR_TOO_MANY_OPEN_FILES:
            strcpy(errorName, "ERROR_TOO_MANY_OPEN_FILES ");
            break;
        case ERROR_ACCESS_DENIED:
            strcpy(errorName, "ERROR_ACCESS_DENIED       ");
            break;
        case ERROR_INVALID_HANDLE:
            strcpy(errorName, "ERROR_INVALID_HANDLE      ");
            break;
        case ERROR_ARENA_TRASHED:
            strcpy(errorName, "ERROR_ARENA_TRASHED       ");
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            strcpy(errorName, "ERROR_NOT_ENOUGH_MEMORY   ");
            break;
        case ERROR_INVALID_BLOCK:
            strcpy(errorName, "ERROR_INVALID_BLOCK       ");
            break;
        case ERROR_BAD_ENVIRONMENT:
            strcpy(errorName, "ERROR_BAD_ENVIRONMENT     ");
            break;
        case ERROR_BAD_FORMAT:
            strcpy(errorName, "ERROR_BAD_FORMAT          ");
            break;
        case ERROR_INVALID_ACCESS:
            strcpy(errorName, "ERROR_INVALID_ACCESS      ");
            break;
        case ERROR_INVALID_DATA:
            strcpy(errorName, "ERROR_INVALID_DATA        ");
            break;
        case ERROR_INVALID_DRIVE:
            strcpy(errorName, "ERROR_INVALID_DRIVE       ");
            break;
        case ERROR_CURRENT_DIRECTORY:
            strcpy(errorName, "ERROR_CURRENT_DIRECTORY   ");
            break;
        case ERROR_NOT_SAME_DEVICE:
            strcpy(errorName, "ERROR_NOT_SAME_DEVICE     ");
            break;
        case ERROR_NO_MORE_FILES:
            strcpy(errorName, "ERROR_NO_MORE_FILES       ");
            break;
        case ERROR_ALREADY_EXISTS:
            strcpy(errorName, "ERROR_ALREADY_EXISTS      ");
            break;
        default:
            strcpy(errorName, "UNKNOWN_ERROR             ");
            break;
    }

    sprintf(message, "Communications error on function '%s'\n\nWin95 Error Code: %d\nWin95 Error Meaning: %s\n\n", function, error, errorName);
    strcat(message, "Suggested solutions:");
    strcat(message, "\n  1) Make sure all cables are firmly connected.");
    strcat(message, "\n  2) Reboot computer.");
    strcat(message, "\n  3) Check to make sure you have the correct COM port setting in 'CONFIG'. (The 3rd button down on the screen where you choose Host or Guest.)");
    strcat(message, "\n  4) Consider lowering the BAUD rate in 'CONFIG' to 19200 or 9600.");
    ShutDown(message);
}

VA(0x004376dc, 0x358)
i16 com_init(u8 portNumber, i32 baudRate, i32 useDtr) {
    i32 error; // Unused, as in Buka; retail still reserves its slot.
    i32 slot;
    BOOL commStatus;
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
    if (gComPorts[slot].handle == INVALID_HANDLE_VALUE) {
        sprintf(gText, "Opening COM%d", portNumber);
        ShutdownComError(gText);
        return -1;
    }
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
    commStatus = SetupComm(gComPorts[slot].handle, COM_RECEIVE_BUFFER_SIZE, COM_TRANSMIT_BUFFER_SIZE);
    if (!commStatus)
        ShutdownComError("Initialize communications paramaters");
    commStatus = SetCommState(gComPorts[slot].handle, &state);
    if (!commStatus)
        ShutdownComError("Configure communications device");
    commTimeouts.ReadIntervalTimeout = MAXDWORD;
    commTimeouts.ReadTotalTimeoutMultiplier = commTimeouts.ReadTotalTimeoutConstant = 0;
    commTimeouts.WriteTotalTimeoutMultiplier = commTimeouts.WriteTotalTimeoutConstant = 0;
    commStatus = SetCommTimeouts(gComPorts[slot].handle, &commTimeouts);
    if (!commStatus)
        ShutdownComError("Set communications timeouts");
    init_anchor(&gComPorts[slot].normalQueue, 1, 0);
    init_anchor(&gComPorts[slot].priorityQueue, 1, 0);
    return slot;
}

VA(0x00437a34, 0x111)
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

VA(0x00437b45, 0xd9)
i16 com_rcv(i16 port, u16 requested, void* buffer) {
    DWORD currentError;
    COMSTAT status;
    u32 currentBytesRead;
    DWORD nRead;
    BOOL ioResult;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        ioResult = ClearCommError(gComPorts[port].handle, &currentError, &status);
        if (!ioResult)
            ShutdownComError("Clear communications error queue");
        if (status.cbInQue <= requested)
            currentBytesRead = status.cbInQue;
        else
            currentBytesRead = requested;
        if (currentBytesRead) {
            ioResult = ReadFile(gComPorts[port].handle, buffer, currentBytesRead, &nRead, NULL);
            if (!ioResult)
                ShutdownComError("Read communications data");
            return static_cast<i16>(nRead);
        }
    }
    return 0;
}

VA(0x00437c1e, 0x147)
i16 com_snd(i16 port, u16, u16 length, void* data, i32 priority) {
    tag_Node* node;
    BOOL result;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        if (!length) {
            result = SetCommBreak(gComPorts[port].handle);
            if (!result)
                ShutdownComError("Set communications break");
            Sleep(COM_BREAK_DELAY);
            result = ClearCommBreak(gComPorts[port].handle);
            if (!result)
                ShutdownComError("Clear communications break");
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

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00437d65, 0x13)
i16 __cdecl com_sess(i32, i32, ...) {
    return 0;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00437d78, 0x66)
u8 com_stat(i16 port, u16) {
    DWORD modemStatus;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE
        && GetCommModemStatus(gComPorts[port].handle, &modemStatus) && (modemStatus & MS_CTS_ON)
        && (modemStatus & MS_RLSD_ON))
        return 1;
    return 0;
}

VA(0x00437dde, 0xe7)
void comm_wrt_task(void) {
    ComPortState* portState;
    tag_Node* node;
    BOOL result;
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
            result = WriteFile(
                    portState->handle,
                    &node->comData[totalWritten],
                    node->len - totalWritten,
                    &sizeWritten,
                    NULL
                );
            if (!result)
                ShutdownComError("Write communications data");
            totalWritten += sizeWritten;
        }
        free(node);
    }
}

// comwin owns retail .bss 0x004ca918-0x004cabb7.
DATA(0x004c29b0)
ComPortState gComPorts[COM_PORT_COUNT];
