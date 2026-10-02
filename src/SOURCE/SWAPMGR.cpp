// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/townManager.h>

#include <stdio.h>
#include <string.h>

// donor PoL RVA 0x00054502; preferred Buka symbol ?Open@swapManager@@UAEHH@Z
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.683274;margin=0.416085;shape=0.517;size=0.729;calls=0.800;strings=port%04d.icn|swapManager|swapbtn.icn;alternate=pol20:int swapManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00054502
VA(0x0046ecb0, 0x75)
swapManager::swapManager(void) {
    m_window = NULL;
    m_selectorIcon = NULL;
    m_selectedSide = -1;
    m_targetSide = -1;
    m_itemType = -1;
    m_selectedSlot = -1;
    m_targetSlot = -1;
    m_heroes[1] = NULL;
    m_heroes[0] = NULL;
}

VA(0x0046ed25, 0x3e)
swapManager::swapManager(class hero* leftHero, class hero* rightHero) {
    m_heroes[1] = leftHero;
    m_heroes[0] = rightHero;
}

VA(0x0046ed63, 0x4d)
void swapManager::Reset(void) {
    m_selectedSide = m_targetSide = m_itemType = m_selectedSlot = m_targetSlot = -1;
}

VA(0x0046edb0, 0x2d5)
short swapManager::Open(short id) {
    tag_message message;
    int i; // Unused; retail still reserves its frame slot.

    Reset();
    m_window = new heroWindow(16, 16, "swapwin.bin");
    if (!m_window)
        MemError();
    SetWinText(m_window, 13);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_ICON;
    sprintf(gText, "port%04d.icn", m_heroes[1]->m_unknown1d);
    message.id = 65;
    message.text = gText;
    m_window->BroadcastMessage(message);
    sprintf(gText, "port%04d.icn", m_heroes[0]->m_unknown1d);
    message.id = 66;
    m_window->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    sprintf(gText, "%s meets %s", m_heroes[1]->m_name, m_heroes[0]->m_name);
    message.text = gText;
    message.id = 77;
    m_window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED;
    message.id = 1;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 2;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 3;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 4;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 5;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 6;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    Update();
    gpWindowManager->AddWindow(m_window, -1, 1);
    KBChangeMenu(hmnuAdv);
    giMonoIconSkip = 2;
    m_selectorIcon = gpResourceManager->GetIcon("swapbtn.icn");
    giMonoIconSkip = -1;
    gpMouseManager->SetPointer(0);
    m_messageFilter = 0x32f;
    m_messageMask = BASE_MANAGER_ACCEPT_SWAP;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "swapManager");
    return 0;
}

// donor PoL RVA 0x000548be; preferred Buka symbol ?Close@swapManager@@UAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.610218;margin=0.602384;shape=0.500;size=0.986;calls=1.000;alternate=pol20:void swapManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x000548be
VA(0x0046f085, 0x123)
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
    message.id = 1;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 2;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 3;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 4;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 5;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.id = 6;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
}

VA(0x0046f1a8, 0x21f)
void swapManager::DrawSelector(void) {
    const short frameColor = 232;
    const short leftArmyBase = 24;
    const short rightMonsterBase = 252;
    const short troopTop = 148;
    const short armySpacing = 35;
    const short art1 = 76;
    const short art2 = 305;
    const short artTop = 194;
    const short itemGap = 35;
    short x = 0;
    short y = 0;

    if (m_selectedSide != -1 && m_selectedSlot != -1) {
        switch (m_selectedSide) {
        case 1:
            switch (m_itemType) {
            case 0:
                x = m_selectedSlot * armySpacing + leftArmyBase - 1;
                y = troopTop - 1;
                break;
            case 1:
                x = art1 + (m_selectedSlot > 6 ? itemGap : 0) - 1;
                y = m_selectedSlot % 7 * itemGap + artTop - 1;
                break;
            }
            break;
        case 0:
            switch (m_itemType) {
            case 0:
                x = m_selectedSlot * armySpacing + rightMonsterBase - 1;
                y = troopTop - 1;
                break;
            case 1:
                x = art2 + (m_selectedSlot > 6 ? itemGap : 0) - 1;
                y = m_selectedSlot % 7 * itemGap + artTop - 1;
                break;
            }
            break;
        }
        m_selectorIcon->FillToBuffer(x + 16, y + 16, 2, frameColor, 0, 0);
        gpWindowManager->UpdateScreenRegion(x + 16, y + 16, 36, 36);
    }
}

// donor PoL RVA 0x00054be3; preferred Buka symbol ?Main@swapManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.525982;margin=0.522986;shape=0.320;size=0.991;calls=0.960;alternate=pol20:int swapManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00054be3
VA(0x0046f3c7, 0x9ac)
short swapManager::Main(struct tag_message& message) {
    signed char closeRequested = 0;
    signed char quickView;
    int artIndex;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickView = 1;
    else
        quickView = 0;
    if (!(message.type & m_messageFilter)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return 2;
        }
        return 0;
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
            if (message.id == 0x7800)
                closeRequested = 1;
            break;
        case WIDGET_NOTIFY_SELECT:
            switch (message.id) {
            case 65:
                if (quickView)
                    break;
                m_heroes[1]->HeroView(1);
                gpAdvManager->RedrawAdvScreen(1);
                Update();
                m_window->DrawWindow();
                Reset();
                gpWindowManager->FadeScreen(0, 8, 0);
                break;
            case 66:
                if (quickView)
                    break;
                m_heroes[0]->HeroView(1);
                gpAdvManager->RedrawAdvScreen(1);
                Update();
                m_window->DrawWindow();
                Reset();
                gpWindowManager->FadeScreen(0, 8, 0);
                break;
            case 88:
            case 89:
            case 90:
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97:
            case 98:
            case 99:
            case 100:
            case 101:
                artIndex = message.id - 88;
                if (!quickView && (m_heroes[1]->m_artifacts[artIndex] == ARTIFACT_MAGIC_BOOK
                                   || m_heroes[1]->m_artifacts[artIndex] == 12)) {
                    NormalDialog("This item can't be traded.", 1, -1, -1, -1, 0, -1, 0, -1);
                    break;
                }
                if (quickView) {
                    if (m_heroes[1]->m_artifacts[artIndex] == -1)
                        break;
                    m_heroes[1]->ViewArtifact(m_heroes[1]->m_artifacts[artIndex], 1);
                    break;
                }
                if (m_itemType != 1) {
                    if (m_heroes[1]->m_artifacts[artIndex] != -1) {
                        m_selectedSide = 1;
                        m_targetSide = -1;
                        m_itemType = 1;
                        m_selectedSlot = artIndex;
                        m_targetSlot = -1;
                    } else {
                        Reset();
                    }
                } else {
                    m_targetSide = 1;
                    m_targetSlot = artIndex;
                    if (m_selectedSide == 1 && m_selectedSlot == m_targetSlot) {
                        m_heroes[1]->ViewArtifact(m_heroes[1]->m_artifacts[artIndex], 0);
                        Reset();
                    }
                    SwapArtifacts();
                    Reset();
                }
                break;
            case 102:
            case 103:
            case 104:
            case 105:
            case 106:
            case 107:
            case 108:
            case 109:
            case 110:
            case 111:
            case 112:
            case 113:
            case 114:
            case 115:
                artIndex = message.id - 102;
                if (!quickView && (m_heroes[0]->m_artifacts[artIndex] == ARTIFACT_MAGIC_BOOK
                                   || m_heroes[0]->m_artifacts[artIndex] == 12)) {
                    NormalDialog("This item can't be traded.", 1, -1, -1, -1, 0, -1, 0, -1);
                    break;
                }
                if (quickView) {
                    if (m_heroes[0]->m_artifacts[artIndex] == -1)
                        break;
                    m_heroes[0]->ViewArtifact(m_heroes[0]->m_artifacts[artIndex], 1);
                    break;
                }
                if (m_itemType != 1) {
                    if (m_heroes[0]->m_artifacts[artIndex] != -1) {
                        m_selectedSide = 0;
                        m_targetSide = -1;
                        m_itemType = 1;
                        m_selectedSlot = artIndex;
                        m_targetSlot = -1;
                    } else {
                        Reset();
                    }
                } else {
                    m_targetSide = 0;
                    m_targetSlot = artIndex;
                    if (m_selectedSide == 0 && m_selectedSlot == m_targetSlot) {
                        m_heroes[0]->ViewArtifact(m_heroes[0]->m_artifacts[artIndex], 0);
                        Reset();
                    }
                    SwapArtifacts();
                    Reset();
                }
                break;
            case 78:
            case 79:
            case 80:
            case 81:
            case 82:
                if (quickView) {
                    if (m_heroes[1]->m_army.m_creatureTypes[message.id - 78] != -1)
                        gpGame->ViewArmy(119, 20,
                                         m_heroes[1]->m_army.m_creatureTypes[message.id - 78],
                                         m_heroes[1]->m_army.m_creatureCounts[message.id - 78],
                                         0, 0, 0, 1, m_heroes[1], 0, &m_heroes[1]->m_army);
                    break;
                }
                if (m_itemType) {
                    if (m_heroes[1]->m_army.m_creatureTypes[message.id - 78] != -1) {
                        m_selectedSide = 1;
                        m_targetSide = -1;
                        m_itemType = 0;
                        m_selectedSlot = message.id - 78;
                        m_targetSlot = -1;
                    } else {
                        Reset();
                    }
                } else {
                    m_targetSide = 1;
                    m_targetSlot = message.id - 78;
                    if (m_selectedSide == 1 && m_selectedSlot == m_targetSlot) {
                        ViewMon();
                        Reset();
                    }
                    if ((message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS)
                        && m_selectedSide != m_targetSide
                        && (m_heroes[m_targetSide]->m_army.m_creatureTypes[m_targetSlot] == -1
                            || m_heroes[m_selectedSide]->m_army.m_creatureTypes[m_selectedSlot]
                                   == m_heroes[m_targetSide]->m_army.m_creatureTypes[m_targetSlot])) {
                        SplitMons();
                        Reset();
                    } else {
                        SwapMons();
                        Reset();
                    }
                }
                break;
            case 83:
            case 84:
            case 85:
            case 86:
            case 87:
                if (quickView) {
                    if (m_heroes[0]->m_army.m_creatureTypes[message.id - 83] != -1)
                        gpGame->ViewArmy(119, 20,
                                         m_heroes[0]->m_army.m_creatureTypes[message.id - 83],
                                         m_heroes[0]->m_army.m_creatureCounts[message.id - 83],
                                         0, 0, 0, 1, m_heroes[0], 0, &m_heroes[0]->m_army);
                    break;
                }
                if (m_itemType) {
                    if (m_heroes[0]->m_army.m_creatureTypes[message.id - 83] != -1) {
                        m_selectedSide = 0;
                        m_targetSide = -1;
                        m_itemType = 0;
                        m_selectedSlot = message.id - 83;
                        m_targetSlot = -1;
                    } else {
                        Reset();
                    }
                } else {
                    m_targetSide = 0;
                    m_targetSlot = message.id - 83;
                    if (m_selectedSide == 0 && m_selectedSlot == m_targetSlot) {
                        ViewMon();
                        Reset();
                    }
                    if ((message.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS)
                        && m_selectedSide != m_targetSide
                        && (m_heroes[m_targetSide]->m_army.m_creatureTypes[m_targetSlot] == -1
                            || m_heroes[m_selectedSide]->m_army.m_creatureTypes[m_selectedSlot]
                                   == m_heroes[m_targetSide]->m_army.m_creatureTypes[m_targetSlot])) {
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
        return 2;
    }
    return 1;
}

VA(0x0046fd73, 0xa5)
void swapManager::ViewMon(void) {
    gpGame->ViewArmy(119, 20, m_heroes[m_selectedSide]->m_army.m_creatureTypes[m_targetSlot],
                     m_heroes[m_selectedSide]->m_army.m_creatureCounts[m_targetSlot], NULL,
                     m_heroes[m_selectedSide]->m_army.GetNumArmies() == 1, 0, 0,
                     m_heroes[m_selectedSide], NULL, &m_heroes[m_selectedSide]->m_army);
}

// donor PoL RVA 0x000556d3; preferred Buka symbol ?ViewMon@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.509716;margin=0.520416;shape=0.289;size=0.921;calls=1.000;alternate=pol20:void swapManager::ViewMon(void)@0x000556d3


// donor PoL RVA 0x00055b42; preferred Buka symbol ?Update@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.575258;margin=0.321463;shape=0.441;size=0.938;calls=0.900;alternate=pol20:void swapManager::Update(void)@0x00055b42
VA(0x0046fe18, 0x112)
void swapManager::SwapArtifacts(void) {
    signed char dstArt;
    signed char srcArt;

    srcArt = m_heroes[m_selectedSide]->m_artifacts[m_selectedSlot];
    dstArt = m_heroes[m_targetSide]->m_artifacts[m_targetSlot];
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_selectedSide], srcArt, 1);
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_targetSide], dstArt, 1);
    m_heroes[m_selectedSide]->m_artifacts[m_selectedSlot] = dstArt;
    m_heroes[m_targetSide]->m_artifacts[m_targetSlot] = srcArt;
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_selectedSide], dstArt, 0);
    gpAdvManager->GiveTakeArtifactStat(m_heroes[m_targetSide], srcArt, 0);
}

VA(0x0046ff2a, 0x28e)
void swapManager::SwapMons(void) {
    armyGroup* destTroops;
    armyGroup* sourceTroops;
    short i;
    short j;

    sourceTroops = &m_heroes[m_selectedSide]->m_army;
    destTroops = &m_heroes[m_targetSide]->m_army;
    if (sourceTroops != destTroops) {
        if (sourceTroops->GetNumArmies() == 1
            && (destTroops->m_creatureTypes[m_targetSlot] == -1
                || destTroops->IsMember(sourceTroops->m_creatureTypes[m_selectedSlot])))
            return;
        if (destTroops->IsMember(sourceTroops->m_creatureTypes[m_selectedSlot])) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (sourceTroops->m_creatureTypes[m_selectedSlot] == destTroops->m_creatureTypes[i])
                    break;
            }
            destTroops->m_creatureCounts[i] += sourceTroops->m_creatureCounts[m_selectedSlot];
            sourceTroops->m_creatureTypes[m_selectedSlot] = -1;
            sourceTroops->m_creatureCounts[m_selectedSlot] = 0;
            return;
        } else if (sourceTroops->IsMember(destTroops->m_creatureTypes[m_targetSlot])) {
            for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                if (destTroops->m_creatureTypes[m_targetSlot] == sourceTroops->m_creatureTypes[j])
                    break;
            }
            sourceTroops->m_creatureCounts[j] += destTroops->m_creatureCounts[m_targetSlot];
            destTroops->m_creatureTypes[m_targetSlot] = -1;
            destTroops->m_creatureCounts[m_targetSlot] = 0;
            if (m_selectedSlot != j) {
                destTroops->m_creatureTypes[m_targetSlot] = sourceTroops->m_creatureTypes[m_selectedSlot];
                destTroops->m_creatureCounts[m_targetSlot] = sourceTroops->m_creatureCounts[m_selectedSlot];
                sourceTroops->m_creatureTypes[m_selectedSlot] = -1;
                sourceTroops->m_creatureCounts[m_selectedSlot] = 0;
            }
            return;
        }
    }
    sourceTroops->Swap(m_selectedSlot, destTroops, m_targetSlot);
}

VA(0x004701b8, 0x492)
void swapManager::Update(void) {
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.text = gText;
    for (i = 0; i < HERO_PRIMARY_STAT_COUNT; i++) {
        message.id = i + 67;
        sprintf(gText, "%d", m_heroes[1]->m_primaryStats[i]);
        m_window->BroadcastMessage(message);
        message.id = i + 72;
        sprintf(gText, "%d", m_heroes[0]->m_primaryStats[i]);
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + 78;
        if (m_heroes[1]->m_army.m_creatureTypes[i] == -1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[1]->m_army.m_creatureTypes[i];
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + 116;
        if (m_heroes[1]->m_army.m_creatureTypes[i] == -1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            sprintf(gText, "%d", m_heroes[1]->m_army.m_creatureCounts[i]);
            message.text = gText;
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + 83;
        if (m_heroes[0]->m_army.m_creatureTypes[i] == -1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[0]->m_army.m_creatureTypes[i];
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + 121;
        if (m_heroes[0]->m_army.m_creatureTypes[i] == -1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            sprintf(gText, "%d", m_heroes[0]->m_army.m_creatureCounts[i]);
            message.text = gText;
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + 88;
        if (m_heroes[1]->m_artifacts[i] == -1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[1]->m_artifacts[i];
        }
        m_window->BroadcastMessage(message);
    }
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + 102;
        if (m_heroes[0]->m_artifacts[i] == -1) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_heroes[0]->m_artifacts[i];
        }
        m_window->BroadcastMessage(message);
    }
}

// donor PoL RVA 0x00055fbd; preferred Buka symbol ?SplitMons@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.761691;margin=0.047739;shape=0.487;size=0.960;calls=1.000;strings=splitwin.bin;alternate=pol20:void swapManager::SplitMons(void)@0x00055fbd
VA(0x0047064a, 0x39f)
void swapManager::SplitMons(void) {
    short textId;
    armyGroup* dstTroops;
    armyGroup* selectedArmy;
    short amountWidget;
    tag_message message;
    short found;
    short i;

    amountWidget = 68;
    found = 0;
    selectedArmy = &m_heroes[m_selectedSide]->m_army;
    dstTroops = &m_heroes[m_targetSide]->m_army;
    found = 0;
    textId = 1;
    gpTownManager->m_heroWindow1 = new heroWindow(177, 20, "splitwin.bin");
    if (!gpTownManager->m_heroWindow1)
        MemError();
    gpTownManager->m_splitAmount = 0;
    gpTownManager->m_splitMaximum = selectedArmy->m_creatureCounts[m_selectedSlot];
    message.type = MESSAGE_WIDGET;
    sprintf(gText, "Move how many %s troops from %s to %s?",
            gArmyNames[selectedArmy->m_creatureTypes[m_selectedSlot]],
            m_heroes[m_selectedSide]->m_name, m_heroes[m_targetSide]->m_name);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "%d", gpTownManager->m_splitAmount);
    message.id = 68;
    message.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    gpWindowManager->DoDialog(gpTownManager->m_heroWindow1, SplitArmyHandler, 0);
    delete gpTownManager->m_heroWindow1;
    if (gpWindowManager->m_dialogResult == 0x7802) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (selectedArmy->m_creatureTypes[m_selectedSlot] == dstTroops->m_creatureTypes[i]) {
                found = 1;
                break;
            }
        }
        if (found) {
            dstTroops->m_creatureCounts[i] += gpTownManager->m_splitAmount;
        } else {
            if (dstTroops->m_creatureTypes[m_targetSlot] != -1) {
                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                    if (dstTroops->m_creatureTypes[i] == -1)
                        break;
                }
                if (i < ARMY_GROUP_SLOT_COUNT)
                    m_targetSlot = i;
            }
            dstTroops->m_creatureTypes[m_targetSlot] = selectedArmy->m_creatureTypes[m_selectedSlot];
            dstTroops->m_creatureCounts[m_targetSlot] = gpTownManager->m_splitAmount;
        }
        selectedArmy->m_creatureCounts[m_selectedSlot] -= gpTownManager->m_splitAmount;
        if (selectedArmy->m_creatureCounts[m_selectedSlot] == 0)
            selectedArmy->m_creatureTypes[m_selectedSlot] = -1;
    }
}
