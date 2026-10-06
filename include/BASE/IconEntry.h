#ifndef HOMM1_BASE_ICONENTRY_H
#define HOMM1_BASE_ICONENTRY_H

// Packed icon-frame directory; width is at +4, stride is 12.
#pragma pack(push, 1)
struct IconEntry {
    i16 x;
    i16 y;
    i16 w;
    i16 h;
    u32 srcOffset;
};
#pragma pack(pop)

#endif // HOMM1_BASE_ICONENTRY_H
