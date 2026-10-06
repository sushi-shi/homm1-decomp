#ifndef HOMM1_BASE_ICONM2B_H
#define HOMM1_BASE_ICONM2B_H

#include <BASE/icon.h>
#include <Domains.h>

class bitmap;
void MonoIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 color, H1_ENUM_PARAM(IconDrawOffsetMode, i32) offsetMode);
void FlipMonoIconToBitmap(
    icon* ic,
    bitmap* bmp,
    i32 x,
    i32 y,
    i32 frame,
    i32 color,
    H1_ENUM_PARAM(IconDrawOffsetMode, i32) offsetMode
);

#endif // HOMM1_BASE_ICONM2B_H
