#ifndef HOMM1_SOURCE_TOWNMANAGER_H
#define HOMM1_SOURCE_TOWNMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <BASE/message.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/town.h>

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
    TOWN_REDRAW_FIRST_CONTROL = 0x24,
    TOWN_REDRAW_LAST_CONTROL = 0x25,
    TOWN_VIEWPORT_WIDTH = 0x280,
    TOWN_VIEWPORT_HEIGHT = 0x100
H1_ENUM_CONST_END(TownManagerStorageConstant)

H1_ENUM_BEGIN(TownArmyCommand)
    TOWN_ARMY_COMMAND_NONE = -1,
    TOWN_ARMY_COMMAND_SELECT = 0,
    TOWN_ARMY_COMMAND_VIEW = 1,
    TOWN_ARMY_COMMAND_MERGE = 2,
    TOWN_ARMY_COMMAND_SWAP = 3,
    TOWN_ARMY_COMMAND_VIEW_HERO = 4,
    TOWN_ARMY_COMMAND_SPLIT = 5,
    TOWN_ARMY_COMMAND_GARRISON = 6
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
    TOWN_TEXT_DWELLING = 21,
    TOWN_TEXT_COUNT = 22
H1_ENUM_END(TownCommandText)
H1_ENUM_STEPPED(TownCommandText)
extern H1_ENUM_ARRAY(char*, gTownCommand, TownCommandText, TOWN_TEXT_COUNT);

H1_ENUM_ID_BEGIN(TownControl)
    TOWN_EMPTY_STATUS_CONTROL_FIRST = 0x1c,
    TOWN_EMPTY_STATUS_CONTROL_LAST = 0x1d,
    TOWN_GARRISON_FIRST_CONTROL = 0x10,
    TOWN_GARRISON_SLOT_FIRST = 0x11,
    // The five army slots of each strip run FIRST..LAST (Main's hover range).
    TOWN_GARRISON_SLOT_LAST = 0x15,
    TOWN_HERO_FIRST_CONTROL = 0x16,
    TOWN_HERO_SLOT_FIRST = 0x17,
    TOWN_HERO_SLOT_LAST = 0x1b,
    TOWN_CLOSE_CONTROL = DIALOG_BUTTON_0
H1_ENUM_ID_END(TownControl)

// Town objects: gTownObjectType's empty entry and a .tod without a border
// widget are NONE (m_selectedBuilding's empty value is BUILDING_SLOT_NONE).
// gTownObjectNames holds the neutral objects, the four town-type
// prefixes, then the faction-object suffixes (index type + TOWN_TYPE_COUNT).
// The mage guild's border grows 20 pixels a level above 0x61, bottom 0x99;
// its level frames come in pairs.
H1_ENUM_CONST_BEGIN(TownObjectConstant)
    TOWN_OBJECT_NONE = -1,
    TOWN_MAGE_GUILD_LEVEL_HEIGHT = 20,
    TOWN_MAGE_GUILD_BASE_HEIGHT = 0x61,
    TOWN_MAGE_GUILD_BOTTOM_Y = 0x99,
    // Two tower frames (and tower controls) per mage-guild level.
    TOWN_MAGE_GUILD_LEVEL_FRAME_STRIDE = 2
H1_ENUM_CONST_END(TownObjectConstant)

// SetupThievesGuild's rows (gThievesCategoryNames order); its categories
// argument is the row bound.
H1_ENUM_BEGIN(TownThievesCategory)
    // SetupThievesGuild's categories argument: count the rows from the
    // player's thieves' guilds (one guild 3 rows, two 5, three 7, four all).
    THIEVES_CATEGORIES_BY_GUILDS = -1,
    THIEVES_CATEGORY_TOWNS = 0,
    THIEVES_CATEGORY_CASTLES = 1,
    THIEVES_CATEGORY_HEROES = 2,
    THIEVES_CATEGORY_GOLD = 3,
    THIEVES_CATEGORY_WOOD_AND_ORE = 4,
    THIEVES_CATEGORY_RARE_RESOURCES = 5,
    THIEVES_CATEGORY_OBELISKS = 6,
    THIEVES_CATEGORY_ARMY_STRENGTH = 7,
    THIEVES_CATEGORY_COUNT = 8
H1_ENUM_END(TownThievesCategory)
H1_ENUM_STEPPED(TownThievesCategory)

// SetupThievesGuild's layout: a dead player's stat, the rank columns and
// category rows, the flag frames and the tie centring step.
H1_ENUM_CONST_BEGIN(ThievesGuildLayoutConstant)
    TOWN_THIEVES_DEAD_PLAYER_STAT = -1,
    THIEVES_RANK_FIRST_X = 0x120,
    THIEVES_PLAYER_COLUMN_WIDTH = 0x61,
    THIEVES_FIRST_CATEGORY_Y = 0x1b,
    THIEVES_CATEGORY_ROW_HEIGHT = 0x1a,
    THIEVES_FLAG_FRAME_BASE = 0xe,
    THIEVES_RANK_ICON_WIDTH = 0x12,
    THIEVES_RANK_ICON_HEIGHT = 0x16,
    THIEVES_PLAYER_WIDTH = 0x48,
    THIEVES_TIE_CENTERING_STEP = 9,
    THIEVES_RANK_COUNT = 4
H1_ENUM_CONST_END(ThievesGuildLayoutConstant)

// well.bin control bases (one per dwelling) and the well's frames per type.
H1_ENUM_CONST_BEGIN(TownWellConstant)
    TOWN_WELL_FIRST_ICON_CONTROL = 1,
    TOWN_WELL_FIRST_NAME_CONTROL = 7,
    TOWN_WELL_FIRST_MONSTER_ICON_CONTROL = 0xd,
    TOWN_WELL_FIRST_CREATURE_CONTROL = 0x13,
    TOWN_WELL_FIRST_AVAILABLE_CONTROL = 0x19,
    TOWN_WELL_FRAMES_PER_TYPE = 7
H1_ENUM_CONST_END(TownWellConstant)

// mageguild.bin control bases (one per spell slot), the description and the
// tower frames.
H1_ENUM_CONST_BEGIN(TownMageConstant)
    TOWN_MAGE_FIRST_SPELL_CONTROL = 1,
    TOWN_MAGE_FIRST_ICON_CONTROL = 10,
    TOWN_MAGE_FIRST_NAME_CONTROL = 0x13,
    TOWN_MAGE_FIRST_TOWER_CONTROL = 0x1c,
    TOWN_MAGE_DESCRIPTION_CONTROL = 0x50,
    TOWN_MAGE_TOWER_FRAME_COUNT = 8
H1_ENUM_CONST_END(TownMageConstant)

// splitwin.bin (swapManager/townManager split dialog) and ViewArmy's position.
H1_ENUM_CONST_BEGIN(TownSplitConstant)
    TOWN_SPLIT_PROMPT_CONTROL = 1,
    TOWN_SPLIT_AMOUNT_CONTROL = 0x44,
    TOWN_SPLIT_INCREASE_CONTROL = 0x45,
    TOWN_SPLIT_DECREASE_CONTROL = 0x46,
    TOWN_SPLIT_SETUP_AMOUNT_CONTROL = 4,
    TOWN_SPLIT_WINDOW_X = 0xb1,
    TOWN_SPLIT_WINDOW_Y = 0x14,
    TOWN_ARMY_VIEW_X = 0x77,
    TOWN_ARMY_VIEW_Y = 0x14
H1_ENUM_CONST_END(TownSplitConstant)

// castle.bin controls SetupCastle fills: icon/name/state bases per building
// (generic structures first, then the dwellings), the hero and status rows.
H1_ENUM_CONST_BEGIN(TownCastleControl)
    TOWN_CASTLE_FIRST_ICON_CONTROL = 1,
    TOWN_CASTLE_FIRST_DWELLING_NAME_CONTROL = 0x17,
    TOWN_CASTLE_FIRST_STATE_CONTROL = 0x20,
    TOWN_CASTLE_FIRST_DWELLING_STATE_CONTROL = 0x27,
    TOWN_CASTLE_HERO_CONTROL = 0x30,
    TOWN_CASTLE_STATUS_CONTROL = 0x32,
    TOWN_CASTLE_HERO_STATE_CONTROL = 0x50,
    TOWN_CASTLE_STATUS_FIRST_CONTROL = 0x1f4,
    TOWN_CASTLE_STATUS_TEXT_CONTROL = 0x1f6,
    // The generic structures before the tent (mage guild .. well).
    TOWN_CASTLE_SPECIAL_BUILDING_COUNT = 5,
    TOWN_CASTLE_STATUS_X = 0xa,
    TOWN_CASTLE_STATUS_Y = 0xf0,
    TOWN_CASTLE_STATUS_WIDTH = 0x21a,
    TOWN_CASTLE_STATUS_HEIGHT = 0x10
H1_ENUM_CONST_END(TownCastleControl)

// RecruitHero's m_recruitState: which of the two tavern heroes was hired.
H1_ENUM_CONST_BEGIN(TownRecruitHeroConstant)
    RECRUIT_HERO_NONE = -1
H1_ENUM_CONST_END(TownRecruitHeroConstant)

// buybuil%d.bin controls BuyBuild fills: the building's picture and name.
H1_ENUM_ID_BEGIN(TownBuyBuildControl)
    BUY_BUILD_ICON_CONTROL = 2,
    BUY_BUILD_NAME_CONTROL = 3
H1_ENUM_ID_END(TownBuyBuildControl)

// castle.bin state frames over a building's icon.
H1_ENUM_BEGIN(TownCastleFrame)
    // No state frame: SetupCastle clears the state widget.
    TOWN_CASTLE_FRAME_NONE = -1,
    TOWN_CASTLE_FRAME_BUILT = 0xb,
    TOWN_CASTLE_FRAME_CANNOT_BUILD = 0xc,
    TOWN_CASTLE_FRAME_CANNOT_AFFORD = 0xd
H1_ENUM_END(TownCastleFrame)

// The castle window's status-bar text rows (gCastleInfo).
H1_ENUM_BEGIN(TownCastleInfoText)
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
    TOWN_CASTLE_INFO_COUNT = 14
H1_ENUM_END(TownCastleInfoText)
extern H1_ENUM_ARRAY(char*, gCastleInfo, TownCastleInfoText, TOWN_CASTLE_INFO_COUNT);

// The tavern window and its animation (the tavern plays MUSIC_TRACK_TAVERN;
// MUSIC_TRACK_TOWN_FIRST + townTheme[type] is a town's ambient track).
H1_ENUM_CONST_BEGIN(TownTavernConstant)
    TOWN_TAVERN_WINDOW_X = 0xa2,
    TOWN_TAVERN_WINDOW_Y = 0xa,
    TOWN_TAVERN_ANIMATION_DELAY = 70,
    TOWN_TAVERN_UNUSED_FRAME = 2,
    TOWN_TAVERN_ANIMATION_CONTROL = 2,
    TOWN_TAVERN_ANIMATION_FRAME_COUNT = 8,
    TOWN_TAVERN_FIRST_ANIMATION_FRAME = 1
H1_ENUM_CONST_END(TownTavernConstant)

// Town purchases and building tables: the spell book and boat prices and
// the six dwellings each faction's rows hold in the gDwelling* tables
// (BuyBuild splits neutral buildings from dwellings at
// BUILDING_SLOT_STRUCTURE_LAST).
H1_ENUM_CONST_BEGIN(TownBuildConstant)
    TOWN_SPELL_BOOK_COST = 500,
    TOWN_BOAT_GOLD_COST = 1000,
    TOWN_BOAT_WOOD_COST = 10,
    TOWN_DWELLINGS_PER_FACTION = 6,
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
    TOWN_EMPTY_HERO_PORTRAIT_FRAME = 8,
    // strip's type argument (stored in strip::m_stripType, which HoMM1 never
    // reads): the garrison strip with or without a visiting hero and the
    // hero strip.
    TOWN_CREST_FRAME_WITH_HERO = 1,
    TOWN_CREST_FRAME_WITHOUT_HERO = 4,
    TOWN_HERO_STRIP_FRAME_COUNT = 3
H1_ENUM_CONST_END(TownScreenConstant)

// rcrthero.bin widget ids: the two candidates' portraits, class labels and
// select buttons (dimmed for the cannot-recruit quick view).
H1_ENUM_ID_BEGIN(TownRecruitHeroControl)
    RECRUIT_HERO_PORTRAIT_FIRST = 2,
    RECRUIT_HERO_PORTRAIT_SECOND = 3,
    RECRUIT_HERO_CLASS_FIRST = 6,
    RECRUIT_HERO_CLASS_SECOND = 7,
    RECRUIT_HERO_SELECT_FIRST = 8,
    RECRUIT_HERO_SELECT_SECOND = 9
H1_ENUM_ID_END(TownRecruitHeroControl)

// The constructor, UnloadTown, ShowText, ResetStrips and recruitUnit::Close
// fix these packed offsets.
#pragma pack(push, 1)
class townManager : public baseManager {
public:
    town* m_town;
    bitmap* m_backgroundBitmap;
    townObject* m_townObjects[TOWN_MANAGER_OBJECT_CAPACITY];
    i8 m_townObjectCount;
    // Main covers the town bottom while a building dialog is open.
    heroWindow* m_coverWindow;
    heroWindow* m_townWindow;
    strip* m_garrisonStrip;
    strip* m_heroStrip;
    strip* m_selectedStrip;
    i16 m_selectedArmySlot;
    strip* m_swapStrip;
    i16 m_swapArmySlot;
    strip* m_pendingStrip;
    i16 m_pendingArmySlot;
    bankBox* m_bankBox;
    char m_statusText[TOWN_MANAGER_STATUS_TEXT_SIZE];
    i16 m_lastHoverId;
    H1_ENUM_STORAGE(TownArmyCommand, i8) m_command;
    // SetupCastle's recruit-slot state and its affordable/buildable masks.
    i8 m_recruitResult;
    u16 m_affordableBuildings;
    u16 m_buildableBuildings;
    i8 m_castleDialogActive;
    H1_ENUM_STORAGE(BuildingSlotType, i16) m_selectedBuilding;
    heroWindow* m_heroWindow0;
    heroWindow* m_heroWindow1;
    i16 m_splitAmount;
    i16 m_splitMaximum;
    // RecruitHero: the chosen tavern slot (-1 if none) and both candidates.
    i16 m_recruitState;
    hero* m_recruitHeroes[2];
    // HoMM1 Main tests this additional mask against message.type.
    i16 m_dispatchMask;
    // --- constructors ---
    townManager(void);
    // --- virtual methods (vtable order) ---
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 id) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void SetupExtraStuff(void);
    void SetTown(town* value) {
        m_town = value;
    }
    void ChangeTown(void);
    void SetupTown(void);
    void UnloadTown(void);
    void SetArmyCommand(H1_ENUM_PARAM(MessageModifier, i16) qualifier);
    void SetCommandAndText(struct tag_message& message);
    void ShowText(char*);
    void DoCommand(H1_ENUM_PARAM(TownArmyCommand, i8) command);
    void RedrawTownScreen(void);
    void SplitArmy(void);
    void ShiftQualChange(void);
    void ResetStrips(void);
    void Toggle(H1_ENUM_PARAM(BuildingSlotType, i8) building);
    void DrawTown(i8 updateScreen, i32 drawFlags);
    i16 BuyBuild(H1_ENUM_PARAM(BuildingSlotType, i16) building, i8 cannotBuy, i8 quickView);
    void BuildObj(H1_ENUM_PARAM(BuildingSlotType, i16) building);
    void SetupMage(class heroWindow* window);
    i8 RecruitHero(i8 cannotRecruit);
    void DoTavern(void);
    void SetupWell(class heroWindow* window);
    void SetupThievesGuild(
        class heroWindow* window,
        H1_ENUM_PARAM(TownThievesCategory, i16) categories
    );
    void SetupCastle(class heroWindow* window);
    char* GetBuildingName(H1_ENUM_PARAM(BuildingSlotType, i16) building);
    // HoMM1 keeps the thieves-guild helpers as townManager members.
    void GetCategoryStats(
        H1_ENUM_PARAM(TownThievesCategory, i8) category,
        i32* const stats,
        i8* const order
    );
    void SortStats(i32* const stats, i8* const order);
};
#pragma pack(pop)

// BuildObj's fizzle rectangle per town type and building (0x00491680).
struct TownBuildingExtent {
    i16 x;
    i16 y;
    i16 width;
    i16 height;
};

// Open's per-type town-object layout (0x0048d428).
extern const H1_ENUM_ARRAY_ROWS(i8, gTownObjectType, TownType, TOWN_TYPE_COUNT, TOWN_MANAGER_OBJECT_CAPACITY);
H1_ENUM_RETURN(MessageDispatchResult, i16) TavernHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) MageGuildHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SplitArmyHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) CastleHandler(struct tag_message& message);
// BuyBuild's cost-row layouts by resource count, and the thieves'-guild
// counts SetupThievesGuild reveals more categories for.
H1_ENUM_CONST_BEGIN(TownCountConstant)
    BUILD_RESOURCES_FIVE = 5,
    BUILD_RESOURCES_SIX = 6,
    BUILD_RESOURCES_SEVEN = 7,
    THIEVES_GUILDS_TWO = 2,
    THIEVES_GUILDS_THREE = 3
H1_ENUM_CONST_END(TownCountConstant)

#endif // HOMM1_SOURCE_TOWNMANAGER_H
