#include <H1/Ints.h>

#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

icon::icon(i16 id) : resource(RESOURCE_CATEGORY_ICON, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gResourceManager->PointToFile(id);
    m_frameCount = gResourceManager->ReadWord();
    u32 length = gResourceManager->ReadLong();
    m_data = static_cast<u8*>(malloc(length));
    gResourceManager->ReadBlock(m_data, length);
    PostprocessIcon(this);
}

icon::~icon(void) {
    free(m_data);
}

void icon::DrawToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i8 orientation,
    i8 offsetMode
) {
    if (gComputeExtent != false) {
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
    if (gLimitToExtent != false
        && (gCurrArmyDrawn == false || m_drawLeft > gMaxExtentX || m_drawRight < gMinExtentX
            || m_drawTop > gMaxExtentY || m_drawBottom < gMinExtentY))
        return;
    if (gIconClipOn != false) {
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

void icon::ClipFillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    i8 orientation,
    i8 offsetMode,
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

void icon::FillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    i8 orientation,
    i8 offsetMode
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

void icon::DimToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i8 orientation,
    i8 offsetMode
) {
    if (gComputeExtent != false) {
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
    if (gLimitToExtent != false
        && (gCurrArmyDrawn == false || m_drawLeft > gMaxExtentX || m_drawRight < gMinExtentX
            || m_drawTop > gMaxExtentY || m_drawBottom < gMinExtentY))
        return;
    if (orientation == ICON_DRAW_NORMAL)
        DimIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
    else
        FlipDimIconToBitmap(this, gWindowManager->m_screen, x, y, frame, offsetMode);
}
