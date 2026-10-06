#include <PLATFORM/SmackerDecoder.h>

#include <cstring>
#include <utility>

// The format, as RAD's SMACKW32 reads it: a header (frame count, rate,
// audio tracks, the sizes of the four Huffman trees), each frame's size and
// type, the trees, then the frames. A frame holds an optional palette
// update, a chunk per audio track present, and the video, which is a stream
// of 4x4 blocks coded through the trees. Bits are read least significant
// first.

namespace platform::smacker {

namespace {

constexpr u32 kNode = 0x80000000u;
constexpr int kSmallTreeDepth = 32;
constexpr int kBigTreeDepth = 512;
constexpr u32 kMaxFrames = 0xFFFFFF;
constexpr int kMaxDimension = 4096;
constexpr u32 kMaxAudioChunk = 1u << 24;

enum : u32 {
    kRingFrame = 0x01,
};

enum : u8 {
    kFramePalette = 0x01,
};

enum : u8 {
    kAudioBink = 0x08,
    kAudioDct = 0x04,
    kAudioStereo = 0x10,
    kAudio16Bit = 0x20,
    kAudioPresent = 0x40,
    kAudioPacked = 0x80,
};

enum BlockType { kBlockMono = 0, kBlockFull = 1, kBlockSkip = 2, kBlockFill = 3 };

// The run lengths of the block types' 6-bit run codes.
const u16 kBlockRuns[64] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,  13,  14,  15,   16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,  29,  30,  31,   32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,  45,  46,  47,   48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 128, 256, 512, 1024, 2048};

// The palette's 6-bit components as 8-bit ones.
const u8 kPaletteLevels[64] = {
    0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30,
    0x34, 0x38, 0x3C, 0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D, 0x61, 0x65,
    0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D, 0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A,
    0x9E, 0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE, 0xC3, 0xC7, 0xCB, 0xCF,
    0xD3, 0xD7, 0xDB, 0xDF, 0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF};

u32 ReadU32(const u8* data) {
    return static_cast<u32>(data[0]) | static_cast<u32>(data[1]) << 8
           | static_cast<u32>(data[2]) << 16 | static_cast<u32>(data[3]) << 24;
}

}  // namespace

// Least significant bit first; reading past the end gives zeros and marks
// the reader overrun.
class BitReader {
public:
    BitReader(const u8* data, size_t size) : m_data(data), m_bits(size * 8) {}

    u32 Bit() {
        if (m_position >= m_bits) {
            m_overrun = true;
            return 0;
        }
        u32 bit = (static_cast<u32>(m_data[m_position >> 3]) >> (m_position & 7)) & 1u;
        m_position++;
        return bit;
    }

    u32 Bits(int count) {
        u32 value = 0;
        for (int i = 0; i < count; i++)
            value |= Bit() << i;
        return value;
    }

    bool Overrun() const { return m_overrun; }

private:
    const u8* m_data;
    size_t m_bits;
    size_t m_position = 0;
    bool m_overrun = false;
};

namespace {

// A Huffman tree of bytes: a node holds its children, a leaf its value. A
// tree of a single leaf decodes without reading bits.
struct SmallTree {
    struct Node {
        i32 zero = -1;   // child for a 0 bit, -1 for a leaf
        i32 one = -1;
        u8 value = 0;
    };
    std::vector<Node> nodes;
    int leaves = 0;

    bool Read(BitReader& bits);
    u8 Decode(BitReader& bits) const;

private:
    i32 ReadNode(BitReader& bits, int depth);
};

i32 SmallTree::ReadNode(BitReader& bits, int depth) {
    if (depth > kSmallTreeDepth || nodes.size() > 1024)
        return -1;
    i32 index = static_cast<i32>(nodes.size());
    nodes.emplace_back();
    if (!bits.Bit()) {
        if (++leaves > 256)
            return -1;
        nodes[static_cast<size_t>(index)].value = static_cast<u8>(bits.Bits(8));
        return bits.Overrun() ? -1 : index;
    }
    i32 zero = ReadNode(bits, depth + 1);
    if (zero < 0)
        return -1;
    i32 one = ReadNode(bits, depth + 1);
    if (one < 0)
        return -1;
    nodes[static_cast<size_t>(index)].zero = zero;
    nodes[static_cast<size_t>(index)].one = one;
    return index;
}

bool SmallTree::Read(BitReader& bits) {
    nodes.clear();
    leaves = 0;
    return ReadNode(bits, 0) == 0;
}

u8 SmallTree::Decode(BitReader& bits) const {
    if (nodes.empty())
        return 0;
    const Node* node = &nodes[0];
    while (node->zero >= 0)
        node = &nodes[static_cast<size_t>(bits.Bit() ? node->one : node->zero)];
    return node->value;
}

}  // namespace

namespace {

struct BigTreeReader {
    BitReader& bits;
    const SmallTree* low;
    const SmallTree* high;
    std::array<u32, 3> escapes;
    std::array<i64, 3> last;
    std::vector<u32>& values;
    size_t capacity;

    // Returns the number of entries the subtree takes, or -1.
    i64 Read(int depth) {
        if (depth > kBigTreeDepth || values.size() >= capacity || bits.Overrun())
            return -1;
        if (!bits.Bit()) {
            u32 value = low->Decode(bits) | static_cast<u32>(high->Decode(bits)) << 8;
            for (size_t i = 0; i < 3; i++) {
                if (value == escapes[i]) {
                    last[i] = static_cast<i64>(values.size());
                    value = 0;
                    break;
                }
            }
            values.push_back(value);
            return 1;
        }
        size_t node = values.size();
        values.push_back(0);
        i64 left = Read(depth + 1);
        if (left < 0)
            return -1;
        values[node] = kNode | static_cast<u32>(left);
        i64 right = Read(depth + 1);
        if (right < 0)
            return -1;
        return left + 1 + right;
    }
};

// A value from a big tree, updating its cache of recent values.
u32 DecodeBig(BitReader& bits, std::vector<u32>& values, const std::array<size_t, 3>& last) {
    size_t at = 0;
    while (values[at] & kNode) {
        if (bits.Bit())
            at += values[at] & ~kNode;
        at++;
        if (at >= values.size())
            return 0;
    }
    u32 value = values[at];
    if (value != values[last[0]]) {
        values[last[2]] = values[last[1]];
        values[last[1]] = values[last[0]];
        values[last[0]] = value;
    }
    return value;
}

}  // namespace

bool Decoder::ReadBigTree(BitReader& bits, BigTree& tree, u32 sizeHint) {
    tree.values.clear();
    if (!bits.Bit()) {
        // No tree: every code is 0 and reads no bits.
        tree.values = {0, 0};
        tree.last = {1, 1, 1};
        return true;
    }
    SmallTree bytes[2];
    for (SmallTree& byteTree : bytes) {
        if (!bits.Bit())
            continue;
        if (!byteTree.Read(bits))
            return false;
        bits.Bit();
    }
    BigTreeReader reader{bits, &bytes[0], &bytes[1], {}, {-1, -1, -1}, tree.values, 0};
    for (u32& escape : reader.escapes)
        escape = bits.Bits(16);
    reader.capacity = (static_cast<size_t>(sizeHint) + 3) / 4;
    if (reader.capacity > (1u << 24))
        return false;
    tree.values.reserve(reader.capacity + 3);
    if (reader.Read(0) < 0)
        return false;
    bits.Bit();
    for (size_t i = 0; i < 3; i++) {
        if (reader.last[i] < 0) {
            reader.last[i] = static_cast<i64>(tree.values.size());
            tree.values.push_back(0);
        }
        tree.last[i] = static_cast<size_t>(reader.last[i]);
    }
    return !bits.Overrun();
}

bool Decoder::ReadTrees(const u8* data, size_t size) {
    BitReader bits(data + 16, size - 16);
    BigTree* trees[4] = {&m_mmap, &m_mclr, &m_full, &m_type};
    for (int i = 0; i < 4; i++) {
        if (!ReadBigTree(bits, *trees[i], ReadU32(data + 4 * i)))
            return false;
    }
    return true;
}

bool Decoder::Open(std::vector<u8> file) {
    m_file.clear();
    const size_t headerSize = 104;
    if (file.size() < headerSize)
        return false;
    const u8* header = file.data();
    bool version2 = std::memcmp(header, "SMK2", 4) == 0;
    m_version4 = std::memcmp(header, "SMK4", 4) == 0;
    if (!version2 && !m_version4)
        return false;
    u32 width = ReadU32(header + 4);
    u32 height = ReadU32(header + 8);
    u32 frames = ReadU32(header + 12);
    i32 rate = static_cast<i32>(ReadU32(header + 16));
    u32 flags = ReadU32(header + 20);
    if (width == 0 || height == 0 || width > kMaxDimension || height > kMaxDimension)
        return false;
    // Frame times are milliseconds when positive, units of 10 microseconds
    // when negative; none is 10 frames a second.
    double unitsPerFrame = rate > 0 ? rate * 100.0 : rate < 0 ? -static_cast<double>(rate) : 10000.0;
    m_framesPerSecond = 100000.0 / unitsPerFrame;
    u32 tableFrames = frames + ((flags & kRingFrame) != 0 ? 1 : 0);
    if (frames == 0 || tableFrames > kMaxFrames)
        return false;
    u32 treeSize = ReadU32(header + 52);
    for (int track = 0; track < kAudioTracks; track++) {
        u32 field = ReadU32(header + 72 + 4 * track);
        u8 audioFlags = static_cast<u8>(field >> 24);
        AudioFormat& format = m_audio[static_cast<size_t>(track)];
        format = AudioFormat();
        format.rate = static_cast<int>(field & 0xFFFFFF);
        format.present = format.rate != 0;
        format.compressed = (audioFlags & kAudioPacked) != 0;
        format.sixteenBit = (audioFlags & kAudio16Bit) != 0;
        format.stereo = (audioFlags & kAudioStereo) != 0;
        format.decodable = format.present && (audioFlags & (kAudioBink | kAudioDct)) == 0;
    }
    size_t tables = headerSize + static_cast<size_t>(tableFrames) * 5;
    if (treeSize > file.size() || tables > file.size() - treeSize)
        return false;
    m_frameSizes.resize(tableFrames);
    m_frameTypes.resize(tableFrames);
    for (u32 i = 0; i < tableFrames; i++) {
        m_frameSizes[i] = ReadU32(header + headerSize + 4 * i);
        m_frameTypes[i] = header[headerSize + 4 * tableFrames + i];
    }
    // The four tree sizes (header + 56) followed by the trees themselves.
    std::vector<u8> trees(16 + treeSize);
    std::memcpy(trees.data(), header + 56, 16);
    std::memcpy(trees.data() + 16, header + tables, treeSize);
    m_width = static_cast<int>(width);
    m_height = static_cast<int>(height);
    m_frames = static_cast<int>(frames);
    if (!ReadTrees(trees.data(), trees.size()))
        return false;
    m_firstFrame = tables + treeSize;
    m_file = std::move(file);
    Rewind();
    return true;
}

void Decoder::Rewind() {
    m_position = m_firstFrame;
    m_current = 0;
    m_pixels.assign(static_cast<size_t>(m_width) * static_cast<size_t>(m_height), 0);
    m_palette.fill(0);
}

void Decoder::UpdatePalette(const u8* data, size_t size) {
    std::array<u8, 768> old = m_palette;
    size_t at = 0;
    size_t entry = 0;
    while (entry < 256 && at < size) {
        u8 code = data[at++];
        if (code & 0x80) {
            // Entries kept as they are.
            entry += (code & 0x7F) + 1u;
        } else if (code & 0x40) {
            // A run copied from the previous palette.
            if (at >= size)
                break;
            size_t from = data[at++];
            size_t count = (code & 0x3F) + 1u;
            if (from + count > 256)
                break;
            for (; count > 0 && entry < 256; count--, entry++, from++)
                std::memcpy(&m_palette[entry * 3], &old[from * 3], 3);
        } else {
            // A new colour.
            if (size - at < 2)
                break;
            m_palette[entry * 3] = kPaletteLevels[code & 0x3F];
            m_palette[entry * 3 + 1] = kPaletteLevels[data[at++] & 0x3F];
            m_palette[entry * 3 + 2] = kPaletteLevels[data[at++] & 0x3F];
            entry++;
        }
    }
}

bool Decoder::DecodeAudio(const u8* data, size_t size, const AudioFormat& format, std::vector<u8>& out) {
    if (!format.decodable)
        return true;
    if (!format.compressed) {
        out.insert(out.end(), data, data + size);
        return true;
    }
    if (size < 4)
        return false;
    u32 unpacked = ReadU32(data);
    if (unpacked > kMaxAudioChunk)
        return false;
    BitReader bits(data + 4, size - 4);
    if (!bits.Bit())
        return true;   // a chunk without samples
    bool stereo = bits.Bit() != 0;
    bool sixteenBit = bits.Bit() != 0;
    if (stereo != format.stereo || sixteenBit != format.sixteenBit)
        return false;
    int channels = stereo ? 2 : 1;
    int bytes = sixteenBit ? 2 : 1;
    if (unpacked % static_cast<u32>(channels * bytes) != 0)
        return false;
    SmallTree trees[4];
    for (int i = 0; i < (1 << ((sixteenBit ? 1 : 0) + (stereo ? 1 : 0))); i++) {
        bits.Bit();
        if (!trees[i].Read(bits))
            return false;
        bits.Bit();
    }
    if (bits.Overrun())
        return false;
    size_t start = out.size();
    out.resize(start + unpacked);
    u8* samples = out.data() + start;
    int last = stereo ? 1 : 0;
    if (sixteenBit) {
        u32 count = unpacked / 2;
        u16 predicted[2] = {0, 0};
        for (int channel = last; channel >= 0; channel--) {
            u32 value = bits.Bits(16);
            predicted[channel] = static_cast<u16>((value >> 8) | ((value & 0xFF) << 8));
        }
        u32 i = 0;
        for (; i <= static_cast<u32>(last) && i < count; i++) {
            samples[2 * i] = static_cast<u8>(predicted[i]);
            samples[2 * i + 1] = static_cast<u8>(predicted[i] >> 8);
        }
        for (; i < count; i++) {
            int channel = static_cast<int>(i & static_cast<u32>(last));
            u32 low = trees[2 * channel].Decode(bits);
            u32 high = trees[2 * channel + 1].Decode(bits);
            predicted[channel] = static_cast<u16>(predicted[channel] + (low | high << 8));
            samples[2 * i] = static_cast<u8>(predicted[channel]);
            samples[2 * i + 1] = static_cast<u8>(predicted[channel] >> 8);
        }
    } else {
        u8 predicted[2] = {0, 0};
        for (int channel = last; channel >= 0; channel--)
            predicted[channel] = static_cast<u8>(bits.Bits(8));
        u32 i = 0;
        for (; i <= static_cast<u32>(last) && i < unpacked; i++)
            samples[i] = predicted[i];
        for (; i < unpacked; i++) {
            int channel = static_cast<int>(i & static_cast<u32>(last));
            predicted[channel] = static_cast<u8>(predicted[channel] + trees[channel].Decode(bits));
            samples[i] = predicted[channel];
        }
    }
    if (bits.Overrun()) {
        out.resize(start);
        return false;
    }
    return true;
}

bool Decoder::DecodeVideo(const u8* data, size_t size) {
    BitReader bits(data, size);
    BigTree* trees[4] = {&m_mmap, &m_mclr, &m_full, &m_type};
    // The recent-value caches start each frame at zero.
    for (BigTree* tree : trees) {
        for (size_t slot : tree->last)
            tree->values[slot] = 0;
    }
    const size_t stride = static_cast<size_t>(m_width);
    const int blocksWide = m_width / 4;
    const int blocks = blocksWide * (m_height / 4);
    int block = 0;
    auto at = [&](int index) {
        return m_pixels.data() + static_cast<size_t>(index / blocksWide) * stride * 4
               + static_cast<size_t>(index % blocksWide) * 4;
    };
    auto put16 = [](u8* out, u32 value) {
        out[0] = static_cast<u8>(value);
        out[1] = static_cast<u8>(value >> 8);
    };
    while (block < blocks) {
        if (bits.Overrun())
            return false;
        u32 type = DecodeBig(bits, m_type.values, m_type.last);
        int run = kBlockRuns[(type >> 2) & 0x3F];
        switch (type & 3) {
            case kBlockMono:
                for (; run > 0 && block < blocks; run--, block++) {
                    u32 colours = DecodeBig(bits, m_mclr.values, m_mclr.last);
                    u32 map = DecodeBig(bits, m_mmap.values, m_mmap.last);
                    u8 high = static_cast<u8>(colours >> 8);
                    u8 low = static_cast<u8>(colours);
                    u8* out = at(block);
                    for (int row = 0; row < 4; row++, out += stride, map >>= 4) {
                        for (int column = 0; column < 4; column++)
                            out[column] = (map >> column) & 1 ? high : low;
                    }
                }
                break;
            case kBlockFull: {
                int mode = 0;
                if (m_version4) {
                    if (bits.Bit())
                        mode = 1;
                    else if (bits.Bit())
                        mode = 2;
                }
                for (; run > 0 && block < blocks; run--, block++) {
                    u8* out = at(block);
                    if (mode == 0) {
                        for (int row = 0; row < 4; row++, out += stride) {
                            put16(out + 2, DecodeBig(bits, m_full.values, m_full.last));
                            put16(out, DecodeBig(bits, m_full.values, m_full.last));
                        }
                    } else if (mode == 1) {
                        // Each 2x2 square one colour.
                        for (int half = 0; half < 2; half++) {
                            u32 pair = DecodeBig(bits, m_full.values, m_full.last);
                            for (int row = 0; row < 2; row++, out += stride) {
                                out[0] = out[1] = static_cast<u8>(pair);
                                out[2] = out[3] = static_cast<u8>(pair >> 8);
                            }
                        }
                    } else {
                        // Each row twice.
                        for (int half = 0; half < 2; half++) {
                            u32 right = DecodeBig(bits, m_full.values, m_full.last);
                            u32 left = DecodeBig(bits, m_full.values, m_full.last);
                            for (int row = 0; row < 2; row++, out += stride) {
                                put16(out, left);
                                put16(out + 2, right);
                            }
                        }
                    }
                }
                break;
            }
            case kBlockSkip:
                for (; run > 0 && block < blocks; run--)
                    block++;
                break;
            case kBlockFill: {
                u8 colour = static_cast<u8>(type >> 8);
                for (; run > 0 && block < blocks; run--, block++) {
                    u8* out = at(block);
                    for (int row = 0; row < 4; row++, out += stride)
                        std::memset(out, colour, 4);
                }
                break;
            }
        }
    }
    return true;
}

bool Decoder::DecodeFrame(int audioTrack, std::vector<u8>* audio) {
    if (m_file.empty() || m_current >= m_frames)
        return false;
    size_t size = m_frameSizes[static_cast<size_t>(m_current)] & ~3u;
    u8 type = m_frameTypes[static_cast<size_t>(m_current)];
    if (m_position > m_file.size() || size > m_file.size() - m_position)
        return false;
    const u8* frame = m_file.data() + m_position;
    m_position += size;
    m_current++;
    size_t used = 0;
    if (type & kFramePalette) {
        if (size == 0)
            return false;
        size_t paletteSize = static_cast<size_t>(frame[0]) * 4;
        if (paletteSize == 0 || paletteSize > size)
            return false;
        UpdatePalette(frame + 1, paletteSize - 1);
        used = paletteSize;
    }
    for (int track = 0; track < kAudioTracks; track++) {
        if ((type & (2 << track)) == 0)
            continue;
        if (size - used < 4)
            return false;
        size_t length = ReadU32(frame + used);
        if (length < 4 || length > size - used)
            return false;
        if (track == audioTrack && audio != nullptr
            && !DecodeAudio(frame + used + 4, length - 4, m_audio[static_cast<size_t>(track)], *audio))
            return false;
        used += length;
    }
    return DecodeVideo(frame + used, size - used);
}

}  // namespace platform::smacker
