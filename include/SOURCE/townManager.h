#ifndef HOMM1_SOURCE_TOWNMANAGER_H
#define HOMM1_SOURCE_TOWNMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 26 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class bankBox;
class heroWindow;
class icon;
class strip;
class town;
class townObject;
struct tag_message;

H1_ENUM_BEGIN(TownManagerStorageConstant)
TOWN_MANAGER_OBJECT_CAPACITY = 16, TOWN_MANAGER_STATUS_TEXT_SIZE =
                                       0x58 H1_ENUM_END(TownManagerStorageConstant)

// Constructor, UnloadTown, ShowText and recruitUnit::Close fix these offsets.
#pragma pack(push, 1)
                                           class townManager : public baseManager {
public:
    town* m_town;
    icon* m_backgroundIcon;
    townObject* m_townObjects[TOWN_MANAGER_OBJECT_CAPACITY];
    signed char m_townObjectCount;
    int m_unknown79;
    heroWindow* m_townWindow;
    strip* m_garrisonStrip;
    strip* m_heroStrip;
    char m_unknown89[0x12];
    bankBox* m_bankBox;
    char m_statusText[TOWN_MANAGER_STATUS_TEXT_SIZE];
    // --- constructors ---
    townManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void SetupExtraStuff(void);
    void SetTown(town* value) {
        m_town = value;
    }
    void ChangeTown(void);
    void SetupTown(void);
    void UnloadTown(void);
    void SetArmyCommand(int);
    void SetCommandAndText(struct tag_message&);
    void ShowText(char*);
    void DoCommand(int);
    void RedrawTownScreen(void);
    void SplitArmy(void);
    void ShiftQualChange(void);
    void ResetStrips(void);
    void Toggle(int);
    void DrawTown(int, int);
    int BuyBuild(int, int, int);
    void BuildObj(int);
    void SetupMage(class heroWindow*);
    int RecruitHero(int, int);
    void DoTavern(void);
    void SetupWell(class heroWindow*);
    void SetupThievesGuild(class heroWindow*, int);
    void SetupCastle(class heroWindow*, int);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_TOWNMANAGER_H
