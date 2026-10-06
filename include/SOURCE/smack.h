#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

enum SmackApiConstant {
    SMACK_TRACK_1 = 0x200,
    SMACK_TRACKS = 0xfe00,
    SMACK_AUTO_EXTRA = -1,
    SMACK_SURFACE_SLOW = 1
};

#pragma pack(push, 1)
struct Smack {
    unsigned long Version;
    unsigned long Width;
    unsigned long Height;
    unsigned long Frames;
    char unknown10[0x5c];
    unsigned long paletteSelector;
    unsigned char Palette[0x304];
    unsigned char alternatePalette[0x304];
    char unknown678[0xc];
    long LastRectx;
    long LastRecty;
    long LastRectw;
    long LastRecth;
};
#pragma pack(pop)

extern "C" Smack* SmackOpen(char*, unsigned long, long);
extern "C" void SmackClose(Smack*);
extern "C" unsigned short SmackDoFrame(Smack*);
extern "C" void SmackNextFrame(Smack*);
extern "C" void SmackGoto(Smack*, unsigned long);
extern "C" unsigned short SmackSoundOnOff(Smack*, unsigned long);
extern "C" void SmackToBuffer(
    Smack*,
    unsigned long,
    unsigned long,
    unsigned long,
    unsigned long,
    void*,
    unsigned long
);
extern "C" unsigned short SmackToBufferRect(Smack*, unsigned long);
extern "C" void SmackVolumePan(Smack*, unsigned long, unsigned long, unsigned long);
extern "C" unsigned short SmackWait(Smack*);
extern "C" unsigned char SmackSoundUseMSS(void*);

#endif
