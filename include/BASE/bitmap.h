#ifndef HOMM1_BASE_BITMAP_H
#define HOMM1_BASE_BITMAP_H

#include <BASE/resource.h>
#include <H1/Macros.h>

#pragma pack(push, 1)
class bitmap : public resource {
public:
    short m_bitmapType;
    short m_width;
    short m_height;
    signed char *m_pixels;

    // --- constructors ---
    bitmap(void);
    bitmap(short int, short int, short int);
    bitmap(unsigned long int);
    virtual ~bitmap();
    // --- methods ---
    void DrawToBufferCareful(short int, short int);
    void DrawToBuffer(short int, short int);
    void DrawToScreen(short int, short int);
    void GrabScreen(short int, short int);
    void GrabBitmap(class bitmap *, short int, short int);
    void GrabBitmapCareful(class bitmap *, short int, short int);
    void CopyTo(class bitmap *, int, int, int, int, int, int);
    void CopyToCareful(class bitmap *, int, int, int, int, int, int);
};
#pragma pack(pop)

void PostprocessBitmap(signed char *, int, int);
#endif // HOMM1_BASE_BITMAP_H
