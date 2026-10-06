// The network save compressor (vendor/lzhuf/encoder.cpp, the portable
// decoder src/PORT/BASE/LZHUFDEC.cpp): the input is decoded as a compressed
// stream from an exactly-sized buffer with DecodeDataBounded, into an output
// of exactly its claimed size and into one a byte too small (which must be
// refused); then, as plain data, it is encoded, and the encoding must decode
// from an exactly-sized buffer back to the input.

#include "FuzzSupport.h"

#include <BASE/LZHUF.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

// encoder.cpp's EncodeData polls the sound system; nothing to do here.
extern "C" void PollSound() {}

namespace {

// The largest decoded size the harness allocates.
const u32 kMaxOutput = 1u << 20;

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > (1u << 20))
        return 0;

    // As a compressed stream: exact-size input and output buffers.
    std::unique_ptr<char[]> stream(new char[size > 0 ? size : 1]);
    std::memcpy(stream.get(), data, size);
    u32 claimed = 0;
    if (size >= 4)
        claimed = (static_cast<u32>(data[0]) << 24) | (static_cast<u32>(data[1]) << 16)
                  | (static_cast<u32>(data[2]) << 8) | data[3];
    if (claimed <= kMaxOutput) {
        std::unique_ptr<char[]> output(new char[claimed > 0 ? claimed : 1]);
        i32 decoded = DecodeDataBounded(output.get(), claimed, stream.get(), static_cast<u32>(size));
        if (decoded != -1 && decoded != static_cast<i32>(claimed)) {
            std::fprintf(stderr, "fuzz_lzhuf: decoded size differs from the claimed size\n");
            std::abort();
        }
        if (claimed > 0
            && DecodeDataBounded(output.get(), claimed - 1, stream.get(), static_cast<u32>(size)) != -1) {
            std::fprintf(stderr, "fuzz_lzhuf: a stream larger than its output was decoded\n");
            std::abort();
        }
    }

    // As plain data: the round trip.
    if (size >= 1) {
        std::vector<char> source(data, data + size);
        std::vector<char> packed(size * 2 + 64);
        i32 encoded = EncodeData(packed.data(), source.data(), static_cast<u32>(size));
        size_t length = static_cast<size_t>(encoded) + 4;
        std::unique_ptr<char[]> exact(new char[length]);
        std::memcpy(exact.get(), packed.data(), length);
        std::unique_ptr<char[]> output(new char[size]);
        i32 decoded = DecodeDataBounded(output.get(), static_cast<u32>(size), exact.get(),
                                        static_cast<u32>(length));
        if (decoded != static_cast<i32>(size) || std::memcmp(output.get(), data, size) != 0) {
            std::fprintf(stderr, "fuzz_lzhuf: round trip changed the data (%d of %zu, encoded %d)\n", decoded, size, encoded);
            std::abort();
        }
    }
    return 0;
}
