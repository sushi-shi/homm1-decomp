#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

class bitmap;

// Map-grid (taxicab) distance of an offset (Buka 2.1 Misc.h).
#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

void SetPalette(signed char*, int);
void LogTruncate();
void LogInt(char*, int);
void LogStr(char*);
void LogStr(char*, long, long);
void LogStr(char*, long, long, long, long, long);
void LogStr(char*, long, long, long, long, long, long, long);

#endif
