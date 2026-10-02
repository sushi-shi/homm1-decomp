// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/Modem.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/comwin.h>
#include <SOURCE/netwinRuntime.h>
#include <SOURCE/dialogTypes.h>

#include <string.h>

// donor PoL RVA 0x0000d4df; preferred Buka symbol ?WriteModemPacket@@YIXPADH@Z
// donor Buka TU SOURCE/Modem; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.395642;margin=0.361596;shape=0.175;size=0.824;calls=0.667;alternate=pol20:void WriteModemPacket(char *, int)@0x0000d4df
VA(0x0045a16b, 0xdc)
void WriteModemPacket(char* buffer, int length) {
    char buf[544];
    int pos = 0;
    if (length > 256)
        return;

    buf[pos] = MODEM_PACKET_ESCAPE;
    ++pos;
    buf[pos] = 0;
    ++pos;
    while (length--) {
        if (*buffer == MODEM_PACKET_ESCAPE) {
            buf[pos] = MODEM_PACKET_ESCAPE;
            ++pos;
        }
        buf[pos] = *buffer;
        ++buffer;
        ++pos;
    }
    buf[pos] = MODEM_PACKET_ESCAPE;
    ++pos;
    buf[pos] = 1;
    ++pos;
    while (write_buffer(buf, pos) == 0)
        ForcePollSound();
}

// donor PoL RVA 0x000a3ec7; preferred Buka symbol ?TransmitRemoteData@@YIHPADHHCCCC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.560856;margin=1.192457;shape=0.450;size=0.831;calls=1.000;alternate=pol20:int TransmitRemoteData(char *, int, int, signed char, signed char, signed char, signed char)@0x000a3ec7
VA(0x0045a247, 0x231)
// HoMM1 callers pass an eighth flag that maps a game position to its net position.
int TransmitRemoteData(
    char* data,
    int destination,
    int length,
    signed char command,
    signed char reliable,
    signed char allowRetryDialog,
    signed char messageType,
    signed char gamePosDestination
) {
    int k;
    int retval;
    int j;
    RemoteMessage msg;
    int tries;

    if (!gbRemoteOn || gbInNetSetup)
        return 1;
    if (gamePosDestination && destination != REMOTE_BROADCAST_PLAYER)
        destination = gbGamePosToNetPos[destination];
    retval = 0;
    tries = 0;
    iIDCtr++;
    msg.sender = giThisNetPos;
    msg.id = iIDCtr;
    if (messageType != REMOTE_MESSAGE_DEFAULT)
        msg.type = messageType;
    else if (reliable)
        msg.type = REMOTE_MESSAGE_RELIABLE;
    else
        msg.type = REMOTE_MESSAGE_UNRELIABLE;
    msg.payloadSize = length;
    msg.command = command;
    if (length > 0)
        memcpy(msg.payload.data, data, length);
    while (retval == 0 && tries <= REMOTE_RETRY_COUNT) {
        retval = SendRemoteData(
            (unsigned char*)&msg,
            0,
            destination,
            length + REMOTE_MESSAGE_HEADER_SIZE
        );
        if (!reliable && retval) {
            return 1;
        } else if (retval) {
            k = 0;
            while (k < REMOTE_CONFIRM_POLL_COUNT) {
                ForcePollSound();
                if (giLastConfirm == iIDCtr)
                    return 1;
                retval = 0;
                DelayMilli(10);
                k++;
            }
        } else {
            DelayMilli(1000);
        }
        if (allowRetryDialog && tries == REMOTE_RETRY_COUNT && retval == 0) {
            NormalDialog("Error sending data.  Keep trying??", 2, -1, -1, -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                tries = -1;
        }
        tries++;
    }
    return retval;
}

// donor PoL RVA 0x000a40e1; preferred Buka symbol ?GetRemoteData@@YIPADC@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.517569;margin=0.974708;shape=0.366;size=0.825;calls=1.000;alternate=pol20:char * GetRemoteData(signed char)@0x000a40e1
VA(0x0045a478, 0x10c)
char* GetRemoteData(signed char remove) {
    int oldest;
    int i;
    int index;

    if (!gbRemoteOn || gbInNetSetup)
        return 0;
    oldest = 999999999;
    index = -1;
    for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
        if (rcvBuf[i].type && iInOrder[i] < oldest) {
            oldest = iInOrder[i];
            index = i;
        }
    }
    if (index >= 0) {
        memcpy(rcvBufOut, &rcvBuf[index], REMOTE_MESSAGE_SIZE);
        if (remove)
            rcvBuf[index].type = REMOTE_MESSAGE_NONE;
        rcvBuf[index].sender = NetPosToGamePos(rcvBuf[index].sender);
        return rcvBufOut;
    }
    return 0;
}

// donor PoL RVA 0x000a41ec; preferred Buka symbol ?PollRemote@@YIXXZ
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void PollRemote(void)@0x000a41ec
// PollRemote's heartbeat clocks, timeout latch, recent-id ring and the
// incoming/outgoing message buffers.
extern long lLastHeartbeatSend;
extern long lLastHeartbeatReceive;
extern signed char bInTimeoutFail;
extern RemoteMessage sndBuf;
extern RemoteMessage rcvBufIn;
extern int iLastIds[REMOTE_RECENT_ID_COUNT];
extern int iInOrderCtr;
extern int iCurLastID;
// The other side's ready flag and the heartbeat-seen flag.
extern int gbRemoteReady;
extern int gbHeartbeatSeen;

VA(0x0045a584, 0x4fe)
void PollRemote(void) {
    signed char newControl;
    signed char queueFull;
    int i;
    int numQueued;
    int result;

    if (!gbRemoteOn)
        return;
    if (iMPBaseType == MULTIPLAYER_BASE_MODEM)
        comm_wrt_task();
    else if (iMPBaseType == MULTIPLAYER_BASE_NETWORK)
        nb_thr_ctl();
    if (gbInNetSetup)
        return;
    numQueued = 0;
    queueFull = 0;
    if (KBTickCount() - lLastHeartbeatSend > 5000) {
        sndBuf.sender = giThisNetPos;
        sndBuf.type = REMOTE_MESSAGE_HEARTBEAT;
        sndBuf.payloadSize = 1;
        sndBuf.command = (giCurPlayer << 4) + iCurHourGlassPhase;
        sndBuf.payload.data[0] = 1;
        SendRemoteData((unsigned char*)&sndBuf, 0, 1 - giThisNetPos, 10);
        lLastHeartbeatSend = KBTickCount();
    }
    if (KBTickCount() > lLastHeartbeatReceive + 60000 && !bInTimeoutFail) {
        NormalDialog(
            "The other player's computer is not responding.  Do you wish to wait longer?",
            2, -1, -1, -1, 0, -1, 0, -1);
        if (gpWindowManager->m_dialogResult == 0x7805) {
            lLastHeartbeatReceive = KBTickCount();
        } else {
            bInTimeoutFail = 1;
            if (gbHumanPlayer[giCurPlayer]) {
                if (giCurPlayer == giThisGamePos)
                    newControl = 0;
                else
                    newControl = 1;
            } else {
                if (giHostGamePos == giThisGamePos)
                    newControl = 0;
                else
                    newControl = 1;
            }
            ReceiveRemotePlayerExit(1 - giThisGamePos, newControl, 0, 1);
        }
    }
    for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
        if (rcvBuf[i].type)
            numQueued++;
    }
    if (numQueued == REMOTE_QUEUE_CAPACITY)
        queueFull = 1;
    result = 1;
    while (result) {
    nextIncoming:
        result = ReceiveRemoteData(0, (unsigned char*)&rcvBufIn, REMOTE_BROADCAST_PLAYER);
        if (result && rcvBufIn.sender != giThisNetPos) {
            if (rcvBufIn.type == REMOTE_MESSAGE_CONFIRM) {
                giLastConfirm = rcvBufIn.id;
                goto done;
            } else if (rcvBufIn.type == REMOTE_MESSAGE_HEARTBEAT) {
                if (rcvBufIn.payloadSize == 1 && rcvBufIn.payload.data[0] == 1)
                    gbRemoteReady = 1;
                lLastHeartbeatReceive = KBTickCount();
                gbHeartbeatSeen = 1;
                if (giHostGamePos != giThisGamePos && giCurPlayer != giThisGamePos
                    && gpAdvManager->m_active == 1 && rcvBufIn.command / 16 != giThisGamePos) {
                    giCurPlayer = rcvBufIn.command / 16;
                    iCurHourGlassPhase = rcvBufIn.command - giCurPlayer * 16;
                }
                goto done;
            } else if (queueFull) {
                goto done;
            }
            if (rcvBufIn.type == REMOTE_MESSAGE_RELIABLE) {
                sndBuf.sender = giThisNetPos;
                sndBuf.id = rcvBufIn.id;
                sndBuf.type = REMOTE_MESSAGE_CONFIRM;
                sndBuf.payloadSize = 0;
                SendRemoteData((unsigned char*)&sndBuf, 0, rcvBufIn.sender, REMOTE_MESSAGE_HEADER_SIZE);
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (rcvBuf[i].type && rcvBuf[i].id == rcvBufIn.id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_RECENT_ID_COUNT; i++) {
                if (iLastIds[i] == rcvBufIn.id)
                    goto nextIncoming;
            }
            for (i = 0; i < REMOTE_QUEUE_CAPACITY; i++) {
                if (!rcvBuf[i].type) {
                    iInOrder[i] = iInOrderCtr++;
                    memcpy(&rcvBuf[i], &rcvBufIn, REMOTE_MESSAGE_SIZE);
                    numQueued++;
                    iLastIds[iCurLastID] = rcvBufIn.id;
                    iCurLastID = (iCurLastID + 1) % REMOTE_RECENT_ID_COUNT;
                    if (numQueued == REMOTE_QUEUE_CAPACITY)
                        goto done;
                    goto nextIncoming;
                }
            }
        }
    }
done:;
}

// donor PoL RVA 0x000a48e0; preferred Buka symbol ?TransmitAndWait@@YIHPADHHCCPAPAD@Z
// donor Buka TU SOURCE/REMOTE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.561659;margin=0.362301;shape=0.477;size=0.829;calls=1.000;alternate=pol20:int TransmitAndWait(char *, int, int, signed char, signed char, char * *)@0x000a48e0
VA(0x0045aa82, 0x14f)
int TransmitAndWait(
    char* bytes,
    int destination,
    int length,
    signed char command,
    signed char responseCommand,
    char** response
) {
    int start;
    int result;
    RemoteMessage* received;
    char complete;

    if (!gbRemoteOn || gbInNetSetup)
        return 1;
    received = 0;
    result =
        TransmitRemoteData(bytes, destination, length, command, 1, 1, REMOTE_MESSAGE_DEFAULT, 1);
    if (result == 0)
        goto transmitComplete;
    start = KBTickCount();
    complete = 0;
    while (!complete) {
        if (KBTickCount() > start + 20000) {
            NormalDialog("Error sending data.  Keep trying??", 2, -1, -1, -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                start = KBTickCount();
            } else {
                result = 0;
                goto transmitComplete;
            }
        }
        ForcePollSound();
        received = (RemoteMessage*)GetRemoteData(1);
        if (received && received->type == REMOTE_MESSAGE_RELIABLE
            && received->command == responseCommand)
            complete = 1;
    }
    *response = (char*)received;
transmitComplete:
    return result;
}
