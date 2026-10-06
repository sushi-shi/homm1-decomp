#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

class bitmap;

enum FileDescriptorConstant {
    FILE_DESCRIPTOR_INVALID = -1
};

#define READ_FILE_VALUE(fd, value) read((fd), &(value), sizeof(value))
#define WRITE_FILE_VALUE(fd, value) write((fd), &(value), sizeof(value))

#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

void SetPalette(i8* paletteData, b32 updateDisplay);

#endif
