#ifndef HOMM1_BASE_BITMAP_H
#define HOMM1_BASE_BITMAP_H

#include <BASE/resource.h>
#include <H1/Macros.h>

// bitmap::m_bitmapType: 0x21 marks a plain off-screen memory bitmap (Buka bitmap.h).
H1_ENUM_BEGIN_SPLIT(BitmapType, short)
    BITMAP_TYPE_NONE = 0,
    BITMAP_TYPE_MEMORY = 0x21
H1_ENUM_END_SPLIT(BitmapType)

H1_ENUM_CONST_BEGIN(BitmapCopyConstant)
    BITMAP_COPY_STRIDE = 640
H1_ENUM_CONST_END(BitmapCopyConstant)

#pragma pack(push, 1)
class bitmap : public resource {
public:
    H1_ENUM_STORAGE(BitmapType, short) m_bitmapType;
    short m_width;
    short m_height;
    signed char* m_pixels;

    // --- constructors ---
    bitmap(void);
    bitmap(short int, short int, short int);
    bitmap(short);
    virtual inline ~bitmap();
    // --- methods ---
    void DrawToBufferCareful(short int x, short int y);
    void DrawToBuffer(short int, short int);
    void DrawToScreen(short int x, short int y);
    void GrabScreen(short int, short int);
    void GrabBitmap(class bitmap*, short int, short int);
    void GrabBitmapCareful(class bitmap* source, short int x, short int y);
    void Write(char*);
    void CopyTo(class bitmap*, int, int, int, int, int, int);
    void CopyToCareful(class bitmap* destination, int destinationX, int destinationY, int sourceX, int sourceY, int width, int height);
};
#pragma pack(pop)

void PostprocessBitmap(signed char*, int, int);
#endif // HOMM1_BASE_BITMAP_H
