// Combat stacks. Retail starts this object at army::army (0x00466490),
// after the SOURCE/AI object's int3 fill; hero::hero starts the next one.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/soundManager.h>
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

// DrawToBuffer's outline colours (palette indices FillToBuffer paints the
// sprite with): the stack m_limitCreature highlights, a beneficial spell
// (haste, bless, protection, anti-magic) and any other spell. SpecialAttack
// saves a MISSILE_PATCH_WIDTH x MISSILE_PATCH_HEIGHT screen patch centred on
// the missile (half sizes either side) and restores it each step (Buka
// ArmyDrawingConstant / CombatMissileAnimationConstant roles).
H1_ENUM_CONST_BEGIN(ArmyDrawingConstant)
    ARMY_LIMIT_OUTLINE_COLOR = 0xe4,
    ARMY_GOOD_SPELL_OUTLINE_COLOR = 0xf7,
    ARMY_BAD_SPELL_OUTLINE_COLOR = 0xe0,
    ARMY_MISSILE_PATCH_WIDTH = 70,
    ARMY_MISSILE_PATCH_HEIGHT = 60,
    ARMY_MISSILE_HALF_WIDTH = 35,
    ARMY_MISSILE_HALF_HEIGHT = 30
H1_ENUM_CONST_END(ArmyDrawingConstant)

VA(0x00466490, 0xc9)
army::army(void) {
    int i;

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
    giSpellEffectFrame = 0;
    CLEAR_ARMY_TARGET(this);
    m_attackDirection = COMBAT_DIRECTION_INVALID;
    m_unknown04 = 0;
    m_moveTargetHex = 0;
}

// The Windows build waits on no sample channel.
VA(0x00466559, 0x18)
void army::WaitSample(int) {
    return;
}

VA(0x00466571, 0x71)
void army::InitClean(void) {
    int i;

    for (i = 0; i < ARMY_SAMPLE_COUNT; i++)
        m_samples[i] = NULL;
    m_effectAnimation = COMBAT_EFFECT_NONE;
    m_drawShadow = 1;
    m_attackIcon = NULL;
    m_walkIcon = NULL;
    m_standIcon = NULL;
}

// The commanding hero's attack and defense raise the copied creature stats.
// @early-stop 99.91: `commander->m_primaryStats[0] + m_stats.attack` - the /Od
// add takes m_stats.attack first. vc4trace sortsim: both sides are two constant
// adds over a load (member offsets are value-hashed constants, so header member
// order cannot move them); only k4(this) and k4(commander) decide. Retail needs
// commander's C1 handle 10-11 past its place relative to `this` (or a whole-TU
// shift S with +k: S=1 k=9..11, S=2 k=8..11, S=5 k=5..8); no uniform shift in
// 0..511 works. Declaring commander at its first assignment moves it +1 only.
VA(0x004665e2, 0x122)
void army::Init(signed char type, short quantity, signed char side, signed char index) {
    hero* commander;

    InitClean();
    m_creatureType = type;
    memcpy(&m_stats, &gMonsterDatabase[type].stats, sizeof(tag_monsterStats));
    m_unknown29 = 6;
    m_spellEffect = SPELL_NONE;
    m_spellEndCondition = ARMY_CANCEL_SPELLS_NONE;
    commander = gpCombatManager->m_heroes[side];
    if (commander) {
        m_stats.attack = commander->m_primaryStats[HERO_PRIMARY_ATTACK] + m_stats.attack;
        m_stats.defense = commander->m_primaryStats[HERO_PRIMARY_DEFENSE] + m_stats.defense;
    }
    m_facing = side ^ 1;
    m_walkYStep = 0;
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    m_baseSpeed = m_stats.speed;
    m_quantity = quantity;
    m_initialQuantity = m_quantity;
    m_hitPointsLost = 0;
    m_damageMode = ARMY_DAMAGE_RANDOM;
    m_powFrames = ARMY_POW_NONE;
    m_side = side;
    m_index = index;
}

VA(0x00466704, 0x237)
void army::LoadResources(void) {
    char sprite[16];
    int i;
    char buf[16];

    if (m_creatureType != CREATURE_SWORDSMAN)
        strcpy(sprite, gArmyNames[m_creatureType]);
    else
        strcpy(sprite, "swrdsman");
    sprintf(gText, "%s.std", sprite);
    giMonoIconSkip = 0;
    m_standIcon = gpResourceManager->GetIcon(gText);
    giMonoIconSkip = -1;
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
    for (i = 0; i < ARMY_SAMPLE_COUNT; i++) {
        if (m_samples[i]) {
            m_samples[i]->m_playbackData.volume = ARMY_SAMPLE_VOLUME;
            m_samples[i]->m_playbackData.channelType = ARMY_SAMPLE_CHANNEL;
            m_samples[i]->m_playbackData.loopCount = 1;
        }
    }
}

// donor PoL RVA 0x0004b36e; preferred Buka symbol ?FreeResources@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463954;margin=0.140795;shape=0.318;size=0.842;calls=0.750;alternate=pol20:void army::FreeResources(void)@0x0004b36e
VA(0x0046693b, 0xf1)
void army::FreeResources(void) {
    int i;

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
VA(0x00466a2c, 0x855)
void army::DrawToBuffer(short x, short y) {
    short effectX;
    signed char offsetMode;
    signed char outlined;
    short outlineColor;
    char countText[ARMY_QUANTITY_TEXT_SIZE];
    short iconX;
    short qtyX;

    offsetMode = ICON_DRAW_OFFSET_FULL;
    outlined = 0;
    if ((m_animationFrame == 2 || m_animationFrame >= 3)
        && ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_hex % COMBAT_GRID_COLUMNS <= 2 || m_hex % COMBAT_GRID_COLUMNS >= 6)
            || !(m_stats.attributes & MONSTER_FLAGS_WIDE)
                   && (m_hex % COMBAT_GRID_COLUMNS <= 1 || m_hex % COMBAT_GRID_COLUMNS >= 7)))
        gbIconClipOn = 1;
    if (m_walkYStep) {
        y += m_animationFrame * m_walkYStep;
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
                        outlineColor = ARMY_GOOD_SPELL_OUTLINE_COLOR;
                        break;
                    default:
                        outlineColor = ARMY_BAD_SPELL_OUTLINE_COLOR;
                        break;
                }
                if (!outlined && m_animationFrame == 1)
                    m_standIcon->FillToBuffer(x, y, 0, outlineColor, m_facing, offsetMode);
                iconX = x;
                if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    if (m_facing == ARMY_FACING_RIGHT)
                        iconX += 75;
                    else
                        iconX -= 95;
                } else if (m_facing == ARMY_FACING_LEFT) {
                    iconX -= 39;
                }
                gpCombatManager->m_combatIcons[COMBAT_ICON_SPELLS]->DrawToBuffer(
                    iconX,
                    y - 40,
                    m_spellEffect,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (m_animationFrame == 1 && gpCombatManager->m_showArmyQuantities) {
                if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    if (m_facing == ARMY_FACING_RIGHT)
                        qtyX = x + 75;
                    else
                        qtyX = x - 95;
                } else {
                    if (m_facing == ARMY_FACING_RIGHT)
                        qtyX = x + 8;
                    else
                        qtyX = x - 39;
                }
                gpCombatManager->m_combatIcons[COMBAT_ICON_TEXTBAR]
                    ->DrawToBuffer(qtyX, y - 11, 5, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
                sprintf(countText, "%d", m_quantity);
                gpCombatManager->m_smallFont
                    ->DrawBoundedString(countText, qtyX, y - 12, 20, 12, 1, FONT_ALIGN_CENTER);
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
            effectX = x;
            if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (m_facing == ARMY_FACING_RIGHT) {
                    effectX += 39;
                    x += 75;
                } else {
                    effectX -= 39;
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
            gCurLoadedSpellIcon->DrawToBuffer(effectX, y, giSpellEffectFrame, m_facing, offsetMode);
            break;
    }
    gbIconClipOn = 0;
}

VA(0x00467281, 0x63)
void army::Stand(signed char redraw) {
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    m_walkYStep = 0;
    gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    if (redraw)
        gpCombatManager->DrawFrame(1);
}

VA(0x004672e4, 0x61)
void army::Wince(void) {
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 2;
    m_walkYStep = 0;
    gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
    gpCombatManager->SetGridMode(m_facing != ARMY_FACING_RIGHT);
}

// One hex of walking: six frames redrawn inside the union of the old and
// new extents; a stack turned away from the step moves before animating.
VA(0x00467345, 0x852)
void army::Walk(short direction, signed char standAfter, signed char continued) {
    int rectMaxX;
    int rectMaxY;
    short moveDist;
    short startFrame;
    short step;
    short reverse;
    short i;
    short baseHex;
    short flag;
    int rectMinX;
    int partnerHex;
    short targetHex;
    int rectMinY;
    int nextTail;

    if (!continued) {
        giMinExtentX = giMinExtentY = COMBAT_EXTENT_MIN_START;
        giMaxExtentX = giMaxExtentY = 0;
        gbComputeExtent = 1;
        gbSaveBiggestExtent = 1;
        DrawToBuffer(
            gpCombatManager->m_hexCells[m_hex].m_x,
            gpCombatManager->m_hexCells[m_hex].m_y
        );
        gbSaveBiggestExtent = 0;
        gbComputeExtent = 0;
    }
    if (giMinExtentX < 0)
        giMinExtentX = 0;
    if (giMinExtentY < 0)
        giMinExtentY = 0;
    if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
        giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
    if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
        giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
    rectMinX = giMinExtentX - 5;
    rectMinY = giMinExtentY - 5;
    rectMaxX = giMaxExtentX + 5;
    rectMaxY = giMaxExtentY + 5;
    moveDist = 16;
    reverse = 0;
    m_walkYStep = 0;
    if (direction < COMBAT_DIRECTION_WESTERN_FIRST) {
        if (m_facing == ARMY_FACING_RIGHT) {
            startFrame = 0;
            step = 1;
        } else {
            startFrame = 5;
            step = -1;
            reverse = 1;
        }
    } else if (m_facing == ARMY_FACING_LEFT) {
        startFrame = 0;
        step = 1;
    } else {
        startFrame = 5;
        step = -1;
        reverse = 1;
    }
    if (direction == COMBAT_DIRECTION_NORTHWEST || direction == COMBAT_DIRECTION_NORTHEAST)
        m_walkYStep = -16;
    if (direction == COMBAT_DIRECTION_SOUTHWEST || direction == COMBAT_DIRECTION_SOUTHEAST)
        m_walkYStep = 16;
    baseHex = m_hex;
    hexcell tempCell;
    hexcell tailCell;
    if (reverse) {
        targetHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(targetHex)) {
            tempCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
            if (m_stats.attributes & MONSTER_FLAGS_WIDE)
                tailCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex + (m_facing ? -1 : 1)]);
            gpCombatManager->m_hexCells[targetHex].TakeOccupant(&tempCell);
            partnerHex = targetHex + (m_facing ? -1 : 1);
            if (ValidHex(partnerHex) && (m_stats.attributes & MONSTER_FLAGS_WIDE))
                gpCombatManager->m_hexCells[partnerHex].TakeOccupant(&tailCell);
            m_hex = targetHex;
            if (m_walkYStep)
                m_walkYStep = -m_walkYStep;
        }
    } else {
        flag = 0;
        if (m_facing == ARMY_FACING_RIGHT && direction == COMBAT_DIRECTION_SOUTHEAST)
            flag = 1;
        else if (m_facing == ARMY_FACING_LEFT && direction == COMBAT_DIRECTION_NORTHWEST)
            flag = 1;
        gpCombatManager->SetGridMode(flag);
    }
    m_animationSequence = ARMY_ANIMATION_WALK;
    m_animationFrame = startFrame;
    gpSoundManager->MemorySample(m_samples[ARMY_SAMPLE_MOVE]);
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
            rectMinX = giMinExtentX;
            rectMinY = giMinExtentY;
            rectMaxX = giMaxExtentX;
            rectMaxY = giMaxExtentY;
        }
        giMinExtentX = giMinExtentY = COMBAT_EXTENT_MIN_START;
        giMaxExtentX = giMaxExtentY = 0;
        gbComputeExtent = 1;
        gbSaveBiggestExtent = 1;
        DrawToBuffer(
            gpCombatManager->m_hexCells[m_hex].m_x,
            gpCombatManager->m_hexCells[m_hex].m_y
        );
        gbComputeExtent = 0;
        gbSaveBiggestExtent = 0;
        if (giMinExtentX < 0)
            giMinExtentX = 0;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
            giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
        if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        gbCurrArmyDrawn = 0;
        gbComputeExtent = 1;
        gbLimitToExtent = 1;
        m_drawShadow = 0;
        gpCombatManager->DrawFrame(0);
        m_drawShadow = 1;
        gbLimitToExtent = 0;
        gbComputeExtent = 0;
        gbCurrArmyDrawn = 1;
        if (giMinExtentX < rectMinX)
            rectMinX = giMinExtentX;
        if (giMinExtentY < rectMinY)
            rectMinY = giMinExtentY;
        if (giMaxExtentX > rectMaxX)
            rectMaxX = giMaxExtentX;
        if (giMaxExtentY > rectMaxY)
            rectMaxY = giMaxExtentY;
        DelayTil(glTimers);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        gpWindowManager->UpdateScreenRegion(
            rectMinX,
            rectMinY,
            rectMaxX - rectMinX + 1,
            rectMaxY - rectMinY + 1
        );
        m_animationFrame += step;
    }
    if (!reverse) {
        targetHex = GetAdjacentCellIndex(m_hex, direction);
        if (ValidHex(targetHex)) {
            tempCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
            nextTail = m_hex + (m_facing ? -1 : 1);
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && ValidHex(nextTail))
                tailCell.TakeOccupant(&gpCombatManager->m_hexCells[nextTail]);
            gpCombatManager->m_hexCells[targetHex].TakeOccupant(&tempCell);
            nextTail = targetHex + (m_facing ? -1 : 1);
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && ValidHex(nextTail))
                gpCombatManager->m_hexCells[nextTail].TakeOccupant(&tailCell);
            m_hex = targetHex;
        }
    }
    if (standAfter == 1)
        Stand(1);
}

// A ranged attack: turn toward the target, animate the missile hex by hex
// over a saved screen patch, apply wall and luck modifiers, report the
// damage; creature 14 shoots twice.
VA(0x00467b97, 0xcca)
void army::SpecialAttack(void) {
    int targetHexCol;
    int dmg;
    int xEnd;
    signed char arrowFrame;
    int firstX;
    int destY;
    int killCount;
    army* target;
    int startY;
    int destX;
    int startX;
    int maxY;
    bitmap* saved;
    int iMaxX;
    signed char faceLeft;
    int facing;
    int dx;
    int dy;
    int yStep;
    int posY;
    int steps;
    int j;
    int yOffset[5];
    int i;
    int y2;
    signed char myRow;
    int y0;
    int prevY;
    signed char targetRow;
    signed char srcCol;
    signed char pitchSign;
    int offY;
    int minY;
    int prevX;
    signed char tgtCol;
    int offX;
    int minX;
    int xStep;
    int posX;
    signed char inCastle;

    facing = m_facing;
    m_walkYStep = 0;
    if (m_targetSide < 0 || m_targetIndex < 0)
        return;
    target = &gpCombatManager->m_armies[m_targetSide][m_targetIndex];
    tgtCol = target->m_hex % COMBAT_GRID_COLUMNS;
    targetRow = target->m_hex / COMBAT_GRID_COLUMNS;
    srcCol = m_hex % COMBAT_GRID_COLUMNS;
    myRow = m_hex / COMBAT_GRID_COLUMNS;
    facing = m_facing;
    if (tgtCol > srcCol || !(myRow & 1) && tgtCol == srcCol)
        m_facing = ARMY_FACING_RIGHT;
    else
        m_facing = ARMY_FACING_LEFT;
    gpCombatManager->SetGridMode(m_facing == ARMY_FACING_RIGHT);
    CheckLuck();
    m_animationSequence = ARMY_ANIMATION_ATTACK;
    gpSoundManager->MemorySample(m_samples[ARMY_SAMPLE_SHOOT]);
    for (i = 0; i < 4; i++) {
        m_animationFrame = i + 1;
        gpCombatManager->UpdateGrid(m_hex, m_stats.attributes);
        gpCombatManager->DrawFrame(1);
    }
    targetHexCol = tgtCol;
    if (target->m_stats.attributes & MONSTER_FLAGS_WIDE) {
        if (target->m_facing == ARMY_FACING_LEFT)
            targetHexCol--;
        else
            targetHexCol++;
    }
    dx = targetHexCol - srcCol;
    faceLeft = ICON_DRAW_NORMAL;
    if (dx < 0) {
        faceLeft = ICON_DRAW_FLIPPED;
        dx = -dx;
    }
    dy = targetRow - myRow;
    if (dy < 0)
        dy = -dy;
    steps = __max(dy, dx);
    arrowFrame = 7;
    pitchSign = 0;
    if (targetRow < myRow)
        pitchSign = -1;
    else if (targetRow > myRow)
        pitchSign = 1;
    if (dy > 1)
        arrowFrame += pitchSign;
    if (dx <= 3) {
        if (dy > 2)
            arrowFrame += pitchSign;
        if (dy == 1)
            arrowFrame += pitchSign;
    }
    yOffset[0] = -20;
    yOffset[1] = -15;
    yOffset[2] = 0;
    yOffset[3] = 15;
    yOffset[4] = 20;
    startX = gpCombatManager->m_hexCells[m_hex].m_x + (m_facing == ARMY_FACING_RIGHT ? 80 : -80);
    startY = gpCombatManager->m_hexCells[m_hex].m_y - 90 + yOffset[arrowFrame - 5];
    destX = gpCombatManager->m_hexCells[targetRow * COMBAT_GRID_COLUMNS + targetHexCol].m_x;
    destY = gpCombatManager->m_hexCells[targetRow * COMBAT_GRID_COLUMNS].m_y - 90;
    if (dx == 0)
        xStep = 0;
    else
        xStep = (destX - startX) / (steps * 2);
    if (dy == 0)
        yStep = 0;
    else
        yStep = (destY - startY) / (steps * 2);
    firstX = xStep + startX;
    xEnd = destX - xStep * steps * 2;
    offX = (firstX + xEnd) / 2 - firstX;
    y0 = yStep + startY;
    y2 = destY - steps * yStep * 2;
    offY = (y0 + y2) / 2 - y0;
    posX = offX + startX;
    posY = offY + startY;
    iMaxX = 0;
    minX = LOGICAL_SCREEN_WIDTH - 1;
    maxY = 0;
    minY = LOGICAL_SCREEN_HEIGHT - 1;
    saved = new bitmap(BITMAP_TYPE_MEMORY, ARMY_MISSILE_PATCH_WIDTH, ARMY_MISSILE_PATCH_HEIGHT);
    saved->GrabBitmap(
        gpWindowManager->m_screen,
        posX - ARMY_MISSILE_HALF_WIDTH,
        posY - ARMY_MISSILE_HALF_HEIGHT
    );
    prevX = posX;
    prevY = posY;
    for (j = 0; j < steps * 2; j++) {
        saved->DrawToBuffer(prevX - ARMY_MISSILE_HALF_WIDTH, prevY - ARMY_MISSILE_HALF_HEIGHT);
        if (prevX - ARMY_MISSILE_HALF_WIDTH < minX)
            minX = prevX - ARMY_MISSILE_HALF_WIDTH;
        if (prevX + ARMY_MISSILE_HALF_WIDTH > iMaxX)
            iMaxX = prevX + ARMY_MISSILE_HALF_WIDTH;
        if (prevY - ARMY_MISSILE_HALF_HEIGHT < minY)
            minY = prevY - ARMY_MISSILE_HALF_HEIGHT;
        if (prevY + ARMY_MISSILE_HALF_HEIGHT > maxY)
            maxY = prevY + ARMY_MISSILE_HALF_HEIGHT;
        saved->GrabBitmap(
            gpWindowManager->m_screen,
            posX - ARMY_MISSILE_HALF_WIDTH,
            posY - ARMY_MISSILE_HALF_HEIGHT
        );
        m_attackIcon->DrawToBuffer(posX, posY, arrowFrame, faceLeft, ICON_DRAW_OFFSET_FULL);
        if (posX - ARMY_MISSILE_HALF_WIDTH < minX)
            minX = posX - ARMY_MISSILE_HALF_WIDTH;
        if (posX + ARMY_MISSILE_HALF_WIDTH > iMaxX)
            iMaxX = posX + ARMY_MISSILE_HALF_WIDTH;
        if (posY - ARMY_MISSILE_HALF_HEIGHT < minY)
            minY = posY - ARMY_MISSILE_HALF_HEIGHT;
        if (posY + ARMY_MISSILE_HALF_HEIGHT > maxY)
            maxY = posY + ARMY_MISSILE_HALF_HEIGHT;
        DelayTil(glTimers);
        UPDATE_INCLUSIVE_REGION(minX, minY, iMaxX, maxY);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 15;
        prevX = posX;
        prevY = posY;
        posX += xStep;
        posY += yStep;
    }
    saved->DrawToBuffer(prevX - ARMY_MISSILE_HALF_WIDTH, prevY - ARMY_MISSILE_HALF_HEIGHT);
    gpWindowManager->UpdateScreenRegion(
        prevX - ARMY_MISSILE_HALF_WIDTH,
        prevY - ARMY_MISSILE_HALF_HEIGHT,
        ARMY_MISSILE_PATCH_WIDTH,
        ARMY_MISSILE_PATCH_HEIGHT
    );
    delete saved;
    m_stats.shots--;
    inCastle = 0;
    if (gpCombatManager->m_castleSide[COMBAT_DEFENDER_SIDE]
        && m_hex % COMBAT_GRID_COLUMNS <= COMBAT_CASTLE_WALL_COLUMN - 1
        && target->m_hex % COMBAT_GRID_COLUMNS >= COMBAT_CASTLE_WALL_COLUMN + 1) {
        int targetR;
        int wallDist;
        int gateHex;
        int hitRow;
        int colDist;
        int myR;
        int sCol;
        int tgtC;

        sCol = m_hex % COMBAT_GRID_COLUMNS;
        myR = m_hex / COMBAT_GRID_COLUMNS;
        colDist = sCol - COMBAT_CASTLE_WALL_COLUMN;
        tgtC = target->m_hex % COMBAT_GRID_COLUMNS;
        targetR = target->m_hex / COMBAT_GRID_COLUMNS;
        wallDist = COMBAT_CASTLE_WALL_COLUMN - sCol;
        hitRow = targetR;
        if (abs(targetR - myR) >= 2)
            hitRow -= -(-((targetR - myR) / 2));
        if (abs(targetR - myR) % 2 == 1) {
            if (colDist < wallDist || colDist == wallDist && (myR == 1 || myR == 3)) {
                if (myR < targetR)
                    hitRow--;
                else
                    hitRow++;
            }
        }
        if (hitRow > COMBAT_GRID_LAST_ROW)
            hitRow = COMBAT_GRID_LAST_ROW;
        if (hitRow < 0)
            hitRow = 0;
        if (gpCombatManager->m_hexCells[hitRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN]
                    .m_obstacleIndex
                == COMBAT_WALL_DAMAGED
            || gpCombatManager->m_hexCells[hitRow * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN]
                       .m_obstacleIndex
                   == COMBAT_WALL_INTACT)
            inCastle = 1;
        else
            inCastle = 0;
    }
    DamageEnemy(target, &dmg, &killCount, 1, inCastle ? ARMY_CASTLE_WALL_DEFENSE_BONUS : 0);
    if (killCount > 0)
        sprintf(
            gText,
            "%s %s %d %s.  %d %s %s.",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "do" : "does",
            dmg,
            "Damage",
            killCount,
            CREATURE_DISPLAY_NAME(target->m_creatureType, killCount),
            killCount > 1 ? "perish" : "perishes"
        );
    else
        sprintf(
            gText,
            "%s %s %d %s.",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "do" : "does",
            dmg,
            "Damage"
        );
    gText[0] -= 32;
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.powEffect);
    if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
        target->Stand(0);
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK)
        CancelSpell();
    WaitSample(ARMY_SAMPLE_SHOOT);
    m_facing = facing;
    Stand(1);
    if (target->m_quantity > 0)
        target->Stand(1);
    if (!gbSecondShot && m_creatureType == CREATURE_ELF && target->m_quantity > 0) {
        gbSecondShot = 1;
        SpecialAttack();
        gbSecondShot = 0;
    }
}

// Attacks every enemy next to the stack (the hydra), then turns them back.
VA(0x00468861, 0x765)
void army::DoHydraAttack(void) {
    int killedNow;
    int damage;
    short occSide;
    short i;
    short dir;
    short armyIndex;
    short attackMask;
    army* pTarget;
    short targetHex;
    int totalLost;
    int totDmg;
    army* eachArmy;

    m_walkYStep = 0;
    CheckLuck();
    gpSoundManager->MemorySample(m_samples[ARMY_SAMPLE_ATTACK]);
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
    totalLost = 0;
    totDmg = totalLost;
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (!(attackMask & (1 << dir))) {
            targetHex = m_hex;
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_facing == ARMY_FACING_LEFT && dir > COMBAT_DIRECTION_EASTERN_LAST
                    || m_facing == ARMY_FACING_RIGHT
                           && (dir < COMBAT_DIRECTION_WESTERN_FIRST
                               || dir > COMBAT_DIRECTION_WESTERN_LAST))) {
                if (m_facing == ARMY_FACING_LEFT)
                    targetHex = m_hex - 1;
                else
                    targetHex = m_hex + 1;
            }
            targetHex = GetAdjacentCellIndex(targetHex, dir);
            if (ValidHex(targetHex)) {
                occSide = gpCombatManager->m_hexCells[targetHex].m_occupantSide;
                armyIndex = gpCombatManager->m_hexCells[targetHex].m_occupantIndex;
                if (occSide >= 0 && armyIndex >= 0) {
                    gpCombatManager->m_limitCreatureCount[occSide][armyIndex]++;
                    pTarget = &gpCombatManager->m_armies[occSide][armyIndex];
                    if (!pTarget->m_hitByCreature) {
                        pTarget->m_hitByCreature = 1;
                        DamageEnemy(pTarget, &damage, &killedNow, 0, 0);
                        totDmg += damage;
                        totalLost += killedNow;
                    }
                }
            }
        }
    }
    if (totalLost > 0)
        sprintf(
            gText,
            "%s %s %d %s.  %d %s %s.",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "do" : "does",
            totDmg,
            "Damage",
            totalLost,
            totalLost > 1 ? "creatures" : "creature",
            totalLost > 1 ? "perish" : "perishes"
        );
    else
        sprintf(
            gText,
            "%s %s %d %s.",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "do" : "does",
            totDmg,
            "Damage"
        );
    gText[0] -= 32;
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.powEffect);
    WaitSample(ARMY_SAMPLE_ATTACK);
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
            targetHex = m_hex;
            if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
                && (m_facing == ARMY_FACING_LEFT && dir > COMBAT_DIRECTION_EASTERN_LAST
                    || m_facing == ARMY_FACING_RIGHT
                           && (dir < COMBAT_DIRECTION_WESTERN_FIRST
                               || dir > COMBAT_DIRECTION_WESTERN_LAST))) {
                if (m_facing == ARMY_FACING_LEFT)
                    targetHex = m_hex - 1;
                else
                    targetHex = m_hex + 1;
            }
            targetHex = GetAdjacentCellIndex(targetHex, dir);
            if (ValidHex(targetHex)) {
                occSide = gpCombatManager->m_hexCells[targetHex].m_occupantSide;
                armyIndex = gpCombatManager->m_hexCells[targetHex].m_occupantIndex;
                if (occSide >= 0 && armyIndex >= 0) {
                    gpCombatManager->m_limitCreatureCount[occSide][armyIndex]++;
                    pTarget = &gpCombatManager->m_armies[occSide][armyIndex];
                    if (!(pTarget->m_stats.attributes & MONSTER_FLAGS_DEAD))
                        pTarget->Stand(0);
                }
            }
        }
    }
    m_targetSide = targetHex = ARMY_HEX_INVALID;
    gpCombatManager->m_computeExtent = 0;
    for (occSide = 0; occSide < COMBAT_SIDE_COUNT; occSide++) {
        for (armyIndex = 0; armyIndex < gpCombatManager->m_numArmies[occSide]; armyIndex++) {
            eachArmy = &gpCombatManager->m_armies[occSide][armyIndex];
            if (!(eachArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)
                || eachArmy->m_powFrames == ARMY_POW_NONE)
                eachArmy->Stand(0);
        }
    }
    gpCombatManager->DrawFrame(1);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00468fc6, 0x2d)
void army::DirDoAttack(short direction) {
    m_attackDirection = direction;
    DoAttack(0);
}

// A melee strike in m_attackDirection: breath attackers (attribute 8) also
// hit the hex behind, some creatures cast on the target, the target
// retaliates once, and creatures 5 and 8 strike twice.
VA(0x00468ff3, 0x108d)
void army::DoAttack(int retaliation) {
    int unused;
    int oldMode;
    short frameBase;
    army* target2;
    int dmg;
    short newHex;
    int curDir;
    short facing;
    int attackDir;
    int didCast;
    army* target;
    int kills;

    oldMode = 0;
    dmg = 0;
    kills = 0;
    didCast = 0;
    if (retaliation)
        gpCombatManager->m_currentSide = 1 - gpCombatManager->m_currentSide;
    if (m_creatureType == CREATURE_HYDRA) {
        DoHydraAttack();
        if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_ATTACK && !retaliation)
            CancelSpell();
        goto secondStrike;
    }
    attackDir = m_attackDirection;
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
    gpCombatManager->SetGridMode(m_facing == ARMY_FACING_RIGHT);
    CheckLuck();
    newHex = m_hex;
    if ((m_stats.attributes & MONSTER_FLAGS_WIDE)
        && (facing == ARMY_FACING_LEFT && m_attackDirection >= COMBAT_DIRECTION_WESTERN_FIRST
            || facing == ARMY_FACING_RIGHT
                   && (m_attackDirection <= COMBAT_DIRECTION_EASTERN_LAST
                       || m_attackDirection >= COMBAT_DIRECTION_WIDE_FIRST))) {
        if (facing == ARMY_FACING_LEFT)
            newHex = m_hex - 1;
        else
            newHex = m_hex + 1;
    }
    newHex = GetAdjacentCellIndex(newHex, m_attackDirection);
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index]++;
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        short behindHex;

        if (ValidHex(newHex) && gpCombatManager->m_hexCells[newHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[newHex].m_occupantIndex >= 0)
            gpCombatManager
                ->m_limitCreatureCount[gpCombatManager->m_hexCells[newHex].m_occupantSide]
                                      [gpCombatManager->m_hexCells[newHex].m_occupantIndex]++;
        behindHex = GetAdjacentCellIndex(newHex, m_attackDirection);
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
    gpSoundManager->MemorySample(m_samples[ARMY_SAMPLE_ATTACK]);
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
    target2 = NULL;
    target = NULL;
    if (ValidHex(newHex)) {
        int savedKilled;
        short nextHex;

        if (gpCombatManager->m_hexCells[newHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[newHex].m_occupantIndex >= 0) {
            target =
                &gpCombatManager->m_armies[gpCombatManager->m_hexCells[newHex].m_occupantSide]
                                          [gpCombatManager->m_hexCells[newHex].m_occupantIndex];
            gpCombatManager->m_limitCreatureCount[target->m_side][target->m_index]++;
            gpCombatManager->m_computeExtent = 1;
            DamageEnemy(target, &dmg, &kills, 0, 0);
        }
        savedKilled = kills;
        nextHex = GetAdjacentCellIndex(newHex, m_attackDirection);
        if ((m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)
            && m_attackDirection < COMBAT_DIRECTION_ADJACENT_COUNT && ValidHex(nextHex)
            && gpCombatManager->m_hexCells[nextHex].m_occupantSide >= 0
            && gpCombatManager->m_hexCells[nextHex].m_occupantIndex >= 0
            && gpCombatManager->m_hexCells[newHex].m_occupantIndex
                   != gpCombatManager->m_hexCells[nextHex].m_occupantIndex) {
            gpCombatManager
                ->m_limitCreatureCount[gpCombatManager->m_hexCells[nextHex].m_occupantSide]
                                      [gpCombatManager->m_hexCells[nextHex].m_occupantIndex]++;
            newHex = nextHex;
            if (ValidHex(newHex)
                && gpCombatManager->m_hexCells[newHex].m_occupantSide != COMBAT_SIDE_NONE
                && gpCombatManager->m_hexCells[newHex].m_occupantIndex != COMBAT_ARMY_INDEX_NONE) {
                m_animationFrame = frameBase + 6;
                gpCombatManager->m_computeExtent = 1;
                gpCombatManager->DrawFrame(1);
                target2 =
                    &gpCombatManager->m_armies[gpCombatManager->m_hexCells[newHex].m_occupantSide]
                                              [gpCombatManager->m_hexCells[newHex].m_occupantIndex];
                DamageEnemy(target2, &dmg, &kills, 0, 0);
                if (target2->m_quantity > 0)
                    target2->Stand(1);
            }
        }
        kills = savedKilled;
    }
    if (gbGenieHalf)
        sprintf(
            gText,
            "%s %s half the enemy troops!",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "destroy" : "destroys"
        );
    else if (kills > 0)
        sprintf(
            gText,
            "%s %s %d %s.  %d %s %s.",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "do" : "does",
            dmg,
            "Damage",
            kills,
            CREATURE_DISPLAY_NAME(target->m_creatureType, kills),
            kills > 1 ? "perish" : "perishes"
        );
    else
        sprintf(
            gText,
            "%s %s %d %s.",
            CREATURE_DISPLAY_NAME(m_creatureType, m_quantity),
            m_quantity > 1 ? "do" : "does",
            dmg,
            "Damage"
        );
    gText[0] -= 32;
    gpCombatManager->CombatMessage(gText, 1);
    PowEffect(m_stats.powEffect);
    gpCombatManager->m_extendLimitDown = oldMode;
    switch (m_creatureType) {
        case CREATURE_CYCLOPS:
            if (SRandom(1, 5) == 3) {
                if (target && target->m_spellEffect != SPELL_ANTI_MAGIC
                    && target->m_creatureType != CREATURE_DRAGON
                    && (target->m_creatureType != CREATURE_DWARF || SRandom(0, 4) != 1)
                    && !(target->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                    gpCombatManager->CastSpell(SPELL_PARALYZE, target->m_hex, 1, ARMY_HEX_INVALID);
                    didCast = 1;
                }
            } else if (SRandom(1, 5) == 3 && target2 && target2->m_spellEffect != SPELL_ANTI_MAGIC
                       && target2->m_creatureType != CREATURE_DRAGON
                       && (target2->m_creatureType != CREATURE_DWARF || SRandom(0, 4) != 1)
                       && !(target2->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                gpCombatManager->CastSpell(SPELL_PARALYZE, target2->m_hex, 1, ARMY_HEX_INVALID);
                didCast = 1;
            }
            break;
        case CREATURE_UNICORN:
            if (SRandom(1, 5) == 3 && target && target->m_spellEffect != SPELL_ANTI_MAGIC
                && target->m_creatureType != CREATURE_DRAGON
                && (target->m_creatureType != CREATURE_DWARF || SRandom(0, 127) % 4 != 1)
                && !(target->m_stats.attributes & MONSTER_FLAGS_DEAD)) {
                gpCombatManager->CastSpell(SPELL_BLIND, target->m_hex, 1, ARMY_HEX_INVALID);
                didCast = 1;
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
    if (target) {
        gpCombatManager->m_limitCreatureCount[target->m_side][target->m_index] = 1;
        if (!(target->m_stats.attributes & MONSTER_FLAGS_DEAD))
            target->Stand(0);
    }
    if (target2) {
        gpCombatManager->m_limitCreatureCount[target2->m_side][target2->m_index] = 1;
        if (!(target2->m_stats.attributes & MONSTER_FLAGS_DEAD))
            target2->Stand(0);
    }
    WaitSample(ARMY_SAMPLE_ATTACK);
    if (m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) {
        if (target2) {
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
    if (target && target->m_quantity > 0) {
        gpCombatManager->m_computeExtent = 1;
        target->Stand(1);
        if (target->m_spellEffect == SPELL_PARALYZE
            || target->m_creatureType != CREATURE_GRIFFIN
                   && (target->m_stats.attributes & MONSTER_FLAGS_RETALIATED)
            || m_creatureType == CREATURE_ROGUE || m_creatureType == CREATURE_SPRITE || didCast
            || retaliation) {
            goto secondStrike;
        } else {
            target->m_attackDirection = OppositeDirection(m_attackDirection);
            if (target->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                short checkHex;

                checkHex = GetAdjacentCellIndex(
                    target->m_hex,
                    target->m_facing ? COMBAT_DIRECTION_NORTHWEST : COMBAT_DIRECTION_NORTHEAST
                );
                if (m_hex == checkHex)
                    target->m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                checkHex = GetAdjacentCellIndex(
                    target->m_hex,
                    static_cast<signed char>(
                        target->m_facing ? COMBAT_DIRECTION_SOUTHWEST : COMBAT_DIRECTION_SOUTHEAST
                    )
                );
                if (m_hex == checkHex)
                    target->m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
            }
            target->DoAttack(1);
            target->m_stats.attributes |= MONSTER_FLAGS_RETALIATED;
            if (target->m_creatureType == CREATURE_GHOST)
                target->m_quantity +=
                    gpCombatManager
                        ->m_ghostKills[gpCombatManager->m_hexCells[target->m_hex].m_occupantSide];
        }
    }
secondStrike:
    if ((m_creatureType == CREATURE_WOLF || m_creatureType == CREATURE_PALADIN) && target
        && target->m_quantity > 0 && !retaliation && m_spellEffect != SPELL_PARALYZE
        && m_quantity > 0) {
        curDir = m_attackDirection;
        m_attackDirection = attackDir;
        DoAttack(1);
        m_attackDirection = curDir;
    }
    m_targetSide = newHex = ARMY_HEX_INVALID;
    if (retaliation)
        gpCombatManager->m_currentSide = 1 - gpCombatManager->m_currentSide;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046a080, 0x49)
void army::ResetPath(void) {
    short i;

    for (i = 0; i < COMBAT_HEX_COUNT; i++)
        gpCombatManager->m_hexCells[i].m_pathFlag = 0;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046a0c9, 0x27)
short army::WalkTo(void) {
    return WalkTo(m_moveTargetHex);
}

// Walks the found path one hex at a time, at most the stack's speed.
VA(0x0046a0f0, 0xfc)
short army::WalkTo(short destHex) {
    signed char step;
    int moved;

    m_targetSide = m_targetIndex = COMBAT_ARMY_INDEX_NONE;
    if (!FindPath(m_hex, destHex, m_stats.speed, 1, ARMY_PATH_ANY_TARGET_HEX))
        return ARMY_PATH_BLOCKED;
    moved = 0;
    for (step = gpSearchArray->m_pathLength - 1; step >= 0; step--) {
        Walk(gpSearchArray->m_directions[step], 0, gpSearchArray->m_pathLength - 1 != step);
        moved++;
        if (moved >= m_stats.speed)
            step = -1;
    }
    if (!m_spellEndCondition)
        CancelSpell();
    Stand(1);
    return 0;
}

VA(0x0046a1ec, 0x27)
short army::AttackTo(void) {
    return AttackTo(m_moveTargetHex);
}

// Flyers jump next to the target; walkers stop short when out of moves.
VA(0x0046a213, 0x1c9)
short army::AttackTo(short destHex) {
    signed char step;
    int moved;

    if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
        if (m_hex != destHex)
            FlyTo(destHex);
        DoAttack(0);
        return 0;
    }
    if ((m_stats.attributes & MONSTER_FLAGS_BREATH_ATTACK) && m_hex == m_moveTargetHex) {
        DoAttack(0);
        return 0;
    }
    if (FindPath(m_hex, destHex, m_stats.speed, 1, ARMY_PATH_ANY_TARGET_HEX)) {
        if (gpSearchArray->m_pathLength == 1) {
            m_attackDirection = gpSearchArray->m_directions[0];
            DoAttack(0);
        } else {
            step = 0;
            moved = 0;
            for (step = gpSearchArray->m_pathLength - 1; step; step--) {
                Walk(gpSearchArray->m_directions[step], 0, gpSearchArray->m_pathLength - 1 != step);
                moved++;
                if (moved >= m_stats.speed && step != 1) {
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

// donor PoL RVA 0x0004f93e; preferred Buka symbol ?CheckLuck@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.723107;margin=0.234495;shape=0.473;size=0.925;calls=0.875;strings=badluck.82m|goodluck.82m;alternate=pol20:void army::CheckLuck(void)@0x0004f93e
VA(0x0046a3dc, 0x249)
void army::CheckLuck(void) {
    int luck;

    if (!gpCombatManager->m_heroes[m_side])
        return;
    m_luck = ARMY_LUCK_NONE;
    luck = gpGame->GetLuck(gpCombatManager->m_heroes[m_side], this);
    if (luck > 0 && SRandom(1, 12) <= luck)
        m_luck = ARMY_LUCK_GOOD;
    if (luck < 0 && SRandom(1, 12) < -luck)
        m_luck = ARMY_LUCK_BAD;
    if (m_luck) {
        SAMPLE2 sample = NULL_SAMPLE2;
        if (m_luck < 0)
            sprintf(gText, "badluck.82m");
        else
            sprintf(gText, "goodluck.82m");
        sample = LoadPlaySample(gText);
        if (m_luck < 0) {
            sprintf(
                gText,
                "Bad luck descends on the %s",
                CREATURE_DISPLAY_NAME(m_creatureType, m_quantity)
            );
            gpCombatManager->CombatMessage(gText, 1);
            Wince();
            SpellEffect(COMBAT_EFFECT_BAD_LUCK, 180);
        } else {
            sprintf(
                gText,
                "Good luck shines on the %s",
                CREATURE_DISPLAY_NAME(m_creatureType, m_quantity)
            );
            gpCombatManager->CombatMessage(gText, 1);
            Stand(1);
            SpellEffect(COMBAT_EFFECT_GOOD_LUCK, 180);
        }
        Stand(1);
        WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
    }
}

// donor PoL RVA 0x0004fbc0; preferred Buka symbol ?DamageEnemy@army@@QAEXPAV1@PAH1HH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.387284;margin=0.228483;shape=0.296;size=0.628;calls=0.714;alternate=pol20:void army::DamageEnemy(class army *, int *, int *, int, int)@0x0004fbc0
VA(0x0046a625, 0x2ae)
void army::DamageEnemy(
    class army* target,
    int* damageResult,
    int* killedResult,
    int rangedAttack,
    int defenseModifier
) {
    float total;
    short delta;
    int halfDamage;
    int damage;
    short defenseBonus;
    short index;
    short attBonus;

    if (!target)
        return;
    total = 0;
    gbGenieHalf = 0;
    for (index = 0; index < m_quantity; index++) {
        switch (m_damageMode) {
            case ARMY_DAMAGE_MAXIMUM:
                total += m_stats.damageMax;
                break;
            case ARMY_DAMAGE_MINIMUM:
                total += m_stats.damageMin;
                break;
            default:
                total += SRandom(m_stats.damageMin, m_stats.damageMax);
                break;
        }
    }
    attBonus = 0;
    defenseBonus = 0;
    delta = m_stats.attack + attBonus - (target->m_stats.defense + defenseBonus + defenseModifier);
    if (delta > 20)
        delta = 20;
    if (delta < -20)
        delta = -20;
    total *= gfBattleStat[delta + 20];
    if (m_luck > 0)
        total *= 2;
    if (m_luck < 0)
        total /= 2;
    m_luck = ARMY_LUCK_NONE;
    if ((m_stats.attributes & MONSTER_FLAGS_SHOOTER) && !rangedAttack)
        total /= 2;
    if (m_damageMode == ARMY_DAMAGE_HALF)
        total /= 2;
    damage = static_cast<int>(total + 0.5);
    if (m_creatureType == CREATURE_GENIE && SRandom(1, 5) == 2) {
        halfDamage = target->m_stats.hitPoints * ((target->m_quantity + 1) / 2);
        if (damage < halfDamage) {
            gbGenieHalf = 1;
            damage = halfDamage;
        }
    }
    if (damage > 32000)
        damage = 32000;
    if (damage <= 0)
        damage = 1;
    *damageResult = damage;
    *killedResult = target->Damage(damage);
}

// donor PoL RVA 0x0005012e; preferred Buka symbol ?Damage@army@@QAEHJH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.419408;margin=1.157007;shape=0.216;size=0.693;calls=1.000;alternate=pol20:int army::Damage(long int, int)@0x0005012e
// A stack whose spell (2) breaks on damage loses it.
VA(0x0046a8d3, 0x176)
int army::Damage(long int damage) {
    signed char facing;
    int minKilled;
    int kills;

    damage += m_hitPointsLost;
    kills = damage / m_stats.hitPoints;
    m_hitPointsLost = damage % m_stats.hitPoints;
    minKilled = m_quantity / 5;
    if (minKilled == 0)
        minKilled = 1;
    if (kills > 0)
        m_powFrames = ARMY_POW_FRAMES_HIT;
    else
        m_powFrames = ARMY_POW_NONE;
    if (m_quantity < kills)
        kills = m_quantity;
    m_quantity = m_quantity - kills;
    if (m_quantity <= 0)
        m_powFrames = ARMY_POW_FRAMES_KILLED;
    facing = m_facing;
    m_facing = gpCombatManager
                   ->m_armies[gpCombatManager->m_currentSide][gpCombatManager->m_currentArmyIndex]
                   .m_facing
               ^ 1;
    Wince();
    m_facing = facing;
    gpCombatManager->DrawFrame(1);
    if (m_spellEndCondition == ARMY_CANCEL_SPELLS_AFTER_DAMAGE) {
        m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
        if (m_spellEffect != SPELL_BLIND)
            m_stats.attributes |= MONSTER_FLAGS_RETALIATED;
        CancelSpell();
    }
    return kills;
}

// Plays the impact effect on every stack hit this attack (m_powFrames),
// fading the killed ones out, then restores the grid.
VA(0x0046aa49, 0x8a9)
void army::PowEffect(signed char effect) {
    short frames;
    short stackIndex;
    short armyNum;
    short step;
    short longest;
    int cellHex;
    army* curArmy;
    short side;

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
    if (effect != gCurLoadedSpellFileId) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(gPowEffectNames[effect]);
        gCurLoadedSpellFileId = effect;
    }
    for (side = 0; side < COMBAT_SIDE_COUNT; side++)
        for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++)
            if (gpCombatManager->m_armies[side][stackIndex].m_powFrames > 0)
                gpSoundManager->MemorySample(
                    gpCombatManager->m_armies[side][stackIndex].m_samples[ARMY_SAMPLE_WINCE]
                );
    step = 0;
    gpCombatManager->ResetLimitCreature();
    while (step < frames && step < 5) {
        gpCombatManager->m_computeExtent = 1;
        glTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + 30;
        for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
            for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++) {
                if (gpCombatManager->m_armies[side][stackIndex].m_powFrames >= step) {
                    gpCombatManager->m_armies[side][stackIndex].m_animationSequence =
                        ARMY_ANIMATION_EFFECT;
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex])
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                } else if (gpCombatManager->m_armies[side][stackIndex].m_animationSequence
                           != ARMY_ANIMATION_ATTACK) {
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex]
                        && gpCombatManager->m_armies[side][stackIndex].m_animationSequence)
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                    gpCombatManager->m_armies[side][stackIndex].m_animationSequence =
                        ARMY_ANIMATION_STAND;
                }
            }
        }
        giSpellEffectFrame = step;
        gpCombatManager->DrawFrame(1);
        step++;
    }
    while (step < frames && step < 10) {
        gpCombatManager->m_computeExtent = 1;
        glTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + 30;
        for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
            for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++) {
                if (gpCombatManager->m_armies[side][stackIndex].m_powFrames
                    >= ARMY_POW_FRAMES_KILLED) {
                    gpCombatManager->m_armies[side][stackIndex].m_animationSequence =
                        ARMY_ANIMATION_EFFECT;
                    gpCombatManager->m_armies[side][stackIndex].m_stats.attributes |=
                        MONSTER_FLAGS_DEAD;
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex])
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                } else if (gpCombatManager->m_armies[side][stackIndex].m_animationSequence
                           != ARMY_ANIMATION_ATTACK) {
                    if (!gpCombatManager->m_limitCreatureCount[side][stackIndex]
                        && gpCombatManager->m_armies[side][stackIndex].m_animationSequence)
                        gpCombatManager->m_limitCreatureCount[side][stackIndex]++;
                    gpCombatManager->m_armies[side][stackIndex].m_animationSequence =
                        ARMY_ANIMATION_STAND;
                }
            }
        }
        giSpellEffectFrame = step;
        gpCombatManager->DrawFrame(1);
        step++;
    }
    while (++step < 10)
        DelayMilli(15);
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++) {
            curArmy = &gpCombatManager->m_armies[side][stackIndex];
            if ((curArmy->m_stats.attributes & MONSTER_FLAGS_DEAD)
                && curArmy->m_powFrames != ARMY_POW_NONE) {
                cellHex = curArmy->m_hex;
                if (ValidHex(cellHex))
                    gpCombatManager->m_hexCells[cellHex].m_occupantSide = COMBAT_SIDE_NONE;
                if (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                    cellHex = curArmy->m_hex + (curArmy->m_facing ? -1 : 1);
                    gpCombatManager->m_hexCells[cellHex].m_occupantSide = COMBAT_SIDE_NONE;
                }
            } else if (curArmy->m_animationSequence != ARMY_ANIMATION_ATTACK) {
                curArmy->m_animationSequence = ARMY_ANIMATION_STAND;
            }
            curArmy->m_powFrames = ARMY_POW_NONE;
            gpCombatManager->UpdateGrid(curArmy->m_hex, curArmy->m_stats.attributes);
        }
    }
    gpCombatManager->DrawFrame(1);
    for (side = 0; side < COMBAT_SIDE_COUNT; side++)
        for (stackIndex = 0; stackIndex < gpCombatManager->m_numArmies[side]; stackIndex++)
            gpCombatManager->m_armies[side][stackIndex].WaitSample(ARMY_SAMPLE_WINCE);
}

VA(0x0046b2f2, 0x34)
unsigned long int army::Strength(void) {
    return gMonsterDatabase[m_creatureType].fightValue * m_quantity;
}

// Plays a combat effect animation over this stack.
VA(0x0046b326, 0x131)
void army::SpellEffect(short effect, int frameDelay) {
    short frame;
    short effectFileId;
    short frameCount;

    m_effectAnimation = effect;
    effectFileId = MAKEFILEID(gCombatFxNames[effect]);
    if (gCurLoadedSpellFileId != effectFileId) {
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
        gCurLoadedSpellIcon = gpResourceManager->GetIcon(effectFileId);
        gCurLoadedSpellFileId = effectFileId;
    }
    m_animationSequence = ARMY_ANIMATION_EFFECT;
    frameCount = 10;
    gpCombatManager->ResetLimitCreature();
    gpCombatManager->m_limitCreatureCount[m_side][m_index] = 1;
    for (frame = 0; frame < frameCount; frame++) {
        gpCombatManager->m_computeExtent = 1;
        glTimers[COMBAT_EFFECT_TIMER_SLOT] = KBTickCount() + frameDelay;
        giSpellEffectFrame = frame;
        gpCombatManager->DrawFrame(1);
        DelayTil(glTimers + COMBAT_EFFECT_TIMER_SLOT);
    }
    m_effectAnimation = COMBAT_EFFECT_NONE;
}

// Slow (and the other speed spells) restore the base speed and flight;
// effect 9 gave three defense.
VA(0x0046b457, 0xb2)
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
VA(0x0046b509, 0x1de)
void army::GoBerserk(void) {
    signed char found;
    short tryCount;
    short dir;
    short attackMask;
    short targetHex;
    short target;

    found = 0;
    dir = COMBAT_DIRECTION_NORTHEAST;
    tryCount = 0;
    while (!found) {
        attackMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
        if (attackMask != COMBAT_ALL_DIRECTIONS_BLOCKED) {
            while (!found) {
                dir = Random(COMBAT_DIRECTION_NORTHEAST, COMBAT_DIRECTION_WIDE_EAST);
                if (!(attackMask & (1 << dir))) {
                    giNextAction = ACTION_MOVE;
                    ValidAttack(
                        m_hex,
                        dir,
                        ARMY_ATTACK_TARGET_OCCUPIED,
                        ARMY_HEX_INVALID,
                        &targetHex
                    );
                    giNextActionGridIndex = targetHex;
                    found = 1;
                }
            }
        } else if (m_stats.attributes & MONSTER_FLAGS_FLYING) {
            target = Random(1, 43);
            if (gpCombatManager->m_hexCells[target].m_occupantSide != COMBAT_SIDE_NONE) {
                m_targetSide = gpCombatManager->m_hexCells[target].m_occupantSide;
                m_targetIndex = gpCombatManager->m_hexCells[target].m_occupantIndex;
                if (ValidFlight(target, ARMY_PATH_ANY_TARGET_HEX)) {
                    giNextAction = ACTION_MOVE;
                    giNextActionGridIndex = target;
                    found++;
                }
            } else {
                giNextAction = ACTION_MOVE;
                giNextActionGridIndex = target;
            }
        } else {
            dir = Random(COMBAT_DIRECTION_NORTHEAST, COMBAT_DIRECTION_NORTHWEST);
            if (ValidMove(dir)) {
                giNextAction = ACTION_MOVE;
                giNextActionGridIndex = m_hex;
                giNextActionGridIndex = GetAdjacentCellIndex(giNextActionGridIndex, dir);
            }
            found++;
        }
        tryCount++;
    }
}

// Attacks the stack on the hex (flying, shooting or picking the adjacent
// direction) or moves there; a second argument forbids attacking.
VA(0x0046b6e7, 0x3a3)
void army::MoveAttack(int hex, int moveOnly) {
    hexcell* pCell;
    int baseHex;
    short meleeMask;
    short atkMask;
    int adjHex;
    int dirIndex;

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
            && meleeMask == COMBAT_ALL_DIRECTIONS_BLOCKED && m_moveTargetHex != m_hex
            && !ValidFlight(m_moveTargetHex, ARMY_PATH_ANY_TARGET_HEX))
            return;
        if (m_spellEffect == SPELL_BERZERKER)
            atkMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_OCCUPIED, ARMY_HEX_INVALID);
        else
            atkMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID);
        if (atkMask == COMBAT_ALL_DIRECTIONS_BLOCKED && m_stats.shots > 0) {
            SpecialAttack();
        } else if (meleeMask == COMBAT_ALL_DIRECTIONS_BLOCKED) {
            AttackTo();
        } else {
            for (dirIndex = 0; dirIndex < COMBAT_DIRECTION_COUNT; dirIndex++) {
                if (dirIndex < COMBAT_DIRECTION_ADJACENT_COUNT
                    || (m_stats.attributes & MONSTER_FLAGS_WIDE)) {
                    baseHex = m_hex;
                    if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && m_facing == ARMY_FACING_RIGHT
                        && dirIndex >= COMBAT_DIRECTION_EASTERN_FIRST
                        && dirIndex <= COMBAT_DIRECTION_EASTERN_LAST)
                        baseHex++;
                    if ((m_stats.attributes & MONSTER_FLAGS_WIDE) && m_facing == ARMY_FACING_LEFT
                        && dirIndex >= COMBAT_DIRECTION_WESTERN_FIRST
                        && dirIndex <= COMBAT_DIRECTION_WESTERN_LAST)
                        baseHex--;
                    if (dirIndex >= COMBAT_DIRECTION_WIDE_FIRST) {
                        if (m_facing == ARMY_FACING_RIGHT)
                            baseHex++;
                        else
                            baseHex--;
                    }
                    adjHex = GetAdjacentCellIndex(baseHex, dirIndex);
                    if (ValidHex(adjHex)) {
                        pCell = &gpCombatManager->m_hexCells[adjHex];
                        if (pCell->m_occupantSide == m_targetSide
                            && pCell->m_occupantIndex == m_targetIndex)
                            m_attackDirection = dirIndex;
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

// ARMY owns retail .data 0x004a0820-0x004a0a57 and .bss 0x004ca908-0x004ca917.
DATA(0x004a0888)
int gbSecondShot = 0;
DATA(0x004ca908)
signed char gbGenieHalf;
