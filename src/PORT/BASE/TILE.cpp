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

inline bool IsNegative(u32 value) {
    return (value & 0x80000000u) != 0;
}

// 32-bit row * stride + column (the asm's `mul` + `add`), used as a signed
// offset. Source and destination positions stay integer offsets because the
// asm walks them outside the buffers between rows (e.g. one row before the
// tile after the last vertically flipped row).
inline ptrdiff_t Offset32(u32 row, u32 stride, u32 column) {
    return static_cast<i32>(row * stride + column);
}

} // namespace

// Draws tile (tile & TILE_INDEX_MASK) of `tiles` with its top-left corner at
// (x, y) of `dest`, optionally mirrored. No clipping.
//
// Retained asm behaviour:
//  - The index check is index <= m_tileCount, so index == m_tileCount draws
//    the bytes just past the last tile.
//  - Unflipped and vertically flipped rows copy whole DWORDs only,
//    (m_tileWidth & ~3) bytes, and both positions advance by the copied
//    amount (the destination plus stride - m_tileWidth), so widths that are
//    not a multiple of 4 drift.
//  - The vertical and combined flips assume square tiles: they draw
//    m_tileWidth rows and take the tile's last row/byte from width * width.
//    The horizontal-only flip draws m_tileHeight rows of (m_tileWidth & ~7)
//    pixels.
// The asm's private _gTileScratch/_gTileRows words are locals here.
extern "C" void TileToBitmap(tileset* tiles, u32 tile, bitmap* dest, i32 x, i32 y) {
    u32 destStride = static_cast<u16>(dest->m_width);
    u8* pixels = dest->m_pixels;
    ptrdiff_t to = Offset32(static_cast<u32>(y), destStride, static_cast<u32>(x));

    u32 index = tile & TILE_INDEX_MASK;
    if (IsNegative(static_cast<u32>(tiles->m_tileCount) - index))
        return;

    u32 width = tiles->m_tileWidth;
    u32 height = tiles->m_tileHeight;
    u32 rowSkip = destStride - width;
    const u8* data = tiles->m_data;
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
            memcpy(pixels + to, data + from, copied);
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
                pixels[to--] = data[from++];
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
            pixels[to++] = data[from--];
        to += static_cast<i32>(rowSkip);
    } while (--rows != 0);
}
