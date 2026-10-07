#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindowManager.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bitmap::bitmap(void) : resource(RESOURCE_CATEGORY_BITMAP, 0, RESOURCE_REFERENCE_UNMANAGED, NULL) {
    m_bitmapType = BITMAP_TYPE_NONE;
    m_width = 0;
    m_height = 0;
    m_pixels = NULL;
}

bitmap::bitmap(i16 type, i16 width, i16 height)
    : resource(RESOURCE_CATEGORY_BITMAP, 0, RESOURCE_REFERENCE_UNMANAGED, NULL) {
    m_bitmapType = type;
    m_width = width;
    m_height = height;
    m_pixels = static_cast<u8*>(malloc(width * height));
}

bitmap::bitmap(i16 id) : resource(RESOURCE_CATEGORY_BITMAP, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gResourceManager->PointToFile(id);
    m_bitmapType = (gResourceManager->ReadWord());
    m_width = gResourceManager->ReadWord();
    m_height = gResourceManager->ReadWord();
    i32 size = m_width * m_height;
    m_pixels = static_cast<u8*>(malloc(size));
    PollSound();
    gResourceManager->ReadBlock(m_pixels, size);
    PostprocessBitmap(m_pixels, m_width, m_height);
    PollSound();
}

bitmap::~bitmap(void) {
    if (m_pixels != NULL)
        free(m_pixels);
    m_pixels = NULL;
}

void bitmap::DrawToBuffer(i16 x, i16 y) {
    PollSound();
    BlitBitmap(this, 0, 0, m_width, m_height, gWindowManager->m_screen, x, y);
    PollSound();
}

void bitmap::DrawToScreen(i16 x, i16 y) {
    PollSound();
    BlitBitmapToScreen(this, 0, 0, m_width, m_height, x, y);
    PollSound();
}

void bitmap::GrabScreen(i16 x, i16 y) {
    GrabScreenBitmap(this, x, y);
}

void bitmap::GrabBitmap(bitmap* source, i16 x, i16 y) {
    BlitBitmap(source, x, y, m_width, m_height, this, 0, 0);
}

void bitmap::Write(char* filename) {
    palette* combatPaletteData;
    i32 unusedData;
    i32 file = open(filename, O_WRONLY | O_CREAT | O_BINARY, S_IWRITE);
    if (file == FILE_DESCRIPTOR_INVALID)
        return;
    combatPaletteData = gResourceManager->GetPalette("combat.pal");
    i8* paletteData = combatPaletteData->Data();
    write(file, paletteData, PALETTE_DATA_SIZE);
    write(file, m_pixels, m_width * m_height);
    close(file);
    gResourceManager->Dispose(combatPaletteData);
}

void bitmap::CopyTo(
    bitmap* destination,
    i32 destinationX,
    i32 destinationY,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height
) {
    PollSound();
    if (width != LOGICAL_SCREEN_WIDTH) {
        for (i32 row = 0; row < height; row++) {
            memcpy(
                destination->m_pixels + destinationX + (destinationY + row) * LOGICAL_SCREEN_WIDTH,
                m_pixels + sourceX + (sourceY + row) * LOGICAL_SCREEN_WIDTH,
                width
            );
        }
    } else {
        memcpy(
            destination->m_pixels + destinationX + destinationY * LOGICAL_SCREEN_WIDTH,
            m_pixels + sourceX + sourceY * LOGICAL_SCREEN_WIDTH,
            width * height
        );
    }
    PollSound();
}
