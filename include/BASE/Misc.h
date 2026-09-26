#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

class bitmap;

int Random(int, int);
void CycleColors(void);
void FadeIn(int);
void FadeOut(int);
void SetPalette(signed char *, int);
void ProcessAssert(int, char *, int);
void LogTruncate();
void LogStr(char *);
void LogInt(char *, int);
void LogStr(char *, long, long);
void LogStr(char *, long, long, long, long, long);
void BlitBitmapToScreen(
    bitmap *, int, int, int, int, int, int);

void WritePrefs();

#endif
