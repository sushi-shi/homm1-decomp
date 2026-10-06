#include <H1/Ints.h>

#include <PLATFORM/Records.h>

#include <PLATFORM/File.h>

#include <string.h>

H1_STATIC_ASSERT(sizeof(float) == 4, "floats are stored as IEEE single precision");

RecordWriter::RecordWriter() {}

void RecordWriter::Put(u8 value) {
    m_bytes.push_back(value);
}

void RecordWriter::Put(i8 value) {
    Put(static_cast<u8>(value));
}

void RecordWriter::Put(char value) {
    Put(static_cast<u8>(value));
}

void RecordWriter::Put(u16 value) {
    Put(static_cast<u8>(value & 0xff));
    Put(static_cast<u8>(value >> 8));
}

void RecordWriter::Put(i16 value) {
    Put(static_cast<u16>(value));
}

void RecordWriter::Put(u32 value) {
    Put(static_cast<u8>(value & 0xff));
    Put(static_cast<u8>((value >> 8) & 0xff));
    Put(static_cast<u8>((value >> 16) & 0xff));
    Put(static_cast<u8>(value >> 24));
}

void RecordWriter::Put(i32 value) {
    Put(static_cast<u32>(value));
}

void RecordWriter::Put(float value) {
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    Put(bits);
}

void RecordWriter::Bytes(const void* data, i32 count) {
    const u8* bytes = static_cast<const u8*>(data);
    for (i32 i = 0; i < count; i++)
        m_bytes.push_back(bytes[i]);
}

void RecordWriter::Zeros(i32 count) {
    for (i32 i = 0; i < count; i++)
        m_bytes.push_back(0);
}

void RecordWriter::Put(const i8* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Put(values[i]);
}

void RecordWriter::Put(const u8* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Put(values[i]);
}

void RecordWriter::Put(const char* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Put(values[i]);
}

void RecordWriter::Put(const i16* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Put(values[i]);
}

void RecordWriter::Put(const u16* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Put(values[i]);
}

void RecordWriter::Put(const i32* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Put(values[i]);
}

i32 RecordWriter::Size() const {
    return static_cast<i32>(m_bytes.size());
}

const u8* RecordWriter::Data() const {
    return m_bytes.empty() ? NULL : &m_bytes[0];
}

bool RecordWriter::SaveFile(const char* path) const {
    return FileReplace(path, Data(), Size());
}

RecordReader::RecordReader() : m_data(NULL), m_size(0), m_offset(0), m_ok(true) {}

RecordReader::RecordReader(const u8* data, i32 size)
    : m_data(data), m_size(size), m_offset(0), m_ok(true) {}

bool RecordReader::LoadFile(const char* path) {
    i32 file = FileOpen(path, FILE_OPEN_READ);
    if (file == FILE_INVALID)
        return false;
    i32 length = FileLength(file);
    bool ok = length >= 0;
    if (ok) {
        m_owned.assign(static_cast<size_t>(length), 0);
        ok = length == 0 || FileReadExact(file, &m_owned[0], length);
    }
    FileClose(file);
    if (!ok) {
        m_owned.clear();
        return false;
    }
    m_data = m_owned.empty() ? NULL : &m_owned[0];
    m_size = length;
    m_offset = 0;
    m_ok = true;
    return true;
}

bool RecordReader::Take(u8* bytes, i32 count) {
    if (count < 0 || !m_ok || m_size - m_offset < count) {
        m_ok = false;
        if (count > 0)
            memset(bytes, 0, static_cast<size_t>(count));
        return false;
    }
    if (count > 0)
        memcpy(bytes, m_data + m_offset, static_cast<size_t>(count));
    m_offset += count;
    return true;
}

void RecordReader::Get(u8& value) {
    Take(&value, 1);
}

void RecordReader::Get(i8& value) {
    u8 byte;
    Take(&byte, 1);
    value = static_cast<i8>(byte);
}

void RecordReader::Get(char& value) {
    u8 byte;
    Take(&byte, 1);
    value = static_cast<char>(byte);
}

void RecordReader::Get(u16& value) {
    u8 bytes[2];
    Take(bytes, 2);
    value = static_cast<u16>(bytes[0] | (bytes[1] << 8));
}

void RecordReader::Get(i16& value) {
    u16 bits;
    Get(bits);
    value = static_cast<i16>(bits);
}

void RecordReader::Get(u32& value) {
    u8 bytes[4];
    Take(bytes, 4);
    value = static_cast<u32>(bytes[0]) | (static_cast<u32>(bytes[1]) << 8)
            | (static_cast<u32>(bytes[2]) << 16) | (static_cast<u32>(bytes[3]) << 24);
}

void RecordReader::Get(i32& value) {
    u32 bits;
    Get(bits);
    value = static_cast<i32>(bits);
}

void RecordReader::Get(float& value) {
    u32 bits;
    Get(bits);
    memcpy(&value, &bits, sizeof(value));
}

i16 RecordReader::GetI16() {
    i16 value;
    Get(value);
    return value;
}

u16 RecordReader::GetU16() {
    u16 value;
    Get(value);
    return value;
}

i32 RecordReader::GetI32() {
    i32 value;
    Get(value);
    return value;
}

u32 RecordReader::GetU32() {
    u32 value;
    Get(value);
    return value;
}

float RecordReader::GetF32() {
    float value;
    Get(value);
    return value;
}

void RecordReader::Bytes(void* data, i32 count) {
    Take(static_cast<u8*>(data), count);
}

void RecordReader::Skip(i32 count) {
    if (count < 0 || !m_ok || m_size - m_offset < count) {
        m_ok = false;
        return;
    }
    m_offset += count;
}

void RecordReader::Get(i8* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Get(values[i]);
}

void RecordReader::Get(u8* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Get(values[i]);
}

void RecordReader::Get(char* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Get(values[i]);
}

void RecordReader::Get(i16* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Get(values[i]);
}

void RecordReader::Get(u16* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Get(values[i]);
}

void RecordReader::Get(i32* values, i32 count) {
    for (i32 i = 0; i < count; i++)
        Get(values[i]);
}

bool RecordReader::Ok() const {
    return m_ok;
}

i32 RecordReader::Offset() const {
    return m_offset;
}

i32 RecordReader::Remaining() const {
    return m_size - m_offset;
}

void CopyTextField(char* destination, i32 capacity, const char* field, i32 fieldSize) {
    i32 length = 0;
    if (capacity <= 0)
        return;
    while (length < fieldSize && length < capacity - 1 && field[length] != '\0')
        length++;
    memcpy(destination, field, static_cast<size_t>(length));
    destination[length] = '\0';
}
