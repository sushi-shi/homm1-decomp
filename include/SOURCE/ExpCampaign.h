#ifndef HOMM1_SOURCE_EXPCAMPAIGN_H
#define HOMM1_SOURCE_EXPCAMPAIGN_H

#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class ExpCampaign {
public:
    // --- constructors ---
    ExpCampaign(void);
    ~ExpCampaign();
    // --- methods ---
    void ResetMapChoices(void);
    void ResetMapsPlayed(void);
    void ResetAwards(void);
    void ResetBonusChoices(void);
    void GrantAward(i32 award);
    void RemoveAward(i32 award);
    i8 HasAward(i32 award);
    void SetMapWasPlayed(void);
    void InitNewCampaign(i32 campaignId);
    void InitMap(void);
    void ShowInfo(i32, i32);
    void UpdateInfo(i32 redraw);
    i32 HandleVictory(void);
    void HandleVictory1(void);
    void HandleVictory2(void);
    void HandleVictory3(void);
    void HandleVictory4(void);
    void ReplaySmacker(void);
    void ReplaySmacker1(void);
    void ReplaySmacker2(void);
    void ReplaySmacker3(void);
    void ReplaySmacker4(void);
    u8 IsCompleted(void);
    i8 IsThisMapCompleted(void);

private:
    static i32
    MessageHandler(struct tag_message& message); // ?...@ExpCampaign@@CIH... (private static)
public:
    void Autosave(void);
    i32 Choose(void);
    i16 Days(void);
    i32 CampaignID(void);
    char* JosephName(void);
    char* IvanName(void);
    i8 IsSpecialGoldenBow(i32 x, i32 y);
    i8 IsSpecialUA(void);
    i8 IsSpecialLossCondition(i32 playerIndex);
};
#endif // HOMM1_SOURCE_EXPCAMPAIGN_H
