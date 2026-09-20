#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

class bitmap;

void ProcessAssert(int, char *, int);
void LogStr(char *);
void BlitBitmapToScreen(
    bitmap *, int, int, int, int, int, int);

#endif
