#ifndef HOMM1_SOURCE_SMACKMANAGER_H
#define HOMM1_SOURCE_SMACKMANAGER_H

#include <SOURCE/smack.h>

enum SmackVideo {
    SMACK_BUKA = 0,
    SMACK_NWCLOGO = 1,
    SMACK_INTRO = 2,
    SMACK_LOSE = 3,
    SMACK_WIN1 = 4,
    SMACK_WIN2 = 5,
    SMACK_COUNT = 6
};

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

extern SSmackOptions gSmackOptions[SMACK_COUNT];
extern i8 gMovieId;
// Movie sound: Miles on Windows, the platform audio device in the native port.
void InitSmackSound();
void ShutdownSmackSound();
i32 SmackSoundReady();
void UseSmackSound(i32 volume);
void ConvertSmackerPalette(u8* paletteData);
void DoAdvance(Smack* smack, b32 drawFrame, b32 advanceFrame, b32 updatePalette, b32 skipPalette);
void SmackMain();
void CloseSmackers();
b32 PlaySmacker(i32 smackNumber);

enum SmackSoundConstant {
    SMACK_SOUND_FORMAT_COUNT = 12,
    SMACK_FALLBACK_CHANNELS = 1,
    SMACK_FALLBACK_SAMPLE_RATE = 22050,
    SMACK_FALLBACK_BITS_PER_SAMPLE = 8
};

enum SmackTextConstant {
    SMACK_WIN2_TEXT_FIRST_FRAME = 22
};

#endif
