#define WIN32_LEAN_AND_MEAN

#include <H1/Ints.h>

#include <SOURCE/netwinHost.h>

#include <windows.h>

#include <BASE/Misc.h>
#include <PLATFORM/Records.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/remoteRecords.h>

#include <nb30.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

i32 is_netbios_avail(void) {
    NCB ncb;
    memset(&ncb, 0, sizeof(ncb));
    for (gNetbiosLana = 0; gNetbiosLana < MAX_LANA; gNetbiosLana++) {
        memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_command = static_cast<u8>(NETBIOS_COMMAND_PROBE);
        ncb.ncb_lana_num = gNetbiosLana;
        if (Netbios(&ncb) == NRC_ILLCMD)
            break;
    }
    if (gNetbiosLana < MAX_LANA) {
        gNetbiosAvail = 1;
        return 1;
    }
    return 0;
}

extern "C" u16 __cdecl nb_init(u16 maxSessions) {
    u8* buffer;
    NCB ncb;
    i32 jj;
    i32 returnCode;
    if (is_netbios_avail() == 0)
        return 1;
    if (gNetbiosAvail != 0) {
        gNbMaxSess = maxSessions;
        for (jj = 0; jj < NETBIOS_SESSION_COUNT; jj++) {
            gNetStatus[jj] = 0;
            gNbSessLsn[jj] = NETBIOS_INVALID_ID;
            memset(&gNbSessNcb[jj], 0, sizeof(gNbSessNcb[jj]));
        }
        memset(gNbNameBuf, 0, sizeof(gNbNameBuf));
        InitializeCriticalSection(&gNbRcvLock);
        InitializeCriticalSection(&gNbSndLock);
        init_anchor(&gNbRcvQueue, 1, 0);
        init_anchor(&gNbSndQueue, 1, 0);
        init_anchor(&gNbPriorityQueue, 1, 0);
        for (jj = 0; jj < NETBIOS_THREAD_EVENT_COUNT; jj++)
            gNbEvents[jj] = CreateEventA(NULL, TRUE, FALSE, NULL);
        memset(&ncb, 0, sizeof(ncb));
        buffer = static_cast<u8*>(GlobalAlloc(GPTR, NETBIOS_ADAPTER_STATUS_SIZE));
        ncb.ncb_command = NCBASTAT;
        ncb.ncb_length = NETBIOS_ADAPTER_STATUS_SIZE;
        ncb.ncb_buffer = buffer;
        ncb.ncb_lana_num = gNetbiosLana;
        if (Netbios(&ncb) == NRC_ENVNOTDEF) {
            memset(&ncb, 0, sizeof(ncb));
            ncb.ncb_command = NCBRESET;
            ncb.ncb_lana_num = gNetbiosLana;
            ncb.ncb_callname[NETBIOS_RESET_SESSION_LIMIT_INDEX] = NETBIOS_RESET_SESSION_LIMIT;
            ncb.ncb_callname[NETBIOS_RESET_NAME_LIMIT_INDEX] = NETBIOS_RESET_NAME_LIMIT;
            Netbios(&ncb);
        }
        GlobalFree(buffer);
        gNbShutdown = 0;
        return 0;
    }
    return 1;
}

extern "C" void __cdecl nb_term(i32 port) {
    NCB ncb;
    tag_Node* node;
    i32 idx;

    for (idx = 0; idx < NETBIOS_SESSION_COUNT; idx++)
        nb_close_session(idx);
    if (gNbCtlNcb.ncb_cmd_cplt == NRC_PENDING) {
        memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_command = NCBCANCEL;
        ncb.ncb_lana_num = gNetbiosLana;
        ncb.ncb_buffer =
            reinterpret_cast<PUCHAR>(
                &gNbCtlNcb
            );
        Netbios(&ncb);
    }
    if (gNetStatus[gNbMaxSess] & NETBIOS_SESSION_NAME_REGISTERED) {
        memset(&ncb, 0, sizeof(ncb));
        memcpy(ncb.ncb_name, gNbNameBuf[gNbMaxSess].bytes, NCBNAMSZ);
        ncb.ncb_command = NCBDELNAME;
        ncb.ncb_lana_num = gNetbiosLana;
        Netbios(&ncb);
    }
    EnterCriticalSection(&gNbSndLock);
    FREE_NODE_QUEUE(node, &gNbSndQueue);
    FREE_NODE_QUEUE(node, &gNbPriorityQueue);
    LeaveCriticalSection(&gNbSndLock);
    DeleteCriticalSection(&gNbSndLock);
    for (idx = 0; idx < NETBIOS_THREAD_EVENT_COUNT; idx++) {
        CloseHandle(gNbEvents[idx]);
        gNbEvents[idx] = NULL;
    }
    gNbShutdown |= 1;
    SetEvent(gNbEvents[NETBIOS_WAKE_EVENT]);
    EnterCriticalSection(&gNbRcvLock);
    FREE_NODE_QUEUE(node, &gNbRcvQueue);
    LeaveCriticalSection(&gNbRcvLock);
    DeleteCriticalSection(&gNbRcvLock);
}

extern "C" i16 __cdecl nb_rcv(i32 port, u16 maxLength, void* buffer) {
    tag_Node* node;
    i32 size;

    EnterCriticalSection(&gNbRcvLock);
    node = pop_node(&gNbRcvQueue);
    LeaveCriticalSection(&gNbRcvLock);
    if (node) {
        size = __min(node->len, maxLength);
        memcpy(buffer, node->data, size);
        free(node);
        return size;
    }
    return 0;
}

extern "C" i16 __cdecl nb_snd(i32 port, u16 session, u16 length, void* data, i32 priority) {
    tag_Node* node;
    if (session == gNbMaxSess && length == 0) {
        nb_add_name();
        return NRC_GOODRET;
    }
    if (!(gNetStatus[session] & NETBIOS_SESSION_ACTIVE))
        return NRC_SNUMOUT;
    node = static_cast<tag_Node*>(malloc(length + NETBIOS_PACKET_HEADER_SIZE));
    node->len = length;
    node->sessionIndex = session;
    memcpy(node->data, data, length);
    EnterCriticalSection(&gNbSndLock);
    if (priority)
        add_node(&gNbPriorityQueue, node);
    else
        add_node(&gNbSndQueue, node);
    LeaveCriticalSection(&gNbSndLock);
    SetEvent(gNbEvents[NETBIOS_WAKE_EVENT]);
    return NRC_GOODRET;
}

extern "C" i16 __cdecl
nb_sess(i32 port, i32 operation, ...) {
    i32 releaseSource;
    i32 destinationSession;
    NCB lastNcb;
    i32 savedSession;
    va_list args;
    i16 result;
    char* callNameEntry;

    va_start(args, operation);
    switch (operation) {
        case NETBIOS_SESSION_REGISTER:
            callNameEntry = va_arg(args, char*);
            gNetStatus[gNbMaxSess] &= ~NETBIOS_SESSION_ERROR;
            nb_format_name(callNameEntry, gNbNameBuf[gNbMaxSess].bytes);
            memset(&gNbSessNcb[gNbMaxSess], 0, sizeof(NCB));
            memcpy(gNbSessNcb[gNbMaxSess].ncb_name, gNbNameBuf[gNbMaxSess].bytes, NCBNAMSZ);
            gNbSessNcb[gNbMaxSess].ncb_command = NCBADDNAME | ASYNCH;
            gNbSessNcb[gNbMaxSess].ncb_post = nb_add_name_done;
            gNbSessNcb[gNbMaxSess].ncb_cmd_cplt = NRC_PENDING;
            gNbSessNcb[gNbMaxSess].ncb_lana_num = gNetbiosLana;
            result = Netbios(&gNbSessNcb[gNbMaxSess]);
            break;

        case NETBIOS_SESSION_RECEIVE_ANY: {
            destinationSession = va_arg(args, i32);
            if (gNbSessNcb[destinationSession].ncb_cmd_cplt == NRC_PENDING) {
                switch (gNbSessNcb[destinationSession].ncb_command & ~ASYNCH) {
                    case NCBCALL:
                    case NCBDGRECVBC:
                        return NRC_GOODRET;
                    default:
                        break;
                }
                memset(&lastNcb, 0, sizeof(lastNcb));
                lastNcb.ncb_command = NCBCANCEL;
                lastNcb.ncb_lana_num = gNetbiosLana;
                lastNcb.ncb_buffer = reinterpret_cast<PUCHAR>(&gNbSessNcb[destinationSession]);
                Netbios(&lastNcb);
            }
            result = nb_recv_any(destinationSession);
            break;
        }

        case NETBIOS_SESSION_CALL:
            destinationSession = va_arg(args, i32);
            callNameEntry = va_arg(args, char*);
            nb_format_name(callNameEntry, gNbNameBuf[destinationSession].bytes);
            result = nb_call(destinationSession, gNbNameBuf[destinationSession].bytes);
            break;

        case NETBIOS_SESSION_LISTEN_ANY:
            destinationSession = va_arg(args, i32);
            nb_snd(0, gNbMaxSess, 0, NULL, 0);
            result = nb_listen(destinationSession, gNbListenName);
            break;

        case NETBIOS_SESSION_LISTEN:
            destinationSession = va_arg(args, i32);
            callNameEntry = va_arg(args, char*);
            nb_format_name(callNameEntry, gNbNameBuf[destinationSession].bytes);
            result = nb_listen(destinationSession, gNbNameBuf[destinationSession].bytes);
            break;

        case NETBIOS_SESSION_MOVE:
            savedSession = va_arg(args, i32);
            destinationSession = va_arg(args, i32);
            releaseSource = va_arg(args, i32);
            if (savedSession == gNbMaxSess)
                gNbMaxSess = destinationSession;
            if (gNbSessLsn[savedSession] == NETBIOS_INVALID_ID)
                return NRC_GOODRET;
            gNbSessLsn[destinationSession] = gNbSessLsn[savedSession];
            gNetStatus[destinationSession] = gNetStatus[savedSession];
            memcpy(gNbNameBuf[destinationSession].bytes, gNbNameBuf[savedSession].bytes, NCBNAMSZ);
            nb_arm_recv(destinationSession);
            if (releaseSource != 0) {
                gNbSessLsn[savedSession] = NETBIOS_INVALID_ID;
                gNetStatus[savedSession] = 0;
                memset(gNbNameBuf[savedSession].bytes, 0, NCBNAMSZ);
            }
            result = NRC_GOODRET;
            break;

        case NETBIOS_SESSION_CLOSE:
            destinationSession = va_arg(args, i32);
            if (gNbSessNcb[destinationSession].ncb_cmd_cplt == NRC_PENDING) {
                memset(&lastNcb, 0, sizeof(lastNcb));
                lastNcb.ncb_command = NCBCANCEL;
                lastNcb.ncb_lana_num = gNetbiosLana;
                lastNcb.ncb_buffer = reinterpret_cast<PUCHAR>(&gNbSessNcb[destinationSession]);
                Netbios(&lastNcb);
            }
            nb_close_session(destinationSession);
            result = NRC_GOODRET;
            break;

        case NETBIOS_SESSION_CLEAR_CONNECTED:
            destinationSession = va_arg(args, i32);
            gNetStatus[destinationSession] &= ~NETBIOS_SESSION_CONNECTED;
            result = NRC_GOODRET;
            break;

        case NETBIOS_SESSION_GET_NAME:
            destinationSession = va_arg(args, i32);
            callNameEntry = va_arg(args, char*);
            memcpy(callNameEntry, gNbNameBuf[destinationSession].bytes, NCBNAMSZ);
            result = NRC_GOODRET;
            break;

        default:
            return 1;
    }
    if (result == NRC_PENDING)
        result = NRC_GOODRET;
    return result;
}

extern "C" u8 __cdecl nb_stat(i32 port, u16 session) {
    return gNetStatus[session];
}

void nb_thr_ctl(void)
{
    NCB ncbVal;
    b32 sendComplete;
    tag_Node* packet;
    b32 running;
    i32 i;
    u8 result;

    running = true;
    if (WaitForMultipleObjects(NETBIOS_THREAD_EVENT_COUNT, gNbEvents, FALSE, 0) == WAIT_TIMEOUT)
        return;
    if (WaitForSingleObject(gNbEvents[NETBIOS_WAKE_EVENT], 0) == WAIT_OBJECT_0)
        ResetEvent(gNbEvents[NETBIOS_WAKE_EVENT]);
    for (i = 0; i < NETBIOS_SESSION_COUNT; i++) {
        if (WaitForSingleObject(gNbEvents[i + NETBIOS_RECEIVE_EVENT_FIRST], 0) == WAIT_OBJECT_0) {
            ResetEvent(gNbEvents[i + NETBIOS_RECEIVE_EVENT_FIRST]);
            nb_recv_complete(i);
        }
    }
    while (running) {
        EnterCriticalSection(&gNbSndLock);
        packet = pop_node(&gNbPriorityQueue);
        if (packet == NULL)
            packet = pop_node(&gNbSndQueue);
        LeaveCriticalSection(&gNbSndLock);
        if (packet == NULL) {
            running = false;
        } else {
            memset(&gNbCtlNcb, 0, sizeof(gNbCtlNcb));
            gNbCtlNcb.ncb_lsn = gNbSessLsn[packet->sessionIndex];
            if (gNbCtlNcb.ncb_lsn != NETBIOS_INVALID_ID) {
                memcpy(gNbSessBuf, packet->data, packet->len);
                gNbCtlNcb.ncb_buffer = gNbSessBuf;
                gNbCtlNcb.ncb_length = packet->len;
                gNbCtlNcb.ncb_command = NCBSEND;
                gNbCtlNcb.ncb_lana_num = gNetbiosLana;
                sendComplete = false;
                while (!sendComplete) {
                    result = Netbios(&gNbCtlNcb);
                    switch (result) {
                        case NRC_GOODRET:
                            sendComplete = true;
                            break;
                        case NRC_PENDING:
                            H1_ASSERT(0);
                            break;
                        case NRC_SNUMOUT:
                        case NRC_SCLOSED:
                        case NRC_SABORT:
                            gNetStatus[packet->sessionIndex] &= ~NETBIOS_SESSION_ACTIVE;
                            break;
                        default:
                            break;
                    }
                }
            }
            free(packet);
        }
    }
}

void nb_add_name(void) {
    if (gNbCtlNcb.ncb_cmd_cplt != NRC_PENDING) {
        RecordWriter announce;
        WriteNetbiosAnnounce(announce, gNbNameBuf[gNbMaxSess].bytes);
        memcpy(gNbSessBuf, announce.Data(), announce.Size());
        memset(&gNbCtlNcb, 0, sizeof(gNbCtlNcb));
        gNbCtlNcb.ncb_command = NCBDGSENDBC | ASYNCH;
        gNbCtlNcb.ncb_num = gNbLocalNum;
        gNbCtlNcb.ncb_length = announce.Size();
        gNbCtlNcb.ncb_buffer = gNbSessBuf;
        gNbCtlNcb.ncb_lana_num = gNetbiosLana;
        Netbios(&gNbCtlNcb);
    }
}

void __stdcall nb_add_name_done(NCB* ncb)
{
    char buffer[80];
    i32 j;

    H1_ASSERT(ncb == &gNbSessNcb[gNbMaxSess]);
    switch (ncb->ncb_retcode) {
        case NRC_GOODRET:
        case NRC_CANOCCR:
            gNbLocalNum = ncb->ncb_num;
            memcpy(gNbNameBuf[gNbMaxSess].bytes, ncb->ncb_name, NCBNAMSZ);
            gNetStatus[gNbMaxSess] |= NETBIOS_SESSION_NAME_REGISTERED;
            break;
        case NRC_DUPNAME:
        case NRC_INUSE:
        case NRC_NAMCONF:
        case NRC_DUPENV:
            for (j = NCBNAMSZ - 1; j >= 0; j--) {
                ncb->ncb_name[j]++;
                if (ncb->ncb_name[j] != gNbNameBuf[gNbMaxSess].bytes[j])
                    break;
            }
            Netbios(ncb);
            break;
        case NRC_CMDCAN:
            break;
        default:
            wsprintfA(buffer, "Add Name Error %02x\n", ncb->ncb_retcode);
            OutputDebugStringA(buffer);
            gNetStatus[gNbMaxSess] |= NETBIOS_SESSION_ERROR;
            break;
    }
}

u16 nb_recv_any(i32 session) {
    if (gNbSessNcb[session].ncb_cmd_cplt != NRC_PENDING) {
        memset(&gNbSessNcb[session], 0, sizeof(NCB));
        gNbSessNcb[session].ncb_command = NCBDGRECVBC | ASYNCH;
        gNbSessNcb[session].ncb_num = gNbLocalNum;
        gNbSessNcb[session].ncb_length = NETBIOS_PAYLOAD_SIZE;
        gNbSessNcb[session].ncb_buffer = gNbRcvData[session];
        gNbSessNcb[session].ncb_post = nb_recv_any_done;
        Netbios(&gNbSessNcb[session]);
    }
    return gNbSessNcb[session].ncb_cmd_cplt;
}

void __stdcall nb_recv_any_done(NCB* ncb) {
    i32 i;
    u8 hostName[NCBNAMSZ];

    for (i = 0; i < NETBIOS_SESSION_COUNT; i++) {
        if (ncb == &gNbSessNcb[i])
            break;
    }
    if (i >= NETBIOS_SESSION_COUNT)
        return;
    if (gNbSessNcb[i].ncb_retcode == NRC_GOODRET) {
        RecordReader announce(gNbRcvData[i], gNbSessNcb[i].ncb_length);
        if (ReadNetbiosAnnounce(announce, hostName)) {
            memcpy(gNbNameBuf[i].bytes, hostName, NCBNAMSZ);
            nb_call(i, gNbNameBuf[i].bytes);
        } else {
            Netbios(&gNbSessNcb[i]);
        }
    } else if (gNbSessNcb[i].ncb_retcode != NRC_CMDCAN
               && gNbSessNcb[i].ncb_retcode != NRC_CANOCCR) {
        Netbios(&gNbSessNcb[i]);
    }
}

u16 nb_call(i32 session, void* name) {
    memset(&gNbSessNcb[session], 0, sizeof(NCB));
    memcpy(gNbSessNcb[session].ncb_callname, name, NCBNAMSZ);
    memcpy(gNbSessNcb[session].ncb_name, gNbNameBuf[gNbMaxSess].bytes, NCBNAMSZ);
    gNbSessNcb[session].ncb_command = NCBCALL | ASYNCH;
    gNbSessNcb[session].ncb_cmd_cplt = NRC_PENDING;
    gNbSessNcb[session].ncb_post = nb_call_done;
    gNbSessNcb[session].ncb_lana_num = gNetbiosLana;
    gNbCallRetries = 0;
    return Netbios(&gNbSessNcb[session]);
}

u16 nb_listen(i32 session, void* name) {
    memset(&gNbSessNcb[session], 0, sizeof(NCB));
    memcpy(gNbSessNcb[session].ncb_callname, name, NCBNAMSZ);
    memcpy(gNbSessNcb[session].ncb_name, gNbNameBuf[gNbMaxSess].bytes, NCBNAMSZ);
    gNbSessNcb[session].ncb_command = NCBLISTEN | ASYNCH;
    gNbSessNcb[session].ncb_cmd_cplt = NRC_PENDING;
    gNbSessNcb[session].ncb_post = nb_call_done;
    gNbSessNcb[session].ncb_lana_num = gNetbiosLana;
    gNbCallRetries = 0;
    return Netbios(&gNbSessNcb[session]);
}

void __stdcall nb_call_done(NCB* ncb) {
    i32 i;

    for (i = 0; i < NETBIOS_SESSION_COUNT; i++) {
        if (ncb == &gNbSessNcb[i])
            break;
    }
    if (i >= NETBIOS_SESSION_COUNT)
        return;
    switch (gNbSessNcb[i].ncb_retcode) {
        case NRC_GOODRET:
            gNbSessLsn[i] = gNbSessNcb[i].ncb_lsn;
            memcpy(gNbNameBuf[i].bytes, gNbSessNcb[i].ncb_callname, NCBNAMSZ);
            gNetStatus[i] |= NETBIOS_SESSION_ACTIVE | NETBIOS_SESSION_CONNECTED;
            nb_arm_recv(i);
            break;
        case NRC_CMDCAN:
        case NRC_CANOCCR:
            break;
        default:
            gNbCallRetries++;
            if (gNbCallRetries < NETBIOS_CALL_RETRY_LIMIT) {
                Sleep(NETBIOS_CALL_RETRY_DELAY);
                Netbios(&gNbSessNcb[i]);
            }
            break;
    }
}

void nb_arm_recv(i32 session)
{
    u8 result;

    while (1) {
        H1_ASSERT(gNbSessNcb[session].ncb_retcode != NRC_PENDING);
        memset(&gNbSessNcb[session], 0, sizeof(NCB));
        gNbSessNcb[session].ncb_command = NCBRECV | ASYNCH;
        gNbSessNcb[session].ncb_lsn = gNbSessLsn[session];
        gNbSessNcb[session].ncb_buffer = gNbRcvData[session];
        gNbSessNcb[session].ncb_length = NETBIOS_PAYLOAD_SIZE;
        gNbSessNcb[session].ncb_lana_num = gNetbiosLana;
        gNbSessNcb[session].ncb_event = gNbEvents[session + NETBIOS_RECEIVE_EVENT_FIRST];
        result = Netbios(&gNbSessNcb[session]);
        switch (result) {
            case NRC_GOODRET:
            case NRC_SNUMOUT:
            case NRC_SCLOSED:
            case NRC_SABORT:
            case NRC_PENDING:
                return;
            default:
                Sleep(NETBIOS_RECEIVE_RETRY_DELAY);
                continue;
        }
    }
}

void nb_close_session(i32 session) {
    NCB ncb;

    if (gNbSessNcb[session].ncb_cmd_cplt == NRC_PENDING) {
        memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_command = NCBCANCEL;
        ncb.ncb_lana_num = gNetbiosLana;
        ncb.ncb_buffer = reinterpret_cast<PUCHAR>(
            &gNbSessNcb[session]
        );
        Netbios(&ncb);
    }
    if (gNbSessLsn[session] != NETBIOS_INVALID_ID) {
        memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_lsn = gNbSessLsn[session];
        ncb.ncb_command = NCBHANGUP;
        ncb.ncb_lana_num = gNetbiosLana;
        Netbios(&ncb);
        gNetStatus[session] &= ~NETBIOS_SESSION_ACTIVE;
    }
}

void nb_recv_complete(i32 session) {
    tag_Node* node;

    switch (gNbSessNcb[session].ncb_command & ~ASYNCH) {
        case NCBRECV:
            switch (gNbSessNcb[session].ncb_retcode) {
                case NRC_GOODRET:
                    node = static_cast<tag_Node*>(
                        malloc(gNbSessNcb[session].ncb_length + NETBIOS_PACKET_HEADER_SIZE)
                    );
                    if (node != NULL) {
                        node->len = gNbSessNcb[session].ncb_length;
                        node->sessionIndex = session;
                        memcpy(node->data, gNbRcvData[session], node->len);
                        EnterCriticalSection(&gNbRcvLock);
                        add_node(&gNbRcvQueue, node);
                        LeaveCriticalSection(&gNbRcvLock);
                    }
                    nb_arm_recv(session);
                    break;
                case NRC_SNUMOUT:
                case NRC_SCLOSED:
                case NRC_SABORT:
                    gNetStatus[session] &= ~NETBIOS_SESSION_ACTIVE;
                    break;
                default:
                    nb_arm_recv(session);
                    break;
            }
    }
}

void nb_format_name(char* source, u8* destination) {
    u32 i;

    memset(destination, 0, NCBNAMSZ);
    for (i = 0; i < NCBNAMSZ - 1 && *source != '\0'; i++, source++)
        destination[i] = *source;
    for (; i < NCBNAMSZ - 1; i++)
        destination[i] = ' ';
}

i32 gOldNetwinWord;
u8 gNbCallRetries = 0;
u8 gNetbiosAvail = 0;
u8 gNbShutdown = 0;
u8 gNbMaxSess = 255;
u8 gNetStatus[7] = {0, 0, 0, 0, 0, 0, 0};
// The protocol version is part of the group name, so the game never pairs
// with other versions on the network.
char* gNbGroupName = "Empire TE1 ";
u8* gNbListenName =
    reinterpret_cast<u8*>(const_cast<char*>("*"));
tag_Anchor gNbPriorityQueue;
u8 gNbSessLsn[7];
u8 gNbRcvData[7][0x1000];
NetbiosName gNbNameBuf[7];
u8 gNbSessBuf[0xfd0];
u8 gNetwinDeadName[48];
NCB gNbSessNcb[7];
NCB gNbCtlNcb;
u8 gNbLocalNum;
tag_Anchor gNbRcvQueue;
tag_Anchor gNbSndQueue;
CRITICAL_SECTION gNbRcvLock;
HANDLE gNbEvents[9];
CRITICAL_SECTION gNbSndLock;
u8 gNetbiosLana;
