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
static char gTargetName[ARMY_TARGET_NAME_SIZE];

VA(0x00413330, 0xbc)
army::army(void) {
    H1_ENUM_LOCAL(ArmySampleType, i32) i;

    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 0;
    m_attackIcon = NULL;
    m_walkIcon = NULL;
    m_standIcon = NULL;
    m_hex = 0;
    for (i = ARMY_SAMPLE_MOVE; i < ARMY_SAMPLE_COUNT; i++)
        m_samples[i] = NULL;
    m_effectAnimation = COMBAT_EFFECT_NONE;
    m_drawShadow = true;
    gCurLoadedSpellIcon = NULL;
    gCurLoadedSpellFileId = 0;
    gSpellEffectFrame = 0;
    CLEAR_ARMY_TARGET(this);
    m_attackDirection = COMBAT_DIRECTION_INVALID;
    m_unused04 = 0;
    m_moveTargetHex = 0;
}

VA(0x004133ec, 0x64)
void army::InitClean(void) {
    H1_ENUM_LOCAL(ArmySampleType, i32) i;

    for (i = ARMY_SAMPLE_MOVE; i < ARMY_SAMPLE_COUNT; i++)
        m_samples[i] = NULL;
    m_effectAnimation = COMBAT_EFFECT_NONE;
    m_drawShadow = true;
    m_attackIcon = NULL;
    m_walkIcon = NULL;
    m_standIcon = NULL;
}

// The commanding hero's attack and defense raise the copied creature stats.
VA(0x00413450, 0x105)
void army::Init(
    H1_ENUM_PARAM(CreatureType, i8) creatureType,
    i16 quantity,
    H1_ENUM_PARAM(CombatSide, i8) side,
    i8 index
) {
    hero* commander;

    InitClean();
    m_creatureType = creatureType;
    memcpy(&m_stats, &gMonsterDatabase[creatureType].stats, sizeof(tag_monsterStats));
    m_unused29 = 6;
    m_spellEffect = SPELL_NONE;
    m_spellEndCondition = ARMY_CANCEL_SPELLS_NONE;
    commander = gCombatManager->m_heroes[side];
    if (commander) {
        m_stats.attack += commander->m_primaryStats[HERO_PRIMARY_ATTACK];
        m_stats.defense += commander->m_primaryStats[HERO_PRIMARY_DEFENSE];
    }
    m_facing = H1_ENUM_DECODE(ArmyFacing, H1_ENUM_ENCODE(CombatSide, side) ^ 1);
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
    H1_ENUM_LOCAL(ArmySampleType, i32) idx;
    char buffer[16];

    if (m_creatureType != CREATURE_SWORDSMAN)
        strcpy(sprite, gArmySpriteNames[m_creatureType]);
    else
        strcpy(sprite, "swrdsman");
    sprintf(gText, "%s.std", sprite);
    gMonoIconSkip = 0;
    m_standIcon = gResourceManager->GetIcon(gText);
    gMonoIconSkip = -1;
    sprintf(gText, "%s.wlk", sprite);
    m_walkIcon = gResourceManager->GetIcon(gText);
    sprintf(gText, "move%02d.82M", H1_ENUM_ENCODE(CreatureType, m_creatureType));
    m_samples[ARMY_SAMPLE_MOVE] = gResourceManager->GetSample(gText);
    sprintf(gText, "atksnd%02d.82M", H1_ENUM_ENCODE(CreatureType, m_creatureType));
    m_samples[ARMY_SAMPLE_ATTACK] = gResourceManager->GetSample(gText);
    sprintf(gText, "wince%02d.82M", H1_ENUM_ENCODE(CreatureType, m_creatureType));
    m_samples[ARMY_SAMPLE_WINCE] = gResourceManager->GetSample(gText);
    if (m_stats.attributes & MONSTER_FLAGS_SHOOTER) {
        sprintf(gText, "%s.atk", sprite);
        m_attackIcon = gResourceManager->GetIcon(gText);
        sprintf(gText, "shoot%02d.82M", H1_ENUM_ENCODE(CreatureType, m_creatureType));
        m_samples[ARMY_SAMPLE_SHOOT] = gResourceManager->GetSample(gText);
    } else {
        m_attackIcon = NULL;
        m_samples[ARMY_SAMPLE_SHOOT] = NULL;
    }
    for (idx = ARMY_SAMPLE_MOVE; idx < ARMY_SAMPLE_COUNT; idx++) {
        if (m_samples[idx]) {
            m_samples[idx]->m_playbackData.repeat = 0;
            m_samples[idx]->m_playbackData.volume = SAMPLE_VOLUME_FULL;
        }
    }
}

VA(0x00413760, 0xd4)
void army::FreeResources(void) {
    H1_ENUM_LOCAL(ArmySampleType, i32) i;

    if (m_standIcon) {
        gResourceManager->Dispose(m_standIcon);
        m_standIcon = NULL;
    }
    if (m_walkIcon) {
        gResourceManager->Dispose(m_walkIcon);
        m_walkIcon = NULL;
    }
    if ((m_stats.attributes & MONSTER_FLAGS_SHOOTER) && m_attackIcon) {
        gResourceManager->Dispose(m_attackIcon);
        m_attackIcon = NULL;
    }
    for (i = ARMY_SAMPLE_MOVE; i < ARMY_SAMPLE_COUNT; i++) {
        if (m_samples[i]) {
            gResourceManager->Dispose(m_samples[i]);
            m_samples[i] = NULL;
        }
    }
}

// m_animationSequence selects the stand, walk, attack or spell-effect pose and
// m_animationFrame its frame; m_drawShadow adds the shadow frames.
VA(0x00413834, 0x7a5)
void army::DrawToBuffer(i16 x, i16 y) {
    i16 effectPosX;
    H1_ENUM_LOCAL(IconDrawOffsetMode, i8) offsetMode;
    b8 outlined;
    i16 spellOutlineColor;
    char countText[ARMY_QUANTITY_TEXT_SIZE];
    i16 spellMarkerX;
    i16 countX;

    offsetMode = ICON_DRAW_OFFSET_FULL;
    outlined = false;
    if ((m_animationFrame == ARMY_EDGE_CLIP_FRAME || m_animationFrame >= ARMY_EDGE_CLIP_LATER_FRAME)
        && ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_hex % COMBAT_GRID_COLUMNS <= 2 || m_hex % COMBAT_GRID_COLUMNS >= 6)
            || !(m_stats.attributes & MONSTER_FLAGS_WIDE)
                   && (m_hex % COMBAT_GRID_COLUMNS <= 1 || m_hex % COMBAT_GRID_COLUMNS >= 7)))
        gIconClipOn = true;
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
                        m_standIcon->DimToBuffer(
                            x,
                            y,
                            m_animationFrame + 8,
                            ARMY_FACING_ORIENTATION(m_facing),
                            offsetMode
                        );
                    break;
                case CREATURE_CYCLOPS:
                case CREATURE_PHOENIX:
                case CREATURE_DRAGON:
                    if (m_drawShadow)
                        m_standIcon->DimToBuffer(
                            x,
                            y,
                            m_animationFrame + 15,
                            ARMY_FACING_ORIENTATION(m_facing),
                            offsetMode
                        );
                    break;
                default:
                    if (m_drawShadow)
                        m_standIcon->DimToBuffer(
                            x,
                            y,
                            m_animationFrame + 9,
                            ARMY_FACING_ORIENTATION(m_facing),
                            offsetMode
                        );
                    break;
            }
            if (m_animationFrame > 4 && m_creatureType != CREATURE_HYDRA)
                m_standIcon->DrawToBuffer(x, y, 5, ARMY_FACING_ORIENTATION(m_facing), offsetMode);
            m_standIcon->DrawToBuffer(
                x,
                y,
                m_animationFrame,
                ARMY_FACING_ORIENTATION(m_facing),
                offsetMode
            );
            if (m_hex == gCombatManager->m_limitCreatureHex
                && gCombatManager->m_limitCreature == true) {
                m_standIcon->FillToBuffer(
                    x,
                    y,
                    0,
                    ARMY_LIMIT_OUTLINE_COLOR,
                    ARMY_FACING_ORIENTATION(m_facing),
                    offsetMode
                );
                outlined = true;
            }
            if (m_spellEffect != SPELL_NONE) {
                switch (m_spellEffect) {
                    case SPELL_HASTE:
                    case SPELL_BLESS:
                    case SPELL_PROTECTION:
                    case SPELL_ANTI_MAGIC:
                        spellOutlineColor = ARMY_GOOD_SPELL_OUTLINE_COLOR;
                        break;
                    default:
                        spellOutlineColor = ARMY_BAD_SPELL_OUTLINE_COLOR;
                        break;
                }
                if (!outlined && m_animationFrame == 1)
                    m_standIcon->FillToBuffer(
                        x,
                        y,
                        0,
                        spellOutlineColor,
                        ARMY_FACING_ORIENTATION(m_facing),
                        offsetMode
                    );
                spellMarkerX = x;
                if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    if (m_facing == ARMY_FACING_RIGHT)
                        spellMarkerX += 75;
                    else
                        spellMarkerX -= 95;
                } else if (m_facing == ARMY_FACING_LEFT) {
                    spellMarkerX -= 39;
                }
                gCombatManager->m_combatIcons[COMBAT_ICON_SPELLS]->DrawToBuffer(
                    spellMarkerX,
                    y - 40,
                    H1_ENUM_ENCODE(SpellType, m_spellEffect),
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (m_animationFrame == 1 && gCombatManager->m_showArmyQuantities) {
                if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    if (m_facing == ARMY_FACING_RIGHT)
                        countX = x + 75;
                    else
                        countX = x - 95;
                } else {
                    if (m_facing == ARMY_FACING_RIGHT)
                        countX = x + 8;
                    else
                        countX = x - 39;
                }
                gCombatManager->m_combatIcons[COMBAT_ICON_TEXTBAR]
                    ->DrawToBuffer(countX, y - 11, 5, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
                sprintf(countText, "%d", m_quantity);
                gCombatManager->m_smallFont
                    ->DrawBoundedString(countText, countX, y - 12, 20, 12, 1, FONT_ALIGN_CENTER);
            }
            break;
        case ARMY_ANIMATION_WALK:
            if (m_drawShadow)
                m_walkIcon->DimToBuffer(
                    x,
                    y,
                    m_animationFrame + 6,
                    ARMY_FACING_ORIENTATION(m_facing),
                    offsetMode
                );
            m_walkIcon->DrawToBuffer(
                x,
                y,
                m_animationFrame,
                ARMY_FACING_ORIENTATION(m_facing),
                offsetMode
            );
            break;
        case ARMY_ANIMATION_ATTACK:
            if (m_animationFrame < 5) {
                if (m_drawShadow)
                    m_attackIcon->DimToBuffer(
                        x,
                        y,
                        m_animationFrame + 9,
                        ARMY_FACING_ORIENTATION(m_facing),
                        offsetMode
                    );
                m_attackIcon->DrawToBuffer(x, y, 0, ARMY_FACING_ORIENTATION(m_facing), offsetMode);
            }
            m_attackIcon->DrawToBuffer(
                x,
                y,
                m_animationFrame,
                ARMY_FACING_ORIENTATION(m_facing),
                offsetMode
            );
            break;
        case ARMY_ANIMATION_EFFECT:
            if (!(m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                switch (m_creatureType) {
                    case CREATURE_HYDRA:
                        if (m_drawShadow)
                            m_standIcon->DimToBuffer(
                                x,
                                y,
                                m_animationFrame + 8,
                                ARMY_FACING_ORIENTATION(m_facing),
                                offsetMode
                            );
                        break;
                    case CREATURE_CYCLOPS:
                    case CREATURE_PHOENIX:
                    case CREATURE_DRAGON:
                        if (m_drawShadow)
                            m_standIcon->DimToBuffer(
                                x,
                                y,
                                m_animationFrame + 15,
                                ARMY_FACING_ORIENTATION(m_facing),
                                offsetMode
                            );
                        break;
                    default:
                        if (m_drawShadow)
                            m_standIcon->DimToBuffer(
                                x,
                                y,
                                m_animationFrame + 9,
                                ARMY_FACING_ORIENTATION(m_facing),
                                offsetMode
                            );
                        break;
                }
                m_standIcon->DrawToBuffer(
                    x,
                    y,
                    m_animationFrame,
                    ARMY_FACING_ORIENTATION(m_facing),
                    offsetMode
                );
            }
            effectPosX = x;
            if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (m_facing == ARMY_FACING_RIGHT) {
                    effectPosX += 39;
                    x += 75;
                } else {
                    effectPosX -= 39;
                    x -= 95;
                }
            } else if (m_facing == ARMY_FACING_LEFT) {
                x -= 39;
            }
            if (m_spellEffect != SPELL_NONE)
                gCombatManager->m_combatIcons[COMBAT_ICON_SPELLS]->DrawToBuffer(
                    x,
                    y - 40,
                    H1_ENUM_ENCODE(SpellType, m_spellEffect),
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            gCurLoadedSpellIcon->DrawToBuffer(
                effectPosX,
                y,
                gSpellEffectFrame,
                ARMY_FACING_ORIENTATION(m_facing),
                offsetMode
            );
            break;
    }
    gIconClipOn = false;
}

VA(0x00413fd9, 0x55)
void army::Stand(b8 redraw) {
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    m_walkYStep = 0;
    gCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    if (redraw)
        gCombatManager->DrawFrame(true);
}

VA(0x0041402e, 0x56)
void army::Wince(void) {
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 2;
    m_walkYStep = 0;
    gCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    gCombatManager->SetDrawRightToLeft(
        m_facing != ARMY_FACING_RIGHT ? static_cast<i8>(1) : static_cast<i8>(0)
    );
}

// One hex of walking: six frames redrawn inside the union of the old and
// new extents; a stack turned away from the step moves before animating.
#define occupantCell tempCell   // frame-slot spelling
#define unusedDistance moveDist // frame-slot spelling
VA(0x00414084, 0x7d0)
void army::Walk(H1_ENUM_PARAM(CombatHexDirection, i16) direction, b8 standAfter, b8 continued) {
    i32 boundMaxX;
    i32 boundBottom;
    i16 unusedDistance;
    i16 startFrame;
    i16 stepCount;
    i16 backwards;
    i16 i;
    i16 originalHex;
    i16 drawRightToLeft;
    i32 boundMinX;
    i32 destRearIndex;
    i16 newHex;
    i32 boundTop;
    i32 backHexIndex;

    if (!continued) {
        gMinExtentX = gMinExtentY = COMBAT_EXTENT_MIN_START;
        gMaxExtentX = gMaxExtentY = 0;
        gComputeExtent = 1;
        gSaveBiggestExtent = true;
        DrawToBuffer(gCombatManager->m_hexCells[m_hex].m_x, gCombatManager->m_hexCells[m_hex].m_y);
        gSaveBiggestExtent = false;
        gComputeExtent = 0;
    }
    if (gMinExtentX < 0)
        gMinExtentX = 0;
    if (gMinExtentY < 0)
        gMinExtentY = 0;
    if (gMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
        gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
    if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
        gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
    boundMinX = gMinExtentX - 5;
    boundTop = gMinExtentY - 5;
    boundMaxX = gMaxExtentX + 5;
    boundBottom = gMaxExtentY + 5;
    unusedDistance = 16;
    backwards = 0;
    m_walkYStep = 0;
    if (direction < COMBAT_DIRECTION_WESTERN_FIRST) {
        if (m_facing == ARMY_FACING_RIGHT) {
            startFrame = 0;
            stepCount = 1;
        } else {
            startFrame = 5;
            stepCount = -1;
            backwards = 1;
        }
    } else if (m_facing == ARMY_FACING_LEFT) {
        startFrame = 0;
        stepCount = 1;
    } else {
        startFrame = 5;
        stepCount = -1;
        backwards = 1;
    }
    if (direction == COMBAT_DIRECTION_NORTHWEST || direction == COMBAT_DIRECTION_NORTHEAST)
        m_walkYStep = -16;
    if (direction == COMBAT_DIRECTION_SOUTHWEST || direction == COMBAT_DIRECTION_SOUTHEAST)
        m_walkYStep = 16;
    originalHex = m_hex;
    hexcell occupantCell;
    hexcell rearTemp;
    if (backwards) {
        newHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(newHex)) {
            occupantCell.TakeOccupant(&gCombatManager->m_hexCells[m_hex]);
            if (m_stats.attributes & MONSTER_FLAGS_WIDE)
                rearTemp.TakeOccupant(
                    &gCombatManager->m_hexCells[m_hex + ARMY_WIDE_HEX_STEP(m_facing)]
                );
            gCombatManager->m_hexCells[newHex].TakeOccupant(&occupantCell);
            destRearIndex = newHex + ARMY_WIDE_HEX_STEP(m_facing);
            if (ValidHex(destRearIndex) && (m_stats.attributes & MONSTER_FLAGS_WIDE))
                gCombatManager->m_hexCells[destRearIndex].TakeOccupant(&rearTemp);
            m_hex = newHex;
            if (m_walkYStep)
                m_walkYStep = -m_walkYStep;
        }
    } else {
        drawRightToLeft = 0;
        if (m_facing == ARMY_FACING_RIGHT && direction == COMBAT_DIRECTION_SOUTHEAST)
            drawRightToLeft = 1;
        else if (m_facing == ARMY_FACING_LEFT && direction == COMBAT_DIRECTION_NORTHWEST)
            drawRightToLeft = 1;
        gCombatManager->SetDrawRightToLeft(drawRightToLeft);
    }
    m_animationSequence = ARMY_ANIMATION_WALK;
    m_animationFrame = startFrame;
    PlaySample(m_samples[ARMY_SAMPLE_MOVE]);
    if (!continued) {
        if (ValidHex(m_hex))
            gCombatManager->m_hexCells[m_hex].m_occupantSide = COMBAT_SIDE_NONE;
        gCombatManager->DrawFrame(false);
        if (ValidHex(m_hex))
            gCombatManager->m_hexCells[m_hex].m_occupantSide = gCombatManager->m_currentSide;
        gWindowManager->m_screen->CopyTo(
            gCombatManager->m_backgroundBuffer,
            0,
            0,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            COMBAT_VIEW_HEIGHT
        );
        gCombatManager->m_backgroundDrawn = false;
    }
    for (i = 0; i < 6; i++) {
        if (continued || i) {
            gCombatManager->m_backgroundBuffer->CopyTo(
                gWindowManager->m_screen,
                gMinExtentX,
                gMinExtentY,
                gMinExtentX,
                gMinExtentY,
                gMaxExtentX - gMinExtentX + 1,
                gMaxExtentY - gMinExtentY + 1
            );
            boundMinX = gMinExtentX;
            boundTop = gMinExtentY;
            boundMaxX = gMaxExtentX;
            boundBottom = gMaxExtentY;
        }
        gMinExtentX = gMinExtentY = COMBAT_EXTENT_MIN_START;
        gMaxExtentX = gMaxExtentY = 0;
        gComputeExtent = 1;
        gSaveBiggestExtent = true;
        DrawToBuffer(gCombatManager->m_hexCells[m_hex].m_x, gCombatManager->m_hexCells[m_hex].m_y);
        gComputeExtent = 0;
        gSaveBiggestExtent = false;
        if (gMinExtentX < 0)
            gMinExtentX = 0;
        if (gMinExtentY < 0)
            gMinExtentY = 0;
        if (gMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
            gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
        if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        gCurrArmyDrawn = false;
        gComputeExtent = 1;
        gLimitToExtent = 1;
        m_drawShadow = false;
        gCombatManager->DrawFrame(false);
        m_drawShadow = true;
        gLimitToExtent = 0;
        gComputeExtent = 0;
        gCurrArmyDrawn = true;
        if (gMinExtentX < boundMinX)
            boundMinX = gMinExtentX;
        if (gMinExtentY < boundTop)
            boundTop = gMinExtentY;
        if (gMaxExtentX > boundMaxX)
            boundMaxX = gMaxExtentX;
        if (gMaxExtentY > boundBottom)
            boundBottom = gMaxExtentY;
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        UPDATE_INCLUSIVE_REGION(boundMinX, boundTop, boundMaxX, boundBottom);
        m_animationFrame += stepCount;
    }
    if (!backwards) {
        newHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(newHex)) {
            occupantCell.TakeOccupant(&gCombatManager->m_hexCells[m_hex]);
            backHexIndex = m_hex + ARMY_WIDE_HEX_STEP(m_facing);
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && ValidHex(backHexIndex))
                rearTemp.TakeOccupant(&gCombatManager->m_hexCells[backHexIndex]);
            gCombatManager->m_hexCells[newHex].TakeOccupant(&occupantCell);
            backHexIndex = newHex + ARMY_WIDE_HEX_STEP(m_facing);
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && ValidHex(backHexIndex))
                gCombatManager->m_hexCells[backHexIndex].TakeOccupant(&rearTemp);
            m_hex = newHex;
        }
    }
    if (standAfter == true)
        Stand(true);
}
#undef occupantCell
#undef unusedDistance

// A ranged attack: turn toward the target, animate the missile hex by hex
// over a saved screen patch, apply wall and luck modifiers, report the
// damage; creature 14 shoots twice.
#define launchY y1               // frame-slot spelling
#define landPosY y2              // frame-slot spelling
#define archerColumn srcCol      // frame-slot spelling
#define defenderRow aimRow       // frame-slot spelling
#define defenderColumn targetCol // frame-slot spelling
#define wallRow hitRow           // frame-slot spelling
VA(0x00414854, 0xc74)
void army::SpecialAttack(void) {
    DATA(0x004a67d8)
    static b32 gSecondShot = false;
    i8 missileFrame;
    i32 oldTipX;
    i8 originalColumn;
    H1_ENUM_LOCAL(IconDrawOrientation, i8) drawFlipped;
    i32 oldTipY;
    i8 targetColumn;
    i8 originalRow;
    i32 gainX;
    i32 inFlightX;
    i32 gainY;
    i32 inFlightY;
    i32 i;
    i32 landX;
    army* enemyStack;
    i8 enemyRow;
    i8 slopeDirection;
    i32 adjustX;
    i32 clipTop;
    i32 clipLeft;
    i32 k;
    bitmap* backing;
    i32 aimColumn;
    H1_ENUM_LOCAL(ArmyFacing, i32) wasFacing;
    i32 landY;
    i32 fullXLen;
    i32 flightSteps;
    i32 startX;
    i32 launchX;
    i32 launchY;
    i32 maxX;
    i32 adjustY;
    i32 killed;
    i32 fullYLen;
    i32 liftOffsets[5];
    i32 startY;
    i32 landPosX;
    i32 landPosY;
    i32 maxY;
    b8 wallPenalty;
    i32 damageDone;

    wasFacing = m_facing;
    m_walkYStep = 0;
    if (m_targetSide < COMBAT_SIDE_FIRST || m_targetIndex < 0)
        return;
    enemyStack = &gCombatManager->m_armies[m_targetSide][m_targetIndex];
    targetColumn = enemyStack->m_hex % COMBAT_GRID_COLUMNS;
    enemyRow = enemyStack->m_hex / COMBAT_GRID_COLUMNS;
    originalColumn = m_hex % COMBAT_GRID_COLUMNS;
    originalRow = m_hex / COMBAT_GRID_COLUMNS;
    wasFacing = m_facing;
    if (targetColumn > originalColumn || !(originalRow & 1) && targetColumn == originalColumn)
        m_facing = ARMY_FACING_RIGHT;
    else
        m_facing = ARMY_FACING_LEFT;
    gCombatManager->SetDrawRightToLeft(
        m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(1) : static_cast<i8>(0)
    );
    CheckLuck();
    m_animationSequence = ARMY_ANIMATION_ATTACK;
    PlaySample(m_samples[ARMY_SAMPLE_SHOOT]);
    for (i = 0; i < 4; i++) {
        m_animationFrame = i + 1;
        gCombatManager->UpdateGrid(m_hex, m_stats.attributes);
        gCombatManager->DrawFrame(true);
    }
    aimColumn = targetColumn;
    if (enemyStack->m_stats.attributes & MONSTER_FLAGS_WIDE) {
        aimColumn += enemyStack->m_facing == ARMY_FACING_LEFT ? -1 : 1;
    }
    fullXLen = aimColumn - originalColumn;
    drawFlipped = ICON_DRAW_NORMAL;
    if (fullXLen < 0) {
        drawFlipped = ICON_DRAW_FLIPPED;
        fullXLen = -fullXLen;
    }
    fullYLen = enemyRow - originalRow;
    if (fullYLen < 0)
        fullYLen = -fullYLen;
    flightSteps = __max(fullXLen, fullYLen);
    missileFrame = 7;
    slopeDirection = 0;
    if (originalRow > enemyRow)
        slopeDirection = -1;
    else if (originalRow < enemyRow)
        slopeDirection = 1;
    if (fullYLen > 1)
        missileFrame += slopeDirection;
    if (fullXLen <= 3) {
        if (fullYLen > 2)
            missileFrame += slopeDirection;
        if (fullYLen == 1)
            missileFrame += slopeDirection;
    }
    liftOffsets[0] = -20;
    liftOffsets[1] = -15;
    liftOffsets[2] = 0;
    liftOffsets[3] = 15;
    liftOffsets[4] = 20;
    startX = gCombatManager->m_hexCells[m_hex].m_x + (m_facing == ARMY_FACING_RIGHT ? 80 : -80);
    startY = gCombatManager->m_hexCells[m_hex].m_y - 90 + liftOffsets[missileFrame - 5];
    landX = gCombatManager->m_hexCells[enemyRow * COMBAT_GRID_COLUMNS + aimColumn].m_x;
    landY = gCombatManager->m_hexCells[enemyRow * COMBAT_GRID_COLUMNS].m_y - 90;
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
    launchY = startY + gainY;
    landPosY = landY - flightSteps * 2 * gainY;
    adjustY = (launchY + landPosY) / 2 - launchY;
    inFlightX = startX + adjustX;
    inFlightY = startY + adjustY;
    maxX = 0;
    clipLeft = LOGICAL_SCREEN_WIDTH - 1;
    maxY = 0;
    clipTop = LOGICAL_SCREEN_HEIGHT - 1;
    backing = new bitmap(BITMAP_TYPE_MEMORY, ARMY_MISSILE_PATCH_WIDTH, ARMY_MISSILE_PATCH_HEIGHT);
    backing->GrabBitmap(
        gWindowManager->m_screen,
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
            gWindowManager->m_screen,
            inFlightX - ARMY_MISSILE_HALF_WIDTH,
            inFlightY - ARMY_MISSILE_HALF_HEIGHT
        );
        m_attackIcon
            ->DrawToBuffer(inFlightX, inFlightY, missileFrame, drawFlipped, ICON_DRAW_OFFSET_FULL);
        if (inFlightX - ARMY_MISSILE_HALF_WIDTH < clipLeft)
            clipLeft = inFlightX - ARMY_MISSILE_HALF_WIDTH;
        if (inFlightX + ARMY_MISSILE_HALF_WIDTH > maxX)
            maxX = inFlightX + ARMY_MISSILE_HALF_WIDTH;
        if (inFlightY - ARMY_MISSILE_HALF_HEIGHT < clipTop)
            clipTop = inFlightY - ARMY_MISSILE_HALF_HEIGHT;
        if (inFlightY + ARMY_MISSILE_HALF_HEIGHT > maxY)
            maxY = inFlightY + ARMY_MISSILE_HALF_HEIGHT;
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        UPDATE_INCLUSIVE_REGION(clipLeft, clipTop, maxX, maxY);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 15;
        oldTipX = inFlightX;
        oldTipY = inFlightY;
        inFlightX += gainX;
        inFlightY += gainY;
    }
    backing->DrawToBuffer(oldTipX - ARMY_MISSILE_HALF_WIDTH, oldTipY - ARMY_MISSILE_HALF_HEIGHT);
    gWindowManager->UpdateScreenRegion(
        oldTipX - ARMY_MISSILE_HALF_WIDTH,
        oldTipY - ARMY_MISSILE_HALF_HEIGHT,
        ARMY_MISSILE_PATCH_WIDTH,
        ARMY_MISSILE_PATCH_HEIGHT
    );
    delete backing;
    m_stats.shots--;
    wallPenalty = false;
    if (gCombatManager->m_castleSide[COMBAT_DEFENDER_SIDE]
        && m_hex % COMBAT_GRID_COLUMNS <= COMBAT_CASTLE_WALL_COLUMN - 1
        && enemyStack->m_hex % COMBAT_GRID_COLUMNS >= COMBAT_CASTLE_WALL_COLUMN + 1) {
        i32 wallDistance;
        i32 defenderRow;
        i32 defenderColumn;
        i32 archerColumn;
        i32 unusedHex;
        i32 wallRow;
        i32 archerRow;
        i32 pastWall;

        archerColumn = m_hex % COMBAT_GRID_COLUMNS;
        archerRow = m_hex / COMBAT_GRID_COLUMNS;
        pastWall = archerColumn - COMBAT_CASTLE_WALL_COLUMN;
        defenderColumn = enemyStack->m_hex % COMBAT_GRID_COLUMNS;
        defenderRow = enemyStack->m_hex / COMBAT_GRID_COLUMNS;
        wallDistance = COMBAT_CASTLE_WALL_COLUMN - archerColumn;
        wallRow = defenderRow;
        if (abs(defenderRow - archerRow) >= 2)
            wallRow -= (defenderRow - archerRow) / 2;
        if (abs(defenderRow - archerRow) % 2 == 1) {
            if (pastWall < wallDistance
                || pastWall == wallDistance
                       && (archerRow == COMBAT_UPPER_WALL_ROW
                           || archerRow == COMBAT_LOWER_WALL_ROW)) {
                if (archerRow < defenderRow)
                    wallRow--;
                else
                    wallRow++;
            }
        }
        if (wallRow > COMBAT_GRID_LAST_ROW)
            wallRow = COMBAT_GRID_LAST_ROW;
        if (wallRow < 0)
            wallRow = 0;
        wallPenalty =
            gCombatManager->m_hexCells[wallRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN]
                    .m_obstacleIndex
                == COMBAT_WALL_DAMAGED
            || gCombatManager->m_hexCells[wallRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN]
                       .m_obstacleIndex
                   == COMBAT_WALL_INTACT;
    }
    DamageEnemy(
        enemyStack,
        &damageDone,
        &killed,
        true,
        wallPenalty ? ARMY_CASTLE_WALL_DEFENSE_BONUS : 0
    );
    if (killed > 0) {
        strcpy(gTargetName, gArmyNames[enemyStack->m_creatureType]);
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
            killed <= 1 ? gTargetName : gArmyNamesPlural[enemyStack->m_creatureType],
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
    gCombatManager->CombatMessage(gText, true);
    PowEffect(H1_ENUM_DECODE(CombatPowEffect, m_stats.powEffect));
    if (!(enemyStack->m_stats.attributes & MONSTER_FLAGS_DEAD))
        enemyStack->Stand(false);
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK)
        CancelSpell();
    WaitSample(m_samples[ARMY_SAMPLE_SHOOT]);
    m_facing = wasFacing;
    Stand(true);
    if (enemyStack->m_quantity > 0)
        enemyStack->Stand(true);
    if (!gSecondShot && m_creatureType == CREATURE_ELF && enemyStack->m_quantity > 0) {
        gSecondShot = true;
        SpecialAttack();
        gSecondShot = false;
    }
}
#undef launchY
#undef landPosY
#undef archerColumn
#undef defenderRow
#undef defenderColumn
#undef wallRow

// Attacks every enemy next to the stack (the hydra), then turns them back.
VA(0x004154c8, 0x763)
void army::DoHydraAttack(void) {
    i32 killedNow;
    i32 damage;
    H1_ENUM_LOCAL(CombatSide, i16) occupantSideIndex;
    i16 i;
    H1_ENUM_LOCAL(CombatHexDirection, i16) dir;
    i16 armyIndex;
    i16 attackMask;
    army* struckArmy;
    i16 hitHex;
    i32 totalDead;
    i32 totalDamageDone;
    army* currentStack;

    m_walkYStep = 0;
    CheckLuck();
    PlaySample(m_samples[ARMY_SAMPLE_ATTACK]);
    m_animationSequence = ARMY_ANIMATION_STAND;
    gCombatManager->ResetLimitCreature();
    gCombatManager->m_limitCreatureCount[m_side][m_index]++;
    for (i = 0; i < 5; i++) {
        gCombatManager->m_computeExtent = true;
        m_animationFrame = i + 3;
        gCombatManager->DrawFrame(true);
    }
    if (m_spellEffect == SPELL_BERZERKER)
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
    else
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
    gCombatManager->ResetHitByCreature();
    totalDead = 0;
    totalDamageDone = totalDead;
    for (dir = COMBAT_DIRECTION_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (!(attackMask & H1_ENUM_BIT(CombatHexDirection, dir))) {
            hitHex = m_hex;
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_facing == ARMY_FACING_LEFT && dir > COMBAT_DIRECTION_EASTERN_LAST
                    || m_facing == ARMY_FACING_RIGHT
                           && (dir < COMBAT_DIRECTION_WESTERN_FIRST
                               || dir > COMBAT_DIRECTION_WESTERN_LAST))) {
                if (m_facing == ARMY_FACING_LEFT)
                    hitHex = m_hex - 1;
                else
                    hitHex = m_hex + 1;
            }
            hitHex = GetAdjacentCellIndex(hitHex, dir);
            if (ValidHex(hitHex)) {
                occupantSideIndex = gCombatManager->m_hexCells[hitHex].m_occupantSide;
                armyIndex = gCombatManager->m_hexCells[hitHex].m_occupantIndex;
                if (occupantSideIndex >= COMBAT_SIDE_FIRST && armyIndex >= 0) {
                    gCombatManager->m_limitCreatureCount[occupantSideIndex][armyIndex]++;
                    struckArmy = &gCombatManager->m_armies[occupantSideIndex][armyIndex];
                    if (!struckArmy->m_hitByCreature) {
                        struckArmy->m_hitByCreature = true;
                        DamageEnemy(struckArmy, &damage, &killedNow, false, 0);
                        totalDamageDone += damage;
                        totalDead += killedNow;
                    }
                }
            }
        }
    }
    if (totalDead > 0)
        sprintf(
            gText,
            "%s %s %s %d %s.  %d %s %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            totalDamageDone,
            localization::Tr("combat.fragment.damage_points"),
            totalDead,
            totalDead <= 1 ? localization::Tr("combat.fragment.troop")
                           : localization::Tr("combat.fragment.troops"),
            totalDead <= 1 ? localization::Tr("combat.fragment.dies")
                           : localization::Tr("combat.fragment.killed")
        );
    else
        sprintf(
            gText,
            "%s %s %s %d %s.",
            localization::Tr("combat.fragment.attack"),
            gArmyNamesPlural[m_creatureType],
            localization::Tr("combat.fragment.does_damage"),
            totalDamageDone,
            localization::Tr("combat.fragment.damage_points")
        );
    gText[0] = CyrillicToUpper(gText[0]);
    gCombatManager->CombatMessage(gText, true);
    PowEffect(H1_ENUM_DECODE(CombatPowEffect, m_stats.powEffect));
    WaitSample(m_samples[ARMY_SAMPLE_ATTACK]);
    m_animationSequence = ARMY_ANIMATION_STAND;
    gCombatManager->ResetLimitCreature();
    gCombatManager->m_limitCreatureCount[m_side][m_index]++;
    for (i = 4; i >= 0; i--) {
        gCombatManager->m_computeExtent = true;
        m_animationFrame = i + 3;
        gCombatManager->DrawFrame(true);
    }
    if (m_spellEffect == SPELL_BERZERKER)
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
    else
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
    for (dir = COMBAT_DIRECTION_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (!(attackMask & H1_ENUM_BIT(CombatHexDirection, dir))) {
            hitHex = m_hex;
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_facing == ARMY_FACING_LEFT && dir > COMBAT_DIRECTION_EASTERN_LAST
                    || m_facing == ARMY_FACING_RIGHT
                           && (dir < COMBAT_DIRECTION_WESTERN_FIRST
                               || dir > COMBAT_DIRECTION_WESTERN_LAST))) {
                if (m_facing == ARMY_FACING_LEFT)
                    hitHex = m_hex - 1;
                else
                    hitHex = m_hex + 1;
            }
            hitHex = GetAdjacentCellIndex(hitHex, dir);
            if (ValidHex(hitHex)) {
                occupantSideIndex = gCombatManager->m_hexCells[hitHex].m_occupantSide;
                armyIndex = gCombatManager->m_hexCells[hitHex].m_occupantIndex;
                if (occupantSideIndex >= COMBAT_SIDE_FIRST && armyIndex >= 0) {
                    gCombatManager->m_limitCreatureCount[occupantSideIndex][armyIndex]++;
                    struckArmy = &gCombatManager->m_armies[occupantSideIndex][armyIndex];
                    if (!(struckArmy->m_stats.attributes & MONSTER_FLAGS_DEAD))
                        struckArmy->Stand(false);
                }
            }
        }
    }
    m_targetSide = H1_ENUM_DECODE(CombatSide, hitHex = ARMY_HEX_INVALID);
    gCombatManager->m_computeExtent = false;
    for (occupantSideIndex = COMBAT_SIDE_FIRST; occupantSideIndex < COMBAT_SIDE_COUNT;
         occupantSideIndex++) {
        for (armyIndex = 0; armyIndex < gCombatManager->m_numArmies[occupantSideIndex];
             armyIndex++) {
            currentStack = &gCombatManager->m_armies[occupantSideIndex][armyIndex];
            if (!(currentStack->m_stats.attributes & MONSTER_FLAGS_DEAD)
                || currentStack->m_powFrames == ARMY_POW_NONE)
                currentStack->Stand(false);
        }
    }
    gCombatManager->DrawFrame(true);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00415c2b, 0x22)
void army::DirDoAttack(H1_ENUM_PARAM(CombatHexDirection, i16) direction) {
    m_attackDirection = direction;
    DoAttack(false);
}

// A melee strike in m_attackDirection: breath attackers (attribute 8) also
// hit the hex behind, some creatures cast on the target, the target
// retaliates once, and creatures 5 and 8 strike twice.
VA(0x00415c4d, 0x1092)
void army::DoAttack(b32 retaliation) {
    i32 ignored;
    b32 prevExtendDown;
    i16 frameBase;
    army* rearVictim;
    i32 damageDone;
    i16 lastHex;
    H1_ENUM_LOCAL(CombatHexDirection, i32) savedAttackDirection;
    H1_ENUM_LOCAL(ArmyFacing, i16) originalFacing;
    H1_ENUM_LOCAL(CombatHexDirection, i32) initialAttackDirection;
    b32 specialWorked;
    army* struckArmy;
    i32 killed;

    prevExtendDown = false;
    damageDone = 0;
    killed = 0;
    specialWorked = false;
    if (retaliation)
        gCombatManager->m_currentSide = COMBAT_OPPOSING_SIDE(gCombatManager->m_currentSide);
    if (m_creatureType == CREATURE_HYDRA) {
        DoHydraAttack();
        if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK && !retaliation)
            CancelSpell();
        goto secondStrike;
    }
    initialAttackDirection = m_attackDirection;
    originalFacing = m_facing;
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
    gCombatManager->SetDrawRightToLeft(
        m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(1) : static_cast<i8>(0)
    );
    CheckLuck();
    lastHex = m_hex;
    if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
        && (originalFacing == ARMY_FACING_LEFT
                && m_attackDirection >= COMBAT_DIRECTION_WESTERN_FIRST
            || originalFacing == ARMY_FACING_RIGHT
                   && (m_attackDirection <= COMBAT_DIRECTION_EASTERN_LAST
                       || m_attackDirection >= COMBAT_DIRECTION_WIDE_FIRST))) {
        if (originalFacing == ARMY_FACING_LEFT)
            lastHex = m_hex - 1;
        else
            lastHex = m_hex + 1;
    }
    lastHex = GetAdjacentCellIndex(lastHex, m_attackDirection);
    gCombatManager->ResetLimitCreature();
    gCombatManager->m_limitCreatureCount[m_side][m_index]++;
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        i16 behindHex;

        if (ValidHex(lastHex)
            && gCombatManager->m_hexCells[lastHex].m_occupantSide >= COMBAT_SIDE_FIRST
            && gCombatManager->m_hexCells[lastHex].m_occupantIndex >= 0)
            gCombatManager
                ->m_limitCreatureCount[gCombatManager->m_hexCells[lastHex].m_occupantSide]
                                      [gCombatManager->m_hexCells[lastHex].m_occupantIndex]++;
        behindHex = GetAdjacentCellIndex(lastHex, m_attackDirection);
        if (ValidHex(behindHex)
            && gCombatManager->m_hexCells[behindHex].m_occupantSide >= COMBAT_SIDE_FIRST
            && gCombatManager->m_hexCells[behindHex].m_occupantIndex >= 0) {
            gCombatManager
                ->m_limitCreatureCount[gCombatManager->m_hexCells[behindHex].m_occupantSide]
                                      [gCombatManager->m_hexCells[behindHex].m_occupantIndex]++;
            if (m_attackDirection == COMBAT_DIRECTION_SOUTHEAST
                || m_attackDirection == COMBAT_DIRECTION_SOUTHWEST)
                gCombatManager->m_extendLimitDown = true;
        }
    }
    prevExtendDown = gCombatManager->m_extendLimitDown;
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 3;
    gCombatManager->DrawFrame(true);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    PlaySample(m_samples[ARMY_SAMPLE_ATTACK]);
    m_animationFrame = 4;
    gCombatManager->m_computeExtent = true;
    gCombatManager->DrawFrame(true);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    m_animationFrame = frameBase;
    gCombatManager->m_computeExtent = true;
    gCombatManager->DrawFrame(true);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        m_animationFrame = frameBase + 3;
        gCombatManager->m_computeExtent = true;
        gCombatManager->DrawFrame(true);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 105;
    }
    rearVictim = NULL;
    struckArmy = NULL;
    if (ValidHex(lastHex)) {
        i32 targetKilled;
        i16 breathHex;

        if (gCombatManager->m_hexCells[lastHex].m_occupantSide >= COMBAT_SIDE_FIRST
            && gCombatManager->m_hexCells[lastHex].m_occupantIndex >= 0) {
            struckArmy =
                &gCombatManager->m_armies[gCombatManager->m_hexCells[lastHex].m_occupantSide]
                                         [gCombatManager->m_hexCells[lastHex].m_occupantIndex];
            gCombatManager->m_limitCreatureCount[struckArmy->m_side][struckArmy->m_index]++;
            gCombatManager->m_computeExtent = true;
            DamageEnemy(struckArmy, &damageDone, &killed, false, 0);
        }
        targetKilled = killed;
        breathHex = GetAdjacentCellIndex(lastHex, m_attackDirection);
        if ((m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)
            && m_attackDirection < COMBAT_DIRECTION_ADJACENT_COUNT && ValidHex(breathHex)
            && gCombatManager->m_hexCells[breathHex].m_occupantSide >= COMBAT_SIDE_FIRST
            && gCombatManager->m_hexCells[breathHex].m_occupantIndex >= 0
            && gCombatManager->m_hexCells[breathHex].m_occupantIndex
                   != gCombatManager->m_hexCells[lastHex].m_occupantIndex) {
            gCombatManager
                ->m_limitCreatureCount[gCombatManager->m_hexCells[breathHex].m_occupantSide]
                                      [gCombatManager->m_hexCells[breathHex].m_occupantIndex]++;
            lastHex = breathHex;
            if (ValidHex(lastHex)
                && gCombatManager->m_hexCells[lastHex].m_occupantSide != COMBAT_SIDE_NONE
                && gCombatManager->m_hexCells[lastHex].m_occupantIndex != COMBAT_ARMY_INDEX_NONE) {
                m_animationFrame = frameBase + 6;
                gCombatManager->m_computeExtent = true;
                gCombatManager->DrawFrame(true);
                rearVictim =
                    &gCombatManager->m_armies[gCombatManager->m_hexCells[lastHex].m_occupantSide]
                                             [gCombatManager->m_hexCells[lastHex].m_occupantIndex];
                DamageEnemy(rearVictim, &damageDone, &killed, false, 0);
                if (rearVictim->m_quantity > 0)
                    rearVictim->Stand(true);
            }
        }
        killed = targetKilled;
    }
    if (gGenieHalf)
        sprintf(
            gText,
            localization::Tr("combat.genie.half_army.buka"),
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity <= 1 ? localization::Tr("combat.fragment.destroy_singular")
                            : localization::Tr("combat.fragment.destroy_plural")
        );
    else if (killed > 0) {
        strcpy(gTargetName, gArmyNames[struckArmy->m_creatureType]);
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
            killed <= 1 ? gTargetName : gArmyNamesPlural[struckArmy->m_creatureType],
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
    gCombatManager->CombatMessage(gText, true);
    PowEffect(H1_ENUM_DECODE(CombatPowEffect, m_stats.powEffect));
    gCombatManager->m_extendLimitDown = prevExtendDown;
    switch (m_creatureType) {
        case CREATURE_CYCLOPS:
            if (SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_SPECIAL_ROLL_HIT) {
                if (struckArmy && struckArmy->m_spellEffect != SPELL_ANTI_MAGIC
                    && struckArmy->m_creatureType != CREATURE_DRAGON
                    && (struckArmy->m_creatureType != CREATURE_DWARF || SRandom(0, 4) != 1)
                    && !(struckArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                    gCombatManager
                        ->CastSpell(SPELL_PARALYZE, struckArmy->m_hex, true, ARMY_HEX_INVALID);
                    specialWorked = true;
                }
            } else if (SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_SPECIAL_ROLL_HIT && rearVictim
                       && rearVictim->m_spellEffect != SPELL_ANTI_MAGIC
                       && rearVictim->m_creatureType != CREATURE_DRAGON
                       && (rearVictim->m_creatureType != CREATURE_DWARF || SRandom(0, 4) != 1)
                       && !(rearVictim->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                gCombatManager
                    ->CastSpell(SPELL_PARALYZE, rearVictim->m_hex, true, ARMY_HEX_INVALID);
                specialWorked = true;
            }
            break;
        case CREATURE_UNICORN:
            if (SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_SPECIAL_ROLL_HIT && struckArmy
                && struckArmy->m_spellEffect != SPELL_ANTI_MAGIC
                && struckArmy->m_creatureType != CREATURE_DRAGON
                && (struckArmy->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(struckArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                gCombatManager->CastSpell(SPELL_BLIND, struckArmy->m_hex, true, ARMY_HEX_INVALID);
                specialWorked = true;
            }
            break;
        case CREATURE_GHOST:
            gCombatManager->m_ghostKills[gCombatManager->m_hexCells[m_hex].m_occupantSide] = killed;
            break;
        default:
            break;
    }
    gCombatManager->ResetLimitCreature();
    gCombatManager->m_extendLimitDown = prevExtendDown;
    gCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    if (struckArmy) {
        gCombatManager->m_limitCreatureCount[struckArmy->m_side][struckArmy->m_index] = 1;
        if (!(struckArmy->m_stats.attributes & MONSTER_FLAGS_DEAD))
            struckArmy->Stand(false);
    }
    if (rearVictim) {
        gCombatManager->m_limitCreatureCount[rearVictim->m_side][rearVictim->m_index] = 1;
        if (!(rearVictim->m_stats.attributes & MONSTER_FLAGS_DEAD))
            rearVictim->Stand(false);
    }
    WaitSample(m_samples[ARMY_SAMPLE_ATTACK]);
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        if (rearVictim) {
            m_animationFrame = frameBase + 6;
            gCombatManager->m_computeExtent = true;
            gCombatManager->DrawFrame(true);
        }
        m_animationFrame = frameBase + 3;
        gCombatManager->m_computeExtent = true;
        gCombatManager->DrawFrame(true);
        m_animationFrame = frameBase;
        gCombatManager->m_computeExtent = true;
        gCombatManager->DrawFrame(true);
    }
    m_animationFrame = 4;
    gCombatManager->m_computeExtent = true;
    gCombatManager->DrawFrame(true);
    m_animationFrame = 3;
    gCombatManager->m_computeExtent = true;
    gCombatManager->DrawFrame(true);
    gCombatManager->m_computeExtent = true;
    Stand(true);
    m_facing = originalFacing;
    gCombatManager->m_computeExtent = true;
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK && !retaliation)
        CancelSpell();
    gCombatManager->m_computeExtent = true;
    Stand(true);
    if (m_creatureType == CREATURE_GHOST)
        m_quantity +=
            gCombatManager->m_ghostKills[gCombatManager->m_hexCells[m_hex].m_occupantSide];
    if (struckArmy && struckArmy->m_quantity > 0) {
        gCombatManager->m_computeExtent = true;
        struckArmy->Stand(true);
        if (struckArmy->m_spellEffect == SPELL_PARALYZE
            || struckArmy->m_creatureType != CREATURE_GRIFFIN
                   && (struckArmy->m_stats.attributes & MONSTER_FLAGS_RETALIATED)
            || m_creatureType == CREATURE_ROGUE || m_creatureType == CREATURE_SPRITE
            || specialWorked || retaliation) {
            goto secondStrike;
        } else {
            struckArmy->m_attackDirection = OppositeDirection(m_attackDirection);
            if (struckArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                i16 checkHex;

                checkHex = GetAdjacentCellIndex(
                    struckArmy->m_hex,
                    H1_ENUM_ENCODE(ArmyFacing, struckArmy->m_facing)
                        ? H1_ENUM_CAST(CombatHexDirection, i8, COMBAT_DIRECTION_NORTHWEST)
                        : H1_ENUM_CAST(CombatHexDirection, i8, COMBAT_DIRECTION_NORTHEAST)
                );
                if (checkHex == m_hex)
                    struckArmy->m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                checkHex = GetAdjacentCellIndex(
                    struckArmy->m_hex,
                    H1_ENUM_ENCODE(ArmyFacing, struckArmy->m_facing)
                        ? H1_ENUM_CAST(CombatHexDirection, i8, COMBAT_DIRECTION_SOUTHWEST)
                        : H1_ENUM_CAST(CombatHexDirection, i8, COMBAT_DIRECTION_SOUTHEAST)
                );
                if (checkHex == m_hex)
                    struckArmy->m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
            }
            struckArmy->DoAttack(true);
            struckArmy->m_stats.attributes |= MONSTER_FLAGS_RETALIATED;
            if (struckArmy->m_creatureType == CREATURE_GHOST)
                struckArmy->m_quantity +=
                    gCombatManager->m_ghostKills[gCombatManager->m_hexCells[struckArmy->m_hex]
                                                     .m_occupantSide];
        }
    }
secondStrike:
    if ((m_creatureType == CREATURE_WOLF || m_creatureType == CREATURE_PALADIN) && struckArmy
        && struckArmy->m_quantity > 0 && !retaliation && m_spellEffect != SPELL_PARALYZE
        && m_quantity > 0) {
        savedAttackDirection = m_attackDirection;
        m_attackDirection = initialAttackDirection;
        DoAttack(true);
        m_attackDirection = savedAttackDirection;
    }
    m_targetSide = H1_ENUM_DECODE(CombatSide, lastHex = ARMY_HEX_INVALID);
    if (retaliation)
        gCombatManager->m_currentSide = COMBAT_OPPOSING_SIDE(gCombatManager->m_currentSide);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00416cdf, 0x3d)
void army::ResetPath(void) {
    i16 i;

    for (i = 0; i < COMBAT_HEX_COUNT; i++)
        gCombatManager->m_hexCells[i].m_pathFlag = false;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00416d1c, 0x1c)
i16 army::WalkTo(void) {
    return WalkTo(m_moveTargetHex);
}

// Walks the found path one hex at a time, at most the stack's speed.
VA(0x00416d38, 0xe6)
i16 army::WalkTo(i16 destination) {
    i32 stepCount;
    i8 pathIndex;

    m_targetSide = H1_ENUM_DECODE(CombatSide, m_targetIndex = COMBAT_ARMY_INDEX_NONE);
    if (!FindPath(m_hex, destination, m_stats.speed, true, ARMY_PATH_ANY_TARGET_HEX))
        return ARMY_PATH_BLOCKED;
    stepCount = 0;
    for (pathIndex = gSearchArray->m_pathLength - 1; pathIndex >= 0; pathIndex--) {
        Walk(
            H1_ENUM_DECODE(CombatHexDirection, gSearchArray->m_directions[pathIndex]),
            false,
            pathIndex != gSearchArray->m_pathLength - 1
        );
        stepCount++;
        if (stepCount >= m_stats.speed)
            pathIndex = -1;
    }
    if (!H1_ENUM_ENCODE(ArmySpellCancelType, m_spellEndCondition))
        CancelSpell();
    Stand(true);
    return 0;
}

VA(0x00416e1e, 0x1c)
i16 army::AttackTo(void) {
    return AttackTo(m_moveTargetHex);
}

// Flyers jump next to the target; walkers stop short when out of moves.
VA(0x00416e3a, 0x1a2)
i16 army::AttackTo(i16 destination) {
    i32 stepCount;
    i8 pathIndex;

    if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
        if (m_hex != destination)
            FlyTo(destination);
        DoAttack(false);
        return 0;
    }
    if ((m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) && m_moveTargetHex == m_hex) {
        DoAttack(false);
        return 0;
    }
    if (FindPath(m_hex, destination, m_stats.speed, true, ARMY_PATH_ANY_TARGET_HEX)) {
        if (gSearchArray->m_pathLength == 1) {
            m_attackDirection = H1_ENUM_DECODE(CombatHexDirection, gSearchArray->m_directions[0]);
            DoAttack(false);
        } else {
            pathIndex = 0;
            stepCount = 0;
            for (pathIndex = gSearchArray->m_pathLength - 1; pathIndex; pathIndex--) {
                Walk(
                    H1_ENUM_DECODE(CombatHexDirection, gSearchArray->m_directions[pathIndex]),
                    false,
                    pathIndex != gSearchArray->m_pathLength - 1
                );
                stepCount++;
                if (stepCount >= m_stats.speed && pathIndex != 1) {
                    Stand(true);
                    return ARMY_PATH_BLOCKED;
                }
            }
            if (!H1_ENUM_ENCODE(ArmySpellCancelType, m_spellEndCondition))
                CancelSpell();
            m_attackDirection = H1_ENUM_DECODE(CombatHexDirection, gSearchArray->m_directions[0]);
            DoAttack(false);
        }
        return 0;
    }
    return ARMY_PATH_BLOCKED;
}

VA(0x00416fdc, 0x1a0)
void army::CheckLuck(void) {
    i32 luck;

    if (!gCombatManager->m_heroes[m_side])
        return;
    m_luck = ARMY_LUCK_NONE;
    luck = gGame->GetLuck(gCombatManager->m_heroes[m_side], this);
    if (luck > 0 && SRandom(1, 12) <= luck)
        m_luck = ARMY_LUCK_GOOD;
    if (luck < 0 && SRandom(1, 12) < -luck)
        m_luck = ARMY_LUCK_BAD;
    if (H1_ENUM_ENCODE(ArmyLuck, m_luck)) {
        class sample* luckSample;
        if (m_luck < ARMY_LUCK_NONE)
            sprintf(gText, "badluck.82m");
        else
            sprintf(gText, "goodluck.82m");
        luckSample = LoadPlaySample(gText);
        if (m_luck < ARMY_LUCK_NONE) {
            sprintf(
                gText,
                localization::Tr("combat.luck.bad.buka"),
                gArmyNamesPlural[m_creatureType]
            );
            gCombatManager->CombatMessage(gText, true);
            Wince();
            SpellEffect(COMBAT_EFFECT_BAD_LUCK, 180);
        } else {
            sprintf(
                gText,
                localization::Tr("combat.luck.good.buka"),
                gArmyNamesPlural[m_creatureType]
            );
            gCombatManager->CombatMessage(gText, true);
            Stand(true);
            SpellEffect(COMBAT_EFFECT_GOOD_LUCK, 180);
        }
        Stand(true);
        WaitSample(luckSample);
    }
}

#define genieDamage hurt // frame-slot spelling
VA(0x0041717c, 0x215)
void army::DamageEnemy(
    class army* target,
    i32* damageResult,
    i32* killedResult,
    b32 rangedAttack,
    i32 defenseModifier
) {
    i16 defenseExtra;
    i16 battleDiff;
    i32 genieDamage;
    i32 damage;
    float rolledTotal;
    i16 creature;
    i16 attackAdd;

    if (!target)
        return;
    rolledTotal = 0;
    gGenieHalf = false;
    for (creature = 0; creature < m_quantity; creature++) {
        switch (m_damageMode) {
            case ARMY_DAMAGE_MAXIMUM:
                rolledTotal += m_stats.damageMax;
                break;
            case ARMY_DAMAGE_MINIMUM:
                rolledTotal += m_stats.damageMin;
                break;
            default:
                rolledTotal += SRandom(m_stats.damageMin, m_stats.damageMax);
                break;
        }
    }
    attackAdd = 0;
    defenseExtra = 0;
    battleDiff =
        m_stats.attack + attackAdd - (target->m_stats.defense + defenseExtra + defenseModifier);
    if (battleDiff > 20)
        battleDiff = 20;
    if (battleDiff < -20)
        battleDiff = -20;
    rolledTotal *= gBattleStat[battleDiff + 20];
    if (m_luck > ARMY_LUCK_NONE)
        rolledTotal *= 2;
    if (m_luck < ARMY_LUCK_NONE)
        rolledTotal /= 2;
    m_luck = ARMY_LUCK_NONE;
    if ((m_stats.attributes & MONSTER_FLAGS_SHOOTER) && !rangedAttack)
        rolledTotal /= 2;
    if (m_damageMode == ARMY_DAMAGE_HALF)
        rolledTotal /= 2;
    damage = rolledTotal + 0.5;
    if (m_creatureType == CREATURE_GENIE
        && SRandom(1, ARMY_SPECIAL_ROLL_MAX) == ARMY_GENIE_ROLL_HIT) {
        genieDamage = ((target->m_quantity + 1) / 2) * target->m_stats.hitPoints;
        if (genieDamage > damage) {
            gGenieHalf = true;
            damage = genieDamage;
        }
    }
    if (damage > 32000)
        damage = 32000;
    if (damage <= 0)
        damage = 1;
    *damageResult = damage;
    *killedResult = target->Damage(damage);
}
#undef genieDamage

// A stack whose spell (2) breaks on damage loses it.
VA(0x00417391, 0x160)
i32 army::Damage(i32 damage) {
    H1_ENUM_LOCAL(ArmyFacing, i8) oldFacing;
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
    m_facing = H1_ENUM_DECODE(
        ArmyFacing,
        H1_ENUM_ENCODE(
            ArmyFacing,
            gCombatManager
                ->m_armies[gCombatManager->m_currentSide][gCombatManager->m_currentArmyIndex]
                .m_facing
        ) ^ 1
    );
    Wince();
    m_facing = oldFacing;
    gCombatManager->DrawFrame(true);
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
void army::PowEffect(H1_ENUM_PARAM(CombatPowEffect, i8) effect) {
    i16 frames;
    i16 armyIndex;
    i16 armySlot;
    i16 effectFrame;
    i16 longestPow;
    i32 vacatedHex;
    army* currentArmy;
    H1_ENUM_LOCAL(CombatSide, i16) stackSide;

    longestPow = 0;
    for (armySlot = 0; armySlot < gCombatManager->m_numArmies[COMBAT_ATTACKER_SIDE]; armySlot++)
        if (gCombatManager->m_armies[COMBAT_ATTACKER_SIDE][armySlot].m_powFrames > longestPow)
            longestPow = gCombatManager->m_armies[COMBAT_ATTACKER_SIDE][armySlot].m_powFrames;
    for (armySlot = 0; armySlot < gCombatManager->m_numArmies[COMBAT_DEFENDER_SIDE]; armySlot++)
        if (gCombatManager->m_armies[COMBAT_DEFENDER_SIDE][armySlot].m_powFrames > longestPow)
            longestPow = gCombatManager->m_armies[COMBAT_DEFENDER_SIDE][armySlot].m_powFrames;
    if (longestPow >= ARMY_POW_FRAMES_KILLED)
        frames = 10;
    else
        frames = longestPow;
    if (gCurLoadedSpellFileId != H1_ENUM_ENCODE(CombatPowEffect, effect)) {
        gResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon =
            gResourceManager->GetIcon(gPowEffectNames[H1_ENUM_ENCODE(CombatPowEffect, effect)]);
        gCurLoadedSpellFileId = H1_ENUM_ENCODE(CombatPowEffect, effect);
    }
    for (stackSide = COMBAT_SIDE_FIRST; stackSide < COMBAT_SIDE_COUNT; stackSide++)
        for (armyIndex = 0; armyIndex < gCombatManager->m_numArmies[stackSide]; armyIndex++)
            if (gCombatManager->m_armies[stackSide][armyIndex].m_powFrames > 0)
                PlaySample(
                    gCombatManager->m_armies[stackSide][armyIndex].m_samples[ARMY_SAMPLE_WINCE]
                );
    effectFrame = 0;
    gCombatManager->ResetLimitCreature();
    while (effectFrame < frames && effectFrame < 5) {
        gCombatManager->m_computeExtent = true;
        gTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + 30;
        for (stackSide = COMBAT_SIDE_FIRST; stackSide < COMBAT_SIDE_COUNT; stackSide++) {
            for (armyIndex = 0; armyIndex < gCombatManager->m_numArmies[stackSide]; armyIndex++) {
                if (gCombatManager->m_armies[stackSide][armyIndex].m_powFrames >= effectFrame) {
                    gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence =
                        ARMY_ANIMATION_EFFECT;
                    if (!gCombatManager->m_limitCreatureCount[stackSide][armyIndex])
                        gCombatManager->m_limitCreatureCount[stackSide][armyIndex]++;
                } else if (gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence
                           != ARMY_ANIMATION_ATTACK) {
                    if (!gCombatManager->m_limitCreatureCount[stackSide][armyIndex]
                        && H1_ENUM_ENCODE(
                            ArmyAnimationSequence,
                            gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence
                        ))
                        gCombatManager->m_limitCreatureCount[stackSide][armyIndex]++;
                    gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence =
                        ARMY_ANIMATION_STAND;
                }
            }
        }
        gSpellEffectFrame = effectFrame;
        gCombatManager->DrawFrame(true);
        effectFrame++;
    }
    while (effectFrame < frames && effectFrame < 10) {
        gCombatManager->m_computeExtent = true;
        gTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + 30;
        for (stackSide = COMBAT_SIDE_FIRST; stackSide < COMBAT_SIDE_COUNT; stackSide++) {
            for (armyIndex = 0; armyIndex < gCombatManager->m_numArmies[stackSide]; armyIndex++) {
                if (gCombatManager->m_armies[stackSide][armyIndex].m_powFrames
                    >= ARMY_POW_FRAMES_KILLED) {
                    gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence =
                        ARMY_ANIMATION_EFFECT;
                    gCombatManager->m_armies[stackSide][armyIndex].m_stats.attributes |=
                        MONSTER_FLAGS_DEAD;
                    if (!gCombatManager->m_limitCreatureCount[stackSide][armyIndex])
                        gCombatManager->m_limitCreatureCount[stackSide][armyIndex]++;
                } else if (gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence
                           != ARMY_ANIMATION_ATTACK) {
                    if (!gCombatManager->m_limitCreatureCount[stackSide][armyIndex]
                        && H1_ENUM_ENCODE(
                            ArmyAnimationSequence,
                            gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence
                        ))
                        gCombatManager->m_limitCreatureCount[stackSide][armyIndex]++;
                    gCombatManager->m_armies[stackSide][armyIndex].m_animationSequence =
                        ARMY_ANIMATION_STAND;
                }
            }
        }
        gSpellEffectFrame = effectFrame;
        gCombatManager->DrawFrame(true);
        effectFrame++;
    }
    while (++effectFrame < 10)
        DelayMilli(15);
    for (stackSide = COMBAT_SIDE_FIRST; stackSide < COMBAT_SIDE_COUNT; stackSide++) {
        for (armyIndex = 0; armyIndex < gCombatManager->m_numArmies[stackSide]; armyIndex++) {
            currentArmy = &gCombatManager->m_armies[stackSide][armyIndex];
            if ((currentArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)
                && currentArmy->m_powFrames != ARMY_POW_NONE) {
                vacatedHex = currentArmy->m_hex;
                if (ValidHex(vacatedHex))
                    gCombatManager->m_hexCells[vacatedHex].m_occupantSide = COMBAT_SIDE_NONE;
                if (currentArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    vacatedHex = currentArmy->m_hex + ARMY_WIDE_HEX_STEP(currentArmy->m_facing);
                    gCombatManager->m_hexCells[vacatedHex].m_occupantSide = COMBAT_SIDE_NONE;
                }
            } else if (currentArmy->m_animationSequence != ARMY_ANIMATION_ATTACK) {
                currentArmy->m_animationSequence = ARMY_ANIMATION_STAND;
            }
            currentArmy->m_powFrames = ARMY_POW_NONE;
            gCombatManager->UpdateGrid(currentArmy->m_hex, currentArmy->m_stats.attributes);
        }
    }
    gCombatManager->DrawFrame(true);
    for (stackSide = COMBAT_SIDE_FIRST; stackSide < COMBAT_SIDE_COUNT; stackSide++)
        for (armyIndex = 0; armyIndex < gCombatManager->m_numArmies[stackSide]; armyIndex++)
            WaitSample(gCombatManager->m_armies[stackSide][armyIndex].m_samples[ARMY_SAMPLE_WINCE]);
}

VA(0x00417d6f, 0x25)
u32 army::Strength(void) {
    return gMonsterDatabase[m_creatureType].fightValue * m_quantity;
}

// Plays a combat effect animation over this stack.
VA(0x00417d94, 0x11b)
void army::SpellEffect(H1_ENUM_PARAM(CombatEffectAnimation, i16) effect, i32 frameDelay) {
    i16 effectFrame;
    i16 iconFileId;
    i16 frameCount;

    m_effectAnimation = effect;
    iconFileId = MAKEFILEID(gCombatFxNames[effect]);
    if (gCurLoadedSpellFileId != iconFileId) {
        gResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gResourceManager->GetIcon(iconFileId);
        gCurLoadedSpellFileId = iconFileId;
    }
    m_animationSequence = ARMY_ANIMATION_EFFECT;
    frameCount = 10;
    gCombatManager->ResetLimitCreature();
    gCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    for (effectFrame = 0; effectFrame < frameCount; effectFrame++) {
        gCombatManager->m_computeExtent = true;
        gTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + frameDelay;
        gSpellEffectFrame = effectFrame;
        gCombatManager->DrawFrame(true);
        DelayTil(gTimers + COMBAT_EFFECT_TIMER_SLOT);
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
    i8 targetFound;
    i16 tryNumber;
    H1_ENUM_LOCAL(CombatHexDirection, i16) attackDir;
    i16 attackMask;
    i16 attackHex;
    i16 targetHex;

    targetFound = 0;
    attackDir = COMBAT_DIRECTION_NORTHEAST;
    tryNumber = 0;
    while (!targetFound) {
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
        if (attackMask != COMBAT_ALL_DIRECTIONS_BLOCKED) {
            while (!targetFound) {
                attackDir = H1_ENUM_DECODE(
                    CombatHexDirection,
                    Random(
                        H1_ENUM_ENCODE(CombatHexDirection, COMBAT_DIRECTION_NORTHEAST),
                        H1_ENUM_ENCODE(CombatHexDirection, COMBAT_DIRECTION_WIDE_EAST)
                    )
                );
                if (!(attackMask & H1_ENUM_BIT(CombatHexDirection, attackDir))) {
                    gNextAction = ACTION_MOVE;
                    ValidAttack(
                        m_hex,
                        attackDir,
                        ARMY_ATTACK_TARGET_OCCUPIED,
                        ARMY_HEX_INVALID,
                        &attackHex
                    );
                    gNextActionGridIndex = attackHex;
                    targetFound = 1;
                }
            }
        } else if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
            targetHex = Random(1, 43);
            if (gCombatManager->m_hexCells[targetHex].m_occupantSide != COMBAT_SIDE_NONE) {
                m_targetSide = gCombatManager->m_hexCells[targetHex].m_occupantSide;
                m_targetIndex = gCombatManager->m_hexCells[targetHex].m_occupantIndex;
                if (ValidFlight(targetHex, ARMY_PATH_ANY_TARGET_HEX)) {
                    SET_NEXT_COMBAT_MOVE(targetHex);
                    targetFound++;
                }
            } else {
                SET_NEXT_COMBAT_MOVE(targetHex);
            }
        } else {
            attackDir = H1_ENUM_DECODE(
                CombatHexDirection,
                Random(
                    H1_ENUM_ENCODE(CombatHexDirection, COMBAT_DIRECTION_NORTHEAST),
                    H1_ENUM_ENCODE(CombatHexDirection, COMBAT_DIRECTION_NORTHWEST)
                )
            );
            if (ValidMove(attackDir)) {
                SET_NEXT_COMBAT_MOVE(m_hex);
                gNextActionGridIndex = GetAdjacentCellIndex(gNextActionGridIndex, attackDir);
            }
            targetFound++;
        }
        tryNumber++;
    }
}

// Attacks the stack on the hex (flying, shooting or picking the adjacent
// direction) or moves there; a second argument forbids attacking.
#define adjacentCell cellItem // frame-slot spelling
VA(0x0041811f, 0x34a)
void army::MoveAttack(i32 destination, b32 moveOnly) {
    hexcell* adjacentCell;
    i32 attackFromHex;
    i16 baseAttackMask;
    i16 enemyAttackMask;
    i32 neighborHex;
    H1_ENUM_LOCAL(CombatHexDirection, i32) direction;

    gCombatManager->m_limitCreature = false;
    CLEAR_ARMY_TARGET(this);
    if (!ValidHex(destination))
        return;
    if (gCombatManager->m_hexCells[destination].m_occupantSide != COMBAT_SIDE_NONE
        && (gCombatManager->m_hexCells[destination].m_occupantSide != gCombatManager->m_currentSide
            || gCombatManager->m_hexCells[destination].m_occupantIndex
                   != gCombatManager->m_currentArmyIndex)) {
        if (moveOnly)
            return;
        m_targetSide = gCombatManager->m_hexCells[destination].m_occupantSide;
        m_targetIndex = gCombatManager->m_hexCells[destination].m_occupantIndex;
        m_moveTargetHex = destination;
        baseAttackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ASSIGNED, ARMY_HEX_INVALID);
        if ((m_stats.attributes & MONSTER_FLAGS_FLYING)
            && baseAttackMask == COMBAT_ALL_DIRECTIONS_BLOCKED && m_hex != m_moveTargetHex
            && !ValidFlight(m_moveTargetHex, ARMY_PATH_ANY_TARGET_HEX))
            return;
        if (m_spellEffect == SPELL_BERZERKER)
            enemyAttackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
        else
            enemyAttackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
        if (enemyAttackMask == COMBAT_ALL_DIRECTIONS_BLOCKED && m_stats.shots > 0) {
            SpecialAttack();
        } else if (baseAttackMask == COMBAT_ALL_DIRECTIONS_BLOCKED) {
            AttackTo();
        } else {
            for (direction = COMBAT_DIRECTION_FIRST; direction < COMBAT_DIRECTION_COUNT;
                 direction++) {
                if (direction < COMBAT_DIRECTION_ADJACENT_COUNT
                    || (m_stats.attributes & MONSTER_FLAGS_WIDE)) {
                    attackFromHex = m_hex;
                    if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && m_facing == ARMY_FACING_RIGHT
                        && direction >= COMBAT_DIRECTION_EASTERN_FIRST
                        && direction <= COMBAT_DIRECTION_EASTERN_LAST)
                        attackFromHex++;
                    if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && m_facing == ARMY_FACING_LEFT
                        && direction >= COMBAT_DIRECTION_WESTERN_FIRST
                        && direction <= COMBAT_DIRECTION_WESTERN_LAST)
                        attackFromHex--;
                    if (direction >= COMBAT_DIRECTION_WIDE_FIRST)
                        attackFromHex += ARMY_WIDE_HEX_STEP(m_facing);
                    neighborHex = GetAdjacentCellIndex(attackFromHex, direction);
                    if (ValidHex(neighborHex)) {
                        adjacentCell = &gCombatManager->m_hexCells[neighborHex];
                        if (HEX_HAS_OCCUPANT(*adjacentCell, m_targetSide, m_targetIndex))
                            m_attackDirection = direction;
                    }
                }
            }
            DoAttack(false);
        }
    } else if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
        m_moveTargetHex = destination;
        if (!ValidFlight(m_moveTargetHex, ARMY_PATH_ANY_TARGET_HEX))
            return;
        FlyTo(m_moveTargetHex);
    } else {
        WalkTo(destination);
    }
    gCombatManager->m_limitCreature = true;
}
#undef adjacentCell

// DamageEnemy sets this byte when the genie halves its target stack.
DATA(0x004a67d4)
b8 gGenieHalf;
