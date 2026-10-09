// The combat hero (general) and creature quick views.

#include <match.h>

#include <SOURCE/VIEW.h>

#include <BASE/dialog.h>
#include <BASE/display.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/widget.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>

#include <stdio.h>

// The combat hero window, with Cast Spell, Retreat and Surrender dimmed when
// the side cannot use them.
#define luck iLuck // frame-slot spelling
VA(0x00465ef0, 0x4fe)
i8 combatManager::ViewGeneral(
    H1_ENUM_PARAM(CombatSide, i32) side,
    b32 allowActions,
    b32 quickView
) {
    i16 pictureCtrlVal;
    i16 borderIdNum;
    i32 morale;
    i32 luck;
    i16 theBarId;
    i16 castSpellControl;
    tag_message packet;
    i16 surrenderBtn;
    i16 activeColorControl;
    heroWindow* wnd;
    i16 savedNameCtrl;
    i16 prevFrameWidgetId;
    i16 statBoxIdVal;
    i16 corner;
    i32 nextSpare;
    i16 oldCaption;
    i16 edgeCtrlVal;
    i16 retreatIdIndex;
    i16 activeCtrl;

    if (m_heroes[side] == NULL)
        return 0;
    // vgenwin.bin widget ids: retail stores the whole block though nothing
    // reads it.
    savedNameCtrl = GENERAL_NAME_WIDGET;
    pictureCtrlVal = GENERAL_PORTRAIT_WIDGET;
    activeColorControl = GENERAL_COLOR_WIDGET;
    statBoxIdVal = GENERAL_STATS_WIDGET;
    borderIdNum = GENERAL_CONTROL_NONE;
    oldCaption = GENERAL_NAME_WIDGET;
    corner = GENERAL_CONTROL_SEVEN;
    theBarId = GENERAL_CONTROL_EIGHT;
    edgeCtrlVal = GENERAL_CONTROL_NINE;
    castSpellControl = GENERAL_CAST_SPELL;
    retreatIdIndex = GENERAL_RETREAT;
    surrenderBtn = GENERAL_SURRENDER;
    activeCtrl = GENERAL_CONTROL_THIRTEEN;
    prevFrameWidgetId = GENERAL_CONTROL_FOURTEEN;
    gCurGeneral = side;
    packet.type = MESSAGE_WIDGET;
    wnd = new heroWindow(195, 60, "vgenwin.bin");
    if (wnd == NULL)
        MemError();
    sprintf(gText, "port%04d.icn", m_heroes[side]->m_portrait);
    packet.command = WIDGET_COMMAND_SET_ICON;
    packet.id = GENERAL_PORTRAIT_WIDGET;
    packet.text = gText;
    wnd->BroadcastMessage(packet);
    packet.command = WIDGET_COMMAND_SET_FRAME;
    packet.id = GENERAL_COLOR_WIDGET;
    packet.value =
        H1_ENUM_ENCODE(PlayerColor, gGame->m_players[m_heroes[side]->m_owner].Color()) + 1;
    wnd->BroadcastMessage(packet);
    sprintf(
        gText,
        localization::Tr("hero.title"),
        m_heroes[side]->m_name,
        gClassNames[m_heroes[side]->m_heroClass]
    );
    packet.command = WIDGET_COMMAND_SET_TEXT;
    packet.id = GENERAL_NAME_WIDGET;
    packet.text = gText;
    wnd->BroadcastMessage(packet);
    morale = m_heroes[side]->m_army.GetMorale(m_heroes[side], NULL);
    luck = gGame->GetLuck(m_heroes[side], NULL);
    sprintf(
        gText,
        "\n%s%d\n%s%d\n%s%d\n%s%d\n%s%s\n%s%s\n",
        gViewGeneralLabels[0],
        m_heroes[side]->m_primaryStats[HERO_PRIMARY_ATTACK],
        gViewGeneralLabels[1],
        m_heroes[side]->m_primaryStats[HERO_PRIMARY_DEFENSE],
        gViewGeneralLabels[2],
        m_heroes[side]->m_primaryStats[HERO_PRIMARY_SPELL_POWER],
        gViewGeneralLabels[3],
        m_heroes[side]->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
        gViewGeneralLabels[4],
        gMoraleText[morale + 3],
        gViewGeneralLabels[5],
        gLuckText[luck + 3]
    );
    packet.command = WIDGET_COMMAND_SET_TEXT;
    packet.id = GENERAL_STATS_WIDGET;
    packet.text = gText;
    wnd->BroadcastMessage(packet);
    if (m_heroes[side] == NULL || !allowActions || !m_heroes[side]->HasArtifact(ARTIFACT_MAGIC_BOOK)
        || m_heroCastSpell[side] || gCurGeneral != m_currentSide) {
        packet.command = WIDGET_COMMAND_CLEAR_FLAGS;
        packet.id = GENERAL_CAST_SPELL;
        packet.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(packet);
        packet.command = WIDGET_COMMAND_SET_FLAGS;
        packet.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(packet);
    }
    if (!allowActions || m_heroes[COMBAT_OPPOSING_SIDE(m_currentSide)] == NULL
        || gCurGeneral != m_currentSide) {
        packet.command = WIDGET_COMMAND_CLEAR_FLAGS;
        packet.id = GENERAL_SURRENDER;
        packet.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(packet);
        packet.command = WIDGET_COMMAND_SET_FLAGS;
        packet.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(packet);
    }
    if (!allowActions || gCurGeneral != m_currentSide
        || (gCurGeneral == COMBAT_DEFENDER_SIDE && m_combatTowns[COMBAT_DEFENDER_SIDE] != NULL)
        || m_sideRetreated[COMBAT_DEFENDER_SIDE] || m_sideRetreated[COMBAT_ATTACKER_SIDE]) {
        packet.command = WIDGET_COMMAND_CLEAR_FLAGS;
        packet.id = GENERAL_RETREAT;
        packet.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(packet);
        packet.command = WIDGET_COMMAND_SET_FLAGS;
        packet.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(packet);
    }
    if (quickView) {
        gMouseManager->ReallyHidePointer();
        gWindowManager->AddWindow(wnd, WINDOW_Z_ORDER_APPEND, true);
        QuickViewWait();
        gWindowManager->RemoveWindow(wnd);
        gMouseManager->ReallyShowPointer();
    } else
        gWindowManager->DoDialog(wnd, HandleViewGeneral, false);
    delete wnd;
    m_gridUpdateRow = 0;
    DrawFrame(true);
    if (!quickView)
        DoCommand(H1_ENUM_DECODE(CombatMessageCommand, gWindowManager->m_dialogResult));
    return 0;
}
#undef luck

// Cast Spell, Retreat, Surrender and Close end the dialog; hovering shows
// their help line.
VA(0x004663ee, 0x1a7)
H1_ENUM_RETURN(MessageDispatchResult, i16) HandleViewGeneral(tag_message& message) {
    H1_ENUM_LOCAL(ViewGeneralHoverHelp, i32) helpIndex;
    i16 prevCtrl;
    i16 borderId;
    i16 theBarId;
    i16 oldControl;
    i16 curSurrenderBtn;
    b8 dialogEnded;
    i16 colorControlVal;
    i16 nameCtrl;
    i16 oldFrameWidgetId;
    i16 statBoxIdPos;
    i16 activeCornerCtrl;
    i16 captionCtrl;
    i16 nextCtrl;
    i16 retreatIdIndex;
    i16 activeCtrl;

    nameCtrl = GENERAL_NAME_WIDGET;
    prevCtrl = GENERAL_PORTRAIT_WIDGET;
    colorControlVal = GENERAL_COLOR_WIDGET;
    statBoxIdPos = GENERAL_STATS_WIDGET;
    borderId = GENERAL_CONTROL_NONE;
    captionCtrl = GENERAL_NAME_WIDGET;
    activeCornerCtrl = GENERAL_CONTROL_SEVEN;
    theBarId = GENERAL_CONTROL_EIGHT;
    nextCtrl = GENERAL_CONTROL_NINE;
    oldControl = GENERAL_CAST_SPELL;
    retreatIdIndex = GENERAL_RETREAT;
    curSurrenderBtn = GENERAL_SURRENDER;
    activeCtrl = GENERAL_CONTROL_THIRTEEN;
    oldFrameWidgetId = GENERAL_CONTROL_FOURTEEN;
    dialogEnded = false;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case GENERAL_CAST_SPELL:
                    case GENERAL_RETREAT:
                    case GENERAL_SURRENDER:
                    case DIALOG_BUTTON_0:
                        if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)) {
                            gWindowManager->m_dialogResult = message.id;
                            dialogEnded = true;
                            break;
                        }
                }
                break;
            case WIDGET_COMMAND_HOVER:
                if (message.id == gWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case GENERAL_CAST_SPELL:
                        helpIndex = GENERAL_HOVER_HELP_CAST_SPELL;
                        break;
                    case GENERAL_RETREAT:
                        helpIndex = GENERAL_HOVER_HELP_RETREAT;
                        break;
                    case GENERAL_SURRENDER:
                        helpIndex = GENERAL_HOVER_HELP_SURRENDER;
                        break;
                    case DIALOG_BUTTON_0:
                        helpIndex = GENERAL_HOVER_HELP_CLOSE;
                        break;
                    default:
                        helpIndex = GENERAL_HOVER_HELP_HERO;
                        break;
                }
                gCombatManager->CombatMessage(gViewGeneralHelp[helpIndex], true);
                return MESSAGE_DISPATCH_CONSUME;
                break;
        }
    }
    if (dialogEnded) {
        message.id = H1_ENUM_ENCODE(BaseWidgetCommand, WIDGET_COMMAND_DIALOG_SELECT);
        message.command = H1_ENUM_DECODE(BaseWidgetCommand, message.id);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// The creature quick view, placed beside the stack and clamped to the screen.
#define windowX xWnd                        // frame-slot spelling
#define windowY yWindow                     // frame-slot spelling
#define stackOffsetX xDelta                 // frame-slot spelling
#define unusedViewWidth viewWidthConstant   // frame-slot spelling
#define unusedViewHeight viewHeightConstant // frame-slot spelling
#define unusedViewOffsetX viewXOffsetFixed  // frame-slot spelling
#define unusedViewOffsetY viewYOffsetConst  // frame-slot spelling
VA(0x00466595, 0x135)
void combatManager::ViewArmy(army* viewedArmy, H1_ENUM_PARAM(CombatSide, i32) side, b32 quickView) {
    i16 windowX;
    i16 unusedViewOffsetY;
    i16 windowY;
    i16 stackOffsetX;
    i16 unusedViewOffsetX;
    i16 unusedViewWidth;
    i16 unusedViewHeight;

    if (viewedArmy == NULL)
        return;
    unusedViewWidth = 488 - 86;
    unusedViewHeight = 229;
    unusedViewOffsetX = 86;
    unusedViewOffsetY = 164;
    windowX = m_hexCells[viewedArmy->m_hex].m_x;
    windowY = m_hexCells[viewedArmy->m_hex].m_y;
    stackOffsetX = (viewedArmy->m_facing == ARMY_FACING_LEFT ? 43 : 0) + 80;
    windowX -= stackOffsetX;
    if (windowX < 0)
        windowX = 0;
    if (windowX + 488 > LOGICAL_SCREEN_WIDTH)
        windowX = 151;
    windowY -= 164;
    if (windowY < 0)
        windowY = 0;
    if (windowY + 229 > COMBAT_VIEW_HEIGHT)
        windowY = 230;
    gGame->ViewArmy(
        windowX,
        windowY,
        viewedArmy->m_creatureType,
        viewedArmy->m_quantity,
        m_combatTowns[side],
        true,
        viewedArmy->m_facing,
        quickView,
        m_heroes[side],
        viewedArmy,
        m_armyGroups[side]
    );
}
#undef windowX
#undef windowY
#undef stackOffsetX
#undef unusedViewWidth
#undef unusedViewHeight
#undef unusedViewOffsetX
#undef unusedViewOffsetY
