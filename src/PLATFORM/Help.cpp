// WinHelp 3.1/4.0 to HTML (include/PLATFORM/Help.h).
//
// The format is Microsoft's undocumented .HLP layout as described by the
// helpdeco project's notes (M. Winterhoff, "Windows Help file format"): an
// internal file system whose directory is a B+ tree; |SYSTEM (version,
// compression, title, language); |TOPIC, a chain of TOPICLINK records in 4 KB
// blocks, each LZ77-compressed, whose text is phrase-compressed against
// |Phrases (3.1) or |PhrIndex and |PhrImage (4.0, "Hall" compression);
// |CONTEXT (context-string hash to topic offset), |TTLBTREE (titles), |FONT,
// |KWBTREE and |KWDATA (the keyword index) and |bmN pictures (SHG/MRB).
// WinHelp 3.0 files (version 15), which lay their topics out differently, are
// refused.
//
// Everything read from the file is bounds-checked: the help file is user
// data. A malformed part is skipped and counted in the report; only a file
// whose structure cannot be read at all fails.

#include <PLATFORM/Help.h>

#include <PLATFORM/Platform.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace platform::help {

namespace {

using Bytes = std::vector<u8>;

constexpr u32 kHelpMagic = 0x00035F3F;
constexpr u16 kBtreeMagic = 0x293B;
constexpr u16 kSystemMagic = 0x036C;
constexpr size_t kBtreeHeaderSize = 38;
constexpr size_t kTopicBlockHeaderSize = 12;
constexpr size_t kTopicLinkSize = 21;
constexpr u32 kTopicPositionBlockShift = 14;
constexpr u32 kTopicPositionMask = 0x3FFF;
constexpr i32 kTopicOffsetBlockSize = 0x8000;
constexpr size_t kMaxFileSize = 64u << 20;
// A link written before its target topic is known (see ResolveLinks).
constexpr char kLinkOpen[] = "<!--link:";
constexpr char kLinkClose[] = "-->";
constexpr size_t kLinkOpenLength = sizeof(kLinkOpen) - 1;
constexpr size_t kLinkCloseLength = sizeof(kLinkClose) - 1;
constexpr size_t kMaxRecordText = 1u << 20;
constexpr size_t kMaxPictureBytes = 64u << 20;

enum RecordType : u8 {
    RECORD_TOPIC_HEADER = 0x02,
    RECORD_DISPLAY = 0x20,
    RECORD_TABLE = 0x23
};

// ---------------------------------------------------------------- reading

// A bounds-checked little-endian cursor. Reading past the end marks the
// reader failed and returns zeros.
class Reader {
public:
    Reader() = default;
    Reader(const u8* data, size_t size) : m_data(data), m_size(size) {}

    bool Ok() const { return !m_failed; }
    size_t Position() const { return m_position; }
    size_t Size() const { return m_size; }
    size_t Remaining() const { return m_failed ? 0 : m_size - m_position; }
    // Never null, so that empty ranges can be copied from a failed reader.
    const u8* Here() const {
        static const u8 kNothing = 0;
        return m_data != nullptr ? m_data + m_position : &kNothing;
    }
    bool AtEnd() const { return m_failed || m_position >= m_size; }

    bool Seek(size_t position) {
        if (m_failed || position > m_size) {
            m_failed = true;
            return false;
        }
        m_position = position;
        return true;
    }
    bool Skip(size_t count) {
        if (!Need(count))
            return false;
        m_position += count;
        return true;
    }
    u8 Peek() const { return AtEnd() ? 0 : m_data[m_position]; }
    u8 U8() {
        if (!Need(1))
            return 0;
        return m_data[m_position++];
    }
    u16 U16() {
        if (!Need(2))
            return 0;
        u16 value = static_cast<u16>(m_data[m_position] | (m_data[m_position + 1] << 8));
        m_position += 2;
        return value;
    }
    u32 U32() {
        if (!Need(4))
            return 0;
        u32 value = 0;
        for (size_t i = 0; i < 4; i++)
            value |= static_cast<u32>(m_data[m_position + i]) << (8 * i);
        m_position += 4;
        return value;
    }
    i16 I16() { return static_cast<i16>(U16()); }
    i32 I32() { return static_cast<i32>(U32()); }

    // WinHelp's variable-length numbers: when the first byte's low bit is
    // set the long form is used; the value is the number shifted right once,
    // and the signed forms are biased by half the range.
    u16 CompressedU16() {
        return (Peek() & 1) != 0 ? static_cast<u16>(U16() >> 1) : static_cast<u16>(U8() >> 1);
    }
    i16 CompressedI16() {
        if ((Peek() & 1) != 0)
            return static_cast<i16>(static_cast<i32>(U16() >> 1) - 0x4000);
        return static_cast<i16>(static_cast<i32>(U8() >> 1) - 0x40);
    }
    u32 CompressedU32() {
        return (Peek() & 1) != 0 ? U32() >> 1 : static_cast<u32>(U16() >> 1);
    }
    i32 CompressedI32() {
        if ((Peek() & 1) != 0)
            return static_cast<i32>(U32() >> 1) - 0x40000000;
        return static_cast<i32>(U16() >> 1) - 0x4000;
    }

    // A NUL-terminated string; a string that runs to the end is taken whole.
    std::string CString() {
        std::string text;
        while (!AtEnd()) {
            u8 c = m_data[m_position++];
            if (c == 0)
                break;
            text += static_cast<char>(c);
        }
        return text;
    }

    // The next count bytes as their own reader.
    Reader Sub(size_t count) {
        if (!Need(count)) {
            Reader failed;
            failed.m_failed = true;
            return failed;
        }
        Reader sub(m_data + m_position, count);
        m_position += count;
        return sub;
    }

private:
    bool Need(size_t count) {
        if (m_failed || count > m_size - m_position) {
            m_failed = true;
            return false;
        }
        return true;
    }

    const u8* m_data = nullptr;
    size_t m_size = 0;
    size_t m_position = 0;
    bool m_failed = false;
};

struct Span {
    const u8* data = nullptr;
    size_t size = 0;
};

Reader Read(Span span) {
    return Reader(span.data, span.size);
}

// WinHelp's LZ77: a flag byte announces eight items, each a literal byte
// (flag bit clear) or a 16-bit reference (12-bit distance - 1, 4-bit length
// - 3) into the output so far. limit caps the output.
Bytes DecompressLz77(const u8* data, size_t size, size_t limit) {
    Bytes out;
    size_t in = 0;
    while (in < size && out.size() < limit) {
        u8 flags = data[in++];
        for (int bit = 0; bit < 8 && in < size && out.size() < limit; bit++, flags >>= 1) {
            if ((flags & 1) == 0) {
                out.push_back(data[in++]);
                continue;
            }
            if (in + 1 >= size)
                return out;
            u32 word = static_cast<u32>(data[in] | (data[in + 1] << 8));
            in += 2;
            size_t length = (word >> 12) + 3;
            size_t distance = (word & 0x0FFF) + 1;
            if (distance > out.size())
                return out;
            for (size_t i = 0; i < length && out.size() < limit; i++)
                out.push_back(out[out.size() - distance]);
        }
    }
    return out;
}

// The pictures' run-length coding: a count byte with the top bit set copies
// that many (low 7 bits) literal bytes, otherwise repeats the next byte.
Bytes DecompressRle(const Bytes& data, size_t limit) {
    Bytes out;
    size_t in = 0;
    while (in < data.size() && out.size() < limit) {
        u8 count = data[in++];
        size_t n = count & 0x7F;
        if ((count & 0x80) != 0) {
            for (size_t i = 0; i < n && in < data.size() && out.size() < limit; i++)
                out.push_back(data[in++]);
        } else {
            if (in >= data.size())
                break;
            u8 value = data[in++];
            for (size_t i = 0; i < n && out.size() < limit; i++)
                out.push_back(value);
        }
    }
    return out;
}

// ---------------------------------------------------------------- file system

// Calls entry once per leaf entry of a B+ tree, with the reader at the entry;
// entry reads exactly one entry and returns false to stop. Returns false when
// the tree's structure is malformed.
bool ForEachLeafEntry(Span tree, const std::function<bool(Reader&)>& entry) {
    Reader header = Read(tree);
    u16 magic = header.U16();
    header.U16();  // flags
    u16 pageSize = header.U16();
    header.Skip(16);  // structure
    header.I16();     // must be zero
    header.I16();     // page splits
    i16 rootPage = header.I16();
    header.I16();  // must be -1
    i16 totalPages = header.I16();
    i16 levels = header.I16();
    header.I32();  // total entries
    if (!header.Ok() || magic != kBtreeMagic || pageSize < 8 || totalPages <= 0 || levels <= 0
        || levels > 16 || rootPage < 0 || rootPage >= totalPages)
        return false;
    auto page = [&](i32 number, Reader& reader) {
        if (number < 0 || number >= totalPages)
            return false;
        size_t start = kBtreeHeaderSize + static_cast<size_t>(number) * pageSize;
        if (start > tree.size || pageSize > tree.size - start)
            return false;
        reader = Reader(tree.data + start, pageSize);
        return true;
    };
    i32 current = rootPage;
    for (i16 level = 1; level < levels; level++) {
        Reader index;
        if (!page(current, index))
            return false;
        index.U16();  // unused
        index.I16();  // entries
        current = index.I16();  // the first child
    }
    std::set<i32> visited;
    while (current != -1) {
        Reader leaf;
        if (!visited.insert(current).second || !page(current, leaf))
            return false;
        leaf.U16();  // unused
        i16 entries = leaf.I16();
        leaf.I16();  // previous page
        i16 next = leaf.I16();
        for (i16 i = 0; i < entries; i++) {
            if (!entry(leaf))
                return true;
            if (!leaf.Ok())
                return false;
        }
        current = next;
    }
    return true;
}

class HelpFile {
public:
    bool Open(const Bytes& bytes, std::string& error) {
        m_bytes = &bytes;
        Reader header(bytes.data(), bytes.size());
        u32 magic = header.U32();
        i32 directory = header.I32();
        if (!header.Ok() || magic != kHelpMagic) {
            error = "not a WinHelp file";
            return false;
        }
        Span tree;
        if (!Internal(directory, tree)) {
            error = "the help file's directory is outside the file";
            return false;
        }
        bool ok = ForEachLeafEntry(tree, [&](Reader& reader) {
            std::string name = reader.CString();
            i32 offset = reader.I32();
            Span file;
            if (reader.Ok() && Internal(offset, file))
                m_files[name] = file;
            return true;
        });
        if (!ok || m_files.empty()) {
            error = "the help file's directory is damaged";
            return false;
        }
        return true;
    }

    bool Find(const char* name, Span& span) const {
        auto found = m_files.find(name);
        if (found == m_files.end())
            return false;
        span = found->second;
        return true;
    }

private:
    // An internal file: reserved and used sizes, a flag byte, then the data.
    bool Internal(i32 offset, Span& span) const {
        if (offset < 0)
            return false;
        Reader reader(m_bytes->data(), m_bytes->size());
        if (!reader.Seek(static_cast<size_t>(offset)))
            return false;
        reader.I32();  // reserved space
        i32 used = reader.I32();
        reader.U8();  // flags
        if (!reader.Ok() || used < 0)
            return false;
        span.data = reader.Here();
        span.size = std::min(static_cast<size_t>(used), reader.Remaining());
        return true;
    }

    const Bytes* m_bytes = nullptr;
    std::map<std::string, Span> m_files;
};

// ---------------------------------------------------------------- text

std::string Escape(const std::string& utf8) {
    std::string out;
    out.reserve(utf8.size());
    for (char c : utf8) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            default:
                if (static_cast<u8>(c) >= 0x20 || c == '\t' || c == '\n')
                    out += c;
                break;
        }
    }
    return out;
}

std::string Base64(const Bytes& data) {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        u32 chunk = static_cast<u32>(data[i]) << 16;
        if (i + 1 < data.size())
            chunk |= static_cast<u32>(data[i + 1]) << 8;
        if (i + 2 < data.size())
            chunk |= data[i + 2];
        out += kAlphabet[(chunk >> 18) & 63];
        out += kAlphabet[(chunk >> 12) & 63];
        out += i + 1 < data.size() ? kAlphabet[(chunk >> 6) & 63] : '=';
        out += i + 2 < data.size() ? kAlphabet[chunk & 63] : '=';
    }
    return out;
}

// The WinHelp context-string hash (the key of |CONTEXT).
u32 ContextHash(const std::string& context) {
    if (context.empty())
        return 1;
    u32 hash = 0;
    for (char c : context) {
        u8 byte = static_cast<u8>(c);
        i32 value;
        if (byte == '!')
            value = 0x0B;
        else if (byte == '.')
            value = 0x0C;
        else if (byte == '_')
            value = 0x0D;
        else if (byte == '0')
            value = 0x0A;
        else if (byte >= 'a' && byte <= 'z')
            value = byte - 0x50;
        else
            value = byte - 0x30;
        hash = hash * 43u + static_cast<u32>(value);
    }
    return hash;
}

// ---------------------------------------------------------------- pictures

void PutU16(Bytes& out, u32 value) {
    out.push_back(static_cast<u8>(value));
    out.push_back(static_cast<u8>(value >> 8));
}

void PutU32(Bytes& out, u32 value) {
    PutU16(out, value & 0xFFFF);
    PutU16(out, value >> 16);
}

// Decodes one picture of an SHG/MRB container into a BMP file. Device-
// dependent (type 5) and device-independent (type 6) bitmaps are supported;
// metafiles are not.
bool DecodePicture(Span container, Bytes& bmp) {
    Reader file = Read(container);
    u16 magic = file.U16();
    u16 count = file.U16();
    if (!file.Ok() || (magic != 0x506C && magic != 0x706C))
        return false;
    for (u16 index = 0; index < count; index++) {
        Reader offsets = Read(container);
        offsets.Seek(4 + 4u * index);
        u32 start = offsets.U32();
        Reader picture = Read(container);
        if (!offsets.Ok() || !picture.Seek(start))
            return false;
        u8 type = picture.U8();
        u8 packing = picture.U8();
        if (type != 5 && type != 6)
            continue;
        picture.CompressedU32();  // horizontal resolution
        picture.CompressedU32();  // vertical resolution
        picture.CompressedU16();  // planes
        u32 bitCount = picture.CompressedU16();
        u32 width = picture.CompressedU32();
        u32 height = picture.CompressedU32();
        u32 colorsUsed = picture.CompressedU32();
        picture.CompressedU32();  // important colors
        u32 dataSize = picture.CompressedU32();
        picture.CompressedU32();  // hotspot size
        u32 dataOffset = picture.U32();
        picture.U32();  // hotspot offset
        if (!picture.Ok() || width == 0 || height == 0 || width > 8192 || height > 8192
            || (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24))
            return false;
        u32 paletteSize = 0;
        if (bitCount <= 8)
            paletteSize = colorsUsed != 0 && colorsUsed <= (1u << bitCount) ? colorsUsed
                                                                          : 1u << bitCount;
        Bytes palette;
        if (type == 6) {
            for (u32 i = 0; i < paletteSize * 4; i++)
                palette.push_back(picture.U8());
        } else if (bitCount == 1) {
            // A monochrome device bitmap: 0 is black, 1 white.
            palette = {0, 0, 0, 0, 255, 255, 255, 0};
            paletteSize = 2;
        } else if (bitCount <= 8) {
            return false;  // the device's palette is unknown
        }
        Reader data = Read(container);
        if (!picture.Ok() || !data.Seek(static_cast<size_t>(start) + dataOffset))
            return false;
        Reader packed = data.Sub(dataSize);
        if (!packed.Ok())
            return false;
        // Device bitmaps pad rows to 16 bits and run top-down; DIBs pad to
        // 32 bits and run bottom-up, like the BMP written here.
        size_t rowBits = static_cast<size_t>(width) * bitCount;
        size_t sourceStride = type == 5 ? (rowBits + 15) / 16 * 2 : (rowBits + 31) / 32 * 4;
        size_t stride = (rowBits + 31) / 32 * 4;
        size_t needed = sourceStride * height;
        if (needed > kMaxPictureBytes)
            return false;
        if (dataSize == 0)
            return false;
        Bytes pixels(packed.Here(), packed.Here() + dataSize);
        if (packing == 2 || packing == 3)
            pixels = DecompressLz77(pixels.data(), pixels.size(), kMaxPictureBytes);
        if (packing == 1 || packing == 3)
            pixels = DecompressRle(pixels, needed);
        if (packing > 3 || pixels.size() < needed)
            return false;

        u32 headerSize = 14 + 40 + paletteSize * 4;
        u32 imageSize = static_cast<u32>(stride * height);
        bmp.clear();
        bmp.push_back('B');
        bmp.push_back('M');
        PutU32(bmp, headerSize + imageSize);
        PutU32(bmp, 0);
        PutU32(bmp, headerSize);
        PutU32(bmp, 40);
        PutU32(bmp, width);
        PutU32(bmp, height);
        PutU16(bmp, 1);
        PutU16(bmp, bitCount);
        PutU32(bmp, 0);
        PutU32(bmp, imageSize);
        PutU32(bmp, 2835);
        PutU32(bmp, 2835);
        PutU32(bmp, paletteSize);
        PutU32(bmp, 0);
        bmp.insert(bmp.end(), palette.begin(), palette.end());
        size_t copy = std::min(stride, sourceStride);
        for (u32 row = 0; row < height; row++) {
            size_t source = type == 5 ? (height - 1 - row) * sourceStride : row * sourceStride;
            bmp.insert(bmp.end(), pixels.begin() + static_cast<std::ptrdiff_t>(source),
                       pixels.begin() + static_cast<std::ptrdiff_t>(source + copy));
            bmp.insert(bmp.end(), stride - copy, 0);
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- conversion

struct Font {
    std::string css;
};

struct Topic {
    i32 offset = 0;
    std::string title;  // UTF-8
    std::string body;   // HTML
    i32 browseBack = -1;
    i32 browseForward = -1;
};

struct ContentsEntry {
    int level = 1;
    std::string title;    // UTF-8
    std::string anchor;   // empty: a heading or an unresolved link
    bool heading = true;
};

class Converter {
public:
    Converter(const Bytes& bytes, Report& report, const Labels& labels)
        : m_bytes(bytes), m_report(report), m_labels(labels) {}

    bool Run(const std::string& contents, int codepage, std::string& html, std::string& error) {
        if (!m_file.Open(m_bytes, error))
            return false;
        if (!ReadSystem(error))
            return false;
        m_codepage = codepage != 0 ? codepage : m_codepage;
        m_report.codepage = m_codepage;
        if (!ReadPhrases(error))
            return false;
        ReadFonts();
        ReadContexts();
        if (!ReadTopics(error))
            return false;
        ReadKeywords();
        ReadContentsFile(contents);
        Write(html);
        return true;
    }

private:
    std::string Text(const std::string& raw) const { return ToUtf8(raw, m_codepage); }

    // ------------------------------------------------------------ |SYSTEM

    bool ReadSystem(std::string& error) {
        Span system;
        if (!m_file.Find("|SYSTEM", system)) {
            error = "the help file has no |SYSTEM";
            return false;
        }
        Reader reader = Read(system);
        u16 magic = reader.U16();
        m_version = reader.U16();
        reader.U16();  // major
        reader.I32();  // generation date
        u16 flags = reader.U16();
        m_report.version = m_version;
        if (!reader.Ok() || magic != kSystemMagic) {
            error = "the help file's |SYSTEM is damaged";
            return false;
        }
        if (m_version <= 16) {
            error = "WinHelp 3.0 files are not supported";
            return false;
        }
        m_compressed = flags == 4 || flags == 8;
        m_blockSize = flags == 8 ? 2048 : 4096;
        int charsetPage = 0;
        int languagePage = 0;
        while (reader.Remaining() >= 4) {
            u16 type = reader.U16();
            u16 size = reader.U16();
            Reader record = reader.Sub(size);
            if (!reader.Ok())
                break;
            switch (type) {
                case 1:
                    m_title = record.CString();
                    break;
                case 9:  // language: four unknown words, then the LCID
                    if (record.Size() >= 10) {
                        record.Skip(8);
                        u16 language = record.U16() & 0x3FF;
                        // Russian, Ukrainian, Belarusian, Bulgarian, Macedonian.
                        bool cyrillic = language == 0x19 || language == 0x22 || language == 0x23
                                        || language == 0x02 || language == 0x2F;
                        languagePage = cyrillic ? 1251 : 1252;
                    }
                    break;
                case 11:  // character set
                    if (record.Size() >= 1) {
                        u8 charset = record.U8();
                        if (charset == 204)
                            charsetPage = 1251;
                        else if (charset == 0)
                            charsetPage = 1252;
                    }
                    break;
                default:
                    break;
            }
        }
        m_codepage = charsetPage != 0 ? charsetPage : languagePage != 0 ? languagePage : 1252;
        return true;
    }

    // ------------------------------------------------------------ phrases

    bool ReadPhrases(std::string& error) {
        Span index;
        Span image;
        if (m_file.Find("|PhrIndex", index) && m_file.Find("|PhrImage", image))
            return ReadHallPhrases(index, image, error);
        Span phrases;
        if (m_file.Find("|Phrases", phrases))
            return ReadOldPhrases(phrases, error);
        return true;
    }

    bool ReadHallPhrases(Span index, Span image, std::string& error) {
        Reader reader = Read(index);
        reader.I32();  // always 1
        i32 entries = reader.I32();
        reader.I32();  // compressed size of the bit stream
        i32 imageSize = reader.I32();
        i32 imageCompressedSize = reader.I32();
        reader.I32();  // always 0
        u16 bits = reader.U16() & 0x0F;
        reader.U16();
        if (!reader.Ok() || entries < 0 || entries > 0x10000 || imageSize < 0) {
            error = "the help file's phrase index is damaged";
            return false;
        }
        // Each phrase's length in a bit stream of 32-bit little-endian words,
        // least significant bit first.
        u32 word = 0;
        u32 mask = 0;
        auto bit = [&]() {
            mask <<= 1;
            if (mask == 0) {
                word = reader.U32();
                mask = 1;
            }
            return (word & mask) != 0;
        };
        std::vector<size_t> offsets{0};
        for (i32 i = 0; i < entries && reader.Ok(); i++) {
            size_t length = 1;
            while (bit() && reader.Ok())
                length += static_cast<size_t>(1) << bits;
            for (u16 b = 0; b < bits && b < 5; b++) {
                if (bit())
                    length += static_cast<size_t>(1) << b;
            }
            offsets.push_back(offsets.back() + length);
        }
        if (!reader.Ok()) {
            error = "the help file's phrase index is damaged";
            return false;
        }
        Bytes text;
        if (imageSize == imageCompressedSize)
            text.assign(image.data, image.data + std::min(image.size, static_cast<size_t>(imageSize)));
        else
            text = DecompressLz77(image.data, image.size, static_cast<size_t>(imageSize));
        for (size_t i = 0; i + 1 < offsets.size(); i++) {
            size_t from = std::min(offsets[i], text.size());
            size_t to = std::min(offsets[i + 1], text.size());
            m_phrases.emplace_back(text.begin() + static_cast<std::ptrdiff_t>(from),
                                   text.begin() + static_cast<std::ptrdiff_t>(to));
        }
        m_hall = true;
        return true;
    }

    bool ReadOldPhrases(Span phrases, std::string& error) {
        Reader reader = Read(phrases);
        u16 count = reader.U16();
        reader.U16();  // 0x0100
        i32 size = m_compressed ? reader.I32() : -1;
        std::vector<u16> offsets;
        for (u32 i = 0; i <= count; i++)
            offsets.push_back(reader.U16());
        if (!reader.Ok() || offsets.empty()) {
            error = "the help file's phrase table is damaged";
            return false;
        }
        Bytes text;
        if (size >= 0)
            text = DecompressLz77(reader.Here(), reader.Remaining(), static_cast<size_t>(size));
        else
            text.assign(reader.Here(), reader.Here() + reader.Remaining());
        // Offsets count from the start of the offset table.
        size_t base = offsets[0];
        for (size_t i = 0; i + 1 < offsets.size(); i++) {
            size_t from = offsets[i] >= base ? std::min<size_t>(offsets[i] - base, text.size()) : 0;
            size_t to = offsets[i + 1] >= base
                            ? std::min<size_t>(offsets[i + 1] - base, text.size())
                            : 0;
            m_phrases.emplace_back(text.begin() + static_cast<std::ptrdiff_t>(from),
                                   text.begin() + static_cast<std::ptrdiff_t>(std::max(from, to)));
        }
        return true;
    }

    void AppendPhrase(Bytes& out, size_t index) const {
        if (index < m_phrases.size())
            out.insert(out.end(), m_phrases[index].begin(), m_phrases[index].end());
    }

    // A record's text, phrase-compressed when it is shorter than its length.
    Bytes Expand(Reader raw, size_t length) const {
        length = std::min(length, kMaxRecordText);
        Bytes out;
        if (length <= raw.Remaining() || m_phrases.empty()) {
            out.assign(raw.Here(), raw.Here() + std::min(length, raw.Remaining()));
            return out;
        }
        while (!raw.AtEnd() && out.size() < length) {
            u8 c = raw.U8();
            if (m_hall) {
                if ((c & 1) == 0) {
                    AppendPhrase(out, c / 2u);
                } else if ((c & 3) == 1) {
                    AppendPhrase(out, 128u + (c / 4u) * 256u + raw.U8());
                } else if ((c & 7) == 3) {
                    size_t n = c / 8u + 1;
                    for (size_t i = 0; i < n && !raw.AtEnd(); i++)
                        out.push_back(raw.U8());
                } else if ((c & 15) == 7) {
                    out.insert(out.end(), c / 16u + 1, ' ');
                } else {
                    out.insert(out.end(), c / 16u + 1, 0);
                }
            } else if (c > 0 && c < 16) {
                u32 code = ((c - 1u) << 8) | raw.U8();
                AppendPhrase(out, code >> 1);
                if ((code & 1) != 0)
                    out.push_back(' ');
            } else {
                out.push_back(c);
            }
        }
        out.resize(std::min(out.size(), length));
        return out;
    }

    // ------------------------------------------------------------ fonts

    void ReadFonts() {
        Span span;
        if (!m_file.Find("|FONT", span))
            return;
        Reader reader = Read(span);
        u16 faceCount = reader.U16();
        u16 descriptorCount = reader.U16();
        u16 facesOffset = reader.U16();
        u16 descriptorsOffset = reader.U16();
        size_t end = span.size;
        if (facesOffset >= 12) {
            reader.U16();  // styles
            u16 stylesOffset = reader.U16();
            if (stylesOffset > descriptorsOffset)
                end = std::min<size_t>(end, stylesOffset);
        }
        if (!reader.Ok() || faceCount == 0 || descriptorCount == 0
            || descriptorsOffset <= facesOffset || descriptorsOffset > end)
            return;
        size_t faceLength = static_cast<size_t>(descriptorsOffset - facesOffset) / faceCount;
        size_t descriptorSize = (end - descriptorsOffset) / descriptorCount;
        if (faceLength == 0 || descriptorSize != 11)
            return;  // only the 3.1 descriptors carry the styles read here
        std::vector<std::string> faces;
        for (u16 i = 0; i < faceCount; i++) {
            Reader face = Read(span);
            face.Seek(facesOffset + i * faceLength);
            Reader name = face.Sub(faceLength);
            faces.push_back(name.CString());
        }
        static const char* const kFamilies[] = {
            "sans-serif", "monospace", "serif", "sans-serif", "cursive", "fantasy"};
        for (u16 i = 0; i < descriptorCount; i++) {
            Reader descriptor = Read(span);
            descriptor.Seek(descriptorsOffset + i * descriptorSize);
            u8 attributes = descriptor.U8();
            u8 halfPoints = descriptor.U8();
            u8 family = descriptor.U8();
            u16 face = descriptor.U16();
            if (!descriptor.Ok())
                break;
            std::string css;
            std::string decoration;
            if (face < faces.size()) {
                std::string name;
                for (char c : faces[face]) {
                    if (std::isalnum(static_cast<u8>(c)) != 0 || c == ' ' || c == '-')
                        name += c;
                }
                if (!name.empty())
                    css += "font-family:'" + name + "'," + kFamilies[family < 6 ? family : 0] + ";";
            }
            // Sizes are in half points; scaled by 1.25 for a browser page,
            // which is read at a larger size than a 1995 help window.
            if (halfPoints > 0)
                css += "font-size:" + std::to_string(halfPoints * 5 / 8)
                       + (halfPoints * 5 % 8 != 0 ? "." + std::to_string(halfPoints * 5 % 8 * 125) : "")
                       + "pt;";
            if ((attributes & 0x01) != 0)
                css += "font-weight:bold;";
            if ((attributes & 0x02) != 0)
                css += "font-style:italic;";
            if ((attributes & 0x04) != 0)
                decoration += " underline";
            if ((attributes & 0x08) != 0)
                decoration += " line-through";
            if ((attributes & 0x20) != 0)
                css += "font-variant:small-caps;";
            if (!decoration.empty())
                css += "text-decoration:" + decoration.substr(1) + ";";
            m_fonts.push_back(Font{css});
        }
    }

    // ------------------------------------------------------------ contexts

    void ReadContexts() {
        Span span;
        if (!m_file.Find("|CONTEXT", span))
            return;
        ForEachLeafEntry(span, [&](Reader& reader) {
            u32 hash = reader.U32();
            i32 offset = reader.I32();
            if (reader.Ok())
                m_contexts[hash] = offset;
            return true;
        });
    }

    // The topic holding a topic offset: the last topic starting at or before it.
    int TopicAt(i32 offset) const {
        if (m_topics.empty() || offset < 0)
            return -1;
        auto after = std::upper_bound(
            m_topicOffsets.begin(), m_topicOffsets.end(), offset);
        if (after == m_topicOffsets.begin())
            return -1;
        return static_cast<int>(after - m_topicOffsets.begin()) - 1;
    }

    // The anchor a context string's hash leads to: the topic, or a place
    // inside it; empty when the file has no such context.
    std::string AnchorOfHash(u32 hash) const {
        auto found = m_anchors.find(hash);
        if (found != m_anchors.end())
            return found->second;
        auto context = m_contexts.find(hash);
        int topic = context == m_contexts.end() ? -1 : TopicAt(context->second);
        return topic >= 0 ? "t" + std::to_string(topic) : std::string();
    }

    // Contexts whose offset falls before the end of the next text record get
    // their anchor: the topic itself while its scrolling text has not begun,
    // else a mark before the record.
    void PlaceContexts(i32 end) {
        while (m_nextContext < m_contextOrder.size() && m_contextOrder[m_nextContext].first < end) {
            u32 hash = m_contextOrder[m_nextContext].second;
            m_nextContext++;
            if (m_topics.empty())
                continue;
            if (m_topicScrollRecords == 0) {
                m_anchors[hash] = "t" + std::to_string(m_topics.size() - 1);
            } else {
                char id[16];
                std::snprintf(id, sizeof(id), "c%08x", hash);
                m_anchors[hash] = id;
                m_topics.back().body += "<span id=\"" + std::string(id) + "\"></span>";
            }
        }
    }

    // ------------------------------------------------------------ topics

    bool ReadTopics(std::string& error) {
        Span span;
        if (!m_file.Find("|TOPIC", span)) {
            error = "the help file has no |TOPIC";
            return false;
        }
        // Every block's text, back to back; a topic position is a block
        // number and an offset into the block counting its 12-byte header.
        Bytes stream;
        std::vector<size_t> blockStart;
        std::vector<size_t> blockLength;
        i32 firstLink = -1;
        for (size_t start = 0; start < span.size; start += m_blockSize) {
            size_t end = std::min(span.size, start + m_blockSize);
            if (end - start < kTopicBlockHeaderSize)
                break;
            Reader header(span.data + start, kTopicBlockHeaderSize);
            header.I32();  // last topic link
            i32 first = header.I32();
            if (start == 0)
                firstLink = first;
            const u8* data = span.data + start + kTopicBlockHeaderSize;
            size_t size = end - start - kTopicBlockHeaderSize;
            Bytes block = m_compressed
                              ? DecompressLz77(data, size, (1u << kTopicPositionBlockShift)
                                                               - kTopicBlockHeaderSize)
                              : Bytes(data, data + size);
            blockStart.push_back(stream.size());
            blockLength.push_back(block.size());
            stream.insert(stream.end(), block.begin(), block.end());
        }
        auto linear = [&](i32 position, size_t& offset) {
            if (position < 0)
                return false;
            u32 block = static_cast<u32>(position) >> kTopicPositionBlockShift;
            u32 within = static_cast<u32>(position) & kTopicPositionMask;
            if (block >= blockStart.size() || within < kTopicBlockHeaderSize
                || within - kTopicBlockHeaderSize > blockLength[block])
                return false;
            offset = blockStart[block] + within - kTopicBlockHeaderSize;
            return true;
        };

        for (const auto& [hash, offset] : m_contexts)
            m_contextOrder.emplace_back(offset, hash);
        std::sort(m_contextOrder.begin(), m_contextOrder.end());

        i32 position = firstLink;
        size_t previous = 0;
        bool first = true;
        i32 currentBlock = -1;
        i32 topicOffset = 0;
        size_t limit = stream.size() / kTopicLinkSize + 1;
        while (position != -1 && limit-- > 0) {
            size_t at;
            if (!linear(position, at) || (!first && at <= previous))
                break;
            first = false;
            previous = at;
            Reader link(stream.data() + at, stream.size() - at);
            i32 blockSize = link.I32();
            i32 dataLength2 = link.I32();
            link.I32();  // previous link
            i32 next = link.I32();
            i32 dataLength1 = link.I32();
            u8 type = link.U8();
            if (!link.Ok() || dataLength1 < static_cast<i32>(kTopicLinkSize)
                || blockSize < dataLength1 || static_cast<size_t>(blockSize) > stream.size() - at
                || dataLength2 < 0)
                break;
            // Topic offsets count text characters from the block the record
            // starts in, 0x8000 per block (the key of |CONTEXT and |TTLBTREE).
            i32 block = static_cast<i32>(static_cast<u32>(position) >> kTopicPositionBlockShift);
            if (block != currentBlock) {
                currentBlock = block;
                topicOffset = block * kTopicOffsetBlockSize;
            }
            Reader data1(stream.data() + at + kTopicLinkSize,
                         static_cast<size_t>(dataLength1) - kTopicLinkSize);
            Reader data2(stream.data() + at + static_cast<size_t>(dataLength1),
                         static_cast<size_t>(blockSize - dataLength1));
            if (type == RECORD_TOPIC_HEADER) {
                StartTopic(position, topicOffset, data1,
                           Expand(data2, static_cast<size_t>(dataLength2)));
            } else if (type == RECORD_DISPLAY || type == RECORD_TABLE) {
                PlaceContexts(topicOffset + std::max(dataLength2, 1));
                if (!m_topics.empty()) {
                    RenderRecord(position, type, data1,
                                 Expand(data2, static_cast<size_t>(dataLength2)));
                    if (!m_inNonScroll)
                        m_topicScrollRecords++;
                }
                topicOffset += dataLength2;
            }
            position = next;
        }
        FinishTopic();
        if (m_topics.empty()) {
            error = "the help file has no topics";
            return false;
        }
        m_report.topics = static_cast<int>(m_topics.size());
        // Links found while reading resolve now that every topic is known.
        for (Topic& topic : m_topics)
            ResolveLinks(topic.body);
        return true;
    }

    void StartTopic(i32 position, i32 offset, Reader header, const Bytes& text) {
        FinishTopic();
        Topic topic;
        topic.offset = offset;
        header.I32();  // block size
        topic.browseBack = header.I32();
        topic.browseForward = header.I32();
        header.I32();  // topic number
        i32 nonScroll = header.I32();
        m_scroll = header.I32();
        Reader title(text.data(), text.size());
        topic.title = Text(title.CString());
        m_inNonScroll = header.Ok() && nonScroll != -1 && m_scroll != position;
        if (m_inNonScroll)
            topic.body += "<div class=\"nsr\">";
        m_topics.push_back(std::move(topic));
        m_topicOffsets.push_back(offset);
        m_topicScrollRecords = 0;
        m_font = -1;
    }

    void FinishTopic() {
        if (m_topics.empty())
            return;
        CloseNonScroll();
    }

    void CloseNonScroll() {
        if (m_inNonScroll) {
            m_topics.back().body += "</div>";
            m_inNonScroll = false;
        }
    }

    // Links are written as placeholders (an HTML comment holding the topic's
    // hash, which escaped topic text cannot contain) until all topics are
    // known, then replaced by the topic's anchor.
    void ResolveLinks(std::string& body) {
        std::string out;
        size_t at = 0;
        while (true) {
            size_t open = body.find(kLinkOpen, at);
            if (open == std::string::npos)
                break;
            size_t close = body.find(kLinkClose, open);
            if (close == std::string::npos)
                break;
            out.append(body, at, open - at);
            std::string digits =
                body.substr(open + kLinkOpenLength, close - open - kLinkOpenLength);
            u32 hash = static_cast<u32>(std::strtoul(digits.c_str(), nullptr, 10));
            std::string anchor = AnchorOfHash(hash);
            if (anchor.empty())
                m_report.jumpsUnresolved++;
            out += "#" + anchor;
            at = close + kLinkCloseLength;
        }
        out.append(body, at, std::string::npos);
        body = std::move(out);
    }

    struct Paragraph {
        std::string style;
        bool open = false;
        bool empty = true;
    };

    // The paragraph settings that precede a record's (or table cell's) text.
    std::string ReadParagraphInfo(Reader& reader) {
        reader.Skip(4);
        u16 bits = reader.U16();
        std::string style;
        if ((bits & 0x0001) != 0)
            reader.CompressedI32();
        i16 above = (bits & 0x0002) != 0 ? reader.CompressedI16() : 0;
        i16 below = (bits & 0x0004) != 0 ? reader.CompressedI16() : 0;
        if ((bits & 0x0008) != 0)
            reader.CompressedI16();  // line spacing
        i16 left = (bits & 0x0010) != 0 ? reader.CompressedI16() : 0;
        i16 right = (bits & 0x0020) != 0 ? reader.CompressedI16() : 0;
        i16 firstLine = (bits & 0x0040) != 0 ? reader.CompressedI16() : 0;
        if ((bits & 0x0100) != 0)
            reader.Skip(3);  // border flags and width
        if ((bits & 0x0200) != 0) {
            i16 stops = reader.CompressedI16();
            for (i16 i = 0; i < stops && reader.Ok(); i++) {
                if ((reader.CompressedU16() & 0x4000) != 0)
                    reader.CompressedU16();
            }
        }
        // Measurements are in twips; 15 to a CSS pixel.
        auto px = [](i32 twips) { return std::to_string(twips / 15) + "px;"; };
        if (above > 0)
            style += "margin-top:" + px(above);
        if (below > 0)
            style += "margin-bottom:" + px(below);
        if (left != 0)
            style += "margin-left:" + px(left);
        if (right != 0)
            style += "margin-right:" + px(right);
        if (firstLine != 0)
            style += "text-indent:" + px(firstLine);
        if ((bits & 0x0800) != 0)
            style += "text-align:center;";
        else if ((bits & 0x0400) != 0)
            style += "text-align:right;";
        return style;
    }

    void RenderRecord(i32 position, u8 type, Reader info, const Bytes& text) {
        m_report.records++;
        if (position == m_scroll)
            CloseNonScroll();
        std::string& out = m_topics.back().body;
        info.CompressedU32();  // record size
        info.CompressedU16();  // text length
        bool table = type == RECORD_TABLE;
        if (table) {
            u8 columns = info.U8();
            u8 tableType = info.U8();
            if (tableType == 0 || tableType == 2)
                info.I16();  // minimum width
            info.Skip(4u * columns);
            out += "<table class=\"help\"><tr>";
        }
        Reader strings(text.data(), text.size());
        bool ok = true;
        i32 lastColumn = -1;
        while (ok && info.Ok()) {
            if (table) {
                i16 column = info.I16();
                if (column == -1 || !info.Ok())
                    break;
                info.Skip(3);
                if (column <= lastColumn)
                    out += "</tr><tr>";
                lastColumn = column;
                out += "<td>";
            }
            std::string style = ReadParagraphInfo(info);
            ok = RenderCommands(info, strings, style, out);
            if (table)
                out += "</td>";
            else
                break;
        }
        if (table)
            out += "</tr></table>";
        if (!ok || !info.Ok())
            m_report.recordsFailed++;
    }

    // The current font carries over paragraph and record ends, as in
    // WinHelp; a paragraph shows it as a span, re-opened after each change
    // outside links.
    struct Line {
        Paragraph paragraph;
        int spanFont = -1;
        bool link = false;
        bool spanLink = false;
    };

    void SyncFont(Line& line, std::string& out) {
        if (line.link || !line.paragraph.open || line.spanFont == m_font)
            return;
        if (line.spanFont >= 0)
            out += "</span>";
        line.spanFont = m_font;
        if (m_font >= 0)
            out += "<span class=\"f" + std::to_string(m_font) + "\">";
    }

    void OpenParagraph(Line& line, std::string& out) {
        if (line.paragraph.open)
            return;
        out += line.paragraph.style.empty() ? "<p>" : "<p style=\"" + line.paragraph.style + "\">";
        line.paragraph.open = true;
        line.paragraph.empty = true;
        SyncFont(line, out);
    }

    void CloseLink(Line& line, std::string& out) {
        if (line.link)
            out += line.spanLink ? "</span>" : "</a>";
        line.link = false;
    }

    void CloseParagraph(Line& line, std::string& out) {
        CloseLink(line, out);
        if (line.paragraph.open) {
            if (line.paragraph.empty)
                out += "<br>";
            if (line.spanFont >= 0)
                out += "</span>";
            out += "</p>";
        }
        line.spanFont = -1;
        line.paragraph.open = false;
    }

    void OpenLink(Line& line, std::string& out, const std::string& tag, bool span) {
        OpenParagraph(line, out);
        CloseLink(line, out);
        SyncFont(line, out);
        out += tag;
        line.link = true;
        line.spanLink = span;
    }

    // Text strings and formatting commands alternate: a string, a command,
    // a string, ... until the 0xFF command.
    bool RenderCommands(Reader& info, Reader& strings, const std::string& style, std::string& out) {
        Line line;
        line.paragraph.style = style;
        while (true) {
            std::string raw = strings.CString();
            if (!raw.empty()) {
                OpenParagraph(line, out);
                SyncFont(line, out);
                out += Escape(Text(raw));
                line.paragraph.empty = false;
            }
            u8 command = info.U8();
            if (!info.Ok())
                command = 0;  // the commands ran out: unparsed
            switch (command) {
                case 0xFF:
                    CloseLink(line, out);
                    if (line.paragraph.open) {
                        if (line.spanFont >= 0)
                            out += "</span>";
                        out += "</p>";
                    }
                    return true;
                case 0x20:  // field
                    info.I32();
                    break;
                case 0x21:  // data type
                    info.I16();
                    break;
                case 0x80: {  // font change
                    i16 number = info.I16();
                    m_font = number >= 0 && static_cast<size_t>(number) < m_fonts.size() ? number : -1;
                    break;
                }
                case 0x81:  // line break
                    OpenParagraph(line, out);
                    out += "<br>";
                    line.paragraph.empty = false;
                    break;
                case 0x82:  // end of paragraph
                    CloseParagraph(line, out);
                    break;
                case 0x83:  // tab
                    OpenParagraph(line, out);
                    out += "<span class=\"tab\"></span>";
                    break;
                case 0x86:
                case 0x87:
                case 0x88:
                    OpenParagraph(line, out);
                    RenderPicture(info, command, out);
                    line.paragraph.empty = false;
                    break;
                case 0x89:  // end of hotspot
                    CloseLink(line, out);
                    break;
                case 0x8B:
                    OpenParagraph(line, out);
                    out += "&nbsp;";
                    break;
                case 0x8C:
                    OpenParagraph(line, out);
                    out += "&#8209;";
                    break;
                case 0xC8:
                case 0xCC: {  // macro hotspot: shown, not run
                    i16 length = info.I16();
                    info.Skip(length > 0 ? static_cast<size_t>(length) : 0);
                    OpenLink(line, out, "<span class=\"macro\">", true);
                    break;
                }
                case 0xE0:
                case 0xE1: {  // 3.0 jump or popup by topic offset
                    i32 offset = info.I32();
                    int topic = TopicAt(offset);
                    OpenLink(line, out,
                             "<a class=\"" + std::string(command == 0xE0 ? "popup" : "jump")
                                 + "\" href=\"#t" + std::to_string(topic) + "\">",
                             false);
                    m_report.jumps++;
                    if (topic < 0)
                        m_report.jumpsUnresolved++;
                    break;
                }
                case 0xE2:
                case 0xE3:
                case 0xE6:
                case 0xE7: {  // popup or jump by context hash
                    u32 hash = info.U32();
                    bool popup = command == 0xE2 || command == 0xE6;
                    OpenLink(line, out,
                             "<a class=\"" + std::string(popup ? "popup" : "jump") + "\" href=\"" + kLinkOpen
                                 + std::to_string(hash) + kLinkClose + "\">",
                             false);
                    m_report.jumps++;
                    break;
                }
                case 0xEA:
                case 0xEB:
                case 0xEE:
                case 0xEF: {  // jump or popup to another file or window
                    i16 length = info.I16();
                    Reader target = info.Sub(length > 0 ? static_cast<size_t>(length) : 0);
                    u8 kind = target.U8();
                    u32 hash = target.U32();
                    if (kind == 1)
                        target.U8();  // window number
                    std::string file = kind == 4 || kind == 6 ? target.CString() : std::string();
                    bool popup = command == 0xEA || command == 0xEE;
                    if (file.empty()) {
                        OpenLink(line, out,
                                 "<a class=\"" + std::string(popup ? "popup" : "jump")
                                     + "\" href=\"" + kLinkOpen + std::to_string(hash) + kLinkClose
                                     + "\">",
                                 false);
                        m_report.jumps++;
                    } else {
                        OpenLink(line, out, "<span class=\"external\">", true);
                    }
                    break;
                }
                default:
                    // Unknown formatting: keep what was rendered.
                    CloseParagraph(line, out);
                    return false;
            }
        }
    }

    void RenderPicture(Reader& info, u8 command, std::string& out) {
        u8 type = info.U8();
        u32 size = info.CompressedU32();
        if (type == 0x22)
            info.CompressedU16();  // hotspots
        Reader picture = info.Sub(size);
        if (!info.Ok() || (type != 0x03 && type != 0x22))
            return;
        u16 embedded = picture.U16();
        u16 number = picture.U16();
        Span container;
        bool found = false;
        if (embedded == 0) {
            std::string name = "|bm" + std::to_string(number);
            found = m_file.Find(name.c_str(), container) || m_file.Find(name.substr(1).c_str(), container);
        } else if (picture.Ok()) {
            container.data = picture.Here();
            container.size = picture.Remaining();
            found = true;
        }
        Bytes bmp;
        if (!found || !DecodePicture(container, bmp)) {
            m_report.imagesFailed++;
            return;
        }
        m_report.images++;
        const char* align = command == 0x87 ? " class=\"left\"" : command == 0x88 ? " class=\"right\"" : "";
        out += "<img" + std::string(align) + " alt=\"\" src=\"data:image/bmp;base64," + Base64(bmp)
               + "\">";
    }

    // ------------------------------------------------------------ keywords

    void ReadKeywords() {
        Span tree;
        Span data;
        if (!m_file.Find("|KWBTREE", tree) || !m_file.Find("|KWDATA", data))
            return;
        ForEachLeafEntry(tree, [&](Reader& reader) {
            std::string word = reader.CString();
            u16 count = reader.U16();
            i32 offset = reader.I32();
            if (!reader.Ok() || offset < 0)
                return true;
            Reader topics = Read(data);
            topics.Seek(static_cast<size_t>(offset));
            std::vector<int> targets;
            for (u16 i = 0; i < count && topics.Ok(); i++) {
                int topic = TopicAt(topics.I32());
                if (topics.Ok() && topic >= 0
                    && std::find(targets.begin(), targets.end(), topic) == targets.end())
                    targets.push_back(topic);
            }
            m_keywords.emplace_back(Text(word), std::move(targets));
            return true;
        });
        m_report.keywords = static_cast<int>(m_keywords.size());
    }

    // ------------------------------------------------------------ .CNT

    // Lines "N Heading" and "N Title=context[@file][>window]"; lines that
    // start with ':' are directives.
    void ReadContentsFile(const std::string& contents) {
        std::istringstream lines(contents);
        std::string line;
        while (std::getline(lines, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
                line.pop_back();
            if (line.empty())
                continue;
            if (line[0] == ':') {
                if (line.compare(0, 7, ":Title ") == 0)
                    m_contentsTitle = Text(line.substr(7));
                continue;
            }
            size_t digits = 0;
            while (digits < line.size() && std::isdigit(static_cast<u8>(line[digits])) != 0)
                digits++;
            if (digits == 0 || digits > 2)
                continue;
            ContentsEntry entry;
            entry.level = std::clamp(std::atoi(line.substr(0, digits).c_str()), 1, 9);
            std::string rest = line.substr(digits);
            if (!rest.empty() && rest[0] == ' ')
                rest.erase(0, 1);
            size_t equals = rest.find('=');
            if (equals == std::string::npos) {
                entry.title = Text(rest);
            } else {
                entry.heading = false;
                entry.title = Text(rest.substr(0, equals));
                std::string context = rest.substr(equals + 1);
                size_t end = context.find_first_of("@>");
                bool otherFile = end != std::string::npos && context[end] == '@';
                context = context.substr(0, end);
                m_report.contentsEntries++;
                entry.anchor = otherFile ? std::string() : AnchorOfHash(ContextHash(context));
                if (entry.anchor.empty())
                    m_report.contentsUnresolved++;
            }
            m_contents.push_back(std::move(entry));
        }
    }

    // ------------------------------------------------------------ output

    std::string TopicLink(int topic) const {
        return "<a href=\"#t" + std::to_string(topic) + "\">"
               + Escape(m_topics[static_cast<size_t>(topic)].title) + "</a>";
    }

    size_t WriteContents(size_t at, int level, std::string& out) const {
        out += "<ul>";
        while (at < m_contents.size() && m_contents[at].level >= level) {
            const ContentsEntry& entry = m_contents[at];
            out += "<li>";
            if (entry.level > level) {
                at = WriteContents(at, entry.level, out);
                out += "</li>";
                continue;
            }
            if (entry.heading)
                out += "<span class=\"book\">" + Escape(entry.title) + "</span>";
            else if (!entry.anchor.empty())
                out += "<a href=\"#" + entry.anchor + "\">" + Escape(entry.title)
                       + "</a>";
            else
                out += Escape(entry.title);
            at++;
            if (at < m_contents.size() && m_contents[at].level > level)
                at = WriteContents(at, m_contents[at].level, out);
            out += "</li>";
        }
        out += "</ul>";
        return at;
    }

    void Write(std::string& html) const {
        bool russian = m_codepage == 1251;
        const std::string& contentsLabel = m_labels.contents;
        const std::string& indexLabel = m_labels.index;
        const std::string& backLabel = m_labels.contents;
        std::string title = !m_contentsTitle.empty() ? m_contentsTitle : Text(m_title);
        if (title.empty())
            title = "Help";
        html.clear();
        html += "<!DOCTYPE html>\n<html lang=\"";
        html += russian ? "ru" : "en";
        html += "\">\n<head>\n<meta charset=\"utf-8\">\n"
                "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                "<meta name=\"generator\" content=\"homm1 help converter "
                + std::to_string(CONVERTER_VERSION) + "\">\n<title>" + Escape(title)
                + "</title>\n<style>\n"
                  "body{margin:0;background:#fbf8ef;color:#1b1b1b;font:16px/1.45 'Segoe UI',"
                  "Arial,sans-serif}\n"
                  "header{background:#3c2f1e;color:#f3e7c9;padding:.6em 1.2em}\n"
                  "header h1{margin:0;font-size:1.3em}\n"
                  "header a{color:#f3e7c9}\n"
                  "main,#home{max-width:48em;margin:0 auto;padding:0 1.2em 2em}\n"
                  "#home h2{font-size:1.1em;margin:1.2em 0 .4em}\n"
                  "#home ul{list-style:none;padding-left:1.2em;margin:.1em 0}\n"
                  "#home>ul,#home>nav>ul{padding-left:0}\n"
                  ".book{font-weight:bold}\n"
                  ".index li{margin:.1em 0}\n"
                  "section.topic{display:none;padding:.4em 0 1em}\n"
                  "section.topic:target,section.topic:has(:target){display:block}\n"
                  "body:has(:target) #home{display:none}\n"
                  ".topnav{font-size:.85em;margin:.6em 0;display:flex;gap:1em}\n"
                  ".nsr{background:#efe6cc;border-bottom:1px solid #c9b98e;margin:0 -1.2em .8em;"
                  "padding:.5em 1.2em}\n"
                  "section p{margin:0 0 .15em}\n"
                  "a{color:#0b6b16}\n"
                  "a.popup{text-decoration-style:dotted}\n"
                  ".tab{display:inline-block;min-width:2em}\n"
                  "img.left{float:left;margin:0 .6em .3em 0}\n"
                  "img.right{float:right;margin:0 0 .3em .6em}\n"
                  "table.help{border-collapse:collapse}table.help td{vertical-align:top;"
                  "padding:.1em .5em}\n";
        for (size_t i = 0; i < m_fonts.size(); i++)
            html += ".f" + std::to_string(i) + "{" + m_fonts[i].css + "}\n";
        html += "</style>\n</head>\n<body>\n<header><h1><a href=\"#\">" + Escape(title)
                + "</a></h1></header>\n<div id=\"home\">\n<nav><h2>" + contentsLabel + "</h2>\n";
        if (!m_contents.empty()) {
            WriteContents(0, 1, html);
        } else {
            html += "<ul>";
            for (size_t i = 0; i < m_topics.size(); i++)
                html += "<li>" + TopicLink(static_cast<int>(i)) + "</li>";
            html += "</ul>";
        }
        html += "\n</nav>\n";
        if (!m_keywords.empty()) {
            html += "<nav class=\"index\"><h2>" + indexLabel + "</h2>\n<ul>";
            for (const auto& [word, topics] : m_keywords) {
                html += "<li>";
                if (topics.size() == 1) {
                    html += "<a href=\"#t" + std::to_string(topics[0]) + "\">" + Escape(word)
                            + "</a>";
                } else {
                    html += Escape(word);
                    for (int topic : topics)
                        html += " &middot; " + TopicLink(topic);
                }
                html += "</li>";
            }
            html += "</ul>\n</nav>\n";
        }
        html += "</div>\n<main>\n";
        for (size_t i = 0; i < m_topics.size(); i++) {
            const Topic& topic = m_topics[i];
            html += "<section class=\"topic\" id=\"t" + std::to_string(i) + "\" title=\""
                    + Escape(topic.title) + "\">\n<div class=\"topnav\"><a href=\"#\">"
                    + backLabel + "</a>";
            int back = topic.browseBack != -1 ? TopicAt(topic.browseBack) : -1;
            int forward = topic.browseForward != -1 ? TopicAt(topic.browseForward) : -1;
            if (back >= 0 && back != static_cast<int>(i))
                html += "<a href=\"#t" + std::to_string(back) + "\">&lt;&lt;</a>";
            if (forward >= 0 && forward != static_cast<int>(i))
                html += "<a href=\"#t" + std::to_string(forward) + "\">&gt;&gt;</a>";
            html += "</div>\n" + topic.body + "\n</section>\n";
        }
        html += "</main>\n</body>\n</html>\n";
    }

    const Bytes& m_bytes;
    Report& m_report;
    const Labels& m_labels;
    HelpFile m_file;
    u16 m_version = 0;
    bool m_compressed = false;
    size_t m_blockSize = 4096;
    int m_codepage = 1252;
    std::string m_title;
    std::string m_contentsTitle;
    bool m_hall = false;
    std::vector<Bytes> m_phrases;
    std::vector<Font> m_fonts;
    std::map<u32, i32> m_contexts;
    std::vector<Topic> m_topics;
    std::vector<i32> m_topicOffsets;
    i32 m_scroll = -1;
    std::vector<std::pair<i32, u32>> m_contextOrder;
    size_t m_nextContext = 0;
    std::map<u32, std::string> m_anchors;
    int m_topicScrollRecords = 0;
    int m_font = -1;
    bool m_inNonScroll = false;
    std::vector<std::pair<std::string, std::vector<int>>> m_keywords;
    std::vector<ContentsEntry> m_contents;
};

bool ReadWholeFile(const std::string& path, Bytes& bytes) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size < 0 || static_cast<u64>(size) > kMaxFileSize)
        return false;
    file.seekg(0, std::ios::beg);
    bytes.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
}

// Size and modification time, to tell when a cached conversion is stale.
std::string Fingerprint(const std::string& path) {
    std::error_code error;
    std::filesystem::path file(path);
    auto size = std::filesystem::file_size(file, error);
    if (error)
        return "-";
    auto time = std::filesystem::last_write_time(file, error);
    if (error)
        return std::to_string(size);
    return std::to_string(size) + ":"
           + std::to_string(static_cast<long long>(time.time_since_epoch().count()));
}

}  // namespace

bool Convert(
    const std::vector<u8>& helpFile,
    const std::string& contentsFile,
    int codepage,
    std::string& html,
    Report& report,
    std::string& error,
    const Labels& labels
) {
    report = Report();
    Converter converter(helpFile, report, labels);
    return converter.Run(contentsFile, codepage, html, error);
}

bool ConvertWinHelp(
    const std::string& hlpPath,
    const std::string& cntPath,
    std::string& html,
    std::string& error,
    const Labels& labels
) {
    Bytes help;
    if (!ReadWholeFile(hlpPath, help)) {
        error = "cannot read " + hlpPath;
        return false;
    }
    Bytes contents;
    if (!cntPath.empty())
        ReadWholeFile(cntPath, contents);
    Report report;
    return Convert(help, std::string(contents.begin(), contents.end()), 0, html, report, error,
                   labels);
}

bool PrepareHelp(
    const std::string& hlpPath,
    const std::string& cntPath,
    const std::string& outputDirectory,
    std::string& htmlPath,
    std::string& error,
    const Labels& labels
) {
    std::filesystem::path directory(outputDirectory);
    std::string stem = std::filesystem::path(hlpPath).stem().string();
    std::transform(stem.begin(), stem.end(), stem.begin(),
                   [](char c) { return static_cast<char>(std::tolower(static_cast<u8>(c))); });
    if (stem.empty())
        stem = "help";
    std::filesystem::path html = directory / (stem + ".html");
    std::filesystem::path stamp = directory / (stem + ".stamp");
    htmlPath = html.string();
    std::string expected = "homm1-help " + std::to_string(CONVERTER_VERSION) + "\n"
                           + Fingerprint(hlpPath) + "\n"
                           + (cntPath.empty() ? std::string("-") : Fingerprint(cntPath)) + "\n"
                           + labels.contents + "\n" + labels.index + "\n";
    {
        std::ifstream existing(stamp, std::ios::binary);
        std::stringstream contents;
        contents << existing.rdbuf();
        std::error_code ignored;
        if (existing && contents.str() == expected && std::filesystem::exists(html, ignored))
            return true;
    }
    std::string document;
    if (!ConvertWinHelp(hlpPath, cntPath, document, error, labels))
        return false;
    std::error_code failure;
    std::filesystem::create_directories(directory, failure);
    std::filesystem::path partial = directory / (stem + ".html.partial");
    {
        std::ofstream file(partial, std::ios::binary | std::ios::trunc);
        file.write(document.data(), static_cast<std::streamsize>(document.size()));
        if (!file) {
            error = "cannot write " + partial.string();
            return false;
        }
    }
    std::filesystem::rename(partial, html, failure);
    if (failure) {
        error = "cannot write " + html.string() + ": " + failure.message();
        return false;
    }
    std::ofstream(stamp, std::ios::binary | std::ios::trunc) << expected;
    return true;
}

}  // namespace platform::help
