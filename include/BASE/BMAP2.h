#ifndef HOMM1_BASE_BMAP2_ROUTINES_H
#define HOMM1_BASE_BMAP2_ROUTINES_H

class bitmap;
void DimBitmapArea(bitmap* bmp, i32 x, i32 y, i32 w, i32 h);
void FillBitmapArea(bitmap* image, i32 x, i32 y, i32 width, i32 height, i32 color);

#endif
