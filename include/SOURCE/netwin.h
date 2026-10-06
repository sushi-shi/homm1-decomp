#ifndef HOMM1_SOURCE_NETWIN_H
#define HOMM1_SOURCE_NETWIN_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <nb30.h>
#include <SOURCE/comwin.h>

enum NetbiosProbeCommand {
    NETBIOS_COMMAND_PROBE = 0x7f
};

extern u8 gNetbiosLana;
extern u8 gNetbiosAvail;

enum NetbiosRuntimeConstant {
    NETBIOS_SESSION_COUNT = 7,
    NETBIOS_SESSION_ACTIVE = 1,
    NETBIOS_SESSION_NAME_REGISTERED = 2,
    NETBIOS_SESSION_CONNECTED = 8,
    NETBIOS_SESSION_ERROR = 0x80,
    NETBIOS_PACKET_HEADER_SIZE = 11,
    NETBIOS_INVALID_ID = 0xff,
    NETBIOS_THREAD_EVENT_COUNT = 9,
    NETBIOS_WAKE_EVENT = 0,
    NETBIOS_RECEIVE_EVENT_FIRST = 2,
    NETBIOS_PAYLOAD_SIZE = 0x1000,
    NETBIOS_CALL_RETRY_LIMIT = 20,
    NETBIOS_CALL_RETRY_DELAY = 100,
    NETBIOS_RECEIVE_RETRY_DELAY = 50,
    NETBIOS_ADAPTER_STATUS_SIZE = 0x400,
    NETBIOS_RESET_SESSION_LIMIT_INDEX = 0,
    NETBIOS_RESET_NAME_LIMIT_INDEX = 2,
    NETBIOS_RESET_SESSION_LIMIT = 20,
    NETBIOS_RESET_NAME_LIMIT = 10
};

enum NetbiosSessionOperation {
    NETBIOS_SESSION_REGISTER = 0,
    NETBIOS_SESSION_RECEIVE_ANY = 1,
    NETBIOS_SESSION_CALL = 2,
    NETBIOS_SESSION_LISTEN_ANY = 3,
    NETBIOS_SESSION_LISTEN = 4,
    NETBIOS_SESSION_MOVE = 5,
    NETBIOS_SESSION_CLOSE = 6,
    NETBIOS_SESSION_CLEAR_CONNECTED = 7,
    NETBIOS_SESSION_GET_NAME = 9
};

struct NetbiosName {
    u8 bytes[NCBNAMSZ];
};

void nb_add_name(void);
void nb_format_name(char* source, u8* destination);
void __stdcall nb_add_name_done(NCB* ncb);
u16 nb_recv_any(i32 session);
u16 nb_call(i32 session, void* name);
u16 nb_listen(i32 session, void* name);
void nb_arm_recv(i32 session);
void nb_close_session(i32 session);
void nb_recv_complete(i32 session);
void __stdcall nb_recv_any_done(NCB* ncb);
void __stdcall nb_call_done(NCB* ncb);
void nb_thr_ctl(void);
extern u8* gNbListenName;

extern u8 gNbMaxSess;
extern u8 gNbShutdown;
extern u8 gNetStatus[7];
extern u8 gNbSessLsn[7];
extern NCB gNbSessNcb[7];
extern NCB gNbCtlNcb;
extern u8 gNbSessBuf[];
extern u8 gNbLocalNum;
extern char* gNbGroupName;
extern u8 gNbCallRetries;
extern u8 gNbRcvData[7][0x1000];
extern NetbiosName gNbNameBuf[7];
extern CRITICAL_SECTION gNbRcvLock;
extern CRITICAL_SECTION gNbSndLock;
extern tag_Anchor gNbRcvQueue;
extern tag_Anchor gNbSndQueue;
extern tag_Anchor gNbFreeQueue;
extern HANDLE gNbEvents[9];
extern "C" u16 __cdecl nb_init(u16 maxSessions);
extern "C" void __cdecl nb_term(i32);
extern "C" i16 __cdecl nb_rcv(i32, u16 len, void* buffer);
extern "C" i16 __cdecl nb_snd(i32, u16 session, u16 len, void* data, i32 queueToFree);
extern "C" i16 __cdecl nb_sess(i32, i32 operation, ...);
extern "C" u8 __cdecl nb_stat(i32, u16 session);

#endif
