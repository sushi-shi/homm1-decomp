#ifndef HOMM1_SOURCE_HIGHSCOREMANAGER_H
#define HOMM1_SOURCE_HIGHSCOREMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class heroWindow;

H1_ENUM_BEGIN(HighScoreManagerConstant)
    HIGH_SCORE_DISPLAY_ENTRY_COUNT = 10,
    HIGH_SCORE_ANIMATION_FRAME_COUNT = 18,
    HIGH_SCORE_DISPATCH_MASK = 0x32f,
    HIGH_SCORE_FADE_OUT = 1,
    HIGH_SCORE_FADE_STEPS = 8
H1_ENUM_END(HighScoreManagerConstant)

#pragma pack(push, 1)
class highScoreManager : public baseManager {
public:
    short m_animationFrames[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    short m_monsterTypes[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    signed char m_showCampaignScores;
    heroWindow *m_window;
    // HoMM1 Main tests this additional mask against message.m_type.
    short m_dispatchMask;
    // --- constructors ---
    highScoreManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Update(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_HIGHSCOREMANAGER_H
