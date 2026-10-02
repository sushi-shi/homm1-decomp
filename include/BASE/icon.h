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

// clang-format off
// The orientation argument of the icon blitters: FLIPPED selects the
// mirrored Flip*IconToBitmap path (Buka IconDraw.h IconDrawOrientation).
H1_ENUM_BEGIN(IconDrawOrientation)
    ICON_DRAW_NORMAL = 0,
    ICON_DRAW_FLIPPED = 1
H1_ENUM_END(IconDrawOrientation)
// clang-format on

#pragma pack(push, 1)
class icon : public resource {
public:
    short m_frameCount;
    unsigned char *m_data;
    short m_drawLeft;
    short m_drawRight;
    short m_drawTop;
    short m_drawBottom;
    // --- constructors ---
    icon(short);
    virtual inline ~icon();
    // --- methods ---
    void DrawToBuffer(short, short, short, H1_ENUM_PARAM(IconDrawOrientation, signed char), signed char);
    int CombatClipDrawToBuffer(int, int, int, struct SLimitData *, int, int, unsigned char *, signed char *);
    void ClipFillToBuffer(short, short, short, short, H1_ENUM_PARAM(IconDrawOrientation, signed char), signed char, int, int,
                          int, int);
    void FillToBuffer(short, short, short, short, H1_ENUM_PARAM(IconDrawOrientation, signed char), signed char);
    void DimToBuffer(short, short, short, H1_ENUM_PARAM(IconDrawOrientation, signed char), signed char);
};
#pragma pack(pop)

void PostprocessIcon(icon *);

extern signed char gbIconClipOn;
extern int gbComputeExtent;
extern int gbSaveBiggestExtent;
extern int gbLimitToExtent;
extern int gbCurrArmyDrawn;
extern int giMaxExtentX;
extern int giMaxExtentY;
extern int giMinExtentX;
extern int giMinExtentY;
extern unsigned char gMonoColorMap[];
extern int giMonoIconSkip;
#endif // HOMM1_BASE_ICON_H
