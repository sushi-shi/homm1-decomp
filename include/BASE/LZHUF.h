#ifndef HOMM1_BASE_LZHUF_H
#define HOMM1_BASE_LZHUF_H

i32 EncodeData(char* destination, char* source, u32 sourceLength);
i32 DecodeData(char* destination, char* source);

#ifdef HOMM1_PORT
// DecodeData for a stream from elsewhere (the portable decoder only): the
// stream is sourceLength bytes and the output holds capacity bytes. Returns
// the decoded size, or -1 when the stream claims more than capacity bytes or
// ends before it has produced them; nothing is read or written outside the
// two buffers. DecodeData itself trusts the stream, as the original did.
i32 DecodeDataBounded(char* destination, u32 capacity, char* source, u32 sourceLength);
#endif

#endif
