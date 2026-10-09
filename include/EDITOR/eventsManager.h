#ifndef HOMM1_EDITOR_EVENTSMANAGER_H
#define HOMM1_EDITOR_EVENTSMANAGER_H

// The map-information tool and the editor dialogs of its unit
// (src/EDITOR/EVENTMGR.cpp). The unit name EVENTMGR is descriptive; Open
// stores the class name "eventsManager". The dialog, record and global names
// are descriptive: the windows are cellwin.bin, edittown.bin, monedit.bin,
// heroedit.bin, clearwin.bin, dtlwind.bin and editnew.bin.

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/game.h>

class heroWindow;
class icon;
class iconWidget;
struct tag_message;
struct editTownExtra;
struct editHeroExtra;

// SetWinText rows of the editor's dialogs: the editor's own rows of the
// window-text table, so each call converts the row to the WindowTextId that
// SetWinText takes.
H1_ENUM_CONST_BEGIN(EventsWindowText)
    EVENTS_WINDOW_TEXT_TOWN = 0x65,
    EVENTS_WINDOW_TEXT_MONSTER = 0x66,
    EVENTS_WINDOW_TEXT_HERO = 0x67,
    EVENTS_WINDOW_TEXT_CLEAR = 0x68,
    EVENTS_WINDOW_TEXT_MAP_DETAILS = 0x69,
    EVENTS_WINDOW_TEXT_NEW_MAP = 0x6a
H1_ENUM_CONST_END(EventsWindowText)

// The editor dialogs' closing buttons (heroWindowManager::m_dialogResult):
// cancel keeps the edited record or settings as they were. Widget ids, as
// DialogButtonId's reserved slots they alias.
H1_ENUM_ID_BEGIN(EventsDialogButton)
    EVENTS_DIALOG_CANCEL = DIALOG_BUTTON_1,
    EVENTS_DIALOG_OK = DIALOG_BUTTON_2
H1_ENUM_ID_END(EventsDialogButton)

H1_ENUM_CONST_BEGIN(EventsManagerLayout)
// Every dialog opens at (16, 16).
    EVENTS_DIALOG_X = 16,
    EVENTS_DIALOG_Y = 16,
    // Main outlines the hovered cell (overlay.icn) in this palette colour.
    EVENTS_HOVER_COLOR = 1
H1_ENUM_CONST_END(EventsManagerLayout)

// cellwin.bin: a text field per mapCell byte and per editMapCellPair word
// from CELL_WINDOW_FIRST_BYTE (CellWindowField order), a toggle per m_flags
// bit from CELL_WINDOW_FIRST_FLAG (bit order) and a toggle for the trigger's
// event bit (MAP_TRIGGER_EVENT).
H1_ENUM_CONST_BEGIN(CellWindowConstant)
    CELL_WINDOW_FIRST_BYTE = 0x2bc,
    CELL_WINDOW_FIRST_FLAG = 0x40,
    CELL_WINDOW_EVENT_TOGGLE = 0x48,
    CELL_WINDOW_BYTE_MASK = 0xff
H1_ENUM_CONST_END(CellWindowConstant)

// cellwin.bin's text fields, offsets from CELL_WINDOW_FIRST_BYTE: the ten
// mapCell bytes in record order, then the cell's object and overlay ids.
H1_ENUM_CONST_BEGIN(CellWindowField)
    CELL_FIELD_TILE_INDEX = 0,
    CELL_FIELD_OBJECT_TILESET = 1,
    CELL_FIELD_OBJECT_INDEX = 2,
    CELL_FIELD_OVERLAY_TILESET = 3,
    CELL_FIELD_OVERLAY_INDEX = 4,
    CELL_FIELD_EXTRA_FRAME = 5,
    CELL_FIELD_FLAGS = 6,
    CELL_FIELD_SECONDARY_TRIGGER = 7,
    CELL_FIELD_TRIGGER_TYPE = 8,
    CELL_FIELD_OBJECT_METADATA = 9,
    CELL_FIELD_OBJECT_ID = 10,
    CELL_FIELD_OVERLAY_ID = 11
H1_ENUM_CONST_END(CellWindowField)

// edittown.bin, monedit.bin and heroedit.bin share the garrison and owner
// rows: troop types, names and counts by army slot, and an owner radio
// button per owner value (FIRST_PLAYER + owner) from the town's unset owner
// MAP_TOWN_OWNER_UNSET (-2) and the neutral GAME_PLAYER_NONE (-1) up to the
// last player.
H1_ENUM_CONST_BEGIN(ExtraWindowConstant)
    EXTRA_WINDOW_FIRST_TYPE = 0x200,
    EXTRA_WINDOW_FIRST_NAME = 0x205,
    EXTRA_WINDOW_FIRST_COUNT = 0x20a,
    EXTRA_WINDOW_FIRST_PLAYER = 0x240,
    EXTRA_WINDOW_FIRST_OWNER = EXTRA_WINDOW_FIRST_PLAYER + MAP_TOWN_OWNER_UNSET,
    EXTRA_WINDOW_OWNER_END = EXTRA_WINDOW_FIRST_PLAYER + GAME_PLAYER_COUNT,
    EXTRA_WINDOW_MAX_COUNT = 999,
    // The last creature type (CREATURE_GENIE) a troop type field accepts.
    EXTRA_WINDOW_MAX_TYPE = 27,
    TOWN_WINDOW_CUSTOMIZED = 0x1f7,
    TOWN_WINDOW_FIRST_FIELD = 0x1f8,
    TOWN_WINDOW_LAST_FIELD = 0x243,
    // A toggle per building slot (BuildingSlotType) up to the last dwelling,
    // except the tent and castle slots the town type decides.
    TOWN_WINDOW_FIRST_BUILDING = 0x225,
    // The mage guild field shows town::m_mageGuildLevel + 1 (levels 1..4).
    TOWN_WINDOW_MAGE_GUILD = 0x230,
    TOWN_WINDOW_MIN_MAGE_GUILD = 1,
    TOWN_WINDOW_MAX_MAGE_GUILD = 4,
    // monedit.bin's stack count (mapCell::m_objectMetadata; 0 lets the game
    // choose it) is the first garrison count field.
    MONSTER_WINDOW_COUNT = EXTRA_WINDOW_FIRST_COUNT,
    MONSTER_WINDOW_MAX_COUNT = 127,
    HERO_WINDOW_HERO_ID = 0x215,
    HERO_WINDOW_FIRST_ARTIFACT = 0x216,
    HERO_WINDOW_EXPERIENCE = 0x21a,
    HERO_WINDOW_HERO_NAME = 0x21b,
    HERO_WINDOW_FIRST_ARTIFACT_NAME = 0x21c,
    // An artifact field shows the artifact id + 1 (0: none), up to the last
    // artifact before the magic book (ARTIFACT_REGULAR_END).
    HERO_WINDOW_MAX_ARTIFACT = 37,
    HERO_WINDOW_MAX_EXPERIENCE = 999999,
    // HeroWindowHandler's field: the edited control's offset from the hero id.
    HERO_FIELD_HERO_ID = 0,
    HERO_FIELD_FIRST_ARTIFACT = 1,
    HERO_FIELD_LAST_ARTIFACT = 4,
    HERO_FIELD_EXPERIENCE = 5
H1_ENUM_CONST_END(ExtraWindowConstant)

// clearwin.bin's toggles: one per gClearFlags bit; bits 0..19 are object
// classes, bit 20 erases them all, bit 21 erases the whole map at once.
H1_ENUM_CONST_BEGIN(ClearWindowConstant)
    CLEAR_WINDOW_FIRST_TOGGLE = 0x40,
    CLEAR_WINDOW_LAST_TOGGLE = 0x5f,
    CLEAR_WINDOW_TOGGLE_COUNT = 31,
    CLEAR_WINDOW_LAST_CLASS = 19,
    CLEAR_WINDOW_EVERYTHING = 20,
    CLEAR_FLAG_EVERY_CLASS = 0xfffff,
    CLEAR_FLAG_EVERYTHING = 0x100000,
    CLEAR_FLAG_WHOLE_MAP = 0x200000,
    CLEAR_FLAG_ALL = 0x7fffffff
H1_ENUM_CONST_END(ClearWindowConstant)

// dtlwind.bin: difficulty and size radio rows, and the name, description and
// file-code fields (the header's name and description per language).
H1_ENUM_CONST_BEGIN(DetailsWindowConstant)
    DETAILS_WINDOW_FIRST_DIFFICULTY = 0x1f4,
    DETAILS_WINDOW_DIFFICULTY_COUNT = 4,
    DETAILS_WINDOW_FIRST_SIZE = 0x258,
    DETAILS_WINDOW_SIZE_COUNT = 3,
    DETAILS_WINDOW_NAME = 0x320,
    DETAILS_WINDOW_DESCRIPTION = 0x321,
    DETAILS_WINDOW_MAP_CODE = 0x322,
    DETAILS_WINDOW_MAP_CODE_LENGTH = 4
H1_ENUM_CONST_END(DetailsWindowConstant)

// editnew.bin: a track and a knob (escroll.icn) per terrain and density row,
// the arrow buttons, the town placement pair and the save-unseen toggle.
H1_ENUM_CONST_BEGIN(NewMapWindowConstant)
    NEW_MAP_FIRST_TERRAIN_DECREASE = 100,
    NEW_MAP_FIRST_TERRAIN_INCREASE = 200,
    NEW_MAP_FIRST_TERRAIN_TRACK = 400,
    NEW_MAP_FIRST_TERRAIN_KNOB = 500,
    NEW_MAP_FIRST_DENSITY_DECREASE = 600,
    NEW_MAP_FIRST_DENSITY_INCREASE = 700,
    NEW_MAP_FIRST_DENSITY_TRACK = 900,
    NEW_MAP_FIRST_DENSITY_KNOB = 1000,
    NEW_MAP_SCATTER_TERRAIN = 1100,
    NEW_MAP_CENTRE_TERRAIN = 1101,
    NEW_MAP_SAVE_UNSEEN = 1300,
    NEW_MAP_TRACK_X = 154,
    NEW_MAP_TRACK_WIDTH = 250,
    NEW_MAP_TRACK_HEIGHT = 16,
    // A knob's x at 0 percent.
    NEW_MAP_KNOB_X = 157,
    NEW_MAP_KNOB_Y_OFFSET = 3,
    NEW_MAP_KNOB_WIDTH = 17,
    NEW_MAP_KNOB_HEIGHT = 8,
    NEW_MAP_ROW_HEIGHT = 24,
    NEW_MAP_FIRST_TERRAIN_Y = 26,
    NEW_MAP_FIRST_DENSITY_Y = 222,
    // A dragged knob travels from NEW_MAP_KNOB_X to 383 and is grabbed by its
    // middle; UpdateNewMapWindow places it over 227 pixels.
    NEW_MAP_KNOB_RIGHT = 383,
    NEW_MAP_KNOB_TRAVEL = 227,
    NEW_MAP_KNOB_HALF_WIDTH = NEW_MAP_KNOB_WIDTH / 2,
    // The land terrains keep at least 20 percent; water is capped at 75.
    NEW_MAP_MINIMUM_LAND = 20,
    NEW_MAP_MAXIMUM_WATER = 75
H1_ENUM_CONST_END(NewMapWindowConstant)

// Closes the running dialog: the dialog manager reads the select command.
#define FINISH_DIALOG_SELECT(message)                                                              \
    ((message).type = MESSAGE_WIDGET,                                                              \
     (message).command = H1_ENUM_DECODE(                                                           \
         BaseWidgetCommand,                                                                        \
         (message).id = H1_ENUM_ENCODE(BaseWidgetCommand, WIDGET_COMMAND_DIALOG_SELECT)            \
     ))

#pragma pack(push, 1)

class eventsManager : public baseManager {
public:
    // The cursor frame drawn over the hovered cell (overlay.icn).
    icon* m_cursorIcon;
    // The tool panel's backdrop.
    iconWidget* m_panel;
    // Main's mask over message.type (EDIT_MANAGER_DISPATCH_MASK).
    i16 m_dispatchMask;

    eventsManager(void);
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(tag_message& message) OVERRIDE;
    // The raw cell editor (debug level 1 and above).
    void EditCell(i16 x, i16 y);
    void EditTown(i16 x, i16 y);
    void UpdateTownWindow(editTownExtra* town);
    void EditMonster(i16 x, i16 y);
    void EditHero(i16 x, i16 y);
    void UpdateHeroWindow(editHeroExtra* hero);
};
#pragma pack(pop)

H1_ENUM_RETURN(MessageDispatchResult, i16) CellWindowHandler(tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) TownWindowHandler(tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) MonsterWindowHandler(tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) HeroWindowHandler(tag_message& message);
// Runs the eraser options window (clearwin.bin); on OK with the whole-map
// toggle set it erases the selected classes everywhere. Returns true on OK.
b32 ClearOptionsDialog(void);
void UpdateClearWindow(void);
H1_ENUM_RETURN(MessageDispatchResult, i16) ClearWindowHandler(tag_message& message);
// Edits the map header (dtlwind.bin); returns 0 when cancelled.
b32 MapDetailsDialog(b32 randomMap);
void UpdateMapDetailsWindow(void);
H1_ENUM_RETURN(MessageDispatchResult, i16) MapDetailsWindowHandler(tag_message& message);
// Sets up the random map generator (editnew.bin); returns false when
// cancelled.
b32 NewMapDialog(void);
void UpdateNewMapWindow(void);
// After a slider changed terrain changedTerrain, tops grass up to the land
// minimum. Without one (TERRAIN_INVALID, as the window closes) it scales the
// terrains to 100 percent less the share read at index -1, then caps water
// and rebalances from it.
void BalanceTerrainPercents(H1_ENUM_PARAM(TerrainType, i32) changedTerrain);
H1_ENUM_RETURN(MessageDispatchResult, i16) NewMapWindowHandler(tag_message& message);
// Drags a generator slider: a terrain row when terrainRow is set, else a
// density row.
void DragNewMapSlider(b32 terrainRow, i32 index);

extern editTownExtra gTownEdit;
extern editHeroExtra gHeroEdit;

#endif // HOMM1_EDITOR_EVENTSMANAGER_H
