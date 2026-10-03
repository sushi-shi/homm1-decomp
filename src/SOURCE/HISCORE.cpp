// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/widget.h>
#include <BASE/WINMGR_TYPES.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

#include <fcntl.h>
#include <io.h>
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
        if (!giHighScoreType)
            m_showCampaignScores = 1;
        else
            m_showCampaignScores = 0;
    }
}

// HoMM1 keeps an empty destructor; it only restores this class's vtable.
VA(0x004010a0, 0x1f)
highScoreManager::~highScoreManager() {}

// donor PoL RVA 0x00089a96; preferred Buka symbol ?Open@highScoreManager@@UAEHH@Z
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.779726;margin=0.242262;shape=0.537;size=0.989;calls=0.923;strings=highScoreManager|hiscore.bin;alternate=pol20:int highScoreManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x00089a96
VA(0x004010bf, 0x169)
short highScoreManager::Open(short id) {
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    sprintf(gText, "hiscore.bmp");
    gpResourceManager->GetBackdrop(gText, gpWindowManager->m_screen);
    m_window = new heroWindow(0, 0, "hiscore.bin");
    if (m_window == NULL)
        MemError();
    Update();
    gpWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "highScoreManager");
    KBChangeMenu(hmnuDflt);
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
    glTimers[static_cast<int>(HIGH_SCORE_TIMER_SLOT)] =
        KBTickCount() + static_cast<int>(HIGH_SCORE_ANIMATION_DELAY);
    return static_cast<short>(BASE_MANAGER_SUCCESS);
}

// Buka HISCORE.cpp:51-56; retail window owner is +0x59, active is +0x2e.
VA(0x00401228, 0x5d)
void highScoreManager::Close(void) {
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
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

    if (glTimers[static_cast<int>(HIGH_SCORE_TIMER_SLOT)] < KBTickCount()) {
        glTimers[static_cast<int>(HIGH_SCORE_TIMER_SLOT)] =
            KBTickCount() + HIGH_SCORE_ANIMATION_DELAY;
        for (rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++) {
            m_animationFrames[rank] =
                (m_animationFrames[rank] + 1) % HIGH_SCORE_ANIMATION_FRAME_COUNT;
            windowMessage.type = MESSAGE_WIDGET;
            windowMessage.id = rank + HIGH_SCORE_FIRST_MONSTER_WIDGET;
            windowMessage.command = WIDGET_COMMAND_SET_FRAME;
            windowMessage.value =
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
    if ((message.modifiers & static_cast<int>(MESSAGE_MODIFIER_RIGHT_BUTTON)))
        return MESSAGE_DISPATCH_CONSUME;

    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case HIGH_SCORE_STANDARD_BUTTON:
                        case HIGH_SCORE_CAMPAIGN_BUTTON:
                            m_showCampaignScores = 1 - m_showCampaignScores;
                            Update();
                            m_window->DrawWindow(1);
                            break;
                        case HIGH_SCORE_CLOSE_BUTTON:
                            message.value = message.id;
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
        message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x00089e6a; preferred Buka symbol ?Update@highScoreManager@@QAEXXZ
// donor Buka TU SOURCE/HISCORE; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.683474;margin=0.421632;shape=0.404;size=0.957;calls=0.730;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:void highScoreManager::Update(void)@0x00089e6a
// Buka HISCORE.cpp:121-283; HoMM1 reads 0x57-byte records, names the
// rating creature directly and highlights the new entry by fill colour.
VA(0x004014ee, 0x667)
void highScoreManager::Update(void) {
    signed char bNoFile;
    char fileName[HIGH_SCORE_FILENAME_LENGTH];
    tag_message message;
    HighScoreEntry record;
    int handle;
    int i;
    extern char gcDataPath[];

    bNoFile = 0;
    if (m_showCampaignScores)
        sprintf(fileName, "%sCAMPAIGN.HS", gcDataPath);
    else
        sprintf(fileName, "%sSTANDARD.HS", gcDataPath);
    handle = open(fileName, _O_BINARY);
    if (handle == -1)
        bNoFile = 1;

    sprintf(gText, "hiscore.bmp");
    gpResourceManager->GetBackdrop(gText, gpWindowManager->m_screen);

    message.type = MESSAGE_WIDGET;
    message.id = HIGH_SCORE_TITLE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_showCampaignScores)
        message.value = HIGH_SCORE_CAMPAIGN_TITLE_FRAME;
    else
        message.value = HIGH_SCORE_STANDARD_TITLE_FRAME;
    m_window->BroadcastMessage(message);

    message.id = HIGH_SCORE_SUBTITLE_WIDGET;
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_showCampaignScores)
        message.value = HIGH_SCORE_CAMPAIGN_SUBTITLE_FRAME;
    else
        message.value = HIGH_SCORE_STANDARD_SUBTITLE_FRAME;
    m_window->BroadcastMessage(message);

    if (m_showCampaignScores)
        message.id = HIGH_SCORE_CAMPAIGN_BUTTON;
    else
        message.id = HIGH_SCORE_STANDARD_BUTTON;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_window->BroadcastMessage(message);

    if (m_showCampaignScores)
        message.id = HIGH_SCORE_STANDARD_BUTTON;
    else
        message.id = HIGH_SCORE_CAMPAIGN_BUTTON;
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_window->BroadcastMessage(message);

    for (i = 0; i < HIGH_SCORE_DISPLAY_ENTRY_COUNT; i++) {
        if (bNoFile)
            record.score = HIGH_SCORE_EMPTY;
        else
            read(handle, &record, sizeof(record));

        if (record.score == HIGH_SCORE_EMPTY) {
            m_monsterTypes[i] = 0;
            sprintf(gText, "");
        } else {
            m_monsterTypes[i] =
                GetMonType(record.score, static_cast<signed char>(!m_showCampaignScores));
        }

        message.id = i + HIGH_SCORE_FIRST_MONSTER_WIDGET;
        if (record.score == HIGH_SCORE_EMPTY)
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        else
            message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_window->BroadcastMessage(message);

        if (record.score != HIGH_SCORE_EMPTY) {
            m_animationFrames[i] = (m_animationFrames[i] + 1) % HIGH_SCORE_ANIMATION_FRAME_COUNT;
            message.id = i + HIGH_SCORE_FIRST_MONSTER_WIDGET;
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_monsterTypes[i] * HIGH_SCORE_MONSTER_FRAME_STRIDE
                            + m_animationFrames[i] / HIGH_SCORE_ANIMATION_FRAME_DIVISOR;
            m_window->BroadcastMessage(message);
        }

        message.command = WIDGET_COMMAND_SET_TEXT;
        message.text = gText;
        message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET;
        if (record.score != HIGH_SCORE_EMPTY)
            sprintf(gText, record.playerName);
        m_window->BroadcastMessage(message);

        message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                     + HIGH_SCORE_TEXT_SCENARIO_OFFSET;
        if (record.score != HIGH_SCORE_EMPTY)
            sprintf(gText, record.scenarioName);
        m_window->BroadcastMessage(message);

        message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                     + HIGH_SCORE_TEXT_SCORE_OFFSET;
        if (record.score != HIGH_SCORE_EMPTY)
            sprintf(gText, "%d", record.score);
        m_window->BroadcastMessage(message);

        message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                     + HIGH_SCORE_TEXT_RATING_OFFSET;
        if (record.score != HIGH_SCORE_EMPTY) {
            sprintf(gText, "%s", gArmyNames[m_monsterTypes[i]]);
            gText[0] -= 'a' - 'A';
        }
        m_window->BroadcastMessage(message);

        if (giHighScoreRank == i) {
            if ((m_showCampaignScores && !giHighScoreType)
                || (!m_showCampaignScores && giHighScoreType)) {
                message.command = WIDGET_COMMAND_SET_COLOR;
                message.value = HIGH_SCORE_HIGHLIGHT_COLOR;
            } else {
                message.command = WIDGET_COMMAND_SET_COLOR;
                message.value = HIGH_SCORE_NORMAL_COLOR;
            }
            message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET;
            m_window->BroadcastMessage(message);
            message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                         + HIGH_SCORE_TEXT_SCENARIO_OFFSET;
            m_window->BroadcastMessage(message);
            message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                         + HIGH_SCORE_TEXT_SCORE_OFFSET;
            m_window->BroadcastMessage(message);
            message.id = i * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                         + HIGH_SCORE_TEXT_RATING_OFFSET;
            m_window->BroadcastMessage(message);
        }
    }
    if (!bNoFile)
        close(handle);
}
