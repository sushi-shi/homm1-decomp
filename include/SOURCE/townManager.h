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
class hero;
class heroWindow;
class icon;
class strip;
class town;
class townObject;
struct tag_message;

// clang-format off
H1_ENUM_CONST_BEGIN(TownManagerStorageConstant)
    TOWN_MANAGER_OBJECT_CAPACITY = 16,
    TOWN_MANAGER_STATUS_TEXT_SIZE = 0x50,
    TOWN_MANAGER_DISPATCH_MASK = 0x32f,
    TOWN_STATUS_TEXT_CONTROL = 0x386,
    TOWN_STATUS_REGION_Y = 0x1ce,
    TOWN_STATUS_REGION_WIDTH = 0x280,
    TOWN_STATUS_REGION_HEIGHT = 0x10,
    TOWN_NAME_TEXT_CONTROL = 0x25,
    TOWN_REDRAW_INTERVAL = 0x96,
    TOWN_FIRST_FACTION_OBJECT = 5,
    TOWN_CREST_NO_HERO_OFFSET = 0x10,
    TOWN_MANAGER_MESSAGE_MASK = 0x800,
    TOWN_REDRAW_FIRST_CONTROL = 0x24,
    TOWN_REDRAW_LAST_CONTROL = 0x25,
    TOWN_VIEWPORT_WIDTH = 0x280,
    TOWN_VIEWPORT_HEIGHT = 0x100
H1_ENUM_CONST_END(TownManagerStorageConstant)
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
    TOWN_BUILDING_CASTLE_FLAG = 0x40,
    THIEVES_RANK_FIRST_X = 0x120,
    THIEVES_PLAYER_COLUMN_WIDTH = 0x61,
    THIEVES_FIRST_CATEGORY_Y = 0x1b,
    THIEVES_CATEGORY_ROW_HEIGHT = 0x1a,
    THIEVES_FLAG_FRAME_BASE = 0xe,
    THIEVES_RANK_ICON_WIDTH = 0x12,
    THIEVES_RANK_ICON_HEIGHT = 0x16,
    THIEVES_PLAYER_WIDTH = 0x48,
    THIEVES_TIE_CENTERING_STEP = 9,
    THIEVES_CATEGORY_COUNT = 8,
    THIEVES_RANK_COUNT = 4
H1_ENUM_END(TownThievesCategory)

H1_ENUM_CONST_BEGIN(TownWellConstant)
    TOWN_WELL_DWELLING_COUNT = 6,
    TOWN_WELL_FIRST_ICON_CONTROL = 1,
    TOWN_WELL_FIRST_NAME_CONTROL = 7,
    TOWN_WELL_FIRST_MONSTER_ICON_CONTROL = 0xd,
    TOWN_WELL_FIRST_CREATURE_CONTROL = 0x13,
    TOWN_WELL_FIRST_AVAILABLE_CONTROL = 0x19,
    TOWN_WELL_FRAMES_PER_TYPE = 7,
    TOWN_WELL_FIRST_DWELLING_BUILDING = 7
H1_ENUM_CONST_END(TownWellConstant)

H1_ENUM_CONST_BEGIN(TownMageConstant)
    TOWN_MAGE_FIRST_SPELL_CONTROL = 1,
    TOWN_MAGE_FIRST_ICON_CONTROL = 10,
    TOWN_MAGE_FIRST_NAME_CONTROL = 0x13,
    TOWN_MAGE_FIRST_TOWER_CONTROL = 0x1c,
    TOWN_MAGE_DESCRIPTION_CONTROL = 0x50,
    TOWN_MAGE_SPELL_COUNT = 9,
    TOWN_MAGE_TOWER_FRAME_COUNT = 8,
    TOWN_MAGE_WIDGET_VISIBLE_FLAG = 4
H1_ENUM_CONST_END(TownMageConstant)

H1_ENUM_CONST_BEGIN(TownSplitConstant)
    TOWN_SPLIT_PROMPT_CONTROL = 1,
    TOWN_SPLIT_AMOUNT_CONTROL = 0x44,
    TOWN_SPLIT_INCREASE_CONTROL = 0x45,
    TOWN_SPLIT_DECREASE_CONTROL = 0x46,
    TOWN_SPLIT_SETUP_AMOUNT_CONTROL = 4,
    TOWN_SPLIT_WINDOW_X = 0xb1,
    TOWN_SPLIT_WINDOW_Y = 0x14,
    TOWN_CASTLE_BUILDING_COUNT = 13,
    TOWN_CASTLE_FRAME_BUILT = 0xb,
    TOWN_CASTLE_FRAME_CANNOT_BUILD = 0xc,
    TOWN_CASTLE_FRAME_CANNOT_AFFORD = 0xd,
    TOWN_CASTLE_FIRST_ICON_CONTROL = 1,
    TOWN_CASTLE_FIRST_DWELLING_NAME_CONTROL = 0x17,
    TOWN_CASTLE_FIRST_STATE_CONTROL = 0x20,
    TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL = 0x27,
    TOWN_CASTLE_HERO_STATE_CONTROL = 0x50,
    TOWN_CASTLE_SPECIAL_BUILDING_COUNT = 5,
    TOWN_WIDGET_VISIBLE_FLAG = 4,
    TOWN_CASTLE_HERO_CONTROL = 0x30,
    TOWN_CASTLE_STATUS_CONTROL = 0x32,
    TOWN_CASTLE_STATUS_FIRST_CONTROL = 0x1f4,
    TOWN_CASTLE_STATUS_TEXT_CONTROL = 0x1f6,
    TOWN_CASTLE_STATUS_X = 0xa,
    TOWN_CASTLE_STATUS_Y = 0xf0,
    TOWN_CASTLE_STATUS_WIDTH = 0x21a,
    TOWN_CASTLE_STATUS_HEIGHT = 0x10,
    TOWN_CASTLE_INFO_BUILD_MAGE_GUILD = 0,
    TOWN_CASTLE_INFO_MAGE_GUILD_MAX_LEVEL = 1,
    TOWN_CASTLE_INFO_CANNOT_AFFORD_MAGE_LEVEL = 2,
    TOWN_CASTLE_INFO_ADD_MAGE_GUILD_LEVEL = 3,
    TOWN_CASTLE_INFO_ALREADY_BUILT = 4,
    TOWN_CASTLE_INFO_CANNOT_BUILD = 5,
    TOWN_CASTLE_INFO_CANNOT_AFFORD = 6,
    TOWN_CASTLE_INFO_BUILD = 7,
    TOWN_CASTLE_INFO_CANNOT_AFFORD_HERO = 8,
    TOWN_CASTLE_INFO_TOO_MANY_HEROES = 9,
    TOWN_CASTLE_INFO_TOWN_OCCUPIED = 10,
    TOWN_CASTLE_INFO_RECRUIT_HERO = 11,
    TOWN_CASTLE_INFO_EXIT = 12,
    TOWN_CASTLE_INFO_OPTIONS = 13,
    TOWN_ARMY_VIEW_X = 0x77,
    TOWN_ARMY_VIEW_Y = 0x14,
    TOWN_BUILDING_MAGE_GUILD = 0,
    TOWN_BUILDING_TENT = 5,
    TOWN_BUILDING_CASTLE = 6,
    TOWN_BUILDING_FIRST_DWELLING = 7,
    TOWN_BUILDING_LAST_DWELLING = 12,
    TOWN_OBJECT_ENABLED_FLAG = 2
H1_ENUM_CONST_END(TownSplitConstant)

H1_ENUM_CONST_BEGIN(TownTavernConstant)
    TOWN_TAVERN_WINDOW_X = 0xa2,
    TOWN_TAVERN_WINDOW_Y = 0xa,
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
H1_ENUM_CONST_END(TownTavernConstant)

// Town purchases and building tables: the spell book and boat prices
// (Buka TOWN_SPELL_BOOK_COST), the six dwellings each faction's rows hold in
// the gDwelling* tables, and BuyBuild's split between neutral buildings
// (gNeutralBuildingCosts rows 0..6) and dwellings.
H1_ENUM_CONST_BEGIN(TownBuildConstant)
    TOWN_SPELL_BOOK_COST = 500,
    TOWN_BOAT_GOLD_COST = 1000,
    TOWN_BOAT_WOOD_COST = 10,
    TOWN_DWELLINGS_PER_FACTION = 6,
    TOWN_NEUTRAL_BUILDING_LAST = 6,
    TOWN_NEUTRAL_BUILDING_COUNT = 7,
    TOWN_DWELLING_COST_ROWS = 28,
    TOWN_MAGE_GUILD_COST_LEVEL_LAST = 3
H1_ENUM_CONST_END(TownBuildConstant)

// Town screen layout: the garrison and visiting-hero strips below the
// 0x100-pixel town view, the bank box beside them, and the empty hero
// strip's placeholder portrait frame.
H1_ENUM_CONST_BEGIN(TownScreenConstant)
    TOWN_GARRISON_STRIP_Y = 0x100,
    TOWN_HERO_STRIP_Y = 0x163,
    TOWN_BANK_BOX_X = 0x222,
    TOWN_BANK_BOX_Y = 0x100,
    TOWN_EMPTY_HERO_PORTRAIT_FRAME = 8
H1_ENUM_CONST_END(TownScreenConstant)

// rcrthero.bin widget ids: the two candidates' portraits, class labels and
// select buttons (dimmed for the cannot-recruit quick view).
H1_ENUM_BEGIN(TownRecruitHeroControl)
    RECRUIT_HERO_PORTRAIT_FIRST = 2,
    RECRUIT_HERO_PORTRAIT_SECOND = 3,
    RECRUIT_HERO_CLASS_FIRST = 6,
    RECRUIT_HERO_CLASS_SECOND = 7,
    RECRUIT_HERO_SELECT_FIRST = 8,
    RECRUIT_HERO_SELECT_SECOND = 9
H1_ENUM_END(TownRecruitHeroControl)
// clang-format on

// The constructor, UnloadTown, ShowText, ResetStrips and recruitUnit::Close
// fix these packed offsets; names follow Buka where the use matches.
#pragma pack(push, 1)
class townManager : public baseManager {
public:
    town* m_town;
    bitmap* m_backgroundBitmap;
    townObject* m_townObjects[TOWN_MANAGER_OBJECT_CAPACITY];
    signed char m_townObjectCount;
    // Main covers the town bottom while a building dialog is open.
    heroWindow* m_coverWindow;
    heroWindow* m_townWindow;
    strip* m_garrisonStrip;
    strip* m_heroStrip;
    strip* m_selectedStrip;
    short m_selectedArmySlot;
    strip* m_swapStrip;
    short m_swapArmySlot;
    strip* m_pendingStrip;
    short m_pendingArmySlot;
    bankBox* m_bankBox;
    char m_statusText[TOWN_MANAGER_STATUS_TEXT_SIZE];
    short m_lastHoverId;
    H1_ENUM_STORAGE(TownArmyCommand, signed char) m_command;
    // SetupCastle's recruit-slot state and its affordable/buildable masks.
    signed char m_recruitResult;
    unsigned short m_affordableBuildings;
    unsigned short m_buildableBuildings;
    signed char m_castleDialogActive;
    short m_selectedBuilding;
    heroWindow* m_heroWindow0;
    heroWindow* m_heroWindow1;
    short m_splitAmount;
    short m_splitMaximum;
    // RecruitHero: the chosen tavern slot (-1 if none) and both candidates.
    short m_recruitState;
    hero* m_recruitHeroes[2];
    // HoMM1 Main tests this additional mask against message.type.
    short m_dispatchMask;
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
    void SetArmyCommand(short);
    void SetCommandAndText(struct tag_message&);
    void ShowText(char*);
    void DoCommand(signed char);
    void RedrawTownScreen(void);
    void SplitArmy(void);
    void ShiftQualChange(void);
    void ResetStrips(void);
    void Toggle(signed char);
    void DrawTown(signed char, int);
    short BuyBuild(short, signed char, signed char);
    void BuildObj(short);
    void SetupMage(class heroWindow*);
    signed char RecruitHero(signed char);
    void DoTavern(void);
    void SetupWell(class heroWindow*);
    void SetupThievesGuild(class heroWindow*, short);
    void SetupCastle(class heroWindow*);
    char* GetBuildingName(short);
    // HoMM1 keeps the thieves-guild helpers as townManager members.
    void GetCategoryStats(signed char, long* const, signed char* const);
    void SortStats(long* const, signed char* const);
};
#pragma pack(pop)

// BuildObj's fizzle rectangle per town type and building (0x00491868).
struct TownBuildingExtent {
    short x;
    short y;
    short width;
    short height;
};
extern TownBuildingExtent gTownBuildingExtents[4][16];

extern char* cTownCommand[];
extern signed char townTheme[];
// Open's per-type town-object layout (0x0048c028).
extern const signed char gTownObjectType[4][TOWN_MANAGER_OBJECT_CAPACITY];
short TavernHandler(struct tag_message&);
short MageGuildHandler(struct tag_message&);
short SplitArmyHandler(struct tag_message&);
short CastleHandler(struct tag_message&);
// KB's tavern recruit dialog handler (retail 0x0045140e).
short RecruitHeroHandler(struct tag_message&);
extern char* cCastleInfo[];
#endif // HOMM1_SOURCE_TOWNMANAGER_H
