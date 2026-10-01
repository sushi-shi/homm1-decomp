// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/resourceManager.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/kbwin.h>

#include <stdio.h>
#include <string.h>

// Buka HISCORE.cpp:22-29; HoMM1 adds the dispatch mask and score-type selection.
VA(0x00401000, 0xa0)
highScoreManager::highScoreManager(void) {
    int rank;
    m_dispatchMask = HIGH_SCORE_DISPATCH_MASK;
    for (rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++)
        m_animationFrames[rank] = Random(0, HIGH_SCORE_ANIMATION_FRAME_COUNT - 1);
    m_showCampaignScores = 0;
    if (gbShowHighScore) {
        if (!gbStandardHighScore)
            m_showCampaignScores = 1;
        else
            m_showCampaignScores = 0;
    }
}

// donor PoL RVA 0x00089a96; preferred Buka symbol ?Open@highScoreManager@@UAEHH@Z
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.779726;margin=0.242262;shape=0.537;size=0.989;calls=0.923;strings=highScoreManager|hiscore.bin;alternate=pol20:int highScoreManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00089a96
VA(0x004010bf, 0x169)
short highScoreManager::Open(short id) {
    gpWindowManager->FadeScreen(HIGH_SCORE_FADE_OUT, HIGH_SCORE_FADE_STEPS, 0);
    sprintf(gText, "hiscore.bmp");
    gpResourceManager->GetBackdrop(gText, gpWindowManager->m_screen);
    m_window = new heroWindow(0, 0, "hiscore.bin");
    if (m_window == 0)
        MemError();
    Update();
    gpWindowManager->AddWindow(m_window, -1, 1);
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "highScoreManager");
    KBChangeMenu(hmnuDflt);
    gpWindowManager->FadeScreen(HIGH_SCORE_FADE_IN, HIGH_SCORE_FADE_STEPS, 0);
    glTimers[static_cast<int>(HIGH_SCORE_TIMER_SLOT)] =
        KBTickCount() + static_cast<int>(HIGH_SCORE_ANIMATION_DELAY);
    return static_cast<short>(HIGH_SCORE_MANAGER_OPEN_OK);
}

// Buka HISCORE.cpp:51-56; retail window owner is +0x59, active is +0x2e.
VA(0x00401228, 0x5d)
void highScoreManager::Close(void) {
    gpWindowManager->FadeScreen(HIGH_SCORE_FADE_OUT, HIGH_SCORE_FADE_STEPS, NULL);
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
}

// donor PoL RVA 0x00089c40; preferred Buka symbol ?Main@highScoreManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479652;margin=0.177846;shape=0.336;size=0.742;calls=1.000;alternate=pol20:int highScoreManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00089c40
VA(0x00401285, 0x269)
short highScoreManager::Main(struct tag_message& message) {
    int retVal;
    int rank;
    tag_message windowMessage;

    retVal = 0;
    if (gbShowHighScore != 0)
        gbShowHighScore = 0;

    if (glTimers[HIGH_SCORE_TIMER_SLOT] < KBTickCount()) {
        glTimers[HIGH_SCORE_TIMER_SLOT] = KBTickCount() + HIGH_SCORE_ANIMATION_DELAY;
        for (rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++) {
            m_animationFrames[rank] =
                (m_animationFrames[rank] + 1) % HIGH_SCORE_ANIMATION_FRAME_COUNT;
            windowMessage.type = MESSAGE_WIDGET;
            windowMessage.payload.widget.id = rank + HIGH_SCORE_FIRST_MONSTER_WIDGET;
            windowMessage.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
            windowMessage.payload.widget.data.value =
                m_monsterTypes[rank] * HIGH_SCORE_MONSTER_FRAME_STRIDE
                + m_animationFrames[rank] / static_cast<int>(HIGH_SCORE_ANIMATION_FRAME_DIVISOR);
            m_window->BroadcastMessage(windowMessage);
        }
        m_window->DrawWindow(0, HIGH_SCORE_ANIMATED_WIDGET_FIRST, HIGH_SCORE_ANIMATED_WIDGET_LAST);
        gpWindowManager->UpdateScreenRegion(
            HIGH_SCORE_UPDATE_X,
            HIGH_SCORE_UPDATE_Y,
            HIGH_SCORE_UPDATE_WIDTH,
            HIGH_SCORE_UPDATE_HEIGHT
        );
    }

    if (!(m_dispatchMask & message.type)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if ((message.payload.mouse.modifiers & static_cast<int>(MESSAGE_MODIFIER_RIGHT_BUTTON)))
        return MESSAGE_DISPATCH_CONSUME;

    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.payload.widget.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.payload.widget.id) {
                        case HIGH_SCORE_STANDARD_BUTTON:
                        case HIGH_SCORE_CAMPAIGN_BUTTON:
                            m_showCampaignScores = 1 - m_showCampaignScores;
                            Update();
                            m_window->DrawWindow(1);
                            break;
                        case HIGH_SCORE_CLOSE_BUTTON:
                            message.payload.widget.data.value = message.payload.widget.id;
                            retVal = 1;
                            break;
                    }
                    break;
            }
            break;
        default:
            break;
    }

    if (retVal == 1) {
        message.type = MESSAGE_EXECUTIVE;
        message.payload.executive.command = EXECUTIVE_COMMAND_RETURN_RESULT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x00089e6a; preferred Buka symbol ?Update@highScoreManager@@QAEXXZ
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.683474;margin=0.421632;shape=0.404;size=0.957;calls=0.730;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:void highScoreManager::Update(void)@0x00089e6a
VA(0x004014ee, 0x672)
void highScoreManager::Update(void) {}
