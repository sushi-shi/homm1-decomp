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

// clang-format off
H1_ENUM_BEGIN(TownManagerStorageConstant)
    TOWN_MANAGER_OBJECT_CAPACITY = 16,
    TOWN_MANAGER_STATUS_TEXT_SIZE = 0x50,
    TOWN_MANAGER_DISPATCH_MASK = 0x32f,
    TOWN_STATUS_TEXT_CONTROL = 0x386,
    TOWN_STATUS_REGION_Y = 0x1ce,
    TOWN_STATUS_REGION_WIDTH = 0x280,
    TOWN_STATUS_REGION_HEIGHT = 0x10
H1_ENUM_END(TownManagerStorageConstant)
// clang-format on

// clang-format off
H1_ENUM_BEGIN(TownArmyCommand)
    TOWN_ARMY_COMMAND_NONE = -1,
    TOWN_ARMY_COMMAND_SELECT = 0,
    TOWN_ARMY_COMMAND_VIEW = 1,
    TOWN_ARMY_COMMAND_MERGE = 2,
    TOWN_ARMY_COMMAND_SWAP = 3,
    TOWN_ARMY_COMMAND_VIEW_HERO = 4,
    TOWN_ARMY_COMMAND_SPLIT = 5,
    TOWN_SHIFT_QUALIFIER_MASK = 3
H1_ENUM_END(TownArmyCommand)
// clang-format on

// The constructor, UnloadTown, ShowText, ResetStrips and recruitUnit::Close
// fix these packed offsets; names follow Buka where the use matches.
#pragma pack(push, 1)
class townManager : public baseManager {
public:
    town *m_town;
    icon *m_backgroundIcon;
    townObject *m_townObjects[TOWN_MANAGER_OBJECT_CAPACITY];
    signed char m_townObjectCount;
    int m_unknown79;
    heroWindow *m_townWindow;
    strip *m_garrisonStrip;
    strip *m_heroStrip;
    strip *m_selectedStrip;
    short m_selectedArmySlot;
    strip *m_swapStrip;
    short m_swapArmySlot;
    strip *m_pendingStrip;
    short m_pendingArmySlot;
    bankBox *m_bankBox;
    char m_statusText[TOWN_MANAGER_STATUS_TEXT_SIZE];
    short m_lastHoverId;
    H1_ENUM_STORAGE(TownArmyCommand, signed char) m_command;
    signed char m_unknownf2;
    short m_unknownf3;
    short m_unknownf5;
    signed char m_castleDialogActive;
    short m_selectedBuilding;
    heroWindow *m_heroWindow0;
    heroWindow *m_heroWindow1;
    short m_splitAmount;
    short m_splitMaximum;
    short m_unknown106;
    int m_unknown108;
    int m_unknown10c;
    // HoMM1 Main tests this additional mask against message.type.
    short m_dispatchMask;
    // --- constructors ---
    townManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void SetupExtraStuff(void);
    void SetTown(town *value) { m_town = value; }
    void ChangeTown(void);
    void SetupTown(void);
    void UnloadTown(void);
    void SetArmyCommand(int);
    void SetCommandAndText(struct tag_message &);
    void ShowText(char *);
    void DoCommand(int);
    void RedrawTownScreen(void);
    void SplitArmy(void);
    void ShiftQualChange(void);
    void ResetStrips(void);
    void Toggle(signed char);
    void DrawTown(int, int);
    int BuyBuild(int, int, int);
    void BuildObj(int);
    void SetupMage(class heroWindow *);
    int RecruitHero(int, int);
    void DoTavern(void);
    void SetupWell(class heroWindow *);
    void SetupThievesGuild(class heroWindow *, int);
    void SetupCastle(class heroWindow *, int);
    char *GetBuildingName(int);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_TOWNMANAGER_H
