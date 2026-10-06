// Portable equivalent of the BMAP2.asm bitmap block routines, built by the native port.

#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/BMAP2.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Public in BMAP2.asm; no header declares them.
extern "C" u8 gDimPalette[256];
void MoveBitmapArea(bitmap* bmp, i32 sourceX, i32 sourceY, i32 width, i32 height, i32 destX, i32 destY);

// Darkening lookup table: gDimPalette[colour] is the dimmed palette index.
// Shared with Icon2b's dim routines.
extern "C" {
u8 gDimPalette[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
    0x35, 0x35, 0x35, 0x35, 0x35, 0x35, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x53, 0x54,
    0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x65, 0x65, 0x65, 0x65, 0x65, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7D, 0x7D, 0x7D, 0x7D, 0x7D, 0x83, 0x84,
    0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
    0x95, 0x95, 0x95, 0x95, 0x95, 0x95, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4,
    0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4,
    0xC5, 0xC5, 0xC5, 0xC5, 0xC5, 0xC5, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD5, 0xD5, 0xD5, 0xD5, 0xD5, 0xBC, 0xBF, 0xC0, 0xC3, 0x71, 0x75, 0x79, 0x7D, 0xE3, 0xE4,
    0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x3F, 0x42, 0x3D, 0x45, 0x49,
    0x4D, 0xF4, 0xF4, 0xF4, 0xF4, 0xF5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
}

// The asm's private scratch words _gBitmapSourceSkip, _gBitmapRowSkip and
// _gBitmapUnused are not PUBLIC; they are locals here.
//
// Address arithmetic follows the asm's 32-bit registers: row * stride + column
// is formed modulo 2^32 and then used as a signed pixel offset. Offsets stay
// integers until a byte is accessed so that intermediate positions outside the
// buffer (e.g. one row past the end after the last row) are well defined.

namespace {

inline bool IsNegative(u32 value) {
    return (value & 0x80000000u) != 0;
}

inline ptrdiff_t PixelOffset(u32 row, u32 stride, u32 column) {
    return static_cast<i32>(row * stride + column);
}

// `rep movsd` (count / 4) then `rep movsb` (count % 4) with the direction flag
// clear. Each move reads its source before writing, so overlapping ranges see
// the asm's DWORD granularity; disjoint ranges are a plain memcpy.
void CopyForward(u8* destination, const u8* source, u32 count) {
    uintptr_t to = reinterpret_cast<uintptr_t>(destination);
    uintptr_t from = reinterpret_cast<uintptr_t>(source);
    if (to + count <= from || from + count <= to) {
        memcpy(destination, source, count);
        return;
    }
    for (u32 dwords = count >> 2; dwords != 0; --dwords) {
        u8 moved[4];
        memcpy(moved, source, 4);
        memcpy(destination, moved, 4);
        source += 4;
        destination += 4;
    }
    for (u32 bytes = count & 3; bytes != 0; --bytes)
        *destination++ = *source++;
}

} // namespace

// Copies a width x height block from source (sourceX, sourceY) to destination
// (dx, dy). Nothing is drawn when the block is wider than either bitmap.
void BlitBitmap(
    bitmap* source,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height,
    bitmap* destination,
    i32 dx,
    i32 dy
) {
    u32 count = static_cast<u32>(width);
    u32 sourceStride = static_cast<u16>(source->m_width);
    if (IsNegative(sourceStride - count))
        return;
    u32 destinationStride = static_cast<u16>(destination->m_width);
    if (IsNegative(destinationStride - count))
        return;
    // The asm counts rows with `dec; jne` and copies `width` bytes with rep
    // movs: a height <= 0 runs at least 2^31 rows and a negative width copies
    // gigabytes per row. Both crash there and are no-ops here.
    if (height <= 0 || width < 0)
        return;

    ptrdiff_t from = PixelOffset(static_cast<u32>(sourceY), sourceStride, static_cast<u32>(sourceX));
    ptrdiff_t to = PixelOffset(static_cast<u32>(dy), destinationStride, static_cast<u32>(dx));
    u32 rows = static_cast<u32>(height);
    do {
        CopyForward(destination->m_pixels + to, source->m_pixels + from, count);
        from += sourceStride;
        to += destinationStride;
    } while (--rows != 0);
}

// Scrolls a block inside one bitmap from (sourceX, sourceY) to (destX, destY).
// Not referenced by the game; kept for symbol parity with BMAP2.asm.
//
// Retained asm quirks:
//  - When the rows are equal the asm compares sourceX with destY (not destX)
//    to pick the direction, returns if they are equal, and then uses sourceX
//    as the source *row*.
//  - Different rows copy DWORDs first. Going backwards each row starts at
//    its last byte and every DWORD move copies the addressed byte and the
//    three after it, so with r = width & 3 a row copies block columns [0, r)
//    and [r + 3, width + 3): the three columns past the right edge are
//    copied and columns [r, r + 3) are not (once width >= 4). Same-row moves
//    copy byte by byte.
//  - Overlap behaves like the asm's sequential moves, not like memmove.
// Bytes of those DWORD moves that fall outside the m_width * m_height pixel
// buffer are skipped here; the asm reads/writes them out of bounds.
void MoveBitmapArea(bitmap* bmp, i32 sourceX, i32 sourceY, i32 width, i32 height, i32 destX, i32 destY) {
    u32 stride = static_cast<u16>(bmp->m_width);
    u32 count = static_cast<u32>(width);
    u32 rowSkip = stride - count;
    if (IsNegative(rowSkip))
        return;

    bool backward;
    i32 sourceRow = sourceY;
    if (sourceY < destY) {
        backward = true;
    } else if (sourceY != destY) {
        backward = false;
    } else {
        sourceRow = sourceX;
        if (sourceX < destY)
            backward = true;
        else if (sourceX == destY)
            return;
        else
            backward = false;
    }
    bool sameRow = sourceY == destY;

    // A height <= 0 loops >= 2^31 rows in the asm; a negative width copies
    // gigabytes. Both crash there and are no-ops here.
    if (height <= 0 || width < 0)
        return;

    u8* pixels = bmp->m_pixels;
    u32 rows = static_cast<u32>(height);

    if (!backward) {
        ptrdiff_t from = PixelOffset(static_cast<u32>(sourceRow), stride, static_cast<u32>(sourceX));
        ptrdiff_t to = PixelOffset(static_cast<u32>(destY), stride, static_cast<u32>(destX));
        do {
            if (sameRow) {
                for (u32 i = 0; i < count; ++i)
                    pixels[to + i] = pixels[from + i];
            } else {
                CopyForward(pixels + to, pixels + from, count);
            }
            from += stride;
            to += stride;
        } while (--rows != 0);
        return;
    }

    // Backward: start at the last byte of the bottom row and walk up.
    u32 lastRowOffset = static_cast<u32>(height) - 1;
    u32 lastColumnOffset = count - 1;
    ptrdiff_t from = PixelOffset(
        static_cast<u32>(sourceRow) + lastRowOffset,
        stride,
        static_cast<u32>(sourceX) + lastColumnOffset
    );
    ptrdiff_t to = PixelOffset(
        static_cast<u32>(destY) + lastRowOffset,
        stride,
        static_cast<u32>(destX) + lastColumnOffset
    );
    ptrdiff_t limit = static_cast<ptrdiff_t>(stride) * static_cast<u16>(bmp->m_height);
    do {
        ptrdiff_t s = from;
        ptrdiff_t d = to;
        if (sameRow) {
            for (u32 i = 0; i < count; ++i)
                pixels[d--] = pixels[s--];
        } else {
            // rep movsd with DF set: each move copies [s, s + 3] to [d, d + 3].
            for (u32 dwords = count >> 2; dwords != 0; --dwords) {
                u8 moved[4];
                bool present[4];
                for (int b = 0; b < 4; ++b) {
                    present[b] = s + b >= 0 && s + b < limit;
                    moved[b] = present[b] ? pixels[s + b] : 0;
                }
                for (int b = 0; b < 4; ++b) {
                    if (present[b] && d + b >= 0 && d + b < limit)
                        pixels[d + b] = moved[b];
                }
                s -= 4;
                d -= 4;
            }
            for (u32 bytes = count & 3; bytes != 0; --bytes)
                pixels[d--] = pixels[s--];
        }
        from -= stride;
        to -= stride;
    } while (--rows != 0);
}

// Replaces every pixel of the block with its gDimPalette entry. Nothing is
// drawn when the block is wider than the bitmap (unsigned compare).
void DimBitmapArea(bitmap* bmp, i32 x, i32 y, i32 w, i32 h) {
    u32 stride = static_cast<u16>(bmp->m_width);
    u32 count = static_cast<u32>(w);
    if (stride < count)
        return;
    // The asm's pixel `loop` runs 2^32 times for w == 0 and its row
    // `dec; jne` at least 2^31 times for h <= 0; no-ops here.
    if (count == 0 || h <= 0)
        return;

    u8* pixels = bmp->m_pixels;
    ptrdiff_t row = PixelOffset(static_cast<u32>(y), stride, static_cast<u32>(x));
    u32 rows = static_cast<u32>(h);
    do {
        u8* pixel = pixels + row;
        for (u32 i = 0; i < count; ++i)
            pixel[i] = gDimPalette[pixel[i]];
        row += stride;
    } while (--rows != 0);
}

// Fills the block with the low byte of `color`. Nothing is drawn when the
// block is wider than the bitmap (unsigned compare).
void FillBitmapArea(bitmap* image, i32 x, i32 y, i32 width, i32 height, i32 color) {
    u32 stride = static_cast<u16>(image->m_width);
    u32 count = static_cast<u32>(width);
    if (stride < count)
        return;
    // The row `dec; jne` runs at least 2^31 times for height <= 0.
    if (height <= 0)
        return;

    u8* pixels = image->m_pixels;
    ptrdiff_t row = PixelOffset(static_cast<u32>(y), stride, static_cast<u32>(x));
    u32 rows = static_cast<u32>(height);
    do {
        memset(pixels + row, static_cast<u8>(color), count);
        row += stride;
    } while (--rows != 0);
}
