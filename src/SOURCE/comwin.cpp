#include <H1/Ints.h>

#include <SOURCE/comwinHost.h>
#include <SOURCE/KB.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
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
    char errorName[COM_ERROR_NAME_SIZE];
    char message[COM_ERROR_MESSAGE_SIZE];
    DWORD errorCode;

    errorCode = GetLastError();

    switch (errorCode) {
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

    sprintf(
        message,
        "Communications error on function '%s'\n\nWin95 Error Code: %d\nWin95 Error Meaning: "
        "%s\n\n",
        function,
        errorCode,
        errorName
    );
    strcat(message, "Suggested solutions:");
    strcat(message, "\n  1) Make sure all cables are firmly connected.");
    strcat(message, "\n  2) Reboot computer.");
    strcat(
        message,
        "\n  3) Check to make sure you have the correct COM port setting in 'CONFIG'. (The 3rd "
        "button down on the screen where you choose Host or Guest.)"
    );
    strcat(message, "\n  4) Consider lowering the BAUD rate in 'CONFIG' to 19200 or 9600.");
    ShutDown(message);
}

i16 com_init(u8 portNumber, i32 baudRate, i32 dsrFlowControl) {
    i32 err;
    i32 slot;
    BOOL rv;
    DCB state;
    char portName[12];
    COMMTIMEOUTS portTimeouts;

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
    state.fOutxDsrFlow = dsrFlowControl != 0;
    state.fDtrControl = DTR_CONTROL_ENABLE;
    state.fInX = FALSE;
    state.fOutX = FALSE;
    state.fNull = FALSE;
    state.fRtsControl = RTS_CONTROL_HANDSHAKE;
    state.fAbortOnError = TRUE;
    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    rv = SetupComm(gComPorts[slot].handle, COM_RECEIVE_BUFFER_SIZE, COM_TRANSMIT_BUFFER_SIZE);
    if (!rv)
        ShutdownComError("Initialize communications paramaters");
    rv = SetCommState(gComPorts[slot].handle, &state);
    if (!rv)
        ShutdownComError("Configure communications device");
    portTimeouts.ReadIntervalTimeout = MAXDWORD;
    portTimeouts.ReadTotalTimeoutMultiplier = portTimeouts.ReadTotalTimeoutConstant = 0;
    portTimeouts.WriteTotalTimeoutMultiplier = portTimeouts.WriteTotalTimeoutConstant = 0;
    rv = SetCommTimeouts(gComPorts[slot].handle, &portTimeouts);
    if (!rv)
        ShutdownComError("Set communications timeouts");
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
        FREE_NODE_QUEUE(node, &gComPorts[port].normalQueue);
        FREE_NODE_QUEUE(node, &gComPorts[port].priorityQueue);
    }
}

i16 com_rcv(i16 port, u16 requested, void* buffer) {
    DWORD err;
    COMSTAT status;
    u32 n;
    DWORD bytesRead;
    BOOL success;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        success = ClearCommError(gComPorts[port].handle, &err, &status);
        if (!success)
            ShutdownComError("Clear communications error queue");
        n = requested < status.cbInQue ? requested : status.cbInQue;
        if (n) {
            success = ReadFile(gComPorts[port].handle, buffer, n, &bytesRead, NULL);
            if (!success)
                ShutdownComError("Read communications data");
            return bytesRead;
        }
    }
    return 0;
}

i16 com_snd(i16 port, u16 session, u16 length, void* data, i32 priority) {
    tag_Node* node;
    BOOL success;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE) {
        if (!length) {
            success = SetCommBreak(gComPorts[port].handle);
            if (!success)
                ShutdownComError("Set communications break");
            Sleep(COM_BREAK_DELAY);
            success = ClearCommBreak(gComPorts[port].handle);
            if (!success)
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

i16 __cdecl com_sess(i32 port, i32 operation, ...) {
    return 0;
}

u8 com_stat(i16 port, u16 session) {
    DWORD modemStatus;

    if (gComPorts[port].handle != INVALID_HANDLE_VALUE
        && GetCommModemStatus(gComPorts[port].handle, &modemStatus) && (modemStatus & MS_CTS_ON)
        && (modemStatus & MS_RLSD_ON))
        return 1;
    return 0;
}

void comm_wrt_task(void) {
    ComPortState* comPort;
    tag_Node* packetNode;
    BOOL success;
    u32 totalWritten;
    DWORD sizeWritten;

    comPort = gComPorts;
    while (comPort->handle != INVALID_HANDLE_VALUE) {
        packetNode = pop_node(&comPort->priorityQueue);
        if (!packetNode)
            packetNode = pop_node(&comPort->normalQueue);
        if (!packetNode)
            return;
        totalWritten = 0;
        while (comPort->handle != INVALID_HANDLE_VALUE && totalWritten < packetNode->len) {
            success = WriteFile(
                comPort->handle,
                &packetNode->comData[totalWritten],
                packetNode->len - totalWritten,
                &sizeWritten,
                NULL
            );
            if (!success)
                ShutdownComError("Write communications data");
            totalWritten += sizeWritten;
        }
        free(packetNode);
    }
}

ComPortState gComPorts[COM_PORT_COUNT];
