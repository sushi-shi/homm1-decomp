#ifndef HOMM1_EDITOR_EVENTSMANAGER_H
#define HOMM1_EDITOR_EVENTSMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <SOURCE/game.h>

class heroWindow;
class icon;
class iconWidget;
struct tag_message;

enum EventsWindowText {
    EVENTS_WINDOW_TEXT_TOWN = 0x65,
    EVENTS_WINDOW_TEXT_MONSTER = 0x66,
    EVENTS_WINDOW_TEXT_HERO = 0x67,
    EVENTS_WINDOW_TEXT_CLEAR = 0x68,
    EVENTS_WINDOW_TEXT_MAP_DETAILS = 0x69,
    EVENTS_WINDOW_TEXT_NEW_MAP = 0x6a
};

enum EventsDialogButton {
EVENTS_DIALOG_CANCEL = DIALOG_BUTTON_1, EVENTS_DIALOG_OK =
                                            DIALOG_BUTTON_2 };

                                        enum EventsManagerLayout {
    EVENTS_DIALOG_X = 16,
    EVENTS_DIALOG_Y = 16,
    EVENTS_HOVER_COLOR = 1
};

enum CellWindowConstant {
    CELL_WINDOW_FIRST_BYTE = 0x2bc,
    CELL_WINDOW_FIRST_FLAG = 0x40,
    CELL_WINDOW_EVENT_TOGGLE = 0x48,
    CELL_WINDOW_BYTE_MASK = 0xff
};

enum CellWindowField {
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
};

enum ExtraWindowConstant {
    EXTRA_WINDOW_FIRST_TYPE = 0x200,
    EXTRA_WINDOW_FIRST_NAME = 0x205,
    EXTRA_WINDOW_FIRST_COUNT = 0x20a,
    EXTRA_WINDOW_FIRST_PLAYER = 0x240,
    EXTRA_WINDOW_FIRST_OWNER = EXTRA_WINDOW_FIRST_PLAYER + MAP_TOWN_OWNER_UNSET,
    EXTRA_WINDOW_OWNER_END = EXTRA_WINDOW_FIRST_PLAYER + GAME_PLAYER_COUNT,
    EXTRA_WINDOW_MAX_COUNT = 999,
    EXTRA_WINDOW_MAX_TYPE = 27,
    TOWN_WINDOW_CUSTOMIZED = 0x1f7,
    TOWN_WINDOW_FIRST_FIELD = 0x1f8,
    TOWN_WINDOW_LAST_FIELD = 0x243,
    TOWN_WINDOW_FIRST_BUILDING = 0x225,
    TOWN_WINDOW_MAGE_GUILD = 0x230,
    TOWN_WINDOW_MIN_MAGE_GUILD = 1,
    TOWN_WINDOW_MAX_MAGE_GUILD = 4,
    MONSTER_WINDOW_COUNT = EXTRA_WINDOW_FIRST_COUNT,
    MONSTER_WINDOW_MAX_COUNT = 127,
    HERO_WINDOW_HERO_ID = 0x215,
    HERO_WINDOW_FIRST_ARTIFACT = 0x216,
    HERO_WINDOW_EXPERIENCE = 0x21a,
    HERO_WINDOW_HERO_NAME = 0x21b,
    HERO_WINDOW_FIRST_ARTIFACT_NAME = 0x21c,
    HERO_WINDOW_MAX_ARTIFACT = 37,
    HERO_WINDOW_MAX_EXPERIENCE = 999999,
    HERO_FIELD_HERO_ID = 0,
    HERO_FIELD_FIRST_ARTIFACT = 1,
    HERO_FIELD_LAST_ARTIFACT = 4,
    HERO_FIELD_EXPERIENCE = 5
};

enum ClearWindowConstant {
    CLEAR_WINDOW_FIRST_TOGGLE = 0x40,
    CLEAR_WINDOW_LAST_TOGGLE = 0x5f,
    CLEAR_WINDOW_TOGGLE_COUNT = 31,
    CLEAR_WINDOW_LAST_CLASS = 19,
    CLEAR_WINDOW_EVERYTHING = 20,
    CLEAR_FLAG_EVERY_CLASS = 0xfffff,
    CLEAR_FLAG_EVERYTHING = 0x100000,
    CLEAR_FLAG_WHOLE_MAP = 0x200000,
    CLEAR_FLAG_ALL = 0x7fffffff
};

enum DetailsWindowConstant {
    DETAILS_WINDOW_FIRST_DIFFICULTY = 0x1f4,
    DETAILS_WINDOW_DIFFICULTY_COUNT = 4,
    DETAILS_WINDOW_FIRST_SIZE = 0x258,
    DETAILS_WINDOW_SIZE_COUNT = 3,
    DETAILS_WINDOW_NAME = 0x320,
    DETAILS_WINDOW_DESCRIPTION = 0x321,
    DETAILS_WINDOW_MAP_CODE = 0x322,
    DETAILS_WINDOW_MAP_CODE_LENGTH = 4
};

enum NewMapWindowConstant {
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
    NEW_MAP_KNOB_X = 157,
    NEW_MAP_KNOB_Y_OFFSET = 3,
    NEW_MAP_KNOB_WIDTH = 17,
    NEW_MAP_KNOB_HEIGHT = 8,
    NEW_MAP_ROW_HEIGHT = 24,
    NEW_MAP_FIRST_TERRAIN_Y = 26,
    NEW_MAP_FIRST_DENSITY_Y = 222,
    NEW_MAP_KNOB_RIGHT = 383,
    NEW_MAP_KNOB_TRAVEL = 227,
    NEW_MAP_KNOB_HALF_WIDTH = NEW_MAP_KNOB_WIDTH / 2,
    NEW_MAP_MINIMUM_LAND = 20,
    NEW_MAP_MAXIMUM_WATER = 75
};

#define FINISH_DIALOG_SELECT(message)                                                              \
    ((message).type = MESSAGE_WIDGET,                                                              \
     (message).command = ((message).id = WIDGET_COMMAND_DIALOG_SELECT))

#pragma pack(push, 1)

class eventsManager : public baseManager {
public:
    icon* m_cursorIcon;
    iconWidget* m_panel;
    i16 m_dispatchMask;

    eventsManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(tag_message& message) ;
    void EditCell(i16 x, i16 y);
    void EditTown(i16 x, i16 y);
    void UpdateTownWindow(editTownExtra* town);
    void EditMonster(i16 x, i16 y);
    void EditHero(i16 x, i16 y);
    void UpdateHeroWindow(editHeroExtra* hero);
};
#pragma pack(pop)

i16 CellWindowHandler(tag_message& message);
i16 TownWindowHandler(tag_message& message);
i16 MonsterWindowHandler(tag_message& message);
i16 HeroWindowHandler(tag_message& message);
b32 ClearOptionsDialog(void);
void UpdateClearWindow(void);
i16 ClearWindowHandler(tag_message& message);
i32 MapDetailsDialog(b32 randomMap);
void UpdateMapDetailsWindow(void);
i16 MapDetailsWindowHandler(tag_message& message);
b32 NewMapDialog(void);
void UpdateNewMapWindow(void);
void BalanceTerrainPercents(i32 changedTerrain);
i16 NewMapWindowHandler(tag_message& message);
void DragNewMapSlider(b32 terrainRow, i32 index);

extern editTownExtra gTownEdit;
extern editHeroExtra gHeroEdit;

#endif
