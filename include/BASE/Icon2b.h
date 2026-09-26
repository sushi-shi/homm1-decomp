#ifndef HOMM1_BASE_ICON2B_H
#define HOMM1_BASE_ICON2B_H

class icon;
class bitmap;
void IconToBitmap(icon *, bitmap *, int, int, int, int);
void FlipIconToBitmap(icon *, bitmap *, int, int, int, int);
void ClippedIconToBitmap(icon *, bitmap *, int, int, int, int);
void FlipClippedIconToBitmap(icon *, bitmap *, int, int, int, int);

#endif // HOMM1_BASE_ICON2B_H
