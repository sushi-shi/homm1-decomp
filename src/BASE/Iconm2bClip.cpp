// HoMM1's C++ mono clipping path, corresponding to donor Iconm2b.cpp.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/IconEntry.h>
#include <BASE/Iconm2b.h>

#include <string.h>

#pragma intrinsic(memcpy, memset)

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
            if (run != 0) {
                position += run;
                source++;
            } else
                drawing = 0;
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
            source++;
        } else {
            position = x + entry->x;
            rowOffset += ICON_SCREEN_ROW_BYTES;
            row++;
            source++;
        }
    }
}

// Clipped colour icon blit kept beside the mono path. Retail keeps every
// working value in file statics, as in the assembly renderers.
static int sClipRight;
static signed char *sClipRow;
static IconEntry *sClipEntry;
static unsigned int sClipRun;
static int sClipBottom;
static int sClipX;
static int sClipY;
static unsigned char *sClipSource;
static int sClipInside;
static int sClipRowStart;

VA(0x00473ad0, 0x2ad)
void ClipIconToBitmap(icon *sourceIcon, bitmap *destination, int x, int y, int frame, int mode, int clipX, int clipY, int clipW, int clipH)
{
    sClipEntry = reinterpret_cast<IconEntry *>(sourceIcon->m_data) + frame; // byte-evidenced: packed frame directory decoded from resource bytes.
    sClipSource = sourceIcon->m_data + sClipEntry->srcOffset;
    sClipX = sClipRowStart = x + sClipEntry->x;
    sClipY = y + sClipEntry->y;
    if (sClipRowStart < clipX || sClipRowStart + sClipEntry->w > clipX + clipW
        || sClipY < clipY || sClipY + sClipEntry->h > clipY + clipH) {
        sClipInside = 0;
        sClipRight = clipX + clipW - 1;
        sClipBottom = clipY + clipH - 1;
    } else {
        sClipInside = 1;
    }
    sClipRow = destination->m_pixels + destination->m_width * sClipY;
    for (;;) {
        sClipRun = *sClipSource++;
        if (static_cast<signed char>(sClipRun) < 0) {
            if ((sClipRun & ICON_MONO_SKIP_MASK) == 0)
                return;
            sClipX += sClipRun & ICON_MONO_SKIP_MASK;
            continue;
        }
        if (sClipRun != 0) {
            if (sClipInside) {
                memcpy(sClipRow + sClipX, sClipSource, sClipRun);
            } else if (sClipY >= clipY && sClipBottom >= sClipY && sClipRun + sClipX >= clipX
                       && sClipX <= sClipRight) {
                if (sClipX >= clipX) {
                    if (sClipRight >= sClipX + sClipRun)
                        memcpy(sClipRow + sClipX, sClipSource, sClipRun);
                    else
                        memcpy(sClipRow + sClipX, sClipSource, sClipRight - sClipX + 1);
                } else {
                    if (*sClipSource + sClipX <= sClipRight)
                        memcpy(sClipRow + sClipX, sClipSource, sClipRun - clipX + sClipX);
                    else
                        memcpy(sClipRow + sClipX, sClipSource, clipW);
                }
            }
            sClipX += sClipRun;
            sClipSource += sClipRun;
        } else {
            sClipX = sClipRowStart;
            sClipY++;
            sClipRow += destination->m_width;
        }
    }
}
