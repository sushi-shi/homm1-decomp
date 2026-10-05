// HoMM1 VIEW: the combat hero (general) and creature quick views.
// Retail int3 padding bounds this object at 0x00438310-0x00438bf1.

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
VA(0x00465ef0, 0x4fe)
i8 combatManager::ViewGeneral(i32 side, i32 allowActions, i32 quickView) {
    i16 pictureCtrlVal;
    i16 borderIdNum;
    i32 morale;
    i32 iLuck;
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
    giCurGeneral = side;
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
    packet.value = gpGame->m_players[m_heroes[side]->m_owner].Color() + 1;
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
    iLuck = gpGame->GetLuck(m_heroes[side], NULL);
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
        gLuckText[iLuck + 3]
    );
    packet.command = WIDGET_COMMAND_SET_TEXT;
    packet.id = GENERAL_STATS_WIDGET;
    packet.text = gText;
    wnd->BroadcastMessage(packet);
    if (m_heroes[side] == NULL || allowActions == 0
        || !m_heroes[side]->HasArtifact(ARTIFACT_MAGIC_BOOK) || m_heroCastSpell[side] != 0
        || giCurGeneral != m_currentSide) {
        packet.command = WIDGET_COMMAND_CLEAR_FLAGS;
        packet.id = GENERAL_CAST_SPELL;
        packet.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(packet);
        packet.command = WIDGET_COMMAND_SET_FLAGS;
        packet.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(packet);
    }
    if (allowActions == 0 || m_heroes[1 - m_currentSide] == NULL || giCurGeneral != m_currentSide) {
        packet.command = WIDGET_COMMAND_CLEAR_FLAGS;
        packet.id = GENERAL_SURRENDER;
        packet.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(packet);
        packet.command = WIDGET_COMMAND_SET_FLAGS;
        packet.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(packet);
    }
    if (allowActions == 0 || giCurGeneral != m_currentSide
        || (giCurGeneral == COMBAT_DEFENDER_SIDE && m_combatTowns[COMBAT_DEFENDER_SIDE] != NULL)
        || m_sideRetreated[COMBAT_DEFENDER_SIDE] != 0
        || m_sideRetreated[COMBAT_ATTACKER_SIDE] != 0) {
        packet.command = WIDGET_COMMAND_CLEAR_FLAGS;
        packet.id = GENERAL_RETREAT;
        packet.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(packet);
        packet.command = WIDGET_COMMAND_SET_FLAGS;
        packet.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(packet);
    }
    if (quickView) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(wnd, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(wnd);
        gpMouseManager->ReallyShowPointer();
    } else
        gpWindowManager->DoDialog(wnd, HandleViewGeneral, 0);
    delete wnd;
    m_gridUpdateRow = 0;
    DrawFrame(1);
    if (!quickView)
        DoCommand(gpWindowManager->m_dialogResult);
    return 0;
}

// Cast Spell, Retreat, Surrender and Close end the dialog; hovering shows
// their help line.
VA(0x004663ee, 0x1a7)
H1_ENUM_RETURN(MessageDispatchResult, i16) HandleViewGeneral(tag_message& message) {
    i32 pos;
    i16 prevCtrl;
    i16 borderId;
    i16 theBarId;
    i16 oldControl;
    i16 curSurrenderBtn;
    i8 result;
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
    result = 0;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case GENERAL_CAST_SPELL:
                    case GENERAL_RETREAT:
                    case GENERAL_SURRENDER:
                    case DIALOG_BUTTON_0:
                        if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)) {
                            gpWindowManager->m_dialogResult = message.id;
                            result = 1;
                            break;
                        }
                }
                break;
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gpWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case GENERAL_CAST_SPELL:
                        pos = GENERAL_HOVER_HELP_CAST_SPELL;
                        break;
                    case GENERAL_RETREAT:
                        pos = GENERAL_HOVER_HELP_RETREAT;
                        break;
                    case GENERAL_SURRENDER:
                        pos = GENERAL_HOVER_HELP_SURRENDER;
                        break;
                    case DIALOG_BUTTON_0:
                        pos = GENERAL_HOVER_HELP_CLOSE;
                        break;
                    default:
                        pos = GENERAL_HOVER_HELP_HERO;
                        break;
                }
                gpCombatManager->CombatMessage(gViewGeneralHelp[pos], 1);
                return MESSAGE_DISPATCH_CONSUME;
                break;
        }
    }
    if (result) {
        message.id = WIDGET_COMMAND_DIALOG_SELECT;
        message.command = message.id;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// The creature quick view, placed beside the stack and clamped to the screen.
VA(0x00466595, 0x135)
void combatManager::ViewArmy(army* viewedArmy, i32 side, i32 quickView) {
    i16 xWnd;
    i16 viewYOffsetConst;
    i16 yWindow;
    i16 xDelta;
    i16 viewXOffsetFixed;
    i16 viewWidthConstant;
    i16 viewHeightConstant;

    if (viewedArmy == NULL)
        return;
    viewWidthConstant = 488 - 86;
    viewHeightConstant = 229;
    viewXOffsetFixed = 86;
    viewYOffsetConst = 164;
    xWnd = m_hexCells[viewedArmy->m_hex].m_x;
    yWindow = m_hexCells[viewedArmy->m_hex].m_y;
    xDelta = (viewedArmy->m_facing == ARMY_FACING_LEFT ? 43 : 0) + 80;
    xWnd -= xDelta;
    if (xWnd < 0)
        xWnd = 0;
    if (xWnd + 488 > LOGICAL_SCREEN_WIDTH)
        xWnd = 151;
    yWindow -= 164;
    if (yWindow < 0)
        yWindow = 0;
    if (yWindow + 229 > COMBAT_VIEW_HEIGHT)
        yWindow = 230;
    gpGame->ViewArmy(
        xWnd,
        yWindow,
        viewedArmy->m_creatureType,
        viewedArmy->m_quantity,
        m_combatTowns[side],
        1,
        viewedArmy->m_facing,
        quickView,
        m_heroes[side],
        viewedArmy,
        m_armyGroups[side]
    );
}
