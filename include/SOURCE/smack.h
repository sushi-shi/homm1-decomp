#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

// Smacker (RAD Game Tools) playback API as HoMM1 links it through
// smkwai32.dll.  Retail imports that DLL by ordinal only and every call site
// cleans its own arguments (`add esp, N`), so the period header declared the
// API __cdecl.  The ordinal -> name map is recorded with its call-site
// evidence in config/retail/function_referents.tsv.  Only the members the
// game reads are named; offsets come from retail smackManager::Main.

#pragma pack(push, 1)
struct Smack {
    unsigned long Version;
    unsigned long Width;
    unsigned long Height;
    unsigned long Frames;
    char unknown10[0x5c];
    unsigned long PalType;
    unsigned char Palette[0x304];
    unsigned char AltPalette[0x304];
    char unknown678[0xc];
    long LastRectx;
    long LastRecty;
    long LastRectw;
    long LastRecth;
};
#pragma pack(pop)

extern "C" Smack *SmackOpen(char *, unsigned long, long);
extern "C" void SmackClose(Smack *);
extern "C" unsigned short SmackDoFrame(Smack *);
extern "C" void SmackNextFrame(Smack *);
extern "C" void SmackGoto(Smack *, unsigned long);
extern "C" unsigned short SmackSoundOnOff(Smack *, unsigned long);
extern "C" void SmackToBuffer(Smack *, unsigned long, unsigned long, unsigned long, unsigned long, void *,
                              unsigned long);
extern "C" unsigned short SmackToBufferRect(Smack *, unsigned long);
extern "C" void SmackVolumePan(Smack *, unsigned long, unsigned long, unsigned long);
extern "C" unsigned short SmackWait(Smack *);
extern "C" unsigned char SmackSoundUseMSS(void *);

#endif
