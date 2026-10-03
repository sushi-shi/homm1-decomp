#ifndef HOMM1_SOURCE_EXPCAMPAIGN_H
#define HOMM1_SOURCE_EXPCAMPAIGN_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 36 methods, 0 own-virtual, 0 static data.

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
    void GrantAward(int award);
    void RemoveAward(int award);
    signed char HasAward(int award);
    void SetMapWasPlayed(void);
    void InitNewCampaign(int campaignId);
    void InitMap(void);
    void ShowInfo(int, int);
    void UpdateInfo(int redraw);
    int HandleVictory(void);
    void HandleVictory1(void);
    void HandleVictory2(void);
    void HandleVictory3(void);
    void HandleVictory4(void);
    void ReplaySmacker(void);
    void ReplaySmacker1(void);
    void ReplaySmacker2(void);
    void ReplaySmacker3(void);
    void ReplaySmacker4(void);
    unsigned char IsCompleted(void);
    signed char IsThisMapCompleted(void);
private:
    static int MessageHandler(struct tag_message & message);   // ?...@ExpCampaign@@CIH... (private static)
public:
    void Autosave(void);
    int Choose(void);
    short int Days(void);
    int CampaignID(void);
    char * JosephName(void);
    char * IvanName(void);
    signed char IsSpecialGoldenBow(int x, int y);
    signed char IsSpecialUA(void);
    signed char IsSpecialLossCondition(int playerIndex);
};
#endif // HOMM1_SOURCE_EXPCAMPAIGN_H
