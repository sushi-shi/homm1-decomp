#ifndef HOMM1_PLATFORM_RECORDS_H
#define HOMM1_PLATFORM_RECORDS_H

// Byte-exact encoding of the game's file records. Every value is written and
// read as little-endian bytes of its declared width, one field at a time, so
// a file's layout never depends on the host's structure packing, pointer
// size, `long` width or byte order. Reads past the end of the data fail the
// reader (Ok() turns false) and yield zero bytes instead of stale memory.
//
// Plain C++98: the Visual C++ 6 build compiles it too.

#include <H1/Ints.h>

#include <stddef.h>
#include <vector>

// A compile-time check that also works with Visual C++ 6, which predates
// static_assert.
#if defined(_MSC_VER) && _MSC_VER < 1600
#define H1_STATIC_ASSERT_JOIN2(a, b) a##b
#define H1_STATIC_ASSERT_JOIN(a, b) H1_STATIC_ASSERT_JOIN2(a, b)
#define H1_STATIC_ASSERT(condition, message)                                                      \
    typedef char H1_STATIC_ASSERT_JOIN(StaticAssertion, __LINE__)[(condition) ? 1 : -1]
#else
#define H1_STATIC_ASSERT(condition, message) static_assert((condition), message)
#endif

class RecordWriter {
public:
    RecordWriter();

    void Put(i8 value);
    void Put(u8 value);
    void Put(i16 value);
    void Put(u16 value);
    void Put(i32 value);
    void Put(u32 value);
    void Put(float value);
    void Put(char value);
    void Bytes(const void* data, i32 count);
    void Zeros(i32 count);

    // An array of fields. Arrays of multi-byte fields inside packed
    // structures are written element by element by the caller instead: a
    // pointer to such an array is misaligned.
    void Put(const i8* values, i32 count);
    void Put(const u8* values, i32 count);
    void Put(const char* values, i32 count);
    void Put(const i16* values, i32 count);
    void Put(const u16* values, i32 count);
    void Put(const i32* values, i32 count);

    i32 Size() const;
    const u8* Data() const;

    // Writes the whole record set to a game path; on hosts that support it
    // the file is replaced only once the new contents are safely on disk.
    bool SaveFile(const char* path) const;

private:
    std::vector<u8> m_bytes;
};

class RecordReader {
public:
    RecordReader();
    RecordReader(const u8* data, i32 size);

    // Reads a whole game file.
    bool LoadFile(const char* path);

    void Get(i8& value);
    void Get(u8& value);
    void Get(i16& value);
    void Get(u16& value);
    void Get(i32& value);
    void Get(u32& value);
    void Get(float& value);
    void Get(char& value);
    void Bytes(void* data, i32 count);
    void Skip(i32 count);

    // Values by return, for fields of packed structures, which must not be
    // bound to references.
    i16 GetI16();
    u16 GetU16();
    i32 GetI32();
    u32 GetU32();
    float GetF32();

    void Get(i8* values, i32 count);
    void Get(u8* values, i32 count);
    void Get(char* values, i32 count);
    void Get(i16* values, i32 count);
    void Get(u16* values, i32 count);
    void Get(i32* values, i32 count);

    bool Ok() const;
    i32 Offset() const;
    i32 Remaining() const;

private:
    bool Take(u8* bytes, i32 count);

    std::vector<u8> m_owned;
    const u8* m_data;
    i32 m_size;
    i32 m_offset;
    bool m_ok;
};

// Copies a fixed-width text field that need not be terminated into a
// destination of capacity bytes, always terminating it.
void CopyTextField(char* destination, i32 capacity, const char* field, i32 fieldSize);

#endif
