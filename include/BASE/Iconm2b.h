#ifndef HOMM1_BASE_ICONM2B_H
#define HOMM1_BASE_ICONM2B_H

class icon;
class bitmap;
void MonoIconToBitmap(icon *, bitmap *, int, int, int, int, int);
void FlipMonoIconToBitmap(icon *, bitmap *, int, int, int, int, int);

void ClippedMonoIconToBitmap(icon *, bitmap *, int, int, int, int, int, int, int, int, int);
void ClipIconToBitmap(icon *, bitmap *, int, int, int, int, int, int, int, int);

#endif // HOMM1_BASE_ICONM2B_H
