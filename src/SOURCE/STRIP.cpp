// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>

// donor PoL RVA 0x00032230; preferred Buka symbol ??0strip@@QAE@HHHKHPAVarmyGroup@@HHH@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.710816;margin=0.058489;shape=0.438;size=0.937;calls=0.800;strings=strip.icn;alternate=pol20:void strip::constructor(int, int, int, unsigned long int, int, class armyGroup *, int, int, int)@0x00032230
VA(0x00463630, 0x2de)
strip::strip(short x, short y, signed char stripType, short portraitId,
             signed char portraitFrame, class armyGroup* army, short firstBorderId,
             int drawWindow) {
    short i;

    m_selectedSlot = -1;
    m_x = x;
    m_y = y;
    m_stripType = stripType;
    m_portraitIcon = gpResourceManager->GetIcon(portraitId);
    m_portraitFrame = portraitFrame;
    m_army = army;
    m_stripIcon = gpResourceManager->GetIcon("strip.icn");
    m_monsterIcon = gpResourceManager->GetIcon("monsters.icn");
    m_font = gpResourceManager->GetFont("smalfont.fnt");
    m_window = new heroWindow(m_x, m_y, 552, 105, 8);
    if (!m_window)
        MemError();
    if (m_army) {
        m_borders[0] = new border(5, 6, 101, 93, firstBorderId, 1, 0, 0);
        if (!m_borders[0])
            MemError();
        m_window->AddWidget(m_borders[0], -1);
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            m_borders[i + 1] = new border(i * 88 + 112, 6, 82, 93, i + firstBorderId + 1, 1, 0, 0);
            if (!m_borders[i + 1])
                MemError();
            m_window->AddWidget(m_borders[i + 1], -1);
        }
    }
    DrawIcons(drawWindow);
    gpWindowManager->AddWindow(m_window, -1, drawWindow);
}

// donor PoL RVA 0x000324ae; preferred Buka symbol ??1strip@@QAE@XZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.560366;margin=0.278966;shape=0.418;size=0.916;calls=1.000;alternate=pol20:void strip::~destructor(void)@0x000324ae
VA(0x0046390e, 0x112)
strip::~strip() {
    short i;

    gpWindowManager->RemoveWindow(m_window);
    if (m_army) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            delete m_borders[i];
        delete m_borders[ARMY_GROUP_SLOT_COUNT];
    }
    delete m_window;
    gpResourceManager->Dispose(m_font);
    gpResourceManager->Dispose(m_stripIcon);
    gpResourceManager->Dispose(m_monsterIcon);
    gpResourceManager->Dispose(m_portraitIcon);
}

// donor PoL RVA 0x000325f2; preferred Buka symbol ?Draw@strip@@QAEXXZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.508282;margin=0.688808;shape=0.381;size=0.803;calls=1.000;alternate=pol20:void strip::Draw(void)@0x000325f2
VA(0x00463a20, 0x42)
void strip::Draw(void) {
    DrawIcons(1);
    gpWindowManager->UpdateScreenRegion(m_x, m_y, 552, 105);
}

// donor PoL RVA 0x00032632; preferred Buka symbol ?DrawIcons@strip@@QAEXH@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411442;margin=0.465209;shape=0.280;size=0.697;calls=0.714;alternate=pol20:void strip::DrawIcons(int)@0x00032632
VA(0x00463a62, 0x26e)
void strip::DrawIcons(signed char drawWindow) {
    short i;
    signed char creatureType;

    m_portraitIcon->DrawToBuffer(m_x + 5, m_y + 6, m_portraitFrame, 0, 0);
    if (!m_army) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            m_stripIcon->DrawToBuffer(m_x + i * 88 + 112, m_y + 6, 2, 0, 0);
        m_window->DrawWindow(drawWindow);
        return;
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        creatureType = m_army->m_creatureTypes[i];
        if (creatureType != -1) {
            m_stripIcon->DrawToBuffer(m_x + i * 88 + 112, m_y + 6, creatureType / 6 + 3, 0, 0);
            m_monsterIcon->DrawToBuffer(m_x + i * 88 + 119, m_y + 19, creatureType, 0, 0);
            sprintf(gText, "%d", m_army->m_creatureCounts[i]);
            m_font->DrawBoundedString(gText, m_x + i * 88 + 112, m_y + 86, 77, 13, 1, 2);
        } else {
            m_stripIcon->DrawToBuffer(m_x + i * 88 + 112, m_y + 6, 2, 0, 0);
        }
    }
    m_window->DrawWindow(drawWindow);
    if (m_selectedSlot != -1)
        m_stripIcon->DrawToBuffer(m_x + m_selectedSlot * 88 + 112, m_y + 6, 1, 0, 0);
}

VA(0x00463cd0, 0x37)
void strip::DrawFrame(void) {
    m_stripIcon->DrawToBuffer(m_x, m_y, 0, 0, 0);
}

// donor PoL RVA 0x00032a38; preferred Buka symbol ??0bankBox@@QAE@HHPAVplayerData@@@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.721149;margin=0.159729;shape=0.520;size=0.843;calls=0.833;strings=bankbox.bin;alternate=pol20:void bankBox::constructor(int, int, class playerData *)@0x00032a38
VA(0x00463d07, 0xfe)
bankBox::bankBox(short x, short y, class playerData* player) {
    m_player = player;
    m_x = x;
    m_y = y;
    m_window = new heroWindow(m_x, m_y, "bankbox.bin");
    if (!m_window)
        MemError();
    gpWindowManager->AddWindow(m_window, -1, 1);
    Update();
}

// donor PoL RVA 0x00032aea; preferred Buka symbol ??1bankBox@@QAE@XZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.486146;margin=0.167932;shape=0.360;size=0.776;calls=1.000;alternate=pol20:void bankBox::~destructor(void)@0x00032aea
VA(0x00463e05, 0x43)
bankBox::~bankBox() {
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
}

VA(0x00463e48, 0xc5)
void bankBox::Update(void) {
    char text[12];
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < PLAYER_RESOURCE_COUNT - 1; i++) {
        sprintf(text, "%d", m_player->m_resources[i]);
        message.payload.widget.id = i + 30;
        message.payload.widget.data.text = text;
        m_window->BroadcastMessage(message);
    }
    sprintf(text, "%d", m_player->m_resources[PLAYER_RESOURCE_COUNT - 1]);
    message.payload.widget.id = 36;
    message.payload.widget.data.text = text;
    m_window->BroadcastMessage(message);
    m_window->DrawWindow();
}
