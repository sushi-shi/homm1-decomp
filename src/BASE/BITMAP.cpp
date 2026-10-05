// Located from HoMM2 Buka 2.1; HoMM1 uses the same bitmap core with
// 16-bit dimensions and coordinates.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindowManager.h>
#include <BASE/miscwin.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

VA_COMPGEN(0x00473336, 0x3e, "??1bitmap@@UAE@XZ", 0x00473180)
VA(0x00473180, 0x4c)
VA_COMPGEN(0x004735e0, 0x2e, "??_Gbitmap@@UAEPAXI@Z", 0x00473180)
bitmap::bitmap(void) : resource(RESOURCE_CATEGORY_BITMAP, 0, RESOURCE_REFERENCE_UNMANAGED, NULL) {
    m_bitmapType = BITMAP_TYPE_NONE;
    m_width = 0;
    m_height = 0;
    m_pixels = NULL;
}

VA(0x004731cc, 0x64)
bitmap::bitmap(i16 type, i16 width, i16 height)
    : resource(RESOURCE_CATEGORY_BITMAP, 0, RESOURCE_REFERENCE_UNMANAGED, NULL) {
    m_bitmapType = type;
    m_width = width;
    m_height = height;
    m_pixels = static_cast<i8*>(malloc(width * height));
}

// Retail's ID constructor reads the packed bitmap and postprocesses its pixels.
VA(0x00473230, 0x106)
bitmap::bitmap(i16 id) : resource(RESOURCE_CATEGORY_BITMAP, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gpResourceManager->PointToFile(id);
    m_bitmapType = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    i32 size = m_width * m_height;
    m_pixels = static_cast<i8*>(malloc(size));
    PollSound();
    gpResourceManager->ReadBlock(m_pixels, size);
    PostprocessBitmap(m_pixels, m_width, m_height);
    PollSound();
}

bitmap::~bitmap(void) {
    if (m_pixels != NULL)
        free(m_pixels);
    m_pixels = NULL;
}

VA(0x00473374, 0x4b)
void bitmap::DrawToBuffer(i16 x, i16 y) {
    PollSound();
    BlitBitmap(this, 0, 0, m_width, m_height, gpWindowManager->m_screen, x, y);
    PollSound();
}

// HoMM2 Buka bitmap::DrawToScreen.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x004733bf, 0x41)
void bitmap::DrawToScreen(i16 x, i16 y) {
    PollSound();
    BlitBitmapToScreen(this, 0, 0, m_width, m_height, x, y);
    PollSound();
}

VA(0x00473400, 0x23)
void bitmap::GrabScreen(i16 x, i16 y) {
    GrabScreenBitmap(this, x, y);
}

VA(0x00473423, 0x3b)
void bitmap::GrabBitmap(bitmap* source, i16 x, i16 y) {
    BlitBitmap(source, x, y, m_width, m_height, this, 0, 0);
}

// Raw screenshot writer: combat palette followed by the pixel plane.
// Buka returns immediately when the output file cannot be opened.
VA(0x0047345e, 0xa3)
void bitmap::Write(char* filename) {
    palette* combatPaletteData;
    i32 unusedData; // dead local: retail's /Od frame holds its unreferenced slot
    i32 file = open(filename, O_WRONLY | O_CREAT | O_BINARY, S_IWRITE);
    if (file == -1)
        return;
    combatPaletteData = gpResourceManager->GetPalette("combat.pal");
    i8* paletteData = combatPaletteData->Data();
    write(file, paletteData, PALETTE_DATA_SIZE);
    write(file, m_pixels, m_width * m_height);
    close(file);
    gpResourceManager->Dispose(combatPaletteData);
}

VA(0x00473501, 0xa6)
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
    if (width != BITMAP_COPY_STRIDE) {
        for (i32 row = 0; row < height; row++) {
            memcpy(
                destination->m_pixels + destinationX + (destinationY + row) * BITMAP_COPY_STRIDE,
                m_pixels + sourceX + (sourceY + row) * BITMAP_COPY_STRIDE,
                width
            );
        }
    } else {
        memcpy(
            destination->m_pixels + destinationX + destinationY,
            m_pixels + sourceX + sourceY,
            width * height
        );
    }
    PollSound();
}
