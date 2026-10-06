#ifndef HOMM1_SOURCE_NETWINHOST_H
#define HOMM1_SOURCE_NETWINHOST_H

// The Windows NetBIOS transport's sessions and worker thread. Only netwin.cpp
// includes it.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <nb30.h>

#include <SOURCE/netwin.h>

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
extern tag_Anchor gNbPriorityQueue;
extern HANDLE gNbEvents[9];
#endif
