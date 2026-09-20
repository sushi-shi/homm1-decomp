/*
 * REFERENCE ONLY: this file is not a build input. It is the ordinary-C
 * correspondence source for the legacy decoder objects linked into
 * HEROES.EXE. The short table/index types are part of this port.
 */
#include <match.h>

#define N 4096
#define F 60
#define THRESHOLD 2
#define N_CHAR (256 - THRESHOLD + F)
#define T (N_CHAR * 2 - 1)
#define R (T - 1)
#define MAX_FREQ 0x8000

extern short son[T];
extern short prnt[T + N_CHAR];
extern unsigned short freq[T + 1];
extern unsigned char text_buf[N + F - 1];
extern unsigned long textsize;
extern unsigned long decodeSize;
extern unsigned short getbuf;
extern unsigned char getlen;
extern char *codePtr;
extern char *decodeOutput;
extern const unsigned char d_code[256];
extern const unsigned char d_len[256];

int GetBit(void);
int DecodePosition(void);
void UpdateDecoderTree(short c);
void ReconstructDecoderTree(void);
void Decode(void);
void LzhufMemmove(void *destination, const void *source, unsigned count);

#ifdef __WATCOMC__
#pragma aux GetBit "_GetBit"
#pragma aux DecodePosition "_DecodePosition"
#pragma aux UpdateDecoderTree "_UpdateDecoderTree"
#pragma aux ReconstructDecoderTree "_ReconstructDecoderTree"
#pragma aux Decode "_Decode"
#pragma aux LzhufMemmove "_LzhufMemmove"
#endif

VA(0x0047fca5, 0x85)
int GetBit(void)
{
    if (getlen > 8) {
        register unsigned short i = getbuf;
        getbuf <<= 1;
        getlen--;
        return (i & 0x8000) >> 15;
    } else {
        register unsigned i;
        register unsigned short dx = getbuf;
        register unsigned char glen = getlen;
        register unsigned char *cursor = (unsigned char *)codePtr;

        do {
            if ((int)(i = *cursor++) < 0)
                i = 0;
            dx |= (unsigned short)(i << (8 - glen));
            glen += 8;
        } while (glen <= 8);
        getbuf = (unsigned short)(dx << 1);
        getlen = (unsigned char)(glen - 1);
        codePtr = (char *)cursor;
        return (dx & 0x8000) >> 15;
    }
}

VA(0x0047fd2a, 0xc9)
int DecodePosition(void)
{
    register unsigned i;
    register unsigned short dx = getbuf;
    register unsigned char glen = getlen;
    unsigned j, c;

    while (glen <= 8) {
        if ((int)(i = (unsigned char)*codePtr++) < 0)
            i = 0;
        dx |= (unsigned short)(i << (8 - glen));
        glen += 8;
    }
    getbuf = (unsigned short)(dx << 8);
    getlen = (unsigned char)(glen - 8);
    i = (dx >> 8) & 0xff;

    c = (unsigned)d_code[i] << 6;
    j = d_len[i];
    j -= 2;
    while (j--)
        i = (i << 1) + GetBit();
    return c | (i & 0x3f);
}

VA(0x0047fdf3, 0x119)
void UpdateDecoderTree(short c)
{
    short k, l, j, i;

    if (freq[R] == MAX_FREQ)
        ReconstructDecoderTree();
    c = prnt[c + T];
    do {
        k = (short)++freq[c];
        if (k > freq[l = (short)(c + 1)]) {
            while (k > freq[++l])
                ;
            l--;
            freq[c] = freq[l];
            freq[l] = (unsigned short)k;
            i = son[c];
            prnt[i] = l;
            if (i < T)
                prnt[i + 1] = l;
            j = son[l];
            son[l] = i;
            prnt[j] = c;
            if (j < T)
                prnt[j + 1] = c;
            son[c] = j;
            c = l;
        }
    } while ((int)(c = prnt[c]) != 0);
}

VA(0x0047ff0c, 0x153)
void ReconstructDecoderTree(void)
{
    short i, j, k;
    unsigned short f, l;

    j = 0;
    for (i = 0; i < T; i++) {
        if (son[i] >= T) {
            freq[j] = (unsigned short)((freq[i] + 1) / 2);
            son[j] = son[i];
            j++;
        }
    }
    for (i = 0, j = N_CHAR; j < T; i += 2, j++) {
        k = (short)(i + 1);
        f = freq[j] = (unsigned short)(freq[i] + freq[k]);
        for (k = (short)(j - 1); f < freq[k]; k--)
            ;
        k++;
        l = (unsigned short)((j - k) * 2);
        LzhufMemmove(&freq[k + 1], &freq[k], l);
        freq[k] = f;
        LzhufMemmove(&son[k + 1], &son[k], l);
        son[k] = i;
    }
    for (i = 0; i < T; i++) {
        if ((k = son[i]) >= T)
            prnt[k] = i;
        else
            prnt[k] = prnt[k + 1] = i;
    }
}

VA(0x0048005f, 0x129)
void Decode(void)
{
    short i, j, k, r, c;
    unsigned long count;
    char *output;
    unsigned long length;

    output = decodeOutput;
    length = decodeSize;
    r = N - F;
    for (count = 0; count < textsize + length; ) {
        c = son[R];
        while ((unsigned short)c < T) {
            c = (short)(c + GetBit());
            c = son[c];
        }
        c = (short)(c - T);
        UpdateDecoderTree(c);
        if (c < 256) {
            if (count >= textsize)
                *output++ = (char)c;
            text_buf[r++] = (unsigned char)c;
            r &= N - 1;
            count++;
        } else {
            i = (short)((r - DecodePosition() - 1) & (N - 1));
            j = (short)(c - 255 + THRESHOLD);
            for (k = 0; k < j; k++) {
                c = text_buf[(i + k) & (N - 1)];
                if (count >= textsize && count < textsize + length)
                    *output++ = (char)c;
                text_buf[r++] = (unsigned char)c;
                r &= N - 1;
                count++;
            }
        }
    }
}
