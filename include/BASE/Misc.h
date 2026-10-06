#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

#include <Domains.h>

class bitmap;

// Read or write one whole value of the file's record; the value's
// own size is the transfer size.
#define READ_FILE_VALUE(fd, value) read((fd), &(value), sizeof(value))
#define WRITE_FILE_VALUE(fd, value) write((fd), &(value), sizeof(value))

// Map-grid (taxicab) distance of an offset.
#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

void SetPalette(i8* paletteData, b32 updateDisplay);

#endif
