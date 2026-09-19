// Located from HoMM2 Buka 2.1; HoMM1 uses the same bitmap core with
// 16-bit dimensions and coordinates.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindowManager.h>
#include <H1/KB.h>

#include <stdlib.h>

extern heroWindowManager *gpWindowManager;

VA(0x0047a720, 0x4d)
bitmap::bitmap(short type, short width, short height)
    : resource(RESOURCE_CATEGORY_BITMAP, 0, -1, 0)
{
    m_bitmapType = type;
    m_width = width;
    m_height = height;
    m_pixels = static_cast<signed char *>(malloc(width * height));
}

bitmap::~bitmap(void)
{
    if (m_pixels != 0)
        free(m_pixels);
    m_pixels = 0;
}

VA(0x0047a820, 0x3e)
void bitmap::DrawToBuffer(short x, short y)
{
    PollSound();
    BlitBitmap(
        this, 0, 0, m_width, m_height, gpWindowManager->m_screen, x, y);
    PollSound();
}

VA(0x0047a860, 0x18)
void bitmap::GrabScreen(short x, short y)
{
    GrabScreenBitmap(this, x, y);
}

VA(0x0047a880, 0x2b)
void bitmap::GrabBitmap(bitmap *source, short x, short y)
{
    BlitBitmap(source, x, y, m_width, m_height, this, 0, 0);
}
