#ifndef HOMM1_BASE_BMAP2_H
#define HOMM1_BASE_BMAP2_H

class bitmap;

void BlitBitmap(
    bitmap* source,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height,
    bitmap* destination,
    i32 destinationX,
    i32 destinationY
);
void DimBitmapArea(bitmap* image, i32 x, i32 y, i32 width, i32 height);
void FillBitmapArea(bitmap* image, i32 x, i32 y, i32 width, i32 height, i32 color);

#endif
