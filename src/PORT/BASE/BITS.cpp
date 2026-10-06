// Portable equivalent of the BITS.asm bit-array routines, built by the native port.

#include <H1/Ints.h>

#include <BASE/BITS.h>

// BitClear is public in BITS.asm but no header declares it.
extern "C" void BitClear(void* bits, u32 bit);

namespace {

// Byte holding `bit` (bits are numbered LSB-first within each byte).
inline u8* BitByte(void* bits, u32 bit) {
    return static_cast<u8*>(bits) + (bit >> 3);
}

inline u8 BitMask(u32 bit) {
    return static_cast<u8>(1u << (bit & 7));
}

} // namespace

// Returns 1 when `bit` is set, else 0.
//
// The asm tests a DWORD loaded at bits + (bit >> 3), touching up to three
// bytes past the addressed one. Its mask only covers the addressed byte, so
// that byte alone decides the result; this reads just that byte.
extern "C" i32 BitTest(const void* bits, u32 bit) {
    const u8* byte = static_cast<const u8*>(bits) + (bit >> 3);
    return (*byte & BitMask(bit)) != 0 ? 1 : 0;
}

// Sets `bit`. The asm ORs into a DWORD at the addressed byte, rewriting the
// next three bytes unchanged; only the addressed byte is touched here.
extern "C" void BitSet(void* bits, u32 bit) {
    *BitByte(bits, bit) |= BitMask(bit);
}

// Clears `bit`. Same DWORD read-modify-write note as BitSet.
extern "C" void BitClear(void* bits, u32 bit) {
    *BitByte(bits, bit) &= static_cast<u8>(~BitMask(bit));
}
