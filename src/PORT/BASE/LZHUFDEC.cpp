// Portable equivalent of the vendor/lzhuf/decoder/Decoder.asm LZHUF decoder, built by the native port.

#include <H1/Ints.h>

#include <string.h>

// The decoder state and tables are defined in vendor/lzhuf/encoder.cpp (the
// asm only declares them EXTERN); the types here match those definitions.
// <BASE/LZHUF_internal.h> is not included because its static encoder
// prototypes trigger -Wunused-function in any other translation unit.
extern "C" {
extern i16 son[627];
extern i16 prnt[941];
extern u16 freq[628];
extern u8 text_buf[4155];
extern u16 getbuf;
extern u8 getlen;
extern char* dataPtr;
extern char* outputPos;
extern u32 decodeLen;
extern u32 textsize;
extern u8 d_code[256];
extern u8 d_len[256];

// Procedures defined by Decoder.asm. Only Decode is called from C++
// (DecodeData in encoder.cpp); the others were register-convention helpers
// and are given ordinary C signatures.
void LzhufMemmove(void* destination, const void* source, u32 count);
i32 GetBit(void);
u16 DecodePosition(void);
void UpdateDecoderTree(i32 character);
void ReconstructDecoderTree(void);
void Decode();
}

namespace {

// LZSS window and adaptive Huffman tree geometry (as in encoder.cpp).
const u32 kWindowSize = 4096;
const u32 kWindowMask = kWindowSize - 1;
const u32 kLookAhead = 60;
const i32 kThreshold = 2;
const i32 kCharacterCount = 256 - kThreshold + kLookAhead; // 314 symbols
const i32 kTreeSize = kCharacterCount * 2 - 1;             // 627 nodes
const i32 kRoot = kTreeSize - 1;
const u16 kMaxFrequency = 0x8000;

// Tops up the 16-bit bit buffer with whole input bytes until more than 8 bits
// are buffered, and returns the widened buffer. getlen is compared as a
// signed byte and each byte is shifted by (8 - getlen) & 31, as in the asm.
//
// Like the asm this may read one or two bytes beyond the last code byte of
// the stream (it refills whenever 8 or fewer bits are buffered).
u32 FillBitBuffer(i8& length) {
    u32 buffer = getbuf;
    do {
        u32 byte = static_cast<u8>(*dataPtr++);
        buffer |= byte << ((8u - static_cast<u32>(static_cast<u8>(length))) & 31u);
        length = static_cast<i8>(static_cast<u8>(length) + 8u);
    } while (length <= 8);
    return buffer;
}

} // namespace

// memmove: the asm copies forward by DWORDs or, for an overlapping move to a
// higher address, backward by WORDs; both equal memmove for every input.
extern "C" void LzhufMemmove(void* destination, const void* source, u32 count) {
    memmove(destination, source, count);
}

// Returns the next input bit (0 or 1).
extern "C" i32 GetBit(void) {
    i8 length = static_cast<i8>(getlen);
    u32 buffer = getbuf;
    if (length <= 8)
        buffer = FillBitBuffer(length);
    getbuf = static_cast<u16>(buffer << 1);
    getlen = static_cast<u8>(length - 1);
    return static_cast<i32>((buffer >> 15) & 1);
}

// Decodes a match position (0..4095): the upper 6 bits come from d_code via
// the next input byte, then d_len - 2 more bits extend the lower 6 bits.
extern "C" u16 DecodePosition(void) {
    i8 length = static_cast<i8>(getlen);
    u32 buffer = getbuf;
    if (length <= 8)
        buffer = FillBitBuffer(length);
    getbuf = static_cast<u16>(buffer << 8);
    getlen = static_cast<u8>(length - 8);
    u32 i = (buffer >> 8) & 0xFF;

    u32 upper = static_cast<u32>(d_code[i]) << 6;
    // The asm counts a 16-bit register down from d_len - 2 to -1.
    for (u16 extra = static_cast<u16>(d_len[i] - 2); extra != 0; --extra)
        i = (i << 1) + static_cast<u32>(GetBit());
    return static_cast<u16>(upper | (i & 0x3F));
}

// Increments the frequency of `character` and its ancestors, swapping nodes
// to keep freq[] sorted (the LZHUF "update" step). Frequencies are compared as
// a signed 16-bit count against the zero-extended neighbour, as in the asm.
extern "C" void UpdateDecoderTree(i32 character) {
    if (freq[kRoot] == kMaxFrequency)
        ReconstructDecoderTree();

    i16 c = prnt[static_cast<i16>(character) + kTreeSize];
    do {
        u16 count = static_cast<u16>(freq[c] + 1);
        freq[c] = count;
        i32 k = static_cast<i16>(count);
        i32 l = c + 1;
        if (k > freq[static_cast<i16>(l)]) {
            while (k > freq[static_cast<i16>(++l)]) {
            }
            --l;
            i16 swapped = static_cast<i16>(l);
            freq[c] = freq[swapped];
            freq[swapped] = count;

            i16 i = son[c];
            prnt[i] = swapped;
            if (i < kTreeSize)
                prnt[i + 1] = swapped;

            i16 j = son[swapped];
            son[swapped] = i;
            prnt[j] = c;
            if (j < kTreeSize)
                prnt[j + 1] = c;
            son[c] = j;

            c = swapped;
        }
        c = prnt[c];
    } while (c != 0);
}

// Rebuilds the tree when the root frequency reaches kMaxFrequency: collects
// the leaves with halved (rounded up) frequencies, then re-adds the internal
// nodes in sorted order and recomputes the parent links.
//
// The asm fetches son[i] as the high half of a DWORD at &son[i - 1], so for
// i == 0 it also reads the two bytes before son[]; only son[i] is read here.
extern "C" void ReconstructDecoderTree(void) {
    i32 j = 0;
    for (i32 i = 0; i < kTreeSize; ++i) {
        if (son[i] >= kTreeSize) {
            freq[static_cast<i16>(j)] = static_cast<u16>((freq[i] + 1) / 2);
            son[static_cast<i16>(j)] = son[i];
            ++j;
        }
    }

    i16 i = 0;
    for (j = kCharacterCount; static_cast<i16>(j) < kTreeSize; ++j, i = static_cast<i16>(i + 2)) {
        u16 f = static_cast<u16>(freq[i] + freq[i + 1]);
        freq[static_cast<i16>(j)] = f;
        i32 k = j;
        do {
            --k;
        } while (f < freq[static_cast<i16>(k)]);
        ++k;
        u16 bytes = static_cast<u16>((j - k) * 2);
        i16 slot = static_cast<i16>(k);
        LzhufMemmove(&freq[slot + 1], &freq[slot], bytes);
        freq[slot] = f;
        LzhufMemmove(&son[slot + 1], &son[slot], bytes);
        son[slot] = i;
    }

    for (i16 node = 0; node < kTreeSize; node = static_cast<i16>(node + 1)) {
        i32 k = son[node];
        if (k < kTreeSize) {
            prnt[k + 1] = node;
            prnt[k] = node;
        } else {
            prnt[k] = node;
        }
    }
}

// Decodes textsize + decodeLen symbols' worth of output from dataPtr, writing
// the bytes whose index lies in [textsize, textsize + decodeLen) to
// outputPos. DecodeData sets up getbuf/getlen, the initial tree, textsize = 0,
// decodeLen and outputPos before calling this.
//
// outputPos is read but not advanced. A match that runs past the end still
// updates the window and the counter for its full length, but only bytes
// inside the output range are stored.
extern "C" void Decode() {
    char* out = outputPos;
    u32 r = kWindowSize - kLookAhead;
    u32 count = 0;

    while (count < textsize + decodeLen) {
        // Walk from the root to a leaf, one input bit per level.
        u32 node = static_cast<u16>(son[kRoot]);
        while (node < static_cast<u32>(kTreeSize))
            node = static_cast<u16>(son[(node + static_cast<u32>(GetBit())) & 0xFFFF]);
        i16 c = static_cast<i16>(node - kTreeSize);
        UpdateDecoderTree(c);

        if (c < 256) {
            if (count >= textsize)
                *out++ = static_cast<char>(c);
            text_buf[r] = static_cast<u8>(c);
            ++count;
            r = (r + 1) & kWindowMask;
            continue;
        }

        // Match: copy `length` bytes from `position` back in the window.
        u32 position = (r - DecodePosition() - 1) & kWindowMask;
        i16 length = static_cast<i16>(c - 253);
        if (length <= 0)
            continue;
        i16 k = 0;
        do {
            u8 byte = text_buf[(position + static_cast<u32>(k)) & kWindowMask];
            if (count >= textsize && count < textsize + decodeLen)
                *out++ = static_cast<char>(byte);
            text_buf[r] = byte;
            ++count;
            ++k;
            r = (r + 1) & kWindowMask;
        } while (k < length);
    }
}
