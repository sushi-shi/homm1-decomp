#ifndef HOMM1_BASE_BITMAP_H
#define HOMM1_BASE_BITMAP_H

#include <BASE/resource.h>

enum BitmapType {
    BITMAP_TYPE_NONE = 0,
    BITMAP_TYPE_MEMORY = 0x21
};

enum BitmapFileConstant {
    // The type, width and height before a bitmap's pixels.
    BITMAP_HEADER_SIZE = 6
};

#pragma pack(push, 1)
class bitmap : public resource {
public:
    i16 m_bitmapType;
    i16 m_width;
    i16 m_height;
    u8* m_pixels;

    bitmap(void);
    bitmap(i16 type, i16 width, i16 height);
    bitmap(i16 id);
    virtual ~bitmap();
    void DrawToBuffer(i16 x, i16 y);
    void DrawToScreen(i16 x, i16 y);
    void GrabScreen(i16 x, i16 y);
    void GrabBitmap(class bitmap* source, i16 x, i16 y);
    void Write(char* filename);
    void CopyTo(
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

#endif
