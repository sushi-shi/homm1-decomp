// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <SOURCE/netwin.h>

#include <windows.h>

#include <H1/All.h>
#include <SOURCE/netwinRuntime.h>

#include <nb30.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <BASE/Misc.h>

// Compiler line-base word for netlo.cpp's ProcessAssert sites.
DATA(0x0048f214) short gNbThrCtlLineBase;

// donor PoL RVA 0x000a6be0; preferred Buka symbol ?is_netbios_avail@@YIHXZ
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.601182;margin=0.610067;shape=0.521;size=0.881;calls=1.000;alternate=pol20:int is_netbios_avail(void)@0x000a6be0
VA(0x00413c40, 0xa8)
int is_netbios_avail(void) {
    NCB ncb;
    memset(&ncb, 0, sizeof(ncb));
    for (gNetbiosLana = 0; gNetbiosLana < MAX_LANA; gNetbiosLana++) {
        memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_command = static_cast<unsigned char>(NETBIOS_COMMAND_PROBE);
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

// donor PoL RVA 0x000a6c88; preferred Buka symbol _nb_init
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.569810;margin=0.557141;shape=0.568;size=0.810;calls=0.714;alternate=pol20:@nb_init@8@0x000a6c88
VA(0x00413ce8, 0x1b2)
H1_C_LINKAGE unsigned short __cdecl nb_init(unsigned short maxSessions, unsigned short maxNames) {
    unsigned char* statusBuf;
    NCB ncb;
    int i;
    // Retained unused result local from the PoL donor; retail reserves its frame word.
    int returnCode;
    if (is_netbios_avail() == 0)
        return 1;
    if (gNetbiosAvail != 0) {
        gNbMaxSess = static_cast<unsigned char>(maxSessions);
        for (i = 0; i < static_cast<int>(NETBIOS_SESSION_COUNT); i++) {
            gNetStatus[i] = 0;
            gNbSessLsn[i] = static_cast<unsigned char>(NETBIOS_INVALID_ID);
            memset(&gNbSessNcb[i], 0, sizeof(gNbSessNcb[i]));
        }
        memset(gNbNameBuf, 0, sizeof(gNbNameBuf));
        InitializeCriticalSection(&gNbRcvLock);
        InitializeCriticalSection(&gNbSndLock);
        init_anchor(&gNbRcvQueue, 1, 0);
        init_anchor(&gNbSndQueue, 1, 0);
        init_anchor(&gNbFreeQueue, 1, 0);
        for (i = 0; i < static_cast<int>(NETBIOS_THREAD_EVENT_COUNT); i++)
            gNbEvents[i] = CreateEventA(0, 1, 0, 0);
        memset(&ncb, 0, sizeof(ncb));
        statusBuf = static_cast<unsigned char*>(
            GlobalAlloc(GPTR, static_cast<int>(NETBIOS_ADAPTER_STATUS_SIZE))
        );
        ncb.ncb_command = NCBASTAT;
        ncb.ncb_length = static_cast<unsigned short>(NETBIOS_ADAPTER_STATUS_SIZE);
        ncb.ncb_buffer = statusBuf;
        ncb.ncb_lana_num = gNetbiosLana;
        if (Netbios(&ncb) == NRC_ENVNOTDEF) {
            memset(&ncb, 0, sizeof(ncb));
            ncb.ncb_command = NCBRESET;
            ncb.ncb_lana_num = gNetbiosLana;
            ncb.ncb_callname[static_cast<int>(NETBIOS_RESET_SESSION_LIMIT_INDEX)] =
                static_cast<unsigned char>(NETBIOS_RESET_SESSION_LIMIT);
            ncb.ncb_callname[static_cast<int>(NETBIOS_RESET_NAME_LIMIT_INDEX)] =
                static_cast<unsigned char>(NETBIOS_RESET_NAME_LIMIT);
            Netbios(&ncb);
        }
        GlobalFree(statusBuf);
        gNbShutdown = 0;
        return 0;
    }
    return 1;
}

// Buka netwin.cpp:149-193; HoMM1 drains the free queue and keeps the
// cancel/delete-name sequence on one stack NCB.
VA(0x00413e9a, 0x1f0)
H1_C_LINKAGE void __cdecl nb_term(void)
{
    NCB ncb;
    tag_Node *node;
    int i;

    for (i = 0; i < static_cast<int>(NETBIOS_SESSION_COUNT); i++)
        nb_close_session(i);
    if (gNbCtlNcb.ncb_cmd_cplt == NRC_PENDING) {
        memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_command = NCBCANCEL;
        ncb.ncb_lana_num = gNetbiosLana;
        ncb.ncb_buffer = reinterpret_cast<PUCHAR>(&gNbCtlNcb);
        Netbios(&ncb);
    }
    if (gNetStatus[gNbMaxSess] & static_cast<int>(NETBIOS_SESSION_NAME_REGISTERED)) {
        memset(&ncb, 0, sizeof(ncb));
        memcpy(ncb.ncb_name, gNbNameBuf[gNbMaxSess].bytes, NCBNAMSZ);
        ncb.ncb_command = NCBDELNAME;
        ncb.ncb_lana_num = gNetbiosLana;
        Netbios(&ncb);
    }
    EnterCriticalSection(&gNbSndLock);
    while ((node = pop_node(&gNbSndQueue)) != 0)
        free(node);
    while ((node = pop_node(&gNbFreeQueue)) != 0)
        free(node);
    LeaveCriticalSection(&gNbSndLock);
    DeleteCriticalSection(&gNbSndLock);
    for (i = 0; i < static_cast<int>(NETBIOS_THREAD_EVENT_COUNT); i++) {
        CloseHandle(gNbEvents[i]);
        gNbEvents[i] = 0;
    }
    gNbShutdown |= 1;
    SetEvent(gNbEvents[0]);
    EnterCriticalSection(&gNbRcvLock);
    while ((node = pop_node(&gNbRcvQueue)) != 0)
        free(node);
    LeaveCriticalSection(&gNbRcvLock);
    DeleteCriticalSection(&gNbRcvLock);
}

// Buka netwin.cpp:195-217; HoMM1 keeps the unused leading argument.
VA(0x0041408a, 0x90)
H1_C_LINKAGE unsigned short __cdecl nb_rcv(int, unsigned short len, void *buffer)
{
    tag_Node *node;
    int size;

    EnterCriticalSection(&gNbRcvLock);
    node = pop_node(&gNbRcvQueue);
    LeaveCriticalSection(&gNbRcvLock);
    if (node) {
        size = node->len < len ? node->len : len;
        memcpy(buffer, node->data, size);
        free(node);
        return size;
    }
    return 0;
}

// donor PoL RVA 0x000a7186; preferred Buka symbol _nb_snd
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.424579;margin=0.396789;shape=0.244;size=0.738;calls=0.875;alternate=pol20:@nb_snd@12@0x000a7186
VA(0x0041411a, 0x104)
// Retail has an unused leading argument and an explicit queue selection argument.
// Their stack positions are proven by all four retail call sites.
H1_C_LINKAGE short __cdecl
nb_snd(int, unsigned short session, unsigned short len, void* data, int queueToFree) {
    tag_Node* node;
    if (gNbMaxSess == session && len == 0) {
        nb_add_name();
        return 0;
    }
    if (!(gNetStatus[session] & static_cast<int>(NETBIOS_SESSION_ACTIVE)))
        return static_cast<short>(NETBIOS_RESULT_SESSION_OUT_OF_RANGE);
    node = static_cast<tag_Node*>(malloc(len + static_cast<int>(NETBIOS_PACKET_HEADER_SIZE)));
    node->len = len;
    node->sessionIndex = static_cast<unsigned char>(session);
    memcpy(node->data, data, len);
    EnterCriticalSection(&gNbSndLock);
    if (queueToFree)
        add_node(&gNbFreeQueue, node);
    else
        add_node(&gNbSndQueue, node);
    LeaveCriticalSection(&gNbSndLock);
    SetEvent(gNbEvents[0]);
    return 0;
}

// donor PoL RVA 0x000a726a; preferred Buka symbol _nb_sess
// donor Buka TU SOURCE/netwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.596373;margin=0.181597;shape=0.465;size=0.963;calls=1.000;alternate=pol20:_nb_sess@0x000a726a
VA(0x0041421e, 0x4f6)
H1_C_LINKAGE short __cdecl nb_sess(int, int operation, ...) {
    NCB ncb;
    char* callName;
    int destinationSession;
    int bFree;
    va_list argList;
    short returnCode;
    int oldSession;

    va_start(argList, operation);
    switch (operation) {
        case NETBIOS_SESSION_REGISTER:
            callName = va_arg(argList, char*);
            gNetStatus[gNbMaxSess] &= ~static_cast<int>(NETBIOS_SESSION_ERROR);
            nb_format_name(callName, gNbNameBuf[gNbMaxSess].bytes);
            memset(&gNbSessNcb[gNbMaxSess], 0, sizeof(NCB));
            memcpy(gNbSessNcb[gNbMaxSess].ncb_name, gNbNameBuf[gNbMaxSess].bytes, NCBNAMSZ);
            gNbSessNcb[gNbMaxSess].ncb_command = NCBADDNAME | ASYNCH;
            gNbSessNcb[gNbMaxSess].ncb_post = nb_add_name_done;
            gNbSessNcb[gNbMaxSess].ncb_cmd_cplt = NRC_PENDING;
            gNbSessNcb[gNbMaxSess].ncb_lana_num = gNetbiosLana;
            returnCode = static_cast<short>(Netbios(&gNbSessNcb[gNbMaxSess]));
            break;

        case NETBIOS_SESSION_RECEIVE_ANY: {
            destinationSession = va_arg(argList, int);
            if (gNbSessNcb[destinationSession].ncb_cmd_cplt == NRC_PENDING) {
                switch (gNbSessNcb[destinationSession].ncb_command & ~ASYNCH) {
                    case NCBCALL:
                    case NCBDGRECVBC:
                        return 0;
                    default:
                        break;
                }
                memset(&ncb, 0, sizeof(ncb));
                ncb.ncb_command = NCBCANCEL;
                ncb.ncb_lana_num = gNetbiosLana;
                ncb.ncb_buffer = reinterpret_cast<unsigned char*>(
                    &gNbSessNcb[destinationSession]
                ); // API-forced: NCBCANCEL receives the target NCB through Win32 ncb_buffer (PUCHAR).
                Netbios(&ncb);
            }
            returnCode = nb_recv_any(destinationSession);
            break;
        }

        case NETBIOS_SESSION_CALL:
            destinationSession = va_arg(argList, int);
            callName = va_arg(argList, char*);
            nb_format_name(callName, gNbNameBuf[destinationSession].bytes);
            returnCode = nb_call(destinationSession, gNbNameBuf[destinationSession].bytes);
            break;

        case NETBIOS_SESSION_LISTEN_ANY:
            destinationSession = va_arg(argList, int);
            nb_snd(0, gNbMaxSess, 0, 0, 0);
            returnCode = nb_listen(destinationSession, gNbListenName);
            break;

        case NETBIOS_SESSION_LISTEN:
            destinationSession = va_arg(argList, int);
            callName = va_arg(argList, char*);
            nb_format_name(callName, gNbNameBuf[destinationSession].bytes);
            returnCode = nb_listen(destinationSession, gNbNameBuf[destinationSession].bytes);
            break;

        case NETBIOS_SESSION_MOVE:
            oldSession = va_arg(argList, int);
            destinationSession = va_arg(argList, int);
            bFree = va_arg(argList, int);
            if (oldSession == gNbMaxSess)
                gNbMaxSess = static_cast<u8>(destinationSession);
            if (gNbSessLsn[oldSession] == static_cast<int>(NETBIOS_INVALID_ID))
                return 0;
            gNbSessLsn[destinationSession] = gNbSessLsn[oldSession];
            gNetStatus[destinationSession] = gNetStatus[oldSession];
            memcpy(gNbNameBuf[destinationSession].bytes, gNbNameBuf[oldSession].bytes, NCBNAMSZ);
            nb_arm_recv(destinationSession);
            if (bFree != 0) {
                gNbSessLsn[oldSession] = static_cast<int>(NETBIOS_INVALID_ID);
                gNetStatus[oldSession] = 0;
                memset(gNbNameBuf[oldSession].bytes, 0, NCBNAMSZ);
            }
            returnCode = 0;
            break;

        case NETBIOS_SESSION_CLOSE:
            destinationSession = va_arg(argList, int);
            if (gNbSessNcb[destinationSession].ncb_cmd_cplt == NRC_PENDING) {
                memset(&ncb, 0, sizeof(ncb));
                ncb.ncb_command = NCBCANCEL;
                ncb.ncb_lana_num = gNetbiosLana;
                ncb.ncb_buffer = reinterpret_cast<unsigned char*>(
                    &gNbSessNcb[destinationSession]
                ); // API-forced: NCBCANCEL receives the target NCB through Win32 ncb_buffer (PUCHAR).
                Netbios(&ncb);
            }
            nb_close_session(destinationSession);
            returnCode = 0;
            break;

        case NETBIOS_SESSION_CLEAR_CONNECTED:
            destinationSession = va_arg(argList, int);
            gNetStatus[destinationSession] &= ~static_cast<int>(NETBIOS_SESSION_CONNECTED);
            returnCode = 0;
            break;

        case NETBIOS_SESSION_GET_NAME:
            destinationSession = va_arg(argList, int);
            callName = va_arg(argList, char*);
            memcpy(callName, gNbNameBuf[destinationSession].bytes, NCBNAMSZ);
            returnCode = 0;
            break;

        default:
            return 1;
    }
    if (returnCode == NRC_PENDING)
        returnCode = 0;
    return returnCode;
}

// Buka netwin.cpp:374-380.
VA(0x00414714, 0x1e)
H1_C_LINKAGE char __cdecl nb_stat(int, unsigned short session)
{
    return gNetStatus[session];
}

// Buka netwin.cpp:382-453; HoMM1 asserts through its netlo.cpp line base.
VA(0x00414732, 0x26f)
void nb_thr_ctl(void)
{
    NCB ncb;
    tag_Node *pkt;
    int keepRunning;
    unsigned char result;
    int i;
    int sendComplete;

    keepRunning = 1;
    if (WaitForMultipleObjects(NETBIOS_THREAD_EVENT_COUNT, gNbEvents, 0, 0) == WAIT_TIMEOUT)
        return;
    if (WaitForSingleObject(gNbEvents[0], 0) == WAIT_OBJECT_0)
        ResetEvent(gNbEvents[0]);
    for (i = 0; i < static_cast<int>(NETBIOS_SESSION_COUNT); i++) {
        if (WaitForSingleObject(gNbEvents[i + NETBIOS_RECEIVE_EVENT_FIRST], 0) == WAIT_OBJECT_0) {
            ResetEvent(gNbEvents[i + NETBIOS_RECEIVE_EVENT_FIRST]);
            nb_recv_complete(i);
        }
    }
    while (keepRunning) {
        EnterCriticalSection(&gNbSndLock);
        pkt = pop_node(&gNbFreeQueue);
        if (pkt == 0)
            pkt = pop_node(&gNbSndQueue);
        LeaveCriticalSection(&gNbSndLock);
        if (pkt == 0) {
            keepRunning = 0;
        } else {
            memset(&gNbCtlNcb, 0, sizeof(gNbCtlNcb));
            gNbCtlNcb.ncb_lsn = gNbSessLsn[pkt->sessionIndex];
            if (gNbCtlNcb.ncb_lsn != NETBIOS_INVALID_ID) {
                memcpy(gNbSessBuf, pkt->data, pkt->len);
                gNbCtlNcb.ncb_buffer = gNbSessBuf;
                gNbCtlNcb.ncb_length = pkt->len;
                gNbCtlNcb.ncb_command = NCBSEND;
                gNbCtlNcb.ncb_lana_num = gNetbiosLana;
                sendComplete = 0;
                while (!sendComplete) {
                    result = Netbios(&gNbCtlNcb);
                    switch (result) {
                        case NRC_GOODRET:
                            sendComplete = 1;
                            break;
                        case NRC_PENDING:
                            ProcessAssert(0, "D:\\Heroes\\Source\\netlo.cpp", gNbThrCtlLineBase + 82);
                            break;
                        case NRC_SNUMOUT:
                        case NRC_SCLOSED:
                        case NRC_SABORT:
                            gNetStatus[pkt->sessionIndex] &= ~static_cast<int>(NETBIOS_SESSION_ACTIVE);
                            break;
                        default:
                            break;
                    }
                }
            }
            free(pkt);
        }
    }
}
