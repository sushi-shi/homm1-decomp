#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

// Smacker (RAD Game Tools) playback API as HoMM1 links it through
// smkwai32.dll.  Retail imports that DLL by ordinal only and every call site
// cleans its own arguments (`add esp, N`), so the period header declared the
// API __cdecl.  The ordinal -> name map is recorded with its call-site
// evidence in config/retail/function_referents.tsv.  Only the members the
// game reads are named; offsets come from retail smackManager::Main.  Frames,
// Palette and the LastRect fields keep the SDK names their use proves; this
// older layout's +0x6c palette selector and +0x374 second palette are named
// from smackManager::Main's use alone.

#include <Domains.h>

// Flags and sentinels of this SDK generation as smackManager::Main passes
// them. SmackOpen's audio-track bits sit four bits lower than in the 3.0g
// SDK HoMM2 ships (SMACKTRACK1 0x2000, Buka AUDIO_OPEN_FLAGS 0xfe000): the
// seven tracks are 0x200..0x8000, and SmackVolumePan addresses track 1 by its
// bit. SMACK_AUTO_EXTRA is SmackOpen's automatic extra-buffer sentinel and
// SMACK_SURFACE_SLOW the SmackToBufferRect copy mode (3.0g SMACKAUTOEXTRA /
// SMACKSURFACESLOW, as Buka's SMACKMGR spells the same calls).
H1_ENUM_CONST_BEGIN(SmackApiConstant)
    SMACK_TRACK_1 = 0x200,
    SMACK_TRACKS = 0xfe00,
    SMACK_AUTO_EXTRA = -1,
    SMACK_SURFACE_SLOW = 1
H1_ENUM_CONST_END(SmackApiConstant)

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
