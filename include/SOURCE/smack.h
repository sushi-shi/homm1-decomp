#ifndef HOMM1_SOURCE_SMACK_H
#define HOMM1_SOURCE_SMACK_H

// Buka imports nine Smacker 3.0g entries by ordinal from its shipped DLL.
// Retail accesses NewPalette +0x68, Palette +0x6c, FrameNum +0x374 and
// LastRectx +0x380.

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
typedef struct SmackSumTag {
    u32 TotalTime;         /* total time                                      */
    u32 MS100PerFrame;     /* MS*100 per frame (100000/x = Frames/Sec)        */
    u32 TotalOpenTime;     /* Time to open and prepare for decompression      */
    u32 TotalFrames;       /* Total Frames displayed                          */
    u32 SkippedFrames;     /* Total number of skipped frames                  */
    u32 SoundSkips;        /* Total number of sound skips                     */
    u32 TotalBlitTime;     /* Total time spent blitting                       */
    u32 TotalReadTime;     /* Total time spent reading                        */
    u32 TotalDecompTime;   /* Total time spent decompressing                  */
    u32 TotalBackReadTime; /* Total time spent reading in background          */
    u32 TotalReadSpeed;    /* Total io speed (bytes/second)                   */
    u32 SlowestFrameTime;  /* Slowest single frame time                       */
    u32 Slowest2FrameTime; /* Second slowest single frame time                */
    u32 SlowestFrameNum;   /* Slowest single frame number                     */
    u32 Slowest2FrameNum;  /* Second slowest single frame number              */
    u32 AverageFrameSize;  /* Average size of the frame                       */
    u32 HighestMemAmount;  /* Highest amount of memory allocated              */
    u32 TotalExtraMemory;  /* Total extra memory allocated                    */
    u32 HighestExtraUsed;  /* Highest extra memory actually used              */
    u32 BitmapHandle;      /* GDI bitmap handle retained by this 3.0g build   */
    u32 SoundWindowProc;   /* previous sound-window procedure                 */
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

#endif // HOMM1_SOURCE_SMACK_H
