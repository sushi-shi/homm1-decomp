#ifndef HOMM1_BASE_BMAP2_H
#define HOMM1_BASE_BMAP2_H

class bitmap;

void BlitBitmap(
    bitmap *, int, int, int, int, bitmap *, int, int);
void GrabScreenBitmap(bitmap *, int, int);

#endif
