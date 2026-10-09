#include <H1/Ints.h>

#include <BASE/baseManager.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/widget.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/bankBox.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/recruitUnit.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SetupRecruitWin(
    heroWindow* window,
    i32 creatureType,
    i32 goldCost,
    i32 resourceType,
    i32 resourceCost,
    i32 available
) {
    char monsterName[RECRUIT_NAME_SIZE];
    char label[RECRUIT_LABEL_SIZE];
    tag_message message;

    strcpy(monsterName, GetMonsterName(creatureType));
    monsterName[0] = CyrillicToLower(monsterName[0]);
    sprintf(label, "%s %s", localization::Tr("recruitment.title.prefix"), monsterName);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, RECRUIT_TITLE_CONTROL);
    message.text = label;
    window->BroadcastMessage(message);

    sprintf(label, "%d", goldCost);
    message.id = RECRUIT_GOLD_COST_CONTROL;
    window->BroadcastMessage(message);
    if (resourceType != RESOURCE_NONE) {
        sprintf(label, "%d", resourceCost);
        message.id = RECRUIT_RESOURCE_COST_CONTROL;
        window->BroadcastMessage(message);
    }

    sprintf(gText, "%s%d", localization::Tr("recruitment.available.label"), available);
    message.id = RECRUIT_AVAILABLE_CONTROL;
    message.text = gText;
    window->BroadcastMessage(message);

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, RECRUIT_CREATURE_CONTROL);
    message.value = creatureType;
    window->BroadcastMessage(message);
    if (resourceType != RESOURCE_NONE) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = RECRUIT_RESOURCE_ICON_CONTROL;
        message.value = resourceType;
        window->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = RECRUIT_RESOURCE_IMAGE_CONTROL;
        window->BroadcastMessage(message);
    }
}

i16 recruitUnit::Open(i16 priority) {
    i32 resourceMaximum;
    i32 goldMaximum;

    m_window = new heroWindow(
        RECRUIT_WINDOW_X,
        RECRUIT_WINDOW_Y,
        const_cast<char*>(m_resourceType == RESOURCE_NONE ? "recruit0.bin" : "recruit1.bin")
    );
    if (m_window == NULL)
        MemError();
    m_quantity = 0;
    m_goldTotal = 0;
    m_resourceTotal = 0;
    SetupRecruitWin(
        m_window,
        m_creatureType,
        m_goldCost,
        m_resourceType,
        m_resourceCost,
        *m_available
    );
    Update();
    gWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_SET_FLAGS,
        RECRUIT_CLOSE_CONTROL,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    gWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, true);

    goldMaximum = gCurPlayerData->m_resources[RESOURCE_GOLD] / m_goldCost;
    if (m_resourceType != RESOURCE_NONE) {
        resourceMaximum = gCurPlayerData->m_resources[m_resourceType] / m_resourceCost;
        m_maximum = __min(goldMaximum, resourceMaximum);
    } else
        m_maximum = goldMaximum;
    if (m_maximum > *m_available)
        m_maximum = *m_available;
    m_recruited = false;
    m_noRoom = false;
    if (*m_available == 0) {
        gWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            RECRUIT_CONFIRM_CONTROL,
            WIDGET_FLAG_ENABLED
        );
        gWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            RECRUIT_CONFIRM_CONTROL,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
    }
    KBChangeMenu(gDefaultMenu);
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "recruitManager");
    return BASE_MANAGER_SUCCESS;
}

void recruitUnit::Close(void) {
    gWindowManager->RemoveWindow(m_window);
    delete m_window;
    if (m_noRoom)
        NormalDialog(
            localization::Tr("recruitment.garrison.full"),
            NORMAL_DIALOG_TYPE_OK,
            RECRUIT_NO_ROOM_DIALOG_X,
            RECRUIT_NO_ROOM_DIALOG_Y
        );
    gWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_CLEAR_FLAGS,
        RECRUIT_CLOSE_CONTROL,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    if (m_sourceType == RECRUIT_SOURCE_TOWN && m_recruited) {
        gTownManager->ResetStrips();
        gTownManager->m_bankBox->Update();
    }
    m_active = 0;
}

void recruitUnit::Update(void) {
    char text[20];
    tag_message message;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    sprintf(gText, "%s%d", localization::Tr("recruitment.available.label"), *m_available);
    message.id = RECRUIT_AVAILABLE_CONTROL;
    message.text = gText;
    m_window->BroadcastMessage(message);
    sprintf(gText, "%d", m_quantity);
    message.id = RECRUIT_QUANTITY_CONTROL;
    m_window->BroadcastMessage(message);
    m_goldTotal = m_quantity * m_goldCost;
    sprintf(gText, "%d", m_goldTotal);
    message.id = RECRUIT_GOLD_TOTAL_CONTROL;
    m_window->BroadcastMessage(message);
    if (m_resourceType != RESOURCE_NONE) {
        m_resourceTotal = m_quantity * m_resourceCost;
        sprintf(gText, "%d", m_resourceTotal);
        message.id = RECRUIT_RESOURCE_TOTAL_CONTROL;
        m_window->BroadcastMessage(message);
    }
}

i16 recruitUnit::Main(struct tag_message& message) {
    b32 done;
    i32 cost;
    b8 quickView;

    done = false;
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickView = true;
    else
        quickView = false;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case RECRUIT_INCREASE_CONTROL:
                        if (quickView)
                            break;
                        m_quantity++;
                        if (m_quantity > m_maximum)
                            m_quantity = m_maximum;
                        break;
                    case RECRUIT_DECREASE_CONTROL:
                        if (quickView)
                            break;
                        m_quantity--;
                        if (m_quantity < 0)
                            m_quantity = 0;
                        break;
                    case RECRUIT_QUANTITY_CONTROL:
                        if (quickView)
                            break;
                        message.command = WIDGET_COMMAND_GET_TEXT;
                        m_window->BroadcastMessage(message);
                        m_quantity = atoi(message.text);
                        if (m_quantity < 0)
                            m_quantity = 0;
                        if (m_quantity > m_maximum)
                            m_quantity = m_maximum;
                        break;
                    case RECRUIT_CREATURE_CONTROL:
                        gGame->ViewArmy(
                            RECRUIT_VIEW_ARMY_X,
                            RECRUIT_VIEW_ARMY_Y,
                            m_creatureType,
                            0,
                            NULL,
                            true,
                            ARMY_FACING_RIGHT,
                            quickView,
                            NULL,
                            NULL,
                            NULL
                        );
                        break;
                    default:
                        break;
                }
                Update();
                m_window->MoveWindow(0, 0);
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case RECRUIT_MAXIMUM_CONTROL:
                        if (quickView)
                            break;
                        m_quantity = m_maximum;
                        Update();
                        m_window->MoveWindow(0, 0);
                        break;
                    case RECRUIT_CANCEL_CONTROL:
                        if (quickView)
                            break;
                        m_quantity = 0;
                        done = true;
                        break;
                    case RECRUIT_CONFIRM_CONTROL:
                        if (quickView)
                            break;
                        if (m_quantity == 0) {
                            done = true;
                            goto checkClose;
                        }
                        if (m_army->CanJoin(m_creatureType)) {
                            m_army->Add(m_creatureType, m_quantity, ARMY_GROUP_EMPTY_SLOT);
                        } else {
                            done = true;
                            m_noRoom = true;
                            goto checkClose;
                        }
                        gCurPlayerData->m_resources[RESOURCE_GOLD] -= m_quantity * m_goldCost;
                        if (m_resourceType != RESOURCE_NONE)
                            gCurPlayerData->m_resources[m_resourceType] -=
                                m_quantity * m_resourceCost;
                        *m_available -= m_quantity;
                        m_recruited = true;
                        done = true;
                        break;
                }
                break;
            default:
                break;
        }

    checkClose:
        if (done == true) {
            message.type = MESSAGE_EXECUTIVE;
            message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

recruitUnit::recruitUnit(
    armyGroup* army,
    i8 creatureType,
    i16* available
) {
    i32 unitCosts[RESOURCE_COUNT];
    i32 resourceIndex;

    m_sourceType = RECRUIT_SOURCE_EVENT;
    m_army = army;
    m_creatureType = creatureType;
    m_available = available;
    GetMonsterCost(m_creatureType, unitCosts);
    m_goldCost = unitCosts[RESOURCE_GOLD];
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_NON_GOLD_END; resourceIndex++) {
        if (unitCosts[resourceIndex])
            break;
    }
    if (resourceIndex < RESOURCE_NON_GOLD_END) {
        m_resourceType = resourceIndex;
        m_resourceCost = unitCosts[m_resourceType];
    } else {
        m_resourceType = RESOURCE_NONE;
        m_resourceCost = 0;
    }
}

recruitUnit::recruitUnit(town* townData, i8 dwelling) {
    i32 unitCosts[RESOURCE_COUNT];
    i32 resourceIndex;

    m_sourceType = RECRUIT_SOURCE_TOWN;
    m_army = &townData->m_army;
    m_creatureType = gDwellingType[townData->m_type][dwelling];
    m_available = &townData->m_dwellingAvailable[dwelling];
    GetMonsterCost(m_creatureType, unitCosts);
    m_goldCost = unitCosts[RESOURCE_GOLD];
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_NON_GOLD_END; resourceIndex++) {
        if (unitCosts[resourceIndex])
            break;
    }
    if (resourceIndex < RESOURCE_NON_GOLD_END) {
        m_resourceType = resourceIndex;
        m_resourceCost = unitCosts[m_resourceType];
    } else {
        m_resourceType = RESOURCE_NONE;
        m_resourceCost = 0;
    }
}

void QuickViewRecruit(town* townData, i8 dwelling) {
    i32 monsterType;
    i32 resourceType;
    heroWindow* recruitWindow;
    i32 unitCosts[RESOURCE_COUNT];
    i32 resourceCost;
    i32 goldCost;
    i32 resourceIndex;
    i32 avail;

    monsterType = gDwellingType[townData->m_type][dwelling];
    avail = townData->m_dwellingAvailable[dwelling];
    GetMonsterCost(monsterType, unitCosts);
    goldCost = unitCosts[RESOURCE_GOLD];
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_NON_GOLD_END; resourceIndex++) {
        if (unitCosts[resourceIndex])
            break;
    }
    if (resourceIndex < RESOURCE_NON_GOLD_END) {
        resourceType = resourceIndex;
        resourceCost = unitCosts[resourceType];
    } else {
        resourceType = RESOURCE_NONE;
        resourceCost = 0;
    }

    recruitWindow = new heroWindow(
        RECRUIT_WINDOW_X,
        RECRUIT_WINDOW_Y,
        const_cast<char*>(resourceType == RESOURCE_NONE ? "recruiq0.bin" : "recruiq1.bin")
    );
    if (recruitWindow == NULL)
        MemError();
    SetupRecruitWin(recruitWindow, monsterType, goldCost, resourceType, resourceCost, avail);
    gMouseManager->ReallyHidePointer();
    gWindowManager->AddWindow(recruitWindow, WINDOW_Z_ORDER_APPEND, true);
    QuickViewWait();
    gWindowManager->RemoveWindow(recruitWindow);
    gMouseManager->ReallyShowPointer();
}
