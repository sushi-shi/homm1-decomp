#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

#include <Domains.h>

class bitmap;

// The descriptor the runtime's open returns when a file cannot be opened;
// an unopened descriptor field holds it too.
H1_ENUM_CONST_BEGIN(FileDescriptorConstant)
    FILE_DESCRIPTOR_INVALID = -1
H1_ENUM_CONST_END(FileDescriptorConstant)

// Read or write one whole value of the file's record; the value's
// own size is the transfer size.
#define READ_FILE_VALUE(fd, value) read((fd), &(value), sizeof(value))
#define WRITE_FILE_VALUE(fd, value) write((fd), &(value), sizeof(value))

// Map-grid (taxicab) distance of an offset.
#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

void SetPalette(i8* paletteData, b32 updateDisplay);

#endif
