#ifndef HOMM1_SOURCE_MODEM_H
#define HOMM1_SOURCE_MODEM_H

#include <Domains.h>

// clang-format off
H1_ENUM_BEGIN(ModemResponseLimit)
    MODEM_RESPONSE_LAST = 79
H1_ENUM_END(ModemResponseLimit)

// WriteModemPacket frames a packet as ESCAPE START ... ESCAPE END, doubling
// an ESCAPE byte inside it; ReadPacket undoes it.
H1_ENUM_BEGIN(ModemPacketControl)
    MODEM_PACKET_START = 0,
    MODEM_PACKET_END = 1,
    MODEM_PACKET_ESCAPE = 0x70
H1_ENUM_END(ModemPacketControl)

H1_ENUM_CONST_BEGIN(ModemPacketConstant)
    MODEM_PACKET_MAX_LENGTH = 0x100
H1_ENUM_CONST_END(ModemPacketConstant)
// clang-format on

extern int iLastActionTime;
extern int iModemCommandPos;
extern char cModemCommand[];
extern char GUIMRresponse[];
extern char GUIMRresp[];
extern int GUIMRrespptr;
extern int GUIMRc;
extern int iLastDialPos;
extern char numbuf[];
struct inque_t {
    int readPosition;
    int writePosition;
    char data[4096];
};
extern inque_t inque;
// The transmit queue holds 2K (retail 0x004c9c80-0x004ca487); SETUP.cpp
// completes its type.
extern struct outque_t outque;
extern int iBaudBits;
extern int inescape;
extern int newpacket;
extern int packetlen;
extern char packet[];
extern char idstr[];
extern char remoteidstr[];
extern int oldsec;
extern int stime;
extern int remotestage;
extern int localstage;
extern int WFDCStage;

void GUIModemCommand(char*, char*);
void ModemCommand(char*);
void ModemSetup(void);
long Dial(void);
long Wait(void);
void Connect(void);
signed char GUIModemResponse(char*, char*);
int write_buffer(char*, int);
int read_byte(void);
// Modem.cpp's wait-loop steps that KB's WaitHandler drives.
signed char GUIModemCommandExec(void);
signed char GUIModemResponseExec(void);
int WaitForDirectConnect(void);

#endif // HOMM1_SOURCE_MODEM_H
