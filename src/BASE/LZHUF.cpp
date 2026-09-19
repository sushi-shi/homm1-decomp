// Okumura-style LZSS with adaptive Huffman coding, used for HoMM1's
// multiplayer save transfer.  The tables and stream framing are taken from
// the pinned February 1996 executable.

#include <string.h>

#include <match.h>
#include <BASE/LZHUF.h>

extern "C" void PollSound(void);

enum {
    WINDOW_SIZE = 4096,
    LOOK_AHEAD = 60,
    MATCH_THRESHOLD = 2,
    NIL = WINDOW_SIZE,
    CHARACTER_COUNT = 256 - MATCH_THRESHOLD + LOOK_AHEAD,
    TREE_SIZE = CHARACTER_COUNT * 2 - 1,
    ROOT = TREE_SIZE - 1,
    MAX_FREQUENCY = 0x8000
};

static unsigned char textBuffer[WINDOW_SIZE + LOOK_AHEAD - 1];
static short matchPosition;
static short matchLength;
static short leftChild[WINDOW_SIZE + 1];
static short rightChild[WINDOW_SIZE + 257];
static short parent[WINDOW_SIZE + 1];

static unsigned short frequency[TREE_SIZE + 1];
static short huffmanParent[TREE_SIZE + CHARACTER_COUNT];
static short huffmanChild[TREE_SIZE];

static char *inputCursor;
static char *outputCursor;
static unsigned short inputBits;
static unsigned char inputBitCount;
static unsigned short outputBits;
static unsigned char outputBitCount;
static unsigned long encodedByteCount;

static const unsigned char positionLength[64] = {
    0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

static const unsigned char positionCode[64] = {
    0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
    0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
    0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
    0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
    0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
    0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static const unsigned char decodeCode[256] = {
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

static const unsigned char decodeLength[256] = {
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

static void InitializeHuffman(void)
{
    int i;
    int j;

    for (i = 0; i < CHARACTER_COUNT; ++i) {
        frequency[i] = 1;
        huffmanChild[i] = (short)(i + TREE_SIZE);
        huffmanParent[i + TREE_SIZE] = (short)i;
    }
    i = 0;
    for (j = CHARACTER_COUNT; j <= ROOT; ++j) {
        frequency[j] = (unsigned short)(frequency[i] + frequency[i + 1]);
        huffmanChild[j] = (short)i;
        huffmanParent[i] = huffmanParent[i + 1] = (short)j;
        i += 2;
    }
    frequency[TREE_SIZE] = 0xFFFF;
    huffmanParent[ROOT] = 0;
}

static void InitializeTree(void)
{
    int i;

    for (i = WINDOW_SIZE + 1; i <= WINDOW_SIZE + 256; ++i)
        rightChild[i] = NIL;
    for (i = 0; i < WINDOW_SIZE; ++i)
        parent[i] = NIL;
}

VA(0x0047da60, 0x14f)
static void ReconstructEncoderTree(void)
{
    int i;
    int j;
    int k;
    unsigned short value;

    j = 0;
    for (i = 0; i < TREE_SIZE; ++i) {
        if (huffmanChild[i] >= TREE_SIZE) {
            frequency[j] = (unsigned short)((frequency[i] + 1) / 2);
            huffmanChild[j] = huffmanChild[i];
            ++j;
        }
    }
    for (i = 0, j = CHARACTER_COUNT; j < TREE_SIZE; i += 2, ++j) {
        value = (unsigned short)(frequency[i] + frequency[i + 1]);
        frequency[j] = value;
        for (k = j - 1; value < frequency[k]; --k)
            ;
        ++k;
        memmove(&frequency[k + 1], &frequency[k], (j - k) * sizeof(frequency[0]));
        frequency[k] = value;
        memmove(&huffmanChild[k + 1], &huffmanChild[k],
                (j - k) * sizeof(huffmanChild[0]));
        huffmanChild[k] = (short)i;
    }
    for (i = 0; i < TREE_SIZE; ++i) {
        k = huffmanChild[i];
        if (k >= TREE_SIZE)
            huffmanParent[k] = (short)i;
        else
            huffmanParent[k] = huffmanParent[k + 1] = (short)i;
    }
}

static void UpdateEncoderTree(int character);

static void PutCode(int length, unsigned short code)
{
    outputBits = (unsigned short)(outputBits | (code >> outputBitCount));
    outputBitCount = (unsigned char)(outputBitCount + length);
    if (outputBitCount >= 8) {
        *outputCursor++ = (char)(outputBits >> 8);
        ++encodedByteCount;
        outputBitCount = (unsigned char)(outputBitCount - 8);
        if (outputBitCount >= 8) {
            *outputCursor++ = (char)outputBits;
            ++encodedByteCount;
            outputBitCount = (unsigned char)(outputBitCount - 8);
            outputBits = (unsigned short)(code << (length - outputBitCount));
        } else {
            outputBits = (unsigned short)(outputBits << 8);
        }
    }
}

static void EncodeCharacter(int character)
{
    unsigned short code;
    int length;
    int node;

    code = 0;
    length = 0;
    node = huffmanParent[character + TREE_SIZE];
    do {
        code >>= 1;
        if (node & 1)
            code = (unsigned short)(code + 0x8000);
        ++length;
        node = huffmanParent[node];
    } while (node != ROOT);
    PutCode(length, code);
    UpdateEncoderTree(character);
}

static void EncodePosition(int position)
{
    int upper;

    upper = position >> 6;
    PutCode(positionLength[upper], (unsigned short)(positionCode[upper] << 8));
    PutCode(6, (unsigned short)((position & 0x3F) << 10));
}

VA(0x0047dbb0, 0x23d)
static void UpdateEncoderTree(int character)
{
    int node;
    int next;
    int child;
    int otherChild;
    unsigned short value;

    if (frequency[ROOT] == MAX_FREQUENCY)
        ReconstructEncoderTree();
    node = huffmanParent[character + TREE_SIZE];
    do {
        value = ++frequency[node];
        next = node + 1;
        if (value > frequency[next]) {
            while (value > frequency[++next])
                ;
            --next;
            frequency[node] = frequency[next];
            frequency[next] = value;

            child = huffmanChild[node];
            huffmanParent[child] = (short)next;
            if (child < TREE_SIZE)
                huffmanParent[child + 1] = (short)next;
            otherChild = huffmanChild[next];
            huffmanChild[next] = (short)child;
            huffmanParent[otherChild] = (short)node;
            if (otherChild < TREE_SIZE)
                huffmanParent[otherChild + 1] = (short)node;
            huffmanChild[node] = (short)otherChild;
            node = next;
        }
        node = huffmanParent[node];
    } while (node != 0);
}

VA(0x0047ddf0, 0x1d9)
static void InsertNode(int node)
{
    int compare;
    int i;
    int candidate;
    unsigned char *key;

    compare = 1;
    key = &textBuffer[node];
    candidate = WINDOW_SIZE + 1 + key[0];
    rightChild[node] = leftChild[node] = NIL;
    matchLength = 0;
    for (;;) {
        if (compare >= 0) {
            if (rightChild[candidate] != NIL)
                candidate = rightChild[candidate];
            else {
                rightChild[candidate] = (short)node;
                parent[node] = (short)candidate;
                return;
            }
        } else {
            if (leftChild[candidate] != NIL)
                candidate = leftChild[candidate];
            else {
                leftChild[candidate] = (short)node;
                parent[node] = (short)candidate;
                return;
            }
        }
        for (i = 1; i < LOOK_AHEAD; ++i) {
            compare = key[i] - textBuffer[candidate + i];
            if (compare != 0)
                break;
        }
        if (i > MATCH_THRESHOLD) {
            int position = ((node - candidate) & (WINDOW_SIZE - 1)) - 1;
            if (i > matchLength) {
                matchPosition = (short)position;
                matchLength = (short)i;
                if (i >= LOOK_AHEAD)
                    break;
            } else if (i == matchLength && position < matchPosition) {
                matchPosition = (short)position;
            }
        }
    }
    parent[node] = parent[candidate];
    leftChild[node] = leftChild[candidate];
    rightChild[node] = rightChild[candidate];
    parent[leftChild[candidate]] = (short)node;
    parent[rightChild[candidate]] = (short)node;
    if (rightChild[parent[candidate]] == candidate)
        rightChild[parent[candidate]] = (short)node;
    else
        leftChild[parent[candidate]] = (short)node;
    parent[candidate] = NIL;
}

VA(0x0047dfd0, 0x127)
static void DeleteNode(int node)
{
    int replacement;

    if (parent[node] == NIL)
        return;
    if (rightChild[node] == NIL)
        replacement = leftChild[node];
    else if (leftChild[node] == NIL)
        replacement = rightChild[node];
    else {
        replacement = leftChild[node];
        if (rightChild[replacement] != NIL) {
            do {
                replacement = rightChild[replacement];
            } while (rightChild[replacement] != NIL);
            rightChild[parent[replacement]] = leftChild[replacement];
            parent[leftChild[replacement]] = parent[replacement];
            leftChild[replacement] = leftChild[node];
            parent[leftChild[node]] = (short)replacement;
        }
        rightChild[replacement] = rightChild[node];
        parent[rightChild[node]] = (short)replacement;
    }
    parent[replacement] = parent[node];
    if (rightChild[parent[node]] == node)
        rightChild[parent[node]] = (short)replacement;
    else
        leftChild[parent[node]] = (short)replacement;
    parent[node] = NIL;
}

VA(0x0047d310, 0x743)
long EncodeData(char *destination, char *source, unsigned long sourceLength)
{
    unsigned long consumed;
    int i;
    int length;
    int lastMatchLength;
    int ringStart;
    int ringEnd;
    unsigned char value;

    outputBits = 0;
    outputBitCount = 0;
    encodedByteCount = 0;
    outputCursor = destination;
    *outputCursor++ = (char)(sourceLength >> 24);
    *outputCursor++ = (char)(sourceLength >> 16);
    *outputCursor++ = (char)(sourceLength >> 8);
    *outputCursor++ = (char)sourceLength;

    InitializeHuffman();
    InitializeTree();
    ringStart = 0;
    ringEnd = WINDOW_SIZE - LOOK_AHEAD;
    for (i = ringStart; i < ringEnd; ++i)
        textBuffer[i] = ' ';
    consumed = 0;
    for (length = 0; length < LOOK_AHEAD && consumed < sourceLength; ++length) {
        textBuffer[ringEnd + length] = (unsigned char)*source++;
        ++consumed;
    }
    if (length == 0)
        return 0;

    for (i = 1; i <= LOOK_AHEAD; ++i)
        InsertNode(ringEnd - i);
    InsertNode(ringEnd);
    do {
        if (matchLength > length)
            matchLength = (short)length;
        if (matchLength <= MATCH_THRESHOLD) {
            matchLength = 1;
            EncodeCharacter(textBuffer[ringEnd]);
        } else {
            EncodeCharacter(255 - MATCH_THRESHOLD + matchLength);
            EncodePosition(matchPosition);
        }
        lastMatchLength = matchLength;
        for (i = 0; i < lastMatchLength && consumed < sourceLength; ++i) {
            value = (unsigned char)*source++;
            ++consumed;
            DeleteNode(ringStart);
            textBuffer[ringStart] = value;
            if (ringStart < LOOK_AHEAD - 1)
                textBuffer[ringStart + WINDOW_SIZE] = value;
            ringStart = (ringStart + 1) & (WINDOW_SIZE - 1);
            ringEnd = (ringEnd + 1) & (WINDOW_SIZE - 1);
            InsertNode(ringEnd);
        }
        while (i++ < lastMatchLength) {
            DeleteNode(ringStart);
            ringStart = (ringStart + 1) & (WINDOW_SIZE - 1);
            ringEnd = (ringEnd + 1) & (WINDOW_SIZE - 1);
            if (--length != 0)
                InsertNode(ringEnd);
        }
        PollSound();
    } while (length > 0);

    if (outputBitCount != 0) {
        *outputCursor++ = (char)(outputBits >> 8);
        ++encodedByteCount;
    }
    return (long)encodedByteCount;
}

VA(0x0047fca5, 0x85)
static int GetBit(void)
{
    unsigned int value;

    while (inputBitCount <= 8) {
        value = (unsigned char)*inputCursor++;
        inputBits = (unsigned short)(inputBits | (value << (8 - inputBitCount)));
        inputBitCount = (unsigned char)(inputBitCount + 8);
    }
    value = inputBits;
    inputBits = (unsigned short)(inputBits << 1);
    --inputBitCount;
    return (value & 0x8000) >> 15;
}

static int GetByte(void)
{
    unsigned int value;

    while (inputBitCount <= 8) {
        value = (unsigned char)*inputCursor++;
        inputBits = (unsigned short)(inputBits | (value << (8 - inputBitCount)));
        inputBitCount = (unsigned char)(inputBitCount + 8);
    }
    value = inputBits;
    inputBits = (unsigned short)(inputBits << 8);
    inputBitCount = (unsigned char)(inputBitCount - 8);
    return (value & 0xFF00) >> 8;
}

VA(0x0047fd2a, 0xc9)
static int DecodePosition(void)
{
    int value;
    int position;
    int bits;

    value = GetByte();
    position = decodeCode[value] << 6;
    bits = decodeLength[value] - 2;
    while (bits-- != 0)
        value = (value << 1) + GetBit();
    return position | (value & 0x3F);
}

VA(0x0047ff0c, 0x153)
static void ReconstructDecoderTree(void)
{
    int i;
    int j;
    int k;
    unsigned short value;

    j = 0;
    for (i = 0; i < TREE_SIZE; ++i) {
        if (huffmanChild[i] >= TREE_SIZE) {
            frequency[j] = (unsigned short)((frequency[i] + 1) / 2);
            huffmanChild[j] = huffmanChild[i];
            ++j;
        }
    }
    for (i = 0, j = CHARACTER_COUNT; j < TREE_SIZE; i += 2, ++j) {
        value = (unsigned short)(frequency[i] + frequency[i + 1]);
        frequency[j] = value;
        for (k = j - 1; value < frequency[k]; --k)
            ;
        ++k;
        memmove(&frequency[k + 1], &frequency[k], (j - k) * sizeof(frequency[0]));
        frequency[k] = value;
        memmove(&huffmanChild[k + 1], &huffmanChild[k],
                (j - k) * sizeof(huffmanChild[0]));
        huffmanChild[k] = (short)i;
    }
    for (i = 0; i < TREE_SIZE; ++i) {
        k = huffmanChild[i];
        if (k >= TREE_SIZE)
            huffmanParent[k] = (short)i;
        else
            huffmanParent[k] = huffmanParent[k + 1] = (short)i;
    }
}

VA(0x0047fdf3, 0x119)
static void UpdateDecoderTree(int character)
{
    int node;
    int next;
    int child;
    int otherChild;
    unsigned short value;

    if (frequency[ROOT] == MAX_FREQUENCY)
        ReconstructDecoderTree();
    node = huffmanParent[character + TREE_SIZE];
    do {
        value = ++frequency[node];
        next = node + 1;
        if (value > frequency[next]) {
            while (value > frequency[++next])
                ;
            --next;
            frequency[node] = frequency[next];
            frequency[next] = value;
            child = huffmanChild[node];
            huffmanParent[child] = (short)next;
            if (child < TREE_SIZE)
                huffmanParent[child + 1] = (short)next;
            otherChild = huffmanChild[next];
            huffmanChild[next] = (short)child;
            huffmanParent[otherChild] = (short)node;
            if (otherChild < TREE_SIZE)
                huffmanParent[otherChild + 1] = (short)node;
            huffmanChild[node] = (short)otherChild;
            node = next;
        }
        node = huffmanParent[node];
    } while (node != 0);
}

static int DecodeCharacter(void)
{
    int node;

    node = huffmanChild[ROOT];
    while (node < TREE_SIZE)
        node = huffmanChild[node + GetBit()];
    node -= TREE_SIZE;
    UpdateDecoderTree(node);
    return node;
}

VA(0x0048005f, 0x129)
static void Decode(unsigned long outputLength)
{
    unsigned long count;
    int character;
    int i;
    int j;
    int k;
    int ringPosition;

    for (i = 0; i < WINDOW_SIZE - LOOK_AHEAD; ++i)
        textBuffer[i] = ' ';
    ringPosition = WINDOW_SIZE - LOOK_AHEAD;
    count = 0;
    while (count < outputLength) {
        character = DecodeCharacter();
        if (character < 256) {
            *outputCursor++ = (char)character;
            textBuffer[ringPosition++] = (unsigned char)character;
            ringPosition &= WINDOW_SIZE - 1;
            ++count;
        } else {
            i = (ringPosition - DecodePosition() - 1) & (WINDOW_SIZE - 1);
            j = character - 255 + MATCH_THRESHOLD;
            for (k = 0; k < j && count < outputLength; ++k) {
                character = textBuffer[(i + k) & (WINDOW_SIZE - 1)];
                *outputCursor++ = (char)character;
                textBuffer[ringPosition++] = (unsigned char)character;
                ringPosition &= WINDOW_SIZE - 1;
                ++count;
            }
        }
    }
}

VA(0x0047d250, 0xb9)
long DecodeData(char *destination, char *source)
{
    unsigned long outputLength;

    inputBits = 0;
    inputBitCount = 0;
    inputCursor = source;
    outputLength = (unsigned char)*inputCursor++;
    outputLength = (outputLength << 8) | (unsigned char)*inputCursor++;
    outputLength = (outputLength << 8) | (unsigned char)*inputCursor++;
    outputLength = (outputLength << 8) | (unsigned char)*inputCursor++;
    InitializeHuffman();
    outputCursor = destination;
    Decode(outputLength);
    return (long)outputLength;
}
