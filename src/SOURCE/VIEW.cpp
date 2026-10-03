// HoMM1 VIEW: the combat hero (general) and creature quick views.
// Retail int3 padding bounds this object at 0x00438310-0x00438bf1; Buka 2.1
// SOURCE/VIEW.cpp supplies the family (ViewGeneral, HandleViewGeneral,
// ViewArmy) in the same order.

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

// vgenwin.bin widget ids (Buka 2.1 VIEW.cpp ViewGeneralControl): name,
// portrait, colour and stats boxes, the Cast Spell / Retreat / Surrender
// buttons ViewGeneral disables and HandleViewGeneral returns, and the
// frame widgets the retail block names without using.
H1_ENUM_BEGIN(ViewGeneralControl)
    GENERAL_CONTROL_NONE = 0,
    GENERAL_NAME_WIDGET = 1,
    GENERAL_PORTRAIT_WIDGET = 2,
    GENERAL_COLOR_WIDGET = 3,
    GENERAL_STATS_WIDGET = 4,
    GENERAL_CONTROL_SEVEN = 7,
    GENERAL_CONTROL_EIGHT = 8,
    GENERAL_CONTROL_NINE = 9,
    GENERAL_CAST_SPELL = 10,
    GENERAL_RETREAT = 11,
    GENERAL_SURRENDER = 12,
    GENERAL_CONTROL_THIRTEEN = 13,
    GENERAL_CONTROL_FOURTEEN = 14
H1_ENUM_END(ViewGeneralControl)

// HandleViewGeneral's hover line: the gViewGeneralHelp row (Buka
// ViewGeneralHoverHelp).
H1_ENUM_BEGIN(ViewGeneralHoverHelp)
    GENERAL_HOVER_HELP_CAST_SPELL = 1,
    GENERAL_HOVER_HELP_RETREAT = 2,
    GENERAL_HOVER_HELP_SURRENDER = 3,
    GENERAL_HOVER_HELP_CLOSE = 4,
    GENERAL_HOVER_HELP_HERO = 5
H1_ENUM_END(ViewGeneralHoverHelp)

// Buka VIEW.cpp:101-260 without the captain and spell-point lines: the
// combat hero window, with Cast Spell, Retreat and Surrender dimmed when
// the side cannot use them.
VA(0x00438310, 0x56d)
i8 combatManager::ViewGeneral(i32 side, i32 allowActions, i32 quickView) {
    i16 pictureCtrl;
    i16 borderId;
    i32 morale;
    i32 iLuck;
    i16 barId;
    i16 castSpellControl;
    tag_message message;
    i16 surrenderBtn;
    i16 colorControl;
    heroWindow* wnd;
    i16 nameCtrl;
    i16 frameWidgetId;
    i16 statBoxId;
    i16 cornerCtrl;
    i32 spare;
    i16 captionCtrl;
    i16 edgeCtrl;
    i16 retreatId;
    i16 baseCtrl;

    if (m_heroes[side] == NULL)
        return 0;
    // vgenwin.bin widget ids: retail stores the whole block (as Buka does)
    // though nothing reads it; their slots and the unused spare fix the frame.
    nameCtrl = GENERAL_NAME_WIDGET;
    pictureCtrl = GENERAL_PORTRAIT_WIDGET;
    colorControl = GENERAL_COLOR_WIDGET;
    statBoxId = GENERAL_STATS_WIDGET;
    borderId = GENERAL_CONTROL_NONE;
    captionCtrl = GENERAL_NAME_WIDGET;
    cornerCtrl = GENERAL_CONTROL_SEVEN;
    barId = GENERAL_CONTROL_EIGHT;
    edgeCtrl = GENERAL_CONTROL_NINE;
    castSpellControl = GENERAL_CAST_SPELL;
    retreatId = GENERAL_RETREAT;
    surrenderBtn = GENERAL_SURRENDER;
    baseCtrl = GENERAL_CONTROL_THIRTEEN;
    frameWidgetId = GENERAL_CONTROL_FOURTEEN;
    giCurGeneral = side;
    message.type = MESSAGE_WIDGET;
    wnd = new heroWindow(195, 60, "vgenwin.bin");
    if (wnd == NULL)
        MemError();
    sprintf(gText, "port%04d.icn", m_heroes[side]->m_portrait);
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = GENERAL_PORTRAIT_WIDGET;
    message.text = gText;
    wnd->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = GENERAL_COLOR_WIDGET;
    message.value = gpGame->m_players[m_heroes[side]->m_owner].Color() + 1;
    wnd->BroadcastMessage(message);
    sprintf(gText, "%s the %s", m_heroes[side]->m_name, gClassNames[m_heroes[side]->m_heroClass]);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = GENERAL_NAME_WIDGET;
    message.text = gText;
    wnd->BroadcastMessage(message);
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
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = GENERAL_STATS_WIDGET;
    message.text = gText;
    wnd->BroadcastMessage(message);
    if (m_heroes[side] == NULL || allowActions == 0
        || !m_heroes[side]->HasArtifact(ARTIFACT_MAGIC_BOOK) || m_heroCastSpell[side] != 0
        || m_currentSide != giCurGeneral) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = GENERAL_CAST_SPELL;
        message.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(message);
    }
    if (allowActions == 0 || m_heroes[1 - m_currentSide] == NULL || m_currentSide != giCurGeneral) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = GENERAL_SURRENDER;
        message.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(message);
    }
    if (allowActions == 0 || m_currentSide != giCurGeneral
        || (giCurGeneral == COMBAT_DEFENDER_SIDE && m_combatTowns[COMBAT_DEFENDER_SIDE] != NULL)
        || m_sideRetreated[COMBAT_DEFENDER_SIDE] != 0
        || m_sideRetreated[COMBAT_ATTACKER_SIDE] != 0) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = GENERAL_RETREAT;
        message.value = WIDGET_FLAG_ENABLED;
        wnd->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_DIMMED;
        wnd->BroadcastMessage(message);
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

// Buka VIEW.cpp:290-390 without the right-click help: Cast Spell, Retreat,
// Surrender and Close end the dialog; hovering shows their help line.
VA(0x0043887d, 0x222)
i16 HandleViewGeneral(tag_message& message) {
    i32 hintIndex;
    i16 pictureCtrl;
    i16 borderId;
    i16 barId;
    i16 castSpellControl;
    i16 surrenderBtn;
    i8 retVal;
    i16 colorControl;
    i16 nameCtrl;
    i16 frameWidgetId;
    i16 statBoxId;
    i16 cornerCtrl;
    i16 captionCtrl;
    i16 edgeCtrl;
    i16 retreatId;
    i16 baseCtrl;

    nameCtrl = GENERAL_NAME_WIDGET;
    pictureCtrl = GENERAL_PORTRAIT_WIDGET;
    colorControl = GENERAL_COLOR_WIDGET;
    statBoxId = GENERAL_STATS_WIDGET;
    borderId = GENERAL_CONTROL_NONE;
    captionCtrl = GENERAL_NAME_WIDGET;
    cornerCtrl = GENERAL_CONTROL_SEVEN;
    barId = GENERAL_CONTROL_EIGHT;
    edgeCtrl = GENERAL_CONTROL_NINE;
    castSpellControl = GENERAL_CAST_SPELL;
    retreatId = GENERAL_RETREAT;
    surrenderBtn = GENERAL_SURRENDER;
    baseCtrl = GENERAL_CONTROL_THIRTEEN;
    frameWidgetId = GENERAL_CONTROL_FOURTEEN;
    retVal = 0;
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
                            retVal = 1;
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
                        hintIndex = GENERAL_HOVER_HELP_CAST_SPELL;
                        break;
                    case GENERAL_RETREAT:
                        hintIndex = GENERAL_HOVER_HELP_RETREAT;
                        break;
                    case GENERAL_SURRENDER:
                        hintIndex = GENERAL_HOVER_HELP_SURRENDER;
                        break;
                    case DIALOG_BUTTON_0:
                        hintIndex = GENERAL_HOVER_HELP_CLOSE;
                        break;
                    default:
                        hintIndex = GENERAL_HOVER_HELP_HERO;
                        break;
                }
                gpCombatManager->CombatMessage(gViewGeneralHelp[hintIndex], 1);
                return MESSAGE_DISPATCH_CONSUME;
                break;
        }
    }
    if (retVal) {
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka VIEW.cpp:442-488: the creature quick view, placed beside the stack
// and clamped to the screen.
VA(0x00438a9f, 0x152)
void combatManager::ViewArmy(army* viewedArmy, i32 side, i32 quickView) {
    i16 xPos;
    i16 yPos;
    i16 wndWidth;
    i16 viewXOffset;
    i16 xAdjust;
    i16 viewYOffset;
    i16 height;

    if (viewedArmy == NULL)
        return;
    wndWidth = 488 - 86;
    height = 229;
    viewXOffset = 86;
    viewYOffset = 164;
    xPos = m_hexCells[viewedArmy->m_hex].m_x;
    yPos = m_hexCells[viewedArmy->m_hex].m_y;
    xAdjust = (viewedArmy->m_facing == ARMY_FACING_LEFT ? 43 : 0) + 80;
    xPos -= xAdjust;
    if (xPos < 0)
        xPos = 0;
    if (xPos + 488 > LOGICAL_SCREEN_WIDTH)
        xPos = 151;
    yPos -= 164;
    if (yPos < 0)
        yPos = 0;
    if (yPos + 229 > COMBAT_VIEW_HEIGHT)
        yPos = 230;
    gpGame->ViewArmy(
        xPos,
        yPos,
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
