#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

enum SmackApiConstant {
    SMACK_PRELOAD_ALL = 0x200,
    SMACK_TRACK_1 = 0x2000,
    SMACK_TRACKS = 0xfe000,
    SMACK_AUTO_EXTRA = -1,
    SMACK_SURFACE_SLOW = 1
};

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
typedef struct SmackSumTag {
    u32 TotalTime;
    u32 MS100PerFrame;
    u32 TotalOpenTime;
    u32 TotalFrames;
    u32 SkippedFrames;
    u32 SoundSkips;
    u32 TotalBlitTime;
    u32 TotalReadTime;
    u32 TotalDecompTime;
    u32 TotalBackReadTime;
    u32 TotalReadSpeed;
    u32 SlowestFrameTime;
    u32 Slowest2FrameTime;
    u32 SlowestFrameNum;
    u32 Slowest2FrameNum;
    u32 AverageFrameSize;
    u32 HighestMemAmount;
    u32 TotalExtraMemory;
    u32 HighestExtraUsed;
    u32 BitmapHandle;
    u32 SoundWindowProc;
} SmackSum;
#pragma pack(pop)

extern "C" __declspec(dllimport) Smack* __stdcall SmackOpen(char*, unsigned long, long);
extern "C" __declspec(dllimport) void __stdcall SmackClose(Smack*);
extern "C" __declspec(dllimport) unsigned long __stdcall SmackDoFrame(Smack*);
extern "C" __declspec(dllimport) void __stdcall SmackNextFrame(Smack*);
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
extern "C" __declspec(dllimport) unsigned char __stdcall SmackSoundUseMSS(void*);

extern "C" __declspec(dllimport) void __stdcall SmackSummary(Smack*, SmackSum*);

#endif
