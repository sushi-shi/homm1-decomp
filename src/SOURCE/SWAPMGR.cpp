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
    i32 i;

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
    SET_ADVENTURE_BUTTON_FLAGS(
        message,
        gpAdvManager->m_adventureWindow,
        WIDGET_COMMAND_CLEAR_FLAGS
    );
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

VA(0x0045d25f, 0x114)
void swapManager::Close(void) {
    tag_message message;

    gpResourceManager->Dispose(m_selectorIcon);
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
    gpAdvManager->Activate();
    SET_ADVENTURE_BUTTON_FLAGS(message, gpAdvManager->m_adventureWindow, WIDGET_COMMAND_SET_FLAGS);
}

VA(0x0045d373, 0x1a2)
void swapManager::DrawSelector(void) {
    const i16 frameColor = 232;
    const i16 leftArmyBase = 24;
    const i16 rightMonsterBaseVal = 252;
    const i16 curTop = 148;
    const i16 armySpacingVal = 35;
    const i16 art1 = 76;
    const i16 curArt2 = 305;
    const i16 newTop = 194;
    const i16 itemGap = 35;
    i16 mainX = 0;
    i16 mainY = 0;

    if (m_selectedSide != SWAP_SIDE_NONE && m_selectedSlot != SWAP_SLOT_NONE) {
        switch (m_selectedSide) {
            case SWAP_SIDE_LEFT:
                switch (m_itemType) {
                    case SWAP_ITEM_ARMY:
                        mainX = m_selectedSlot * armySpacingVal + leftArmyBase - 1;
                        mainY = curTop - 1;
                        break;
                    case SWAP_ITEM_ARTIFACT:
                        mainX = art1
                                + (m_selectedSlot > SWAP_ARTIFACTS_PER_COLUMN - 1 ? itemGap : 0)
                                - 1;
                        mainY = m_selectedSlot % SWAP_ARTIFACTS_PER_COLUMN * itemGap + newTop - 1;
                        break;
                }
                break;
            case SWAP_SIDE_RIGHT:
                switch (m_itemType) {
                    case SWAP_ITEM_ARMY:
                        mainX = m_selectedSlot * armySpacingVal + rightMonsterBaseVal - 1;
                        mainY = curTop - 1;
                        break;
                    case SWAP_ITEM_ARTIFACT:
                        mainX = curArt2
                                + (m_selectedSlot > SWAP_ARTIFACTS_PER_COLUMN - 1 ? itemGap : 0)
                                - 1;
                        mainY = m_selectedSlot % SWAP_ARTIFACTS_PER_COLUMN * itemGap + newTop - 1;
                        break;
                }
                break;
        }
        m_selectorIcon->FillToBuffer(
            mainX + 16,
            mainY + 16,
            2,
            frameColor,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        gpWindowManager->UpdateScreenRegion(mainX + 16, mainY + 16, 36, 36);
    }
}

VA(0x0045d515, 0x88c)
H1_ENUM_RETURN(MessageDispatchResult, i16) swapManager::Main(struct tag_message& message) {
    i8 nowCloseRequested = 0;
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
                        nowCloseRequested = 1;
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
                                    NORMAL_DIALOG_TYPE_OK
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
                                    NORMAL_DIALOG_TYPE_OK
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
                                        || m_heroes[m_targetSide]
                                                   ->m_army.m_creatureTypes[m_targetSlot]
                                               == m_heroes[m_selectedSide]
                                                      ->m_army.m_creatureTypes[m_selectedSlot])) {
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
                                        || m_heroes[m_targetSide]
                                                   ->m_army.m_creatureTypes[m_targetSlot]
                                               == m_heroes[m_selectedSide]
                                                      ->m_army.m_creatureTypes[m_selectedSlot])) {
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
    if (nowCloseRequested == 1) {
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

VA(0x0045e628, 0x352)
void swapManager::SplitMons(void) {
    i16 idPos;
    armyGroup* dstTroopsPtr;
    armyGroup* selectedArmy;
    i16 amountWidgetVal;
    tag_message message;
    i16 found;
    i16 lastI;

    amountWidgetVal = TOWN_SPLIT_AMOUNT_CONTROL;
    found = 0;
    selectedArmy = &m_heroes[m_selectedSide]->m_army;
    dstTroopsPtr = &m_heroes[m_targetSide]->m_army;
    found = 0;
    idPos = 1;
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
        for (lastI = 0; lastI < ARMY_GROUP_SLOT_COUNT; lastI++) {
            if (dstTroopsPtr->m_creatureTypes[lastI]
                == selectedArmy->m_creatureTypes[m_selectedSlot]) {
                found = 1;
                break;
            }
        }
        if (found) {
            dstTroopsPtr->m_creatureCounts[lastI] += gpTownManager->m_splitAmount;
        } else {
            if (dstTroopsPtr->m_creatureTypes[m_targetSlot] != CREATURE_NONE) {
                for (lastI = 0; lastI < ARMY_GROUP_SLOT_COUNT; lastI++) {
                    if (dstTroopsPtr->m_creatureTypes[lastI] == CREATURE_NONE)
                        break;
                }
                if (lastI < ARMY_GROUP_SLOT_COUNT)
                    m_targetSlot = lastI;
            }
            dstTroopsPtr->m_creatureTypes[m_targetSlot] =
                selectedArmy->m_creatureTypes[m_selectedSlot];
            dstTroopsPtr->m_creatureCounts[m_targetSlot] = gpTownManager->m_splitAmount;
        }
        selectedArmy->m_creatureCounts[m_selectedSlot] -= gpTownManager->m_splitAmount;
        if (selectedArmy->m_creatureCounts[m_selectedSlot] == 0)
            selectedArmy->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
    }
}
