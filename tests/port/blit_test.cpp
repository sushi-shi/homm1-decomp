// Known-answer and sanitizer stress test for the portable BITS/BMAP2/Icon2b/Icon2bc/TILE routines.
//
// Build and run from the repository root (one command):
//   g++ -std=c++20 -Wall -Wextra -fsigned-char -D__cdecl= -g -fsanitize=address,undefined
//       -fno-sanitize-recover=all -Iinclude tests/port/blit_test.cpp
//       src/PORT/BASE/BITS.cpp src/PORT/BASE/BMAP2.cpp src/PORT/BASE/Icon2b.cpp
//       src/PORT/BASE/Icon2bc.cpp src/PORT/BASE/TILE.cpp -o blit_test && ./blit_test
//
// Pixel buffers are allocated at their exact size so that the sanitizers flag
// any out-of-bounds access on valid inputs.

#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/tileset.h>
#include <BASE/BITS.h>
#include <BASE/bmap2.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>
#include <BASE/TILE.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <vector>

// The routines under test only touch data members; the class constructors
// live in game units that this test does not link.
resource::resource() {}
resource::~resource() {}
bitmap::bitmap(void) {}
bitmap::~bitmap() {}
icon::icon(i16) {}
icon::~icon() {}
tileset::tileset(i16) {}
tileset::~tileset() {}

// Defined by the port units without a header declaration.
extern "C" void BitClear(void* bits, u32 bit);
extern "C" u8 gDimPalette[256];
extern "C" i32 gClipColumn;
void MoveBitmapArea(bitmap* bmp, i32 sourceX, i32 sourceY, i32 width, i32 height, i32 destX, i32 destY);

namespace {

int gFailures = 0;

void Expect(bool condition, const char* what, int line) {
    if (!condition) {
        ++gFailures;
        printf("FAIL line %d: %s\n", line, what);
    }
}
#define EXPECT(cond) Expect((cond), #cond, __LINE__)

u32 gSeed = 0xC0FFEE11;
u32 Rand() {
    gSeed ^= gSeed << 13;
    gSeed ^= gSeed >> 17;
    gSeed ^= gSeed << 5;
    return gSeed;
}
i32 Range(i32 lo, i32 hi) {
    return lo + static_cast<i32>(Rand() % static_cast<u32>(hi - lo + 1));
}

// A bitmap whose pixel buffer is exactly width * height bytes.
struct Canvas {
    bitmap bmp;
    std::unique_ptr<u8[]> pixels;
    Canvas(i32 width, i32 height, u8 fill = 0) : pixels(new u8[static_cast<size_t>(width) * height]) {
        bmp.m_width = static_cast<i16>(width);
        bmp.m_height = static_cast<i16>(height);
        bmp.m_pixels = pixels.get();
        memset(pixels.get(), fill, static_cast<size_t>(width) * height);
    }
    u8& At(i32 x, i32 y) { return pixels[static_cast<size_t>(y) * bmp.m_width + x]; }
    size_t Size() const { return static_cast<size_t>(bmp.m_width) * bmp.m_height; }
};

// Icon resource assembled from frames; m_data is an exact-size heap block.
struct IconBuilder {
    struct Frame {
        IconEntry entry;
        std::vector<u8> stream;
    };
    std::vector<Frame> frames;
    std::unique_ptr<u8[]> data;
    icon ic{0};

    void Add(i16 x, i16 y, i16 w, i16 h, std::vector<u8> stream) {
        Frame f;
        f.entry.x = x;
        f.entry.y = y;
        f.entry.w = w;
        f.entry.h = h;
        f.entry.srcOffset = 0;
        f.stream = std::move(stream);
        frames.push_back(std::move(f));
    }
    icon* Build() {
        size_t size = frames.size() * sizeof(IconEntry);
        for (auto& f : frames)
            size += f.stream.size();
        data.reset(new u8[size]);
        size_t offset = frames.size() * sizeof(IconEntry);
        for (size_t i = 0; i < frames.size(); ++i) {
            frames[i].entry.srcOffset = static_cast<u32>(offset);
            memcpy(data.get() + i * sizeof(IconEntry), &frames[i].entry, sizeof(IconEntry));
            memcpy(data.get() + offset, frames[i].stream.data(), frames[i].stream.size());
            offset += frames[i].stream.size();
        }
        ic.m_frameCount = static_cast<i16>(frames.size());
        ic.m_data = data.get();
        return &ic;
    }
};

// Random well-formed frame stream: every row ends with 0x00, the frame ends
// with 0x80, and no row covers more than `w` columns. Pixel bytes avoid 0 so
// the clipped routines' row-end scan stays on row boundaries.
std::vector<u8> RandomStream(i32 w, i32 h, bool withPixels) {
    std::vector<u8> s;
    for (i32 row = 0; row < h; ++row) {
        i32 col = 0;
        while (col < w && Rand() % 8 != 0) {
            i32 left = w - col;
            i32 n = Range(1, left < 127 ? left : 127);
            if (Rand() % 3 == 0) {
                s.push_back(static_cast<u8>(0x80 | n));
            } else {
                s.push_back(static_cast<u8>(n));
                if (withPixels)
                    for (i32 i = 0; i < n; ++i)
                        s.push_back(static_cast<u8>(Range(1, 255)));
            }
            col += n;
        }
        s.push_back(0x00);
    }
    s.push_back(0x80);
    return s;
}

void TestBits() {
    u8 bits[4] = {0, 0, 0, 0};
    BitSet(bits, 0);
    BitSet(bits, 9);
    BitSet(bits, 31);
    EXPECT(bits[0] == 0x01 && bits[1] == 0x02 && bits[2] == 0x00 && bits[3] == 0x80);
    EXPECT(BitTest(bits, 9) == 1);
    EXPECT(BitTest(bits, 8) == 0);
    BitClear(bits, 9);
    EXPECT(bits[1] == 0x00);
    EXPECT(BitTest(bits, 31) == 1);

    // The last bit of a one-byte heap block: no access past the block.
    std::unique_ptr<u8[]> one(new u8[1]);
    one[0] = 0x80;
    EXPECT(BitTest(one.get(), 7) == 1);
    BitSet(one.get(), 0);
    BitClear(one.get(), 7);
    EXPECT(one[0] == 0x01);
}

void TestBmap2() {
    Canvas c(8, 4);
    FillBitmapArea(&c.bmp, 2, 1, 3, 2, 0x1234);
    for (i32 y = 0; y < 4; ++y)
        for (i32 x = 0; x < 8; ++x)
            EXPECT(c.At(x, y) == ((y >= 1 && y <= 2 && x >= 2 && x <= 4) ? 0x34 : 0));
    FillBitmapArea(&c.bmp, 0, 0, 9, 1, 0x55); // wider than the bitmap: ignored
    EXPECT(c.At(0, 0) == 0);
    FillBitmapArea(&c.bmp, 0, 3, 8, 1, 0x77); // whole last row
    EXPECT(c.At(7, 3) == 0x77);

    Canvas d(8, 4, 0x20);
    d.At(3, 2) = 0x0B;
    DimBitmapArea(&d.bmp, 3, 2, 5, 2);
    EXPECT(d.At(3, 2) == gDimPalette[0x0B] && gDimPalette[0x0B] == 0x10);
    EXPECT(d.At(4, 2) == 0x25 && d.At(7, 3) == 0x25);
    EXPECT(d.At(2, 2) == 0x20 && d.At(3, 1) == 0x20);

    Canvas src(6, 5), dst(10, 7);
    for (size_t i = 0; i < src.Size(); ++i)
        src.pixels[i] = static_cast<u8>(i + 1);
    BlitBitmap(&src.bmp, 1, 2, 5, 3, &dst.bmp, 5, 4);
    for (i32 y = 0; y < 3; ++y)
        for (i32 x = 0; x < 5; ++x)
            EXPECT(dst.At(5 + x, 4 + y) == src.At(1 + x, 2 + y));
    EXPECT(dst.At(4, 4) == 0 && dst.At(5, 3) == 0);

    // MoveBitmapArea moving down (backward copy) with the block in the
    // bottom-right corner. Each row starts at its last byte and the asm's
    // DWORD moves copy the byte addressed plus the three after it, so with
    // r = width & 3 the row copies columns [0, r) and [r + 3, width + 3) of
    // the block: three bytes past its right edge (outside the buffer on the
    // last row, skipped by the port) and not the three after the first r.
    for (i32 width = 1; width <= 9; ++width) {
        Canvas m(9, 6);
        for (size_t i = 0; i < m.Size(); ++i)
            m.pixels[i] = static_cast<u8>(Rand());
        std::vector<u8> before(m.pixels.get(), m.pixels.get() + m.Size());
        std::vector<u8> expect = before;
        i32 sx = 9 - width, sy = 2, h = 3, dy = 3;
        MoveBitmapArea(&m.bmp, sx, sy, width, h, sx, dy);
        i32 size = static_cast<i32>(m.Size());
        auto move = [&](i32 from, i32 to) {
            if (from < size && to < size)
                expect[static_cast<size_t>(to)] = expect[static_cast<size_t>(from)];
        };
        for (i32 row = h - 1; row >= 0; --row) {
            i32 from = (sy + row) * 9 + sx + width - 1, to = (dy + row) * 9 + sx + width - 1;
            for (i32 k = 0; k < width / 4; ++k, from -= 4, to -= 4)
                for (i32 b = 0; b < 4; ++b)
                    move(from + b, to + b);
            for (i32 k = 0; k < width % 4; ++k)
                move(from--, to--);
        }
        EXPECT(memcmp(expect.data(), m.pixels.get(), m.Size()) == 0);
        if (width >= 4) {
            // Column sx + (width & 3) of the top moved row is left untouched
            // (lower rows can receive the previous row's wrapped extra bytes).
            i32 skipped = sx + width % 4;
            EXPECT(m.At(skipped, dy) == before[static_cast<size_t>(dy * 9 + skipped)]);
        }
    }

    // Moving up (forward copy) shifts rows exactly.
    Canvas up(7, 5);
    for (size_t i = 0; i < up.Size(); ++i)
        up.pixels[i] = static_cast<u8>(i);
    MoveBitmapArea(&up.bmp, 1, 2, 5, 3, 2, 0);
    for (i32 y = 0; y < 3; ++y)
        for (i32 x = 0; x < 5; ++x)
            EXPECT(up.At(2 + x, y) == static_cast<u8>((2 + y) * 7 + 1 + x));
}

void TestIconsKnown() {
    IconBuilder b;
    // Frame 0: 4x2 with pixels. Row 0: 4 pixels; row 1: skip 1, 2 pixels.
    b.Add(0, 0, 4, 2, {0x04, 1, 2, 3, 4, 0x00, 0x81, 0x02, 5, 6, 0x00, 0x80});
    // Frame 1: the same shape for mono/dim (no pixel bytes).
    b.Add(0, 0, 4, 2, {0x04, 0x00, 0x81, 0x02, 0x00, 0x80});
    icon* ic = b.Build();

    {
        Canvas c(8, 4);
        IconToBitmap(ic, &c.bmp, 1, 1, 0, 0);
        EXPECT(c.At(1, 1) == 1 && c.At(2, 1) == 2 && c.At(3, 1) == 3 && c.At(4, 1) == 4);
        EXPECT(c.At(2, 2) == 5 && c.At(3, 2) == 6 && c.At(1, 2) == 0 && c.At(4, 2) == 0);
    }
    {
        Canvas c(8, 4);
        FlipIconToBitmap(ic, &c.bmp, 5, 1, 0, 0);
        EXPECT(c.At(5, 1) == 1 && c.At(4, 1) == 2 && c.At(3, 1) == 3 && c.At(2, 1) == 4);
        EXPECT(c.At(4, 2) == 5 && c.At(3, 2) == 6 && c.At(5, 2) == 0);
        Canvas r(8, 4);
        FlipIconToBitmap(ic, &r.bmp, 2, 1, 0, 0); // x - width + 1 < 0: rejected
        EXPECT(r.At(2, 1) == 0);
    }
    {
        Canvas c(8, 4);
        MonoIconToBitmap(ic, &c.bmp, 0, 0, 1, 0x1F7, 0);
        EXPECT(c.At(0, 0) == 0xF7 && c.At(3, 0) == 0xF7 && c.At(0, 1) == 0 && c.At(1, 1) == 0xF7);
        EXPECT(c.At(2, 1) == 0xF7 && c.At(3, 1) == 0);
        Canvas f(8, 4);
        FlipMonoIconToBitmap(ic, &f.bmp, 7, 0, 1, 9, 0);
        EXPECT(f.At(7, 0) == 9 && f.At(4, 0) == 9 && f.At(3, 0) == 0 && f.At(6, 1) == 9 && f.At(5, 1) == 9);
    }
    {
        // Dim rows advance by 640 bytes, so use a 640-wide bitmap.
        Canvas c(640, 3, 0x30);
        DimIconToBitmap(ic, &c.bmp, 10, 1, 1, 0);
        EXPECT(c.At(10, 1) == gDimPalette[0x30] && c.At(13, 1) == gDimPalette[0x30]);
        EXPECT(c.At(10, 2) == 0x30 && c.At(11, 2) == gDimPalette[0x30] && c.At(13, 2) == 0x30);
        Canvas f(640, 3, 0x30);
        FlipDimIconToBitmap(ic, &f.bmp, 13, 0, 1, 0);
        EXPECT(f.At(13, 0) == gDimPalette[0x30] && f.At(10, 0) == gDimPalette[0x30] && f.At(9, 0) == 0x30);
        EXPECT(f.At(12, 1) == gDimPalette[0x30] && f.At(11, 1) == gDimPalette[0x30] && f.At(13, 1) == 0x30);
    }
    {
        gClipColumn = 0;
        Canvas c(8, 4);
        ClippedIconToBitmap(ic, &c.bmp, -2, -1, 0, 0); // only icon (3, 1) is visible
        EXPECT(c.At(0, 0) == 6);
        for (size_t i = 1; i < c.Size(); ++i)
            EXPECT(c.pixels[i] == 0);

        Canvas r(8, 4);
        ClippedIconToBitmap(ic, &r.bmp, 6, 0, 0, 0); // cut at the right edge
        EXPECT(r.At(6, 0) == 1 && r.At(7, 0) == 2 && r.At(7, 1) == 5 && r.At(6, 1) == 0);

        Canvas f(8, 4);
        FlipClippedIconToBitmap(ic, &f.bmp, 9, 0, 0, 0); // icon columns 2, 3 visible
        EXPECT(f.At(7, 0) == 3 && f.At(6, 0) == 4 && f.At(7, 1) == 6 && f.At(5, 0) == 0);

        // Fully visible: clipped and unclipped draw the same.
        Canvas u(8, 4), v(8, 4);
        IconToBitmap(ic, &u.bmp, 2, 1, 0, 0);
        ClippedIconToBitmap(ic, &v.bmp, 2, 1, 0, 0);
        EXPECT(memcmp(u.pixels.get(), v.pixels.get(), u.Size()) == 0);
        Canvas uf(8, 4), vf(8, 4);
        FlipIconToBitmap(ic, &uf.bmp, 6, 1, 0, 0);
        FlipClippedIconToBitmap(ic, &vf.bmp, 6, 1, 0, 0);
        EXPECT(memcmp(uf.pixels.get(), vf.pixels.get(), uf.Size()) == 0);
    }
    {
        // offsetMode != 0 reduces the frame's x offset: 8 -> 4 - 2 = 2.
        IconBuilder o;
        o.Add(8, 0, 1, 1, {0x01, 7, 0x00, 0x80});
        icon* oi = o.Build();
        Canvas c(16, 2);
        IconToBitmap(oi, &c.bmp, 0, 0, 0, 1);
        EXPECT(c.At(2, 0) == 7);
        IconToBitmap(oi, &c.bmp, 0, 1, 0, 0);
        EXPECT(c.At(8, 1) == 7);
    }
}

void TestIconsStress() {
    for (int round = 0; round < 40; ++round) {
        IconBuilder b;
        for (int f = 0; f < 6; ++f) {
            i16 w = static_cast<i16>(Range(1, 90)), h = static_cast<i16>(Range(1, 50));
            b.Add(static_cast<i16>(Range(-30, 30)), static_cast<i16>(Range(-30, 30)), w, h, RandomStream(w, h, true));
        }
        for (int f = 0; f < 6; ++f) {
            i16 w = static_cast<i16>(Range(1, 90)), h = static_cast<i16>(Range(1, 50));
            b.Add(static_cast<i16>(Range(-30, 30)), static_cast<i16>(Range(-30, 30)), w, h, RandomStream(w, h, false));
        }
        icon* ic = b.Build();
        for (int iter = 0; iter < 400; ++iter) {
            i32 frame = Range(0, 5);
            const IconEntry& e = b.frames[static_cast<size_t>(frame)].entry;
            i32 mode = Rand() % 2 ? 0 : Range(1, 3);
            i32 W = Range(200, 640), H = Range(60, 200);
            Canvas c(W, H, static_cast<u8>(Rand()));
            i32 x = Range(-120, W + 120), y = Range(-80, H + 80);
            gClipColumn = 0;
            switch (Range(0, 7)) {
            case 0:
                IconToBitmap(ic, &c.bmp, x, y, frame, mode);
                break;
            case 1:
                // No right-edge test when flipped: keep x - offset on the bitmap.
                FlipIconToBitmap(ic, &c.bmp, Range(0, W - 1) + e.x, y, frame, 0);
                break;
            case 2:
                MonoIconToBitmap(ic, &c.bmp, x, y, frame + 6, static_cast<i32>(Rand()), mode);
                break;
            case 3: {
                const IconEntry& m = b.frames[static_cast<size_t>(frame + 6)].entry;
                FlipMonoIconToBitmap(ic, &c.bmp, Range(0, W - 1) + m.x, y, frame + 6, 5, 0);
                break;
            }
            case 4: {
                Canvas s(640, H);
                DimIconToBitmap(ic, &s.bmp, Range(-120, 760), y, frame + 6, mode);
                break;
            }
            case 5: {
                Canvas s(640, H);
                const IconEntry& m = b.frames[static_cast<size_t>(frame + 6)].entry;
                FlipDimIconToBitmap(ic, &s.bmp, Range(0, 639) + m.x, y, frame + 6, 0);
                break;
            }
            case 6:
                ClippedIconToBitmap(ic, &c.bmp, x, y, frame, mode);
                break;
            case 7: {
                // x - offset must not be negative (frame wholly left of the
                // bitmap); the asm runs away there.
                i32 minimum = e.x > 0 ? e.x : 0;
                FlipClippedIconToBitmap(ic, &c.bmp, x < minimum ? minimum : x, y, frame, 0);
                break;
            }
            }
        }
    }
}

void TestTiles() {
    const u16 size = 8;
    tileset ts{0};
    ts.m_tileCount = 2;
    ts.m_tileWidth = size;
    ts.m_tileHeight = size;
    // index <= m_tileCount is accepted, so the data holds count + 1 tiles.
    std::unique_ptr<u8[]> data(new u8[3 * size * size]);
    for (int i = 0; i < 3 * size * size; ++i)
        data[i] = static_cast<u8>(i * 7 + 1);
    ts.m_data = data.get();

    const u32 flags[4] = {0, 0x8000, 0x4000, 0xC000};
    for (u32 index = 0; index <= 2; ++index)
        for (u32 flag : flags) {
            Canvas c(20, 12);
            TileToBitmap(&ts, flag | index | 0x3000, &c.bmp, 5, 3);
            const u8* t = data.get() + index * size * size;
            for (i32 j = 0; j < size; ++j)
                for (i32 i = 0; i < size; ++i) {
                    i32 dx = (flag & 0x8000) ? size - 1 - i : i;
                    i32 dy = (flag & 0x4000) ? size - 1 - j : j;
                    EXPECT(c.At(5 + dx, 3 + dy) == t[j * size + i]);
                }
            EXPECT(c.At(4, 3) == 0 && c.At(13, 3) == 0 && c.At(5, 2) == 0 && c.At(5, 11) == 0);
        }
    Canvas c(20, 12);
    TileToBitmap(&ts, 3, &c.bmp, 0, 0); // index > m_tileCount: ignored
    EXPECT(c.At(0, 0) == 0);

    // Unflipped and vertical-only rows copy whole DWORDs (width & ~3) and
    // both pointers advance by that amount (plus stride - width on the
    // destination), so a 6-wide tile's second row lands 6 bytes on.
    tileset odd{0};
    odd.m_tileCount = 0;
    odd.m_tileWidth = 6;
    odd.m_tileHeight = 2;
    u8 oddData[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    odd.m_data = oddData;
    Canvas o(8, 2);
    TileToBitmap(&odd, 0, &o.bmp, 1, 0);
    EXPECT(o.At(1, 0) == 1 && o.At(4, 0) == 4 && o.At(5, 0) == 0 && o.At(6, 0) == 0);
    EXPECT(o.At(7, 0) == 5 && o.At(0, 1) == 6 && o.At(1, 1) == 7 && o.At(2, 1) == 8 && o.At(3, 1) == 0);
}

} // namespace

int main() {
    TestBits();
    TestBmap2();
    TestIconsKnown();
    TestIconsStress();
    TestTiles();
    if (gFailures != 0) {
        printf("%d failure(s)\n", gFailures);
        return 1;
    }
    printf("blit_test: all checks passed\n");
    return 0;
}
