// The Smacker movie decoder (src/PLATFORM/SmackerDecoder.cpp), which reads
// the movies in the user's ANIM folder: every frame with the first audio
// track. A movie whose header is refused, or that ends before its last
// frame, counts as refused.

#include "FuzzSupport.h"

#include <PLATFORM/SmackerDecoder.h>

#include <vector>

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > (1u << 24))
        return 0;
    platform::smacker::Decoder decoder;
    if (!decoder.Open(std::vector<u8>(data, data + size))) {
        gFuzzRejected = true;
        return 0;
    }
    std::vector<u8> audio;
    while (decoder.DecodeFrame(0, &audio))
        audio.clear();
    if (decoder.CurrentFrame() < decoder.FrameCount())
        gFuzzRejected = true;
    return 0;
}
