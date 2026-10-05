// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/border.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>
#include <BASE/resourceManager.h>
#include <BASE/widget.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/bankBox.h>
#include <SOURCE/KB.h>
#include <SOURCE/playerData.h>
#include <SOURCE/strip.h>

#include <stdio.h>

// donor PoL RVA 0x00032230; preferred Buka symbol ??0strip@@QAE@HHHKHPAVarmyGroup@@HHH@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.710816;margin=0.058489;shape=0.438;size=0.937;calls=0.800;strings=strip.icn;alternate=pol20:void strip::constructor(int, int, int, unsigned long int, int, class armyGroup *, int, int, int)@0x00032230
VA(0x0045c6b0, 0x290)
strip::strip(
    i16 x,
    i16 y,
    i8 stripType,
    i16 portraitId,
    i8 portraitFrame,
    class armyGroup* army,
    i16 firstBorderId,
    i32 drawWindow
) {
    i16 i;

    m_selectedSlot = STRIP_SLOT_NONE;
    m_x = x;
    m_y = y;
    m_stripType = stripType;
    m_portraitIcon = gpResourceManager->GetIcon(portraitId);
    m_portraitFrame = portraitFrame;
    m_army = army;
    m_stripIcon = gpResourceManager->GetIcon("strip.icn");
    m_monsterIcon = gpResourceManager->GetIcon("monsters.icn");
    m_font = gpResourceManager->GetFont("smalfont.fnt");
    m_window =
        new heroWindow(m_x, m_y, STRIP_WINDOW_WIDTH, STRIP_WINDOW_HEIGHT, WINDOW_FLAG_STRIP_WINDOW);
    if (!m_window)
        MemError();
    if (m_army) {
        m_borders[0] = new border(
            STRIP_PORTRAIT_X,
            STRIP_CONTENT_Y,
            STRIP_PORTRAIT_BORDER_WIDTH,
            STRIP_BORDER_HEIGHT,
            firstBorderId,
            WIDGET_KIND_TRANSPARENT,
            0,
            NULL
        );
        if (!m_borders[0])
            MemError();
        m_window->AddWidget(m_borders[0], WINDOW_Z_ORDER_APPEND);
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            m_borders[i + 1] = new border(
                i * STRIP_ARMY_X_STEP + STRIP_ARMY_FIRST_X,
                STRIP_CONTENT_Y,
                STRIP_ARMY_BORDER_WIDTH,
                STRIP_BORDER_HEIGHT,
                firstBorderId + i + 1,
                WIDGET_KIND_TRANSPARENT,
                0,
                NULL
            );
            if (!m_borders[i + 1])
                MemError();
            m_window->AddWidget(m_borders[i + 1], WINDOW_Z_ORDER_APPEND);
        }
    }
    DrawIcons(drawWindow);
    gpWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, drawWindow);
}

// donor PoL RVA 0x000324ae; preferred Buka symbol ??1strip@@QAE@XZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.560366;margin=0.278966;shape=0.418;size=0.916;calls=1.000;alternate=pol20:void strip::~destructor(void)@0x000324ae
VA(0x0045c940, 0x107)
strip::~strip() {
    i16 i;

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
VA(0x0045ca47, 0x37)
void strip::Draw(void) {
    DrawIcons(1);
    gpWindowManager->UpdateScreenRegion(m_x, m_y, STRIP_WINDOW_WIDTH, STRIP_WINDOW_HEIGHT);
}

// donor PoL RVA 0x00032632; preferred Buka symbol ?DrawIcons@strip@@QAEXH@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411442;margin=0.465209;shape=0.280;size=0.697;calls=0.714;alternate=pol20:void strip::DrawIcons(int)@0x00032632
VA(0x0045ca7e, 0x233)
void strip::DrawIcons(i8 drawWindow) {
    i16 i;
    i8 creatureType;

    m_portraitIcon->DrawToBuffer(
        m_x + STRIP_PORTRAIT_X,
        m_y + STRIP_CONTENT_Y,
        m_portraitFrame,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL
    );
    if (!m_army) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            m_stripIcon->DrawToBuffer(
                m_x + i * STRIP_ARMY_X_STEP + STRIP_ARMY_FIRST_X,
                m_y + STRIP_CONTENT_Y,
                STRIP_EMPTY_FRAME,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        m_window->DrawWindow(drawWindow);
        return;
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        creatureType = m_army->m_creatureTypes[i];
        if (creatureType != CREATURE_NONE) {
            m_stripIcon->DrawToBuffer(
                m_x + i * STRIP_ARMY_X_STEP + STRIP_ARMY_FIRST_X,
                m_y + STRIP_CONTENT_Y,
                creatureType / STRIP_CREATURES_PER_FACTION + STRIP_FACTION_FRAME_OFFSET,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            m_monsterIcon->DrawToBuffer(
                m_x + i * STRIP_ARMY_X_STEP + STRIP_MONSTER_X,
                m_y + STRIP_MONSTER_Y,
                creatureType,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            sprintf(gText, "%d", m_army->m_creatureCounts[i]);
            m_font->DrawBoundedString(
                gText,
                m_x + i * STRIP_ARMY_X_STEP + STRIP_ARMY_FIRST_X,
                m_y + STRIP_QUANTITY_Y,
                STRIP_QUANTITY_WIDTH,
                STRIP_QUANTITY_HEIGHT,
                1,
                FONT_ALIGN_RIGHT
            );
        } else {
            m_stripIcon->DrawToBuffer(
                m_x + i * STRIP_ARMY_X_STEP + STRIP_ARMY_FIRST_X,
                m_y + STRIP_CONTENT_Y,
                STRIP_EMPTY_FRAME,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        }
    }
    m_window->DrawWindow(drawWindow);
    if (m_selectedSlot != STRIP_SLOT_NONE)
        m_stripIcon->DrawToBuffer(
            m_x + m_selectedSlot * STRIP_ARMY_X_STEP + STRIP_ARMY_FIRST_X,
            m_y + STRIP_CONTENT_Y,
            STRIP_SELECTED_FRAME,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0045ccb1, 0x2c)
void strip::DrawFrame(void) {
    m_stripIcon
        ->DrawToBuffer(m_x, m_y, STRIP_BACKGROUND_FRAME, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
}

// donor PoL RVA 0x00032a38; preferred Buka symbol ??0bankBox@@QAE@HHPAVplayerData@@@Z
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.721149;margin=0.159729;shape=0.520;size=0.843;calls=0.833;strings=bankbox.bin;alternate=pol20:void bankBox::constructor(int, int, class playerData *)@0x00032a38
VA(0x0045ccdd, 0xd4)
bankBox::bankBox(i16 x, i16 y, class playerData* player) {
    m_player = player;
    m_x = x;
    m_y = y;
    m_window = new heroWindow(m_x, m_y, "bankbox.bin");
    if (!m_window)
        MemError();
    gpWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    Update();
}

// donor PoL RVA 0x00032aea; preferred Buka symbol ??1bankBox@@QAE@XZ
// donor Buka TU SOURCE/STRIP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.486146;margin=0.167932;shape=0.360;size=0.776;calls=1.000;alternate=pol20:void bankBox::~destructor(void)@0x00032aea
VA(0x0045cdb1, 0x34)
bankBox::~bankBox() {
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
}

VA(0x0045cde5, 0xba)
void bankBox::Update(void) {
    char text[12];
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < RESOURCE_NON_GOLD_END; i++) {
        sprintf(text, "%d", m_player->m_resources[i]);
        message.id = i + BANK_BOX_RESOURCE_FIRST;
        message.text = text;
        m_window->BroadcastMessage(message);
    }
    sprintf(text, "%d", m_player->m_resources[RESOURCE_GOLD]);
    message.id = BANK_BOX_GOLD;
    message.text = text;
    m_window->BroadcastMessage(message);
    m_window->DrawWindow();
}
