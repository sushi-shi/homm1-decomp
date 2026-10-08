#ifndef HOMM1_BASE_ICON_H
#define HOMM1_BASE_ICON_H

#include <BASE/IconEntry.h>
#include <BASE/resource.h>

enum IconMonoRleConstant {
    ICON_MONO_SKIP_MASK = 0x7f,
    ICON_MONO_END_COMMAND = 0x80,
    ICON_MONO_NEWLINE_COMMAND = 0
};

#define ICON_FITS_CLIP(left, top, width, height, clipX, clipY, clipW, clipH)                       \
    ((left) >= (clipX) && (left) + (width) <= (clipX) + (clipW) && (top) >= (clipY)                \
     && (top) + (height) <= (clipY) + (clipH))

enum IconDrawOrientation {
    ICON_DRAW_NORMAL = 0,
    ICON_DRAW_FLIPPED = 1
};

enum IconDrawOffsetMode {
    ICON_DRAW_OFFSET_FULL = 0,
    ICON_DRAW_OFFSET_QUARTER = 1
};

enum IconDrawOffsetConstant {
    ICON_DRAW_QUARTER_OFFSET_SHIFT = 2
};

enum IconFileConstant {
    // The frame count and the data length before an icon's data.
    ICON_HEADER_SIZE = 6
};

#pragma pack(push, 1)
class icon : public resource {
public:
    i16 m_frameCount;
    union {
        u8* m_data;
        IconEntry* m_frames;
        i16* m_frameWords;
    };
    i16 m_drawLeft;
    i16 m_drawRight;
    i16 m_drawTop;
    i16 m_drawBottom;
    // Bytes at m_data: the frame table and the frames' command streams. The
    // drawing routines read nothing outside them.
    u32 m_dataSize;
    icon(i16 id);
    virtual ~icon();
    void DrawToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i8 orientation,
        i8 offsetMode
    );
    void ClipFillToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i16 color,
        i8 orientation,
        i8 offsetMode,
        i32 clipX,
        i32 clipY,
        i32 clipW,
        i32 clipH
    );
    void FillToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i16 color,
        i8 orientation,
        i8 offsetMode
    );
    void DimToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i8 orientation,
        i8 offsetMode
    );
};
#pragma pack(pop)

#endif
