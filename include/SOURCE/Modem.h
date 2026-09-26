#ifndef HOMM1_SOURCE_MODEM_H
#define HOMM1_SOURCE_MODEM_H

#include <Domains.h>

H1_ENUM_BEGIN(ModemResponseLimit)
    MODEM_RESPONSE_LAST = 79
H1_ENUM_END(ModemResponseLimit)

H1_ENUM_BEGIN(ModemPacketControl)
    MODEM_PACKET_ESCAPE = 0x70
H1_ENUM_END(ModemPacketControl)

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
extern int inescape;
extern int newpacket;
extern int packetlen;
extern char packet[];

void GUIModemCommand(char *, char *);
signed char GUIModemResponse(char *, char *);
int write_buffer(char *, int);
int read_byte(void);

#endif // HOMM1_SOURCE_MODEM_H
