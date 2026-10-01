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

// ReadPrefsFromFile reads 0x134 bytes at the owner base; graphics records
// begin at 0x18. Unused provisional HoMM2 sound offsets are superseded.
struct configStruct {
    // ControlPanel cycles these options; GetCursorSampleSet takes walkSpeed.
    int walkSpeed;
    int musicVolume;
    int soundVolume;
    char _pad_0x00c[4];
    int showRoute;
    int blackoutComputer;
    exeGfxConfig gfx[2];
    char _pad_0x048[0x70];
    int musicSource;
    char _pad_0x0bc[0x78];
};
struct SCreatureInfo { unsigned short value; char pad[24]; };
struct tag_tilePoint { signed char x; signed char y; short frameOffset; };
// Retail 31-byte monster rows: philAI::BuildBuilding reads the growth byte.
#pragma pack(push, 1)
struct tag_monsterInfo { signed char unknown00; signed char growth; char unknown02[29]; };
#pragma pack(pop)
extern tag_monsterInfo gMonsterDatabase[];
struct SSpellInfo { char m_pad0[14]; unsigned char m_e; char m_pad1[7]; };
struct SNetPlayerInfo { char m_pad[0xcc]; };
struct SAMPLE2 { class sample *pSample; struct _SAMPLE *pMem; };

#pragma pack(push, 1)
struct monsterRV { int rv; char pad[22]; };
struct SWinSetup { unsigned char status; unsigned short port; char *value; };
#pragma pack(pop)

#endif // HOMM1_H1_TYPES_H
