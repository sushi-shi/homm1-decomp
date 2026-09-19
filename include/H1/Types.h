#ifndef HOMM1_H1_TYPES_H
#define HOMM1_H1_TYPES_H

#include <H1/Ints.h>

// Shared complete records needed by more than one recovered module. Layouts
// remain provisional until their member accesses and allocation sizes are
// matched in HoMM1.

struct SMapChange { char _pad[64]; };
struct SPlayerExit { signed char player[7]; };

typedef unsigned int UInt32;
struct MemEntry;
struct _SAMPLE;

struct configStruct { char pad[0x1a0]; };
struct SCreatureInfo { unsigned short value; char pad[24]; };
struct tag_tilePoint { signed char x; signed char _1; signed char y; signed char _3; };
struct tag_monsterInfo { short attributes; char padding[24]; };
struct SSpellInfo { char m_pad0[14]; unsigned char m_e; char m_pad1[7]; };
struct SNetPlayerInfo { char m_pad[0xcc]; };
struct SAMPLE2 { class sample *pSample; struct _SAMPLE *pMem; };

#pragma pack(push, 1)
struct monsterRV { int rv; char pad[22]; };
struct SWinSetup { unsigned char status; unsigned short port; char *value; };
#pragma pack(pop)

#endif // HOMM1_H1_TYPES_H
