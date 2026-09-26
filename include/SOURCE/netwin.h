#ifndef HOMM1_SOURCE_NETWIN_H
#define HOMM1_SOURCE_NETWIN_H

#include <Domains.h>

H1_ENUM_BEGIN(NetbiosProbeCommand)
    NETBIOS_COMMAND_PROBE = 0x7f
H1_ENUM_END(NetbiosProbeCommand)

extern unsigned char gNetbiosLana;
extern unsigned char gNetbiosAvail;

#endif
