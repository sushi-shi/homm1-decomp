#ifndef HOMM1_BASE_ICON_H
#define HOMM1_BASE_ICON_H

#include <BASE/resource.h>

struct SLimitData;

enum IconMonoRleConstant {
    ICON_MONO_SKIP_MASK = 0x7f,
    ICON_MONO_END_COMMAND = 0x80,
    ICON_MONO_NEWLINE_COMMAND = 0,
    ICON_SCREEN_ROW_BYTES = 640
};

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

#pragma pack(push, 1)
class icon : public resource {
public:
    i16 m_frameCount;
    u8* m_data;
    i16 m_drawLeft;
    i16 m_drawRight;
    i16 m_drawTop;
    i16 m_drawBottom;
    icon(i16 id);
    virtual inline ~icon();
    void DrawToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i8 orientation,
        i8 mode
    );
    i32 CombatClipDrawToBuffer(
        i32 x,
        i32 y,
        i32 frame,
        struct SLimitData* limits,
        i32 orientation,
        i32 offset,
        u8* colorTable,
        i8* yModify
    );
    void ClipFillToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i16 color,
        i8 orientation,
        i8 mode,
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
        i8 mode
    );
    void DimToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        i8 orientation,
        i8 mode
    );
};
#pragma pack(pop)

#endif
