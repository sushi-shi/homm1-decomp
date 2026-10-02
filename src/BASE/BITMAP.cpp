// Located from HoMM2 Buka 2.1; HoMM1 uses the same bitmap core with
// 16-bit dimensions and coordinates.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindowManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#pragma intrinsic(memcpy)

VA(0x0047a6b0, 0x2a)
VA_COMPGEN(0x0047a6e0, 0x3e, "??_Gbitmap@@UAEPAXI@Z", 0x0047a6b0)
bitmap::bitmap(void) : resource(RESOURCE_CATEGORY_BITMAP, 0, -1, NULL) {
    m_bitmapType = 0;
    m_width = 0;
    m_height = 0;
    m_pixels = NULL;
}

VA(0x0047a720, 0x4d)
bitmap::bitmap(short type, short width, short height)
    : resource(RESOURCE_CATEGORY_BITMAP, 0, -1, NULL) {
    m_bitmapType = type;
    m_width = width;
    m_height = height;
    m_pixels = static_cast<signed char*>(malloc(width * height));
}

// Retail's ID constructor reads the packed bitmap and postprocesses its pixels.
VA(0x0047a770, 0xa1)
bitmap::bitmap(short id) : resource(RESOURCE_CATEGORY_BITMAP, id, 1, NULL) {
    gpResourceManager->PointToFile(id);
    m_bitmapType = gpResourceManager->ReadWord();
    m_width = gpResourceManager->ReadWord();
    m_height = gpResourceManager->ReadWord();
    int size = m_width * m_height;
    m_pixels = static_cast<signed char*>(malloc(size));
    PollSound();
    gpResourceManager->ReadBlock(m_pixels, size);
    PostprocessBitmap(m_pixels, m_width, m_height);
    PollSound();
}

bitmap::~bitmap(void) {
    if (m_pixels != 0)
        free(m_pixels);
    m_pixels = 0;
}

VA(0x0047a820, 0x3e)
void bitmap::DrawToBuffer(short x, short y) {
    PollSound();
    BlitBitmap(this, 0, 0, m_width, m_height, gpWindowManager->m_screen, x, y);
    PollSound();
}

VA(0x0047a860, 0x18)
void bitmap::GrabScreen(short x, short y) {
    GrabScreenBitmap(this, x, y);
}

VA(0x0047a880, 0x2b)
void bitmap::GrabBitmap(bitmap* source, short x, short y) {
    BlitBitmap(source, x, y, m_width, m_height, this, 0, 0);
}

// Raw screenshot writer: combat palette followed by the pixel plane.
// Retail colours palette/file/this in that order (esi/edi/ebx); VC4 ties follow
// symbol order, so the palette pointer is declared before the file handle.
VA(0x0047a8b0, 0x7f)
void bitmap::Write(char* filename) {
    palette* combatPalette;
    int file = open(filename, O_WRONLY | O_CREAT | O_BINARY, S_IWRITE);
    if (file != -1) {
        combatPalette = gpResourceManager->GetPalette("combat.pal");
        signed char* paletteData = combatPalette->Data();
        write(file, paletteData, PALETTE_RAW_BYTES);
        write(file, m_pixels, m_width * m_height);
        close(file);
        gpResourceManager->Dispose(combatPalette);
    }
}

VA(0x0047a930, 0xbd)
void bitmap::CopyTo(
    bitmap* destination,
    int destinationX,
    int destinationY,
    int sourceX,
    int sourceY,
    int width,
    int height
) {
    PollSound();
    if (width != BITMAP_COPY_STRIDE) {
        for (int row = 0; row < height; row++) {
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
