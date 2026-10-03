#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

// Smacker 3.0r: named stdcall imports from SMACKW32.DLL. The retail
// playback accesses NewPalette +0x68, Palette +0x6c and LastRectx +0x380.
// Field names correspond to the 3.x SDK; those offsets are verified in 1.2.

#include <Domains.h>

H1_ENUM_CONST_BEGIN(SmackApiConstant)
    SMACK_PRELOAD_ALL = 0x200,
    SMACK_TRACK_1 = 0x2000,
    SMACK_TRACKS = 0xfe000,
    SMACK_AUTO_EXTRA = -1,
    SMACK_SURFACE_SLOW = 1
H1_ENUM_CONST_END(SmackApiConstant)

#pragma pack(push, 1)
struct Smack {
    unsigned long Version;
    unsigned long Width;
    unsigned long Height;
    unsigned long Frames;
    unsigned long MSPerFrame;
    unsigned long SmackerType;
    unsigned long LargestInTrack[7];
    unsigned long tablesize;
    unsigned long codesize;
    unsigned long absize;
    unsigned long detailsize;
    unsigned long typesize;
    unsigned long TrackType[7];
    unsigned long extra;
    unsigned long NewPalette;
    unsigned char Palette[772];
    unsigned long PalType;
    unsigned long FrameNum;
    unsigned long FrameSize;
    unsigned long SndSize;
    long LastRectx;
    long LastRecty;
    long LastRectw;
    long LastRecth;
};
#pragma pack(pop)

extern "C" __declspec(dllimport) Smack* __stdcall SmackOpen(char*, unsigned long, long);
extern "C" __declspec(dllimport) void __stdcall SmackClose(Smack*);
extern "C" __declspec(dllimport) unsigned long __stdcall SmackDoFrame(Smack*);
extern "C" __declspec(dllimport) void __stdcall SmackNextFrame(Smack*);
extern "C" __declspec(dllimport) void __stdcall SmackGoto(Smack*, unsigned long);
extern "C" __declspec(dllimport) unsigned long __stdcall SmackSoundOnOff(Smack*, unsigned long);
extern "C" __declspec(dllimport) void __stdcall SmackToBuffer(
    Smack*,
    unsigned long,
    unsigned long,
    unsigned long,
    unsigned long,
    void*,
    unsigned long
);
extern "C" __declspec(dllimport) unsigned long __stdcall SmackToBufferRect(Smack*, unsigned long);
extern "C" __declspec(dllimport) unsigned long __stdcall SmackWait(Smack*);
extern "C" __declspec(dllimport) unsigned long __stdcall SmackSoundUseMSS(void*);

#endif
