// Icon loading, with a retail post-read hook.

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
    gResourceManager->PointToFile(id);
    m_frameCount = gResourceManager->ReadWord();
    u32 length = gResourceManager->ReadLong();
    m_data = static_cast<u8*>(malloc(length));
    gResourceManager->ReadBlock(m_data, length);
    PostprocessIcon(this);
}

VA(0x00470f5f, 0x2b)
icon::~icon(void) {
    free(m_data);
}

// Each orientation arm sets its own top/bottom.
VA(0x00470f8a, 0x317)
void icon::DrawToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) offsetMode
) {
    if (gComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (offsetMode != ICON_DRAW_OFFSET_FULL)
                m_drawRight = x - (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawRight = x - m_frames[frame].x;
            m_drawLeft = m_drawRight - m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        } else {
            if (offsetMode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = x + (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawLeft = x + m_frames[frame].x;
            m_drawRight = m_drawLeft + m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        }
        if (gSaveBiggestExtent != false) {
            if (m_drawLeft < gMinExtentX)
                gMinExtentX = m_drawLeft;
            if (m_drawTop < gMinExtentY)
                gMinExtentY = m_drawTop;
            if (m_drawRight > gMaxExtentX)
                gMaxExtentX = m_drawRight;
            if (m_drawBottom > gMaxExtentY)
                gMaxExtentY = m_drawBottom;
        }
    }
    if (gLimitToExtent != 0
        && (gCurrArmyDrawn == false || m_drawLeft > gMaxExtentX || m_drawRight < gMinExtentX
            || m_drawTop > gMaxExtentY || m_drawBottom < gMinExtentY))
        return;
    if (gIconClipOn != 0) {
        if (orientation == ICON_DRAW_NORMAL)
            ClippedIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
        else
            FlipClippedIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
    } else {
        if (orientation == ICON_DRAW_NORMAL)
            IconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
        else
            FlipIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
    }
}

VA(0x004712a1, 0x54)
void icon::ClipFillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) offsetMode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
) {
    ClippedMonoIconToBitmap(
        this,
        gWindowManager->m_screen,
        x,
        y,
        frame,
        gMonoColorMap[color],
        offsetMode,
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
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) offsetMode
) {
    if (orientation == ICON_DRAW_NORMAL) {
        if (gLimitToExtent) {
            m_drawLeft = x + m_frames[frame].x;
            m_drawRight = m_drawLeft + m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
            if (!gCurrArmyDrawn || m_drawLeft > gMaxExtentX || m_drawRight < gMinExtentX
                || m_drawTop > gMaxExtentY || m_drawBottom < gMinExtentY)
                return;
        }
        MonoIconToBitmap(
            this,
            gWindowManager->m_screen,
            x,
            y,
            frame,
            gMonoColorMap[color],
            offsetMode
        );
    } else {
        FlipMonoIconToBitmap(
            this,
            gWindowManager->m_screen,
            x,
            y,
            frame,
            gMonoColorMap[color],
            offsetMode
        );
    }
}

VA(0x00471453, 0x2a8)
void icon::DimToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
    H1_ENUM_PARAM(IconDrawOffsetMode, i8) offsetMode
) {
    if (gComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (offsetMode != ICON_DRAW_OFFSET_FULL)
                m_drawRight = x - (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawRight = x - m_frames[frame].x;
            m_drawLeft = m_drawRight - m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        } else {
            if (offsetMode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = x + (m_frames[frame].x >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawLeft = x + m_frames[frame].x;
            m_drawRight = m_drawLeft + m_frames[frame].w;
            m_drawTop = y + m_frames[frame].y;
            m_drawBottom = m_drawTop + m_frames[frame].h;
        }
        if (gSaveBiggestExtent != false) {
            if (m_drawLeft < gMinExtentX)
                gMinExtentX = m_drawLeft;
            if (m_drawTop < gMinExtentY)
                gMinExtentY = m_drawTop;
            if (m_drawRight > gMaxExtentX)
                gMaxExtentX = m_drawRight;
            if (m_drawBottom > gMaxExtentY)
                gMaxExtentY = m_drawBottom;
        }
    }
    if (gLimitToExtent != 0
        && (gCurrArmyDrawn == false || m_drawLeft > gMaxExtentX || m_drawRight < gMinExtentX
            || m_drawTop > gMaxExtentY || m_drawBottom < gMinExtentY))
        return;
    if (orientation == ICON_DRAW_NORMAL)
        DimIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
    else
        FlipDimIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
}

VA_COMPGEN(0x00471740, 0x2e, "??_Gicon@@UAEPAXI@Z", 0x00470ea0)
