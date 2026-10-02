// HoMM1 icon loading follows Buka 2.1, with a retail post-read hook.

#include <match.h>

#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/IconEntry.h>
#include <BASE/Iconm2b.h>
#include <BASE/resourceManager.h>
#include <H1/KB.h>

#include <stdlib.h>

VA(0x00479b20, 0x6d)
icon::icon(short id)
    : resource(RESOURCE_CATEGORY_ICON, id, RESOURCE_REFERENCE_INITIAL, NULL)
{
    gpResourceManager->PointToFile(id);
    m_frameCount = gpResourceManager->ReadWord();
    unsigned long length = gpResourceManager->ReadLong();
    m_data = static_cast<unsigned char *>(malloc(length));
    gpResourceManager->ReadBlock(reinterpret_cast<signed char *>(m_data), length); // byte-evidenced: ReadBlock accepts signed bytes for icon pixel storage.
    PostprocessIcon(this);
}

VA_COMPGEN(0x00479b90, 0x33, "??_Gicon@@UAEPAXI@Z", 0x00479b20)
icon::~icon(void)
{
    free(m_data);
}

// Each orientation arm sets its own top/bottom, as HoMM2 CombatClipDrawToBuffer does; VC4
// tail-merges the two copies and carries the arm's frame-entry address across the join.
VA(0x00479bd0, 0x22a)
void icon::DrawToBuffer(short x, short y, short frame, H1_ENUM_PARAM(IconDrawOrientation, signed char) orientation,
                        H1_ENUM_PARAM(IconDrawOffsetMode, signed char) mode)
{
    if (gbComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawRight = x - (reinterpret_cast<IconEntry *>(m_data)[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT); // byte-evidenced: packed frame entry in resource bytes.
            else
                m_drawRight = x - reinterpret_cast<IconEntry *>(m_data)[frame].x; // byte-evidenced: packed frame entry in resource bytes.
            m_drawLeft = m_drawRight - reinterpret_cast<IconEntry *>(m_data)[frame].w; // byte-evidenced: packed frame entry in resource bytes.
            m_drawTop = reinterpret_cast<IconEntry *>(m_data)[frame].y + y; // byte-evidenced: packed frame entry in resource bytes.
            m_drawBottom = reinterpret_cast<IconEntry *>(m_data)[frame].h + m_drawTop; // byte-evidenced: packed frame entry in resource bytes.
        } else {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = (reinterpret_cast<IconEntry *>(m_data)[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT) + x; // byte-evidenced: packed frame entry in resource bytes.
            else
                m_drawLeft = reinterpret_cast<IconEntry *>(m_data)[frame].x + x; // byte-evidenced: packed frame entry in resource bytes.
            m_drawRight = m_drawLeft + reinterpret_cast<IconEntry *>(m_data)[frame].w; // byte-evidenced: packed frame entry in resource bytes.
            m_drawTop = reinterpret_cast<IconEntry *>(m_data)[frame].y + y; // byte-evidenced: packed frame entry in resource bytes.
            m_drawBottom = reinterpret_cast<IconEntry *>(m_data)[frame].h + m_drawTop; // byte-evidenced: packed frame entry in resource bytes.
        }
        if (gbSaveBiggestExtent != 0) {
            if (giMinExtentX > m_drawLeft) giMinExtentX = m_drawLeft;
            if (giMinExtentY > m_drawTop) giMinExtentY = m_drawTop;
            if (giMaxExtentX < m_drawRight) giMaxExtentX = m_drawRight;
            if (giMaxExtentY < m_drawBottom) giMaxExtentY = m_drawBottom;
        }
    }
    if (gbLimitToExtent != 0 && (gbCurrArmyDrawn == 0 || m_drawLeft > giMaxExtentX
        || m_drawRight < giMinExtentX || m_drawTop > giMaxExtentY || m_drawBottom < giMinExtentY))
        return;
    if (gbIconClipOn != 0) {
        if (orientation == ICON_DRAW_NORMAL)
            ClippedIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
        else
            FlipClippedIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
    } else {
        if (orientation == ICON_DRAW_NORMAL)
            IconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
        else
            FlipIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
    }
}

VA(0x00479e00, 0x51)
void icon::ClipFillToBuffer(short x, short y, short frame, short color,
                            H1_ENUM_PARAM(IconDrawOrientation, signed char) orientation, H1_ENUM_PARAM(IconDrawOffsetMode, signed char) mode, int clipX, int clipY, int clipW, int clipH)
{
    ClippedMonoIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, gMonoColorMap[color], mode, clipX, clipY, clipW, clipH);
}

VA(0x00479e60, 0x132)
void icon::FillToBuffer(short x, short y, short frame, short color,
    H1_ENUM_PARAM(IconDrawOrientation, signed char) orientation, H1_ENUM_PARAM(IconDrawOffsetMode, signed char) mode)
{
    if (orientation == ICON_DRAW_NORMAL) {
        if (gbLimitToExtent) {
            IconEntry *entry = reinterpret_cast<IconEntry *>(m_data) + frame; // byte-evidenced: packed frame directory decoded from icon resource bytes.
            m_drawLeft = x + entry->x;
            m_drawRight = m_drawLeft + entry->w;
            m_drawTop = y + entry->y;
            m_drawBottom = m_drawTop + entry->h;
            if (!gbCurrArmyDrawn || m_drawLeft > giMaxExtentX || m_drawRight < giMinExtentX
                || m_drawTop > giMaxExtentY || m_drawBottom < giMinExtentY)
                return;
        }
        MonoIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, gMonoColorMap[color], mode);
    } else {
        FlipMonoIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, gMonoColorMap[color], mode);
    }
}

VA(0x00479fa0, 0x1c2)
void icon::DimToBuffer(short x, short y, short frame, H1_ENUM_PARAM(IconDrawOrientation, signed char) orientation,
                       H1_ENUM_PARAM(IconDrawOffsetMode, signed char) mode)
{
    if (gbComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawRight = x - (reinterpret_cast<IconEntry *>(m_data)[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT); // byte-evidenced: packed frame entry in resource bytes.
            else
                m_drawRight = x - reinterpret_cast<IconEntry *>(m_data)[frame].x; // byte-evidenced: packed frame entry in resource bytes.
            m_drawLeft = m_drawRight - reinterpret_cast<IconEntry *>(m_data)[frame].w; // byte-evidenced: packed frame entry in resource bytes.
            m_drawTop = reinterpret_cast<IconEntry *>(m_data)[frame].y + y; // byte-evidenced: packed frame entry in resource bytes.
            m_drawBottom = reinterpret_cast<IconEntry *>(m_data)[frame].h + m_drawTop; // byte-evidenced: packed frame entry in resource bytes.
        } else {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = (reinterpret_cast<IconEntry *>(m_data)[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT) + x; // byte-evidenced: packed frame entry in resource bytes.
            else
                m_drawLeft = reinterpret_cast<IconEntry *>(m_data)[frame].x + x; // byte-evidenced: packed frame entry in resource bytes.
            m_drawRight = m_drawLeft + reinterpret_cast<IconEntry *>(m_data)[frame].w; // byte-evidenced: packed frame entry in resource bytes.
            m_drawTop = reinterpret_cast<IconEntry *>(m_data)[frame].y + y; // byte-evidenced: packed frame entry in resource bytes.
            m_drawBottom = reinterpret_cast<IconEntry *>(m_data)[frame].h + m_drawTop; // byte-evidenced: packed frame entry in resource bytes.
        }
        if (gbSaveBiggestExtent != 0) {
            if (giMinExtentX > m_drawLeft) giMinExtentX = m_drawLeft;
            if (giMinExtentY > m_drawTop) giMinExtentY = m_drawTop;
            if (giMaxExtentX < m_drawRight) giMaxExtentX = m_drawRight;
            if (giMaxExtentY < m_drawBottom) giMaxExtentY = m_drawBottom;
        }
    }
    if (gbLimitToExtent != 0 && (gbCurrArmyDrawn == 0 || m_drawLeft > giMaxExtentX
        || m_drawRight < giMinExtentX || m_drawTop > giMaxExtentY || m_drawBottom < giMinExtentY))
        return;
    if (orientation == ICON_DRAW_NORMAL)
        DimIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
    else
        FlipDimIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
}

