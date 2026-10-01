#ifndef HOMM1_SOURCE_REMOTE_H
#define HOMM1_SOURCE_REMOTE_H

#include <Domains.h>

H1_ENUM_BEGIN(RemoteMessageType)
    REMOTE_MESSAGE_DEFAULT = -1,
    REMOTE_MESSAGE_NONE = 0,
    REMOTE_MESSAGE_CONFIRM = 1,
    REMOTE_MESSAGE_RELIABLE = 2,
    REMOTE_MESSAGE_UNRELIABLE = 3,
    REMOTE_MESSAGE_HEARTBEAT = 4
H1_ENUM_END(RemoteMessageType)

H1_ENUM_BEGIN(RemoteBoxCommand)
    BOX_REMOTE_SAVE = 1,
    BOX_REMOTE_SETUP = 0x1f
H1_ENUM_END(RemoteBoxCommand)

H1_ENUM_BEGIN(RemoteConstant)
    REMOTE_BROADCAST_PLAYER = 0x7f,
    REMOTE_MESSAGE_HEADER_SIZE = 9,
    REMOTE_MESSAGE_SIZE = 0x100,
    REMOTE_QUEUE_CAPACITY = 7,
    REMOTE_RECENT_ID_COUNT = 30,
    REMOTE_RETRY_COUNT = 7,
    REMOTE_CONFIRM_POLL_COUNT = 200
H1_ENUM_END(RemoteConstant)

// Retail TransmitRemoteData fills sender/id/type/command/size at +0/+1/+5/+6/+7
// and copies the payload to +9; GetRemoteData copies 0x100-byte records.
#pragma pack(push, 1)
struct RemoteMessage {
    signed char sender;
    int id;
    signed char type;
    signed char command;
    short payloadSize;
    union {
        char data[REMOTE_MESSAGE_SIZE - REMOTE_MESSAGE_HEADER_SIZE];
        // WaitForOtherPlayer passes the first payload dword to ReceiveSaveGame.
        int saveSize;
    } payload;
};
#pragma pack(pop)

extern int gbRemoteOn;
extern signed char gbInNetSetup;
extern int giThisNetPos;
extern int giThisGamePos;
extern int iIDCtr;
extern int giLastConfirm;
extern signed char gbGamePosToNetPos[];
// WaitForOtherPlayer stores the game position of net position zero here.
extern int giHostGamePos;
extern int iInOrder[REMOTE_QUEUE_CAPACITY];
extern RemoteMessage rcvBuf[REMOTE_QUEUE_CAPACITY];
extern char rcvBufOut[REMOTE_MESSAGE_SIZE];

int SendRemoteData(unsigned char *, unsigned char *, int, int);
int ReceiveRemoteData(unsigned char *, unsigned char *, int);
int TransmitRemoteData(char *, int, int, signed char, signed char, signed char, signed char, signed char);
char *GetRemoteData(signed char);
int TransmitAndWait(char *, int, int, signed char, signed char, char **);
signed char NetPosToGamePos(int);
signed char WaitForOtherPlayer(void);

#endif
