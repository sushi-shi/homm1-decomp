#ifndef HOMM1_BASE_ICONENTRY_H
#define HOMM1_BASE_ICONENTRY_H

// HoMM1's packed icon-frame directory; width is at +4, stride is 12.
#pragma pack(push, 1)
struct IconEntry {
    short x;
    short y;
    short w;
    short h;
    unsigned long srcOffset;
};
#pragma pack(pop)

#endif // HOMM1_BASE_ICONENTRY_H
