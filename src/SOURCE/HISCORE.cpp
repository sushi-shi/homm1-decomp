#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/widget.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

VA(0x0043bd60, 0x7f)
highScoreManager::highScoreManager(void) {
    i32 rank;
    m_dispatchMask = HIGH_SCORE_DISPATCH_MASK;
    for (rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++)
        m_animationFrames[rank] = Random(0, HIGH_SCORE_ANIMATION_FRAME_COUNT - 1);
    m_showCampaignScores = 0;
    if (gShowHighScore)
        m_showCampaignScores = !gHighScoreType;
}

VA(0x0043bddf, 0x14)
highScoreManager::~highScoreManager() {}

VA(0x0043bdf3, 0x144)
H1_ENUM_RETURN(BaseManagerStatus, i16) highScoreManager::Open(i16 priority) {
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    sprintf(gText, "hiscore.bmp");
    gResourceManager->GetBackdrop(gText, gWindowManager->m_screen);
    m_window = new heroWindow(0, 0, "hiscore.bin");
    if (m_window == NULL)
        MemError();
    Update();
    gWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "highScoreManager");
    KBChangeMenu(gDefaultMenu);
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
    gTimers[HIGH_SCORE_TIMER_SLOT] = KBTickCount() + HIGH_SCORE_ANIMATION_DELAY;
    return H1_ENUM_CAST(BaseManagerStatus, i16, BASE_MANAGER_SUCCESS);
}

// The window owner is at +0x59, active at +0x2e.
VA(0x0043bf37, 0x4e)
void highScoreManager::Close(void) {
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    gWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
}

VA(0x0043bf85, 0x1fa)
H1_ENUM_RETURN(MessageDispatchResult, i16) highScoreManager::Main(struct tag_message& message) {
    b32 result;
    i32 rank;
    tag_message windowMessage;

    result = false;
    if (gShowHighScore != false)
        gShowHighScore = false;

    if (gTimers[HIGH_SCORE_TIMER_SLOT] < KBTickCount()) {
        gTimers[HIGH_SCORE_TIMER_SLOT] = KBTickCount() + HIGH_SCORE_ANIMATION_DELAY;
        for (rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++) {
            m_animationFrames[rank] =
                (m_animationFrames[rank] + 1) % HIGH_SCORE_ANIMATION_FRAME_COUNT;
            windowMessage.type = MESSAGE_WIDGET;
            windowMessage.id = rank + HIGH_SCORE_FIRST_MONSTER_WIDGET;
            windowMessage.command = WIDGET_COMMAND_SET_FRAME;
            windowMessage.value =
                H1_ENUM_ENCODE(CreatureType, m_monsterTypes[rank]) * HIGH_SCORE_MONSTER_FRAME_STRIDE
                + m_animationFrames[rank] / HIGH_SCORE_ANIMATION_FRAME_DIVISOR;
            m_window->BroadcastMessage(windowMessage);
        }
        m_window->DrawWindow(0, HIGH_SCORE_ANIMATED_WIDGET_FIRST, HIGH_SCORE_ANIMATED_WIDGET_LAST);
        gWindowManager->UpdateScreenRegion(
            HIGH_SCORE_UPDATE_X,
            HIGH_SCORE_UPDATE_Y,
            HIGH_SCORE_UPDATE_WIDTH,
            HIGH_SCORE_UPDATE_HEIGHT
        );
    }

    if (!(message.type & m_dispatchMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
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
                            result = true;
                            break;
                    }
                    break;
            }
            break;
        default:
            break;
    }

    if (result == true) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Update reads 0x57-byte records, names the rating creature directly and
// highlights the new entry by fill colour.
VA(0x0043c17f, 0x5ec)
void highScoreManager::Update(void) {
    HighScoreEntry highScore;
    i32 rank;
    i32 inputFile;
    b8 noScoreFile;
    tag_message hsMessage;
    char scorePath[HIGH_SCORE_FILENAME_LENGTH];

    noScoreFile = false;
    if (m_showCampaignScores)
        sprintf(scorePath, "%sCAMPAIGN.HS", gDataPath);
    else
        sprintf(scorePath, "%sSTANDARD.HS", gDataPath);
    inputFile = open(scorePath, _O_BINARY);
    if (inputFile == FILE_DESCRIPTOR_INVALID)
        noScoreFile = true;

    sprintf(gText, "hiscore.bmp");
    gResourceManager->GetBackdrop(gText, gWindowManager->m_screen);

    hsMessage.type = MESSAGE_WIDGET;
    hsMessage.id = HIGH_SCORE_TITLE_WIDGET;
    hsMessage.command = WIDGET_COMMAND_SET_FRAME;
    hsMessage.value = H1_ENUM_ENCODE(
        HighScoreTitleFrame,
        m_showCampaignScores ? HIGH_SCORE_CAMPAIGN_TITLE_FRAME : HIGH_SCORE_STANDARD_TITLE_FRAME
    );
    m_window->BroadcastMessage(hsMessage);

    hsMessage.id = HIGH_SCORE_SUBTITLE_WIDGET;
    hsMessage.command = WIDGET_COMMAND_SET_FRAME;
    hsMessage.value = H1_ENUM_ENCODE(
        HighScoreTitleFrame,
        m_showCampaignScores ? HIGH_SCORE_CAMPAIGN_SUBTITLE_FRAME
                             : HIGH_SCORE_STANDARD_SUBTITLE_FRAME
    );
    m_window->BroadcastMessage(hsMessage);

    hsMessage.id = m_showCampaignScores ? HIGH_SCORE_CAMPAIGN_BUTTON : HIGH_SCORE_STANDARD_BUTTON;
    hsMessage.command = WIDGET_COMMAND_CLEAR_FLAGS;
    hsMessage.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_window->BroadcastMessage(hsMessage);

    hsMessage.id = m_showCampaignScores ? HIGH_SCORE_STANDARD_BUTTON : HIGH_SCORE_CAMPAIGN_BUTTON;
    hsMessage.command = WIDGET_COMMAND_SET_FLAGS;
    hsMessage.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    m_window->BroadcastMessage(hsMessage);

    for (rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++) {
        if (noScoreFile)
            highScore.score = HIGH_SCORE_EMPTY;
        else
            READ_FILE_VALUE(inputFile, highScore);

        if (highScore.score == HIGH_SCORE_EMPTY) {
            m_monsterTypes[rank] = CREATURE_PEASANT;
            sprintf(gText, "");
        } else {
            m_monsterTypes[rank] = GetMonType(
                highScore.score,
                m_showCampaignScores ? H1_ENUM_CAST(HighScoreType, i8, HIGH_SCORE_TYPE_CAMPAIGN)
                                     : H1_ENUM_CAST(HighScoreType, i8, HIGH_SCORE_TYPE_STANDARD)
            );
        }

        hsMessage.id = rank + HIGH_SCORE_FIRST_MONSTER_WIDGET;
        hsMessage.command = highScore.score == HIGH_SCORE_EMPTY ? WIDGET_COMMAND_CLEAR_FLAGS
                                                                : WIDGET_COMMAND_SET_FLAGS;
        hsMessage.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_window->BroadcastMessage(hsMessage);

        if (highScore.score != HIGH_SCORE_EMPTY) {
            m_animationFrames[rank] =
                (m_animationFrames[rank] + 1) % HIGH_SCORE_ANIMATION_FRAME_COUNT;
            hsMessage.id = rank + HIGH_SCORE_FIRST_MONSTER_WIDGET;
            hsMessage.command = WIDGET_COMMAND_SET_FRAME;
            hsMessage.value =
                H1_ENUM_ENCODE(CreatureType, m_monsterTypes[rank]) * HIGH_SCORE_MONSTER_FRAME_STRIDE
                + m_animationFrames[rank] / HIGH_SCORE_ANIMATION_FRAME_DIVISOR;
            m_window->BroadcastMessage(hsMessage);
        }

        hsMessage.command = WIDGET_COMMAND_SET_TEXT;
        hsMessage.text = gText;
        hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET;
        if (highScore.score != HIGH_SCORE_EMPTY)
            sprintf(gText, highScore.playerName);
        m_window->BroadcastMessage(hsMessage);

        hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                       + HIGH_SCORE_TEXT_SCENARIO_OFFSET;
        if (highScore.score != HIGH_SCORE_EMPTY)
            sprintf(gText, highScore.scenarioName);
        m_window->BroadcastMessage(hsMessage);

        hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                       + HIGH_SCORE_TEXT_SCORE_OFFSET;
        if (highScore.score != HIGH_SCORE_EMPTY)
            sprintf(gText, "%d", highScore.score);
        m_window->BroadcastMessage(hsMessage);

        hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                       + HIGH_SCORE_TEXT_RATING_OFFSET;
        if (highScore.score != HIGH_SCORE_EMPTY) {
            sprintf(gText, "%s", gArmyNames[m_monsterTypes[rank]]);
            gText[0] = CyrillicToUpper(gText[0]);
        }
        m_window->BroadcastMessage(hsMessage);

        if (gHighScoreRank == rank) {
            if ((m_showCampaignScores && !gHighScoreType)
                || (!m_showCampaignScores && gHighScoreType)) {
                hsMessage.command = WIDGET_COMMAND_SET_COLOR;
                hsMessage.value = HIGH_SCORE_HIGHLIGHT_COLOR;
            } else {
                hsMessage.command = WIDGET_COMMAND_SET_COLOR;
                hsMessage.value = HIGH_SCORE_NORMAL_COLOR;
            }
            hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET;
            m_window->BroadcastMessage(hsMessage);
            hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                           + HIGH_SCORE_TEXT_SCENARIO_OFFSET;
            m_window->BroadcastMessage(hsMessage);
            hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                           + HIGH_SCORE_TEXT_SCORE_OFFSET;
            m_window->BroadcastMessage(hsMessage);
            hsMessage.id = rank * HIGH_SCORE_TEXT_WIDGET_STRIDE + HIGH_SCORE_FIRST_TEXT_WIDGET
                           + HIGH_SCORE_TEXT_RATING_OFFSET;
            m_window->BroadcastMessage(hsMessage);
        }
    }
    if (!noScoreFile)
        close(inputFile);
}
