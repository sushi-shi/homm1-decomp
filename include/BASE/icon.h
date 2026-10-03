#ifndef HOMM1_BASE_ICON_H
#define HOMM1_BASE_ICON_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 0 own-virtual, 0 static data.

#include <BASE/resource.h>

// forward declarations:
struct SLimitData;

H1_ENUM_CONST_BEGIN(IconMonoRleConstant)
    ICON_MONO_SKIP_MASK = 0x7f,
    ICON_MONO_END_COMMAND = 0x80,
    ICON_MONO_NEWLINE_COMMAND = 0,
    ICON_SCREEN_ROW_BYTES = 640
H1_ENUM_CONST_END(IconMonoRleConstant)

// The orientation argument of the icon blitters: FLIPPED selects the
// mirrored Flip*IconToBitmap path (Buka IconDraw.h IconDrawOrientation).
H1_ENUM_BEGIN(IconDrawOrientation)
    ICON_DRAW_NORMAL = 0,
    ICON_DRAW_FLIPPED = 1
H1_ENUM_END(IconDrawOrientation)

// The last argument of the icon blitters. Non-zero quarters the frame's x
// offset (Icon2b.asm: x - ((x - x/2) >> 1) at [ebp+1ch]; DrawToBuffer's
// extent uses x >> 2); army walk frames 1..5 pass it.
H1_ENUM_BEGIN(IconDrawOffsetMode)
    ICON_DRAW_OFFSET_FULL = 0,
    ICON_DRAW_OFFSET_QUARTER = 1
H1_ENUM_END(IconDrawOffsetMode)

H1_ENUM_CONST_BEGIN(IconDrawOffsetConstant)
    ICON_DRAW_QUARTER_OFFSET_SHIFT = 2
H1_ENUM_CONST_END(IconDrawOffsetConstant)

#pragma pack(push, 1)
class icon : public resource {
public:
    short m_frameCount;
    unsigned char* m_data;
    short m_drawLeft;
    short m_drawRight;
    short m_drawTop;
    short m_drawBottom;
    // --- constructors ---
    icon(short);
    virtual inline ~icon();
    // --- methods ---
    void DrawToBuffer(
        short,
        short,
        short,
        H1_ENUM_PARAM(IconDrawOrientation, signed char),
        H1_ENUM_PARAM(IconDrawOffsetMode, signed char)
    );
    int CombatClipDrawToBuffer(
        int x,
        int y,
        int frame,
        struct SLimitData* limits,
        int orientation,
        int offset,
        unsigned char* colorTable,
        signed char* yModify
    );
    void ClipFillToBuffer(
        short,
        short,
        short,
        short,
        H1_ENUM_PARAM(IconDrawOrientation, signed char),
        H1_ENUM_PARAM(IconDrawOffsetMode, signed char),
        int,
        int,
        int,
        int
    );
    void FillToBuffer(
        short,
        short,
        short,
        short,
        H1_ENUM_PARAM(IconDrawOrientation, signed char),
        H1_ENUM_PARAM(IconDrawOffsetMode, signed char)
    );
    void DimToBuffer(
        short,
        short,
        short,
        H1_ENUM_PARAM(IconDrawOrientation, signed char),
        H1_ENUM_PARAM(IconDrawOffsetMode, signed char)
    );
};
#pragma pack(pop)

#endif // HOMM1_BASE_ICON_H
