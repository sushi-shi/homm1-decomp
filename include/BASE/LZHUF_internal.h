#ifndef HOMM1_BASE_LZHUF_INTERNAL_H
#define HOMM1_BASE_LZHUF_INTERNAL_H

// Bridge between the VC4 wrapper/encoder and the linked legacy decoder
// members. Keep private declarations out of the implementation file so the
// source-order annotations can follow retail layout.
extern "C" {
void Decode();
extern const unsigned char d_code[256];
extern const unsigned char d_len[256];
extern short initialSon[627];
extern unsigned short initialFrequency[628];
extern short initialParent[941];
}

void LogStr(char *, long, long);

static void UpdateEncoderTree(short character);
static void InsertNode(short node);
static void DeleteNode(short node);

#endif // HOMM1_BASE_LZHUF_INTERNAL_H
