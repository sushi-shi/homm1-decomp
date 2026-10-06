#ifndef HOMM1_EDITOR_EVENTSMANAGER_H
#define HOMM1_EDITOR_EVENTSMANAGER_H

// The map-information tool and the editor dialogs of its unit
// (src/EDITOR/EVENTMGR.cpp). The unit name EVENTMGR is descriptive; Open
// stores the class name "eventsManager". The dialog, record and global names
// are descriptive: the windows are cellwin.bin, edittown.bin, monedit.bin,
// heroedit.bin, clearwin.bin, dtlwind.bin and editnew.bin.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/game.h>

class heroWindow;
class icon;
class iconWidget;
struct tag_message;

// Main tests message.type against the dispatch mask the managers share.
H1_ENUM_CONST_BEGIN(EventsManagerConstant)
    EVENTS_MANAGER_DISPATCH_MASK = 0x32f,
    // EditCell opens only from this debug level.
    EVENTS_CELL_EDIT_DEBUG_LEVEL = 1,
    // The hover cursor sits on 32-pixel cells (16 zoomed) from the map view's
    // origin at (16, 16).
    EVENTS_CELL_SIZE = 32,
    EVENTS_ZOOMED_CELL_SIZE = 16,
    EVENTS_MAP_VIEW_ORIGIN = 16
H1_ENUM_CONST_END(EventsManagerConstant)

// The tool panel (a buttons.icn frame).
H1_ENUM_CONST_BEGIN(EventsManagerLayout)
    EVENTS_PANEL_X = 480,
    EVENTS_PANEL_Y = 197,
    EVENTS_PANEL_WIDTH = 144,
    EVENTS_PANEL_HEIGHT = 139,
    EVENTS_PANEL_FRAME = 37,
    // Every dialog opens at (16, 16).
    EVENTS_DIALOG_X = 16,
    EVENTS_DIALOG_Y = 16
H1_ENUM_CONST_END(EventsManagerLayout)

// cellwin.bin: a text field per cell byte and per pair word, and toggles for
// the bits of the cell's seventh byte and the object type's top bit.
H1_ENUM_CONST_BEGIN(CellWindowConstant)
    CELL_WINDOW_FIRST_BYTE = 0x2bc,
    CELL_WINDOW_FIRST_FLAG = 0x40,
    CELL_WINDOW_OBJECT_FLAG = 0x48,
    CELL_WINDOW_OBJECT_FLAG_BIT = 0x80,
    CELL_WINDOW_NIBBLE_MAX = 15,
    CELL_WINDOW_BYTE_MASK = 0xff
H1_ENUM_CONST_END(CellWindowConstant)

// edittown.bin, monedit.bin and heroedit.bin share the garrison and owner
// rows: troop types, names and counts by slot, and owner radio buttons from
// the unset owner (-2) up.
H1_ENUM_CONST_BEGIN(ExtraWindowConstant)
    EXTRA_WINDOW_FIRST_TYPE = 0x200,
    EXTRA_WINDOW_FIRST_NAME = 0x205,
    EXTRA_WINDOW_FIRST_COUNT = 0x20a,
    EXTRA_WINDOW_FIRST_OWNER = 0x23e,
    EXTRA_WINDOW_NEUTRAL_OWNER = 0x240,
    EXTRA_WINDOW_OWNER_END = 0x244,
    EXTRA_WINDOW_MAX_COUNT = 999,
    EXTRA_WINDOW_MAX_TYPE = 27,
    TOWN_WINDOW_CUSTOMIZED = 0x1f7,
    TOWN_WINDOW_FIRST_FIELD = 0x1f8,
    TOWN_WINDOW_LAST_FIELD = 0x243,
    TOWN_WINDOW_FIRST_BUILDING = 0x225,
    TOWN_WINDOW_MAGE_GUILD = 0x230,
    TOWN_WINDOW_MAX_MAGE_GUILD = 4,
    // Building bits 0..12 have toggles but the tent and castle bits.
    TOWN_WINDOW_LAST_BUILDING_BIT = 12,
    TOWN_WINDOW_TENT_BIT = 5,
    TOWN_WINDOW_CASTLE_BIT = 6,
    MONSTER_WINDOW_COUNT = 0x20a,
    MONSTER_WINDOW_MAX_COUNT = 127,
    HERO_WINDOW_HERO_ID = 0x215,
    HERO_WINDOW_FIRST_ARTIFACT = 0x216,
    HERO_WINDOW_EXPERIENCE = 0x21a,
    HERO_WINDOW_HERO_NAME = 0x21b,
    HERO_WINDOW_FIRST_ARTIFACT_NAME = 0x21c,
    HERO_WINDOW_MAX_HERO_ID = 35,
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
    CLEAR_FLAG_CLASS_MASK = 0xffff,
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
    NEW_MAP_SCATTER_TOWNS = 1100,
    NEW_MAP_CENTRE_TOWNS = 1101,
    NEW_MAP_SAVE_UNSEEN = 1300,
    NEW_MAP_TRACK_X = 154,
    NEW_MAP_TRACK_WIDTH = 250,
    NEW_MAP_TRACK_HEIGHT = 16,
    NEW_MAP_TRACK_FRAME = 20,
    NEW_MAP_KNOB_X = 157,
    NEW_MAP_KNOB_Y_OFFSET = 3,
    NEW_MAP_KNOB_WIDTH = 17,
    NEW_MAP_KNOB_HEIGHT = 8,
    NEW_MAP_KNOB_FRAME = 2,
    NEW_MAP_ROW_HEIGHT = 24,
    NEW_MAP_FIRST_TERRAIN_Y = 26,
    NEW_MAP_FIRST_DENSITY_Y = 222,
    // A knob travels from x 157 to 383; the window lies 16 pixels in and a
    // knob is grabbed by its middle.
    NEW_MAP_KNOB_LEFT = 157,
    NEW_MAP_KNOB_RIGHT = 383,
    NEW_MAP_KNOB_TRAVEL = 227,
    NEW_MAP_DRAG_X_OFFSET = 16,
    NEW_MAP_KNOB_HALF_WIDTH = 8,
    // The land terrains keep at least 20 percent; water is capped at 75.
    NEW_MAP_MINIMUM_LAND = 20,
    NEW_MAP_MAXIMUM_WATER = 75
H1_ENUM_CONST_END(NewMapWindowConstant)

// mouseManager cursor shapes the slider drag switches between.
H1_ENUM_CONST_BEGIN(EventsCursorConstant)
    EVENTS_CURSOR_SLIDER = 2,
    EVENTS_CURSOR_NORMAL = 6
H1_ENUM_CONST_END(EventsCursorConstant)

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
// toggle set it erases the selected classes everywhere. Returns 1 on OK.
i32 ClearOptionsDialog(void);
void UpdateClearWindow(void);
H1_ENUM_RETURN(MessageDispatchResult, i16) ClearWindowHandler(tag_message& message);
// Edits the map header (dtlwind.bin); returns 0 when cancelled.
i32 MapDetailsDialog(b32 randomMap);
void UpdateMapDetailsWindow(void);
H1_ENUM_RETURN(MessageDispatchResult, i16) MapDetailsWindowHandler(tag_message& message);
// Sets up the random map generator (editnew.bin); returns 0 when cancelled.
i32 NewMapDialog(void);
void UpdateNewMapWindow(void);
// Rescales the other terrains after terrain `fixed` changed (-1: none).
void BalanceTerrainPercents(i32 fixed);
H1_ENUM_RETURN(MessageDispatchResult, i16) NewMapWindowHandler(tag_message& message);
// Drags a generator slider: a terrain row when `terrain` is set, else a
// density row.
void DragNewMapSlider(b32 terrain, i32 index);

extern editTownExtra gTownEdit;
extern editHeroExtra gHeroEdit;

#endif // HOMM1_EDITOR_EVENTSMANAGER_H
