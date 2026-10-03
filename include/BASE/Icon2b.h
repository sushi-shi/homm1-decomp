#ifndef HOMM1_BASE_ICON2B_H
#define HOMM1_BASE_ICON2B_H

class icon;
class bitmap;
void IconToBitmap(icon * ic, bitmap * bmp, i32 x, i32 y, i32 frame, i32 offsetMode);
void FlipIconToBitmap(icon * ic, bitmap * bmp, i32 x, i32 y, i32 frame, i32 offsetMode);
void ClippedIconToBitmap(icon * ic, bitmap * bmp, i32 x, i32 y, i32 frame, i32 offsetMode);
void FlipClippedIconToBitmap(icon * ic, bitmap * bmp, i32 x, i32 y, i32 frame, i32 offsetMode);

#endif // HOMM1_BASE_ICON2B_H
