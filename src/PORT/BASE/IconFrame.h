// Shared frame-header prologue of the portable Icon2b/Icon2bc blitters, built by the native port.
#ifndef HOMM1_PORT_BASE_ICONFRAME_H
#define HOMM1_PORT_BASE_ICONFRAME_H

#include <H1/Ints.h>

#include <BASE/bitmap.h>
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

// What a blitter reads: the icon's data, m_dataSize bytes. Positions are
// offsets into it. The asm trusted the command streams; here a command read
// outside the data is the end of the frame, a pixel byte outside it is 0 and
// a scan for the end of a row stops at the end of the data, so a malformed
// icon cannot make a blitter read outside its data.
struct IconSource {
    const u8* data;
    ptrdiff_t size;

    bool Inside(ptrdiff_t at) const { return at >= 0 && at < size; }
    u8 Command(ptrdiff_t at) const { return Inside(at) ? data[at] : 0x80; }
    u8 Pixel(ptrdiff_t at) const { return Inside(at) ? data[at] : 0; }
    // The position after the next 0 byte at or after `at`, or the end of the
    // data when there is none.
    ptrdiff_t AfterRowEnd(ptrdiff_t at) const {
        if (at < 0)
            return size;
        while (at < size) {
            if (data[at++] == 0)
                return at;
        }
        return size;
    }
};

// What a blitter writes: the destination's pixel block, width * height bytes.
// Pixels outside it are not written (the asm wrote wherever a malformed
// frame led). Frames that fit where the routines accept them stay inside, so
// their pictures are the asm's.
struct IconTarget {
    u8* pixels;
    ptrdiff_t size;

    bool Inside(ptrdiff_t at) const { return at >= 0 && at < size; }
    void Put(ptrdiff_t at, u8 value) const {
        if (Inside(at))
            pixels[at] = value;
    }
    u8 Get(ptrdiff_t at) const { return Inside(at) ? pixels[at] : 0; }
    // count pixel bytes from the source, left to right.
    void Copy(ptrdiff_t to, const IconSource& source, ptrdiff_t from, u32 count) const {
        ptrdiff_t n = static_cast<ptrdiff_t>(count);
        if (to >= 0 && from >= 0 && n <= size - to && n <= source.size - from) {
            memcpy(pixels + to, source.data + from, count);
            return;
        }
        for (ptrdiff_t i = 0; i < n; ++i)
            Put(to + i, source.Pixel(from + i));
    }
    void Fill(ptrdiff_t to, u8 value, u32 count) const {
        ptrdiff_t n = static_cast<ptrdiff_t>(count);
        if (to >= 0 && n <= size - to) {
            memset(pixels + to, value, count);
            return;
        }
        for (ptrdiff_t i = 0; i < n; ++i)
            Put(to + i, value);
    }
};

inline IconTarget TargetOf(bitmap* bmp) {
    IconTarget target;
    target.pixels = bmp->m_pixels;
    target.size = static_cast<ptrdiff_t>(static_cast<u16>(bmp->m_width))
                  * static_cast<ptrdiff_t>(static_cast<u16>(bmp->m_height));
    return target;
}

struct IconFrame {
    i32 xAdjust;
    i32 yAdjust;
    u32 width;
    u32 height;
    IconSource source;
    ptrdiff_t commands;  // offset of the frame's command stream
};

// Decodes frame `frame`'s header and publishes it in the gIcon* globals. With
// a non-zero offsetMode the horizontal offset is reduced by the asm's shift
// sequence: half = x >> 1; x = half - ((x - half) >> 1) (arithmetic shifts).
// A frame number outside the icon's frame table draws nothing (the asm read
// a header from whatever followed the table).
inline IconFrame LoadIconFrame(icon* ic, i32 frame, i32 offsetMode) {
    u8* base = ic->m_data;
    gIconDataBase = base;
    IconEntry entry;
    memset(&entry, 0, sizeof(entry));
    IconFrame result;
    result.source.data = base;
    result.source.size = static_cast<ptrdiff_t>(ic->m_dataSize);
    bool known = frame >= 0 && frame < ic->m_frameCount
                 && static_cast<ptrdiff_t>(frame + 1) * static_cast<ptrdiff_t>(sizeof(IconEntry))
                        <= result.source.size;
    if (known)
        memcpy(&entry, base + Offset32(static_cast<u32>(frame), sizeof(IconEntry), 0),
               sizeof(entry));

    i32 xAdjust = entry.x;
    if (offsetMode != 0) {
        i32 half = xAdjust >> 1;
        i32 quarter = (xAdjust - half) >> 1;
        xAdjust = half - quarter;
    }

    result.xAdjust = xAdjust;
    result.yAdjust = entry.y;
    result.width = static_cast<u16>(entry.w);
    result.height = static_cast<u16>(entry.h);
    result.commands = known ? static_cast<ptrdiff_t>(entry.srcOffset) : result.source.size;
    gIconXAdjust = result.xAdjust;
    gIconYAdjust = result.yAdjust;
    gIconWidth = static_cast<i32>(result.width);
    gIconHeight = static_cast<i32>(result.height);
    return result;
}

} // namespace IconBlit

#endif
