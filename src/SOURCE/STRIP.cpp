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

VA(0x0045c6b0, 0x290)
strip::strip(
    i16 x,
    i16 y,
    i8 stripType,
    i16 portraitIconId,
    i8 portraitFrame,
    class armyGroup* troops,
    i16 firstBorderId,
    i32 drawWindow
) {
    i16 i;

    m_selectedSlot = STRIP_SLOT_NONE;
    m_x = x;
    m_y = y;
    m_unusedStripType = stripType;
    m_portraitIcon = gResourceManager->GetIcon(portraitIconId);
    m_portraitFrame = portraitFrame;
    m_army = troops;
    m_stripIcon = gResourceManager->GetIcon("strip.icn");
    m_monsterIcon = gResourceManager->GetIcon("monsters.icn");
    m_font = gResourceManager->GetFont("smalfont.fnt");
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
    gWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, drawWindow);
}

VA(0x0045c940, 0x107)
strip::~strip() {
    i16 i;

    gWindowManager->RemoveWindow(m_window);
    if (m_army) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            delete m_borders[i];
        delete m_borders[ARMY_GROUP_SLOT_COUNT];
    }
    delete m_window;
    gResourceManager->Dispose(m_font);
    gResourceManager->Dispose(m_stripIcon);
    gResourceManager->Dispose(m_monsterIcon);
    gResourceManager->Dispose(m_portraitIcon);
}

VA(0x0045ca47, 0x37)
void strip::Draw(void) {
    DrawIcons(1);
    gWindowManager->UpdateScreenRegion(m_x, m_y, STRIP_WINDOW_WIDTH, STRIP_WINDOW_HEIGHT);
}

VA(0x0045ca7e, 0x233)
void strip::DrawIcons(i8 drawWindow) {
    i16 i;
    H1_ENUM_LOCAL(CreatureType, i8) creatureType;

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
                CREATURE_FACTION(creatureType) + STRIP_FACTION_FRAME_OFFSET,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            m_monsterIcon->DrawToBuffer(
                m_x + i * STRIP_ARMY_X_STEP + STRIP_MONSTER_X,
                m_y + STRIP_MONSTER_Y,
                H1_ENUM_ENCODE(CreatureType, creatureType),
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

VA(0x0045ccdd, 0xd4)
bankBox::bankBox(i16 x, i16 y, class playerData* player) {
    m_player = player;
    m_x = x;
    m_y = y;
    m_window = new heroWindow(m_x, m_y, "bankbox.bin");
    if (!m_window)
        MemError();
    gWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    Update();
}

VA(0x0045cdb1, 0x34)
bankBox::~bankBox() {
    gWindowManager->RemoveWindow(m_window);
    delete m_window;
}

VA(0x0045cde5, 0xba)
void bankBox::Update(void) {
    char text[12];
    tag_message message;
    H1_ENUM_LOCAL(ResourceType, i16) i;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = RESOURCE_FIRST; i < RESOURCE_NON_GOLD_END; i++) {
        sprintf(text, "%d", m_player->m_resources[i]);
        message.id = H1_ENUM_ENCODE(ResourceType, i) + BANK_BOX_RESOURCE_FIRST;
        message.text = text;
        m_window->BroadcastMessage(message);
    }
    sprintf(text, "%d", m_player->m_resources[RESOURCE_GOLD]);
    message.id = BANK_BOX_GOLD;
    message.text = text;
    m_window->BroadcastMessage(message);
    m_window->DrawWindow();
}
