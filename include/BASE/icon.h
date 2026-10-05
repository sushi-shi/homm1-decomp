#ifndef HOMM1_BASE_ICON_H
#define HOMM1_BASE_ICON_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 0 own-virtual, 0 static data.

#include <BASE/IconEntry.h>
#include <BASE/resource.h>

// forward declarations:
struct SLimitData;

H1_ENUM_CONST_BEGIN(IconMonoRleConstant)
    ICON_MONO_SKIP_MASK = 0x7f,
    ICON_MONO_END_COMMAND = 0x80,
    ICON_MONO_NEWLINE_COMMAND = 0
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
    i16 m_frameCount;
    // The loaded ICN resource: a leading directory of m_frameCount packed
    // IconEntry records, then the frame pixels each srcOffset addresses.
    // Buka's font code reads the directory as words (font.h).
    union {
        u8* m_data;
        IconEntry* m_frames;
        i16* m_frameWords;
    };
    i16 m_drawLeft;
    i16 m_drawRight;
    i16 m_drawTop;
    i16 m_drawBottom;
    // --- constructors ---
    icon(i16 id);
    virtual inline ~icon();
    // --- methods ---
    void DrawToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
        H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode
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
        H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
        H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode,
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
        H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
        H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode
    );
    void DimToBuffer(
        i16 x,
        i16 y,
        i16 frame,
        H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
        H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode
    );
};
#pragma pack(pop)

#endif // HOMM1_BASE_ICON_H
