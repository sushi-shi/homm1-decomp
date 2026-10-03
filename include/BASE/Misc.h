#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

class bitmap;

// Map-grid (taxicab) distance of an offset (Buka 2.1 Misc.h).
#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

int Random(int, int);
extern unsigned long iLastSeed;
int SGenRand(void);
int SRandom(int, int);
void SIncRandomize(int, int);
void SRand(int);
void CycleColors(void);
void FadeIn(int);
void FadeOut(int);
void SetPalette(signed char*, int);
void ProcessAssert(int, char*, int);
void LogTruncate();
void PrintMemoryLeaks(void);
void LogStr(char*);
void LogInt(char*, int);
void LogStr(char*, long, long);
void LogStr(char*, long, long, long, long, long);
void LogStr(char*, long, long, long, long, long, long, long);
void AiPrint(char*);
void AbsAiPrint(char*);
void PostprocessPalette(signed char*);
void BlitBitmapToScreen(bitmap*, int, int, int, int, int, int);

void WritePrefs();

#endif
