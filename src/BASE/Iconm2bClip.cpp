// HoMM1's C++ mono clipping path, corresponding to donor Iconm2b.cpp.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/IconEntry.h>
#include <BASE/Iconm2b.h>

#include <string.h>

#pragma intrinsic(memset)

VA(0x004738e0, 0x1e6)
void ClippedMonoIconToBitmap(icon *sourceIcon, bitmap *destination, int x, int y, int frame, int color, int mode, int clipX, int clipY, int clipW, int clipH)
{
    int clipRight = clipX + clipW - 1;
    int clipBottom = clipY + clipH - 1;
    IconEntry *entry = reinterpret_cast<IconEntry *>(sourceIcon->m_data) + frame; // byte-evidenced: packed frame directory decoded from resource bytes.
    unsigned char *source = sourceIcon->m_data + entry->srcOffset;
    int position = x + entry->x;
    int drawing = 1;
    int row = y + entry->y;
    int rowOffset = row * ICON_SCREEN_ROW_BYTES;
    while (drawing != 0) {
        unsigned char run = *source;
        if (static_cast<signed char>(run) < 0) {
            run &= ICON_MONO_SKIP_MASK;
            if (run != 0)
                position += run;
            else {
                drawing = 0;
                continue;
            }
        } else if (run != ICON_MONO_NEWLINE_COMMAND) {
            if (row >= clipY && row <= clipBottom && position + run >= clipX && position <= clipRight) {
                if (position >= clipX) {
                    if (position + run <= clipRight)
                        memset(destination->m_pixels + rowOffset + position, color, run);
                    else
                        memset(destination->m_pixels + rowOffset + position, color, clipRight - position + 1);
                } else {
                    if (position + run <= clipRight)
                        memset(destination->m_pixels + rowOffset + clipX, color, position + run - clipX);
                    else
                        memset(destination->m_pixels + rowOffset + clipX, color, clipW);
                }
            }
            position += *source;
        } else {
            position = x + entry->x;
            row++;
            rowOffset += ICON_SCREEN_ROW_BYTES;
        }
        source++;
    }
}
