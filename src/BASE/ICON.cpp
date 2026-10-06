#include <H1/Ints.h>

#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/IconEntry.h>
#include <BASE/Iconm2b.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

icon::icon(i16 id) : resource(RESOURCE_CATEGORY_ICON, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gpResourceManager->PointToFile(id);
    m_frameCount = gpResourceManager->ReadWord();
    u32 length = gpResourceManager->ReadLong();
    m_data = static_cast<u8*>(malloc(length));
    gpResourceManager->ReadBlock(
        reinterpret_cast<i8*>(m_data),
        length
    );
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
    i8 mode
) {
    if (gComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawRight =
                    x
                    - (reinterpret_cast<IconEntry*>(m_data)[frame].x
                       >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawRight = x
                              - reinterpret_cast<IconEntry*>(m_data)[frame]
                                    .x;
            m_drawLeft = m_drawRight
                         - reinterpret_cast<IconEntry*>(m_data)[frame]
                               .w;
            m_drawTop = reinterpret_cast<IconEntry*>(m_data)[frame].y
                        + y;
            m_drawBottom = reinterpret_cast<IconEntry*>(m_data)[frame].h
                           + m_drawTop;
        } else {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = (reinterpret_cast<IconEntry*>(m_data)[frame].x
                              >> ICON_DRAW_QUARTER_OFFSET_SHIFT)
                             + x;
            else
                m_drawLeft = reinterpret_cast<IconEntry*>(m_data)[frame].x
                             + x;
            m_drawRight = m_drawLeft
                          + reinterpret_cast<IconEntry*>(m_data)[frame]
                                .w;
            m_drawTop = reinterpret_cast<IconEntry*>(m_data)[frame].y
                        + y;
            m_drawBottom = reinterpret_cast<IconEntry*>(m_data)[frame].h
                           + m_drawTop;
        }
        if (gSaveBiggestExtent != 0) {
            if (giMinExtentX > m_drawLeft)
                giMinExtentX = m_drawLeft;
            if (giMinExtentY > m_drawTop)
                giMinExtentY = m_drawTop;
            if (giMaxExtentX < m_drawRight)
                giMaxExtentX = m_drawRight;
            if (giMaxExtentY < m_drawBottom)
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

void icon::ClipFillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    i8 orientation,
    i8 mode,
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

void icon::FillToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i16 color,
    i8 orientation,
    i8 mode
) {
    if (orientation == ICON_DRAW_NORMAL) {
        if (gLimitToExtent) {
            IconEntry* entry =
                reinterpret_cast<IconEntry*>(m_data)
                + frame;
            m_drawLeft = x + entry->x;
            m_drawRight = m_drawLeft + entry->w;
            m_drawTop = y + entry->y;
            m_drawBottom = m_drawTop + entry->h;
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

void icon::DimToBuffer(
    i16 x,
    i16 y,
    i16 frame,
    i8 orientation,
    i8 mode
) {
    if (gComputeExtent != 0) {
        if (orientation != ICON_DRAW_NORMAL) {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawRight =
                    x
                    - (reinterpret_cast<IconEntry*>(m_data)[frame].x
                       >> ICON_DRAW_QUARTER_OFFSET_SHIFT);
            else
                m_drawRight = x
                              - reinterpret_cast<IconEntry*>(m_data)[frame]
                                    .x;
            m_drawLeft = m_drawRight
                         - reinterpret_cast<IconEntry*>(m_data)[frame]
                               .w;
            m_drawTop = reinterpret_cast<IconEntry*>(m_data)[frame].y
                        + y;
            m_drawBottom = reinterpret_cast<IconEntry*>(m_data)[frame].h
                           + m_drawTop;
        } else {
            if (mode != ICON_DRAW_OFFSET_FULL)
                m_drawLeft = (reinterpret_cast<IconEntry*>(m_data)[frame].x
                              >> ICON_DRAW_QUARTER_OFFSET_SHIFT)
                             + x;
            else
                m_drawLeft = reinterpret_cast<IconEntry*>(m_data)[frame].x
                             + x;
            m_drawRight = m_drawLeft
                          + reinterpret_cast<IconEntry*>(m_data)[frame]
                                .w;
            m_drawTop = reinterpret_cast<IconEntry*>(m_data)[frame].y
                        + y;
            m_drawBottom = reinterpret_cast<IconEntry*>(m_data)[frame].h
                           + m_drawTop;
        }
        if (gSaveBiggestExtent != 0) {
            if (giMinExtentX > m_drawLeft)
                giMinExtentX = m_drawLeft;
            if (giMinExtentY > m_drawTop)
                giMinExtentY = m_drawTop;
            if (giMaxExtentX < m_drawRight)
                giMaxExtentX = m_drawRight;
            if (giMaxExtentY < m_drawBottom)
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
