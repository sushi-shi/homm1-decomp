// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/swapManager.h>
#include <SOURCE/townManager.h>

#include <stdio.h>
#include <string.h>

// swapwin.bin widget ids (Buka 2.1 SWAPMGR.cpp SwapManagerControl; HoMM1 has
// no secondary skills). LEFT is the constructor's first hero, m_heroes[SWAP_SIDE_LEFT].
H1_ENUM_BEGIN(SwapManagerControl)
    CONTROL_LEFT_HERO = 65,
    CONTROL_RIGHT_HERO = 66,
    CONTROL_LEFT_PRIMARY_SKILL_FIRST = 67,
    CONTROL_RIGHT_PRIMARY_SKILL_FIRST = 72,
    CONTROL_TITLE = 77,
    CONTROL_LEFT_ARMY_FIRST = 78,
    CONTROL_LEFT_ARMY_LAST = 82,
    CONTROL_RIGHT_ARMY_FIRST = 83,
    CONTROL_RIGHT_ARMY_LAST = 87,
    CONTROL_LEFT_ARTIFACT_FIRST = 88,
    CONTROL_LEFT_ARTIFACT_LAST = 101,
    CONTROL_RIGHT_ARTIFACT_FIRST = 102,
    CONTROL_RIGHT_ARTIFACT_LAST = 115,
    CONTROL_LEFT_ARMY_COUNT_FIRST = 116,
    CONTROL_RIGHT_ARMY_COUNT_FIRST = 121
H1_ENUM_END(SwapManagerControl)

// m_selectedSide/m_targetSide: the m_heroes index. DrawSelector draws side 1
// at the left army/artifact columns, so HoMM1's left hero is index 1 (Buka
// numbers its sides the other way round).
H1_ENUM_BEGIN(SwapManagerSide)
    SWAP_SIDE_NONE = -1,
    SWAP_SIDE_RIGHT = 0,
    SWAP_SIDE_LEFT = 1
H1_ENUM_END(SwapManagerSide)

// m_itemType: what the selection holds (Buka SwapManagerItemType).
H1_ENUM_BEGIN(SwapManagerItemType)
    SWAP_ITEM_NONE = -1,
    SWAP_ITEM_ARMY = 0,
    SWAP_ITEM_ARTIFACT = 1
H1_ENUM_END(SwapManagerItemType)

// m_selectedSlot/m_targetSlot with nothing picked; DrawSelector lays a
// hero's fourteen artifacts out in two columns of seven.
H1_ENUM_CONST_BEGIN(SwapManagerConstant)
    SWAP_SLOT_NONE = -1,
    SWAP_ARTIFACTS_PER_COLUMN = 7
H1_ENUM_CONST_END(SwapManagerConstant)

// Buka 2.1 swapManager::swapManager(void).
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0045cee0, 0x6a)
swapManager::swapManager(void) {
    m_window = NULL;
    m_selectorIcon = NULL;
    m_selectedSide = SWAP_SIDE_NONE;
    m_targetSide = SWAP_SIDE_NONE;
    m_itemType = SWAP_ITEM_NONE;
    m_selectedSlot = SWAP_SLOT_NONE;
    m_targetSlot = SWAP_SLOT_NONE;
    m_heroes[SWAP_SIDE_LEFT] = NULL;
    m_heroes[SWAP_SIDE_RIGHT] = NULL;
}

VA(0x0045cf4a, 0x33)
swapManager::swapManager(class hero* leftHero, class hero* rightHero) {
    m_heroes[SWAP_SIDE_LEFT] = leftHero;
    m_heroes[SWAP_SIDE_RIGHT] = rightHero;
}

VA(0x0045cf7d, 0x2e)
void swapManager::Reset(void) {
    m_selectedSide = m_targetSide = m_itemType = m_selectedSlot = m_targetSlot = SWAP_SLOT_NONE;
}

VA(0x0045cfab, 0x2b4)
i16 swapManager::Open(i16 id) {
    tag_message message;
    i32 i; // Unused; retail still reserves its frame slot.

    Reset();
    m_window = new heroWindow(16, 16, "swapwin.bin");
    if (!m_window)
        MemError();
    SetWinText(m_window, WINDOW_TEXT_SWAP);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_ICON;
    sprintf(gText, "port%04d.icn", m_heroes[SWAP_SIDE_LEFT]->m_portrait);
    message.id = CONTROL_LEFT_HERO;
    message.text = gText;
    m_window->BroadcastMessage(message);
    sprintf(gText, "port%04d.icn", m_heroes[SWAP_SIDE_RIGHT]->m_portrait);
    message.id = CONTROL_RIGHT_HERO;
    m_window->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    sprintf(
        gText,
        localization::Tr("hero.meeting.title"),
        m_heroes[SWAP_SIDE_LEFT]->m_name,
        m_heroes[SWAP_SIDE_RIGHT]->m_name
    );
    message.text = gText;
    message.id = CONTROL_TITLE;
    m_window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED;
    message.id = ADVENTURE_CONTROL_NEXT_HERO;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_CONTINUE_ROUTE;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_OVERVIEW;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_END_TURN;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_ADVENTURE_OPTIONS;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_GAME_OPTIONS;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    Update();
    gpWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    KBChangeMenu(hmnuAdv);
    gMonoIconSkip = 2;
    m_selectorIcon = gpResourceManager->GetIcon("swapbtn.icn");
    gMonoIconSkip = -1;
    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    m_messageFilter = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                      | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                      | MESSAGE_WIDGET;
    m_messageMask = BASE_MANAGER_ACCEPT_SWAP;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "swapManager");
    return BASE_MANAGER_SUCCESS;
}

// donor PoL RVA 0x000548be; preferred Buka symbol ?Close@swapManager@@UAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.610218;margin=0.602384;shape=0.500;size=0.986;calls=1.000;alternate=pol20:void swapManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x000548be
VA(0x0045d25f, 0x114)
void swapManager::Close(void) {
    tag_message message;

    gpResourceManager->Dispose(m_selectorIcon);
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
    gpAdvManager->Activate();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.value = WIDGET_FLAG_ENABLED;
    message.id = ADVENTURE_CONTROL_NEXT_HERO;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_CONTINUE_ROUTE;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_OVERVIEW;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_END_TURN;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_ADVENTURE_OPTIONS;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = ADVENTURE_CONTROL_GAME_OPTIONS;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
}

VA(0x0045d373, 0x1a2)
void swapManager::DrawSelector(void) {
    const i16 frameColor = 232;
    const i16 leftArmyBase = 24;
    const i16 rightMonsterBase = 252;
    const i16 troopTop = 148;
    const i16 armySpacing = 35;
    const i16 art1 = 76;
    const i16 art2 = 305;
    const i16 artTop = 194;
    const i16 itemGap = 35;
    i16 x = 0;
    i16 y = 0;

    if (m_selectedSide != SWAP_SIDE_NONE && m_selectedSlot != SWAP_SLOT_NONE) {
        switch (m_selectedSide) {
            case SWAP_SIDE_LEFT:
                switch (m_itemType) {
                    case SWAP_ITEM_ARMY:
                        x = m_selectedSlot * armySpacing + leftArmyBase - 1;
                        y = troopTop - 1;
                        break;
                    case SWAP_ITEM_ARTIFACT:
                        x = art1 + (m_selectedSlot > SWAP_ARTIFACTS_PER_COLUMN - 1 ? itemGap : 0)
                            - 1;
                        y = m_selectedSlot % SWAP_ARTIFACTS_PER_COLUMN * itemGap + artTop - 1;
                        break;
                }
                break;
            case SWAP_SIDE_RIGHT:
                switch (m_itemType) {
                    case SWAP_ITEM_ARMY:
                        x = m_selectedSlot * armySpacing + rightMonsterBase - 1;
                        y = troopTop - 1;
                        break;
                    case SWAP_ITEM_ARTIFACT:
                        x = art2 + (m_selectedSlot > SWAP_ARTIFACTS_PER_COLUMN - 1 ? itemGap : 0)
                            - 1;
                        y = m_selectedSlot % SWAP_ARTIFACTS_PER_COLUMN * itemGap + artTop - 1;
                        break;
                }
                break;
        }
        m_selectorIcon
            ->FillToBuffer(x + 16, y + 16, 2, frameColor, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
        gpWindowManager->UpdateScreenRegion(x + 16, y + 16, 36, 36);
    }
}

// donor PoL RVA 0x00054be3; preferred Buka symbol ?Main@swapManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.525982;margin=0.522986;shape=0.320;size=0.991;calls=0.960;alternate=pol20:int swapManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00054be3
VA(0x0045d515, 0x88c)
i16 swapManager::Main(struct tag_message& message) {
    i8 closeRequested = 0;
    i8 quickView;
    i32 artIndex;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickView = 1;
    else
        quickView = 0;
    if (!(message.type & m_messageFilter)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    switch (message.type) {
        case MESSAGE_RIGHT_BUTTON_DOWN:
            if (quickView)
                break;
            Reset();
            Update();
            m_window->DrawWindow();
            break;
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    if (quickView)
                        break;
                    if (message.id == DIALOG_BUTTON_0)
                        closeRequested = 1;
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case CONTROL_LEFT_HERO:
                            if (quickView)
                                break;
                            m_heroes[SWAP_SIDE_LEFT]->HeroView(1);
                            gpAdvManager->RedrawAdvScreen(1);
                            Update();
                            m_window->DrawWindow();
                            Reset();
                            gpWindowManager
                                ->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
                            break;
                        case CONTROL_RIGHT_HERO:
                            if (quickView)
                                break;
                            m_heroes[SWAP_SIDE_RIGHT]->HeroView(1);
                            gpAdvManager->RedrawAdvScreen(1);
                            Update();
                            m_window->DrawWindow();
                            Reset();
                            gpWindowManager
                                ->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
                            break;
                        case CONTROL_LEFT_ARTIFACT_FIRST:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 1:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 2:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 3:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 4:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 5:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 6:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 7:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 8:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 9:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 10:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 11:
                        case CONTROL_LEFT_ARTIFACT_FIRST + 12:
                        case CONTROL_LEFT_ARTIFACT_LAST:
                            artIndex = message.id - CONTROL_LEFT_ARTIFACT_FIRST;
                            if (!quickView
                                && (m_heroes[SWAP_SIDE_LEFT]->m_artifacts[artIndex]
                                        == ARTIFACT_MAGIC_BOOK
                                    || m_heroes[SWAP_SIDE_LEFT]->m_artifacts[artIndex]
                                           == ARTIFACT_FIZBIN_OF_MISFORTUNE)) {
                                NormalDialog(
                                    localization::Tr("artifact.trade.forbidden"),
                                    NORMAL_DIALOG_TYPE_OK,
                                    -1,
                                    -1,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_OR_TEXT
                                );
                                break;
                            }
                            if (quickView) {
                                if (m_heroes[SWAP_SIDE_LEFT]->m_artifacts[artIndex]
                                    == ARTIFACT_NONE)
                                    break;
                                m_heroes[SWAP_SIDE_LEFT]->ViewArtifact(
                                    m_heroes[SWAP_SIDE_LEFT]->m_artifacts[artIndex],
                                    1
                                );
                                break;
                            }
                            if (m_itemType != SWAP_ITEM_ARTIFACT) {
                                if (m_heroes[SWAP_SIDE_LEFT]->m_artifacts[artIndex]
                                    != ARTIFACT_NONE) {
                                    m_selectedSide = SWAP_SIDE_LEFT;
                                    m_targetSide = SWAP_SIDE_NONE;
                                    m_itemType = SWAP_ITEM_ARTIFACT;
                                    m_selectedSlot = artIndex;
                                    m_targetSlot = SWAP_SLOT_NONE;
                                } else {
                                    Reset();
                                }
                            } else {
                                m_targetSide = SWAP_SIDE_LEFT;
                                m_targetSlot = artIndex;
                                if (m_selectedSide == SWAP_SIDE_LEFT
                                    && m_selectedSlot == m_targetSlot) {
                                    m_heroes[SWAP_SIDE_LEFT]->ViewArtifact(
                                        m_heroes[SWAP_SIDE_LEFT]->m_artifacts[artIndex],
                                        0
                                    );
                                    Reset();
                                }
                                SwapArtifacts();
                                Reset();
                            }
                            break;
                        case CONTROL_RIGHT_ARTIFACT_FIRST:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 1:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 2:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 3:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 4:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 5:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 6:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 7:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 8:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 9:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 10:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 11:
                        case CONTROL_RIGHT_ARTIFACT_FIRST + 12:
                        case CONTROL_RIGHT_ARTIFACT_LAST:
                            artIndex = message.id - CONTROL_RIGHT_ARTIFACT_FIRST;
                            if (!quickView
                                && (m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[artIndex]
                                        == ARTIFACT_MAGIC_BOOK
                                    || m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[artIndex]
                                           == ARTIFACT_FIZBIN_OF_MISFORTUNE)) {
                                NormalDialog(
                                    localization::Tr("artifact.trade.forbidden"),
                                    NORMAL_DIALOG_TYPE_OK,
                                    -1,
                                    -1,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_OR_TEXT
                                );
                                break;
                            }
                            if (quickView) {
                                if (m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[artIndex]
                                    == ARTIFACT_NONE)
                                    break;
                                m_heroes[SWAP_SIDE_RIGHT]->ViewArtifact(
                                    m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[artIndex],
                                    1
                                );
                                break;
                            }
                            if (m_itemType != SWAP_ITEM_ARTIFACT) {
                                if (m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[artIndex]
                                    != ARTIFACT_NONE) {
                                    m_selectedSide = SWAP_SIDE_RIGHT;
                                    m_targetSide = SWAP_SIDE_NONE;
                                    m_itemType = SWAP_ITEM_ARTIFACT;
                                    m_selectedSlot = artIndex;
                                    m_targetSlot = SWAP_SLOT_NONE;
                                } else {
                                    Reset();
                                }
                            } else {
                                m_targetSide = SWAP_SIDE_RIGHT;
                                m_targetSlot = artIndex;
                                if (m_selectedSide == SWAP_SIDE_RIGHT
                                    && m_selectedSlot == m_targetSlot) {
                                    m_heroes[SWAP_SIDE_RIGHT]->ViewArtifact(
                                        m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[artIndex],
                                        0
                                    );
                                    Reset();
                                }
                                SwapArtifacts();
                                Reset();
                            }
                            break;
                        case CONTROL_LEFT_ARMY_FIRST:
                        case CONTROL_LEFT_ARMY_FIRST + 1:
                        case CONTROL_LEFT_ARMY_FIRST + 2:
                        case CONTROL_LEFT_ARMY_FIRST + 3:
                        case CONTROL_LEFT_ARMY_LAST:
                            if (quickView) {
                                if (m_heroes[SWAP_SIDE_LEFT]
                                        ->m_army
                                        .m_creatureTypes[message.id - CONTROL_LEFT_ARMY_FIRST]
                                    != CREATURE_NONE)
                                    gpGame->ViewArmy(
                                        TOWN_ARMY_VIEW_X,
                                        TOWN_ARMY_VIEW_Y,
                                        m_heroes[SWAP_SIDE_LEFT]
                                            ->m_army
                                            .m_creatureTypes[message.id - CONTROL_LEFT_ARMY_FIRST],
                                        m_heroes[SWAP_SIDE_LEFT]
                                            ->m_army
                                            .m_creatureCounts[message.id - CONTROL_LEFT_ARMY_FIRST],
                                        NULL,
                                        0,
                                        0,
                                        1,
                                        m_heroes[SWAP_SIDE_LEFT],
                                        NULL,
                                        &m_heroes[SWAP_SIDE_LEFT]->m_army
                                    );
                                break;
                            }
                            if (m_itemType) {
                                if (m_heroes[SWAP_SIDE_LEFT]
                                        ->m_army
                                        .m_creatureTypes[message.id - CONTROL_LEFT_ARMY_FIRST]
                                    != CREATURE_NONE) {
                                    m_selectedSide = SWAP_SIDE_LEFT;
                                    m_targetSide = SWAP_SIDE_NONE;
                                    m_itemType = SWAP_ITEM_ARMY;
                                    m_selectedSlot = message.id - CONTROL_LEFT_ARMY_FIRST;
                                    m_targetSlot = SWAP_SLOT_NONE;
                                } else {
                                    Reset();
                                }
                            } else {
                                m_targetSide = SWAP_SIDE_LEFT;
                                m_targetSlot = message.id - CONTROL_LEFT_ARMY_FIRST;
                                if (m_selectedSide == SWAP_SIDE_LEFT
                                    && m_selectedSlot == m_targetSlot) {
                                    ViewMon();
                                    Reset();
                                }
                                if ((message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS)
                                    && m_selectedSide != m_targetSide
                                    && (m_heroes[m_targetSide]->m_army.m_creatureTypes[m_targetSlot]
                                            == CREATURE_NONE
                                        || m_heroes[m_selectedSide]
                                                   ->m_army.m_creatureTypes[m_selectedSlot]
                                               == m_heroes[m_targetSide]
                                                      ->m_army.m_creatureTypes[m_targetSlot])) {
                                    SplitMons();
                                    Reset();
                                } else {
                                    SwapMons();
                                    Reset();
                                }
                            }
                            break;
                        case CONTROL_RIGHT_ARMY_FIRST:
                        case CONTROL_RIGHT_ARMY_FIRST + 1:
                        case CONTROL_RIGHT_ARMY_FIRST + 2:
                        case CONTROL_RIGHT_ARMY_FIRST + 3:
                        case CONTROL_RIGHT_ARMY_LAST:
                            if (quickView) {
                                if (m_heroes[SWAP_SIDE_RIGHT]
                                        ->m_army
                                        .m_creatureTypes[message.id - CONTROL_RIGHT_ARMY_FIRST]
                                    != CREATURE_NONE)
                                    gpGame->ViewArmy(
                                        TOWN_ARMY_VIEW_X,
                                        TOWN_ARMY_VIEW_Y,
                                        m_heroes[SWAP_SIDE_RIGHT]
                                            ->m_army
                                            .m_creatureTypes[message.id - CONTROL_RIGHT_ARMY_FIRST],
                                        m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureCounts
                                            [message.id - CONTROL_RIGHT_ARMY_FIRST],
                                        NULL,
                                        0,
                                        0,
                                        1,
                                        m_heroes[SWAP_SIDE_RIGHT],
                                        NULL,
                                        &m_heroes[SWAP_SIDE_RIGHT]->m_army
                                    );
                                break;
                            }
                            if (m_itemType) {
                                if (m_heroes[SWAP_SIDE_RIGHT]
                                        ->m_army
                                        .m_creatureTypes[message.id - CONTROL_RIGHT_ARMY_FIRST]
                                    != CREATURE_NONE) {
                                    m_selectedSide = SWAP_SIDE_RIGHT;
                                    m_targetSide = SWAP_SIDE_NONE;
                                    m_itemType = SWAP_ITEM_ARMY;
                                    m_selectedSlot = message.id - CONTROL_RIGHT_ARMY_FIRST;
                                    m_targetSlot = SWAP_SLOT_NONE;
                                } else {
                                    Reset();
                                }
                            } else {
                                m_targetSide = SWAP_SIDE_RIGHT;
                                m_targetSlot = message.id - CONTROL_RIGHT_ARMY_FIRST;
                                if (m_selectedSide == SWAP_SIDE_RIGHT
                                    && m_selectedSlot == m_targetSlot) {
                                    ViewMon();
                                    Reset();
                                }
                                if ((message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS)
                                    && m_selectedSide != m_targetSide
                                    && (m_heroes[m_targetSide]->m_army.m_creatureTypes[m_targetSlot]
                                            == CREATURE_NONE
                                        || m_heroes[m_selectedSide]
                                                   ->m_army.m_creatureTypes[m_selectedSlot]
                                               == m_heroes[m_targetSide]
                                                      ->m_army.m_creatureTypes[m_targetSlot])) {
                                    SplitMons();
                                    Reset();
                                } else {
                                    SwapMons();
                                    Reset();
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    if (!quickView) {
                        Update();
                        m_window->DrawWindow();
                        DrawSelector();
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if (closeRequested == 1) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0045dda1, 0x9a)
void swapManager::ViewMon(void) {
    gpGame->ViewArmy(
        TOWN_ARMY_VIEW_X,
        TOWN_ARMY_VIEW_Y,
        m_heroes[m_selectedSide]->m_army.m_creatureTypes[m_targetSlot],
        m_heroes[m_selectedSide]->m_army.m_creatureCounts[m_targetSlot],
        NULL,
        m_heroes[m_selectedSide]->m_army.GetNumArmies() == 1,
        0,
        0,
        m_heroes[m_selectedSide],
        NULL,
        &m_heroes[m_selectedSide]->m_army
    );
}

// Buka 2.1 swapManager::SwapArtifacts.
VA(0x0045de3b, 0x126)
void swapManager::SwapArtifacts(void) {
    i8 targetArtifact;
    i8 selectedArtifact;

    if (m_selectedSide == SWAP_SIDE_NONE && m_targetSide == SWAP_SIDE_NONE)
        return;

    selectedArtifact = m_heroes[m_selectedSide]->m_artifacts[m_selectedSlot];
    targetArtifact = m_heroes[m_targetSide]->m_artifacts[m_targetSlot];
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_selectedSide], selectedArtifact, 1);
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_targetSide], targetArtifact, 1);
    m_heroes[m_selectedSide]->m_artifacts[m_selectedSlot] = targetArtifact;
    m_heroes[m_targetSide]->m_artifacts[m_targetSlot] = selectedArtifact;
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_selectedSide], targetArtifact, 0);
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_targetSide], selectedArtifact, 0);
}

VA(0x0045df61, 0x259)
void swapManager::SwapMons(void) {
    armyGroup* destTroops;
    armyGroup* sourceTroops;
    i16 i;
    i16 j;

    sourceTroops = &m_heroes[m_selectedSide]->m_army;
    destTroops = &m_heroes[m_targetSide]->m_army;
    if (sourceTroops != destTroops) {
        if (sourceTroops->GetNumArmies() == 1
            && (destTroops->m_creatureTypes[m_targetSlot] == CREATURE_NONE
                || destTroops->IsMember(sourceTroops->m_creatureTypes[m_selectedSlot])))
            return;
        if (destTroops->IsMember(sourceTroops->m_creatureTypes[m_selectedSlot])) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (destTroops->m_creatureTypes[i] == sourceTroops->m_creatureTypes[m_selectedSlot])
                    break;
            }
            destTroops->m_creatureCounts[i] += sourceTroops->m_creatureCounts[m_selectedSlot];
            sourceTroops->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
            sourceTroops->m_creatureCounts[m_selectedSlot] = 0;
            return;
        } else if (sourceTroops->IsMember(destTroops->m_creatureTypes[m_targetSlot])) {
            for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                if (sourceTroops->m_creatureTypes[j] == destTroops->m_creatureTypes[m_targetSlot])
                    break;
            }
            sourceTroops->m_creatureCounts[j] += destTroops->m_creatureCounts[m_targetSlot];
            destTroops->m_creatureTypes[m_targetSlot] = CREATURE_NONE;
            destTroops->m_creatureCounts[m_targetSlot] = 0;
            if (j != m_selectedSlot) {
                destTroops->m_creatureTypes[m_targetSlot] =
                    sourceTroops->m_creatureTypes[m_selectedSlot];
                destTroops->m_creatureCounts[m_targetSlot] =
                    sourceTroops->m_creatureCounts[m_selectedSlot];
                sourceTroops->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
                sourceTroops->m_creatureCounts[m_selectedSlot] = 0;
            }
            return;
        }
    }
    sourceTroops->Swap(m_selectedSlot, destTroops, m_targetSlot);
}

VA(0x0045e1ba, 0x46e)
void swapManager::Update(void) {
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.text = gText;
    for (i = 0; i < HERO_PRIMARY_STAT_COUNT; i++) {
        message.id = i + CONTROL_LEFT_PRIMARY_SKILL_FIRST;
        sprintf(gText, "%d", m_heroes[SWAP_SIDE_LEFT]->m_primaryStats[i]);
        m_window->BroadcastMessage(message);
        message.id = i + CONTROL_RIGHT_PRIMARY_SKILL_FIRST;
        sprintf(gText, "%d", m_heroes[SWAP_SIDE_RIGHT]->m_primaryStats[i]);
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + CONTROL_LEFT_ARMY_FIRST;
        if (m_heroes[SWAP_SIDE_LEFT]->m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[SWAP_SIDE_LEFT]->m_army.m_creatureTypes[i];
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + CONTROL_LEFT_ARMY_COUNT_FIRST;
        if (m_heroes[SWAP_SIDE_LEFT]->m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            sprintf(gText, "%d", m_heroes[SWAP_SIDE_LEFT]->m_army.m_creatureCounts[i]);
            message.text = gText;
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + CONTROL_RIGHT_ARMY_FIRST;
        if (m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureTypes[i];
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + CONTROL_RIGHT_ARMY_COUNT_FIRST;
        if (m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            sprintf(gText, "%d", m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureCounts[i]);
            message.text = gText;
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + CONTROL_LEFT_ARTIFACT_FIRST;
        if (m_heroes[SWAP_SIDE_LEFT]->m_artifacts[i] == ARTIFACT_NONE) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[SWAP_SIDE_LEFT]->m_artifacts[i];
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + CONTROL_RIGHT_ARTIFACT_FIRST;
        if (m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[i] == ARTIFACT_NONE) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[i];
        }
        m_window->BroadcastMessage(message);
    }
}

// donor PoL RVA 0x00055fbd; preferred Buka symbol ?SplitMons@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.761691;margin=0.047739;shape=0.487;size=0.960;calls=1.000;strings=splitwin.bin;alternate=pol20:void swapManager::SplitMons(void)@0x00055fbd
VA(0x0045e628, 0x352)
void swapManager::SplitMons(void) {
    i16 textId;
    armyGroup* dstTroops;
    armyGroup* selectedArmy;
    i16 amountWidget;
    tag_message message;
    i16 found;
    i16 i;

    amountWidget = TOWN_SPLIT_AMOUNT_CONTROL;
    found = 0;
    selectedArmy = &m_heroes[m_selectedSide]->m_army;
    dstTroops = &m_heroes[m_targetSide]->m_army;
    found = 0;
    textId = 1;
    gpTownManager->m_heroWindow1 =
        new heroWindow(TOWN_SPLIT_WINDOW_X, TOWN_SPLIT_WINDOW_Y, "splitwin.bin");
    if (!gpTownManager->m_heroWindow1)
        MemError();
    gpTownManager->m_splitAmount = 0;
    gpTownManager->m_splitMaximum = selectedArmy->m_creatureCounts[m_selectedSlot];
    message.type = MESSAGE_WIDGET;
    sprintf(
        gText,
        localization::Tr("army.transfer.prompt"),
        gArmyNames[selectedArmy->m_creatureTypes[m_selectedSlot]],
        m_heroes[m_selectedSide]->m_name,
        m_heroes[m_targetSide]->m_name
    );
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = TOWN_SPLIT_PROMPT_CONTROL;
    message.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "%d", gpTownManager->m_splitAmount);
    message.id = TOWN_SPLIT_AMOUNT_CONTROL;
    message.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    gpWindowManager->DoDialog(gpTownManager->m_heroWindow1, SplitArmyHandler, 0);
    delete gpTownManager->m_heroWindow1;
    if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (dstTroops->m_creatureTypes[i] == selectedArmy->m_creatureTypes[m_selectedSlot]) {
                found = 1;
                break;
            }
        }
        if (found) {
            dstTroops->m_creatureCounts[i] += gpTownManager->m_splitAmount;
        } else {
            if (dstTroops->m_creatureTypes[m_targetSlot] != CREATURE_NONE) {
                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                    if (dstTroops->m_creatureTypes[i] == CREATURE_NONE)
                        break;
                }
                if (i < ARMY_GROUP_SLOT_COUNT)
                    m_targetSlot = i;
            }
            dstTroops->m_creatureTypes[m_targetSlot] =
                selectedArmy->m_creatureTypes[m_selectedSlot];
            dstTroops->m_creatureCounts[m_targetSlot] = gpTownManager->m_splitAmount;
        }
        selectedArmy->m_creatureCounts[m_selectedSlot] -= gpTownManager->m_splitAmount;
        if (selectedArmy->m_creatureCounts[m_selectedSlot] == 0)
            selectedArmy->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
    }
}
