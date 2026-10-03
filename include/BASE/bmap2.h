#ifndef HOMM1_BASE_BMAP2_H
#define HOMM1_BASE_BMAP2_H

class bitmap;

void BlitBitmap(bitmap* source, int sourceX, int sourceY, int width, int height, bitmap* destination, int dx, int dy);
void GrabScreenBitmap(bitmap*, int, int);
void DimBitmapArea(bitmap*, int, int, int, int);
void FillBitmapArea(bitmap* image, int x, int y, int width, int height, int color);

#endif
