// Combat stacks.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/PATH.h>
#include <SOURCE/searchArray.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x004a6770)
static char gTargetName[TARGET_NAME_SIZE];

VA(0x00413330, 0xbc)
army::army(void) {
    i32 i;

    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 0;
    m_attackIcon = NULL;
    m_walkIcon = NULL;
    m_standIcon = NULL;
    m_hex = 0;
    for (i = 0; i < ARMY_SAMPLE_COUNT; i++)
        m_samples[i] = NULL;
    m_effectAnimation = COMBAT_EFFECT_NONE;
    m_drawShadow = 1;
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
    gSpellEffectFrame = 0;
    CLEAR_ARMY_TARGET(this);
    m_attackDirection = COMBAT_DIRECTION_INVALID;
    m_unknown04 = 0;
    m_moveTargetHex = 0;
}

VA(0x004133ec, 0x64)
void army::InitClean(void) {
    i32 i;

    for (i = 0; i < ARMY_SAMPLE_COUNT; i++)
        m_samples[i] = NULL;
    m_effectAnimation = COMBAT_EFFECT_NONE;
    m_drawShadow = 1;
    m_attackIcon = NULL;
    m_walkIcon = NULL;
    m_standIcon = NULL;
}

// The commanding hero's attack and defense raise the copied creature stats.
VA(0x00413450, 0x105)
void army::Init(i8 type, i16 quantity, i8 side, i8 index) {
    hero* commander;

    InitClean();
    m_creatureType = type;
    memcpy(&m_stats, &gMonsterDatabase[type].stats, sizeof(tag_monsterStats));
    m_unknown29 = 6;
    m_spellEffect = SPELL_NONE;
    m_spellEndCondition = ARMY_CANCEL_SPELLS_NONE;
    commander = gpCombatManager->m_heroes[side];
    if (commander) {
        m_stats.attack += commander->m_primaryStats[HERO_PRIMARY_ATTACK];
        m_stats.defense += commander->m_primaryStats[HERO_PRIMARY_DEFENSE];
    }
    m_facing = side ^ 1;
    m_walkYStep = 0;
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    m_baseSpeed = m_stats.speed;
    m_quantity = quantity;
    m_initialQuantity = quantity;
    m_hitPointsLost = 0;
    m_damageMode = ARMY_DAMAGE_RANDOM;
    m_powFrames = ARMY_POW_NONE;
    m_side = side;
    m_index = index;
}

VA(0x00413555, 0x20b)
void army::LoadResources(void) {
    char sprite[16];
    i32 idx;
    char buffer[16];

    if (m_creatureType != CREATURE_SWORDSMAN)
        strcpy(sprite, gArmySpriteNames[m_creatureType]);
    else
        strcpy(sprite, "swrdsman");
    sprintf(gText, "%s.std", sprite);
    gMonoIconSkip = 0;
    m_standIcon = gpResourceManager->GetIcon(gText);
    gMonoIconSkip = -1;
    sprintf(gText, "%s.wlk", sprite);
    m_walkIcon = gpResourceManager->GetIcon(gText);
    sprintf(gText, "move%02d.82M", m_creatureType);
    m_samples[ARMY_SAMPLE_MOVE] = gpResourceManager->GetSample(gText);
    sprintf(gText, "atksnd%02d.82M", m_creatureType);
    m_samples[ARMY_SAMPLE_ATTACK] = gpResourceManager->GetSample(gText);
    sprintf(gText, "wince%02d.82M", m_creatureType);
    m_samples[ARMY_SAMPLE_WINCE] = gpResourceManager->GetSample(gText);
    if (m_stats.attributes & MONSTER_FLAGS_SHOOTER) {
        sprintf(gText, "%s.atk", sprite);
        m_attackIcon = gpResourceManager->GetIcon(gText);
        sprintf(gText, "shoot%02d.82M", m_creatureType);
        m_samples[ARMY_SAMPLE_SHOOT] = gpResourceManager->GetSample(gText);
    } else {
        m_attackIcon = NULL;
        m_samples[ARMY_SAMPLE_SHOOT] = NULL;
    }
    for (idx = 0; idx < ARMY_SAMPLE_COUNT; idx++) {
        if (m_samples[idx]) {
            m_samples[idx]->m_playbackData.repeat = 0;
            m_samples[idx]->m_playbackData.volume = SAMPLE_VOLUME_FULL;
        }
    }
}

VA(0x00413760, 0xd4)
void army::FreeResources(void) {
    i32 i;

    if (m_standIcon) {
        gpResourceManager->Dispose(m_standIcon);
        m_standIcon = NULL;
    }
    if (m_walkIcon) {
        gpResourceManager->Dispose(m_walkIcon);
        m_walkIcon = NULL;
    }
    if ((m_stats.attributes & MONSTER_FLAGS_SHOOTER) && m_attackIcon) {
        gpResourceManager->Dispose(m_attackIcon);
        m_attackIcon = NULL;
    }
    for (i = 0; i < ARMY_SAMPLE_COUNT; i++) {
        if (m_samples[i]) {
            gpResourceManager->Dispose(m_samples[i]);
            m_samples[i] = NULL;
        }
    }
}

// m_animationSequence selects the stand, walk, attack or spell-effect pose and
// m_animationFrame its frame; m_drawShadow adds the shadow frames.
VA(0x00413834, 0x7a5)
void army::DrawToBuffer(i16 x, i16 y) {
    i16 xPos;
    i8 offsetMode;
    i8 outlined;
    i16 outlineColorVal;
    char countText[ARMY_QUANTITY_TEXT_SIZE];
    i16 posX;
    i16 px;

    offsetMode = ICON_DRAW_OFFSET_FULL;
    outlined = 0;
    if ((m_animationFrame == ARMY_EDGE_CLIP_FRAME || m_animationFrame >= ARMY_EDGE_CLIP_LATER_FRAME)
        && ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_hex % COMBAT_GRID_COLUMNS <= 2 || m_hex % COMBAT_GRID_COLUMNS >= 6)
            || !(m_stats.attributes & MONSTER_FLAGS_WIDE)
                   && (m_hex % COMBAT_GRID_COLUMNS <= 1 || m_hex % COMBAT_GRID_COLUMNS >= 7)))
        gbIconClipOn = 1;
    if (m_walkYStep) {
        y += m_walkYStep * m_animationFrame;
        if (m_animationFrame > 0 && m_animationFrame <= 5)
            offsetMode = ICON_DRAW_OFFSET_QUARTER;
    }
    switch (m_animationSequence) {
        case ARMY_ANIMATION_STAND:
            switch (m_creatureType) {
                case CREATURE_HYDRA:
                    if (m_drawShadow)
                        m_standIcon->DimToBuffer(x, y, m_animationFrame + 8, m_facing, offsetMode);
                    break;
                case CREATURE_CYCLOPS:
                case CREATURE_PHOENIX:
                case CREATURE_DRAGON:
                    if (m_drawShadow)
                        m_standIcon->DimToBuffer(x, y, m_animationFrame + 15, m_facing, offsetMode);
                    break;
                default:
                    if (m_drawShadow)
                        m_standIcon->DimToBuffer(x, y, m_animationFrame + 9, m_facing, offsetMode);
                    break;
            }
            if (m_animationFrame > 4 && m_creatureType != CREATURE_HYDRA)
                m_standIcon->DrawToBuffer(x, y, 5, m_facing, offsetMode);
            m_standIcon->DrawToBuffer(x, y, m_animationFrame, m_facing, offsetMode);
            if (m_hex == gpCombatManager->m_limitCreatureHex
                && gpCombatManager->m_limitCreature == 1) {
                m_standIcon->FillToBuffer(x, y, 0, ARMY_LIMIT_OUTLINE_COLOR, m_facing, offsetMode);
                outlined = 1;
            }
            if (m_spellEffect != SPELL_NONE) {
                switch (m_spellEffect) {
                    case SPELL_HASTE:
                    case SPELL_BLESS:
                    case SPELL_PROTECTION:
                    case SPELL_ANTI_MAGIC:
                        outlineColorVal = ARMY_GOOD_SPELL_OUTLINE_COLOR;
                        break;
                    default:
                        outlineColorVal = ARMY_BAD_SPELL_OUTLINE_COLOR;
                        break;
                }
                if (!outlined && m_animationFrame == 1)
                    m_standIcon->FillToBuffer(x, y, 0, outlineColorVal, m_facing, offsetMode);
                posX = x;
                if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    if (m_facing == ARMY_FACING_RIGHT)
                        posX += 75;
                    else
                        posX -= 95;
                } else if (m_facing == ARMY_FACING_LEFT) {
                    posX -= 39;
                }
                gpCombatManager->m_combatIcons[COMBAT_ICON_SPELLS]->DrawToBuffer(
                    posX,
                    y - 40,
                    m_spellEffect,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (m_animationFrame == 1 && gpCombatManager->m_showArmyQuantities) {
                if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    if (m_facing == ARMY_FACING_RIGHT)
                        px = x + 75;
                    else
                        px = x - 95;
                } else {
                    if (m_facing == ARMY_FACING_RIGHT)
                        px = x + 8;
                    else
                        px = x - 39;
                }
                gpCombatManager->m_combatIcons[COMBAT_ICON_TEXTBAR]
                    ->DrawToBuffer(px, y - 11, 5, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
                sprintf(countText, "%d", m_quantity);
                gpCombatManager->m_smallFont
                    ->DrawBoundedString(countText, px, y - 12, 20, 12, 1, FONT_ALIGN_CENTER);
            }
            break;
        case ARMY_ANIMATION_WALK:
            if (m_drawShadow)
                m_walkIcon->DimToBuffer(x, y, m_animationFrame + 6, m_facing, offsetMode);
            m_walkIcon->DrawToBuffer(x, y, m_animationFrame, m_facing, offsetMode);
            break;
        case ARMY_ANIMATION_ATTACK:
            if (m_animationFrame < 5) {
                if (m_drawShadow)
                    m_attackIcon->DimToBuffer(x, y, m_animationFrame + 9, m_facing, offsetMode);
                m_attackIcon->DrawToBuffer(x, y, 0, m_facing, offsetMode);
            }
            m_attackIcon->DrawToBuffer(x, y, m_animationFrame, m_facing, offsetMode);
            break;
        case ARMY_ANIMATION_EFFECT:
            if (!(m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                switch (m_creatureType) {
                    case CREATURE_HYDRA:
                        if (m_drawShadow)
                            m_standIcon
                                ->DimToBuffer(x, y, m_animationFrame + 8, m_facing, offsetMode);
                        break;
                    case CREATURE_CYCLOPS:
                    case CREATURE_PHOENIX:
                    case CREATURE_DRAGON:
                        if (m_drawShadow)
                            m_standIcon
                                ->DimToBuffer(x, y, m_animationFrame + 15, m_facing, offsetMode);
                        break;
                    default:
                        if (m_drawShadow)
                            m_standIcon
                                ->DimToBuffer(x, y, m_animationFrame + 9, m_facing, offsetMode);
                        break;
                }
                m_standIcon->DrawToBuffer(x, y, m_animationFrame, m_facing, offsetMode);
            }
            xPos = x;
            if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (m_facing == ARMY_FACING_RIGHT) {
                    xPos += 39;
                    x += 75;
                } else {
                    xPos -= 39;
                    x -= 95;
                }
            } else if (m_facing == ARMY_FACING_LEFT) {
                x -= 39;
            }
            if (m_spellEffect != SPELL_NONE)
                gpCombatManager->m_combatIcons[COMBAT_ICON_SPELLS]->DrawToBuffer(
                    x,
                    y - 40,
                    m_spellEffect,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            gCurLoadedSpellIcon->DrawToBuffer(xPos, y, gSpellEffectFrame, m_facing, offsetMode);
            break;
    }
    gbIconClipOn = 0;
}

VA(0x00413fd9, 0x55)
void army::Stand(i8 redraw) {
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    m_walkYStep = 0;
    gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    if (redraw)
        gpCombatManager->DrawFrame(1);
}

VA(0x0041402e, 0x56)
void army::Wince(void) {
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 2;
    m_walkYStep = 0;
    gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    gpCombatManager->SetGridMode(
        m_facing != ARMY_FACING_RIGHT ? static_cast<i8>(1) : static_cast<i8>(0)
    );
}

// One hex of walking: six frames redrawn inside the union of the old and
// new extents; a stack turned away from the step moves before animating.
VA(0x00414084, 0x7d0)
void army::Walk(i16 direction, i8 standAfter, i8 continued) {
    i32 rectMaxX;
    i32 ourMaxY;
    i16 moveDist;
    i16 startFrame;
    i16 stepCount;
    i16 newReverse;
    i16 i;
    i16 newBaseHex;
    i16 toggleOn;
    i32 col;
    i32 partnerHexVal;
    i16 theTargetHex;
    i32 ourMinY;
    i32 nextTail;

    if (!continued) {
        giMinExtentX = giMinExtentY = COMBAT_EXTENT_MIN_START;
        giMaxExtentX = giMaxExtentY = 0;
        gComputeExtent = 1;
        gSaveBiggestExtent = 1;
        DrawToBuffer(
            gpCombatManager->m_hexCells[m_hex].m_x,
            gpCombatManager->m_hexCells[m_hex].m_y
        );
        gSaveBiggestExtent = 0;
        gComputeExtent = 0;
    }
    if (giMinExtentX < 0)
        giMinExtentX = 0;
    if (giMinExtentY < 0)
        giMinExtentY = 0;
    if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
        giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
    if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
        giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
    col = giMinExtentX - 5;
    ourMinY = giMinExtentY - 5;
    rectMaxX = giMaxExtentX + 5;
    ourMaxY = giMaxExtentY + 5;
    moveDist = 16;
    newReverse = 0;
    m_walkYStep = 0;
    if (direction < COMBAT_DIRECTION_WESTERN_FIRST) {
        if (m_facing == ARMY_FACING_RIGHT) {
            startFrame = 0;
            stepCount = 1;
        } else {
            startFrame = 5;
            stepCount = -1;
            newReverse = 1;
        }
    } else if (m_facing == ARMY_FACING_LEFT) {
        startFrame = 0;
        stepCount = 1;
    } else {
        startFrame = 5;
        stepCount = -1;
        newReverse = 1;
    }
    if (direction == COMBAT_DIRECTION_NORTHWEST || direction == COMBAT_DIRECTION_NORTHEAST)
        m_walkYStep = -16;
    if (direction == COMBAT_DIRECTION_SOUTHWEST || direction == COMBAT_DIRECTION_SOUTHEAST)
        m_walkYStep = 16;
    newBaseHex = m_hex;
    hexcell tempCell;
    hexcell loc;
    if (newReverse) {
        theTargetHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(theTargetHex)) {
            tempCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
            if (m_stats.attributes & MONSTER_FLAGS_WIDE)
                loc.TakeOccupant(&gpCombatManager->m_hexCells[m_hex + (m_facing ? -1 : 1)]);
            gpCombatManager->m_hexCells[theTargetHex].TakeOccupant(&tempCell);
            partnerHexVal = theTargetHex + (m_facing ? -1 : 1);
            if (ValidHex(partnerHexVal) && (m_stats.attributes & MONSTER_FLAGS_WIDE))
                gpCombatManager->m_hexCells[partnerHexVal].TakeOccupant(&loc);
            m_hex = theTargetHex;
            if (m_walkYStep)
                m_walkYStep = -m_walkYStep;
        }
    } else {
        toggleOn = 0;
        if (m_facing == ARMY_FACING_RIGHT && direction == COMBAT_DIRECTION_SOUTHEAST)
            toggleOn = 1;
        else if (m_facing == ARMY_FACING_LEFT && direction == COMBAT_DIRECTION_NORTHWEST)
            toggleOn = 1;
        gpCombatManager->SetGridMode(toggleOn);
    }
    m_animationSequence = ARMY_ANIMATION_WALK;
    m_animationFrame = startFrame;
    PlaySample(m_samples[ARMY_SAMPLE_MOVE]);
    if (!continued) {
        if (ValidHex(m_hex))
            gpCombatManager->m_hexCells[m_hex].m_occupantSide = COMBAT_SIDE_NONE;
        gpCombatManager->DrawFrame(0);
        if (ValidHex(m_hex))
            gpCombatManager->m_hexCells[m_hex].m_occupantSide = gpCombatManager->m_currentSide;
        gpWindowManager->m_screen->CopyTo(
            gpCombatManager->m_backgroundBuffer,
            0,
            0,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            COMBAT_VIEW_HEIGHT
        );
        gpCombatManager->m_backgroundDrawn = 0;
    }
    for (i = 0; i < 6; i++) {
        if (continued || i) {
            gpCombatManager->m_backgroundBuffer->CopyTo(
                gpWindowManager->m_screen,
                giMinExtentX,
                giMinExtentY,
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            col = giMinExtentX;
            ourMinY = giMinExtentY;
            rectMaxX = giMaxExtentX;
            ourMaxY = giMaxExtentY;
        }
        giMinExtentX = giMinExtentY = COMBAT_EXTENT_MIN_START;
        giMaxExtentX = giMaxExtentY = 0;
        gComputeExtent = 1;
        gSaveBiggestExtent = 1;
        DrawToBuffer(
            gpCombatManager->m_hexCells[m_hex].m_x,
            gpCombatManager->m_hexCells[m_hex].m_y
        );
        gComputeExtent = 0;
        gSaveBiggestExtent = 0;
        if (giMinExtentX < 0)
            giMinExtentX = 0;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
            giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
        if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        gCurrArmyDrawn = 0;
        gComputeExtent = 1;
        gLimitToExtent = 1;
        m_drawShadow = 0;
        gpCombatManager->DrawFrame(0);
        m_drawShadow = 1;
        gLimitToExtent = 0;
        gComputeExtent = 0;
        gCurrArmyDrawn = 1;
        if (giMinExtentX < col)
            col = giMinExtentX;
        if (giMinExtentY < ourMinY)
            ourMinY = giMinExtentY;
        if (giMaxExtentX > rectMaxX)
            rectMaxX = giMaxExtentX;
        if (giMaxExtentY > ourMaxY)
            ourMaxY = giMaxExtentY;
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        UPDATE_INCLUSIVE_REGION(col, ourMinY, rectMaxX, ourMaxY);
        m_animationFrame += stepCount;
    }
    if (!newReverse) {
        theTargetHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(theTargetHex)) {
            tempCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
            nextTail = m_hex + (m_facing ? -1 : 1);
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && ValidHex(nextTail))
                loc.TakeOccupant(&gpCombatManager->m_hexCells[nextTail]);
            gpCombatManager->m_hexCells[theTargetHex].TakeOccupant(&tempCell);
            nextTail = theTargetHex + (m_facing ? -1 : 1);
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && ValidHex(nextTail))
                gpCombatManager->m_hexCells[nextTail].TakeOccupant(&loc);
            m_hex = theTargetHex;
        }
    }
    if (standAfter == 1)
        Stand(1);
}

// A ranged attack: turn toward the target, animate the missile hex by hex
// over a saved screen patch, apply wall and luck modifiers, report the
// damage; creature 14 shoots twice.
VA(0x00414854, 0xc74)
void army::SpecialAttack(void) {
    DATA(0x004a67d8)
    static i32 gSecondShot = 0;
    i8 frameIndex;
    i32 oldTipX;
    i8 originalColumn;
    i8 drawFlipped;
    i32 oldTipY;
    i8 hisCol;
    i8 originalRow;
    i32 gainX;
    i32 inFlightX;
    i32 gainY;
    i32 inFlightY;
    i32 i;
    i32 landX;
    i8 hisRow;
    i8 slopeDirection;
    i32 landY;
    i32 adjustX;
    i32 clipTop;
    i32 clipLeft;
    i32 k;
    bitmap* backing;
    i32 aimColumn;
    i32 wasFacing;
    army* hisStack;
    i32 fullXLen;
    i32 flightSteps;
    i32 startX;
    i32 launchX;
    i32 y1;
    i32 maxX;
    i32 adjustY;
    i32 killed;
    i32 fullYLen;
    i32 liftOffsets[5];
    i32 startY;
    i32 landPosX;
    i32 y2;
    i32 maxY;
    i8 wallPenalty;
    i32 damageDone;

    wasFacing = m_facing;
    m_walkYStep = 0;
    if (m_targetSide < 0 || m_targetIndex < 0)
        return;
    hisStack = &gpCombatManager->m_armies[m_targetSide][m_targetIndex];
    hisCol = hisStack->m_hex % COMBAT_GRID_COLUMNS;
    hisRow = hisStack->m_hex / COMBAT_GRID_COLUMNS;
    originalColumn = m_hex % COMBAT_GRID_COLUMNS;
    originalRow = m_hex / COMBAT_GRID_COLUMNS;
    wasFacing = m_facing;
    if (hisCol > originalColumn || !(originalRow & 1) && hisCol == originalColumn)
        m_facing = ARMY_FACING_RIGHT;
    else
        m_facing = ARMY_FACING_LEFT;
    gpCombatManager->SetGridMode(
        m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(1) : static_cast<i8>(0)
    );
    CheckLuck();
    m_animationSequence = ARMY_ANIMATION_ATTACK;
    PlaySample(m_samples[ARMY_SAMPLE_SHOOT]);
    for (i = 0; i < 4; i++) {
        m_animationFrame = i + 1;
        gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
        gpCombatManager->DrawFrame(1);
    }
    aimColumn = hisCol;
    if (hisStack->m_stats.attributes & MONSTER_FLAGS_WIDE) {
        aimColumn += hisStack->m_facing == ARMY_FACING_LEFT ? -1 : 1;
    }
    fullXLen = aimColumn - originalColumn;
    drawFlipped = ICON_DRAW_NORMAL;
    if (fullXLen < 0) {
        drawFlipped = ICON_DRAW_FLIPPED;
        fullXLen = -fullXLen;
    }
    fullYLen = hisRow - originalRow;
    if (fullYLen < 0)
        fullYLen = -fullYLen;
    flightSteps = __max(fullXLen, fullYLen);
    frameIndex = 7;
    slopeDirection = 0;
    if (originalRow > hisRow)
        slopeDirection = -1;
    else if (originalRow < hisRow)
        slopeDirection = 1;
    if (fullYLen > 1)
        frameIndex += slopeDirection;
    if (fullXLen <= 3) {
        if (fullYLen > 2)
            frameIndex += slopeDirection;
        if (fullYLen == 1)
            frameIndex += slopeDirection;
    }
    liftOffsets[0] = -20;
    liftOffsets[1] = -15;
    liftOffsets[2] = 0;
    liftOffsets[3] = 15;
    liftOffsets[4] = 20;
    startX = gpCombatManager->m_hexCells[m_hex].m_x + (m_facing == ARMY_FACING_RIGHT ? 80 : -80);
    startY = gpCombatManager->m_hexCells[m_hex].m_y - 90 + liftOffsets[frameIndex - 5];
    landX = gpCombatManager->m_hexCells[hisRow * COMBAT_GRID_COLUMNS + aimColumn].m_x;
    landY = gpCombatManager->m_hexCells[hisRow * COMBAT_GRID_COLUMNS].m_y - 90;
    if (fullXLen == 0)
        gainX = 0;
    else
        gainX = (landX - startX) / (flightSteps * 2);
    if (fullYLen == 0)
        gainY = 0;
    else
        gainY = (landY - startY) / (flightSteps * 2);
    launchX = startX + gainX;
    landPosX = landX - flightSteps * 2 * gainX;
    adjustX = (launchX + landPosX) / 2 - launchX;
    y1 = startY + gainY;
    y2 = landY - flightSteps * 2 * gainY;
    adjustY = (y1 + y2) / 2 - y1;
    inFlightX = startX + adjustX;
    inFlightY = startY + adjustY;
    maxX = 0;
    clipLeft = LOGICAL_SCREEN_WIDTH - 1;
    maxY = 0;
    clipTop = LOGICAL_SCREEN_HEIGHT - 1;
    backing = new bitmap(BITMAP_TYPE_MEMORY, ARMY_MISSILE_PATCH_WIDTH, ARMY_MISSILE_PATCH_HEIGHT);
    backing->GrabBitmap(
        gpWindowManager->m_screen,
        inFlightX - ARMY_MISSILE_HALF_WIDTH,
        inFlightY - ARMY_MISSILE_HALF_HEIGHT
    );
    oldTipX = inFlightX;
    oldTipY = inFlightY;
    for (k = 0; k < flightSteps * 2; k++) {
        backing->DrawToBuffer(
            oldTipX - ARMY_MISSILE_HALF_WIDTH,
            oldTipY - ARMY_MISSILE_HALF_HEIGHT
        );
        if (oldTipX - ARMY_MISSILE_HALF_WIDTH < clipLeft)
            clipLeft = oldTipX - ARMY_MISSILE_HALF_WIDTH;
        if (oldTipX + ARMY_MISSILE_HALF_WIDTH > maxX)
            maxX = oldTipX + ARMY_MISSILE_HALF_WIDTH;
        if (oldTipY - ARMY_MISSILE_HALF_HEIGHT < clipTop)
            clipTop = oldTipY - ARMY_MISSILE_HALF_HEIGHT;
        if (oldTipY + ARMY_MISSILE_HALF_HEIGHT > maxY)
            maxY = oldTipY + ARMY_MISSILE_HALF_HEIGHT;
        backing->GrabBitmap(
            gpWindowManager->m_screen,
            inFlightX - ARMY_MISSILE_HALF_WIDTH,
            inFlightY - ARMY_MISSILE_HALF_HEIGHT
        );
        m_attackIcon
            ->DrawToBuffer(inFlightX, inFlightY, frameIndex, drawFlipped, ICON_DRAW_OFFSET_FULL);
        if (inFlightX - ARMY_MISSILE_HALF_WIDTH < clipLeft)
            clipLeft = inFlightX - ARMY_MISSILE_HALF_WIDTH;
        if (inFlightX + ARMY_MISSILE_HALF_WIDTH > maxX)
            maxX = inFlightX + ARMY_MISSILE_HALF_WIDTH;
        if (inFlightY - ARMY_MISSILE_HALF_HEIGHT < clipTop)
            clipTop = inFlightY - ARMY_MISSILE_HALF_HEIGHT;
        if (inFlightY + ARMY_MISSILE_HALF_HEIGHT > maxY)
            maxY = inFlightY + ARMY_MISSILE_HALF_HEIGHT;
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
        UPDATE_INCLUSIVE_REGION(clipLeft, clipTop, maxX, maxY);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 15;
        oldTipX = inFlightX;
        oldTipY = inFlightY;
        inFlightX += gainX;
        inFlightY += gainY;
    }
    backing->DrawToBuffer(oldTipX - ARMY_MISSILE_HALF_WIDTH, oldTipY - ARMY_MISSILE_HALF_HEIGHT);
    gpWindowManager->UpdateScreenRegion(
        oldTipX - ARMY_MISSILE_HALF_WIDTH,
        oldTipY - ARMY_MISSILE_HALF_HEIGHT,
        ARMY_MISSILE_PATCH_WIDTH,
        ARMY_MISSILE_PATCH_HEIGHT
    );
    delete backing;
    m_stats.shots--;
    wallPenalty = 0;
    if (gpCombatManager->m_castleSide[COMBAT_DEFENDER_SIDE]
        && m_hex % COMBAT_GRID_COLUMNS <= COMBAT_CASTLE_WALL_COLUMN - 1
        && hisStack->m_hex % COMBAT_GRID_COLUMNS >= COMBAT_CASTLE_WALL_COLUMN + 1) {
        i32 wallDistance;
        i32 aimRow;
        i32 targetCol;
        i32 srcCol;
        i32 unusedHex;
        i32 hitRow;
        i32 archerRow;
        i32 pastWall;

        srcCol = m_hex % COMBAT_GRID_COLUMNS;
        archerRow = m_hex / COMBAT_GRID_COLUMNS;
        pastWall = srcCol - COMBAT_CASTLE_WALL_COLUMN;
        targetCol = hisStack->m_hex % COMBAT_GRID_COLUMNS;
        aimRow = hisStack->m_hex / COMBAT_GRID_COLUMNS;
        wallDistance = COMBAT_CASTLE_WALL_COLUMN - srcCol;
        hitRow = aimRow;
        if (abs(aimRow - archerRow) >= 2)
            hitRow -= (aimRow - archerRow) / 2;
        if (abs(aimRow - archerRow) % 2 == 1) {
            if (pastWall < wallDistance
                || pastWall == wallDistance
                       && (archerRow == COMBAT_UPPER_WALL_ROW
                           || archerRow == COMBAT_LOWER_WALL_ROW)) {
                if (archerRow < aimRow)
                    hitRow--;
                else
                    hitRow++;
            }
        }
        if (hitRow > COMBAT_GRID_LAST_ROW)
            hitRow = COMBAT_GRID_LAST_ROW;
        if (hitRow < 0)
            hitRow = 0;
        wallPenalty =
            gpCombatManager->m_hexCells[hitRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN]
                    .m_obstacleIndex
                == COMBAT_WALL_DAMAGED
            || gpCombatManager->m_hexCells[hitRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN]
                       .m_obstacleIndex
                   == COMBAT_WALL_INTACT;
    }
    DamageEnemy(
        hisStack,
        &damageDone,
        &killed,
        1,
        wallPenalty ? ARMY_CASTLE_WALL_DEFENSE_BONUS : 0
    );
    if (killed > 0) {
        strcpy(gTargetName, gArmyNames[hisStack->m_creatureType]);
        gTargetName[0] = CyrillicToLower(gTargetName[0]);
        sprintf(
            gText,
            "%s %s %s %d %s.  %d %s %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            damageDone,
            localization::Tr("combat.fragment.damage_points"),
            killed,
            killed <= 1 ? gTargetName : gArmyNamesPlural[hisStack->m_creatureType],
            killed <= 1 ? localization::Tr("combat.fragment.dies")
                        : localization::Tr("combat.fragment.killed")
        );
    } else
        sprintf(
            gText,
            "%s %s %s %d %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            damageDone,
            localization::Tr("combat.fragment.damage_points")
        );
    gText[0] = CyrillicToUpper(gText[0]);
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.powEffect);
    if (!(hisStack->m_stats.attributes & MONSTER_FLAGS_DEAD))
        hisStack->Stand(0);
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK)
        CancelSpell();
    WaitSample(m_samples[ARMY_SAMPLE_SHOOT]);
    m_facing = wasFacing;
    Stand(1);
    if (hisStack->m_quantity > 0)
        hisStack->Stand(1);
    if (!gSecondShot && m_creatureType == CREATURE_ELF && hisStack->m_quantity > 0) {
        gSecondShot = 1;
        SpecialAttack();
        gSecondShot = 0;
    }
}

// Attacks every enemy next to the stack (the hydra), then turns them back.
VA(0x004154c8, 0x763)
void army::DoHydraAttack(void) {
    i32 killedNow;
    i32 damage;
    i16 newSide;
    i16 i;
    i16 dir;
    i16 slot;
    i16 attackMask;
    army* pTarget;
    i16 targetHexValue;
    i32 curLost;
    i32 newDmg;
    army* eachArmyRef;

    m_walkYStep = 0;
    CheckLuck();
    PlaySample(m_samples[ARMY_SAMPLE_ATTACK]);
    m_animationSequence = ARMY_ANIMATION_STAND;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    for (i = 0; i < 5; i++) {
        gpCombatManager->m_computeExtent = 1;
        m_animationFrame = i + 3;
        gpCombatManager->DrawFrame(1);
    }
    if (m_spellEffect == SPELL_BERZERKER)
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
    else
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
    gpCombatManager->ResetHitByCreature();
    curLost = 0;
    newDmg = curLost;
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (!(attackMask & (1 << dir))) {
            targetHexValue = m_hex;
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_facing == ARMY_FACING_LEFT && dir > COMBAT_DIRECTION_EASTERN_LAST
                    || m_facing == ARMY_FACING_RIGHT
                           && (dir < COMBAT_DIRECTION_WESTERN_FIRST
                               || dir > COMBAT_DIRECTION_WESTERN_LAST))) {
                if (m_facing == ARMY_FACING_LEFT)
                    targetHexValue = m_hex - 1;
                else
                    targetHexValue = m_hex + 1;
            }
            targetHexValue = GetAdjacentCellIndex(targetHexValue, dir);
            if (ValidHex(targetHexValue)) {
                newSide = gpCombatManager->m_hexCells[targetHexValue].m_occupantSide;
                slot = gpCombatManager->m_hexCells[targetHexValue].m_occupantIndex;
                if (newSide >= 0 && slot >= 0) {
                    gpCombatManager->m_limitCreatureCount[newSide][slot]++;
                    pTarget = &gpCombatManager->m_armies[newSide][slot];
                    if (!pTarget->m_hitByCreature) {
                        pTarget->m_hitByCreature = 1;
                        DamageEnemy(pTarget, &damage, &killedNow, 0, 0);
                        newDmg += damage;
                        curLost += killedNow;
                    }
                }
            }
        }
    }
    if (curLost > 0)
        sprintf(
            gText,
            "%s %s %s %d %s.  %d %s %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            newDmg,
            localization::Tr("combat.fragment.damage_points"),
            curLost,
            curLost <= 1 ? localization::Tr("combat.fragment.troop")
                         : localization::Tr("combat.fragment.troops"),
            curLost <= 1 ? localization::Tr("combat.fragment.dies")
                         : localization::Tr("combat.fragment.killed")
        );
    else
        sprintf(
            gText,
            "%s %s %s %d %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            newDmg,
            localization::Tr("combat.fragment.damage_points")
        );
    gText[0] = CyrillicToUpper(gText[0]);
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.powEffect);
    WaitSample(m_samples[ARMY_SAMPLE_ATTACK]);
    m_animationSequence = ARMY_ANIMATION_STAND;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    for (i = 4; i >= 0; i--) {
        gpCombatManager->m_computeExtent = 1;
        m_animationFrame = i + 3;
        gpCombatManager->DrawFrame(1);
    }
    if (m_spellEffect == SPELL_BERZERKER)
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
    else
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (!(attackMask & (1 << dir))) {
            targetHexValue = m_hex;
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_facing == ARMY_FACING_LEFT && dir > COMBAT_DIRECTION_EASTERN_LAST
                    || m_facing == ARMY_FACING_RIGHT
                           && (dir < COMBAT_DIRECTION_WESTERN_FIRST
                               || dir > COMBAT_DIRECTION_WESTERN_LAST))) {
                if (m_facing == ARMY_FACING_LEFT)
                    targetHexValue = m_hex - 1;
                else
                    targetHexValue = m_hex + 1;
            }
            targetHexValue = GetAdjacentCellIndex(targetHexValue, dir);
            if (ValidHex(targetHexValue)) {
                newSide = gpCombatManager->m_hexCells[targetHexValue].m_occupantSide;
                slot = gpCombatManager->m_hexCells[targetHexValue].m_occupantIndex;
                if (newSide >= 0 && slot >= 0) {
                    gpCombatManager->m_limitCreatureCount[newSide][slot]++;
                    pTarget = &gpCombatManager->m_armies[newSide][slot];
                    if (!(pTarget->m_stats.attributes & MONSTER_FLAGS_DEAD))
                        pTarget->Stand(0);
                }
            }
        }
    }
    m_targetSide = targetHexValue = ARMY_HEX_INVALID;
    gpCombatManager->m_computeExtent = 0;
    for (newSide = 0; newSide < COMBAT_SIDE_COUNT; newSide++) {
        for (slot = 0; slot < gpCombatManager->m_numArmies[newSide]; slot++) {
            eachArmyRef = &gpCombatManager->m_armies[newSide][slot];
            if (!(eachArmyRef->m_stats.attributes & MONSTER_FLAGS_DEAD)
                || eachArmyRef->m_powFrames == ARMY_POW_NONE)
                eachArmyRef->Stand(0);
        }
    }
    gpCombatManager->DrawFrame(1);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00415c2b, 0x22)
void army::DirDoAttack(i16 direction) {
    m_attackDirection = direction;
    DoAttack(0);
}

// A melee strike in m_attackDirection: breath attackers (attribute 8) also
// hit the hex behind, some creatures cast on the target, the target
// retaliates once, and creatures 5 and 8 strike twice.
VA(0x00415c4d, 0x1092)
void army::DoAttack(i32 retaliation) {
    i32 ignored;
    i32 oldMode;
    i16 frameBase;
    army* target2Info;
    i32 nextDmg;
    i16 lastHex;
    i32 myDir;
    i16 facing;
    i32 way;
    i32 castOk;
    army* targetPtr;
    i32 kills;

    oldMode = 0;
    nextDmg = 0;
    kills = 0;
    castOk = 0;
    if (retaliation)
        gpCombatManager->m_currentSide = 1 - gpCombatManager->m_currentSide;
    if (m_creatureType == CREATURE_HYDRA) {
        DoHydraAttack();
        if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK && !retaliation)
            CancelSpell();
        goto secondStrike;
    }
    way = m_attackDirection;
    facing = m_facing;
    m_walkYStep = 0;
    if (m_attackDirection <= COMBAT_DIRECTION_EASTERN_LAST)
        m_facing = ARMY_FACING_RIGHT;
    else if (m_attackDirection <= COMBAT_DIRECTION_WESTERN_LAST)
        m_facing = ARMY_FACING_LEFT;
    if (m_attackDirection == COMBAT_DIRECTION_NORTHWEST
        || m_attackDirection == COMBAT_DIRECTION_NORTHEAST
        || m_attackDirection == COMBAT_DIRECTION_WIDE_WEST)
        frameBase = 6;
    else if (m_attackDirection == COMBAT_DIRECTION_SOUTHWEST
             || m_attackDirection == COMBAT_DIRECTION_SOUTHEAST
             || m_attackDirection == COMBAT_DIRECTION_WIDE_EAST)
        frameBase = 8;
    else
        frameBase = 7;
    gpCombatManager->SetGridMode(
        m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(1) : static_cast<i8>(0)
    );
    CheckLuck();
    lastHex = m_hex;
    if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
        && (facing == ARMY_FACING_LEFT && m_attackDirection >= COMBAT_DIRECTION_WESTERN_FIRST
            || facing == ARMY_FACING_RIGHT
                   && (m_attackDirection <= COMBAT_DIRECTION_EASTERN_LAST
                       || m_attackDirection >= COMBAT_DIRECTION_WIDE_FIRST))) {
        if (facing == ARMY_FACING_LEFT)
            lastHex = m_hex - 1;
        else
            lastHex = m_hex + 1;
    }
    lastHex = GetAdjacentCellIndex(lastHex, m_attackDirection);
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        i16 behindHex;

        if (ValidHex(lastHex) && gpCombatManager->m_hexCells[lastHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[lastHex].m_occupantIndex >= 0)
            gpCombatManager
                ->m_limitCreatureCount[gpCombatManager->m_hexCells[lastHex].m_occupantSide]
                                      [gpCombatManager->m_hexCells[lastHex].m_occupantIndex]++;
        behindHex = GetAdjacentCellIndex(lastHex, m_attackDirection);
        if (ValidHex(behindHex) && gpCombatManager->m_hexCells[behindHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[behindHex].m_occupantIndex >= 0) {
            gpCombatManager
                ->m_limitCreatureCount[gpCombatManager->m_hexCells[behindHex].m_occupantSide]
                                      [gpCombatManager->m_hexCells[behindHex].m_occupantIndex]++;
            if (m_attackDirection == COMBAT_DIRECTION_SOUTHEAST
                || m_attackDirection == COMBAT_DIRECTION_SOUTHWEST)
                gpCombatManager->m_extendLimitDown = 1;
        }
    }
    oldMode = gpCombatManager->m_extendLimitDown;
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 3;
    gpCombatManager->DrawFrame(1);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    PlaySample(m_samples[ARMY_SAMPLE_ATTACK]);
    m_animationFrame = 4;
    gpCombatManager->m_computeExtent = 1;
    gpCombatManager->DrawFrame(1);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    m_animationFrame = frameBase;
    gpCombatManager->m_computeExtent = 1;
    gpCombatManager->DrawFrame(1);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        m_animationFrame = frameBase + 3;
        gpCombatManager->m_computeExtent = 1;
        gpCombatManager->DrawFrame(1);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    }
    target2Info = NULL;
    targetPtr = NULL;
    if (ValidHex(lastHex)) {
        i32 newKilled;
        i16 nextHex;

        if (gpCombatManager->m_hexCells[lastHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[lastHex].m_occupantIndex >= 0) {
            targetPtr =
                &gpCombatManager->m_armies[gpCombatManager->m_hexCells[lastHex].m_occupantSide]
                                          [gpCombatManager->m_hexCells[lastHex].m_occupantIndex];
            gpCombatManager->m_limitCreatureCount[targetPtr->m_side][targetPtr->m_index]++;
            gpCombatManager->m_computeExtent = 1;
            DamageEnemy(targetPtr, &nextDmg, &kills, 0, 0);
        }
        newKilled = kills;
        nextHex = GetAdjacentCellIndex(lastHex, m_attackDirection);
        if ((m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)
            && m_attackDirection < COMBAT_DIRECTION_ADJACENT_COUNT && ValidHex(nextHex)
            && gpCombatManager->m_hexCells[nextHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[nextHex].m_occupantIndex >= 0
            && gpCombatManager->m_hexCells[nextHex].m_occupantIndex
                   != gpCombatManager->m_hexCells[lastHex].m_occupantIndex) {
            gpCombatManager
                ->m_limitCreatureCount[gpCombatManager->m_hexCells[nextHex].m_occupantSide]
                                      [gpCombatManager->m_hexCells[nextHex].m_occupantIndex]++;
            lastHex = nextHex;
            if (ValidHex(lastHex)
                && gpCombatManager->m_hexCells[lastHex].m_occupantSide != COMBAT_SIDE_NONE
                && gpCombatManager->m_hexCells[lastHex].m_occupantIndex != COMBAT_ARMY_INDEX_NONE) {
                m_animationFrame = frameBase + 6;
                gpCombatManager->m_computeExtent = 1;
                gpCombatManager->DrawFrame(1);
                target2Info = &gpCombatManager
                                   ->m_armies[gpCombatManager->m_hexCells[lastHex].m_occupantSide]
                                             [gpCombatManager->m_hexCells[lastHex].m_occupantIndex];
                DamageEnemy(target2Info, &nextDmg, &kills, 0, 0);
                if (target2Info->m_quantity > 0)
                    target2Info->Stand(1);
            }
        }
        kills = newKilled;
    }
    if (gGenieHalf)
        sprintf(
            gText,
            localization::Tr("combat.genie.half_army.buka"),
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity <= 1 ? localization::Tr("combat.fragment.destroy_singular")
                            : localization::Tr("combat.fragment.destroy_plural")
        );
    else if (kills > 0) {
        strcpy(gTargetName, gArmyNames[targetPtr->m_creatureType]);
        gTargetName[0] = CyrillicToLower(gTargetName[0]);
        sprintf(
            gText,
            "%s %s %s %d %s.  %d %s %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            nextDmg,
            localization::Tr("combat.fragment.damage_points"),
            kills,
            kills <= 1 ? gTargetName : gArmyNamesPlural[targetPtr->m_creatureType],
            kills <= 1 ? localization::Tr("combat.fragment.dies")
                       : localization::Tr("combat.fragment.killed")
        );
    } else
        sprintf(
            gText,
            "%s %s %s %d %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            nextDmg,
            localization::Tr("combat.fragment.damage_points")
        );
    gText[0] = CyrillicToUpper(gText[0]);
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.powEffect);
    gpCombatManager->m_extendLimitDown = oldMode;
    switch (m_creatureType) {
        case CREATURE_CYCLOPS:
            if (SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_SPECIAL_ROLL_HIT) {
                if (targetPtr && targetPtr->m_spellEffect != SPELL_ANTI_MAGIC
                    && targetPtr->m_creatureType != CREATURE_DRAGON
                    && (targetPtr->m_creatureType != CREATURE_DWARF || SRandom(0, 4) != 1)
                    && !(targetPtr->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                    gpCombatManager
                        ->CastSpell(SPELL_PARALYZE, targetPtr->m_hex, 1, ARMY_HEX_INVALID);
                    castOk = 1;
                }
            } else if (SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_SPECIAL_ROLL_HIT && target2Info
                       && target2Info->m_spellEffect != SPELL_ANTI_MAGIC
                       && target2Info->m_creatureType != CREATURE_DRAGON
                       && (target2Info->m_creatureType != CREATURE_DWARF || SRandom(0, 4) != 1)
                       && !(target2Info->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                gpCombatManager->CastSpell(SPELL_PARALYZE, target2Info->m_hex, 1, ARMY_HEX_INVALID);
                castOk = 1;
            }
            break;
        case CREATURE_UNICORN:
            if (SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_SPECIAL_ROLL_HIT && targetPtr
                && targetPtr->m_spellEffect != SPELL_ANTI_MAGIC
                && targetPtr->m_creatureType != CREATURE_DRAGON
                && (targetPtr->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(targetPtr->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                gpCombatManager->CastSpell(SPELL_BLIND, targetPtr->m_hex, 1, ARMY_HEX_INVALID);
                castOk = 1;
            }
            break;
        case CREATURE_GHOST:
            gpCombatManager->m_ghostKills[gpCombatManager->m_hexCells[m_hex].m_occupantSide] =
                kills;
            break;
        default:
            break;
    }
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_extendLimitDown = oldMode;
    gpCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    if (targetPtr) {
        gpCombatManager->m_limitCreatureCount[targetPtr->m_side][targetPtr->m_index] = 1;
        if (!(targetPtr->m_stats.attributes & MONSTER_FLAGS_DEAD))
            targetPtr->Stand(0);
    }
    if (target2Info) {
        gpCombatManager->m_limitCreatureCount[target2Info->m_side][target2Info->m_index] = 1;
        if (!(target2Info->m_stats.attributes & MONSTER_FLAGS_DEAD))
            target2Info->Stand(0);
    }
    WaitSample(m_samples[ARMY_SAMPLE_ATTACK]);
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        if (target2Info) {
            m_animationFrame = frameBase + 6;
            gpCombatManager->m_computeExtent = 1;
            gpCombatManager->DrawFrame(1);
        }
        m_animationFrame = frameBase + 3;
        gpCombatManager->m_computeExtent = 1;
        gpCombatManager->DrawFrame(1);
        m_animationFrame = frameBase;
        gpCombatManager->m_computeExtent = 1;
        gpCombatManager->DrawFrame(1);
    }
    m_animationFrame = 4;
    gpCombatManager->m_computeExtent = 1;
    gpCombatManager->DrawFrame(1);
    m_animationFrame = 3;
    gpCombatManager->m_computeExtent = 1;
    gpCombatManager->DrawFrame(1);
    gpCombatManager->m_computeExtent = 1;
    Stand(1);
    m_facing = facing;
    gpCombatManager->m_computeExtent = 1;
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK && !retaliation)
        CancelSpell();
    gpCombatManager->m_computeExtent = 1;
    Stand(1);
    if (m_creatureType == CREATURE_GHOST)
        m_quantity +=
            gpCombatManager->m_ghostKills[gpCombatManager->m_hexCells[m_hex].m_occupantSide];
    if (targetPtr && targetPtr->m_quantity > 0) {
        gpCombatManager->m_computeExtent = 1;
        targetPtr->Stand(1);
        if (targetPtr->m_spellEffect == SPELL_PARALYZE
            || targetPtr->m_creatureType != CREATURE_GRIFFIN
                   && (targetPtr->m_stats.attributes & MONSTER_FLAGS_RETALIATED)
            || m_creatureType == CREATURE_ROGUE || m_creatureType == CREATURE_SPRITE || castOk
            || retaliation) {
            goto secondStrike;
        } else {
            targetPtr->m_attackDirection = OppositeDirection(m_attackDirection);
            if (targetPtr->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                i16 checkHex;

                checkHex = GetAdjacentCellIndex(
                    targetPtr->m_hex,
                    targetPtr->m_facing ? static_cast<i8>(COMBAT_DIRECTION_NORTHWEST)
                                        : static_cast<i8>(COMBAT_DIRECTION_NORTHEAST)
                );
                if (checkHex == m_hex)
                    targetPtr->m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                checkHex = GetAdjacentCellIndex(
                    targetPtr->m_hex,
                    targetPtr->m_facing ? static_cast<i8>(COMBAT_DIRECTION_SOUTHWEST)
                                        : static_cast<i8>(COMBAT_DIRECTION_SOUTHEAST)
                );
                if (checkHex == m_hex)
                    targetPtr->m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
            }
            targetPtr->DoAttack(1);
            targetPtr->m_stats.attributes |= MONSTER_FLAGS_RETALIATED;
            if (targetPtr->m_creatureType == CREATURE_GHOST)
                targetPtr->m_quantity +=
                    gpCombatManager->m_ghostKills[gpCombatManager->m_hexCells[targetPtr->m_hex]
                                                      .m_occupantSide];
        }
    }
secondStrike:
    if ((m_creatureType == CREATURE_WOLF || m_creatureType == CREATURE_PALADIN) && targetPtr
        && targetPtr->m_quantity > 0 && !retaliation && m_spellEffect != SPELL_PARALYZE
        && m_quantity > 0) {
        myDir = m_attackDirection;
        m_attackDirection = way;
        DoAttack(1);
        m_attackDirection = myDir;
    }
    m_targetSide = lastHex = ARMY_HEX_INVALID;
    if (retaliation)
        gpCombatManager->m_currentSide = 1 - gpCombatManager->m_currentSide;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00416cdf, 0x3d)
void army::ResetPath(void) {
    i16 i;

    for (i = 0; i < COMBAT_HEX_COUNT; i++)
        gpCombatManager->m_hexCells[i].m_pathFlag = 0;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00416d1c, 0x1c)
i16 army::WalkTo(void) {
    return WalkTo(m_moveTargetHex);
}

// Walks the found path one hex at a time, at most the stack's speed.
VA(0x00416d38, 0xe6)
i16 army::WalkTo(i16 destHex) {
    i32 stepCount;
    i8 pathIndex;

    m_targetSide = m_targetIndex = COMBAT_ARMY_INDEX_NONE;
    if (!FindPath(m_hex, destHex, m_stats.speed, 1, ARMY_PATH_ANY_TARGET_HEX))
        return ARMY_PATH_BLOCKED;
    stepCount = 0;
    for (pathIndex = gpSearchArray->m_pathLength - 1; pathIndex >= 0; pathIndex--) {
        Walk(
            gpSearchArray->m_directions[pathIndex],
            0,
            pathIndex != gpSearchArray->m_pathLength - 1
        );
        stepCount++;
        if (stepCount >= m_stats.speed)
            pathIndex = -1;
    }
    if (!m_spellEndCondition)
        CancelSpell();
    Stand(1);
    return 0;
}

VA(0x00416e1e, 0x1c)
i16 army::AttackTo(void) {
    return AttackTo(m_moveTargetHex);
}

// Flyers jump next to the target; walkers stop short when out of moves.
VA(0x00416e3a, 0x1a2)
i16 army::AttackTo(i16 destHex) {
    i32 stepCount;
    i8 pathIndex;

    if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
        if (m_hex != destHex)
            FlyTo(destHex);
        DoAttack(0);
        return 0;
    }
    if ((m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) && m_moveTargetHex == m_hex) {
        DoAttack(0);
        return 0;
    }
    if (FindPath(m_hex, destHex, m_stats.speed, 1, ARMY_PATH_ANY_TARGET_HEX)) {
        if (gpSearchArray->m_pathLength == 1) {
            m_attackDirection = gpSearchArray->m_directions[0];
            DoAttack(0);
        } else {
            pathIndex = 0;
            stepCount = 0;
            for (pathIndex = gpSearchArray->m_pathLength - 1; pathIndex; pathIndex--) {
                Walk(
                    gpSearchArray->m_directions[pathIndex],
                    0,
                    pathIndex != gpSearchArray->m_pathLength - 1
                );
                stepCount++;
                if (stepCount >= m_stats.speed && pathIndex != 1) {
                    Stand(1);
                    return ARMY_PATH_BLOCKED;
                }
            }
            if (!m_spellEndCondition)
                CancelSpell();
            m_attackDirection = gpSearchArray->m_directions[0];
            DoAttack(0);
        }
        return 0;
    }
    return ARMY_PATH_BLOCKED;
}

VA(0x00416fdc, 0x1a0)
void army::CheckLuck(void) {
    i32 luck;

    if (!gpCombatManager->m_heroes[m_side])
        return;
    m_luck = ARMY_LUCK_NONE;
    luck = gpGame->GetLuck(gpCombatManager->m_heroes[m_side], this);
    if (luck > 0 && SRandom(1, 12) <= luck)
        m_luck = ARMY_LUCK_GOOD;
    if (luck < 0 && SRandom(1, 12) < -luck)
        m_luck = ARMY_LUCK_BAD;
    if (m_luck) {
        class sample* sample;
        if (m_luck < 0)
            sprintf(gText, "badluck.82m");
        else
            sprintf(gText, "goodluck.82m");
        sample = LoadPlaySample(gText);
        if (m_luck < 0) {
            sprintf(
                gText,
                localization::Tr("combat.luck.bad.buka"),
                gArmyNamesPlural[m_creatureType]
            );
            gpCombatManager->CombatMessage(gText, 1);
            Wince();
            SpellEffect(COMBAT_EFFECT_BAD_LUCK, 180);
        } else {
            sprintf(
                gText,
                localization::Tr("combat.luck.good.buka"),
                gArmyNamesPlural[m_creatureType]
            );
            gpCombatManager->CombatMessage(gText, 1);
            Stand(1);
            SpellEffect(COMBAT_EFFECT_GOOD_LUCK, 180);
        }
        Stand(1);
        WaitSample(sample);
    }
}

VA(0x0041717c, 0x215)
void army::DamageEnemy(
    class army* target,
    i32* damageResult,
    i32* killedResult,
    i32 rangedAttack,
    i32 defenseModifier
) {
    float theTotal;
    i16 mainDelta;
    i32 hurt;
    i32 damage;
    i16 nextDefenseBonus;
    i16 index;
    i16 attBonus;

    if (!target)
        return;
    theTotal = 0;
    gGenieHalf = 0;
    for (index = 0; index < m_quantity; index++) {
        switch (m_damageMode) {
            case ARMY_DAMAGE_MAXIMUM:
                theTotal += m_stats.damageMax;
                break;
            case ARMY_DAMAGE_MINIMUM:
                theTotal += m_stats.damageMin;
                break;
            default:
                theTotal += SRandom(m_stats.damageMin, m_stats.damageMax);
                break;
        }
    }
    attBonus = 0;
    nextDefenseBonus = 0;
    mainDelta =
        m_stats.attack + attBonus - (target->m_stats.defense + nextDefenseBonus + defenseModifier);
    if (mainDelta > 20)
        mainDelta = 20;
    if (mainDelta < -20)
        mainDelta = -20;
    theTotal *= gBattleStat[mainDelta + 20];
    if (m_luck > 0)
        theTotal *= 2;
    if (m_luck < 0)
        theTotal /= 2;
    m_luck = ARMY_LUCK_NONE;
    if ((m_stats.attributes & MONSTER_FLAGS_SHOOTER) && !rangedAttack)
        theTotal /= 2;
    if (m_damageMode == ARMY_DAMAGE_HALF)
        theTotal /= 2;
    damage = static_cast<i32>(theTotal + 0.5);
    if (m_creatureType == CREATURE_GENIE
        && SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_GENIE_ROLL_HIT) {
        hurt = ((target->m_quantity + 1) / 2) * target->m_stats.hitPoints;
        if (hurt > damage) {
            gGenieHalf = 1;
            damage = hurt;
        }
    }
    if (damage > 32000)
        damage = 32000;
    if (damage <= 0)
        damage = 1;
    *damageResult = damage;
    *killedResult = target->Damage(damage);
}

// A stack whose spell (2) breaks on damage loses it.
VA(0x00417391, 0x160)
i32 army::Damage(i32 damage) {
    i8 oldFacing;
    i32 quantityFifth;
    i32 killed;

    damage += m_hitPointsLost;
    killed = damage / m_stats.hitPoints;
    m_hitPointsLost = damage % m_stats.hitPoints;
    quantityFifth = m_quantity / 5;
    if (quantityFifth == 0)
        quantityFifth = 1;
    if (killed > 0)
        m_powFrames = ARMY_POW_FRAMES_HIT;
    else
        m_powFrames = ARMY_POW_NONE;
    if (killed > m_quantity)
        killed = m_quantity;
    m_quantity -= killed;
    if (m_quantity <= 0)
        m_powFrames = ARMY_POW_FRAMES_KILLED;
    oldFacing = m_facing;
    m_facing = gpCombatManager
                   ->m_armies[gpCombatManager->m_currentSide][gpCombatManager->m_currentArmyIndex]
                   .m_facing
               ^ 1;
    Wince();
    m_facing = oldFacing;
    gpCombatManager->DrawFrame(1);
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_DAMAGE) {
        m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
        if (m_spellEffect != SPELL_BLIND)
            m_stats.attributes |= MONSTER_FLAGS_RETALIATED;
        CancelSpell();
    }
    return killed;
}

// Plays the impact effect on every stack hit this attack (m_powFrames),
// fading the killed ones out, then restores the grid.
VA(0x004174f1, 0x87e)
void army::PowEffect(i8 effect) {
    i16 frames;
    i16 slot;
    i16 armyNum;
    i16 theStep;
    i16 longest;
    i32 nextCellHex;
    army* curArmyPtr;
    i16 curSide;

    longest = 0;
    for (armyNum = 0; armyNum < gpCombatManager->m_numArmies[COMBAT_ATTACKER_SIDE]; armyNum++)
        if (gpCombatManager->m_armies[COMBAT_ATTACKER_SIDE][armyNum].m_powFrames > longest)
            longest = gpCombatManager->m_armies[COMBAT_ATTACKER_SIDE][armyNum].m_powFrames;
    for (armyNum = 0; armyNum < gpCombatManager->m_numArmies[COMBAT_DEFENDER_SIDE]; armyNum++)
        if (gpCombatManager->m_armies[COMBAT_DEFENDER_SIDE][armyNum].m_powFrames > longest)
            longest = gpCombatManager->m_armies[COMBAT_DEFENDER_SIDE][armyNum].m_powFrames;
    if (longest >= ARMY_POW_FRAMES_KILLED)
        frames = 10;
    else
        frames = longest;
    if (gCurLoadedSpellFileId != effect) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(gPowEffectNames[effect]);
        gCurLoadedSpellFileId = effect;
    }
    for (curSide = 0; curSide < COMBAT_SIDE_COUNT; curSide++)
        for (slot = 0; slot < gpCombatManager->m_numArmies[curSide]; slot++)
            if (gpCombatManager->m_armies[curSide][slot].m_powFrames > 0)
                PlaySample(gpCombatManager->m_armies[curSide][slot].m_samples[ARMY_SAMPLE_WINCE]);
    theStep = 0;
    gpCombatManager->ResetLimitCreature();
    while (theStep < frames && theStep < 5) {
        gpCombatManager->m_computeExtent = 1;
        glTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + 30;
        for (curSide = 0; curSide < COMBAT_SIDE_COUNT; curSide++) {
            for (slot = 0; slot < gpCombatManager->m_numArmies[curSide]; slot++) {
                if (gpCombatManager->m_armies[curSide][slot].m_powFrames >= theStep) {
                    gpCombatManager->m_armies[curSide][slot].m_animationSequence =
                        ARMY_ANIMATION_EFFECT;
                    if (!gpCombatManager->m_limitCreatureCount[curSide][slot])
                        gpCombatManager->m_limitCreatureCount[curSide][slot]++;
                } else if (gpCombatManager->m_armies[curSide][slot].m_animationSequence
                           != ARMY_ANIMATION_ATTACK) {
                    if (!gpCombatManager->m_limitCreatureCount[curSide][slot]
                        && gpCombatManager->m_armies[curSide][slot].m_animationSequence)
                        gpCombatManager->m_limitCreatureCount[curSide][slot]++;
                    gpCombatManager->m_armies[curSide][slot].m_animationSequence =
                        ARMY_ANIMATION_STAND;
                }
            }
        }
        gSpellEffectFrame = theStep;
        gpCombatManager->DrawFrame(1);
        theStep++;
    }
    while (theStep < frames && theStep < 10) {
        gpCombatManager->m_computeExtent = 1;
        glTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + 30;
        for (curSide = 0; curSide < COMBAT_SIDE_COUNT; curSide++) {
            for (slot = 0; slot < gpCombatManager->m_numArmies[curSide]; slot++) {
                if (gpCombatManager->m_armies[curSide][slot].m_powFrames
                    >= ARMY_POW_FRAMES_KILLED) {
                    gpCombatManager->m_armies[curSide][slot].m_animationSequence =
                        ARMY_ANIMATION_EFFECT;
                    gpCombatManager->m_armies[curSide][slot].m_stats.attributes |=
                        MONSTER_FLAGS_DEAD;
                    if (!gpCombatManager->m_limitCreatureCount[curSide][slot])
                        gpCombatManager->m_limitCreatureCount[curSide][slot]++;
                } else if (gpCombatManager->m_armies[curSide][slot].m_animationSequence
                           != ARMY_ANIMATION_ATTACK) {
                    if (!gpCombatManager->m_limitCreatureCount[curSide][slot]
                        && gpCombatManager->m_armies[curSide][slot].m_animationSequence)
                        gpCombatManager->m_limitCreatureCount[curSide][slot]++;
                    gpCombatManager->m_armies[curSide][slot].m_animationSequence =
                        ARMY_ANIMATION_STAND;
                }
            }
        }
        gSpellEffectFrame = theStep;
        gpCombatManager->DrawFrame(1);
        theStep++;
    }
    while (++theStep < 10)
        DelayMilli(15);
    for (curSide = 0; curSide < COMBAT_SIDE_COUNT; curSide++) {
        for (slot = 0; slot < gpCombatManager->m_numArmies[curSide]; slot++) {
            curArmyPtr = &gpCombatManager->m_armies[curSide][slot];
            if ((curArmyPtr->m_stats.attributes & MONSTER_FLAGS_DEAD)
                && curArmyPtr->m_powFrames != ARMY_POW_NONE) {
                nextCellHex = curArmyPtr->m_hex;
                if (ValidHex(nextCellHex))
                    gpCombatManager->m_hexCells[nextCellHex].m_occupantSide = COMBAT_SIDE_NONE;
                if (curArmyPtr->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    nextCellHex = curArmyPtr->m_hex + (curArmyPtr->m_facing ? -1 : 1);
                    gpCombatManager->m_hexCells[nextCellHex].m_occupantSide = COMBAT_SIDE_NONE;
                }
            } else if (curArmyPtr->m_animationSequence != ARMY_ANIMATION_ATTACK) {
                curArmyPtr->m_animationSequence = ARMY_ANIMATION_STAND;
            }
            curArmyPtr->m_powFrames = ARMY_POW_NONE;
            gpCombatManager->UpdateGrid(curArmyPtr->m_hex, curArmyPtr->m_stats.attributes);
        }
    }
    gpCombatManager->DrawFrame(1);
    for (curSide = 0; curSide < COMBAT_SIDE_COUNT; curSide++)
        for (slot = 0; slot < gpCombatManager->m_numArmies[curSide]; slot++)
            WaitSample(gpCombatManager->m_armies[curSide][slot].m_samples[ARMY_SAMPLE_WINCE]);
}

VA(0x00417d6f, 0x25)
u32 army::Strength(void) {
    return gMonsterDatabase[m_creatureType].fightValue * m_quantity;
}

// Plays a combat effect animation over this stack.
VA(0x00417d94, 0x11b)
void army::SpellEffect(i16 effect, i32 frameDelay) {
    i16 curFrame;
    i16 effectFileIdIndex;
    i16 frameCount;

    m_effectAnimation = effect;
    effectFileIdIndex = MAKEFILEID(gCombatFxNames[effect]);
    if (gCurLoadedSpellFileId != effectFileIdIndex) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(effectFileIdIndex);
        gCurLoadedSpellFileId = effectFileIdIndex;
    }
    m_animationSequence = ARMY_ANIMATION_EFFECT;
    frameCount = 10;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    for (curFrame = 0; curFrame < frameCount; curFrame++) {
        gpCombatManager->m_computeExtent = 1;
        glTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + frameDelay;
        gSpellEffectFrame = curFrame;
        gpCombatManager->DrawFrame(1);
        DelayTil(glTimers + COMBAT_EFFECT_TIMER_SLOT);
    }
    m_effectAnimation = COMBAT_EFFECT_NONE;
}

// Slow (and the other speed spells) restore the base speed and flight;
// effect 9 gave three defense.
VA(0x00417eaf, 0x9a)
void army::CancelSpell(void) {
    switch (m_spellEffect) {
        case SPELL_HASTE:
        case SPELL_SLOW:
        case SPELL_BLIND:
        case SPELL_BLESS:
        case SPELL_CURSE:
            m_damageMode = ARMY_DAMAGE_RANDOM;
            m_stats.speed = m_baseSpeed;
            m_stats.attributes |=
                gMonsterDatabase[m_creatureType].stats.attributes & MONSTER_FLAGS_FLYING;
            break;
        case SPELL_PROTECTION:
            m_stats.defense -= ARMY_PROTECTION_DEFENSE_BONUS;
            break;
    }
    m_spellEffect = SPELL_NONE;
    m_spellEndCondition = ARMY_CANCEL_SPELLS_NONE;
}

// A berserk stack attacks a random neighbour, or flies or steps at random.
VA(0x00417f49, 0x1d6)
void army::GoBerserk(void) {
    i8 isFound;
    i16 tryCountIndex;
    i16 heading;
    i16 attackMask;
    i16 targetHexValue;
    i16 target;

    isFound = 0;
    heading = COMBAT_DIRECTION_NORTHEAST;
    tryCountIndex = 0;
    while (!isFound) {
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
        if (attackMask != COMBAT_ALL_DIRECTIONS_BLOCKED) {
            while (!isFound) {
                heading = Random(COMBAT_DIRECTION_NORTHEAST, COMBAT_DIRECTION_WIDE_EAST);
                if (!(attackMask & (1 << heading))) {
                    giNextAction = ACTION_MOVE;
                    ValidAttack(
                        m_hex,
                        heading,
                        ARMY_ATTACK_TARGET_OCCUPIED,
                        ARMY_HEX_INVALID,
                        &targetHexValue
                    );
                    giNextActionGridIndex = targetHexValue;
                    isFound = 1;
                }
            }
        } else if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
            target = Random(1, 43);
            if (gpCombatManager->m_hexCells[target].m_occupantSide != COMBAT_SIDE_NONE) {
                m_targetSide = gpCombatManager->m_hexCells[target].m_occupantSide;
                m_targetIndex = gpCombatManager->m_hexCells[target].m_occupantIndex;
                if (ValidFlight(target, ARMY_PATH_ANY_TARGET_HEX)) {
                    SET_NEXT_COMBAT_MOVE(target);
                    isFound++;
                }
            } else {
                SET_NEXT_COMBAT_MOVE(target);
            }
        } else {
            heading = Random(COMBAT_DIRECTION_NORTHEAST, COMBAT_DIRECTION_NORTHWEST);
            if (ValidMove(heading)) {
                SET_NEXT_COMBAT_MOVE(m_hex);
                giNextActionGridIndex = GetAdjacentCellIndex(giNextActionGridIndex, heading);
            }
            isFound++;
        }
        tryCountIndex++;
    }
}

// Attacks the stack on the hex (flying, shooting or picking the adjacent
// direction) or moves there; a second argument forbids attacking.
VA(0x0041811f, 0x34a)
void army::MoveAttack(i32 hex, i32 moveOnly) {
    hexcell* cellItem;
    i32 baseHexVal;
    i16 meleeMask;
    i16 atkMaskNum;
    i32 adjHex;
    i32 dirNo;

    gpCombatManager->m_limitCreature = 0;
    CLEAR_ARMY_TARGET(this);
    if (!ValidHex(hex))
        return;
    if (gpCombatManager->m_hexCells[hex].m_occupantSide != COMBAT_SIDE_NONE
        && (gpCombatManager->m_hexCells[hex].m_occupantSide != gpCombatManager->m_currentSide
            || gpCombatManager->m_hexCells[hex].m_occupantIndex
                   != gpCombatManager->m_currentArmyIndex)) {
        if (moveOnly)
            return;
        m_targetSide = gpCombatManager->m_hexCells[hex].m_occupantSide;
        m_targetIndex = gpCombatManager->m_hexCells[hex].m_occupantIndex;
        m_moveTargetHex = hex;
        meleeMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ASSIGNED, ARMY_HEX_INVALID);
        if ((m_stats.attributes & MONSTER_FLAGS_FLYING)
            && meleeMask == COMBAT_ALL_DIRECTIONS_BLOCKED && m_hex != m_moveTargetHex
            && !ValidFlight(m_moveTargetHex, ARMY_PATH_ANY_TARGET_HEX))
            return;
        if (m_spellEffect == SPELL_BERZERKER)
            atkMaskNum = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
        else
            atkMaskNum = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
        if (atkMaskNum == COMBAT_ALL_DIRECTIONS_BLOCKED && m_stats.shots > 0) {
            SpecialAttack();
        } else if (meleeMask == COMBAT_ALL_DIRECTIONS_BLOCKED) {
            AttackTo();
        } else {
            for (dirNo = 0; dirNo < COMBAT_DIRECTION_COUNT; dirNo++) {
                if (dirNo < COMBAT_DIRECTION_ADJACENT_COUNT
                    || (m_stats.attributes & MONSTER_FLAGS_WIDE)) {
                    baseHexVal = m_hex;
                    if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && m_facing == ARMY_FACING_RIGHT
                        && dirNo >= COMBAT_DIRECTION_EASTERN_FIRST
                        && dirNo <= COMBAT_DIRECTION_EASTERN_LAST)
                        baseHexVal++;
                    if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && m_facing == ARMY_FACING_LEFT
                        && dirNo >= COMBAT_DIRECTION_WESTERN_FIRST
                        && dirNo <= COMBAT_DIRECTION_WESTERN_LAST)
                        baseHexVal--;
                    if (dirNo >= COMBAT_DIRECTION_WIDE_FIRST)
                        baseHexVal += m_facing ? -1 : 1;
                    adjHex = GetAdjacentCellIndex(baseHexVal, dirNo);
                    if (ValidHex(adjHex)) {
                        cellItem = &gpCombatManager->m_hexCells[adjHex];
                        if (HEX_HAS_OCCUPANT(*cellItem, m_targetSide, m_targetIndex))
                            m_attackDirection = dirNo;
                    }
                }
            }
            DoAttack(0);
        }
    } else if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
        m_moveTargetHex = hex;
        if (!ValidFlight(m_moveTargetHex, ARMY_PATH_ANY_TARGET_HEX))
            return;
        FlyTo(m_moveTargetHex);
    } else {
        WalkTo(hex);
    }
    gpCombatManager->m_limitCreature = 1;
}

// DamageEnemy sets this byte when the genie halves its target stack.
DATA(0x004a67d4)
i8 gGenieHalf;
