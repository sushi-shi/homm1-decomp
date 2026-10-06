#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

// The Smacker SDK as the game uses it. On Windows it is SMACKW32.DLL; the
// native port implements the same calls itself.
#if defined(_WIN32) && !defined(HOMM1_PORT)
#define SMACK_IMPORT __declspec(dllimport)
#define SMACK_CALL __stdcall
#else
#define SMACK_IMPORT
#define SMACK_CALL
#endif

enum SmackApiConstant {
    SMACK_PRELOAD_ALL = 0x200,
    SMACK_TRACK_1 = 0x2000,
    SMACK_TRACKS = 0xfe000,
    SMACK_AUTO_EXTRA = -1,
    SMACK_SURFACE_SLOW = 1
};

#pragma pack(push, 1)
struct Smack {
    u32 Version;
    u32 Width;
    u32 Height;
    u32 Frames;
    u32 MSPerFrame;
    u32 SmackerType;
    u32 LargestInTrack[7];
    u32 tablesize;
    u32 codesize;
    u32 absize;
    u32 detailsize;
    u32 typesize;
    u32 TrackType[7];
    u32 extra;
    u32 NewPalette;
    unsigned char Palette[772];
    u32 PalType;
    u32 FrameNum;
    u32 FrameSize;
    u32 SndSize;
    i32 LastRectx;
    i32 LastRecty;
    i32 LastRectw;
    i32 LastRecth;
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

extern "C" SMACK_IMPORT Smack* SMACK_CALL SmackOpen(char*, u32, i32);
extern "C" SMACK_IMPORT void SMACK_CALL SmackClose(Smack*);
extern "C" SMACK_IMPORT u32 SMACK_CALL SmackDoFrame(Smack*);
extern "C" SMACK_IMPORT void SMACK_CALL SmackNextFrame(Smack*);
extern "C" SMACK_IMPORT void SMACK_CALL SmackToBuffer(
    Smack*,
    u32,
    u32,
    u32,
    u32,
    void*,
    u32
);
extern "C" SMACK_IMPORT u32 SMACK_CALL SmackToBufferRect(Smack*, u32);
extern "C" SMACK_IMPORT u32 SMACK_CALL SmackWait(Smack*);
extern "C" SMACK_IMPORT unsigned char SMACK_CALL SmackSoundUseMSS(void*);

extern "C" SMACK_IMPORT void SMACK_CALL SmackSummary(Smack*, SmackSum*);

#endif
