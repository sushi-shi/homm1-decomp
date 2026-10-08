#ifndef HOMM1_BASE_LZHUF_INTERNAL_H
#define HOMM1_BASE_LZHUF_INTERNAL_H

extern "C" {
    void Decode();
    extern u8 d_code[256];
    extern u8 d_len[256];
    extern i16 initialSon[627];
    extern u16 initialFrequency[628];
    extern i16 initialParent[941];
}

void InitializeTree(void);
void ReconstructEncoderTree(void);
void PutCode(i16 length, u16 code);
void EncodeCharacter(u16 character);
void EncodePosition(u16 position);
static void UpdateEncoderTree(i16 character);
static void InsertNode(i16 node);
static void DeleteNode(i16 node);
static void EncodeEnd(void);

#endif
