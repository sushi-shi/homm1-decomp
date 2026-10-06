#ifndef HOMM1_BASE_ICOND2B_H
#define HOMM1_BASE_ICOND2B_H

#include <BASE/icon.h>
#include <Domains.h>

class bitmap;
void DimIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, H1_ENUM_PARAM(IconDrawOffsetMode, i32) offsetMode);
void FlipDimIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, H1_ENUM_PARAM(IconDrawOffsetMode, i32) offsetMode);

#endif // HOMM1_BASE_ICOND2B_H
