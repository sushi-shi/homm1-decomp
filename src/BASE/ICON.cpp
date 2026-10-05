// HoMM1 icon loading follows Buka 2.1, with a retail post-read hook.

#include <match.h>

#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

VA(0x00470ea0, 0xbf)
icon::icon(i16 id) : resource(RESOURCE_CATEGORY_ICON, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gpResourceManager->PointToFile(id);
    m_frameCount = gpResourceManager->ReadWord();
    u32 length = gpResourceManager->ReadLong();
    m_data = static_cast<u8*>(malloc(length));
    gpResourceManager->ReadBlock(
        reinterpret_cast<i8*>(m_data), // API-forced: ReadBlock takes i8*.
        length
    ); // byte-evidenced: ReadBlock accepts signed bytes for icon pixel storage.
    PostprocessIcon(this);
}

icon::~icon(void) {
    free(m_data);
}

VA_COMPGEN(0x00470f5f, 0x2b, "??1icon@@UAE@XZ", 0x00470ea0)
// Each orientation arm sets its own top/bottom, as HoMM2 CombatClipDrawToBuffer does.
VA(0x00470f8a, 0x317)
void icon::DrawToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode
) {
    if (gComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawRight = x - (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawRight = x - m_frames[frame].x;
            m_drawLeft = m_drawRight - m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        } else {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = x + (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawLeft = x + m_frames[frame].x;
            m_drawRight = m_drawLeft + m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        }
        if (gSaveBiggestExtent != 0) {
            if (m_drawLeft < giMinExtentX)
                giMinExtentX = m_drawLeft;
            if (m_drawTop < giMinExtentY)
                giMinExtentY = m_drawTop;
            if (m_drawRight > giMaxExtentX)
                giMaxExtentX = m_drawRight;
            if (m_drawBottom > giMaxExtentY)
                giMaxExtentY = m_drawBottom;
        }
    }
    if (gLimitToExtent != 0
        && (gCurrArmyDrawn == 0 || m_drawLeft > giMaxExtentX || m_drawRight < giMinExtentX
            || m_drawTop > giMaxExtentY || m_drawBottom < giMinExtentY))
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

VA(0x004712a1, 0x54)
void icon::ClipFillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
) {
    ClippedMonoIconToBitmap(
        this,
        gpWindowManager->m_screen,
        x,
        y,
        frame,
        gMonoColorMap[color],
        mode,
        clipX,
        clipY,
        clipW,
        clipH
    );
}

VA(0x004712f5, 0x15e)
void icon::FillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode
) {
    if (orientation == ICON_DRAW_NORMAL) {
        if (gLimitToExtent) {
            m_drawLeft = x + m_frames[frame].x;
            m_drawRight = m_drawLeft + m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
            if (!gCurrArmyDrawn || m_drawLeft > giMaxExtentX || m_drawRight < giMinExtentX
                || m_drawTop > giMaxExtentY || m_drawBottom < giMinExtentY)
                return;
        }
        MonoIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, gMonoColorMap[color], mode);
    } else {
        FlipMonoIconToBitmap(
            this,
            gpWindowManager->m_screen,
            x,
            y,
            frame,
            gMonoColorMap[color],
            mode
        );
    }
}

VA(0x00471453, 0x2a8)
void icon::DimToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) mode
) {
    if (gComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawRight = x - (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawRight = x - m_frames[frame].x;
            m_drawLeft = m_drawRight - m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        } else {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = x + (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawLeft = x + m_frames[frame].x;
            m_drawRight = m_drawLeft + m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        }
        if (gSaveBiggestExtent != 0) {
            if (m_drawLeft < giMinExtentX)
                giMinExtentX = m_drawLeft;
            if (m_drawTop < giMinExtentY)
                giMinExtentY = m_drawTop;
            if (m_drawRight > giMaxExtentX)
                giMaxExtentX = m_drawRight;
            if (m_drawBottom > giMaxExtentY)
                giMaxExtentY = m_drawBottom;
        }
    }
    if (gLimitToExtent != 0
        && (gCurrArmyDrawn == 0 || m_drawLeft > giMaxExtentX || m_drawRight < giMinExtentX
            || m_drawTop > giMaxExtentY || m_drawBottom < giMinExtentY))
        return;
    if (orientation == ICON_DRAW_NORMAL)
        DimIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
    else
        FlipDimIconToBitmap(this, gpWindowManager->m_screen, x, y, frame, mode);
}

VA_COMPGEN(0x00471740, 0x2e, "??_Gicon@@UAEPAXI@Z", 0x00470ea0)
