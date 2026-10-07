#ifndef HOMM1_SOURCE_SMACKMANAGER_H
#define HOMM1_SOURCE_SMACKMANAGER_H

#include <windows.h>

#include <Domains.h>
#include <SOURCE/smack.h>

H1_ENUM_BEGIN(SmackVideo)
    SMACK_BUKA = 0,
    SMACK_NWCLOGO = 1,
    SMACK_INTRO = 2,
    SMACK_LOSE = 3,
    SMACK_WIN1 = 4,
    SMACK_WIN2 = 5,
    SMACK_COUNT = 6
H1_ENUM_END(SmackVideo)

// gSmackOptions rows: movie file names and playback flags, 0x1b bytes each.
#pragma pack(push, 1)
struct SSmackOptions {
    char fileName[9];
    char companionFileName[9];
    b8 fadeIn;
    b8 fadeOut;
    b8 preload;
    b8 waitForInput;
    b8 drawCompanion;
    i16 companionX;
    i16 companionY;
};
#pragma pack(pop)

extern H1_ENUM_ARRAY(SSmackOptions, gSmackOptions, SmackVideo, SMACK_COUNT);
extern H1_ENUM_STORAGE(SmackVideo, i8) gMovieId;
void InitSmackSound();
void ShutdownSmackSound();
void ConvertSmackerPalette(u8* paletteData);
void DoAdvance(Smack* smack, b32 drawFrame, b32 advanceFrame, b32 updatePalette, b32 skipPalette);
void SmackMain();
void CloseSmackers();
i32 PlaySmacker(H1_ENUM_PARAM(SmackVideo, i32) smackNumber);

// InitSmackSound tries the wave formats from 44 kHz 16-bit stereo down to
// 11 kHz 8-bit mono and falls back to 22 kHz 8-bit mono when the device
// reports none of them.
H1_ENUM_CONST_BEGIN(SmackSoundConstant)
    SMACK_SOUND_FORMAT_COUNT = 12,
    SMACK_FALLBACK_CHANNELS = 1,
    SMACK_FALLBACK_SAMPLE_RATE = 22050,
    SMACK_FALLBACK_BITS_PER_SAMPLE = 8
H1_ENUM_CONST_END(SmackSoundConstant)

// The WIN2 movie frame from which SmackMain draws the victory text over it.
H1_ENUM_CONST_BEGIN(SmackTextConstant)
    SMACK_WIN2_TEXT_FIRST_FRAME = 22
H1_ENUM_CONST_END(SmackTextConstant)

#pragma pack(push, 1)
struct SmackSoundFormat {
    DWORD format;
    WORD channels;
    DWORD samplesPerSecond;
    WORD bitsPerSample;
};
#pragma pack(pop)

#endif
