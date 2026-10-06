// Round-trip test: vendor/lzhuf/encoder.cpp EncodeData -> portable LZHUFDEC.cpp DecodeData.
//
// Build and run from the repository root (one command):
//   g++ -std=c++20 -Wall -Wextra -fsigned-char -D__cdecl= -g -fsanitize=address,undefined
//       -fno-sanitize-recover=all -fpermissive -Wno-register -no-pie -Iinclude
//       tests/port/lzhuf_roundtrip_test.cpp vendor/lzhuf/encoder.cpp src/PORT/BASE/LZHUFDEC.cpp
//       -o lzhuf_roundtrip_test && ./lzhuf_roundtrip_test
//
// -fpermissive and -no-pie are only for encoder.cpp, whose InsertNode casts a
// text_buf pointer to u32 (line 426): with a non-PIE link that global lies
// below 4 GiB, so the truncating cast round-trips on 64-bit.
//
// The decoder refills its bit buffer whenever 8 or fewer bits remain, so like
// the original asm it can read up to two bytes past the last code byte; the
// encoded buffers therefore carry kReadSlack extra bytes. Output buffers are
// exact-size heap blocks so the sanitizers catch any overrun.

#include <H1/Ints.h>

#include <BASE/LZHUF.h>

#include <stdio.h>
#include <string.h>

#include <memory>
#include <string>
#include <vector>

// encoder.cpp's EncodeData polls the sound system; nothing to do here.
extern "C" void PollSound() {}

extern "C" char* dataPtr;

namespace {

const size_t kReadSlack = 2;
int gFailures = 0;

u32 gSeed = 0x2545F491;
u32 Rand() {
    gSeed ^= gSeed << 13;
    gSeed ^= gSeed >> 17;
    gSeed ^= gSeed << 5;
    return gSeed;
}

void RoundTrip(const char* name, const std::vector<u8>& input) {
    u32 size = static_cast<u32>(input.size());
    std::vector<char> source(input.begin(), input.end());
    std::vector<char> packed(static_cast<size_t>(size) * 2 + 64);
    i32 codeSize = EncodeData(packed.data(), source.data(), size);
    size_t encoded = static_cast<size_t>(codeSize) + 4; // 4-byte big-endian length header

    // Exact-size copy of the stream (plus the decoder's read slack).
    std::unique_ptr<char[]> stream(new char[encoded + kReadSlack]);
    memcpy(stream.get(), packed.data(), encoded);
    memset(stream.get() + encoded, 0, kReadSlack);
    std::unique_ptr<char[]> output(new char[size]);

    i32 decoded = DecodeData(output.get(), stream.get());
    size_t consumed = static_cast<size_t>(dataPtr - stream.get());
    bool ok = decoded == static_cast<i32>(size) && memcmp(output.get(), input.data(), size) == 0
        && consumed <= encoded + kReadSlack;
    printf("%-24s %7u bytes -> %7zu encoded, read %7zu: %s\n", name, size, encoded, consumed, ok ? "ok" : "FAIL");
    if (!ok)
        ++gFailures;
}

std::vector<u8> Repeat(const char* pattern, size_t size) {
    std::vector<u8> v(size);
    size_t length = strlen(pattern);
    for (size_t i = 0; i < size; ++i)
        v[i] = static_cast<u8>(pattern[i % length]);
    return v;
}

std::vector<u8> Random(size_t size, u32 mask = 0xFF) {
    std::vector<u8> v(size);
    for (auto& b : v)
        b = static_cast<u8>(Rand() & mask);
    return v;
}

// Random data with frequent back-references (LZ-friendly, all match lengths).
std::vector<u8> Structured(size_t size) {
    std::vector<u8> v;
    v.reserve(size);
    while (v.size() < size) {
        if (v.size() > 64 && Rand() % 3 != 0) {
            size_t distance = 1 + Rand() % (v.size() < 4096 ? v.size() : 4096);
            size_t length = 1 + Rand() % 70;
            for (size_t i = 0; i < length && v.size() < size; ++i)
                v.push_back(v[v.size() - distance]);
        } else {
            v.push_back(static_cast<u8>(Rand()));
        }
    }
    return v;
}

} // namespace

int main() {
    // EncodeData cannot encode an empty input: with sourceLength == 0 its
    // `--len` wraps and it emits ~64K symbols. Sizes start at 1.
    RoundTrip("one byte", {0x41});
    RoundTrip("two bytes", {0x00, 0xFF});
    RoundTrip("three zero bytes", {0, 0, 0});
    RoundTrip("60 spaces", Repeat(" ", 60));
    RoundTrip("61 x", Repeat("x", 61));
    RoundTrip("repetitive 5000", Repeat("a", 5000));
    RoundTrip("pattern abcab", Repeat("abcab", 7777));
    RoundTrip("text", Repeat("The quick brown fox jumps over the lazy dog. ", 3000));
    RoundTrip("random 1", Random(1));
    RoundTrip("random 100", Random(100));
    RoundTrip("random 4096", Random(4096));
    RoundTrip("random 4 symbols", Random(20000, 3));
    RoundTrip("structured 30000", Structured(30000));
    // > 32768 symbols forces ReconstructDecoderTree (root frequency 0x8000).
    RoundTrip("random 100000", Random(100000));
    RoundTrip("structured 300000", Structured(300000));
    for (int i = 0; i < 20; ++i) {
        size_t size = 1 + Rand() % 9000;
        RoundTrip("mixed", i % 2 ? Random(size, i % 4 == 1 ? 0x0F : 0xFF) : Structured(size));
    }

    if (gFailures != 0) {
        printf("%d failure(s)\n", gFailures);
        return 1;
    }
    printf("lzhuf_roundtrip_test: all round trips passed\n");
    return 0;
}
