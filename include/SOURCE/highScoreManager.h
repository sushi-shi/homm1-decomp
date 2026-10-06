#ifndef HOMM1_SOURCE_HIGHSCOREMANAGER_H
#define HOMM1_SOURCE_HIGHSCOREMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <BASE/message.h>
#include <SOURCE/creatureTypes.h>

struct tag_message;

class heroWindow;

enum HighScoreControlId {
HIGH_SCORE_CLOSE_BUTTON = DIALOG_BUTTON_0,
    HIGH_SCORE_STANDARD_BUTTON = 100, HIGH_SCORE_TITLE_WIDGET = 0x67,
    HIGH_SCORE_SUBTITLE_WIDGET = 0x68, HIGH_SCORE_FIRST_TEXT_WIDGET = 0x6a,
    HIGH_SCORE_CAMPAIGN_BUTTON = 0x93, HIGH_SCORE_ANIMATED_WIDGET_FIRST = 200,
    HIGH_SCORE_FIRST_MONSTER_WIDGET = 0xc9,
    HIGH_SCORE_ANIMATED_WIDGET_LAST = 210 };

    enum HighScoreTextColumn {
    HIGH_SCORE_TEXT_NAME_OFFSET = 0,
    HIGH_SCORE_TEXT_SCENARIO_OFFSET = 1,
    HIGH_SCORE_TEXT_SCORE_OFFSET = 2,
    HIGH_SCORE_TEXT_RATING_OFFSET = 3,
    HIGH_SCORE_TEXT_WIDGET_STRIDE = 4
};

enum HighScoreManagerConstant {
    HIGH_SCORE_DISPLAY_ENTRY_COUNT = 10,
    HIGH_SCORE_FILENAME_LENGTH = 350,
    HIGH_SCORE_DISPATCH_MASK = 0x32f
};

enum HighScoreAnimationConstant {
    HIGH_SCORE_ANIMATION_FRAME_COUNT = 18,
    HIGH_SCORE_ANIMATION_DELAY = 120,
    HIGH_SCORE_ANIMATION_FRAME_DIVISOR = 3,
    HIGH_SCORE_MONSTER_FRAME_STRIDE = 7
};

enum HighScoreTitleFrame {
    HIGH_SCORE_STANDARD_TITLE_FRAME = 7,
    HIGH_SCORE_CAMPAIGN_SUBTITLE_FRAME = 8,
    HIGH_SCORE_STANDARD_SUBTITLE_FRAME = 9,
    HIGH_SCORE_CAMPAIGN_TITLE_FRAME = 12
};

enum HighScoreLayoutConstant {
    HIGH_SCORE_UPDATE_X = 538,
    HIGH_SCORE_UPDATE_Y = 54,
    HIGH_SCORE_UPDATE_WIDTH = 50,
    HIGH_SCORE_UPDATE_HEIGHT = 400
};

enum HighScoreColor {
    HIGH_SCORE_HIGHLIGHT_COLOR = -65,
    HIGH_SCORE_NORMAL_COLOR = 1
};

class highScoreManager : public baseManager {
public:
    i16 m_animationFrames[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    i16 m_monsterTypes[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    i8 m_showCampaignScores;
    heroWindow* m_window;
    i16 m_dispatchMask;
    highScoreManager(void);
    ~highScoreManager();
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Update(void);
};

enum HighScoreType {
    HIGH_SCORE_TYPE_CAMPAIGN = 0,
    HIGH_SCORE_TYPE_STANDARD = 1
};

enum HighScoreRuntimeConstant {
    HIGH_SCORE_EMPTY = -1
};

#pragma pack(push, 1)
struct HighScoreEntry {
    char playerName[17];
    char scenarioName[15];
    i32 score;
    char unknown24[0x33];
};
#pragma pack(pop)

#endif
