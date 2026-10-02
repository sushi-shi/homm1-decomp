// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <H1/KB.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/kbwin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

H1_ENUM_CONST_BEGIN(RecruitConstant)
RECRUIT_RESOURCE_COUNT = 6,
    RECRUIT_GOLD_RESOURCE = 6, RECRUIT_NO_RESOURCE = -1, RECRUIT_WINDOW_X = 0xa0,
    RECRUIT_WINDOW_Y = 0x10, RECRUIT_VIEW_ARMY_X = 0x77, RECRUIT_VIEW_ARMY_Y = 0x20,
    RECRUIT_NO_ROOM_DIALOG_X = 0xb1, RECRUIT_NO_ROOM_DIALOG_Y = 0x64, RECRUIT_MANAGER_OPEN_OK = 0,
    RECRUIT_WIDGET_FLAGS_DIMMED = 0x4008, RECRUIT_WIDGET_FLAG_ENABLED = 2,
    RECRUIT_NOTIFY_SELECT = 12,
    RECRUIT_COMMAND_GET_TEXT = 7 H1_ENUM_CONST_END(RecruitConstant)

        H1_ENUM_BEGIN(RecruitControl) RECRUIT_CLOSE_CONTROL = 0x7800,
    RECRUIT_CANCEL_CONTROL = 0x7801, RECRUIT_CONFIRM_CONTROL = 0x7802, RECRUIT_TITLE_CONTROL = 0x40,
    RECRUIT_CREATURE_CONTROL = 0x42, RECRUIT_AVAILABLE_CONTROL = 0x43,
    RECRUIT_QUANTITY_CONTROL = 0x44, RECRUIT_INCREASE_CONTROL = 0x45,
    RECRUIT_DECREASE_CONTROL = 0x46, RECRUIT_MAXIMUM_CONTROL = 0x47,
    RECRUIT_GOLD_COST_CONTROL = 0x49, RECRUIT_RESOURCE_ICON_CONTROL = 0x4a,
    RECRUIT_RESOURCE_COST_CONTROL = 0x4b, RECRUIT_GOLD_TOTAL_CONTROL = 0x4d,
    RECRUIT_RESOURCE_IMAGE_CONTROL = 0x4e,
    RECRUIT_RESOURCE_TOTAL_CONTROL = 0x4f H1_ENUM_END(RecruitControl)

    // Buka RECRUIT.cpp:58-112; HoMM1 capitalizes the plural name in place and
    // sets the creature portrait by frame rather than by icon name.
    VA(0x00401b60, 0x164)
void SetupRecruitWin(
    heroWindow* window,
    int creatureType,
    int goldCost,
    int resourceType,
    int resourceCost,
    int available
) {
    char monsterName[20];
    char label[40];
    tag_message message;

    strcpy(monsterName, GetMonsterName(creatureType));
    monsterName[0] -= 'a' - 'A';
    sprintf(label, "%s %s", "Recruit", monsterName);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = RECRUIT_TITLE_CONTROL;
    message.text = label;
    window->BroadcastMessage(message);

    sprintf(label, "%d", goldCost);
    message.id = RECRUIT_GOLD_COST_CONTROL;
    window->BroadcastMessage(message);
    if (resourceType != RECRUIT_NO_RESOURCE) {
        sprintf(label, "%d", resourceCost);
        message.id = RECRUIT_RESOURCE_COST_CONTROL;
        window->BroadcastMessage(message);
    }

    sprintf(gText, "%s%d", "Available: ", available);
    message.id = RECRUIT_AVAILABLE_CONTROL;
    message.text = gText;
    window->BroadcastMessage(message);

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = RECRUIT_CREATURE_CONTROL;
    message.value = creatureType;
    window->BroadcastMessage(message);
    if (resourceType != RECRUIT_NO_RESOURCE) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = RECRUIT_RESOURCE_ICON_CONTROL;
        message.value = resourceType;
        window->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = RECRUIT_RESOURCE_IMAGE_CONTROL;
        window->BroadcastMessage(message);
    }
}

// Buka RECRUIT.cpp:114-178; HoMM1 has no saved recruit menu.
VA(0x00401cc4, 0x282)
short recruitUnit::Open(short priority) {
    int resourceMaximum;
    int goldMaximum;

    m_window = new heroWindow(
        RECRUIT_WINDOW_X,
        RECRUIT_WINDOW_Y,
        const_cast<char*>(m_resourceType == RECRUIT_NO_RESOURCE ? "recruit0.bin" : "recruit1.bin")
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
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_SET_FLAGS,
        RECRUIT_CLOSE_CONTROL,
        RECRUIT_WIDGET_FLAGS_DIMMED
    );
    gpWindowManager->AddWindow(m_window, -1, 1);

    goldMaximum = gpCurPlayer->m_resources[RECRUIT_GOLD_RESOURCE] / m_goldCost;
    if (m_resourceType != RECRUIT_NO_RESOURCE) {
        resourceMaximum = gpCurPlayer->m_resources[m_resourceType] / m_resourceCost;
        m_maximum = goldMaximum < resourceMaximum ? goldMaximum : resourceMaximum;
    } else
        m_maximum = goldMaximum;
    if (*m_available < m_maximum)
        m_maximum = *m_available;
    m_recruited = 0;
    m_noRoom = 0;
    if (*m_available == 0) {
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            RECRUIT_CONFIRM_CONTROL,
            RECRUIT_WIDGET_FLAG_ENABLED
        );
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            RECRUIT_CONFIRM_CONTROL,
            RECRUIT_WIDGET_FLAGS_DIMMED
        );
    }
    KBChangeMenu(hmnuDflt);
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "recruitManager");
    return RECRUIT_MANAGER_OPEN_OK;
}

// Buka RECRUIT.cpp:180-204; HoMM1 refreshes town strips whenever a town
// recruit succeeded.
VA(0x00401f46, 0xd1)
void recruitUnit::Close(void) {
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
    if (m_noRoom)
        NormalDialog(
            "There is no room in the garrison for this army.",
            1,
            RECRUIT_NO_ROOM_DIALOG_X,
            RECRUIT_NO_ROOM_DIALOG_Y,
            -1,
            0,
            -1,
            0,
            -1
        );
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_CLEAR_FLAGS,
        RECRUIT_CLOSE_CONTROL,
        RECRUIT_WIDGET_FLAGS_DIMMED
    );
    if (m_sourceType == RECRUIT_SOURCE_TOWN && m_recruited) {
        gpTownManager->ResetStrips();
        gpTownManager->m_bankBox->Update();
    }
    m_active = 0;
}

// Buka RECRUIT.cpp:206-232. Retail reserves an unreferenced 20-byte text
// buffer above the message; the strings are formatted into gText.
VA(0x00402017, 0x127)
void recruitUnit::Update(void) {
    char text[20];
    tag_message message;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    sprintf(gText, "%s%d", "Available: ", *m_available);
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
    if (m_resourceType != RECRUIT_NO_RESOURCE) {
        m_resourceTotal = m_quantity * m_resourceCost;
        sprintf(gText, "%d", m_resourceTotal);
        message.id = RECRUIT_RESOURCE_TOTAL_CONTROL;
        m_window->BroadcastMessage(message);
    }
}

// Buka RECRUIT.cpp:234-378; HoMM1 handles quantity edits on select and
// the buttons on deselect, redrawing through a zero MoveWindow.
VA(0x0040213e, 0x3e5)
short recruitUnit::Main(struct tag_message& message) {
    int done;
    // Buka's unreferenced cost local; retail reserves its frame word.
    int cost;
    signed char quickView;

    done = 0;
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickView = 1;
    else
        quickView = 0;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case RECRUIT_NOTIFY_SELECT:
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
                        message.command = RECRUIT_COMMAND_GET_TEXT;
                        m_window->BroadcastMessage(message);
                        m_quantity = atoi(message.text);
                        if (m_quantity < 0)
                            m_quantity = 0;
                        if (m_quantity > m_maximum)
                            m_quantity = m_maximum;
                        break;
                    case RECRUIT_CREATURE_CONTROL:
                        gpGame->ViewArmy(
                            RECRUIT_VIEW_ARMY_X,
                            RECRUIT_VIEW_ARMY_Y,
                            m_creatureType,
                            0,
                            NULL,
                            1,
                            0,
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
                        done = 1;
                        break;
                    case RECRUIT_CONFIRM_CONTROL:
                        if (quickView)
                            break;
                        if (m_quantity == 0) {
                            done = 1;
                            goto checkClose;
                        }
                        if (m_army->CanJoin(m_creatureType)) {
                            m_army->Add(m_creatureType, m_quantity, -1);
                        } else {
                            done = 1;
                            m_noRoom = 1;
                            goto checkClose;
                        }
                        gpCurPlayer->m_resources[RECRUIT_GOLD_RESOURCE] -= m_quantity * m_goldCost;
                        if (m_resourceType != RECRUIT_NO_RESOURCE)
                            gpCurPlayer->m_resources[m_resourceType] -= m_quantity * m_resourceCost;
                        *m_available -= m_quantity;
                        m_recruited = 1;
                        done = 1;
                        break;
                }
                break;
            default:
                break;
        }

    checkClose:
        if (done == 1) {
            message.type = MESSAGE_EXECUTIVE;
            message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka RECRUIT.cpp:380-398; HoMM1 stores the creature byte and has no
// refresh-town argument.
VA(0x00402523, 0xd6)
recruitUnit::recruitUnit(armyGroup* army, int creatureType, short* available) {
    int unitCosts[RECRUIT_RESOURCE_COUNT + 1];
    int i;

    m_sourceType = RECRUIT_SOURCE_EVENT;
    m_army = army;
    m_creatureType = creatureType;
    m_available = available;
    GetMonsterCost(m_creatureType, unitCosts);
    m_goldCost = unitCosts[RECRUIT_GOLD_RESOURCE];
    for (i = 0; i < RECRUIT_RESOURCE_COUNT; i++) {
        if (unitCosts[i])
            break;
    }
    if (i < RECRUIT_RESOURCE_COUNT) {
        m_resourceType = i;
        m_resourceCost = unitCosts[m_resourceType];
    } else {
        m_resourceType = RECRUIT_NO_RESOURCE;
        m_resourceCost = 0;
    }
}

VA(0x004025f9, 0xf4)
recruitUnit::recruitUnit(town* townData, signed char dwelling) {
    int unitCosts[RECRUIT_RESOURCE_COUNT + 1];
    int i;

    m_sourceType = RECRUIT_SOURCE_TOWN;
    m_army = &townData->m_army;
    m_creatureType = gDwellingType[townData->m_type][dwelling];
    m_available = &townData->m_garrison[dwelling];
    GetMonsterCost(m_creatureType, unitCosts);
    m_goldCost = unitCosts[RECRUIT_GOLD_RESOURCE];
    for (i = 0; i < RECRUIT_RESOURCE_COUNT; i++) {
        if (unitCosts[i])
            break;
    }
    if (i < RECRUIT_RESOURCE_COUNT) {
        m_resourceType = i;
        m_resourceCost = unitCosts[m_resourceType];
    } else {
        m_resourceType = RECRUIT_NO_RESOURCE;
        m_resourceCost = 0;
    }
}

// Buka RECRUIT.cpp:414-451; HoMM1 hides the pointer around the quick view.
VA(0x004026ed, 0x1b4)
void QuickViewRecruit(town* townData, signed char dwelling) {
    int iGoldCost;
    int avail;
    int resourcePrice;
    int resourceIndex;
    heroWindow* win;
    int resourceType;
    int iMonsterType;
    int unitCosts[RECRUIT_RESOURCE_COUNT + 1];

    iMonsterType = gDwellingType[townData->m_type][dwelling];
    avail = townData->m_garrison[dwelling];
    GetMonsterCost(iMonsterType, unitCosts);
    iGoldCost = unitCosts[RECRUIT_GOLD_RESOURCE];
    for (resourceIndex = 0; resourceIndex < RECRUIT_RESOURCE_COUNT; resourceIndex++) {
        if (unitCosts[resourceIndex])
            break;
    }
    if (resourceIndex < RECRUIT_RESOURCE_COUNT) {
        resourceType = resourceIndex;
        resourcePrice = unitCosts[resourceType];
    } else {
        resourceType = RECRUIT_NO_RESOURCE;
        resourcePrice = 0;
    }

    win = new heroWindow(
        RECRUIT_WINDOW_X,
        RECRUIT_WINDOW_Y,
        const_cast<char*>(resourceType == RECRUIT_NO_RESOURCE ? "recruiq0.bin" : "recruiq1.bin")
    );
    if (win == NULL)
        MemError();
    SetupRecruitWin(win, iMonsterType, iGoldCost, resourceType, resourcePrice, avail);
    gpMouseManager->ReallyHidePointer();
    gpWindowManager->AddWindow(win, -1, 1);
    QuickViewWait();
    gpWindowManager->RemoveWindow(win);
    gpMouseManager->ReallyShowPointer();
}
