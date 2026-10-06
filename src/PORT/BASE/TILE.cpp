// Portable equivalent of the TILE.asm TileToBitmap routine, built by the native port.

#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/tileset.h>
#include <BASE/TILE.h>

#include <stddef.h>
#include <string.h>

namespace {

// Tile argument layout (EQUs in TILE.asm).
enum TileFlag : u32 {
    TILE_INDEX_MASK = 0x0FFF,
    TILE_FLIP_VERTICAL = 0x4000,
    TILE_FLIP_HORIZONTAL = 0x8000
};

// 32-bit row * stride + column (the asm's `mul` + `add`), used as a signed
// offset. Source and destination positions stay integer offsets because the
// asm walks them outside the buffers between rows (e.g. one row before the
// tile after the last vertically flipped row).
inline ptrdiff_t Offset32(u32 row, u32 stride, u32 column) {
    return static_cast<i32>(row * stride + column);
}

// The tileset's data (count * width * height bytes) and the destination's
// pixel block: reads outside the data are 0 and writes outside the block are
// dropped, so a tileset whose tiles are not square, or not a multiple of 8
// wide, or a tile drawn at the edge, cannot take the routine outside them.
// Square tiles drawn inside the destination never reach either bound.
struct Tiles {
    const u8* data;
    ptrdiff_t size;
    u8 At(ptrdiff_t at) const { return at >= 0 && at < size ? data[at] : 0; }
};

struct Destination {
    u8* pixels;
    ptrdiff_t size;
    void Put(ptrdiff_t at, u8 value) const {
        if (at >= 0 && at < size)
            pixels[at] = value;
    }
    void Copy(ptrdiff_t to, const Tiles& source, ptrdiff_t from, u32 count) const {
        ptrdiff_t n = static_cast<ptrdiff_t>(count);
        if (to >= 0 && from >= 0 && n <= size - to && n <= source.size - from) {
            memcpy(pixels + to, source.data + from, count);
            return;
        }
        for (ptrdiff_t i = 0; i < n; ++i)
            Put(to + i, source.At(from + i));
    }
};

} // namespace

// Draws tile (tile & TILE_INDEX_MASK) of `tiles` with its top-left corner at
// (x, y) of `dest`, optionally mirrored. No clipping.
//
// Retained asm behaviour:
//  - Unflipped and vertically flipped rows copy whole DWORDs only,
//    (m_tileWidth & ~3) bytes, and both positions advance by the copied
//    amount (the destination plus stride - m_tileWidth), so widths that are
//    not a multiple of 4 drift.
//  - The vertical and combined flips assume square tiles: they draw
//    m_tileWidth rows and take the tile's last row/byte from width * width.
//    The horizontal-only flip draws m_tileHeight rows of (m_tileWidth & ~7)
//    pixels.
// The asm's private _gTileScratch/_gTileRows words are locals here. The asm
// checked index <= m_tileCount and so drew the bytes just past the last tile
// for index == m_tileCount; that index draws nothing here.
extern "C" void TileToBitmap(tileset* tiles, u32 tile, bitmap* dest, i32 x, i32 y) {
    u32 destStride = static_cast<u16>(dest->m_width);
    Destination pixels;
    pixels.pixels = dest->m_pixels;
    pixels.size = static_cast<ptrdiff_t>(destStride) * static_cast<u16>(dest->m_height);
    ptrdiff_t to = Offset32(static_cast<u32>(y), destStride, static_cast<u32>(x));

    u32 index = tile & TILE_INDEX_MASK;
    if (index >= tiles->m_tileCount)
        return;

    u32 width = tiles->m_tileWidth;
    u32 height = tiles->m_tileHeight;
    u32 rowSkip = destStride - width;
    Tiles data;
    data.data = tiles->m_data;
    data.size = static_cast<ptrdiff_t>(tiles->m_tileCount) * width * height;
    ptrdiff_t from = static_cast<i32>(width * height * index);

    // Row and pixel-group counters are `dec; jne` / `loop` in the asm, so a
    // zero count would run 65536 rows or 2^32 groups and crash; those cases
    // draw nothing here.
    if (!(tile & TILE_FLIP_HORIZONTAL)) {
        bool vertical = (tile & TILE_FLIP_VERTICAL) != 0;
        u32 copied = (width >> 2) * 4;
        u16 rows = static_cast<u16>(vertical ? width : height);
        if (rows == 0)
            return;
        ptrdiff_t sourceStep = static_cast<i32>(copied);
        if (vertical) {
            // Start at the last row and walk up.
            from += static_cast<i32>((width - 1) * width);
            sourceStep = static_cast<i32>(copied - 2 * width);
        }
        do {
            pixels.Copy(to, data, from, copied);
            to += static_cast<i32>(copied + rowSkip);
            from += sourceStep;
        } while (--rows != 0);
        return;
    }

    u32 groups = width >> 3;
    if (groups == 0)
        return;

    if (!(tile & TILE_FLIP_VERTICAL)) {
        // Horizontal flip: each row is written right to left from its last
        // column, 8 pixels per group.
        u16 rows = static_cast<u16>(height);
        if (rows == 0)
            return;
        to += static_cast<i32>(width - 1);
        ptrdiff_t nextRow = static_cast<i32>(rowSkip + 2 * width);
        do {
            for (u32 i = 0; i < groups * 8; ++i)
                pixels.Put(to--, data.At(from++));
            to += nextRow;
        } while (--rows != 0);
        return;
    }

    // Both flips: read the tile backwards from its last byte, writing rows
    // left to right.
    u16 rows = static_cast<u16>(width);
    from += static_cast<i32>(width * width - 1);
    do {
        for (u32 i = 0; i < groups * 8; ++i)
            pixels.Put(to++, data.At(from--));
        to += static_cast<i32>(rowSkip);
    } while (--rows != 0);
}
