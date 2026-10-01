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

// HoMM1 uses six-word graphics records; HoMM2 adds colorMouseCursor.
struct exeGfxConfig {
    int showMenu;
    int x;
    int y;
    int width;
    int height;
    int fullScreen;
};

// ReadPrefsFromFile reads 0x134 bytes at the owner base. The registry
// readers and writers name every persisted field except the 0x50 interval.
struct configStruct {
    int walkSpeed;
    int musicVolume;
    int soundVolume;
    int autosave;
    int showRoute;
    int blackoutComputer;
    exeGfxConfig gfx[2];
    int firstMapOffset;
    int currentMapOffset;
    char _pad_0x050[0x64];
    int cdOffset;
    int musicSource;
    int comPort[2];
    int baudRate[2];
    char modemInitString[100];
    int slowVideo;
};
struct SCreatureInfo { unsigned short value; char pad[24]; };
struct tag_tilePoint { signed char x; signed char _1; signed char y; signed char _3; };
struct SSpellInfo { char m_pad0[14]; unsigned char m_e; char m_pad1[7]; };
struct SNetPlayerInfo { char m_pad[0xcc]; };
struct SAMPLE2 { class sample *pSample; struct _SAMPLE *pMem; };

#pragma pack(push, 1)
// Retail strides creature records by 31 bytes from 0x492060.
struct tag_monsterInfo {
    short cost;
    int fightValue;
    signed char iconIndex;
    signed char growth;
    int hitPoints;
    signed char race;
    signed char speed;
    signed char attack;
    signed char defense;
    signed char damageMin;
    signed char damageMax;
    signed char shots;
    char spriteName[8];
    int attributes;
};
struct monsterRV { int rv; char pad[22]; };
struct SWinSetup { unsigned char status; unsigned short port; char *value; };
#pragma pack(pop)

#endif // HOMM1_H1_TYPES_H
