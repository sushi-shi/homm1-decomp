#ifndef HOMM1_BASE_LZHUF_INTERNAL_H
#define HOMM1_BASE_LZHUF_INTERNAL_H

// Shared between the C++ wrappers and encoder (encoder.cpp) and the assembly
// decoder (decoder/Decoder.asm).
extern "C" {
    void Decode();
    extern u8 d_code[256];
    extern u8 d_len[256];
    extern i16 initialSon[627];
    extern u16 initialFrequency[628];
    extern i16 initialParent[941];
}

void PutCode(i16 length, u16 code);
void EncodeCharacter(u16 character);
void EncodePosition(u16 position);
static void UpdateEncoderTree(i16 character);
static void InsertNode(i16 node);
static void DeleteNode(i16 node);

#endif // HOMM1_BASE_LZHUF_INTERNAL_H
