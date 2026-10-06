// Portable equivalent of the Icon2bc.asm clipped icon-to-bitmap blitters, built by the native port.

#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>

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

// Clipping state, public in Icon2bc.asm. Columns are counted in icon space
// from the frame's first column.
//
// gClipColumn is only reset at the end of a row, never on entry, so the first
// row of a call starts from whatever the previous clipped draw left behind;
// this is kept because it feeds the right-edge test exactly as in the asm.
extern "C" {
i32 gClipRowsLeft;     // rows still to draw
i32 gClipLeftSkip;     // icon columns hidden left of the edge (per row)
i32 gClipVisibleWidth; // right clip column, 0 when nothing is cut on the right
i32 gClipRowSkip;      // hidden columns still to pass on the current row
i32 gClipColumn;       // icon column reached on the current row
}

namespace {

// The clip counters are 32-bit registers in the asm; arithmetic wraps.
inline i32 Plus(i32 value, u32 amount) {
    return static_cast<i32>(static_cast<u32>(value) + amount);
}

inline i32 Minus(i32 value, u32 amount) {
    return static_cast<i32>(static_cast<u32>(value) - amount);
}

// Drops the rest of a row by scanning raw bytes up to and including the next
// 0 byte. Pixel bytes are scanned as well, so a literal 0 pixel also ends the
// scan (asm behaviour). The scan stops at the end of the icon's data.
ptrdiff_t SkipToRowEnd(const IconSource& source, ptrdiff_t command) {
    return source.AfterRowEnd(command);
}

// Vertical clipping shared by both blitters. Rows above the top edge are
// skipped with SkipToRowEnd; sets gClipRowsLeft. Returns false when nothing is
// drawn.
bool ClipRows(const IconSource& source, ptrdiff_t& command, i32& y, i32 yAdjust, u32 height,
              const bitmap* bmp) {
    u32 top = static_cast<u32>(yAdjust) + static_cast<u32>(y);
    if (IsNegative(top)) {
        u32 hiddenRows = 0u - top;
        if (static_cast<i32>(height) <= static_cast<i32>(hiddenRows))
            return false;
        height -= hiddenRows;
        do {
            command = SkipToRowEnd(source, command);
        } while (--hiddenRows != 0);
        top = 0;
    }

    u32 bitmapHeight = static_cast<u16>(bmp->m_height);
    if (static_cast<i32>(bitmapHeight) <= static_cast<i32>(top))
        return false;
    y = static_cast<i32>(top);
    u32 bottom = top + height;
    if (static_cast<i32>(bottom) <= static_cast<i32>(bitmapHeight))
        gClipRowsLeft = static_cast<i32>(height);
    else
        gClipRowsLeft = static_cast<i32>(height - (bottom - bitmapHeight));
    return true;
}

// Runs the frame's RLE stream (see IconFrame.h) with horizontal clipping.
// direction is +1 for normal drawing and -1 for the mirrored variant, which
// writes and skips leftwards; the two asm loops are otherwise identical.
//
// Retained asm quirks:
//  - A skip run that straddles the left edge adds only its visible part to
//    gClipColumn (a straddling pixel run adds both parts).
//  - Skip runs inside the left-skip area are never tested against the right
//    clip column.
//  - When a pixel run reaches the right clip column, the rest of the row is
//    dropped with SkipToRowEnd starting at the run's unread pixel bytes.
void DrawClippedRuns(const IconSource& source, ptrdiff_t command, const IconTarget& target,
                     ptrdiff_t rowStart, u32 stride, ptrdiff_t direction) {
    ptrdiff_t to = rowStart;

    // Moves the destination `amount` pixels along the drawing direction.
    auto advance = [&](u32 amount) {
        to += direction * static_cast<ptrdiff_t>(static_cast<i32>(amount));
    };
    // Draws `count` literal pixel bytes. A count with the sign bit set would
    // be a multi-gigabyte runaway copy in the asm; it draws nothing here.
    auto draw = [&](u32 count) {
        if (IsNegative(count))
            return;
        if (direction > 0) {
            target.Copy(to, source, command, count);
            to += count;
            command += count;
        } else {
            for (u32 i = 0; i < count; ++i)
                target.Put(to--, source.Pixel(command++));
        }
    };
    // End of row: returns false once gClipRowsLeft reaches 0.
    auto nextRow = [&]() {
        rowStart += stride;
        to = rowStart;
        gClipColumn = 0;
        gClipRowSkip = gClipLeftSkip;
        gClipRowsLeft = Minus(gClipRowsLeft, 1);
        return gClipRowsLeft != 0;
    };

    gClipRowSkip = gClipLeftSkip;
    for (;;) {
        u32 code = source.Command(command++);

        if (code & 0x80) {
            u32 run = code & 0x7F;
            if (run == 0)
                return;
            if (gClipLeftSkip != 0) {
                if (!IsNegative(static_cast<u32>(gClipRowSkip) - run)) {
                    gClipRowSkip = Minus(gClipRowSkip, run);
                    gClipColumn = Plus(gClipColumn, run);
                } else {
                    run -= static_cast<u32>(gClipRowSkip);
                    advance(run);
                    gClipColumn = Plus(gClipColumn, run);
                    gClipRowSkip = 0;
                }
                continue;
            }
            if (gClipVisibleWidth != 0
                && IsNegative(static_cast<u32>(gClipVisibleWidth) - static_cast<u32>(gClipColumn) - run)) {
                command = SkipToRowEnd(source, command);
                if (!nextRow())
                    return;
                continue;
            }
            advance(run);
            gClipColumn = Plus(gClipColumn, run);
            continue;
        }

        if (code == 0) {
            if (!nextRow())
                return;
            continue;
        }

        u32 run = code;
        if (gClipLeftSkip != 0) {
            if (!IsNegative(static_cast<u32>(gClipRowSkip) - run)) {
                // Whole run is left of the edge: pass over its pixel bytes.
                command += run;
                gClipColumn = Plus(gClipColumn, run);
                gClipRowSkip = Minus(gClipRowSkip, run);
                continue;
            }
            u32 hidden = static_cast<u32>(gClipRowSkip);
            command += static_cast<i32>(hidden);
            run -= hidden;
            gClipColumn = Plus(gClipColumn, hidden);
            gClipRowSkip = 0;
        }
        if (gClipVisibleWidth != 0) {
            u32 room = static_cast<u32>(gClipVisibleWidth) - static_cast<u32>(gClipColumn);
            if (room == 0) {
                command = SkipToRowEnd(source, command);
                if (!nextRow())
                    return;
                continue;
            }
            if (IsNegative(room - run)) {
                gClipColumn = Plus(gClipColumn, room);
                draw(room);
                command = SkipToRowEnd(source, command);
                if (!nextRow())
                    return;
                continue;
            }
        }
        gClipColumn = Plus(gClipColumn, run);
        draw(run);
    }
}

} // namespace

// IconToBitmap with clipping against all four bitmap edges. (x, y) plus the
// frame offset is the frame's top-left corner.
void ClippedIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    u32 width = f.width;
    ptrdiff_t command = f.commands;

    u32 left = static_cast<u32>(f.xAdjust) + static_cast<u32>(x);
    if (IsNegative(left)) {
        u32 hidden = 0u - left;
        gClipLeftSkip = static_cast<i32>(hidden);
        width -= hidden;
        x = 0;
    } else {
        gClipLeftSkip = 0;
        x = static_cast<i32>(left);
    }

    if (!ClipRows(f.source, command, y, f.yAdjust, f.height, bmp))
        return;

    u32 bitmapWidth = static_cast<u16>(bmp->m_width);
    if (static_cast<i32>(bitmapWidth) <= x)
        return;
    u32 right = static_cast<u32>(x) + width;
    if (static_cast<i32>(right) <= static_cast<i32>(bitmapWidth))
        gClipVisibleWidth = 0;
    else
        gClipVisibleWidth = static_cast<i32>(width - (right - bitmapWidth));

    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), bitmapWidth, static_cast<u32>(x));
    DrawClippedRuns(f.source, command, TargetOf(bmp), rowStart, bitmapWidth, 1);
}

// Mirrored ClippedIconToBitmap: x minus the frame offset is the rightmost
// column and the frame is drawn leftwards. "Left skip" here counts the
// columns hidden past the right edge, and the visible width is x + 1 when the
// frame reaches past column 0.
//
// When x - offset is negative (the frame lies entirely left of the bitmap) the
// asm computes a negative gClipLeftSkip and then reads pixel bytes before the
// run and draws backwards from the right edge across rows; that is
// reproduced, not prevented.
void FlipClippedIconToBitmap(icon* ic, bitmap* bmp, i32 x, i32 y, i32 frame, i32 offsetMode) {
    IconFrame f = LoadIconFrame(ic, frame, offsetMode);
    u32 width = f.width;
    ptrdiff_t command = f.commands;

    x = static_cast<i32>(static_cast<u32>(x) - static_cast<u32>(f.xAdjust));
    u32 bitmapWidth = static_cast<u16>(bmp->m_width);
    // add eax, x with eax = 1 - width; `ja` (no carry, non-zero) means x is
    // left of the last column as an unsigned compare.
    u32 lastColumnNegated = 1u - bitmapWidth;
    u64 sum = static_cast<u64>(lastColumnNegated) + static_cast<u32>(x);
    u32 overhang = static_cast<u32>(sum);
    bool carry = sum > 0xFFFFFFFFu;
    if (!carry && overhang != 0) {
        gClipLeftSkip = 0;
    } else {
        gClipLeftSkip = static_cast<i32>(overhang);
        width -= overhang;
        x = static_cast<i32>(0u - lastColumnNegated);
    }

    if (!ClipRows(f.source, command, y, f.yAdjust, f.height, bmp))
        return;

    if (x < 0)
        return;
    u32 leftmost = static_cast<u32>(x) - width + 1;
    if (!IsNegative(leftmost))
        gClipVisibleWidth = 0;
    else
        gClipVisibleWidth = static_cast<i32>(leftmost + width);

    ptrdiff_t rowStart = Offset32(static_cast<u32>(y), bitmapWidth, static_cast<u32>(x));
    DrawClippedRuns(f.source, command, TargetOf(bmp), rowStart, bitmapWidth, -1);
}
