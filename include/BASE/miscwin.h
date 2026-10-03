#ifndef HOMM1_BASE_MISCWIN_H
#define HOMM1_BASE_MISCWIN_H

// miscwin.cpp: Windows-side screen, palette and clipped-icon helpers.
void ClippedMonoIconToBitmap(class icon*, class bitmap*, int, int, int, int, int, int, int, int, int);
void ClipIconToBitmap(class icon*, class bitmap*, int, int, int, int, int, int, int, int);
int Random(int, int);
void FadeIn(int);
void FadeOut(int);
void PrintMemoryLeaks(void);
void PostprocessPalette(signed char*);
void BlitBitmapToScreen(class bitmap*, int, int, int, int, int, int);
void PostprocessBitmap(signed char*, int, int);
void GrabScreenBitmap(class bitmap*, int, int);
void PostprocessIcon(class icon*);

#endif // HOMM1_BASE_MISCWIN_H
