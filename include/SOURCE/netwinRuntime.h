#ifndef HOMM1_SOURCE_NETWINRUNTIME_H
#define HOMM1_SOURCE_NETWINRUNTIME_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <Domains.h>
#include <SOURCE/comwin.h>

#include <nb30.h>

H1_ENUM_CONST_BEGIN(NetbiosRuntimeConstant)
    NETBIOS_SESSION_COUNT = 7,
    NETBIOS_SESSION_ACTIVE = 1,
    NETBIOS_SESSION_NAME_REGISTERED = 2,
    NETBIOS_SESSION_CONNECTED = 8,
    NETBIOS_SESSION_ERROR = 0x80,
    NETBIOS_PACKET_HEADER_SIZE = 11,
    NETBIOS_INVALID_ID = 0xff,
    NETBIOS_THREAD_EVENT_COUNT = 9,
    // nb_snd/nb_term signal it to wake the NetBIOS thread (nb_thr_ctl polls it
    // first); session receive events follow from RECEIVE_EVENT_FIRST.
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
H1_ENUM_CONST_END(NetbiosRuntimeConstant)

H1_ENUM_BEGIN(NetbiosSessionOperation)
    NETBIOS_SESSION_REGISTER = 0,
    NETBIOS_SESSION_RECEIVE_ANY = 1,
    NETBIOS_SESSION_CALL = 2,
    NETBIOS_SESSION_LISTEN_ANY = 3,
    NETBIOS_SESSION_LISTEN = 4,
    NETBIOS_SESSION_MOVE = 5,
    NETBIOS_SESSION_CLOSE = 6,
    NETBIOS_SESSION_CLEAR_CONNECTED = 7,
    NETBIOS_SESSION_GET_NAME = 9
H1_ENUM_END(NetbiosSessionOperation)

struct NetbiosName {
    unsigned char bytes[NCBNAMSZ];
};

void nb_add_name(void);
void nb_format_name(char *, unsigned char *);
void __stdcall nb_add_name_done(NCB *);
unsigned short nb_recv_any(int);
unsigned short nb_call(int, void *);
unsigned short nb_listen(int, void *);
void nb_arm_recv(int);
void nb_close_session(int);
void nb_recv_complete(int);
void __stdcall nb_recv_any_done(NCB *);
void __stdcall nb_call_done(NCB *);
void nb_thr_ctl(void);
extern unsigned char *gNbListenName;

extern unsigned char gNbMaxSess;
extern unsigned char gNbShutdown;
extern unsigned char gNetStatus[7];
extern unsigned char gNbSessLsn[7];
extern NCB gNbSessNcb[7];
extern NCB gNbCtlNcb;
extern unsigned char gNbSessBuf[];
extern unsigned char gNbLocalNum;
extern char *gNbGroupName;
extern unsigned char gNbCallRetries;
extern unsigned char gNbRcvData[7][0x1000];
extern NetbiosName gNbNameBuf[7];
extern CRITICAL_SECTION gNbRcvLock;
extern CRITICAL_SECTION gNbSndLock;
extern tag_Anchor gNbRcvQueue;
extern tag_Anchor gNbSndQueue;
extern tag_Anchor gNbFreeQueue;
extern HANDLE gNbEvents[9];

#endif
