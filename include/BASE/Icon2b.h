#ifndef HOMM1_BASE_ICON2B_H
#define HOMM1_BASE_ICON2B_H

class icon;
class bitmap;
void IconToBitmap(icon *, bitmap *, i32, i32, i32, i32);
void FlipIconToBitmap(icon *, bitmap *, i32, i32, i32, i32);
void ClippedIconToBitmap(icon *, bitmap *, i32, i32, i32, i32);
void FlipClippedIconToBitmap(icon *, bitmap *, i32, i32, i32, i32);

#endif // HOMM1_BASE_ICON2B_H
