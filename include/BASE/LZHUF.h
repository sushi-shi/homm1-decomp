#ifndef HOMM1_BASE_LZHUF_H
#define HOMM1_BASE_LZHUF_H

// HoMM1's multiplayer-save codec.  The encoded stream starts with the
// uncompressed length as a four-byte, most-significant-byte-first value.
// EncodeData returns the number of bytes after that prefix, matching retail.
long EncodeData(char *destination, char *source, unsigned long sourceLength);
long DecodeData(char *destination, char *source);

#endif // HOMM1_BASE_LZHUF_H
