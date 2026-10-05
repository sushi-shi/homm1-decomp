#ifndef HOMM1_SOURCE_SMACKMANAGER_H
#define HOMM1_SOURCE_SMACKMANAGER_H

#include <Domains.h>
#include <SOURCE/smack.h>

H1_ENUM_BEGIN(SmackVideo)
    SMACK_BUKA = 0,
    SMACK_NWCLOGO = 1,
    SMACK_INTRO = 2,
    SMACK_LOSE = 3,
    SMACK_WIN1 = 4,
    SMACK_WIN2 = 5
H1_ENUM_END(SmackVideo)

// Buka retail 0x0049f850: six packed rows, 0x1b bytes each.
#pragma pack(push, 1)
struct SSmackOptions {
    char fileName[9];
    char companionFileName[9];
    i8 fadeIn;
    i8 fadeOut;
    i8 preload;
    i8 waitForInput;
    i8 drawCompanion;
    i16 companionX;
    i16 companionY;
};
#pragma pack(pop)

extern SSmackOptions SmackOptions[6];
extern i8 gSmackNum;
void InitSmackSound();
void ShutdownSmackSound();
void ConvertSmackerPalette(u8* paletteData);
void DoAdvance(Smack* smack, i32 drawFrame, i32 advanceFrame, i32 updatePalette, i32 skipPalette);
void SmackMain();
void CloseSmackers();
i32 PlaySmacker(H1_ENUM_PARAM(SmackVideo, i32) smackNumber);

#endif
