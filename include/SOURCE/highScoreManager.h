#ifndef HOMM1_SOURCE_HIGHSCOREMANAGER_H
#define HOMM1_SOURCE_HIGHSCOREMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class heroWindow;

// clang-format off
// hiscore.bin control ids (Buka HighScoreControlId; HoMM1 numbers the title
// pair 0x67/0x68, ten animated monsters 201..210 and four text columns per
// row: name, scenario, score, rating).
H1_ENUM_BEGIN(HighScoreControlId)
    HIGH_SCORE_CLOSE_BUTTON          = DIALOG_BUTTON_0,
    HIGH_SCORE_STANDARD_BUTTON       = 100,
    HIGH_SCORE_TITLE_WIDGET          = 0x67,
    HIGH_SCORE_SUBTITLE_WIDGET       = 0x68,
    HIGH_SCORE_FIRST_TEXT_WIDGET     = 0x6a,
    HIGH_SCORE_CAMPAIGN_BUTTON       = 0x93,
    HIGH_SCORE_ANIMATED_WIDGET_FIRST = 200,
    HIGH_SCORE_FIRST_MONSTER_WIDGET  = 0xc9,
    HIGH_SCORE_ANIMATED_WIDGET_LAST  = 210
H1_ENUM_END(HighScoreControlId)

// Text widget columns of one score row (Update's id stride and offsets).
H1_ENUM_CONST_BEGIN(HighScoreTextColumn)
    HIGH_SCORE_TEXT_NAME_OFFSET     = 0,
    HIGH_SCORE_TEXT_SCENARIO_OFFSET = 1,
    HIGH_SCORE_TEXT_SCORE_OFFSET    = 2,
    HIGH_SCORE_TEXT_RATING_OFFSET   = 3,
    HIGH_SCORE_TEXT_WIDGET_STRIDE   = 4
H1_ENUM_CONST_END(HighScoreTextColumn)

H1_ENUM_CONST_BEGIN(HighScoreManagerConstant)
    HIGH_SCORE_DISPLAY_ENTRY_COUNT = 10,
    HIGH_SCORE_FILENAME_LENGTH     = 350,
    // Main tests message.m_type against the mask Buka's managers share.
    HIGH_SCORE_DISPATCH_MASK       = 0x32f
H1_ENUM_CONST_END(HighScoreManagerConstant)

// Monster animation (Buka HighScoreAnimationConstant; HoMM1 strides seven
// frames per rating monster and advances one frame every third tick).
H1_ENUM_CONST_BEGIN(HighScoreAnimationConstant)
    HIGH_SCORE_ANIMATION_FRAME_COUNT   = 18,
    HIGH_SCORE_ANIMATION_DELAY         = 120,
    HIGH_SCORE_TIMER_SLOT              = 0,
    HIGH_SCORE_ANIMATION_FRAME_DIVISOR = 3,
    HIGH_SCORE_MONSTER_FRAME_STRIDE    = 7
H1_ENUM_CONST_END(HighScoreAnimationConstant)

// Title/subtitle frames of the hiscore.bin title icons.
H1_ENUM_BEGIN(HighScoreTitleFrame)
    HIGH_SCORE_STANDARD_TITLE_FRAME    = 7,
    HIGH_SCORE_CAMPAIGN_SUBTITLE_FRAME = 8,
    HIGH_SCORE_STANDARD_SUBTITLE_FRAME = 9,
    HIGH_SCORE_CAMPAIGN_TITLE_FRAME    = 12
H1_ENUM_END(HighScoreTitleFrame)

// Main's animation redraw rectangle.
H1_ENUM_CONST_BEGIN(HighScoreLayoutConstant)
    HIGH_SCORE_UPDATE_X      = 538,
    HIGH_SCORE_UPDATE_Y      = 54,
    HIGH_SCORE_UPDATE_WIDTH  = 50,
    HIGH_SCORE_UPDATE_HEIGHT = 400
H1_ENUM_CONST_END(HighScoreLayoutConstant)

// Text fill colours of the newest entry's row.
H1_ENUM_CONST_BEGIN(HighScoreColor)
    HIGH_SCORE_HIGHLIGHT_COLOR = -65,
    HIGH_SCORE_NORMAL_COLOR    = 1
H1_ENUM_CONST_END(HighScoreColor)
// clang-format on

#pragma pack(push, 1)
        class highScoreManager : public baseManager {
public:
    short m_animationFrames[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    short m_monsterTypes[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    signed char m_showCampaignScores;
    heroWindow* m_window;
    // HoMM1 Main tests this additional mask against message.m_type.
    short m_dispatchMask;
    // --- constructors ---
    highScoreManager(void);
    ~highScoreManager();
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Update(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_HIGHSCOREMANAGER_H
