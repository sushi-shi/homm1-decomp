// Shared frame-header prologue of the portable Icon2b/Icon2bc blitters, built by the native port.
#ifndef HOMM1_PORT_BASE_ICONFRAME_H
#define HOMM1_PORT_BASE_ICONFRAME_H

#include <H1/Ints.h>

#include <BASE/icon.h>
#include <BASE/IconEntry.h>

#include <stddef.h>
#include <string.h>

// Icon frame data
// ---------------
// icon::m_data starts with one IconEntry per frame. IconEntry::srcOffset is
// the offset from m_data to the frame's RLE command stream, read one byte at a
// time:
//   0x00         end of row: continue at the start of the next row
//   0x01..0x7F   n pixels: n literal pixel bytes follow and are drawn
//                (mono and dim frames carry no pixel bytes; the run is filled
//                with a colour or dimmed in place)
//   0x80         end of frame
//   0x81..0xFF   skip (command & 0x7F) pixels
// Flipped variants draw right to left from the anchor column.

// Last frame header decoded by any icon blitter. Defined in Icon2b.cpp
// (public in Icon2b.asm); nothing else in the game reads them.
extern "C" {
extern i32 gIconXAdjust;
extern i32 gIconYAdjust;
extern u8* gIconDataBase;
extern i32 gIconWidth;
extern i32 gIconHeight;
}

namespace IconBlit {

static_assert(sizeof(IconEntry) == 12, "IconEntry must match the 12-byte file layout");

// Sign of a 32-bit register result (the asm's `js`/`jns`).
inline bool IsNegative(u32 value) {
    return (value & 0x80000000u) != 0;
}

// 32-bit row * stride + column (the asm's `mul` + `add`), used as a signed
// offset. Offsets stay integers until a byte is accessed.
inline ptrdiff_t Offset32(u32 row, u32 stride, u32 column) {
    return static_cast<i32>(row * stride + column);
}

struct IconFrame {
    i32 xAdjust;
    i32 yAdjust;
    u32 width;
    u32 height;
    const u8* commands;
};

// Decodes frame `frame`'s header and publishes it in the gIcon* globals. With
// a non-zero offsetMode the horizontal offset is reduced by the asm's shift
// sequence: half = x >> 1; x = half - ((x - half) >> 1) (arithmetic shifts).
inline IconFrame LoadIconFrame(icon* ic, i32 frame, i32 offsetMode) {
    u8* base = ic->m_data;
    gIconDataBase = base;
    IconEntry entry;
    memcpy(&entry, base + Offset32(static_cast<u32>(frame), sizeof(IconEntry), 0), sizeof(entry));

    i32 xAdjust = entry.x;
    if (offsetMode != 0) {
        i32 half = xAdjust >> 1;
        i32 quarter = (xAdjust - half) >> 1;
        xAdjust = half - quarter;
    }

    IconFrame result;
    result.xAdjust = xAdjust;
    result.yAdjust = entry.y;
    result.width = static_cast<u16>(entry.w);
    result.height = static_cast<u16>(entry.h);
    result.commands = base + entry.srcOffset;
    gIconXAdjust = result.xAdjust;
    gIconYAdjust = result.yAdjust;
    gIconWidth = static_cast<i32>(result.width);
    gIconHeight = static_cast<i32>(result.height);
    return result;
}

} // namespace IconBlit

#endif
