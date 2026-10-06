// Portable equivalent of the Icon2b.asm icon-to-bitmap blitters, built by the native port.

#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>

#include "IconFrame.h"

#include <stddef.h>
#include <string.h>

using IconBlit::IconFrame;
using IconBlit::IconSource;
using IconBlit::IconTarget;
using IconBlit::IsNegative;
using IconBlit::LoadIconFrame;
using IconBlit::Offset32;
using IconBlit::TargetOf;

// Last frame header decoded by any icon blitter (see IconFrame.h). Public in
// Icon2b.asm; _gIconUnused is private there and never referenced.
extern "C" {
i32 gIconXAdjust;
i32 gIconYAdjust;
u8* gIconDataBase;
i32 gIconWidth;
i32 gIconHeight;
}

extern "C" u8 gDimPalette[256];

namespace {

// The dim blitters step rows by a fixed 640 bytes (the screen pitch), not by
// the destination bitmap's width, exactly as the asm does.
const ptrdiff_t kDimRowPitch = 0x280;

// value + delta (or value - delta), forced to 0 when the 32-bit result is
// negative.
inline i32 AddClamped(i32 value, i32 delta) {
    u32 sum = static_cast<u32>(value) + static_cast<u32>(delta);
    return IsNegative(sum) ? 0 : static_cast<i32>(sum);
}

inline i32 SubClamped(i32 value, i32 delta) {
    u32 difference = static_cast<u32>(value) - static_cast<u32>(delta);
    return IsNegative(difference) ? 0 : static_cast<i32>(difference);
}

// The right/bottom rejection compares only the low 16 bits, signed.
inline bool PastEdge(i32 origin, u32 extent, i16 edge) {
    return static_cast<i16>(static_cast<u16>(static_cast<u32>(origin) + extent)) > edge;
}

} // namespace

// Draws an icon frame with its origin at (x, y) + the frame offset. A negative
// origin is moved to 0 (not clipped); frames reaching past the right or
// bottom edge are not drawn at all.
void IconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    x = AddClamped(x, f.xAdjust);
    y = AddClamped(y, f.yAdjust);
    if (PastEdge(x, f.width, bmp->m_width) || PastEdge(y, f.height, bmp->m_height))
        return;

    IconTarget target = TargetOf(bmp);
    const IconSource& source = f.source;
    u32 stride = static_cast<u16>(bmp->m_width);
    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), stride, static_cast<u32>(x));
    ptrdiff_t to = rowStart;
    ptrdiff_t command = f.commands;
    for (;;) {
        u8 code = source.Command(command++);
        if (code & 0x80) {
            u32 skip = code & 0x7F;
            if (skip == 0)
                return;
            to += skip;
        } else if (code == 0) {
            rowStart += stride;
            to = rowStart;
        } else {
            target.Copy(to, source, command, code);
            command += code;
            to += code;
        }
    }
}

// Mirrored IconToBitmap: x (after subtracting the frame offset) is the
// rightmost column and runs are drawn leftwards. Rejected when the frame
// would reach left of column 0 or past the bottom edge.
void FlipIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    x = SubClamped(x, f.xAdjust);
    y = AddClamped(y, f.yAdjust);
    if (IsNegative(static_cast<u32>(x) - f.width + 1))
        return;
    if (PastEdge(y, f.height, bmp->m_height))
        return;

    IconTarget target = TargetOf(bmp);
    const IconSource& source = f.source;
    u32 stride = static_cast<u16>(bmp->m_width);
    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), stride, static_cast<u32>(x));
    ptrdiff_t to = rowStart;
    ptrdiff_t command = f.commands;
    for (;;) {
        u8 code = source.Command(command++);
        if (code & 0x80) {
            u32 skip = code & 0x7F;
            if (skip == 0)
                return;
            to -= skip;
        } else if (code == 0) {
            rowStart += stride;
            to = rowStart;
        } else {
            for (u32 i = 0; i < code; ++i)
                target.Put(to--, source.Pixel(command++));
        }
    }
}

// IconToBitmap for mono frames: each run is filled with the low byte of
// `color`; the command stream has no pixel bytes.
void MonoIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 color, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    x = AddClamped(x, f.xAdjust);
    y = AddClamped(y, f.yAdjust);
    if (PastEdge(x, f.width, bmp->m_width) || PastEdge(y, f.height, bmp->m_height))
        return;

    u8 fill = static_cast<u8>(color);
    IconTarget target = TargetOf(bmp);
    const IconSource& source = f.source;
    u32 stride = static_cast<u16>(bmp->m_width);
    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), stride, static_cast<u32>(x));
    ptrdiff_t to = rowStart;
    ptrdiff_t command = f.commands;
    for (;;) {
        u8 code = source.Command(command++);
        if (code & 0x80) {
            u32 skip = code & 0x7F;
            if (skip == 0)
                return;
            to += skip;
        } else if (code == 0) {
            rowStart += stride;
            to = rowStart;
        } else {
            target.Fill(to, fill, code);
            to += code;
        }
    }
}

// Mirrored MonoIconToBitmap. Note the left rejection here is x - width < 0
// (FlipIconToBitmap uses x - width + 1 < 0).
void FlipMonoIconToBitmap(
    icon* ic,
    bitmap* bmp,
    i32 x,
    i32 y,
    i32 frame,
    i32 color,
    i32 offsetMode
) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    x = SubClamped(x, f.xAdjust);
    y = AddClamped(y, f.yAdjust);
    if (IsNegative(static_cast<u32>(x) - f.width))
        return;
    if (PastEdge(y, f.height, bmp->m_height))
        return;

    u8 fill = static_cast<u8>(color);
    IconTarget target = TargetOf(bmp);
    const IconSource& source = f.source;
    u32 stride = static_cast<u16>(bmp->m_width);
    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), stride, static_cast<u32>(x));
    ptrdiff_t to = rowStart;
    ptrdiff_t command = f.commands;
    for (;;) {
        u8 code = source.Command(command++);
        if (code & 0x80) {
            u32 skip = code & 0x7F;
            if (skip == 0)
                return;
            to -= skip;
        } else if (code == 0) {
            rowStart += stride;
            to = rowStart;
        } else {
            for (u32 i = 0; i < code; ++i)
                target.Put(to--, fill);
        }
    }
}

// Darkens the destination under the frame's runs through gDimPalette; the
// command stream has no pixel bytes. Rows advance by 640 bytes.
void DimIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    x = AddClamped(x, f.xAdjust);
    y = AddClamped(y, f.yAdjust);
    if (PastEdge(x, f.width, bmp->m_width) || PastEdge(y, f.height, bmp->m_height))
        return;

    IconTarget target = TargetOf(bmp);
    const IconSource& source = f.source;
    u32 stride = static_cast<u16>(bmp->m_width);
    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), stride, static_cast<u32>(x));
    ptrdiff_t to = rowStart;
    ptrdiff_t command = f.commands;
    for (;;) {
        u8 code = source.Command(command++);
        if (code & 0x80) {
            u32 skip = code & 0x7F;
            if (skip == 0)
                return;
            to += skip;
        } else if (code == 0) {
            rowStart += kDimRowPitch;
            to = rowStart;
        } else {
            for (u32 i = 0; i < code; ++i, ++to)
                target.Put(to, gDimPalette[target.Get(to)]);
        }
    }
}

// Mirrored DimIconToBitmap (left rejection x - width < 0, rows advance by 640).
void FlipDimIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    x = SubClamped(x, f.xAdjust);
    y = AddClamped(y, f.yAdjust);
    if (IsNegative(static_cast<u32>(x) - f.width))
        return;
    if (PastEdge(y, f.height, bmp->m_height))
        return;

    IconTarget target = TargetOf(bmp);
    const IconSource& source = f.source;
    u32 stride = static_cast<u16>(bmp->m_width);
    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), stride, static_cast<u32>(x));
    ptrdiff_t to = rowStart;
    ptrdiff_t command = f.commands;
    for (;;) {
        u8 code = source.Command(command++);
        if (code & 0x80) {
            u32 skip = code & 0x7F;
            if (skip == 0)
                return;
            to -= skip;
        } else if (code == 0) {
            rowStart += kDimRowPitch;
            to = rowStart;
        } else {
            for (u32 i = 0; i < code; ++i, --to)
                target.Put(to, gDimPalette[target.Get(to)]);
        }
    }
}
