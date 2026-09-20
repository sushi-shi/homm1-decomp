// Okumura-style LZSS with adaptive Huffman coding, used for HoMM1's
// multiplayer save transfer.  The tables and stream framing are taken from
// the pinned February 1996 executable.

#include <match.h>

#include <BASE/LZHUF.h>

#include <BASE/LZHUF_internal.h>
#include <H1/KB.h>

#include <string.h>

#define WINDOW_SIZE 4096
#define LOOK_AHEAD 60
#define MATCH_THRESHOLD 2
#define NIL WINDOW_SIZE
#define CHARACTER_COUNT (256 - MATCH_THRESHOLD + LOOK_AHEAD)
#define TREE_SIZE (CHARACTER_COUNT * 2 - 1)
#define ROOT (TREE_SIZE - 1)
#define MAX_FREQUENCY 0x8000

extern "C" {
DATA(0x004cc8f0) short dad[WINDOW_SIZE + 1];
DATA(0x004ce900) short lson[WINDOW_SIZE + 1];
DATA(0x004d0908) short son[TREE_SIZE];
DATA(0x004d0df0) short prnt[TREE_SIZE + CHARACTER_COUNT];
DATA(0x004d1554) unsigned long textsize;
DATA(0x004d1558) unsigned char text_buf[WINDOW_SIZE + LOOK_AHEAD - 1];
DATA(0x004d2598) short rson[WINDOW_SIZE + 257];
DATA(0x004d479c) unsigned long codesize;
DATA(0x004d47a0) unsigned short getbuf;
DATA(0x004d47a8) unsigned short freq[TREE_SIZE + 1];
DATA(0x004d4c90) unsigned char getlen;
DATA(0x004d4c94) char *codePtr;
DATA(0x004d4c98) short match_position;
DATA(0x004d4c9c) short match_length;
DATA(0x004d4ca0) char *decodeOutput;
DATA(0x004ce8f4) unsigned long decodeSize;

DATA(0x004a2f80) unsigned short putbuf = 0;
DATA(0x004a2f84) unsigned char putlen = 0;

#include "tables.inc"
}

DATA(0x004a2f00) static unsigned char positionLength[64] = {
    0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

DATA(0x004a2f40) static unsigned char positionCode[64] = {
    0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
    0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
    0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
    0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
    0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
    0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

extern "C" DATA(0x004a1bd0) const unsigned char d_code[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7, 8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,
    10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
    12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,
    16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,
    20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,
    24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,
    32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,39,
    40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,47,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
};

extern "C" DATA(0x004a1cd0) const unsigned char d_len[256] = {
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

inline void InitializeTree(void)
{
    short i;

    for (i = WINDOW_SIZE + 1; i <= WINDOW_SIZE + 256; ++i)
        rson[i] = NIL;
    for (i = 0; i < WINDOW_SIZE; ++i)
        dad[i] = NIL;
}

VA(0x0047da60, 0x14f)
inline void ReconstructEncoderTree(void)
{
    short i, j, k;
    unsigned short value, length;

    j = 0;
    for (i = 0; i < TREE_SIZE; ++i) {
        if (son[i] >= TREE_SIZE) {
            freq[j] = static_cast<unsigned short>((freq[i] + 1) / 2);
            son[j] = son[i];
            ++j;
        }
    }
    for (i = 0, j = CHARACTER_COUNT; j < TREE_SIZE; i += 2, ++j) {
        value = static_cast<unsigned short>(freq[i] + freq[i + 1]);
        freq[j] = value;
        for (k = j - 1; value < freq[k]; --k)
            ;
        ++k;
        length = static_cast<unsigned short>((j - k) * 2);
        memmove(&freq[k + 1], &freq[k], length);
        freq[k] = value;
        memmove(&son[k + 1], &son[k], length);
        son[k] = static_cast<short>(i);
    }
    for (i = 0; i < TREE_SIZE; ++i) {
        k = son[i];
        if (k >= TREE_SIZE)
            prnt[k] = static_cast<short>(i);
        else
            prnt[k] = prnt[k + 1] = static_cast<short>(i);
    }
}

inline void PutCode(short length, unsigned short code)
{
    putbuf = static_cast<unsigned short>(putbuf | (code >> putlen));
    putlen = static_cast<unsigned char>(putlen + length);
    if (putlen >= 8) {
        *codePtr++ = static_cast<char>(putbuf >> 8);
        putlen = static_cast<unsigned char>(putlen - 8);
        if (putlen >= 8) {
            *codePtr++ = static_cast<char>(putbuf);
            codesize += 2;
            putlen = static_cast<unsigned char>(putlen - 8);
            putbuf = static_cast<unsigned short>(code << (length - putlen));
        } else {
            putbuf = static_cast<unsigned short>(putbuf << 8);
            ++codesize;
        }
    }
}

inline void EncodeCharacter(unsigned short character)
{
    unsigned short code;
    short length, node;

    code = 0;
    length = 0;
    node = prnt[character + TREE_SIZE];
    do {
        code >>= 1;
        if (node & 1)
            code = static_cast<unsigned short>(code + 0x8000);
        ++length;
        node = prnt[node];
    } while (node != ROOT);
    PutCode(length, code);
    UpdateEncoderTree(character);
}

inline void EncodePosition(unsigned short position)
{
    unsigned short upper;

    upper = position >> 6;
    PutCode(positionLength[upper],
            static_cast<unsigned short>(positionCode[upper] << 8));
    PutCode(6, static_cast<unsigned short>((position & 0x3F) << 10));
}

VA(0x0047d250, 0xb9)
long DecodeData(char *destination, char *source)
{
    register unsigned long size;

    textsize = 0;
    codePtr = source;
    size = static_cast<unsigned char>(*codePtr++);
    size <<= 8;
    size |= static_cast<unsigned char>(*codePtr++);
    size <<= 8;
    size |= static_cast<unsigned char>(*codePtr++);
    size <<= 8;
    size |= static_cast<unsigned char>(*codePtr++);
    getbuf = 0;
    getlen = 0;
    memcpy(son, initialSon, sizeof(son));
    memcpy(freq, initialFrequency, sizeof(freq));
    memcpy(prnt, initialParent, sizeof(prnt));
    decodeSize = size;
    decodeOutput = destination;
    Decode();
    LogStr("Data decoded", size, size);
    return static_cast<long>(size);
}
VA(0x0047d310, 0x743)
long EncodeData(char *destination, char *source, unsigned long sourceLength)
{
    register short i, c, r, s, last_match_length;
    register unsigned short len;
    unsigned long consumed;

    getbuf = 0;
    codesize = 0;
    putbuf = 0;
    getlen = 0;
    putlen = 0;
    memset(freq, 0, sizeof(freq));
    memset(prnt, 0, sizeof(prnt));
    memset(son, 0, sizeof(son));

    codePtr = destination;
    *codePtr++ = static_cast<char>(sourceLength >> 24);
    *codePtr++ = static_cast<char>(sourceLength >> 16);
    *codePtr++ = static_cast<char>(sourceLength >> 8);
    *codePtr++ = static_cast<char>(sourceLength);

    consumed = 0;
    memcpy(son, initialSon, sizeof(son));
    memcpy(freq, initialFrequency, sizeof(freq));
    memcpy(prnt, initialParent, sizeof(prnt));
    InitializeTree();
    s = 0;
    r = WINDOW_SIZE - LOOK_AHEAD;
    for (i = s; i < r; ++i)
        text_buf[i] = ' ';
    for (len = 0; len < LOOK_AHEAD && consumed < sourceLength; ++len) {
        text_buf[r + len] = *source++;
        ++consumed;
    }

    for (i = 1; i <= LOOK_AHEAD; ++i)
        InsertNode(r - i);
    InsertNode(r);
    do {
        if (match_length > len)
            match_length = static_cast<short>(len);
        if (match_length <= MATCH_THRESHOLD) {
            match_length = 1;
            EncodeCharacter(text_buf[r]);
        } else {
            EncodeCharacter(255 - MATCH_THRESHOLD + match_length);
            EncodePosition(match_position);
        }
        last_match_length = match_length;
        for (i = 0; i < last_match_length && consumed < sourceLength; ++i) {
            c = *source;
            DeleteNode(s);
            text_buf[s] = static_cast<unsigned char>(c);
            if (s < LOOK_AHEAD - 1)
                text_buf[s + WINDOW_SIZE] = static_cast<unsigned char>(c);
            s = (s + 1) & (WINDOW_SIZE - 1);
            r = (r + 1) & (WINDOW_SIZE - 1);
            InsertNode(r);
            ++consumed;
            ++source;
        }
        while (i++ < last_match_length) {
            DeleteNode(s);
            s = (s + 1) & (WINDOW_SIZE - 1);
            r = (r + 1) & (WINDOW_SIZE - 1);
            if (--len != 0)
                InsertNode(r);
        }
        PollSound();
    } while (len != 0);

    if (putlen != 0) {
        *codePtr++ = static_cast<char>(putbuf >> 8);
        ++codesize;
    }
    return static_cast<long>(codesize);
}

VA(0x0047dbb0, 0x23d)
static void UpdateEncoderTree(short character)
{
    short node, next, child, otherChild;
    short value;

    if (freq[ROOT] == MAX_FREQUENCY)
        ReconstructEncoderTree();
    node = prnt[character + TREE_SIZE];
    do {
        value = ++freq[node];
        next = node + 1;
        if (value > freq[next]) {
            while (value > freq[++next])
                ;
            --next;
            freq[node] = freq[next];
            freq[next] = value;

            child = son[node];
            prnt[child] = static_cast<short>(next);
            if (child < TREE_SIZE)
                prnt[child + 1] = static_cast<short>(next);
            otherChild = son[next];
            son[next] = static_cast<short>(child);
            prnt[otherChild] = static_cast<short>(node);
            if (otherChild < TREE_SIZE)
                prnt[otherChild + 1] = static_cast<short>(node);
            son[node] = static_cast<short>(otherChild);
            node = next;
        }
        node = prnt[node];
    } while (node != 0);
}

VA(0x0047ddf0, 0x1d9)
static void InsertNode(short node)
{
    register unsigned char *key;
    register short compare, i, candidate;
    unsigned short position;

    compare = 1;
    key = &text_buf[node];
    candidate = WINDOW_SIZE + 1 + key[0];
    rson[node] = lson[node] = NIL;
    match_length = 0;
    for (;;) {
        if (compare >= 0) {
            if (rson[candidate] != NIL)
                candidate = rson[candidate];
            else {
                rson[candidate] = static_cast<short>(node);
                dad[node] = static_cast<short>(candidate);
                return;
            }
        } else {
            if (lson[candidate] != NIL)
                candidate = lson[candidate];
            else {
                lson[candidate] = static_cast<short>(node);
                dad[node] = static_cast<short>(candidate);
                return;
            }
        }
        for (i = 1; i < LOOK_AHEAD; ++i) {
            compare = key[i] - text_buf[candidate + i];
            if (compare != 0)
                break;
        }
        if (i > MATCH_THRESHOLD) {
            if (i > match_length) {
                match_position = static_cast<short>(
                    ((node - candidate) & (WINDOW_SIZE - 1)) - 1);
                if ((match_length = i) >= LOOK_AHEAD)
                    break;
            }
            if (match_length == i)
                if (static_cast<int>(position = static_cast<unsigned short>(
                        ((node - candidate) & (WINDOW_SIZE - 1)) - 1)) <
                    match_position)
                    match_position = static_cast<short>(position);
        }
    }
    dad[node] = dad[candidate];
    lson[node] = lson[candidate];
    rson[node] = rson[candidate];
    dad[lson[candidate]] = static_cast<short>(node);
    dad[rson[candidate]] = static_cast<short>(node);
    if (rson[dad[candidate]] == candidate)
        rson[dad[candidate]] = static_cast<short>(node);
    else
        lson[dad[candidate]] = static_cast<short>(node);
    dad[candidate] = NIL;
}

VA(0x0047dfd0, 0x127)
static void DeleteNode(short node)
{
    short replacement;

    if (dad[node] == NIL)
        return;
    if (rson[node] == NIL)
        replacement = lson[node];
    else if (lson[node] == NIL)
        replacement = rson[node];
    else {
        replacement = lson[node];
        if (rson[replacement] != NIL) {
            do {
                replacement = rson[replacement];
            } while (rson[replacement] != NIL);
            rson[dad[replacement]] = lson[replacement];
            dad[lson[replacement]] = dad[replacement];
            lson[replacement] = lson[node];
            dad[lson[node]] = static_cast<short>(replacement);
        }
        rson[replacement] = rson[node];
        dad[rson[node]] = static_cast<short>(replacement);
    }
    dad[replacement] = dad[node];
    if (rson[dad[node]] == node)
        rson[dad[node]] = static_cast<short>(replacement);
    else
        lson[dad[node]] = static_cast<short>(replacement);
    dad[node] = NIL;
}
