#ifndef HOMM1_SOURCE_TOWNMANAGER_H
#define HOMM1_SOURCE_TOWNMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 26 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class bankBox;
class bitmap;
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
    TOWN_STATUS_REGION_HEIGHT = 0x10,
    TOWN_REDRAW_FIRST_CONTROL = 0x24,
    TOWN_REDRAW_LAST_CONTROL = 0x25,
    TOWN_VIEWPORT_WIDTH = 0x280,
    TOWN_VIEWPORT_HEIGHT = 0x100
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
    TOWN_ARMY_COMMAND_GARRISON = 6,
    TOWN_SHIFT_QUALIFIER_MASK = 3
H1_ENUM_END(TownArmyCommand)

H1_ENUM_BEGIN(TownCommandText)
    TOWN_TEXT_REDISTRIBUTE_ARMY = 0,
    TOWN_TEXT_CANNOT_COMBINE_LAST_ARMY = 1,
    TOWN_TEXT_COMBINE_ARMIES = 2,
    TOWN_TEXT_REDISTRIBUTE_TO_EMPTY_SLOT = 3,
    TOWN_TEXT_VIEW_ARMY = 4,
    TOWN_TEXT_CANNOT_MOVE_LAST_ARMY = 5,
    TOWN_TEXT_MOVE_ARMY = 6,
    TOWN_TEXT_EXCHANGE_ARMIES = 7,
    TOWN_TEXT_EXIT = 8,
    TOWN_TEXT_EMPTY_STATUS = 9,
    TOWN_TEXT_GARRISON = 10,
    TOWN_TEXT_EMPTY_SLOT = 11,
    TOWN_TEXT_SELECT_ARMY = 12,
    TOWN_TEXT_VIEW_HERO = 13,
    TOWN_TEXT_BUILDING_0 = 14,
    TOWN_TEXT_DWELLING = 21
H1_ENUM_END(TownCommandText)

H1_ENUM_BEGIN(TownControl)
    TOWN_EMPTY_STATUS_CONTROL_FIRST = 0x1c,
    TOWN_EMPTY_STATUS_CONTROL_LAST = 0x1d,
    TOWN_GARRISON_FIRST_CONTROL = 0x10,
    TOWN_GARRISON_SLOT_FIRST = 0x11,
    TOWN_HERO_FIRST_CONTROL = 0x16,
    TOWN_HERO_SLOT_FIRST = 0x17,
    TOWN_CLOSE_CONTROL = 0x7800
H1_ENUM_END(TownControl)

H1_ENUM_BEGIN(TownThievesCategory)
    THIEVES_CATEGORY_TOWNS = 0,
    THIEVES_CATEGORY_CASTLES = 1,
    THIEVES_CATEGORY_HEROES = 2,
    THIEVES_CATEGORY_GOLD = 3,
    THIEVES_CATEGORY_WOOD_AND_ORE = 4,
    THIEVES_CATEGORY_RARE_RESOURCES = 5,
    THIEVES_CATEGORY_OBELISKS = 6,
    THIEVES_CATEGORY_ARMY_STRENGTH = 7,
    TOWN_THIEVES_DEAD_PLAYER_STAT = -1,
    TOWN_BUILDING_TENT_FLAG = 0x20,
    TOWN_BUILDING_CASTLE_FLAG = 0x40
H1_ENUM_END(TownThievesCategory)

H1_ENUM_BEGIN(TownWellConstant)
    TOWN_WELL_DWELLING_COUNT = 6,
    TOWN_WELL_FIRST_ICON_CONTROL = 1,
    TOWN_WELL_FIRST_NAME_CONTROL = 7,
    TOWN_WELL_FIRST_MONSTER_ICON_CONTROL = 0xd,
    TOWN_WELL_FIRST_CREATURE_CONTROL = 0x13,
    TOWN_WELL_FIRST_AVAILABLE_CONTROL = 0x19,
    TOWN_WELL_FRAMES_PER_TYPE = 7,
    TOWN_WELL_FIRST_DWELLING_BUILDING = 7
H1_ENUM_END(TownWellConstant)

H1_ENUM_BEGIN(TownMageConstant)
    TOWN_MAGE_FIRST_SPELL_CONTROL = 1,
    TOWN_MAGE_FIRST_ICON_CONTROL = 10,
    TOWN_MAGE_FIRST_NAME_CONTROL = 0x13,
    TOWN_MAGE_FIRST_TOWER_CONTROL = 0x1c,
    TOWN_MAGE_DESCRIPTION_CONTROL = 0x50,
    TOWN_MAGE_SPELL_COUNT = 9,
    TOWN_MAGE_TOWER_FRAME_COUNT = 8,
    TOWN_MAGE_WIDGET_VISIBLE_FLAG = 4
H1_ENUM_END(TownMageConstant)

H1_ENUM_BEGIN(TownTavernConstant)
    TOWN_TAVERN_WINDOW_X = 0xa2,
    TOWN_TAVERN_WINDOW_Y = 0xa,
    TOWN_TAVERN_WINDOW_TEXT = 0xe,
    TOWN_TAVERN_MUSIC = 0x2f,
    TOWN_THEME_MUSIC_BASE = 0x1d,
    TOWN_TAVERN_ANIMATION_DELAY = 6,
    TOWN_TAVERN_UNUSED_FRAME = 2,
    TOWN_TAVERN_ANIMATION_CONTROL = 2,
    TOWN_TAVERN_ANIMATION_FRAME_COUNT = 8,
    TOWN_TAVERN_FIRST_ANIMATION_FRAME = 1,
    TOWN_DIALOG_BUTTON_0 = 0x7800,
    TOWN_DIALOG_BUTTON_1 = 0x7801,
    TOWN_DIALOG_BUTTON_2 = 0x7802
H1_ENUM_END(TownTavernConstant)
// clang-format on

// The constructor, UnloadTown, ShowText, ResetStrips and recruitUnit::Close
// fix these packed offsets; names follow Buka where the use matches.
#pragma pack(push, 1)
class townManager : public baseManager {
public:
    town *m_town;
    bitmap *m_backgroundBitmap;
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
    void SetArmyCommand(short);
    void SetCommandAndText(struct tag_message &);
    void ShowText(char *);
    void DoCommand(int);
    void RedrawTownScreen(void);
    void SplitArmy(void);
    void ShiftQualChange(void);
    void ResetStrips(void);
    void Toggle(signed char);
    void DrawTown(signed char, int);
    int BuyBuild(int, int, int);
    void BuildObj(int);
    void SetupMage(class heroWindow *);
    int RecruitHero(int, int);
    void DoTavern(void);
    void SetupWell(class heroWindow *);
    void SetupThievesGuild(class heroWindow *, int);
    void SetupCastle(class heroWindow *, int);
    char *GetBuildingName(int);
    // HoMM1 keeps the thieves-guild helpers as townManager members.
    void GetCategoryStats(signed char, long *const, signed char *const);
    void SortStats(long *const, signed char *const);
};
#pragma pack(pop)

extern char *cTownCommand[];
extern signed char townTheme[];
short TavernHandler(struct tag_message &);
short MageGuildHandler(struct tag_message &);
#endif // HOMM1_SOURCE_TOWNMANAGER_H
