#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
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
    m_selectedSide = m_targetSide = H1_ENUM_DECODE(
        SwapManagerSide,
        H1_ENUM_ENCODE(
            SwapManagerItemType,
            m_itemType =
                H1_ENUM_DECODE(SwapManagerItemType, m_selectedSlot = m_targetSlot = SWAP_SLOT_NONE)
        )
    );
}

VA(0x0045cfab, 0x2b4)
H1_ENUM_RETURN(BaseManagerStatus, i16) swapManager::Open(i16 id) {
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
    SET_ADVENTURE_BUTTON_FLAGS(message, gAdvManager->m_adventureWindow, WIDGET_COMMAND_CLEAR_FLAGS);
    Update();
    gWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    KBChangeMenu(gAdventureMenu);
    gMonoIconSkip = 2;
    m_selectorIcon = gResourceManager->GetIcon("swapbtn.icn");
    gMonoIconSkip = -1;
    gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
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

    gResourceManager->Dispose(m_selectorIcon);
    gWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
    gAdvManager->Activate();
    SET_ADVENTURE_BUTTON_FLAGS(message, gAdvManager->m_adventureWindow, WIDGET_COMMAND_SET_FLAGS);
}

VA(0x0045d373, 0x1a2)
void swapManager::DrawSelector(void) {
    const i16 frameColor = PALETTE_SELECTION_COLOR;
    const i16 leftArmyLeft = 24;
    const i16 rightArmyLeft = 252;
    const i16 armyRow = 148;
    const i16 armyStride = 35;
    const i16 leftArtifactCol = 76;
    const i16 rightArtifactCol = 305;
    const i16 artifactY0 = 194;
    const i16 artifactsSpacing = 35;
    i16 selectorX0 = 0;
    i16 selectorY0 = 0;

    if (m_selectedSide != SWAP_SIDE_NONE && m_selectedSlot != SWAP_SLOT_NONE) {
        switch (m_selectedSide) {
            case SWAP_SIDE_LEFT:
                switch (m_itemType) {
                    case SWAP_ITEM_ARMY:
                        selectorX0 = m_selectedSlot * armyStride + leftArmyLeft - 1;
                        selectorY0 = armyRow - 1;
                        break;
                    case SWAP_ITEM_ARTIFACT:
                        selectorX0 =
                            leftArtifactCol
                            + (m_selectedSlot > SWAP_ARTIFACTS_PER_COLUMN - 1 ? artifactsSpacing
                                                                              : 0)
                            - 1;
                        selectorY0 = m_selectedSlot % SWAP_ARTIFACTS_PER_COLUMN * artifactsSpacing
                                     + artifactY0 - 1;
                        break;
                }
                break;
            case SWAP_SIDE_RIGHT:
                switch (m_itemType) {
                    case SWAP_ITEM_ARMY:
                        selectorX0 = m_selectedSlot * armyStride + rightArmyLeft - 1;
                        selectorY0 = armyRow - 1;
                        break;
                    case SWAP_ITEM_ARTIFACT:
                        selectorX0 =
                            rightArtifactCol
                            + (m_selectedSlot > SWAP_ARTIFACTS_PER_COLUMN - 1 ? artifactsSpacing
                                                                              : 0)
                            - 1;
                        selectorY0 = m_selectedSlot % SWAP_ARTIFACTS_PER_COLUMN * artifactsSpacing
                                     + artifactY0 - 1;
                        break;
                }
                break;
        }
        m_selectorIcon->FillToBuffer(
            selectorX0 + 16,
            selectorY0 + 16,
            2,
            frameColor,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        gWindowManager->UpdateScreenRegion(selectorX0 + 16, selectorY0 + 16, 36, 36);
    }
}

VA(0x0045d515, 0x88c)
H1_ENUM_RETURN(MessageDispatchResult, i16) swapManager::Main(struct tag_message& message) {
    b8 nowCloseRequested = false;
    b8 quickView;
    i32 artIndex;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickView = true;
    else
        quickView = false;
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
                        nowCloseRequested = true;
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case CONTROL_LEFT_HERO:
                            if (quickView)
                                break;
                            m_heroes[SWAP_SIDE_LEFT]->HeroView(true);
                            gAdvManager->RedrawAdvScreen(true);
                            Update();
                            m_window->DrawWindow();
                            Reset();
                            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
                            break;
                        case CONTROL_RIGHT_HERO:
                            if (quickView)
                                break;
                            m_heroes[SWAP_SIDE_RIGHT]->HeroView(true);
                            gAdvManager->RedrawAdvScreen(true);
                            Update();
                            m_window->DrawWindow();
                            Reset();
                            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
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
                                    gGame->ViewArmy(
                                        TOWN_ARMY_VIEW_X,
                                        TOWN_ARMY_VIEW_Y,
                                        m_heroes[SWAP_SIDE_LEFT]
                                            ->m_army
                                            .m_creatureTypes[message.id - CONTROL_LEFT_ARMY_FIRST],
                                        m_heroes[SWAP_SIDE_LEFT]
                                            ->m_army
                                            .m_creatureCounts[message.id - CONTROL_LEFT_ARMY_FIRST],
                                        NULL,
                                        false,
                                        ARMY_FACING_RIGHT,
                                        1,
                                        m_heroes[SWAP_SIDE_LEFT],
                                        NULL,
                                        &m_heroes[SWAP_SIDE_LEFT]->m_army
                                    );
                                break;
                            }
                            if (H1_ENUM_ENCODE(SwapManagerItemType, m_itemType)) {
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
                                    gGame->ViewArmy(
                                        TOWN_ARMY_VIEW_X,
                                        TOWN_ARMY_VIEW_Y,
                                        m_heroes[SWAP_SIDE_RIGHT]
                                            ->m_army
                                            .m_creatureTypes[message.id - CONTROL_RIGHT_ARMY_FIRST],
                                        m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureCounts
                                            [message.id - CONTROL_RIGHT_ARMY_FIRST],
                                        NULL,
                                        false,
                                        ARMY_FACING_RIGHT,
                                        1,
                                        m_heroes[SWAP_SIDE_RIGHT],
                                        NULL,
                                        &m_heroes[SWAP_SIDE_RIGHT]->m_army
                                    );
                                break;
                            }
                            if (H1_ENUM_ENCODE(SwapManagerItemType, m_itemType)) {
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
    if (nowCloseRequested == true) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0045dda1, 0x9a)
void swapManager::ViewMon(void) {
    gGame->ViewArmy(
        TOWN_ARMY_VIEW_X,
        TOWN_ARMY_VIEW_Y,
        m_heroes[m_selectedSide]->m_army.m_creatureTypes[m_targetSlot],
        m_heroes[m_selectedSide]->m_army.m_creatureCounts[m_targetSlot],
        NULL,
        m_heroes[m_selectedSide]->m_army.GetNumArmies() == 1,
        ARMY_FACING_RIGHT,
        0,
        m_heroes[m_selectedSide],
        NULL,
        &m_heroes[m_selectedSide]->m_army
    );
}

VA(0x0045de3b, 0x126)
void swapManager::SwapArtifacts(void) {
    H1_ENUM_LOCAL(ArtifactType, i8) targetArtifact;
    H1_ENUM_LOCAL(ArtifactType, i8) selectedArtifact;

    if (m_selectedSide == SWAP_SIDE_NONE && m_targetSide == SWAP_SIDE_NONE)
        return;

    selectedArtifact = m_heroes[m_selectedSide]->m_artifacts[m_selectedSlot];
    targetArtifact = m_heroes[m_targetSide]->m_artifacts[m_targetSlot];
    gAdvManager
        ->GiveTakeArtifactStat(m_heroes[m_selectedSide], selectedArtifact, EVENT_ARTIFACT_TAKE);
    gAdvManager->GiveTakeArtifactStat(m_heroes[m_targetSide], targetArtifact, EVENT_ARTIFACT_TAKE);
    m_heroes[m_selectedSide]->m_artifacts[m_selectedSlot] = targetArtifact;
    m_heroes[m_targetSide]->m_artifacts[m_targetSlot] = selectedArtifact;
    gAdvManager
        ->GiveTakeArtifactStat(m_heroes[m_selectedSide], targetArtifact, EVENT_ARTIFACT_GIVE);
    gAdvManager
        ->GiveTakeArtifactStat(m_heroes[m_targetSide], selectedArtifact, EVENT_ARTIFACT_GIVE);
}

VA(0x0045df61, 0x259)
void swapManager::SwapMons(void) {
    armyGroup* targetTroops;
    armyGroup* selectedArmy;
    i16 i;
    i16 j;

    selectedArmy = &m_heroes[m_selectedSide]->m_army;
    targetTroops = &m_heroes[m_targetSide]->m_army;
    if (selectedArmy != targetTroops) {
        if (selectedArmy->GetNumArmies() == 1
            && (targetTroops->m_creatureTypes[m_targetSlot] == CREATURE_NONE
                || targetTroops->IsMember(selectedArmy->m_creatureTypes[m_selectedSlot])))
            return;
        if (targetTroops->IsMember(selectedArmy->m_creatureTypes[m_selectedSlot])) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (targetTroops->m_creatureTypes[i]
                    == selectedArmy->m_creatureTypes[m_selectedSlot])
                    break;
            }
            targetTroops->m_creatureCounts[i] += selectedArmy->m_creatureCounts[m_selectedSlot];
            selectedArmy->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
            selectedArmy->m_creatureCounts[m_selectedSlot] = 0;
            return;
        } else if (selectedArmy->IsMember(targetTroops->m_creatureTypes[m_targetSlot])) {
            for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                if (selectedArmy->m_creatureTypes[j] == targetTroops->m_creatureTypes[m_targetSlot])
                    break;
            }
            selectedArmy->m_creatureCounts[j] += targetTroops->m_creatureCounts[m_targetSlot];
            targetTroops->m_creatureTypes[m_targetSlot] = CREATURE_NONE;
            targetTroops->m_creatureCounts[m_targetSlot] = 0;
            if (j != m_selectedSlot) {
                targetTroops->m_creatureTypes[m_targetSlot] =
                    selectedArmy->m_creatureTypes[m_selectedSlot];
                targetTroops->m_creatureCounts[m_targetSlot] =
                    selectedArmy->m_creatureCounts[m_selectedSlot];
                selectedArmy->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
                selectedArmy->m_creatureCounts[m_selectedSlot] = 0;
            }
            return;
        }
    }
    selectedArmy->Swap(m_selectedSlot, targetTroops, m_targetSlot);
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
        sprintf(
            gText,
            "%d",
            m_heroes[SWAP_SIDE_LEFT]->m_primaryStats[H1_ENUM_DECODE(HeroPrimaryStat, i)]
        );
        m_window->BroadcastMessage(message);
        message.id = i + CONTROL_RIGHT_PRIMARY_SKILL_FIRST;
        sprintf(
            gText,
            "%d",
            m_heroes[SWAP_SIDE_RIGHT]->m_primaryStats[H1_ENUM_DECODE(HeroPrimaryStat, i)]
        );
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
            message.value =
                H1_ENUM_ENCODE(CreatureType, m_heroes[SWAP_SIDE_LEFT]->m_army.m_creatureTypes[i]);
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
            message.value =
                H1_ENUM_ENCODE(CreatureType, m_heroes[SWAP_SIDE_RIGHT]->m_army.m_creatureTypes[i]);
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
            message.value = H1_ENUM_ENCODE(ArtifactType, m_heroes[SWAP_SIDE_LEFT]->m_artifacts[i]);
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
            message.value = H1_ENUM_ENCODE(ArtifactType, m_heroes[SWAP_SIDE_RIGHT]->m_artifacts[i]);
        }
        m_window->BroadcastMessage(message);
    }
}

VA(0x0045e628, 0x352)
void swapManager::SplitMons(void) {
    i16 textControl;
    armyGroup* targetTroops;
    armyGroup* selectedArmy;
    i16 amountWidgetId;
    tag_message message;
    i16 found;
    i16 placeSlot;

    amountWidgetId = TOWN_SPLIT_AMOUNT_CONTROL;
    found = 0;
    selectedArmy = &m_heroes[m_selectedSide]->m_army;
    targetTroops = &m_heroes[m_targetSide]->m_army;
    found = 0;
    textControl = TOWN_SPLIT_PROMPT_CONTROL;
    gTownManager->m_childWindow =
        new heroWindow(TOWN_SPLIT_WINDOW_X, TOWN_SPLIT_WINDOW_Y, "splitwin.bin");
    if (!gTownManager->m_childWindow)
        MemError();
    gTownManager->m_splitAmount = 0;
    gTownManager->m_splitMaximum = selectedArmy->m_creatureCounts[m_selectedSlot];
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
    gTownManager->m_childWindow->BroadcastMessage(message);
    sprintf(gText, "%d", gTownManager->m_splitAmount);
    message.id = TOWN_SPLIT_AMOUNT_CONTROL;
    message.text = gText;
    gTownManager->m_childWindow->BroadcastMessage(message);
    gWindowManager->DoDialog(gTownManager->m_childWindow, SplitArmyHandler, false);
    delete gTownManager->m_childWindow;
    if (gWindowManager->m_dialogResult == DIALOG_BUTTON_2) {
        for (placeSlot = 0; placeSlot < ARMY_GROUP_SLOT_COUNT; placeSlot++) {
            if (targetTroops->m_creatureTypes[placeSlot]
                == selectedArmy->m_creatureTypes[m_selectedSlot]) {
                found = 1;
                break;
            }
        }
        if (found) {
            targetTroops->m_creatureCounts[placeSlot] += gTownManager->m_splitAmount;
        } else {
            if (targetTroops->m_creatureTypes[m_targetSlot] != CREATURE_NONE) {
                for (placeSlot = 0; placeSlot < ARMY_GROUP_SLOT_COUNT; placeSlot++) {
                    if (targetTroops->m_creatureTypes[placeSlot] == CREATURE_NONE)
                        break;
                }
                if (placeSlot < ARMY_GROUP_SLOT_COUNT)
                    m_targetSlot = placeSlot;
            }
            targetTroops->m_creatureTypes[m_targetSlot] =
                selectedArmy->m_creatureTypes[m_selectedSlot];
            targetTroops->m_creatureCounts[m_targetSlot] = gTownManager->m_splitAmount;
        }
        selectedArmy->m_creatureCounts[m_selectedSlot] -= gTownManager->m_splitAmount;
        if (selectedArmy->m_creatureCounts[m_selectedSlot] == 0)
            selectedArmy->m_creatureTypes[m_selectedSlot] = CREATURE_NONE;
    }
}
