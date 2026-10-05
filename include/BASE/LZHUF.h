#ifndef HOMM1_BASE_LZHUF_H
#define HOMM1_BASE_LZHUF_H

// HoMM1's multiplayer-save codec.  The encoded stream starts with the
// uncompressed length as a four-byte, most-significant-byte-first value.
// EncodeData returns the number of bytes after that prefix.
i32 EncodeData(char* destination, char* source, u32 sourceLength);
i32 DecodeData(char* destination, char* source);

#endif // HOMM1_BASE_LZHUF_H
