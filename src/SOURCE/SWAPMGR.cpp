// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>

#include <stdio.h>
#include <string.h>

// donor PoL RVA 0x00054502; preferred Buka symbol ?Open@swapManager@@UAEHH@Z
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.683274;margin=0.416085;shape=0.517;size=0.729;calls=0.800;strings=port%04d.icn|swapManager|swapbtn.icn;alternate=pol20:int swapManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00054502
VA(0x0046ecb0, 0x75)
swapManager::swapManager(void) {
    m_window = 0;
    m_selectorIcon = 0;
    m_selectedSide = -1;
    m_targetSide = -1;
    m_itemType = -1;
    m_selectedSlot = -1;
    m_targetSlot = -1;
    m_leftHero = 0;
    m_rightHero = 0;
}

VA(0x0046ed25, 0x3e)
swapManager::swapManager(class hero* leftHero, class hero* rightHero) {
    m_leftHero = leftHero;
    m_rightHero = rightHero;
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
    message.payload.widget.command = WIDGET_COMMAND_SET_ICON;
    sprintf(gText, "port%04d.icn", m_leftHero->m_unknown1d);
    message.payload.widget.id = 65;
    message.payload.widget.data.text = gText;
    m_window->BroadcastMessage(message);
    sprintf(gText, "port%04d.icn", m_rightHero->m_unknown1d);
    message.payload.widget.id = 66;
    m_window->BroadcastMessage(message);
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    sprintf(gText, "%s meets %s", m_leftHero->m_name, m_rightHero->m_name);
    message.payload.widget.data.text = gText;
    message.payload.widget.id = 77;
    m_window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.payload.widget.data.value = 2;
    message.payload.widget.id = 1;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 2;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 3;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 4;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 5;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 6;
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
    message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
    message.payload.widget.data.value = 2;
    message.payload.widget.id = 1;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 2;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 3;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 4;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 5;
    gpAdvManager->m_adventureWindow->BroadcastMessage(message);
    message.payload.widget.id = 6;
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
short swapManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x000556d3; preferred Buka symbol ?ViewMon@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.509716;margin=0.520416;shape=0.289;size=0.921;calls=1.000;alternate=pol20:void swapManager::ViewMon(void)@0x000556d3
VA(0x0046fd73, 0xa5)
void swapManager::ViewMon(void) {}

// donor PoL RVA 0x00055b42; preferred Buka symbol ?Update@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.575258;margin=0.321463;shape=0.441;size=0.938;calls=0.900;alternate=pol20:void swapManager::Update(void)@0x00055b42
VA(0x004701b8, 0x492)
void swapManager::Update(void) {}

// donor PoL RVA 0x00055fbd; preferred Buka symbol ?SplitMons@swapManager@@QAEXXZ
// donor Buka TU SOURCE/SWAPMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.761691;margin=0.047739;shape=0.487;size=0.960;calls=1.000;strings=splitwin.bin;alternate=pol20:void swapManager::SplitMons(void)@0x00055fbd
VA(0x0047064a, 0x3a6)
void swapManager::SplitMons(void) {}
