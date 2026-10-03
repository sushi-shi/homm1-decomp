#ifndef HOMM1_BASE_ICOND2B_H
#define HOMM1_BASE_ICOND2B_H

class icon;
class bitmap;
void DimIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode);
void FlipDimIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode);

#endif // HOMM1_BASE_ICOND2B_H
