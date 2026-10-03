#ifndef HOMM1_BASE_BITMAP_H
#define HOMM1_BASE_BITMAP_H

#include <BASE/resource.h>
#include <H1/Macros.h>

// bitmap::m_bitmapType: 0x21 marks a plain off-screen memory bitmap (Buka bitmap.h).
H1_ENUM_BEGIN_SPLIT(BitmapType, i16)
    BITMAP_TYPE_NONE = 0,
    BITMAP_TYPE_MEMORY = 0x21
H1_ENUM_END_SPLIT(BitmapType)

H1_ENUM_CONST_BEGIN(BitmapCopyConstant)
    BITMAP_COPY_STRIDE = 640
H1_ENUM_CONST_END(BitmapCopyConstant)

#pragma pack(push, 1)
class bitmap : public resource {
public:
    H1_ENUM_STORAGE(BitmapType, i16) m_bitmapType;
    i16 m_width;
    i16 m_height;
    i8* m_pixels;

    // --- constructors ---
    bitmap(void);
    bitmap(i16, i16, i16);
    bitmap(i16);
    virtual inline ~bitmap();
    // --- methods ---
    void DrawToBufferCareful(i16 x, i16 y);
    void DrawToBuffer(i16, i16);
    void DrawToScreen(i16 x, i16 y);
    void GrabScreen(i16, i16);
    void GrabBitmap(class bitmap*, i16, i16);
    void GrabBitmapCareful(class bitmap* source, i16 x, i16 y);
    void Write(char*);
    void CopyTo(class bitmap*, i32, i32, i32, i32, i32, i32);
    void CopyToCareful(
        class bitmap* destination,
        i32 destinationX,
        i32 destinationY,
        i32 sourceX,
        i32 sourceY,
        i32 width,
        i32 height
    );
};
#pragma pack(pop)

#endif // HOMM1_BASE_BITMAP_H
