// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/backdropWidget.h>
#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/BITS.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/soundManager.h>
#include <BASE/soundmgr.h>
#include <BASE/textWidget.h>
#include <BASE/TILE.h>
#include <BASE/tileset.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The route-overlay byte at (column, row) of this->m_visibilityMap (Buka 2.1
// ADVMGR.cpp; HoMM1 indexes row-major as row * size + column).
#define ADVMGR_VISIBILITY_AT(column, row) (m_visibilityMap[(row) * MAP_CELL_GRID_SIZE + (column)])

// Buka's giSeedingValid is the dword zeroed by retail Reseed at VA 0x4c5170.
// Code-use identity only; no initializer-byte coverage is asserted.
extern int giSeedingValid;

// DrawCell's per-call drawing state, kept in module storage as in Buka, which
// defines it ahead of its functions (retail address order).
DATA(0x004c4f48)
int s_drawCloudFrame;
DATA(0x004c4f70)
unsigned short s_drawGroundTile;
DATA(0x004c509c)
signed char s_drawFlipCloud;
DATA(0x004c50a4)
unsigned char s_drawTileset;
DATA(0x004c50a8)
int s_drawCovered;
DATA(0x004c50ac)
int s_drawStoneTile;

H1_ENUM_CONST_BEGIN(AdventureButtonConstant)
    BUTTON_BROADCAST_ARG = 1,
    PANEL_CONTINUE_ROUTE = 2
H1_ENUM_CONST_END(AdventureButtonConstant)

H1_ENUM_CONST_BEGIN(AdventureScreenConstant)
    SCROLL_BORDER = 16
H1_ENUM_CONST_END(AdventureScreenConstant)

H1_ENUM_CONST_BEGIN(AdventureBorderConstant)
    ADVENTURE_VIEWPORT_EXTENT = 480,
    BORDER_EDGE_SIZE = 16,
    BORDER_SIDE_BYTES = 16,
    BORDER_SAVED_SIDE_BYTES = 32,
    BORDER_MIDDLE_END = 464,
    BORDER_BUFFER_SIZE = 0x7400
H1_ENUM_CONST_END(AdventureBorderConstant)

// adv_wind.bin hero locator rows: seven widgets per slot from
// HERO_LOCATOR_WIDGET_BASE + slot * HERO_LOCATOR_WIDGET_STRIDE (UpdateHeroLocator);
// +1 is the mobility bar, +2 the portrait, +5 the clickable
// ADVENTURE_CONTROL_HERO_LOCATOR_n and +6 the selection frame. The town
// column's selection frames start at TOWN_LOCATOR_HIGHLIGHT_FIRST; the
// selected entry's frame takes LOCATOR_HIGHLIGHT_COLOR.
H1_ENUM_CONST_BEGIN(AdventureLocatorWidget)
    HERO_LOCATOR_WIDGET_BASE = 100,
    HERO_LOCATOR_WIDGET_STRIDE = 7,
    HERO_LOCATOR_MOBILITY = 1,
    HERO_LOCATOR_PORTRAIT = 2,
    HERO_LOCATOR_BUTTON = 5,
    HERO_LOCATOR_HIGHLIGHT = 6,
    TOWN_LOCATOR_HIGHLIGHT_FIRST = 32,
    LOCATOR_HIGHLIGHT_COLOR = 0xc5
H1_ENUM_CONST_END(AdventureLocatorWidget)

// locators.icn frames: the empty hero slots (one per slot), the empty town
// slots from EMPTY_TOWN_FIRST, the occupied hero frame, and the town frames
// by town type from TOWN_FIRST, CASTLE_OFFSET further on once it has a castle.
// m_visibilityMap while a route is shown (ShowRoute, DrawCell): a 1-based
// route.icn frame (FRAME_MASK) with FLIPPED mirroring it. The last step is
// the DESTINATION mark; steps the hero reaches today move REACHABLE_OFFSET
// frames on to the second arrow set.
H1_ENUM_CONST_BEGIN(AdventureRouteCell)
    ROUTE_CELL_FRAME_MASK = 0x1f,
    ROUTE_CELL_FLIPPED = 0x20,
    ROUTE_CELL_DESTINATION = 14,
    ROUTE_CELL_REACHABLE_OFFSET = 14
H1_ENUM_CONST_END(AdventureRouteCell)

H1_ENUM_CONST_BEGIN(AdventureLocatorFrame)
    LOCATOR_FRAME_EMPTY_TOWN_FIRST = 4,
    LOCATOR_FRAME_HERO = 8,
    LOCATOR_FRAME_TOWN_FIRST = 12,
    LOCATOR_FRAME_CASTLE_OFFSET = 4
H1_ENUM_CONST_END(AdventureLocatorFrame)

H1_ENUM_CONST_BEGIN(BottomViewPanelConstant)
    BOTTOM_VIEW_DRAW_FIRST_WIDGET = 2000,
    BOTTOM_VIEW_DRAW_LAST_WIDGET = 2200,
    BOTTOM_VIEW_PANEL_X = 480,
    BOTTOM_VIEW_PANEL_Y = 392,
    BOTTOM_VIEW_PANEL_WIDTH = 143,
    BOTTOM_VIEW_PANEL_HEIGHT = 71,
    // Buka 2.1 AdventureBottomViewConstant names: the stone backdrop is wider
    // than the panel; the backdrop/foreground icons and the text widgets take
    // these ids; the text and count buffers are malloc'd at these sizes.
    BOTTOM_VIEW_BACKGROUND_WIDTH = 159,
    BOTTOM_VIEW_BACKGROUND_ID = 2000,
    BOTTOM_VIEW_FOREGROUND_ID = 2001,
    BOTTOM_VIEW_TEXT_ID = 2100,
    BOTTOM_VIEW_TEXT_ID_2 = 2101,
    BOTTOM_VIEW_TEXT_BUFFER_SIZE = 30,
    BOTTOM_VIEW_COUNT_BUFFER_SIZE = 8,
    // Buka 2.1 names: no enemy turn drawn yet, no hourglass frame shown.
    BOTTOM_VIEW_NO_ENEMY = -1,
    BOTTOM_VIEW_NO_ANIMATION = -1
H1_ENUM_CONST_END(BottomViewPanelConstant)

// UpdBottomViewEnemyTurn's hourglass panel (Buka 2.1
// AdventureEnemyTurnViewConstant names, same values): the hourglass,
// running-sand and crest icons, their widget ids and z-orders, the sand
// frame cycle and the animation delays.
// The quick views' army rows (Buka 2.1 AdventureArmyQuickViewConstant,
// AdventureHeroQuickViewConstant and AdventureTownQuickViewConstant names,
// HoMM1 values): 32-pixel creature icons over a 12-pixel label 30 below,
// rows of up to three (the vague layout puts two over three, the first row
// 22 lower when there is only one row, the second 44 lower), a five-stack
// first row nudged 12 pixels apart, label buffers, the windows' default
// positions, and how much a town view reveals (thieves' guilds).
H1_ENUM_CONST_BEGIN(AdventureArmyQuickViewConstant)
    ARMY_QUICK_ICON_SIZE = 32,
    ARMY_QUICK_ICON_BASELINE = 30,
    ARMY_QUICK_AREA_LEFT = 9,
    ARMY_QUICK_LABEL_HEIGHT = 12,
    ARMY_QUICK_FIRST_ROW_SHIFT = 22,
    ARMY_QUICK_SECOND_ROW_SHIFT = 44,
    ARMY_QUICK_FIRST_ROW_COUNT = 2,
    ARMY_QUICK_FIVE_STACK_X_SHIFT = 12,
    ARMY_QUICK_TEXT_WIDTH = 60,
    ARMY_QUICK_TEXT_X_ADJUSTMENT = 14,
    ARMY_QUICK_SIZE_LABEL_CAPACITY = 15,
    HERO_QUICK_ARMY_AREA_WIDTH = 160,
    HERO_QUICK_DETAILED_CREATURE_Y = 110,
    HERO_QUICK_DETAILED_LABEL_Y = 140,
    HERO_QUICK_VAGUE_FIRST_ROW_Y = 65,
    HERO_QUICK_DEFAULT_WINDOW_X = 302,
    HERO_QUICK_LOCATOR_ROW_HEIGHT = 30,
    HERO_QUICK_LOCATOR_BASE_Y = 111,
    HERO_QUICK_ARMY_LABEL_CAPACITY = 5,
    QUICK_VIEW_FLAG_COLOR_STRIDE = 2,
    TOWN_QUICK_ARMY_AREA_WIDTH = 192,
    TOWN_QUICK_FIRST_ROW_Y = 75,
    TOWN_QUICK_DEFAULT_WINDOW_X = 342,
    TOWN_QUICK_DEFAULT_WINDOW_Y = 176,
    TOWN_QUICK_TYPE_FRAME_BASE = 12,
    TOWN_QUICK_CASTLE_FRAME_OFFSET = 4,
    TOWN_QUICK_EMPTY_LABEL_CAPACITY = 20,
    TOWN_QUICK_EMPTY_LABEL_X = 0,
    TOWN_QUICK_EMPTY_LABEL_Y = 100,
    TOWN_QUICK_EMPTY_LABEL_WIDTH = 210
H1_ENUM_CONST_END(AdventureArmyQuickViewConstant)

// TownQuickView's detail level: the owner sees exact counts; others see as
// much as their thieves' guilds reveal, capped at size names (Buka 2.1
// TOWN_QUICK_INFORMATION_* values).
H1_ENUM_BEGIN(TownQuickInformation)
    TOWN_QUICK_INFORMATION_UNKNOWN = 0,
    TOWN_QUICK_INFORMATION_NAMES = 1,
    TOWN_QUICK_INFORMATION_ESTIMATES = 2,
    TOWN_QUICK_INFORMATION_EXACT = 3,
    // The most a rival's thieves' guilds can reveal.
    TOWN_QUICK_INFORMATION_THIEVES_LAST = TOWN_QUICK_INFORMATION_ESTIMATES
H1_ENUM_END(TownQuickInformation)

// Right-click quick views over the map: the window is offset from the
// clicked cell and clamped inside the viewport's inner box (Buka 2.1
// AdventureQuickViewPlacementConstant / AdventureQuickInfoObject names,
// HoMM1 sizes; the right/bottom limits are the box edge minus the size).
H1_ENUM_CONST_BEGIN(AdventureQuickViewPlacementConstant)
    QUICK_VIEW_MIN_X = 16,
    QUICK_VIEW_MIN_Y = 16,
    QUICK_VIEW_RIGHT = 464,
    QUICK_VIEW_BOTTOM = 464,
    HERO_QUICK_VIEW_X_OFFSET = 73,
    HERO_QUICK_VIEW_Y_OFFSET = 65,
    HERO_QUICK_VIEW_WIDTH = 178,
    HERO_QUICK_VIEW_HEIGHT = 162,
    HERO_QUICK_VIEW_RIGHT_X = QUICK_VIEW_RIGHT - HERO_QUICK_VIEW_WIDTH,
    HERO_QUICK_VIEW_BOTTOM_Y = QUICK_VIEW_BOTTOM - HERO_QUICK_VIEW_HEIGHT,
    TOWN_QUICK_VIEW_X_OFFSET = 89,
    TOWN_QUICK_VIEW_Y_OFFSET = 70,
    TOWN_QUICK_VIEW_WIDTH = 210,
    TOWN_QUICK_VIEW_HEIGHT = 172,
    TOWN_QUICK_VIEW_RIGHT_X = QUICK_VIEW_RIGHT - TOWN_QUICK_VIEW_WIDTH,
    TOWN_QUICK_VIEW_BOTTOM_Y = QUICK_VIEW_BOTTOM - TOWN_QUICK_VIEW_HEIGHT,
    QUICK_INFO_X_OFFSET = 57,
    QUICK_INFO_Y_OFFSET = 25,
    QUICK_INFO_WIDTH = 146,
    QUICK_INFO_HEIGHT = 82,
    QUICK_INFO_RIGHT_X = QUICK_VIEW_RIGHT - QUICK_INFO_WIDTH,
    QUICK_INFO_BOTTOM_Y = QUICK_VIEW_BOTTOM - QUICK_INFO_HEIGHT
H1_ENUM_CONST_END(AdventureQuickViewPlacementConstant)

// DrawCell's sprite layout (Buka 2.1 AdventureDrawConstant names, HoMM1
// values): cell pixels, the stone border tiles around the map (corners, then
// four-tile runs per side picked by the coordinate's low bits, the inner
// pattern offset by 16), the cloud variants and flipped/alternate frames,
// the route arrow's y offset, minimon frames (seven per creature, the
// facing frame last), the boat's y offset and the hero frame's mirror bit.
H1_ENUM_CONST_BEGIN(AdventureDrawConstant)
    CELL_PIXELS = 32,
    CELL_PIXEL_SHIFT = 5,
    CELL_LAST_PIXEL = CELL_PIXELS - 1,
    STONE_TILE_NONE = -1,
    STONE_TILE_TOP_LEFT = 16,
    STONE_TILE_TOP_RIGHT = 17,
    STONE_TILE_BOTTOM_RIGHT = 18,
    STONE_TILE_BOTTOM_LEFT = 19,
    STONE_TILE_TOP_BASE = 20,
    STONE_TILE_RIGHT_BASE = 24,
    STONE_TILE_BOTTOM_BASE = 28,
    STONE_TILE_LEFT_BASE = 32,
    STONE_PATTERN_COORDINATE_SHIFT = 16,
    CLOUD_VARIANTS = 4,
    CLOUD_VARIANT_MASK = CLOUD_VARIANTS - 1,
    CLOUD_FLIPPED_FRAME_BASE = 100,
    CLOUD_X_ALTERNATE_FRAME_1 = 1,
    CLOUD_Y_ALTERNATE_FRAME = 3,
    CLOUD_X_ALTERNATE_FRAME_2 = 5,
    ROUTE_DRAW_Y_OFFSET = 2,
    MONSTER_FRAME_STRIDE = 7,
    MONSTER_FACING_FRAME_BASE = MONSTER_FRAME_STRIDE - 1,
    MONSTER_DRAW_Y_OFFSET = 5,
    HERO_BOAT_Y_OFFSET = -10
H1_ENUM_CONST_END(AdventureDrawConstant)

// UpdateScreen's dirty box and animation clock (Buka 2.1
// AdventureUpdateScreenConstant / AdventureAnimationPhaseIndex names; HoMM1
// cycles m_updateMaxX through 6 steps and starts the columns at 0/1/3/5):
// no limit box means the whole 448-pixel viewport at 16,16; odd steps
// advance columns 1 and 3, even ones 0 and 2, each modulo 6 frames.
H1_ENUM_CONST_BEGIN(AdventureUpdateScreenConstant)
    UPDATE_VIEWPORT_ORIGIN = 16,
    UPDATE_VIEWPORT_SIZE = 448,
    UPDATE_ANIMATION_PHASES = 6,
    UPDATE_FRAME_CYCLE = 6
H1_ENUM_CONST_END(AdventureUpdateScreenConstant)

H1_ENUM_CONST_BEGIN(AdventureAnimationPhaseIndex)
    ANIMATION_PHASE_COLUMN_0 = 0,
    ANIMATION_PHASE_COLUMN_1 = 1,
    ANIMATION_PHASE_COLUMN_2 = 2,
    ANIMATION_PHASE_COLUMN_3 = 3,
    ANIMATION_PHASE_COLUMN_0_INITIAL = 0,
    ANIMATION_PHASE_COLUMN_1_INITIAL = 1,
    ANIMATION_PHASE_COLUMN_2_INITIAL = 3,
    ANIMATION_PHASE_COLUMN_3_INITIAL = 5
H1_ENUM_CONST_END(AdventureAnimationPhaseIndex)

// advManager::ViewWorld's 6-pixel map (HoMM1's own view; window position
// as Buka 2.1 Viewwrld's WORLD_WINDOW_X/Y): cells start 24 pixels in;
// ground6.icn has one frame per four ground tiles, vertically flipped tiles
// 31 frames on and horizontally flipped ones drawn one cell-width minus one
// to the right; tilesets[] is indexed by MapTileset; flag6.icn frames 0..3
// are player colours, 5 marks the current hero and 6 an artifact; town flags
// straddle the cell and resource letters sit 3 pixels left.
H1_ENUM_CONST_BEGIN(ViewWorldConstant)
    WORLD_WINDOW_X = 480,
    WORLD_WINDOW_Y = 16,
    VIEW_WORLD_CELL_PIXELS = 6,
    VIEW_WORLD_ORIGIN = 24,
    VIEW_WORLD_TILESET_COUNT = 16,
    VIEW_WORLD_GROUND_TILE_SHIFT = 2,
    VIEW_WORLD_GROUND_FLIPPED_FRAMES = 31,
    VIEW_WORLD_FLAG_CURRENT_HERO = 5,
    VIEW_WORLD_FLAG_ARTIFACT = 6,
    VIEW_WORLD_TOWN_FLAG_LEFT = 4,
    VIEW_WORLD_TOWN_FLAG_RIGHT = 3,
    VIEW_WORLD_RESOURCE_X_SHIFT = 3
H1_ENUM_CONST_END(ViewWorldConstant)

// The radar panel (Buka 2.1 AdventureScreenConstant/AdventureRadarConstant
// names, HoMM1 values): 480..624 x 16..160, two pixels per map cell; trees
// and mountains darken the terrain colour by 3 shades and the viewport box
// is drawn in colour 0xbe.
H1_ENUM_CONST_BEGIN(AdventureRadarConstant)
    RADAR_LEFT = 480,
    RADAR_RIGHT = 624,
    RADAR_TOP = 16,
    RADAR_BOTTOM = 160,
    RADAR_SIZE = RADAR_RIGHT - RADAR_LEFT,
    RADAR_CELL_PIXELS = 2,
    RADAR_TERRAIN_SHADE = 3,
    RADAR_VIEWPORT_COLOR = 0xbe
H1_ENUM_CONST_END(AdventureRadarConstant)

// TeleportTo's fizzle (Buka 2.1 AdventureTeleportConstant names; the
// computed time is not passed on - FizzleForward gets the default delay).
H1_ENUM_CONST_BEGIN(AdventureTeleportConstant)
    TELEPORT_FIZZLE_TIME = 128,
    TELEPORT_REMOTE_FIZZLE_ADJUSTMENT = 64
H1_ENUM_CONST_END(AdventureTeleportConstant)

// SummonBoat (Buka 2.1 AdventureSummonBoatConstant names, HoMM1 values): a
// boat whose heroId has OCCUPIED_FLAG carries that hero; the old berth is
// restored with mode 5; the fizzle boxes around the old berth (clamped to
// the viewport's inner box) and at the hero.
H1_ENUM_CONST_BEGIN(AdventureSummonBoatConstant)
    SUMMON_OCCUPIED_FLAG = 0x80,
    SUMMON_RESTORE_MODE = 5,
    SUMMON_SCREEN_MARGIN = 16,
    SUMMON_SCREEN_LIMIT = 464,
    SUMMON_FIZZLE_X_OFFSET = 32,
    SUMMON_FIZZLE_Y_OFFSET = 16,
    SUMMON_FIZZLE_WIDTH = 96,
    SUMMON_FIZZLE_HEIGHT = 48,
    SUMMON_TARGET_X = 176,
    SUMMON_TARGET_Y = 192,
    SUMMON_TARGET_WIDTH = 128,
    SUMMON_TARGET_HEIGHT = 96
H1_ENUM_CONST_END(AdventureSummonBoatConstant)

// ViewPuzzle (Buka 2.1 AdventurePuzzleViewConstant names): puzzle.icn has
// one piece per obelisk bit (playerData::m_obelisksVisited); the window sits
// beside the viewport; the view centre is nudged off the artifact by
// coordinate residues mod 3 (and mod 2), then the uncovered pieces fizzle
// in over 220 ms.
H1_ENUM_CONST_BEGIN(AdventurePuzzleViewConstant)
    PUZZLE_PIECE_COUNT = 48,
    PUZZLE_WINDOW_X = 480,
    PUZZLE_WINDOW_Y = 16,
    PUZZLE_ALIGNMENT_DIVISOR = 3,
    PUZZLE_Y_ADJUST_X_FACTOR = 2,
    PUZZLE_Y_ADJUST_Y_FACTOR = 5,
    PUZZLE_PARITY_DIVISOR = 2,
    PUZZLE_FIZZLE_TIME = 220
H1_ENUM_CONST_END(AdventurePuzzleViewConstant)

// Buka 2.1 AdventureStateConstant / AdventureOpenConstant names, HoMM1 values:
// the network-turn music hold, the walk
// sample set and volume, and the looping-sample budget per high-memory unit.
H1_ENUM_CONST_BEGIN(AdventureStateConstant)
    FORCED_MUSIC_DELAY = 6000,
    CURSOR_SAMPLE_FAST_SET = 2,
    CURSOR_SAMPLE_VOLUME = 0x40,
    HIGH_MEMORY_BUFFER_DIVISOR = 100,
    // Open's locator scroll knobs (scroll.icn frame 4).
    SCROLL_Y = 195,
    SCROLL_LEFT_X = 540,
    SCROLL_RIGHT_X = 612,
    SCROLL_WIDTH = 8,
    SCROLL_HEIGHT = 17,
    SCROLL_ICON_FRAME = 4
H1_ENUM_CONST_END(AdventureStateConstant)

// SetEnvironmentOrigin/InsertSound's looping map sounds (Buka 2.1
// AdventureEnvironmentSoundConstant names): slots reset to the far volume
// index, two passes (refresh known sounds, then insert new ones) over rings
// whose edges span radius * 2 cells, sounds beyond MAX_DISTANCE stop, and
// the loops play on channel type 3.
H1_ENUM_CONST_BEGIN(AdventureEnvironmentSoundConstant)
    ENVIRONMENT_SOUND_DEFAULT_VOLUME = 127,
    ENVIRONMENT_SOUND_MAX_DISTANCE = 5,
    ENVIRONMENT_SOUND_FIRST_LAYER = 1,
    ENVIRONMENT_SOUND_LAYER_COUNT = 2,
    ENVIRONMENT_SOUND_CHANNEL_TYPE = 3,
    ENVIRONMENT_SOUND_EDGE_SPAN = 2,
    // SetEnvironmentOrigin's rings around the origin (radius 0..COUNT-1).
    ENVIRONMENT_SOUND_RADIUS_COUNT = 4,
    // InsertSound found no slot to take over.
    ENVIRONMENT_SOUND_NO_SLOT = -1
H1_ENUM_CONST_END(AdventureEnvironmentSoundConstant)

// ComboDraw's dirty-cell grid (Buka 2.1 AdventureComboDrawConstant names):
// cloud-covered neighbours of a moving sprite get CLOUD_MARK so the cloud
// pass redraws them, animation runs every FRAME_LIMIT frame steps, the
// cursor marks two cells right, and the update box is clipped to the
// viewport's inner pixels.
H1_ENUM_CONST_BEGIN(AdventureComboDrawConstant)
    COMBO_CLEAR_BYTES = 256,
    COMBO_CLOUD_MARK = 10,
    COMBO_FRAME_LIMIT = 12,
    COMBO_UPDATE_MIN = 16,
    COMBO_UPDATE_MAX = 463,
    COMBO_FAR_NEIGHBOR_OFFSET = 2
H1_ENUM_CONST_END(AdventureComboDrawConstant)

// advManager::Main's debug keys and digit cheat (Buka 2.1
// AdventureCheatConstant names, HoMM1 values): the typed digits roll into a
// six-digit sequence; 101495 reveals the whole map to every player.
H1_ENUM_CONST_BEGIN(AdventureCheatConstant)
    CHEAT_SEQUENCE_RADIX = 10,
    CHEAT_SEQUENCE_MODULUS = 1000000,
    CHEAT_REVEAL_MAP = 101495,
    CHEAT_RESOURCE_AMOUNT = 10,
    CHEAT_GOLD_AMOUNT = 1000,
    CHEAT_EXPERIENCE_AMOUNT = 800,
    CHEAT_SPELL_CHARGES = 5,
    CHEAT_MOBILITY = 2999,
    CHEAT_REVEAL_CENTER = 30,
    CHEAT_REVEAL_RADIUS = 100
H1_ENUM_CONST_END(AdventureCheatConstant)

H1_ENUM_CONST_BEGIN(AdventureEnemyTurnViewConstant)
    ENEMY_TURN_HOURGLASS_X = 493,
    ENEMY_TURN_HOURGLASS_Y = 403,
    ENEMY_TURN_HOURGLASS_WIDTH = 118,
    ENEMY_TURN_HOURGLASS_HEIGHT = 51,
    ENEMY_TURN_CREST_X = 495,
    ENEMY_TURN_ANIMATION_X = 559,
    ENEMY_TURN_ANIMATION_Y = 405,
    ENEMY_TURN_ANIMATION_WIDTH = 50,
    ENEMY_TURN_ANIMATION_HEIGHT = 47,
    ENEMY_TURN_CREST_ID = 2002,
    ENEMY_TURN_SAND_ID = 2003,
    ENEMY_TURN_PHASE_ID = 2004,
    ENEMY_TURN_BACKGROUND_Z = 1000,
    ENEMY_TURN_HOURGLASS_Z = 1010,
    ENEMY_TURN_SAND_Z = 1020,
    ENEMY_TURN_CREST_Z = 1030,
    ENEMY_TURN_PHASE_Z = 1040,
    ENEMY_TURN_SAND_FRAME_OFFSET = 11,
    ENEMY_TURN_SAND_FRAME_LIMIT = 20,
    ENEMY_TURN_SAND_RESTART_FRAME = 16,
    ENEMY_TURN_PHASE_FRAME_OFFSET = 1,
    ENEMY_TURN_CREST_SLOT = 0,
    ENEMY_TURN_SAND_SLOT = 1,
    ENEMY_TURN_PHASE_SLOT = 2,
    ENEMY_TURN_ANIMATION_DELAY = 300,
    ENEMY_TURN_PHASE_DELAY = 700
H1_ENUM_CONST_END(AdventureEnemyTurnViewConstant)

// UpdBottomViewNewTurn's date texts (Buka AdventureNewTurnViewConstant
// names; HoMM1 places the week line at 421).
H1_ENUM_CONST_BEGIN(AdventureNewTurnViewConstant)
    NEW_TURN_DATE_TEXT_X = 479,
    NEW_TURN_WEEK_TEXT_Y = 421,
    NEW_TURN_DAY_TEXT_Y = 438,
    NEW_TURN_DATE_TEXT_WIDTH = 145,
    NEW_TURN_WEEK_TEXT_HEIGHT = 12,
    NEW_TURN_DAY_TEXT_HEIGHT = 25
H1_ENUM_CONST_END(AdventureNewTurnViewConstant)

// UpdBottomViewResMsg's message and resource layout (Buka
// AdventureResourceViewConstant names; HoMM1's text starts at 395 and the
// count at 450).
H1_ENUM_CONST_BEGIN(AdventureResourceViewConstant)
    RESOURCE_VIEW_TEXT_BASE_Y = 395,
    RESOURCE_VIEW_MULTILINE_HEIGHT = 32,
    RESOURCE_VIEW_LINE_HEIGHT = 6,
    RESOURCE_VIEW_TEXT_HEIGHT = 36,
    RESOURCE_VIEW_GOLD_WIDTH = 76,
    RESOURCE_VIEW_GOLD_HEIGHT = 26,
    RESOURCE_VIEW_ICON_WIDTH = 38,
    RESOURCE_VIEW_ICON_HEIGHT = 32,
    RESOURCE_VIEW_ICON_BOTTOM = 463,
    RESOURCE_VIEW_ICON_BOTTOM_PADDING = 14,
    RESOURCE_VIEW_COUNT_X = 511,
    RESOURCE_VIEW_COUNT_Y = 450,
    RESOURCE_VIEW_COUNT_WIDTH = 80,
    RESOURCE_VIEW_COUNT_HEIGHT = 12
H1_ENUM_CONST_END(AdventureResourceViewConstant)

// UpdBottomViewKingdom's nine counters: the seven resources (ResourceType
// order), then castles and villages (Buka AdventureKingdomViewConstant
// names; HoMM1's text rows start at 392).
H1_ENUM_CONST_BEGIN(AdventureKingdomViewConstant)
    KINGDOM_VIEW_ENTRY_COUNT = 9,
    KINGDOM_VIEW_CASTLE_ENTRY = 7,
    KINGDOM_VIEW_TOWN_ENTRY = 8,
    KINGDOM_VIEW_ICON_X = 481,
    KINGDOM_VIEW_ICON_Y = 393,
    KINGDOM_VIEW_TEXT_X_BASE = 464,
    KINGDOM_VIEW_TEXT_Y_BASE = 392,
    KINGDOM_VIEW_TEXT_WIDTH = 32,
    KINGDOM_VIEW_TEXT_HEIGHT = 12,
    KINGDOM_VIEW_RESOURCE_TEXT_Y = 59,
    KINGDOM_VIEW_TOWN_TEXT_Y = 28,
    KINGDOM_VIEW_WOOD_TEXT_X = 15,
    KINGDOM_VIEW_MERCURY_TEXT_X = 38,
    KINGDOM_VIEW_ORE_TEXT_X = 61,
    KINGDOM_VIEW_SULFUR_TEXT_X = 85,
    KINGDOM_VIEW_CRYSTAL_TEXT_X = 109,
    KINGDOM_VIEW_GEMS_TEXT_X = 132,
    KINGDOM_VIEW_GOLD_TEXT_X = 123,
    KINGDOM_VIEW_CASTLE_TEXT_X = 27,
    KINGDOM_VIEW_VILLAGE_TEXT_X = 80
H1_ENUM_CONST_END(AdventureKingdomViewConstant)

// UpdBottomViewHero's army icons and counts (Buka
// AdventureBottomHeroViewConstant names).
H1_ENUM_CONST_BEGIN(AdventureBottomHeroViewConstant)
    BOTTOM_HERO_LABEL_BYTES = 6,
    BOTTOM_HERO_ICON_WIDTH = 32,
    BOTTOM_HERO_ICON_HEIGHT = 28,
    BOTTOM_HERO_LABEL_HEIGHT = 12,
    BOTTOM_HERO_CHARACTER_WIDTH = 5,
    BOTTOM_HERO_FIRST_ICON_ID = 2002,
    BOTTOM_HERO_FIRST_TEXT_ID = 2101
H1_ENUM_CONST_END(AdventureBottomHeroViewConstant)

H1_ENUM_CONST_BEGIN(AdventureScrollConstant)
// The adventure view is ADVMGR_VIEW_CELL_COUNT cells square; the hero
// stands on its centre cell, ADVMGR_VIEW_CENTER from the origin.
    ADVMGR_VIEW_CENTER = 7,
    SCROLL_MIN_ORIGIN = -7,
    SCROLL_MAX_ORIGIN = 64,
    SCROLL_TICK_INTERVAL = 70,
    HOVER_SCROLL_FRAME_FIRST = 32,
    HOVER_SCROLL_FRAME_END = 40
H1_ENUM_CONST_END(AdventureScrollConstant)

H1_ENUM_CONST_BEGIN(AdventurePanelDialogConstant)
    PANEL_CLOSE_WIDGET = 0x7800,
    PANEL_NO_HELP = -1,
    PANEL_VIEW_WORLD_HELP = 0,
    PANEL_VIEW_PUZZLE_HELP = 1,
    PANEL_CAST_SPELL_HELP = 2,
    PANEL_SEARCH_HELP = 3,
    PANEL_VIEW_WORLD = 1,
    PANEL_VIEW_PUZZLE = 2,
    PANEL_CAST_SPELL = 3,
    PANEL_SEARCH = 4
H1_ENUM_CONST_END(AdventurePanelDialogConstant)

H1_ENUM_BEGIN(AdventureDrawMask)
    ADVMGR_DRAW_GROUND = 0x01,
    ADVMGR_DRAW_OBJECT = 0x02,
    ADVMGR_DRAW_OVERLAY = 0x04,
    ADVMGR_DRAW_HERO = 0x08,
    ADVMGR_DRAW_CLOUD = 0x20,
    ADVMGR_VIEW_CELL_COUNT = 15
H1_ENUM_END(AdventureDrawMask)

// GetCloudLookup's unseen-neighbour bits (index into giCloudType): the four
// edge neighbours, then the diagonals clockwise from north-east; off-map
// columns/rows set their three neighbours at once.
H1_ENUM_BEGIN(CloudNeighborMask)
    CLOUD_NORTH = 0x01,
    CLOUD_EAST = 0x02,
    CLOUD_SOUTH = 0x04,
    CLOUD_WEST = 0x08,
    CLOUD_NORTH_EAST = 0x10,
    CLOUD_SOUTH_EAST = 0x20,
    CLOUD_SOUTH_WEST = 0x40,
    CLOUD_NORTH_WEST = 0x80,
    CLOUD_EAST_EDGE = 0x32,
    CLOUD_SOUTH_EDGE = 0x64,
    CLOUD_NORTH_EDGE = 0x91,
    CLOUD_WEST_EDGE = 0xc8
H1_ENUM_END(CloudNeighborMask)

// advManager::Main's right-click help on the six panel buttons: the
// cAdvMenuHelp row (texts: next hero, continue movement, kingdom summary,
// end turn, adventure options, game options).
H1_ENUM_BEGIN(AdventurePanelHelp)
    ADVENTURE_HELP_NONE = -1,
    ADVENTURE_HELP_NEXT_HERO = 0,
    ADVENTURE_HELP_CONTINUE_ROUTE = 1,
    ADVENTURE_HELP_OVERVIEW = 2,
    ADVENTURE_HELP_END_TURN = 3,
    ADVENTURE_HELP_ADVENTURE_OPTIONS = 4,
    ADVENTURE_HELP_GAME_OPTIONS = 5
H1_ENUM_END(AdventurePanelHelp)

// qhero0/qhero1/qtown1.bin widgets: name, portrait, the hero's four primary
// stats from STAT_FIRST, and the owner's flag pair from FLAG (frames colour
// * 2 and the next). The retail frames keep portraitId/statWidget/flagId
// locals with these values. A window x of AT_LOCATOR places the hero view
// beside its locator slot, the town view at its fixed spot.
H1_ENUM_CONST_BEGIN(QuickViewWidget)
    QUICK_VIEW_NAME = 1,
    QUICK_VIEW_PORTRAIT = 2,
    QUICK_VIEW_STAT_FIRST = 3,
    QUICK_VIEW_FLAG = 8,
    QUICK_VIEW_AT_LOCATOR = -1,
    // The map-click views pass no locator slot.
    QUICK_VIEW_NO_LOCATOR = -1
H1_ENUM_CONST_END(QuickViewWidget)

// Buka 2.1's unconditional six-button enable/disable broadcast.
#define SET_ADVENTURE_BUTTON_FLAGS(message, window, cmd)                                           \
    ((message).type = MESSAGE_WIDGET,                                                              \
     (message).command = (cmd),                                                                    \
     (message).value = WIDGET_FLAG_ENABLED,                                                        \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST,                                                     \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 1,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 2,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 3,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_FIRST + 4,                                                 \
     (window)->BroadcastMessage(message),                                                          \
     (message).id = ADVMGR_PANEL_BUTTON_LAST,                                                      \
     (window)->BroadcastMessage(message))

// donor PoL RVA 0x00056350; preferred Buka symbol ??0advManager@@QAE@XZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.538995;margin=0.239070;shape=0.344;size=0.950;calls=1.000;alternate=pol20:void advManager::constructor(void)@0x00056350
VA(0x004252c0, 0x2cc)
advManager::advManager(void) {
    int i;

    m_groundTiles = NULL;
    m_puzzleIcon = NULL;
    m_mapOriginX = 0;
    m_mapOriginY = 0;
    m_updateMinX = 0;
    m_updateMinY = 0;
    m_updateMaxX = 0;
    m_updateMaxY = 0;
    m_selectedCell = ADVMGR_COMMAND_NONE;
    m_cursorActive = 0;
    m_identifyHeroActive = 0;
    m_drawHeroShadows = 1;
    m_adventureBorder = NULL;
    for (i = 0; i < ADVMGR_OBJECT_ICON_COUNT; i++)
        m_objectIcons[i] = NULL;
    for (i = 0; i < ADVMGR_HERO_ICON_COUNT; i++)
        m_heroIcons[i] = NULL;
    for (i = 0; i < ADVMGR_PLAYER_COLOR_COUNT; i++) {
        m_flagIcons[i] = NULL;
        m_boatFlagIcons[i] = NULL;
    }
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = NULL;
    for (i = 0; i < ADVMGR_CURSOR_SAMPLE_COUNT; i++)
        m_cursorSamples[i] = NULL;
    m_puzzleIcon = NULL;
    m_cloudOverlayIcon = NULL;
    m_boatShadowIcon = NULL;
    m_groundTiles = NULL;
    m_cloudTiles = NULL;
    m_stoneTiles = NULL;
    m_adventureWindow = NULL;
    m_visibilityMap = NULL;
    m_heroContextLocked = 0;
    m_townContextLocked = 0;
    bShowIt = 1;
    m_lastQuickViewX = QUICK_VIEW_NONE;
    m_lastQuickViewY = QUICK_VIEW_NONE;
    m_animationPhases[ANIMATION_PHASE_COLUMN_0] = ANIMATION_PHASE_COLUMN_0_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_1] = ANIMATION_PHASE_COLUMN_1_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_2] = ANIMATION_PHASE_COLUMN_2_INITIAL;
    m_animationPhases[ANIMATION_PHASE_COLUMN_3] = ANIMATION_PHASE_COLUMN_3_INITIAL;
    m_mapData = gpGame->GetWorldMapData();
    gMapX = 0;
    gMapY = 0;
    m_cursorFrameCount = 0;
    m_cursorCycle = 0;
    m_cursorTurning = 0;
}

// InitMainClasses deletes gpAdvManager through this vtable-reset destructor.
VA(0x0042558c, 0x1f)
advManager::~advManager() {}

// donor PoL RVA 0x0005665f; preferred Buka symbol ?Open@advManager@@UAEHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.608500;margin=0.280356;shape=0.449;size=0.637;calls=0.611;strings=advManager|adv_wind.bin|advmice.mse;alternate=pol20:int advManager::Open(int);   // virtual [override (implements baseManager pure virtual)]@0x0005665f
VA(0x004255ab, 0xea6)
short advManager::Open(short id) {
    int savedShowIt;
    int firstTime;
    int oldPlayer;
    int oldVolume;
    int i;

    firstTime = 1;
    iCurBottomView = BOTTOM_VIEW_NONE;
    m_openState = 0;
    bShowIt = 0;
    m_adventureBorder = NULL;
    for (i = 0; i < ADVMGR_BOTTOM_VIEW_WIDGET_COUNT; i++) {
        m_bottomViewPrimaryWidgets[i] = NULL;
        m_bottomViewSecondaryWidgets[i] = NULL;
    }
    if (m_adventureWindow == NULL) {
        m_adventureWindow = new heroWindow(0, 0, "adv_wind.bin");
        if (m_adventureWindow == NULL)
            MemError();
        m_scrollLeftButton = new iconWidget(
            SCROLL_LEFT_X,
            SCROLL_Y,
            SCROLL_WIDTH,
            SCROLL_HEIGHT,
            "scroll.icn",
            SCROLL_ICON_FRAME,
            ICON_DRAW_NORMAL,
            ADVENTURE_CONTROL_HERO_KNOB,
            ICON_WIDGET_DRAW,
            1
        );
        if (m_scrollLeftButton == NULL)
            MemError();
        m_adventureWindow->AddWidget(m_scrollLeftButton, WINDOW_Z_ORDER_APPEND);
        m_scrollRightButton = new iconWidget(
            SCROLL_RIGHT_X,
            SCROLL_Y,
            SCROLL_WIDTH,
            SCROLL_HEIGHT,
            "scroll.icn",
            SCROLL_ICON_FRAME,
            ICON_DRAW_NORMAL,
            ADVENTURE_CONTROL_TOWN_KNOB,
            ICON_WIDGET_DRAW,
            1
        );
        if (m_scrollRightButton == NULL)
            MemError();
        m_adventureWindow->AddWidget(m_scrollRightButton, WINDOW_Z_ORDER_APPEND);
        m_panelBackdrops[0] = new backdropWidget(480, 176, 56, 128, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[0] == NULL)
            MemError();
        m_panelBackdrops[1] = new backdropWidget(552, 176, 56, 128, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[1] == NULL)
            MemError();
        m_panelBackdrops[2] = new backdropWidget(539, 194, 10, 92, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[2] == NULL)
            MemError();
        m_panelBackdrops[3] = new backdropWidget(611, 194, 10, 92, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[3] == NULL)
            MemError();
        m_panelBackdrops[4] = new backdropWidget(480, 320, 144, 144, WIDGET_ID_NONE, 32);
        if (m_panelBackdrops[4] == NULL)
            MemError();
        for (i = 0; i < ADVMGR_PANEL_ICON_COUNT; i++)
            m_adventureWindow->AddWidget(m_panelBackdrops[i], WINDOW_Z_ORDER_APPEND);
    }
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    else
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_WAIT);
    if (m_visibilityMap == NULL) {
        m_visibilityMap = new signed char[MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE];
        if (m_visibilityMap == NULL)
            MemError();
    }
    m_routeShown = 0;
    gpWindowManager->AddWindow(m_adventureWindow, 0, 1);
    if (m_groundTiles == NULL)
        m_groundTiles = gpResourceManager->GetTileset("ground32.til");
    if (m_cloudTiles == NULL)
        m_cloudTiles = gpResourceManager->GetTileset("clof32.til");
    if (m_stoneTiles == NULL)
        m_stoneTiles = gpResourceManager->GetTileset("ston.til");
    if (m_cloudOverlayIcon == NULL)
        m_cloudOverlayIcon = gpResourceManager->GetIcon("clop32.icn");
    if (m_objectIcons[TILESET_OBJ32_00] == NULL)
        m_objectIcons[TILESET_OBJ32_00] = gpResourceManager->GetIcon("obj32-00.icn");
    if (m_objectIcons[TILESET_OBJ32_01] == NULL)
        m_objectIcons[TILESET_OBJ32_01] = gpResourceManager->GetIcon("obj32-01.icn");
    if (m_objectIcons[TILESET_OBJ32_02] == NULL)
        m_objectIcons[TILESET_OBJ32_02] = gpResourceManager->GetIcon("obj32-02.icn");
    if (m_objectIcons[TILESET_OBJ32_03] == NULL)
        m_objectIcons[TILESET_OBJ32_03] = gpResourceManager->GetIcon("obj32-03.icn");
    if (m_objectIcons[TILESET_OBJ32_04] == NULL)
        m_objectIcons[TILESET_OBJ32_04] = gpResourceManager->GetIcon("obj32-04.icn");
    if (m_objectIcons[TILESET_OBJ32_05] == NULL)
        m_objectIcons[TILESET_OBJ32_05] = gpResourceManager->GetIcon("obj32-05.icn");
    if (m_objectIcons[TILESET_OBJ32_06] == NULL)
        m_objectIcons[TILESET_OBJ32_06] = gpResourceManager->GetIcon("obj32-06.icn");
    if (m_objectIcons[TILESET_OBJ32_07] == NULL)
        m_objectIcons[TILESET_OBJ32_07] = gpResourceManager->GetIcon("obj32-07.icn");
    if (m_objectIcons[TILESET_MTN32] == NULL)
        m_objectIcons[TILESET_MTN32] = gpResourceManager->GetIcon("mtn32.icn");
    if (m_objectIcons[TILESET_TREE32] == NULL)
        m_objectIcons[TILESET_TREE32] = gpResourceManager->GetIcon("tree32.icn");
    if (m_objectIcons[TILESET_TOWN32] == NULL)
        m_objectIcons[TILESET_TOWN32] = gpResourceManager->GetIcon("town32.icn");
    if (m_objectIcons[TILESET_RSRC32] == NULL)
        m_objectIcons[TILESET_RSRC32] = gpResourceManager->GetIcon("rsrc32.icn");
    if (m_objectIcons[TILESET_MONS32] == NULL)
        m_objectIcons[TILESET_MONS32] = gpResourceManager->GetIcon("mons32.icn");
    if (m_objectIcons[TILESET_ART32] == NULL)
        m_objectIcons[TILESET_ART32] = gpResourceManager->GetIcon("art32.icn");
    if (m_objectIcons[TILESET_FLAG32] == NULL)
        m_objectIcons[TILESET_FLAG32] = gpResourceManager->GetIcon("flag32.icn");
    if (m_objectIcons[TILESET_RESSMALL] == NULL)
        m_objectIcons[TILESET_RESSMALL] = gpResourceManager->GetIcon("ressmall.icn");
    if (m_objectIcons[TILESET_HOURGLAS] == NULL)
        m_objectIcons[TILESET_HOURGLAS] = gpResourceManager->GetIcon("hourglas.icn");
    if (m_objectIcons[TILESET_ROUTE] == NULL)
        m_objectIcons[TILESET_ROUTE] = gpResourceManager->GetIcon("route.icn");
    if (m_objectIcons[TILESET_SMCREST] == NULL)
        m_objectIcons[TILESET_SMCREST] = gpResourceManager->GetIcon("smcrest.icn");
    if (m_objectIcons[TILESET_STONBACK] == NULL)
        m_objectIcons[TILESET_STONBACK] = gpResourceManager->GetIcon("stonback.icn");
    if (m_objectIcons[TILESET_MINIMON] == NULL)
        m_objectIcons[TILESET_MINIMON] = gpResourceManager->GetIcon("minimon.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_KNIGHT] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_KNIGHT] = gpResourceManager->GetIcon("kngt32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_BARBARIAN] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_BARBARIAN] = gpResourceManager->GetIcon("barb32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_SORCERESS] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_SORCERESS] = gpResourceManager->GetIcon("sorc32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_WARLOCK] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_WARLOCK] = gpResourceManager->GetIcon("wrlk32.icn");
    if (m_heroIcons[ADVMGR_HERO_ICON_BOAT] == NULL)
        m_heroIcons[ADVMGR_HERO_ICON_BOAT] = gpResourceManager->GetIcon("boat32.icn");
    gbLoadingMonoIcon = 1;
    if (m_boatShadowIcon == NULL)
        m_boatShadowIcon = gpResourceManager->GetIcon("shadow32.icn");
    gbLoadingMonoIcon = 0;
    if (m_flagIcons[PLAYER_COLOR_BLUE] == NULL)
        m_flagIcons[PLAYER_COLOR_BLUE] = gpResourceManager->GetIcon("b-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_GREEN] == NULL)
        m_flagIcons[PLAYER_COLOR_GREEN] = gpResourceManager->GetIcon("g-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_RED] == NULL)
        m_flagIcons[PLAYER_COLOR_RED] = gpResourceManager->GetIcon("r-flag32.icn");
    if (m_flagIcons[PLAYER_COLOR_YELLOW] == NULL)
        m_flagIcons[PLAYER_COLOR_YELLOW] = gpResourceManager->GetIcon("y-flag32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_BLUE] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_BLUE] = gpResourceManager->GetIcon("b-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_GREEN] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_GREEN] = gpResourceManager->GetIcon("g-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_RED] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_RED] = gpResourceManager->GetIcon("r-bflg32.icn");
    if (m_boatFlagIcons[PLAYER_COLOR_YELLOW] == NULL)
        m_boatFlagIcons[PLAYER_COLOR_YELLOW] = gpResourceManager->GetIcon("y-bflg32.icn");
    gbLoadingMonoIcon = 1;
    if (m_puzzleIcon == NULL)
        m_puzzleIcon = gpResourceManager->GetIcon("radar.icn");
    gbLoadingMonoIcon = 0;
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; i++)
        m_loopingSamples[i] = NULL;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; i++) {
        m_activeSounds[i].soundId = MAP_SOUND_NONE;
        m_activeSounds[i].volume = ENVIRONMENT_SOUND_DEFAULT_VOLUME;
        m_activeSoundMask = 0;
    }
    GetCursorSampleSet(gConfig.walkSpeed);
    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        gpGame->TurnOnAIMusic();
        SetNoDialogMenus(0);
    } else {
        SetNoDialogMenus(1);
    }
    glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
    // 0x100 has no known producer (no MessageType member); retail keeps it.
    m_messageTypeMask = MESSAGE_KEY_DOWN | MESSAGE_KEY_UP | MESSAGE_MOUSE_MOVE
                        | MESSAGE_LEFT_BUTTON_DOWN | MESSAGE_RIGHT_BUTTON_DOWN | 0x100
                        | MESSAGE_WIDGET;
    gpMouseManager->NewUpdate(1);
    oldVolume = gConfig.soundVolume;
    if (gConfig.soundVolume != SOUND_VOLUME_OFF)
        gConfig.soundVolume = SOUND_VOLUME_LAST;
    SetInitialMapOrigin();
    bShowIt = gbThisNetHumanPlayer[giCurPlayer];
    gpMouseManager->SetColorMice(0);
    oldPlayer = giCurPlayer;
    savedShowIt = bShowIt;
    giCurPlayer = giCurWatchPlayer;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    bShowIt = 1;
    RedrawAdvScreen(1);
    giCurPlayer = oldPlayer;
    bShowIt = savedShowIt;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    if (!gbThisNetHumanPlayer[giCurPlayer])
        gpGame->ShowComputerScreen();
    gpMouseManager->ReallyShowPointer();
    KBChangeMenu(hmnuAdv);
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
    giBottomViewOverride = BOTTOM_VIEW_NONE;
    gConfig.soundVolume = oldVolume;
    gpSoundManager->AdjustSoundVolumes();
    m_messageMask = BASE_MANAGER_ACCEPT_ADVENTURE;
    m_priority = id;
    m_active = 1;
    strcpy(m_name, "advManager");
    return 0;
}

// donor PoL RVA 0x00057028; preferred Buka symbol ?Close@advManager@@UAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.555933;margin=0.510723;shape=0.387;size=0.995;calls=0.909;alternate=pol20:void advManager::Close(void);   // virtual [override (implements baseManager pure virtual)]@0x00057028
VA(0x00426451, 0x3bd)
void advManager::Close(void) {
    short index;

    ClearBottomView();
    gpMouseManager->SetPointer(MOUSE_INVALID_CURSOR_FRAME);
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NONE);
    gpSoundManager->StopAllSamples();
    if (m_adventureBorder) {
        free(m_adventureBorder);
        m_adventureBorder = NULL;
    }
    if (gAdvDisposeLevel <= 1) {
        for (index = 0; index < ADVMGR_OBJECT_ICON_COUNT; index++) {
            if (m_objectIcons[index])
                gpResourceManager->Dispose(m_objectIcons[index]);
            m_objectIcons[index] = NULL;
        }
    }
    if (gAdvDisposeLevel <= 0) {
        gpResourceManager->Dispose(m_puzzleIcon);
        m_puzzleIcon = NULL;
        gpResourceManager->Dispose(m_cloudOverlayIcon);
        m_cloudOverlayIcon = NULL;
        for (index = 0; index < ADVMGR_HERO_ICON_COUNT; index++) {
            gpResourceManager->Dispose(m_heroIcons[index]);
            m_heroIcons[index] = NULL;
        }
        gpResourceManager->Dispose(m_boatShadowIcon);
        m_boatShadowIcon = NULL;
        for (index = 0; index < ADVMGR_PLAYER_COLOR_COUNT; index++) {
            gpResourceManager->Dispose(m_flagIcons[index]);
            m_flagIcons[index] = NULL;
            gpResourceManager->Dispose(m_boatFlagIcons[index]);
            m_boatFlagIcons[index] = NULL;
        }
        gpResourceManager->Dispose(m_groundTiles);
        m_groundTiles = NULL;
        gpResourceManager->Dispose(m_cloudTiles);
        m_cloudTiles = NULL;
        gpResourceManager->Dispose(m_stoneTiles);
        m_stoneTiles = NULL;
    }
    for (index = 0; index < ADVMGR_ENVIRONMENT_SOUND_COUNT; index++) {
        if (m_loopingSamples[index])
            gpResourceManager->Dispose(m_loopingSamples[index]);
        m_loopingSamples[index] = NULL;
    }
    for (index = 0; index < ADVMGR_CURSOR_SAMPLE_COUNT; index++) {
        gpResourceManager->Dispose(m_cursorSamples[index]);
        m_cursorSamples[index] = NULL;
    }
    gpWindowManager->RemoveWindow(m_adventureWindow);
    delete m_adventureWindow;
    m_adventureWindow = NULL;
    if (m_visibilityMap)
        delete m_visibilityMap;
    m_visibilityMap = NULL;
    iCurBottomView = BOTTOM_VIEW_NONE;
    m_active = 0;
}

// donor PoL RVA 0x00057432; preferred Buka symbol ?GetCursorSampleSet@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701218;margin=0.695021;shape=0.378;size=0.921;calls=1.000;strings=wsnd%1d%1d.82M;alternate=pol20:void advManager::GetCursorSampleSet(int)@0x00057432
VA(0x0042680e, 0xc7)
void advManager::GetCursorSampleSet(int sampleSet) {
    if (sampleSet >= 1)
        sampleSet = CURSOR_SAMPLE_FAST_SET;
    signed char suffixSample[ADVMGR_CURSOR_SAMPLE_COUNT] = {0, 3, 5, 3, 4, 5, 6};
    for (int index = 0; index < ADVMGR_CURSOR_SAMPLE_COUNT; ++index) {
        sprintf(gText, "wsnd%1d%1d.82M", sampleSet, suffixSample[index]);
        m_cursorSamples[index] = gpResourceManager->GetSample(gText);
        m_cursorSamples[index]->m_playbackData.volume = CURSOR_SAMPLE_VOLUME;
        m_cursorSamples[index]->m_playbackData.channelType = SAMPLE_PLAYBACK_CHANNEL_GROUP;
    }
}

// donor PoL RVA 0x0005751b; preferred Buka symbol ?DoAdvCommand@advManager@@QAEPAVmapCell@@XZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.528946;margin=1.360342;shape=0.339;size=0.921;calls=0.947;alternate=pol20:class mapCell * advManager::DoAdvCommand(void)@0x0005751b
VA(0x004268d5, 0x619)
class mapCell* advManager::DoAdvCommand(void) {
    signed char moveDone;
    town* viewTown;
    signed char userStop;
    hero* pHero;
    int oldMapValid;
    tag_message evt;
    signed char hover;
    mapCell* stopCell;
    int moveChanged;
    short pathIndex;

    stopCell = NULL;
    pHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    userStop = 0;
    hover = 0;
    switch (m_selectedCell) {
        case ADVMGR_COMMAND_MOVE_TO:
            pHero->m_destinationX = m_commandTargetX;
            pHero->m_destinationY = m_commandTargetY;
            goto continue_route;
        case ADVMGR_COMMAND_CONTINUE_ROUTE:
        continue_route:
            gpSearchArray->BuildPath(
                pHero->m_x,
                pHero->m_y,
                pHero->m_destinationX,
                pHero->m_destinationY,
                SEARCH_UNLIMITED_COST
            );
            if (gpSearchArray->m_pathLength > 0) {
                oldMapValid = m_routeShown;
                MobilizeCurrHero(1);
                if (gConfig.showRoute || oldMapValid)
                    ShowRoute(0, 0, 0);
                else if (m_routeShown && m_selectedCell != ADVMGR_COMMAND_CONTINUE_ROUTE)
                    HideRoute(1, 0, 1);
                gpMouseManager->ReallyHidePointer();
                gpInputManager->Flush();
                for (pathIndex = gpSearchArray->m_pathLength - 1; pathIndex >= 0; pathIndex--) {
                    stopCell = MoveHero(
                        gpSearchArray->m_directions[pathIndex],
                        pathIndex == 0,
                        &TrigX,
                        &TrigY,
                        &moveChanged,
                        0,
                        &moveDone
                    );
                    UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, 1, 1);
                    if (stopCell)
                        break;
                    if (moveChanged || moveDone)
                        goto movement_done;
                    evt = gpInputManager->GetEvent();
                    while (evt.type) {
                        if (evt.type == MESSAGE_KEY_DOWN || evt.type == MESSAGE_LEFT_BUTTON_DOWN
                            || evt.type == MESSAGE_RIGHT_BUTTON_DOWN
                            || evt.type == MESSAGE_WIDGET) {
                            userStop = 1;
                            StopCursor(1);
                            goto movement_done;
                        }
                        Process1WindowsMessage();
                        evt = gpInputManager->GetEvent();
                    }
                }
            movement_done:
                if ((pathIndex <= 0 && pHero->m_x == pHero->m_destinationX
                     && pHero->m_y == pHero->m_destinationY)
                    || (userStop && !gConfig.showRoute) || stopCell)
                    HideRoute(1, 1, 1);
                else if (m_selectedCell == ADVMGR_COMMAND_CONTINUE_ROUTE || gConfig.showRoute)
                    ShowRoute(0, 1, 1);
                gpMouseManager->ReallyShowPointer();
                UpdBottomView(1, 1, 1);
                if (stopCell) {
                    StopCursor(1);
                    DoEvent(stopCell, TrigX, TrigY);
                    stopCell = NULL;
                }
                Reseed(0, 0);
                hover = 1;
                CheckDimHero();
            }
            break;
        case ADVMGR_COMMAND_OCCUPIED_TOWN_VIEW:
            DemobilizeCurrHero();
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            viewTown = gpGame->GetTown(pHero->m_occupiedTown);
            viewTown->View();
            stopCell = NULL;
            break;
        case ADVMGR_COMMAND_TOWN_VIEW:
            DemobilizeCurrHero();
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            stopCell = GetCell(
                gpGame->GetTown(gpCurPlayer->m_currentTown)->m_x,
                gpGame->GetTown(gpCurPlayer->m_currentTown)->m_y
            );
            gpGame->GetTown(gpCurPlayer->m_currentTown)->View();
            stopCell = NULL;
            break;
        case ADVMGR_COMMAND_HERO_VIEW:
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            gpGame->GetHero(gpCurPlayer->m_currentHero)->HeroView(0);
            RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
            break;
        case ADVMGR_COMMAND_SELECT_HERO:
            SetHeroContext(
                GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY)
                    ->m_objectMetadata,
                0
            );
            break;
        case ADVMGR_COMMAND_SELECT_TOWN:
            SetTownContext(GetCell(m_mapOriginX + m_lastHoverCell, m_mapOriginY + m_hoverCellY)
                               ->m_objectMetadata);
            break;
        case ADVMGR_COMMAND_NONE:
            break;
    }
    m_selectedCell = ADVMGR_COMMAND_NONE;
    m_lastHoverCell = m_hoverCellY = CURSOR_INVALID_POSITION;
    if (hover)
        ForceNewHover();
    return stopCell;
}

// donor PoL RVA 0x00057d6c; preferred Buka symbol ?Main@advManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.507706;margin=0.523825;shape=0.297;size=0.996;calls=0.852;alternate=pol20:int advManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x00057d6c
VA(0x00426eee, 0xe10)
short advManager::Main(struct tag_message& message) {
    DATA(0x0048fae8)
    static int giCheatSeq = 0;
    int yPos;
    int xPos;
    int retVal;
    mapCell* evtMapCell;
    hero* curHero;
    int townIndex;
    int cmdValue;
    int bQuit;
    int moved;
    signed char bEnded;
    int helpText;
    int dir;

    if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT] && ComboDraw(1))
        UpdateScreen(1, 0);
    if (gbGameOver) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    if (!gbHumanPlayer[giCurPlayer] && (!gbRemoteOn || giHostGamePos == giThisGamePos)) {
        gpPhilAI->DoAI(giCurPlayer);
        gpGame->NextPlayer();
        return MESSAGE_DISPATCH_CONSUME;
    }
    CheckHandleNet();
    if (!gbThisNetHumanPlayer[giCurPlayer])
        return CheckHandleNetPlayerWait(message, 0);
    if (giScreenScroll && gbForegroundApp)
        CheckScreenScroll();
    if (!(message.type & m_messageTypeMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if (!gbNoSound && gConfig.musicVolume && giForceSwitchMusic > 0
        && KBTickCount() - giForceSwitchMusic > FORCED_MUSIC_DELAY
        && gpSoundManager->m_currentTrack == MUSIC_TRACK_NETWORK_TURN) {
        giForceSwitchMusic = FORCED_MUSIC_IDLE;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    retVal = MESSAGE_DISPATCH_CONSUME;
    bQuit = 0;
    evtMapCell = NULL;
    if (message.type) {
        switch (message.type) {
            case MESSAGE_WIDGET:
                switch (message.command) {
                    case WIDGET_COMMAND_HOVER:
                        retVal = ProcessHover(&message);
                        break;
                    case WIDGET_NOTIFY_DESELECT:
                        if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON))
                            retVal = ProcessDeSelect(&message, &bQuit, &evtMapCell);
                        break;
                    case WIDGET_NOTIFY_SELECT:
                        retVal = ProcessSelect(&message, &evtMapCell);
                        break;
                    case WIDGET_NOTIFY_RIGHT_CLICK:
                        helpText = ADVENTURE_HELP_NONE;
                        switch (message.id) {
                            case ADVENTURE_CONTROL_NEXT_HERO:
                                helpText = ADVENTURE_HELP_NEXT_HERO;
                                break;
                            case ADVENTURE_CONTROL_CONTINUE_ROUTE:
                                helpText = ADVENTURE_HELP_CONTINUE_ROUTE;
                                break;
                            case ADVENTURE_CONTROL_OVERVIEW:
                                helpText = ADVENTURE_HELP_OVERVIEW;
                                break;
                            case ADVENTURE_CONTROL_END_TURN:
                                helpText = ADVENTURE_HELP_END_TURN;
                                break;
                            case ADVENTURE_CONTROL_ADVENTURE_OPTIONS:
                                helpText = ADVENTURE_HELP_ADVENTURE_OPTIONS;
                                break;
                            case ADVENTURE_CONTROL_GAME_OPTIONS:
                                helpText = ADVENTURE_HELP_GAME_OPTIONS;
                                break;
                        }
                        if (helpText >= 0)
                            NormalDialog(cAdvMenuHelp[helpText], NORMAL_DIALOG_TYPE_QUICK_VIEW);
                        break;
                }
                break;
            case MESSAGE_KEY_DOWN:
                dir = -1;
                if (gpCurPlayer->CurrentHero() != INVALID_HERO)
                    curHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                else
                    curHero = NULL;
                if (giDebugLevel < 1
                    && (message.keyCode == INPUT_SCAN_F3 || message.keyCode == INPUT_SCAN_F4
                        || message.keyCode == INPUT_SCAN_F5 || message.keyCode == INPUT_SCAN_F6
                        || message.keyCode == INPUT_SCAN_F7 || message.keyCode == INPUT_SCAN_F8
                        || message.keyCode == INPUT_SCAN_F9 || message.keyCode == INPUT_SCAN_F10
                        || message.keyCode == INPUT_SCAN_F11 || message.keyCode == INPUT_SCAN_F12))
                    break;
                switch (message.keyCode) {
                    case INPUT_SCAN_F2:
                        PopNetBox(NULL);
                        break;
                    case INPUT_SCAN_F3:
                        gpGame->m_playerDead[1] = 1;
                        gpGame->m_playerDead[2] = 1;
                        gpGame->m_playerDead[3] = 1;
                        CheckEndGame(1);
                        break;
                    case INPUT_SCAN_F5:
                        gpGame->m_playerDead[0] = 1;
                        CheckEndGame(0);
                        break;
                    case INPUT_SCAN_F6:
                        if (curHero) {
                            for (cmdValue = 0; cmdValue < HERO_SPELL_SLOT_COUNT; cmdValue++)
                                curHero->AddSpell(cmdValue, CHEAT_SPELL_CHARGES, 0);
                        }
                        break;
                    case INPUT_SCAN_F7:
                        if (curHero)
                            GiveExperience(curHero, CHEAT_EXPERIENCE_AMOUNT, 1);
                        break;
                    case INPUT_SCAN_F8:
                        if (curHero) {
                            gpGame->GiveArmy(
                                &curHero->m_army,
                                CREATURE_DRAGON,
                                1,
                                ARMY_GROUP_EMPTY_SLOT
                            );
                            gpGame->GiveArmy(
                                &curHero->m_army,
                                CREATURE_TROLL,
                                1,
                                ARMY_GROUP_EMPTY_SLOT
                            );
                        }
                        break;
                    case INPUT_SCAN_F9:
                        for (cmdValue = 0; cmdValue < RESOURCE_COUNT; cmdValue++) {
                            if (cmdValue == RESOURCE_GOLD)
                                gpCurPlayer->m_resources[cmdValue] += CHEAT_GOLD_AMOUNT;
                            else
                                gpCurPlayer->m_resources[cmdValue] += CHEAT_RESOURCE_AMOUNT;
                        }
                        break;
                    case INPUT_SCAN_F11:
                        if (curHero)
                            curHero->m_remainingMobility = CHEAT_MOBILITY;
                        break;
                    case INPUT_SCAN_F12:
                        gpGame->SetVisibility(
                            CHEAT_REVEAL_CENTER,
                            CHEAT_REVEAL_CENTER,
                            giCurPlayer,
                            CHEAT_REVEAL_RADIUS
                        );
                        UpdateRadar(1, 0);
                        CompleteDraw(0);
                        UpdateScreen(0, 0);
                        break;
                    case INPUT_SCAN_0:
                        cmdValue = 0;
                        goto processCheatDigit;
                    case INPUT_SCAN_1:
                        cmdValue = 1;
                        goto processCheatDigit;
                    case INPUT_SCAN_2:
                        cmdValue = 2;
                        goto processCheatDigit;
                    case INPUT_SCAN_3:
                        cmdValue = 3;
                        goto processCheatDigit;
                    case INPUT_SCAN_4:
                        cmdValue = 4;
                        goto processCheatDigit;
                    case INPUT_SCAN_5:
                        cmdValue = 5;
                        goto processCheatDigit;
                    case INPUT_SCAN_6:
                        cmdValue = 6;
                        goto processCheatDigit;
                    case INPUT_SCAN_7:
                        cmdValue = 7;
                        goto processCheatDigit;
                    case INPUT_SCAN_8:
                        cmdValue = 8;
                        goto processCheatDigit;
                    case INPUT_SCAN_9:
                        cmdValue = 9;
                        goto processCheatDigit;
                    processCheatDigit:
                        giCheatSeq =
                            giCheatSeq * CHEAT_SEQUENCE_RADIX % CHEAT_SEQUENCE_MODULUS + cmdValue;
                        if (giCheatSeq == CHEAT_REVEAL_MAP) {
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                0,
                                CHEAT_REVEAL_RADIUS
                            );
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                1,
                                CHEAT_REVEAL_RADIUS
                            );
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                2,
                                CHEAT_REVEAL_RADIUS
                            );
                            gpGame->SetVisibility(
                                CHEAT_REVEAL_CENTER,
                                CHEAT_REVEAL_CENTER,
                                3,
                                CHEAT_REVEAL_RADIUS
                            );
                            Reseed(0, 0);
                            UpdateRadar(1, 0);
                            CompleteDraw(0);
                            UpdateScreen(0, 0);
                        }
                        break;
                    case INPUT_SCAN_ESCAPE:
                        break;
                    case INPUT_SCAN_NUMPAD_8:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH, 0);
                        else
                            dir = MAP_DIRECTION_NORTH;
                        break;
                    case INPUT_SCAN_NUMPAD_9:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH_EAST, 0);
                        else
                            dir = MAP_DIRECTION_NORTH_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_6:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_EAST, 0);
                        else
                            dir = MAP_DIRECTION_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_3:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH_EAST, 0);
                        else
                            dir = MAP_DIRECTION_SOUTH_EAST;
                        break;
                    case INPUT_SCAN_NUMPAD_2:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH, 0);
                        else
                            dir = MAP_DIRECTION_SOUTH;
                        break;
                    case INPUT_SCAN_NUMPAD_1:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_SOUTH_WEST, 0);
                        else
                            dir = MAP_DIRECTION_SOUTH_WEST;
                        break;
                    case INPUT_SCAN_NUMPAD_4:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_WEST, 0);
                        else
                            dir = MAP_DIRECTION_WEST;
                        break;
                    case INPUT_SCAN_NUMPAD_7:
                        if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                            ScreenScroll(MAP_DIRECTION_NORTH_WEST, 0);
                        else
                            dir = MAP_DIRECTION_NORTH_WEST;
                        break;
                    case INPUT_SCAN_C:
                        CheckCastSpell();
                        break;
                    case INPUT_SCAN_D:
                        ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
                        break;
                    case INPUT_SCAN_P:
                        ViewPuzzle();
                        break;
                    case INPUT_SCAN_V:
                        ViewWorld(SPELL_VIEW_ALL, 0, 0);
                        break;
                    case INPUT_SCAN_N:
                        cmdValue = MAIN_MENU_NEW_GAME;
                        strcpy(
                            gText,
                            "Are you sure you want to restart?  (Your current game will be lost)"
                        );
                        goto confirmGameCommand;
                    case INPUT_SCAN_L:
                        cmdValue = MAIN_MENU_LOAD_GAME;
                        strcpy(
                            gText,
                            "Are you sure you want to load a new game?  (Your current game will be "
                            "lost)"
                        );
                        goto confirmGameCommand;
                    case INPUT_SCAN_Q:
                        cmdValue = MAIN_MENU_QUIT;
                        strcpy(gText, "Are you sure you want to quit?");
                        goto confirmGameCommand;
                    confirmGameCommand:
                        bQuit = 1;
                        NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                            bQuit = 0;
                        else
                            gGameCommand = cmdValue;
                        break;
                    case INPUT_SCAN_S:
                        SaveGame();
                        break;
                    case INPUT_SCAN_I:
                        if (gpGame->m_campaignType > 0)
                            gpGame->ShowCampaignInfo(gpGame->m_campaignScenario, 1, 0);
                        else
                            gpGame->ShowScenInfo();
                        break;
                    case INPUT_SCAN_T:
                        if (gpCurPlayer->m_townCount >= 0) {
                            if (gpCurPlayer->CurrentTown() == GAME_TOWN_NONE) {
                                townIndex = gpCurPlayer->m_townIds[0];
                            } else {
                                townIndex = 0;
                                for (cmdValue = 0; cmdValue < gpCurPlayer->m_townCount;
                                     cmdValue++) {
                                    if (gpCurPlayer->m_townIds[cmdValue]
                                        == gpCurPlayer->CurrentTown()) {
                                        if (cmdValue == gpCurPlayer->m_townCount - 1)
                                            townIndex = gpCurPlayer->m_townIds[0];
                                        else
                                            townIndex = gpCurPlayer->m_townIds[cmdValue + 1];
                                    }
                                }
                            }
                            SetTownContext(townIndex);
                        }
                        break;
                    case INPUT_SCAN_H:
                        SetHeroContext(gpCurPlayer->NextHero(0), 0);
                        break;
                    case INPUT_SCAN_ENTER:
                        if (gpCurPlayer->CurrentTown() != GAME_TOWN_NONE) {
                            m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                            DoAdvCommand();
                        } else if (gpCurPlayer->CurrentHero() != INVALID_HERO) {
                            m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                            DoAdvCommand();
                        }
                        break;
                }
                if (gpCurPlayer->m_currentHero != INVALID_HERO && dir >= 0) {
                    HideRoute(1, 1, 1);
                    gpMouseManager->ReallyHidePointer();
                    evtMapCell = MoveHero(dir, 1, &TrigX, &TrigY, &moved, 0, &bEnded);
                    UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, 1, 1);
                    if (evtMapCell) {
                        StopCursor(1);
                        DoEvent(evtMapCell, TrigX, TrigY);
                        evtMapCell = NULL;
                    }
                    Reseed(0, 0);
                    ForceNewHover();
                    UpdBottomView(1, 1, 1);
                    CheckDimHero();
                    gpMouseManager->ReallyShowPointer();
                }
                break;
        }
    }
    if (evtMapCell)
        DoEvent(evtMapCell, TrigX, TrigY);
    if (gbGameOver || bQuit == 1 || giMenuCommand != APP_MENU_NONE) {
        message.type = MESSAGE_EXECUTIVE;
        message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return retVal;
}

// Buka 2.1 Reseed and HoMM1's seven call sites identify this tiny reset.
VA(0x00427cfe, 0x22)
void advManager::Reseed(int, int) {
    giSeedingValid = 0;
}

// donor PoL RVA 0x00058d68; preferred Buka symbol ?ProcessSelect@advManager@@QAEHPAUtag_message@@PAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526004;margin=0.306176;shape=0.329;size=0.923;calls=0.956;alternate=pol20:int advManager::ProcessSelect(struct tag_message *, class mapCell * *)@0x00058d68
VA(0x00427d20, 0xe39)
int advManager::ProcessSelect(struct tag_message* message, class mapCell** eventCell) {
    short curX;
    short cellType;
    short mapIndex;
    short curY;
    mapCell* hoverCell;
    int iPage;
    int isVisible;
    tag_message mouseMsg;
    tag_message inputMessage;
    hero* hero;
    signed char mobileResult;

    isVisible = 1;
    switch (message->id) {
        case ADVENTURE_CONTROL_HERO_LOCATOR_1:
        case ADVENTURE_CONTROL_HERO_LOCATOR_2:
        case ADVENTURE_CONTROL_HERO_LOCATOR_3:
        case ADVENTURE_CONTROL_HERO_LOCATOR_4:
            iPage = (message->id - ADVENTURE_CONTROL_HERO_LOCATOR_1)
                    / (ADVENTURE_CONTROL_HERO_LOCATOR_2 - ADVENTURE_CONTROL_HERO_LOCATOR_1);
            if (gpCurPlayer->m_heroCount <= iPage)
                break;
            cellType = gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + iPage];
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                HeroQuickView(cellType, iPage, QUICK_VIEW_AT_LOCATOR, QUICK_VIEW_AT_LOCATOR);
            } else if (gpCurPlayer->CurrentHero() == cellType) {
                m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                DoAdvCommand();
            } else {
                HideRoute(1, 0, 1);
                SetHeroContext(cellType, 0);
            }
            break;
        case ADVENTURE_CONTROL_TOWN_LOCATOR_1:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_2:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_3:
        case ADVENTURE_CONTROL_TOWN_LOCATOR_4:
            cellType = gpCurPlayer->m_townIds
                           [gpCurPlayer->m_townLocatorPage + message->id
                            - ADVENTURE_CONTROL_TOWN_LOCATOR_1];
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                TownQuickView(
                    cellType,
                    message->id - ADVENTURE_CONTROL_TOWN_LOCATOR_1,
                    QUICK_VIEW_AT_LOCATOR,
                    QUICK_VIEW_AT_LOCATOR
                );
            } else {
                HideRoute(1, 0, 1);
                if (gpCurPlayer->CurrentTown() == cellType) {
                    m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                    *eventCell = DoAdvCommand();
                } else {
                    SetTownContext(cellType);
                }
            }
            break;
        case ADVENTURE_CONTROL_HERO_PAGE_PREVIOUS:
            if (gpCurPlayer->m_heroLocatorPage > 0) {
                gpCurPlayer->m_heroLocatorPage--;
                UpdateHeroLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_HERO_PAGE_NEXT:
            if (gpCurPlayer->m_heroLocatorPage + LOCATOR_VISIBLE_COUNT < gpCurPlayer->m_heroCount) {
                gpCurPlayer->m_heroLocatorPage++;
                UpdateHeroLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_HERO_KNOB:
            DoHeroKnob();
            break;
        case ADVENTURE_CONTROL_HERO_SCROLL:
            gpMouseManager->MouseCoords(curX, curY);
            curY -= LOCATOR_SCROLL_MOUSE_BASE_Y;
            if (gpCurPlayer->m_heroCount > LOCATOR_VISIBLE_COUNT) {
                iPage = curY
                        / (LOCATOR_SCROLL_MOUSE_SPAN
                           / (gpCurPlayer->m_heroCount - (LOCATOR_VISIBLE_COUNT - 1)));
                if (iPage > gpCurPlayer->m_heroCount - LOCATOR_VISIBLE_COUNT)
                    iPage = gpCurPlayer->m_heroCount - LOCATOR_VISIBLE_COUNT;
            } else {
                iPage = 0;
            }
            gpCurPlayer->m_heroLocatorPage = iPage;
            UpdateHeroLocators(1, 1);
            break;
        case ADVENTURE_CONTROL_TOWN_KNOB:
            DoTownKnob();
            break;
        case ADVENTURE_CONTROL_TOWN_SCROLL:
            gpMouseManager->MouseCoords(curX, curY);
            curY -= LOCATOR_SCROLL_MOUSE_BASE_Y;
            if (gpCurPlayer->m_townCount > LOCATOR_VISIBLE_COUNT) {
                iPage = curY
                        / (LOCATOR_SCROLL_MOUSE_SPAN
                           / (gpCurPlayer->m_townCount - (LOCATOR_VISIBLE_COUNT - 1)));
                if (iPage > gpCurPlayer->m_townCount - LOCATOR_VISIBLE_COUNT)
                    iPage = gpCurPlayer->m_townCount - LOCATOR_VISIBLE_COUNT;
            } else {
                iPage = 0;
            }
            gpCurPlayer->m_townLocatorPage = iPage;
            UpdateTownLocators(1, 1);
            break;
        case ADVENTURE_CONTROL_TOWN_PAGE_PREVIOUS:
            if (gpCurPlayer->m_townLocatorPage > 0) {
                gpCurPlayer->m_townLocatorPage--;
                UpdateTownLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_TOWN_PAGE_NEXT:
            if (gpCurPlayer->m_townLocatorPage + LOCATOR_VISIBLE_COUNT < gpCurPlayer->m_townCount) {
                gpCurPlayer->m_townLocatorPage++;
                UpdateTownLocators(1, 1);
            }
            break;
        case ADVENTURE_CONTROL_MAP_VIEW:
            if (!(gpGame->m_mapExtra[m_lastHoverCell + m_mapOriginX][m_hoverCellY + m_mapOriginY]
                  & giCurPlayerBit))
                isVisible = 0;
            hoverCell = GetCell(m_lastHoverCell + m_mapOriginX, m_hoverCellY + m_mapOriginY);
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                if (!isVisible) {
                    QuickInfo(m_lastHoverCell, m_hoverCellY);
                } else {
                    if (m_lastHoverCell == ADVMGR_VIEW_CENTER && m_hoverCellY == ADVMGR_VIEW_CENTER
                        && gpCurPlayer->CurrentHero() != INVALID_HERO && m_heroContextLocked) {
                        cellType = MAP_OBJECT_HERO;
                        mapIndex = gpCurPlayer->CurrentHero();
                    } else {
                        cellType = hoverCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                        mapIndex = hoverCell->m_objectMetadata;
                    }
                    switch (cellType) {
                        case MAP_OBJECT_HERO:
                            curX = m_lastHoverCell * CELL_PIXELS - HERO_QUICK_VIEW_X_OFFSET;
                            if (curX < QUICK_VIEW_MIN_X)
                                curX = QUICK_VIEW_MIN_X;
                            if (curX + HERO_QUICK_VIEW_WIDTH > QUICK_VIEW_RIGHT)
                                curX = HERO_QUICK_VIEW_RIGHT_X;
                            curY = m_hoverCellY * CELL_PIXELS - HERO_QUICK_VIEW_Y_OFFSET;
                            if (curY < QUICK_VIEW_MIN_Y)
                                curY = QUICK_VIEW_MIN_Y;
                            if (curY + HERO_QUICK_VIEW_HEIGHT > QUICK_VIEW_BOTTOM)
                                curY = HERO_QUICK_VIEW_BOTTOM_Y;
                            HeroQuickView(mapIndex, QUICK_VIEW_NO_LOCATOR, curX, curY);
                            break;
                        case MAP_OBJECT_TOWN:
                            curX = m_lastHoverCell * CELL_PIXELS - TOWN_QUICK_VIEW_X_OFFSET;
                            if (curX < QUICK_VIEW_MIN_X)
                                curX = QUICK_VIEW_MIN_X;
                            if (curX + TOWN_QUICK_VIEW_WIDTH > QUICK_VIEW_RIGHT)
                                curX = TOWN_QUICK_VIEW_RIGHT_X;
                            curY = m_hoverCellY * CELL_PIXELS - TOWN_QUICK_VIEW_Y_OFFSET;
                            if (curY < QUICK_VIEW_MIN_Y)
                                curY = QUICK_VIEW_MIN_Y;
                            if (curY + TOWN_QUICK_VIEW_HEIGHT > QUICK_VIEW_BOTTOM)
                                curY = TOWN_QUICK_VIEW_BOTTOM_Y;
                            TownQuickView(mapIndex, QUICK_VIEW_NO_LOCATOR, curX, curY);
                            break;
                        default:
                            if (gpGame->m_mapExtra[m_lastHoverCell + m_mapOriginX]
                                                  [m_hoverCellY + m_mapOriginY]
                                & giCurPlayerBit)
                                QuickInfo(m_lastHoverCell, m_hoverCellY);
                            break;
                    }
                }
            } else if (isVisible) {
                hero = NULL;
                mobileResult = 0;
                if (gpCurPlayer->m_currentHero != INVALID_HERO) {
                    hero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                    mobileResult = gpGame->IsMobile(hero->m_id);
                }
                if (hero) {
                    if (m_lastHoverCell == ADVMGR_VIEW_CENTER && m_hoverCellY == ADVMGR_VIEW_CENTER
                        && gpCurPlayer->CurrentHero() != INVALID_HERO && m_heroContextLocked) {
                        m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                        DoAdvCommand();
                    } else if ((!mobileResult
                                || (message->modifiers & MESSAGE_MODIFIER_CONTROL_KEYS)
                                || (gConfig.showRoute
                                    && (hero->m_destinationX != m_commandTargetX
                                        || hero->m_destinationY != m_commandTargetY)))
                               && gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY]
                                      .visited) {
                        hero->m_destinationX = m_commandTargetX;
                        hero->m_destinationY = m_commandTargetY;
                        ShowRoute(1, 1, 1);
                    } else {
                        *eventCell = DoAdvCommand();
                    }
                } else {
                    cellType = hoverCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                    mapIndex = hoverCell->m_objectMetadata;
                    if (cellType == MAP_OBJECT_HERO) {
                        if (gpCurPlayer->CurrentHero() == mapIndex) {
                            m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                            DoAdvCommand();
                        } else if (gpGame->GetHero(mapIndex)->m_owner == giCurPlayer) {
                            SetHeroContext(mapIndex, 0);
                        }
                    }
                    if (cellType == MAP_OBJECT_TOWN) {
                        if (gpCurPlayer->CurrentTown() == mapIndex) {
                            m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                            *eventCell = DoAdvCommand();
                        } else if (gpGame->GetTown(mapIndex)->m_owner == giCurPlayer) {
                            SetTownContext(mapIndex);
                        }
                    }
                }
            }
            break;
        case ADVENTURE_CONTROL_RADAR:
            if (message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                NormalDialog(
                    "World Map (Left click to move viewing area).",
                    NORMAL_DIALOG_TYPE_QUICK_VIEW
                );
                break;
            }
            DemobilizeCurrHero();
            gpMouseManager->MouseCoords(curX, curY);
            curX = (curX - RADAR_LEFT) / RADAR_CELL_PIXELS;
            curY = (curY - RADAR_TOP) / RADAR_CELL_PIXELS;
            m_mapOriginX = curX - ADVMGR_VIEW_CENTER;
            m_mapOriginY = curY - ADVMGR_VIEW_CENTER;
            if (m_mapOriginX < SCROLL_MIN_ORIGIN)
                m_mapOriginX = SCROLL_MIN_ORIGIN;
            if (m_mapOriginY < SCROLL_MIN_ORIGIN)
                m_mapOriginY = SCROLL_MIN_ORIGIN;
            if (m_mapOriginX > SCROLL_MAX_ORIGIN)
                m_mapOriginX = SCROLL_MAX_ORIGIN;
            if (m_mapOriginY > SCROLL_MAX_ORIGIN)
                m_mapOriginY = SCROLL_MAX_ORIGIN;
            UpdateRadar(1, 0);
            CompleteDraw(0);
            UpdateScreen(0, 0);
            inputMessage.type = MESSAGE_NONE;
            while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP) {
                Process1WindowsMessage();
                inputMessage = gpInputManager->GetEvent();
                mouseMsg = inputMessage;
                while (inputMessage.type != MESSAGE_LEFT_BUTTON_UP
                       && inputMessage.type != MESSAGE_NONE) {
                    if (inputMessage.type == MESSAGE_MOUSE_MOVE)
                        mouseMsg = inputMessage;
                    Process1WindowsMessage();
                    inputMessage = gpInputManager->GetEvent();
                }
                if (mouseMsg.type == MESSAGE_MOUSE_MOVE) {
                    if (mouseMsg.x < RADAR_LEFT)
                        mouseMsg.x = RADAR_LEFT;
                    if (mouseMsg.x >= RADAR_RIGHT)
                        mouseMsg.x = RADAR_RIGHT - 1;
                    if (mouseMsg.y < RADAR_TOP)
                        mouseMsg.y = RADAR_TOP;
                    if (mouseMsg.y >= RADAR_BOTTOM)
                        mouseMsg.y = RADAR_BOTTOM - 1;
                    gpMouseManager->Main(mouseMsg);
                    curX = (mouseMsg.x - RADAR_LEFT) / RADAR_CELL_PIXELS;
                    curY = (mouseMsg.y - RADAR_TOP) / RADAR_CELL_PIXELS;
                    m_mapOriginX = curX - ADVMGR_VIEW_CENTER;
                    m_mapOriginY = curY - ADVMGR_VIEW_CENTER;
                    if (m_mapOriginX < SCROLL_MIN_ORIGIN)
                        m_mapOriginX = SCROLL_MIN_ORIGIN;
                    if (m_mapOriginY < SCROLL_MIN_ORIGIN)
                        m_mapOriginY = SCROLL_MIN_ORIGIN;
                    if (m_mapOriginX > SCROLL_MAX_ORIGIN)
                        m_mapOriginX = SCROLL_MAX_ORIGIN;
                    if (m_mapOriginY > SCROLL_MAX_ORIGIN)
                        m_mapOriginY = SCROLL_MAX_ORIGIN;
                    UpdateRadar(1, 0);
                    CompleteDraw(0);
                    UpdateScreen(0, 0);
                    mouseMsg.type = MESSAGE_NONE;
                }
            }
            break;
        default:
            break;
    }
    if ((message->modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && message->id >= BOTTOM_VIEW_DRAW_FIRST_WIDGET
        && message->id <= BOTTOM_VIEW_DRAW_LAST_WIDGET)
        NormalDialog(
            "Status Window\n\nThis window provides information on the status of your hero or "
            "kingdom, "
            "and shows the date.  Left click here to cycle through these windows.",
            NORMAL_DIALOG_TYPE_QUICK_VIEW
        );
    return 1;
}

// donor PoL RVA 0x00059c19; preferred Buka symbol ?ProcessDeSelect@advManager@@QAEHPAUtag_message@@PAHPAPAVmapCell@@@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.346709;margin=0.551065;shape=0.313;size=0.541;calls=0.500;alternate=pol20:int advManager::ProcessDeSelect(struct tag_message *, int *, class mapCell * *)@0x00059c19
VA(0x00428b59, 0x1ea)
int advManager::ProcessDeSelect(
    struct tag_message* message,
    int* result,
    class mapCell** eventCell
) {
    switch (message->id) {
        case ADVENTURE_CONTROL_CONTINUE_ROUTE:
            m_selectedCell = ADVMGR_COMMAND_CONTINUE_ROUTE;
            *eventCell = DoAdvCommand();
            break;
        case ADVENTURE_CONTROL_ADVENTURE_OPTIONS:
            AdvPanel();
            break;
        case ADVENTURE_CONTROL_GAME_OPTIONS:
            *result = ControlPanel();
            break;
        case ADVENTURE_CONTROL_END_TURN:
            if (gpCurPlayer->HasMobileHero()) {
                NormalDialog(
                    "One or more Heroes may still move, are you sure you want to end your turn?",
                    NORMAL_DIALOG_TYPE_YES_NO
                );
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                    break;
            }
            gpGame->NextPlayer();
            break;
        case ADVENTURE_CONTROL_NEXT_HERO:
            HideRoute(1, 0, 1);
            SetHeroContext(gpCurPlayer->NextHero(1), 0);
            break;
        case ADVENTURE_CONTROL_OVERVIEW:
            gpGame->Overview();
            RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
            break;
    }
    if (message->id >= BOTTOM_VIEW_DRAW_FIRST_WIDGET
        && message->id <= BOTTOM_VIEW_DRAW_LAST_WIDGET) {
        if (giBottomViewOverride == BOTTOM_VIEW_KINGDOM)
            giBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else if (giBottomViewOverride != BOTTOM_VIEW_NONE)
            giBottomViewOverride = BOTTOM_VIEW_NONE;
        else if (iCurBottomView == BOTTOM_VIEW_KINGDOM)
            giBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else
            giBottomViewOverride = BOTTOM_VIEW_KINGDOM;
        giBottomViewOverrideEndTime = KBTickCount() + 3000;
        UpdBottomView(1, 1, 1);
    }
    return 1;
}

// donor PoL RVA 0x0005a07c; preferred Buka symbol ?ProcessSearch@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.676641;margin=0.529889;shape=0.365;size=0.904;calls=0.935;strings=%s%s|DIGSOUND.82M;alternate=pol20:int advManager::ProcessSearch(int, int)@0x0005a07c
VA(0x00428d43, 0x49b)
int advManager::ProcessSearch(int x, int y) {
    SAMPLE2 sampleData = NULL_SAMPLE2;
    int gaveArtifact;
    hero* myHero;
    mapCell* pCell;
    tag_message message;
    int i;

    myHero = &gpGame->m_heroRecs[gpCurPlayer->CurrentHero()];
    if (myHero->m_mobility != myHero->m_remainingMobility) {
        NormalDialog(
            "Digging for artifacts requires a whole day, try again tomorrow.",
            NORMAL_DIALOG_TYPE_OK
        );
        return 1;
    }
    MobilizeCurrHero(0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (x == ADVMGR_SEARCH_VIEW_CENTER) {
        x = m_mapOriginX + ADVMGR_VIEW_CENTER;
        y = m_mapOriginY + ADVMGR_VIEW_CENTER;
    }
    pCell = GetCell(x, y);
    if (pCell->m_objectIndex != MAP_CELL_NO_FRAME || pCell->m_overlayIndex != MAP_CELL_NO_FRAME) {
        NormalDialog("Try searching on clear ground.", NORMAL_DIALOG_TYPE_OK);
        return 1;
    }
    if (pCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN) {
        NormalDialog("Try looking on land!!!", NORMAL_DIALOG_TYPE_OK);
        return 1;
    }
    if (gbHumanPlayer[giCurPlayer])
        sampleData = LoadPlaySample("DIGSOUND.82M");
    if (pCell->m_objectIndex == MAP_CELL_NO_FRAME) {
        pCell->m_objectTileset = TILESET_OBJ32_07;
        pCell->m_objectIndex = DIG_HOLE_FRAME;
        pCell->m_flags |= MAP_CELL_OBJECT_SHADOW_ONLY;
    }
    CompleteDraw(0);
    UpdateScreen(0, 0);
    GrabScreen();

    if (gpGame->m_ultimateArtifactX == x && gpGame->m_ultimateArtifactY == y
        && gpGame->m_ultimateArtifactId != ARTIFACT_NONE) {
        gaveArtifact = GiveArtifact(myHero, gpGame->m_ultimateArtifactId);
        if (gaveArtifact == GIVE_ARTIFACT_NO_SLOT) {
            NormalDialog(
                "You have no room to carry another artifact!",
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x28
            );
        } else {
            if (gbHumanPlayer[giCurPlayer]) {
                EventSound(MAP_OBJECT_ULTIMATE_ARTIFACT, 0);
                sprintf(
                    gText,
                    "%s%s",
                    "Congratulations! After spending many hours digging here, you have uncovered "
                    "the ",
                    gArtifactNames[gpGame->m_ultimateArtifactId]
                );
                if (gpGame->m_campaignType > 0 && gpGame->m_campaignScenario == 2) {
                    sprintf(
                        gText,
                        "After spending many hours digging here, you have uncovered the Eye of "
                        "Goros!!!!"
                    );
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
                } else {
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
                    myHero->ViewArtifact(gpGame->m_ultimateArtifactId, 0);
                }
                gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
            } else if (gpGame->m_campaignType > 0 && gpGame->m_campaignScenario == 2) {
                sprintf(
                    gText,
                    "A great tragedy - the enemy has found the Eye of Goros!!!  The people abandon "
                    "you, all is lost."
                );
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0xb1, 0x1c);
            }
            gpGame->m_ultimateArtifactId = ARTIFACT_NONE;
        }
    } else if (gbHumanPlayer[giCurPlayer]) {
        NormalDialog("Nothing here.\nWhere could it be?", NORMAL_DIALOG_TYPE_OK, 0x61, 0x28);
    }
    if (gbHumanPlayer[giCurPlayer])
        WaitEndSample(sampleData, SAMPLE_WAIT_DEFAULT);
    for (i = 0; i < gpGame->m_playerCount; i++)
        ComputeUALoc(i);
    myHero->m_remainingMobility = 0;
    UpdBottomView(1, 1, 1);
    CheckDimHero();
    Reseed(0, 0);
    CheckEndGame(0);
    return 1;
}

// donor PoL RVA 0x0005a644; preferred Buka symbol ?ProcessHover@advManager@@QAEHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.464646;margin=0.430920;shape=0.299;size=0.767;calls=0.971;alternate=pol20:int advManager::ProcessHover(int, int)@0x0005a644
VA(0x004291de, 0xc02)
int advManager::ProcessHover(struct tag_message* message) {
    short curX;
    short curY;
    town* pTown;
    hero* hero;
    mapCell* cell;
    int nDays;
    signed char trigType;
    short heroPosX;
    short heroPosY;
    int baseFrame;

    switch (message->id) {
        case ADVENTURE_CONTROL_MAP_VIEW:
            gpMouseManager->MouseCoords(curX, curY);
            if (curX > ADVENTURE_VIEWPORT_EXTENT) {
                gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                return 1;
            }
            curX = curX / CELL_PIXELS;
            curY = curY / CELL_PIXELS;
            if (curX < 0)
                curX = 0;
            if (curY < 0)
                curY = 0;
            if (curX > ADVMGR_VIEW_CELL_COUNT - 1)
                curX = ADVMGR_VIEW_CELL_COUNT - 1;
            if (curY > ADVMGR_VIEW_CELL_COUNT - 1)
                curY = ADVMGR_VIEW_CELL_COUNT - 1;
            if (m_lastHoverCell != curX || m_hoverCellY != curY) {
                m_selectedCell = ADVMGR_COMMAND_NONE;
                m_lastHoverCell = curX;
                m_hoverCellY = curY;
                m_commandTargetX = m_mapOriginX + curX;
                m_commandTargetY = m_mapOriginY + curY;
                if (m_commandTargetX < 0 || m_commandTargetY < 0
                    || m_commandTargetX > MAP_CELL_GRID_SIZE - 1
                    || m_commandTargetY > MAP_CELL_GRID_SIZE - 1
                    || !(gpGame->m_mapExtra[m_commandTargetX][m_commandTargetY] & giCurPlayerBit)) {
                    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                    return 1;
                }
                cell = GetCell(m_commandTargetX, m_commandTargetY);
                if (gpCurPlayer->m_currentHero == INVALID_HERO) {
                    if ((cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TOWN
                        && gpGame->GetTown(cell->m_objectMetadata)->m_owner == giCurPlayer) {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_TOWN);
                        m_selectedCell = ADVMGR_COMMAND_TOWN_VIEW;
                        return 1;
                    } else if ((cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_HERO
                               && gpGame->GetHero(cell->m_objectMetadata)->m_owner == giCurPlayer) {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_HERO);
                        m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                        return 1;
                    } else {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                } else {
                    hero = gpGame->GetHero(gpCurPlayer->m_currentHero);
                    heroPosX = hero->m_x - m_mapOriginX;
                    heroPosY = hero->m_y - m_mapOriginY;
                    if (curX == heroPosX && curY == heroPosY) {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_HERO);
                        m_selectedCell = ADVMGR_COMMAND_HERO_VIEW;
                        return 1;
                    }
                    if (cell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED) {
                        if ((cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TOWN) {
                            pTown = gpGame->GetTown(cell->m_objectMetadata);
                            if (pTown->m_owner == giCurPlayer && m_commandTargetY >= 1
                                && m_commandTargetY < MAP_CELL_GRID_SIZE - 1
                                && (GetCell(m_commandTargetX, m_commandTargetY - 1)->m_triggerType
                                    & MAP_TRIGGER_TYPE_MASK)
                                       == MAP_OBJECT_TOWN
                                && (GetCell(m_commandTargetX, m_commandTargetY + 1)->m_triggerType
                                    & MAP_TRIGGER_TYPE_MASK)
                                       == MAP_OBJECT_TOWN) {
                                gpMouseManager->SetPointer(ADVENTURE_POINTER_TOWN);
                                m_selectedCell = ADVMGR_COMMAND_SELECT_TOWN;
                                return 1;
                            }
                        }
                        gpSearchArray->m_pathLength = 0;
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                    if (!((m_cursorType == ADVMGR_HERO_ICON_BOAT
                           || cell->m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                           || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                           || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
                           || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK))
                          && (m_cursorType != ADVMGR_HERO_ICON_BOAT
                              || cell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                              || cell->m_triggerType == MAP_OBJECT_COAST))) {
                        gpSearchArray->m_pathLength = 0;
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                    SeedTo(m_commandTargetX, m_commandTargetY);
                    if (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].visited) {
                        if (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                            <= hero->m_remainingMobility) {
                            nDays = 0;
                        } else {
                            nDays =
                                (gpSearchArray->m_cells[m_commandTargetX][m_commandTargetY].distance
                                 - hero->m_remainingMobility)
                                    / hero->m_mobility
                                + 1;
                            if (nDays > ADVENTURE_POINTER_DAY_LAST)
                                nDays = ADVENTURE_POINTER_DAY_LAST;
                        }
                        baseFrame = nDays * ADVENTURE_POINTER_DAY_STRIDE;
                        switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                            case MAP_OBJECT_SHIP:
                                if (m_cursorType != ADVMGR_HERO_ICON_BOAT) {
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_SAIL);
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                } else {
                                    gpMouseManager->SetPointer(baseFrame);
                                }
                                break;
                            case MAP_OBJECT_COAST:
                                if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_DISEMBARK
                                    );
                                else if (mapExtra[m_commandTargetX][m_commandTargetY]
                                         & MAP_EXTRA_MONSTER_ADJACENT)
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                else
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_MOVE);
                                m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                break;
                            case MAP_OBJECT_MONSTER:
                                gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_ATTACK);
                                m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                break;
                            case MAP_OBJECT_HERO:
                                if (gpGame->GetHero(cell->m_objectMetadata)->m_owner
                                    != giCurPlayer) {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                } else {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_SELECT_HERO
                                    );
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                }
                                break;
                            case MAP_OBJECT_TOWN:
                                pTown = gpGame->GetTown(cell->m_objectMetadata);
                                if ((cell->m_triggerType & MAP_TRIGGER_EVENT)
                                    && pTown->m_owner != giCurPlayer && pTown->HasGarrison()) {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                    m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                    break;
                                }
                                goto defaultHover;
                            default:
                            defaultHover:
                                trigType = cell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                                if ((mapExtra[m_commandTargetX][m_commandTargetY]
                                     & MAP_EXTRA_MONSTER_ADJACENT)
                                    && m_cursorType != ADVMGR_HERO_ICON_BOAT
                                    && trigType != MAP_OBJECT_SKELETON
                                    && trigType != MAP_OBJECT_TREASURE_CHEST
                                    && trigType != MAP_OBJECT_CAMPFIRE
                                    && trigType != MAP_OBJECT_ANCIENT_LAMP
                                    && trigType != MAP_OBJECT_RESOURCE
                                    && trigType != MAP_OBJECT_ARTIFACT) {
                                    gpMouseManager->SetPointer(
                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                    );
                                } else if (cell->m_triggerType & MAP_TRIGGER_EVENT) {
                                    if (m_cursorType != ADVMGR_HERO_ICON_BOAT) {
                                        switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                                            case MAP_OBJECT_ALCHEMIST_LAB:
                                            case MAP_OBJECT_SIGNPOST:
                                            case MAP_OBJECT_SKELETON:
                                            case MAP_OBJECT_DAEMON_CAVE:
                                            case MAP_OBJECT_TREASURE_CHEST:
                                            case MAP_OBJECT_FAERIE_RING:
                                            case MAP_OBJECT_CAMPFIRE:
                                            case MAP_OBJECT_FOUNTAIN:
                                            case MAP_OBJECT_GAZEBO:
                                            case MAP_OBJECT_ANCIENT_LAMP:
                                            case MAP_OBJECT_GRAVEYARD:
                                            case MAP_OBJECT_STRAW_HUT:
                                            case MAP_OBJECT_HOUSE:
                                            case MAP_OBJECT_CABIN:
                                            case MAP_OBJECT_DWARF_LOG_CABIN:
                                            case MAP_OBJECT_PEASANT_LOG_CABIN:
                                            case MAP_OBJECT_INN_1:
                                            case MAP_OBJECT_INN_2:
                                            case MAP_OBJECT_INN_3:
                                            case MAP_OBJECT_INN_4:
                                            case MAP_OBJECT_DRAGON_CITY:
                                            case MAP_OBJECT_LIGHTHOUSE:
                                            case MAP_OBJECT_WATERWHEEL:
                                            case MAP_OBJECT_MINE:
                                            case MAP_OBJECT_OBELISK:
                                            case MAP_OBJECT_OASIS:
                                            case MAP_OBJECT_RESOURCE:
                                            case MAP_OBJECT_SAWMILL:
                                            case MAP_OBJECT_RANKING_SHRINE:
                                            case MAP_OBJECT_SPELL_SHRINE:
                                            case MAP_OBJECT_SHIPWRECK:
                                            case MAP_OBJECT_STATUE:
                                            case MAP_OBJECT_DESERT_TENT:
                                            case MAP_OBJECT_TOWN:
                                            case MAP_OBJECT_STONE_LITHS:
                                            case MAP_OBJECT_WAGON_CAMP:
                                            case MAP_OBJECT_WELL:
                                            case MAP_OBJECT_WHIRLPOOL:
                                            case MAP_OBJECT_WINDMILL:
                                            case MAP_OBJECT_OAK_TREE:
                                            case MAP_OBJECT_MEGALITH:
                                            case MAP_OBJECT_ARTIFACT:
                                                gpMouseManager->SetPointer(
                                                    baseFrame + ADVENTURE_POINTER_ACTION
                                                );
                                                break;
                                            default:
                                                if (mapExtra[m_commandTargetX][m_commandTargetY]
                                                    & MAP_EXTRA_MONSTER_ADJACENT)
                                                    gpMouseManager->SetPointer(
                                                        baseFrame + ADVENTURE_POINTER_ATTACK
                                                    );
                                                else
                                                    gpMouseManager->SetPointer(
                                                        baseFrame + ADVENTURE_POINTER_MOVE
                                                    );
                                                break;
                                        }
                                    } else {
                                        switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                                            case MAP_OBJECT_BUOY:
                                            case MAP_OBJECT_WHIRLPOOL:
                                                gpMouseManager->SetPointer(
                                                    nDays + ADVENTURE_POINTER_WATER_ACTION
                                                );
                                                break;
                                            default:
                                                gpMouseManager->SetPointer(
                                                    baseFrame + ADVENTURE_POINTER_SAIL
                                                );
                                                break;
                                        }
                                    }
                                } else if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_SAIL);
                                } else {
                                    gpMouseManager->SetPointer(baseFrame + ADVENTURE_POINTER_MOVE);
                                }
                                m_selectedCell = ADVMGR_COMMAND_MOVE_TO;
                                break;
                        }
                        return 1;
                    } else {
                        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                        return 1;
                    }
                }
            }
            break;
        default:
            if (!(gpMouseManager->m_cursorFrame >= HOVER_SCROLL_FRAME_FIRST
                  && gpMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && MouseInScrollZone()))
                gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            return 1;
    }
    return 1;
}

// donor PoL RVA 0x0005b094; preferred Buka symbol ?UpdateScreen@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.492392;margin=0.157827;shape=0.294;size=0.928;calls=0.818;alternate=pol20:void advManager::UpdateScreen(int, int)@0x0005b094
VA(0x00429de0, 0x265)
void advManager::UpdateScreen(signed char cursorUpdate, signed char forceUpdate) {
    if (!forceUpdate && !bShowIt) {
        if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT])
            glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        return;
    }
    gpMouseManager
        ->SaveAndDraw(gpWindowManager->m_screen, m_updateMinX, m_updateMinY, cursorUpdate);
    PollSound();
    giScrollX = m_updateMinX;
    giScrollY = m_updateMinY;
    if (giLimitUpdMinX == UPDATE_NONE)
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_SIZE,
            UPDATE_VIEWPORT_SIZE,
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_ORIGIN
        );
    else
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            giLimitUpdMinX,
            giLimitUpdMinY,
            giLimitUpdMaxX - giLimitUpdMinX,
            giLimitUpdMaxY - giLimitUpdMinY,
            giLimitUpdMinX,
            giLimitUpdMinY
        );
    giScrollY = 0;
    giScrollX = giScrollY;
    PollSound();
    if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT]) {
        ++m_updateMaxX;
        if (m_updateMaxX >= UPDATE_FRAME_CYCLE)
            m_updateMaxX = 0;
        glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        if (m_updateMaxX == 1 || m_updateMaxX == 3 || m_updateMaxX == 5) {
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_1];
            m_animationPhases[ANIMATION_PHASE_COLUMN_1] %= UPDATE_ANIMATION_PHASES;
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_3];
            m_animationPhases[ANIMATION_PHASE_COLUMN_3] %= UPDATE_ANIMATION_PHASES;
        } else {
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_0];
            m_animationPhases[ANIMATION_PHASE_COLUMN_0] %= UPDATE_ANIMATION_PHASES;
            ++m_animationPhases[ANIMATION_PHASE_COLUMN_2];
            m_animationPhases[ANIMATION_PHASE_COLUMN_2] %= UPDATE_ANIMATION_PHASES;
        }
    }
    giLimitUpdMinX = UPDATE_NONE;
    gpMouseManager->RestoreUnderlying();
    Process1WindowsMessage();
}

// donor PoL RVA 0x0005b2ae; preferred Buka symbol ?CompleteDraw@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.459172;margin=1.327145;shape=0.409;size=0.723;calls=0.706;alternate=pol20:void advManager::CompleteDraw(int, int, int, int)@0x0005b2ae
VA(0x0042a045, 0x359)
void advManager::CompleteDraw(short originX, short originY, int forceDraw) {
    int drawX;
    int drawY;

    PollSound();
    if (!forceDraw && !bShowIt)
        return;

    giLimitUpdMinX = UPDATE_NONE;
    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    if (gbAllBlack)
        m_mapOriginX = m_mapOriginY = 0;
    m_comboHeroDrawn = 0;
    m_forceCompleteDraw = 0;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(
            originX + drawX,
            originY,
            drawX,
            0,
            ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT,
            0,
            forceDraw
        );
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++)
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(
                originX + drawX,
                originY + drawY,
                drawX,
                drawY,
                ADVMGR_DRAW_GROUND,
                0,
                forceDraw
            );

    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        PollSound();
        if (m_cursorDirection > MAP_DIRECTION_UNMIRRORED_LAST) {
            for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    0,
                    forceDraw
                );
        } else {
            for (drawX = ADVMGR_VIEW_CELL_COUNT - 1; drawX >= 0; drawX--)
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    0,
                    forceDraw
                );
        }
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(
                originX + drawX,
                originY + drawY,
                drawX,
                drawY,
                ADVMGR_DRAW_OBJECT,
                0,
                forceDraw
            );
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
        DrawCell(
            originX + drawX,
            originY + ADVMGR_VIEW_CELL_COUNT - 1,
            drawX,
            ADVMGR_VIEW_CELL_COUNT - 1,
            ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
            0,
            forceDraw
        );
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++)
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++)
            DrawCell(
                originX + drawX,
                originY + drawY,
                drawX,
                drawY,
                ADVMGR_DRAW_CLOUD,
                0,
                forceDraw
            );

    PollSound();
    UpdBottomView(0, 1, 1);
    DrawAdventureBorder();
    if (gbAllBlack) {
        m_mapOriginX = m_previousOriginX;
        m_mapOriginY = m_previousOriginY;
    }
}

// Buka 2.1 CompleteDraw(update) forwards the current map origin.
VA(0x0042a39e, 0x3a)
void advManager::CompleteDraw(int update) {
    CompleteDraw(m_mapOriginX, m_mapOriginY, update);
}

// Buka 2.1 GetCloudLookup over HoMM1's x-major visibility bytes: edge
// masks first, then each unseen neighbour, indexed into the cloud table.
VA(0x0042a3d8, 0x40d)
int advManager::GetCloudLookup(int x, int y) {
    int cloudMask = 0;

    if (x < 1)
        cloudMask |= CLOUD_WEST_EDGE;
    else if (x >= MAP_CELL_GRID_SIZE - 1)
        cloudMask |= CLOUD_EAST_EDGE;
    if (y < 1)
        cloudMask |= CLOUD_NORTH_EDGE;
    else if (y >= MAP_CELL_GRID_SIZE - 1)
        cloudMask |= CLOUD_SOUTH_EDGE;
    if (cloudMask == 0) {
        if ((gpGame->m_mapExtra[x][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH;
        if ((gpGame->m_mapExtra[x + 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_EAST;
        if ((gpGame->m_mapExtra[x][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH;
        if ((gpGame->m_mapExtra[x - 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_WEST;
        if ((gpGame->m_mapExtra[x + 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_EAST;
        if ((gpGame->m_mapExtra[x + 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_EAST;
        if ((gpGame->m_mapExtra[x - 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_WEST;
        if ((gpGame->m_mapExtra[x - 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_WEST;
    } else {
        if ((cloudMask & CLOUD_NORTH) == 0
            && (gpGame->m_mapExtra[x][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH;
        if ((cloudMask & CLOUD_EAST) == 0
            && (gpGame->m_mapExtra[x + 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_EAST;
        if ((cloudMask & CLOUD_SOUTH) == 0
            && (gpGame->m_mapExtra[x][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH;
        if ((cloudMask & CLOUD_WEST) == 0
            && (gpGame->m_mapExtra[x - 1][y] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_WEST;
        if ((cloudMask & CLOUD_NORTH_EAST) == 0
            && (gpGame->m_mapExtra[x + 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_EAST;
        if ((cloudMask & CLOUD_SOUTH_EAST) == 0
            && (gpGame->m_mapExtra[x + 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_EAST;
        if ((cloudMask & CLOUD_SOUTH_WEST) == 0
            && (gpGame->m_mapExtra[x - 1][y + 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_SOUTH_WEST;
        if ((cloudMask & CLOUD_NORTH_WEST) == 0
            && (gpGame->m_mapExtra[x - 1][y - 1] & giCurWatchPlayerBit) == 0)
            cloudMask |= CLOUD_NORTH_WEST;
    }
    return giCloudType[cloudMask];
}

// @early-stop 99.88: `s_drawGroundTile |= cell0->m_tileIndex;` - retail
// builds the result in eax (mov eax,ecx after the zero-extended byte load,
// then loads the global into cx); we or into ecx. 2 bytes, no other diff.
// Types match retail (byte m_tileIndex at +0 of the local cell0, unsigned
// 16-bit global: zero-extending loads); vc4trace: no handle state of
// s_drawGroundTile/cell0 changes it, solver distance 5 in every tier, no
// TU-state trial closes it. `a = a | b`, `a = b | a`, casts of the byte
// (unsigned short, short, int, unsigned) and of the result, and static
// linkage all compile identically.
// donor PoL RVA 0x0005bb7c; preferred Buka symbol ?DrawCell@advManager@@QAEXHHHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.286321;margin=0.495913;shape=0.238;size=0.410;calls=0.525;alternate=pol20:void advManager::DrawCell(int, int, int, int, int, int)@0x0005bb7c
VA(0x0042a7e5, 0xee8)
void advManager::DrawCell(
    short mapX,
    short mapY,
    short screenX,
    short screenY,
    signed char drawMask,
    signed char drawingPuzzle,
    signed char forceDraw
) {
    signed char frame;
    int heroYOffset6;
    int cursorSuppressed;
    short pixelY3;
    short pixelX7;
    mapCell* cell0;
    hero* drawHero;
    signed char iconIndex;
    signed char flagColor;
    signed char drawHeroIcon0;

    if (!forceDraw && !bShowIt)
        return;
    pixelX7 = screenX << CELL_PIXEL_SHIFT;
    pixelY3 = screenY << CELL_PIXEL_SHIFT;
    cell0 = GetCell(mapX, mapY);
    if (!gbAllBlack
        && (mapX < 0 || mapY < 0 || mapX >= MAP_CELL_GRID_SIZE || mapY >= MAP_CELL_GRID_SIZE)) {
        s_drawStoneTile = STONE_TILE_NONE;
        if (mapX == -1) {
            if (mapY == -1)
                s_drawStoneTile = STONE_TILE_TOP_LEFT;
            else if (mapY == MAP_CELL_GRID_SIZE)
                s_drawStoneTile = STONE_TILE_BOTTOM_LEFT;
            else if (mapY >= 0 && mapY < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = (mapY & CLOUD_VARIANT_MASK) + STONE_TILE_LEFT_BASE;
        } else if (mapX == MAP_CELL_GRID_SIZE) {
            if (mapY == -1)
                s_drawStoneTile = STONE_TILE_TOP_RIGHT;
            else if (mapY == MAP_CELL_GRID_SIZE)
                s_drawStoneTile = STONE_TILE_BOTTOM_RIGHT;
            else if (mapY >= 0 && mapY < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = (mapY & CLOUD_VARIANT_MASK) + STONE_TILE_RIGHT_BASE;
        } else if (mapY == -1) {
            if (mapX >= 0 && mapX < MAP_CELL_GRID_SIZE)
                s_drawStoneTile = (mapX & CLOUD_VARIANT_MASK) + STONE_TILE_TOP_BASE;
        } else if (mapY == MAP_CELL_GRID_SIZE && mapX >= 0 && mapX < MAP_CELL_GRID_SIZE) {
            s_drawStoneTile = (mapX & CLOUD_VARIANT_MASK) + STONE_TILE_BOTTOM_BASE;
        }
        if (s_drawStoneTile == STONE_TILE_NONE)
            s_drawStoneTile =
                (mapX + STONE_PATTERN_COORDINATE_SHIFT) % CLOUD_VARIANTS
                + ((mapY + STONE_PATTERN_COORDINATE_SHIFT) % CLOUD_VARIANTS) * CLOUD_VARIANTS;
        TileToBitmap(m_stoneTiles, s_drawStoneTile, gpWindowManager->m_screen, pixelX7, pixelY3);
        return;
    } else {
        if (!((!gbAllBlack && (gpGame->m_mapExtra[mapX][mapY] & giCurWatchPlayerBit))
              || drawingPuzzle)) {
            s_drawCovered = 1;
            if (gbAllBlack)
                s_drawCloudFrame = 0;
            else
                s_drawCloudFrame = GetCloudLookup(mapX, mapY);
            if (s_drawCloudFrame == 0) {
                if (drawMask & ADVMGR_DRAW_CLOUD)
                    TileToBitmap(
                        m_cloudTiles,
                        (mapX + mapY) & CLOUD_VARIANT_MASK,
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3
                    );
                return;
            }
            if (s_drawCloudFrame >= CLOUD_FLIPPED_FRAME_BASE) {
                s_drawFlipCloud = 1;
                s_drawCloudFrame -= CLOUD_FLIPPED_FRAME_BASE;
            } else {
                s_drawFlipCloud = 0;
            }
            if ((s_drawCloudFrame == CLOUD_X_ALTERNATE_FRAME_1
                 || s_drawCloudFrame == CLOUD_X_ALTERNATE_FRAME_2)
                && (mapX & 1))
                s_drawCloudFrame++;
            if (s_drawCloudFrame == CLOUD_Y_ALTERNATE_FRAME && (mapY & 1))
                s_drawCloudFrame++;
        } else {
            s_drawCovered = 0;
        }
    }
    if (drawMask & ADVMGR_DRAW_CLOUD) {
        if (s_drawCovered) {
            if (s_drawFlipCloud)
                FlipIconToBitmap(
                    m_cloudOverlayIcon,
                    gpWindowManager->m_screen,
                    pixelX7 + CELL_LAST_PIXEL,
                    pixelY3,
                    s_drawCloudFrame - 1,
                    ICON_DRAW_OFFSET_FULL
                );
            else
                IconToBitmap(
                    m_cloudOverlayIcon,
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    s_drawCloudFrame - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        } else if (m_routeShown && ADVMGR_VISIBILITY_AT(mapX, mapY)) {
            if (ADVMGR_VISIBILITY_AT(mapX, mapY) & ROUTE_CELL_FLIPPED)
                FlipIconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gpWindowManager->m_screen,
                    pixelX7 + CELL_LAST_PIXEL,
                    pixelY3 + ROUTE_DRAW_Y_OFFSET,
                    (ADVMGR_VISIBILITY_AT(mapX, mapY) & ROUTE_CELL_FRAME_MASK) - 1,
                    ICON_DRAW_OFFSET_FULL
                );
            else
                IconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3 + ROUTE_DRAW_Y_OFFSET,
                    (ADVMGR_VISIBILITY_AT(mapX, mapY) & ROUTE_CELL_FRAME_MASK) - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        return;
    }
    if (drawMask & ADVMGR_DRAW_GROUND) {
        s_drawGroundTile = cell0->m_flags;
        s_drawGroundTile <<= MAP_CELL_GROUND_FLIP_SHIFT;
        s_drawGroundTile |= cell0->m_tileIndex;
        TileToBitmap(m_groundTiles, s_drawGroundTile, gpWindowManager->m_screen, pixelX7, pixelY3);
        if (cell0->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY) {
            s_drawTileset = cell0->m_objectTileset & MAP_CELL_TILESET_MASK;
            if (!drawingPuzzle || s_drawTileset != TILESET_OBJ32_07
                || cell0->m_objectIndex != DIG_HOLE_FRAME)
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    cell0->m_objectIndex,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
    if (drawMask & ADVMGR_DRAW_OBJECT) {
        if (!(cell0->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
            && cell0->m_objectIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset = cell0->m_objectTileset & MAP_CELL_TILESET_MASK;
            if (s_drawTileset != TILESET_MONS32) {
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    cell0->m_objectIndex,
                    ICON_DRAW_OFFSET_FULL
                );
                if (cell0->m_flags & MAP_CELL_OBJECT_ANIMATED)
                    IconToBitmap(
                        m_objectIcons[s_drawTileset],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3,
                        cell0->m_objectIndex + m_updateMaxX + 1,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
        }
        if (cell0->m_flags & MAP_CELL_OBJECT_EXTRA)
            IconToBitmap(
                m_objectIcons[cell0->m_objectTileset >> MAP_CELL_EXTRA_TILESET_SHIFT],
                gpWindowManager->m_screen,
                pixelX7,
                pixelY3,
                cell0->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
    if (drawMask & ADVMGR_DRAW_HERO) {
        drawHeroIcon0 = 0;
        drawHero = NULL;
        if (!(cell0->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
            && cell0->m_objectIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset = cell0->m_objectTileset & MAP_CELL_TILESET_MASK;
            if (s_drawTileset == TILESET_MONS32 && cell0->m_objectIndex <= CREATURE_COUNT - 1) {
                if (m_lastQuickViewX == mapX && m_lastQuickViewY == mapY) {
                    if (m_mineGuardianFacingLeft)
                        FlipIconToBitmap(
                            m_objectIcons[TILESET_MINIMON],
                            gpWindowManager->m_screen,
                            pixelX7 + 36,
                            pixelY3 - MONSTER_DRAW_Y_OFFSET,
                            cell0->m_objectIndex * MONSTER_FRAME_STRIDE + MONSTER_FACING_FRAME_BASE,
                            ICON_DRAW_OFFSET_FULL
                        );
                    else
                        IconToBitmap(
                            m_objectIcons[TILESET_MINIMON],
                            gpWindowManager->m_screen,
                            pixelX7,
                            pixelY3 - MONSTER_DRAW_Y_OFFSET,
                            cell0->m_objectIndex * MONSTER_FRAME_STRIDE + MONSTER_FACING_FRAME_BASE,
                            ICON_DRAW_OFFSET_FULL
                        );
                } else {
                    ClipIconToBitmap(
                        m_objectIcons[TILESET_MINIMON],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 - MONSTER_DRAW_Y_OFFSET,
                        cell0->m_objectIndex * MONSTER_FRAME_STRIDE
                            + m_animationPhases[mapX & (ADVMGR_ANIMATION_PHASE_COUNT - 1)],
                        ICON_DRAW_OFFSET_FULL,
                        0,
                        0,
                        ADVENTURE_VIEWPORT_EXTENT,
                        ADVENTURE_VIEWPORT_EXTENT
                    );
                }
            }
        }
        if (cell0->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)) {
            flagColor = PLAYER_COLOR_NONE;
            iconIndex = ADVMGR_HERO_ICON_BOAT;
            frame = GetCursorBaseFrame(gpGame->m_boats[cell0->m_objectMetadata].direction);
            drawHeroIcon0 = 1;
            heroYOffset6 = HERO_BOAT_Y_OFFSET;
        } else {
            heroYOffset6 = 0;
            if (cell0->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                drawHero = gpGame->GetHero(cell0->m_objectMetadata);
                if (drawHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    flagColor = PLAYER_COLOR_NONE;
                else
                    flagColor = gpGame->m_players[drawHero->m_owner].m_color;
                if (drawHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    iconIndex = ADVMGR_HERO_ICON_BOAT;
                else
                    iconIndex = drawHero->m_heroClass;
                frame = GetCursorBaseFrame(drawHero->m_direction);
                drawHeroIcon0 = 1;
                if (drawHero->m_eventFlags & HERO_EVENT_EMBARKED)
                    heroYOffset6 = HERO_BOAT_Y_OFFSET;
            }
        }
        if (drawHeroIcon0) {
            if (frame & HERO_FRAME_MIRROR_FLAG) {
                if (screenX == 0 || screenY <= 1 || screenX == ADVMGR_VIEW_CELL_COUNT - 1
                    || screenY == ADVMGR_VIEW_CELL_COUNT - 1) {
                    FlipClippedIconToBitmap(
                        m_heroIcons[iconIndex],
                        gpWindowManager->m_screen,
                        pixelX7 + CELL_PIXELS,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        frame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
                    if (flagColor != PLAYER_COLOR_NONE)
                        FlipClippedIconToBitmap(
                            m_flagIcons[flagColor],
                            gpWindowManager->m_screen,
                            pixelX7 + CELL_PIXELS,
                            pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                            frame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                } else {
                    if (m_drawHeroShadows && iconIndex != ADVMGR_HERO_ICON_BOAT)
                        FlipDimIconToBitmap(
                            m_boatShadowIcon,
                            gpWindowManager->m_screen,
                            pixelX7 + CELL_PIXELS,
                            pixelY3 + CELL_LAST_PIXEL,
                            frame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                    FlipIconToBitmap(
                        m_heroIcons[iconIndex],
                        gpWindowManager->m_screen,
                        pixelX7 + CELL_PIXELS,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        frame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
                    if (flagColor != PLAYER_COLOR_NONE)
                        FlipIconToBitmap(
                            m_flagIcons[flagColor],
                            gpWindowManager->m_screen,
                            pixelX7 + CELL_PIXELS,
                            pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                            frame & HERO_FRAME_INDEX_MASK,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            } else if (screenX == 0 || screenY <= 1 || screenX == ADVMGR_VIEW_CELL_COUNT - 1
                       || screenY == ADVMGR_VIEW_CELL_COUNT - 1) {
                ClippedIconToBitmap(
                    m_heroIcons[iconIndex],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                    frame,
                    ICON_DRAW_OFFSET_FULL
                );
                if (flagColor != PLAYER_COLOR_NONE)
                    ClippedIconToBitmap(
                        m_flagIcons[flagColor],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        frame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
            } else {
                if (m_drawHeroShadows && iconIndex != ADVMGR_HERO_ICON_BOAT)
                    DimIconToBitmap(
                        m_boatShadowIcon,
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 + CELL_LAST_PIXEL,
                        frame,
                        ICON_DRAW_OFFSET_FULL
                    );
                IconToBitmap(
                    m_heroIcons[iconIndex],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                    frame,
                    ICON_DRAW_OFFSET_FULL
                );
                if (flagColor != PLAYER_COLOR_NONE)
                    IconToBitmap(
                        m_flagIcons[flagColor],
                        gpWindowManager->m_screen,
                        pixelX7,
                        pixelY3 + CELL_PIXELS - 1 + heroYOffset6,
                        frame & HERO_FRAME_INDEX_MASK,
                        ICON_DRAW_OFFSET_FULL
                    );
            }
        }
        if (m_cursorActive && (cell0->m_flags & MAP_CELL_HERO_CURSOR) && !m_comboHeroDrawn
            && m_mapOriginX + ADVMGR_VIEW_CENTER == mapX
            && m_mapOriginY + ADVMGR_VIEW_CENTER == mapY) {
            DrawCursor();
            m_comboHeroDrawn = 1;
        }
    }
    if (drawMask & ADVMGR_DRAW_OVERLAY) {
        if (cell0->m_overlayIndex != MAP_CELL_NO_FRAME) {
            s_drawTileset = cell0->m_overlayTileset & MAP_CELL_TILESET_MASK;
            IconToBitmap(
                m_objectIcons[s_drawTileset],
                gpWindowManager->m_screen,
                pixelX7,
                pixelY3,
                cell0->m_overlayIndex,
                ICON_DRAW_OFFSET_FULL
            );
            if (cell0->m_flags & MAP_CELL_OVERLAY_ANIMATED)
                IconToBitmap(
                    m_objectIcons[s_drawTileset],
                    gpWindowManager->m_screen,
                    pixelX7,
                    pixelY3,
                    cell0->m_overlayIndex + m_updateMaxX + 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        if (cell0->m_flags & MAP_CELL_OVERLAY_EXTRA)
            IconToBitmap(
                m_objectIcons[cell0->m_overlayTileset >> MAP_CELL_EXTRA_TILESET_SHIFT],
                gpWindowManager->m_screen,
                pixelX7,
                pixelY3,
                cell0->m_extraFrame,
                ICON_DRAW_OFFSET_FULL
            );
    }
}

// Buka 2.1 GetCell; HoMM1 returns the map base for any off-grid position.
VA(0x0042b6cd, 0x7d)
mapCell* advManager::GetCell(short x, short y) {
    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return m_mapData[0];
    else
        return &m_mapData[x][y];
}

VA(0x0042b74a, 0x57e)
void advManager::UpdateRadar(signed char updateScreen, int partial) {
    short y;
    int firstX;
    int lastY;
    short x;
    short color;
    short owner;
    int lastX;
    int firstY;
    mapCell* cellPtr;

    if (!partial) {
        firstX = 0;
        firstY = 0;
        lastX = MAP_CELL_GRID_SIZE - 1;
        lastY = MAP_CELL_GRID_SIZE - 1;
    } else {
        firstX = m_mapOriginX - 1;
        firstY = m_mapOriginY - 1;
        lastX = m_mapOriginX + ADVMGR_VIEW_CELL_COUNT;
        lastY = m_mapOriginY + ADVMGR_VIEW_CELL_COUNT;
        if (firstX < 0)
            firstX = 0;
        if (firstY < 0)
            firstY = 0;
        if (lastX > MAP_CELL_GRID_SIZE - 1)
            lastX = MAP_CELL_GRID_SIZE - 1;
        if (lastY > MAP_CELL_GRID_SIZE - 1)
            lastY = MAP_CELL_GRID_SIZE - 1;
    }

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;

    gpAdvManager->m_openState = 0;
    for (x = firstX; x <= lastX; x++) {
        for (y = firstY; y <= lastY; y++) {
            if (!(gpGame->m_mapExtra[x][y] & giCurPlayerBit)) {
                m_puzzleIcon->FillToBuffer(
                    x * RADAR_CELL_PIXELS + RADAR_LEFT,
                    y * RADAR_CELL_PIXELS + RADAR_TOP,
                    0,
                    0,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                continue;
            }
            cellPtr = &m_mapData[x][y];
            if ((cellPtr->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_HERO) {
                owner = gpGame->m_availableHeroes[cellPtr->m_objectMetadata];
                if (giCurPlayer == owner)
                    color = gRadarOwnerColor[owner >= 0 ? gpGame->m_players[owner].m_color : 4];
                else
                    color = gRadarTerrainColor[CELL_TERRAIN(cellPtr)];
            } else {
                switch (cellPtr->m_objectTileset & MAP_CELL_TILESET_MASK) {
                    case TILESET_TOWN32:
                        owner = gpGame->m_townOwners[cellPtr->m_objectMetadata];
                        color = gRadarOwnerColor[owner >= 0 ? gpGame->m_players[owner].m_color : 4];
                        break;
                    case TILESET_RSRC32:
                        switch (cellPtr->m_triggerType) {
                            case MAP_OBJECT_ALCHEMIST_LAB:
                            case MAP_OBJECT_MINE:
                            case MAP_OBJECT_SAWMILL:
                            case MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB:
                            case MAP_TRIGGER_EVENT | MAP_OBJECT_MINE:
                            case MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL:
                                owner = gpGame->m_mineOwners[cellPtr->m_objectMetadata];
                                color = gRadarOwnerColor
                                    [owner >= 0 ? gpGame->m_players[owner].m_color : 4];
                                break;
                            default:
                                color = gRadarTerrainColor[CELL_TERRAIN(cellPtr)];
                                break;
                        }
                        break;
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        color = gRadarTerrainColor[CELL_TERRAIN(cellPtr)] + RADAR_TERRAIN_SHADE;
                        break;
                    default:
                        color = gRadarTerrainColor[CELL_TERRAIN(cellPtr)];
                        break;
                }
            }
            m_puzzleIcon->FillToBuffer(
                x * RADAR_CELL_PIXELS + RADAR_LEFT,
                y * RADAR_CELL_PIXELS + RADAR_TOP,
                0,
                color,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        }
    }
    m_puzzleIcon->ClipFillToBuffer(
        m_mapOriginX * RADAR_CELL_PIXELS + RADAR_LEFT,
        m_mapOriginY * RADAR_CELL_PIXELS + RADAR_TOP,
        1,
        RADAR_VIEWPORT_COLOR,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL,
        RADAR_LEFT,
        RADAR_TOP,
        RADAR_SIZE,
        RADAR_SIZE
    );
    if (updateScreen)
        gpWindowManager->UpdateScreenRegion(
            firstX * RADAR_CELL_PIXELS + RADAR_LEFT,
            firstY * RADAR_CELL_PIXELS + RADAR_TOP,
            (lastX - firstX + 1) * RADAR_CELL_PIXELS,
            (lastY - firstY + 1) * RADAR_CELL_PIXELS
        );
}

// donor PoL RVA 0x0005f127; preferred Buka symbol ?QuickInfo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.437984;margin=0.160503;shape=0.304;size=0.325;calls=0.543;strings=qwikinfo.bin;alternate=pol20:void advManager::QuickInfo(int, int)@0x0005f127
VA(0x0042bcc8, 0x596)
void advManager::QuickInfo(short cellX, short cellY) {
    short posX;
    tag_message message;
    char savedTextLocal[200];
    mapCell* curCell;
    short posY;
    short flag;
    heroWindow* window;

    flag = 1;
    curCell = NULL;
    posX = cellX * CELL_PIXELS - QUICK_INFO_X_OFFSET;
    if (posX < QUICK_VIEW_MIN_X)
        posX = QUICK_VIEW_MIN_X;
    if (posX + QUICK_INFO_WIDTH > QUICK_VIEW_RIGHT)
        posX = QUICK_INFO_RIGHT_X;
    posY = cellY * CELL_PIXELS - QUICK_INFO_Y_OFFSET;
    if (posY < QUICK_VIEW_MIN_Y)
        posY = QUICK_VIEW_MIN_Y;
    if (posY + QUICK_INFO_HEIGHT > QUICK_VIEW_BOTTOM)
        posY = QUICK_INFO_BOTTOM_Y;

    window = new heroWindow(posX, posY, "qwikinfo.bin");
    if (!window)
        MemError();

    if (m_mapOriginX + cellX < 0 || m_mapOriginX + cellX >= MAP_CELL_GRID_SIZE
        || m_mapOriginY + cellY < 0 || m_mapOriginY + cellY >= MAP_CELL_GRID_SIZE) {
        sprintf(gText, "\n\n%s", "Border");
    } else {
        curCell = GetCell(m_mapOriginX + cellX, m_mapOriginY + cellY);
        if (!(gpGame->m_mapExtra[m_mapOriginX + cellX][m_mapOriginY + cellY] & giCurPlayerBit)) {
            sprintf(gText, "\n\n%s", "Uncharted territory");
        } else {
            switch (curCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                case MAP_OBJECT_ARTIFACT:
                    sprintf(gText, "\n\n%s", "Artifact");
                    break;
                case MAP_OBJECT_NONE:
                case MAP_OBJECT_COAST:
                case MAP_OBJECT_SHADOW:
                    sprintf(gText, "\n\n%s", gTerrainNames[CELL_TERRAIN(curCell)]);
                    break;
                case MAP_OBJECT_MINE:
                    sprintf(
                        gText,
                        "\n\n%s %s",
                        gResourceNames[gpGame->m_mines[curCell->m_objectMetadata].type],
                        "Mine"
                    );
                    break;
                case MAP_OBJECT_RESOURCE:
                    sprintf(gText, "\n\n%s", gSpellNames[curCell->m_objectIndex + 9]);
                    break;
                case 51:
                    sprintf(gText, "\n\n%s", gResourceNames[curCell->m_objectIndex + 2]);
                    break;
                case MAP_OBJECT_MONSTER:
                    sprintf(
                        gText,
                        "\n\n%s %s",
                        GetArmySizeName(
                            curCell->m_objectMetadata & MONSTER_COUNT_MASK,
                            ARMY_SIZE_NAME_SENTENCE
                        ),
                        gArmyNamesPlural[curCell->m_objectIndex]
                    );
                    break;
                default:
                    sprintf(
                        gText,
                        "\n\n%s",
                        gObjectNames[curCell->m_triggerType & MAP_TRIGGER_TYPE_MASK]
                    );
                    break;
            }
        }
    }

    strcpy(savedTextLocal, gText);
    if (giDebugLevel > 0 && curCell)
        sprintf(
            gText,
            "otile%d oi%d ot%d ei%d fl%d %s X%d Y%d",
            curCell->m_objectTileset,
            curCell->m_objectIndex,
            curCell->m_triggerType,
            curCell->m_objectMetadata,
            curCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY,
            savedTextLocal,
            m_mapOriginX + cellX,
            m_mapOriginY + cellY
        );
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, 1);
    message.text = gText;
    window->BroadcastMessage(message);
    GrabScreen();
    gpWindowManager->AddWindow(window, WINDOW_Z_ORDER_APPEND, 1);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(window);
    delete window;
    gpMouseManager->ShowSystemCursor();
}

// donor PoL RVA 0x00060465; preferred Buka symbol ?UpdateHeroLocator@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.539169;margin=0.099782;shape=0.447;size=0.791;calls=0.917;alternate=pol20:void advManager::UpdateHeroLocator(int, int, int)@0x00060465
VA(0x0042c25e, 0x3c8)
void advManager::UpdateHeroLocator(
    int locatorSlot,
    signed char drawWindow,
    signed char updateScreen
) {
    tag_message message;
    signed char whichHero;
    int wBase;
    int i;
    int activeHero;
    hero* hPtr;
    int moveFrame;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    if (locatorSlot == LOCATOR_SLOT_CURRENT_HERO) {
        activeHero = gpCurPlayer->CurrentHero();
        if (activeHero == INVALID_HERO)
            return;
        for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
            if (gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + i] == activeHero)
                locatorSlot = i;
        }
        if (locatorSlot == LOCATOR_SLOT_CURRENT_HERO)
            return;
    }
    wBase = locatorSlot * HERO_LOCATOR_WIDGET_STRIDE + HERO_LOCATOR_WIDGET_BASE;
    message.type = MESSAGE_WIDGET;
    whichHero = gpCurPlayer->m_heroIds[gpCurPlayer->m_heroLocatorPage + locatorSlot];
    message.command = WIDGET_COMMAND_SET_COLOR;
    message.id = wBase + HERO_LOCATOR_HIGHLIGHT;
    message.value = (gpCurPlayer->m_currentHero == whichHero
                     && gpCurPlayer->m_currentHero != INVALID_HERO && !gbAllBlack)
                        ? LOCATOR_HIGHLIGHT_COLOR
                        : 0;
    m_adventureWindow->BroadcastMessage(message);
    if (whichHero == INVALID_HERO || gbAllBlack) {
        message.id = wBase + HERO_LOCATOR_BUTTON;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = locatorSlot;
        m_adventureWindow->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_DRAW;
        for (i = 0; i <= HERO_LOCATOR_BUTTON - 1; i++) {
            message.id = i + wBase;
            m_adventureWindow->BroadcastMessage(message);
        }
    } else {
        hPtr = gpGame->GetHero(whichHero);
        message.id = wBase + HERO_LOCATOR_BUTTON;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = LOCATOR_FRAME_HERO;
        m_adventureWindow->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        for (i = 0; i <= HERO_LOCATOR_HIGHLIGHT; i++) {
            message.id = i + wBase;
            m_adventureWindow->BroadcastMessage(message);
        }
        moveFrame = hPtr->m_remainingMobility * 22 / 60;
        if (moveFrame < 0)
            moveFrame = 0;
        if (moveFrame > 30)
            moveFrame = 25;
        else if (moveFrame > 26)
            moveFrame = 24;
        else if (moveFrame > 23)
            moveFrame = 23;
        message.id = wBase + HERO_LOCATOR_MOBILITY;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = moveFrame;
        m_adventureWindow->BroadcastMessage(message);
        message.id = wBase + HERO_LOCATOR_PORTRAIT;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.value = whichHero;
        m_adventureWindow->BroadcastMessage(message);
        message.id = wBase + 3;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_adventureWindow->BroadcastMessage(message);
        message.id = wBase + 4;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        m_adventureWindow->BroadcastMessage(message);
    }
    if (drawWindow) {
        m_adventureWindow->DrawWindow(0, wBase, wBase + HERO_LOCATOR_HIGHLIGHT);
        if (updateScreen)
            gpWindowManager->UpdateScreenRegion(481, locatorSlot * 32 + 177, 54, 30);
    }
}

// donor PoL RVA 0x000607ad; preferred Buka symbol ?UpdateHeroLocators@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520706;margin=0.246406;shape=0.456;size=0.777;calls=0.750;alternate=pol20:void advManager::UpdateHeroLocators(int, int)@0x000607ad
VA(0x0042c626, 0x108)
void advManager::UpdateHeroLocators(signed char drawWindow, signed char updateScreen) {
    int locatorSlot;
    double scrollStep;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;

    for (locatorSlot = 0; locatorSlot < LOCATOR_VISIBLE_COUNT; ++locatorSlot)
        UpdateHeroLocator(locatorSlot, 0, 0);

    if (gpCurPlayer->m_heroCount < LOCATOR_PAGE_THRESHOLD) {
        m_scrollLeftButton->m_y = LOCATOR_SCROLL_NO_PAGES_Y;
    } else {
        scrollStep = 73.0 / (gpCurPlayer->m_heroCount - LOCATOR_PAGE_DENOMINATOR_OFFSET);
        m_scrollLeftButton->m_y =
            static_cast<short>(gpCurPlayer->m_heroLocatorPage * scrollStep + 195.0);
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

// donor PoL RVA 0x000608af; preferred Buka symbol ?UpdateTownLocators@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.535916;margin=0.510257;shape=0.379;size=0.971;calls=0.800;alternate=pol20:void advManager::UpdateTownLocators(int, int)@0x000608af
VA(0x0042c72e, 0x27f)
void advManager::UpdateTownLocators(signed char drawWindow, signed char updateScreen) {
    tag_message message;
    short i;
    signed char whichTown;
    double scrollStep;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    message.type = MESSAGE_WIDGET;
    for (i = 0; i < LOCATOR_VISIBLE_COUNT; i++) {
        whichTown = gpCurPlayer->m_townIds[gpCurPlayer->m_townLocatorPage + i];
        message.command = WIDGET_COMMAND_SET_COLOR;
        message.id = i + TOWN_LOCATOR_HIGHLIGHT_FIRST;
        message.value = (gpCurPlayer->m_currentTown != GAME_TOWN_NONE
                         && gpCurPlayer->m_currentTown == whichTown && !gbAllBlack)
                            ? LOCATOR_HIGHLIGHT_COLOR
                            : 0;
        m_adventureWindow->BroadcastMessage(message);
        message.id = i + ADVENTURE_CONTROL_TOWN_LOCATOR_1;
        if (whichTown == GAME_TOWN_NONE || gbAllBlack) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = i + LOCATOR_FRAME_EMPTY_TOWN_FIRST;
            m_adventureWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_ENABLED;
            m_adventureWindow->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_ENABLED;
            m_adventureWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = gpGame->GetTown(whichTown)->m_type + LOCATOR_FRAME_TOWN_FIRST;
            if (gpGame->GetTown(whichTown)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
                message.value += LOCATOR_FRAME_CASTLE_OFFSET;
            m_adventureWindow->BroadcastMessage(message);
        }
    }
    if (gpCurPlayer->m_townCount < LOCATOR_PAGE_THRESHOLD) {
        m_scrollRightButton->m_y = LOCATOR_SCROLL_NO_PAGES_Y;
    } else {
        scrollStep = 74.0 / (gpCurPlayer->m_townCount - LOCATOR_PAGE_DENOMINATOR_OFFSET);
        m_scrollRightButton->m_y =
            static_cast<short>(gpCurPlayer->m_townLocatorPage * scrollStep + 195.0);
    }
    if (drawWindow)
        m_adventureWindow->DrawWindow(updateScreen);
}

// donor PoL RVA 0x00060b97; preferred Buka symbol ?UpdBottomView@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.587774;margin=0.642167;shape=0.494;size=0.843;calls=1.000;alternate=pol20:void advManager::UpdBottomView(int, int, int)@0x00060b97
VA(0x0042c9ad, 0x19f)
void advManager::UpdBottomView(
    signed char forceUpdate,
    signed char drawWindow,
    signed char updateScreen
) {
    signed char updated;

    updated = 0;
    gbForceUpdate = forceUpdate;
    if (giBottomViewOverride == BOTTOM_VIEW_OVERRIDE_DISABLED)
        return;

    if (giBottomViewOverride > BOTTOM_VIEW_NONE) {
        if (KBTickCount() > giBottomViewOverrideEndTime) {
            giBottomViewOverride = BOTTOM_VIEW_NONE;
        } else {
            switch (giBottomViewOverride) {
                case BOTTOM_VIEW_NEW_TURN:
                    updated = UpdBottomViewNewTurn();
                    break;
                case BOTTOM_VIEW_KINGDOM:
                    updated = UpdBottomViewKingdom();
                    break;
                case BOTTOM_VIEW_RESOURCE:
                    updated = UpdBottomViewResMsg();
                    break;
            }
            goto update_bottom_view;
        }
    }

    if (!gbThisNetHumanPlayer[giCurPlayer] || gbAllBlack)
        updated = UpdBottomViewEnemyTurn();
    else if (gpCurPlayer->CurrentHero() == INVALID_HERO)
        updated = UpdBottomViewKingdom();
    else
        updated = UpdBottomViewHero();

update_bottom_view:
    if (updated && drawWindow) {
        m_adventureWindow
            ->DrawWindow(0, BOTTOM_VIEW_DRAW_FIRST_WIDGET, BOTTOM_VIEW_DRAW_LAST_WIDGET);
        if (updateScreen)
            gpWindowManager->UpdateScreenRegion(
                BOTTOM_VIEW_PANEL_X,
                BOTTOM_VIEW_PANEL_Y,
                BOTTOM_VIEW_PANEL_WIDTH,
                BOTTOM_VIEW_PANEL_HEIGHT
            );
    }
    forceUpdate = gbForceUpdate;
}

// donor PoL RVA 0x00060d63; preferred Buka symbol ?ClearBottomView@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.613154;margin=1.071587;shape=0.489;size=0.961;calls=1.000;alternate=pol20:void advManager::ClearBottomView(void)@0x00060d63
VA(0x0042cb4c, 0x132)
void advManager::ClearBottomView(void) {
    int widgetIndex;

    if (iCurBottomView == BOTTOM_VIEW_NONE)
        return;

    for (widgetIndex = 0; widgetIndex < ADVMGR_BOTTOM_VIEW_WIDGET_COUNT; ++widgetIndex) {
        if (m_bottomViewPrimaryWidgets[widgetIndex] != NULL) {
            m_adventureWindow->RemoveWidget(m_bottomViewPrimaryWidgets[widgetIndex]);
            delete m_bottomViewPrimaryWidgets[widgetIndex];
        }
        if (m_bottomViewSecondaryWidgets[widgetIndex] != NULL) {
            m_adventureWindow->RemoveWidget(m_bottomViewSecondaryWidgets[widgetIndex]);
            delete m_bottomViewSecondaryWidgets[widgetIndex];
        }
        m_bottomViewPrimaryWidgets[widgetIndex] = NULL;
        m_bottomViewSecondaryWidgets[widgetIndex] = NULL;
    }
    iCurBottomViewEnemy = BOTTOM_VIEW_NO_ENEMY;
    iCurBottomView = BOTTOM_VIEW_NONE;
    iLastAnimFrame = BOTTOM_VIEW_NO_ANIMATION;
}

// donor PoL RVA 0x00060e95; preferred Buka symbol ?UpdBottomViewEnemyTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.738388;margin=0.356312;shape=0.508;size=0.910;calls=0.857;strings=brcrest.icn|hourglas.icn|stonback.icn;alternate=pol20:int advManager::UpdBottomViewEnemyTurn(void)@0x00060e95
VA(0x0042cc7e, 0x5bf)
signed char advManager::UpdBottomViewEnemyTurn(void) {
    DATA(0x0048ff4c)
    static long iLastSandAnimTime = 0;
    DATA(0x0048ff50)
    static long iLastNewSandAnimTime = 0;
    signed char updated;
    tag_message message;

    updated = 0;
    message.type = MESSAGE_WIDGET;
    if (iCurBottomView != BOTTOM_VIEW_ENEMY_TURN) {
        updated = 1;
        gbForceUpdate = 1;
        ClearBottomView();
        iCurBottomView = BOTTOM_VIEW_ENEMY_TURN;

        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
            BOTTOM_VIEW_PANEL_X,
            BOTTOM_VIEW_PANEL_Y,
            BOTTOM_VIEW_PANEL_WIDTH,
            BOTTOM_VIEW_PANEL_HEIGHT,
            "stonback.icn",
            0,
            ICON_DRAW_NORMAL,
            BOTTOM_VIEW_BACKGROUND_ID,
            ICON_WIDGET_DRAW,
            1
        );
        if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
            ENEMY_TURN_BACKGROUND_Z
        );

        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
            ENEMY_TURN_HOURGLASS_X,
            ENEMY_TURN_HOURGLASS_Y,
            ENEMY_TURN_HOURGLASS_WIDTH,
            ENEMY_TURN_HOURGLASS_HEIGHT,
            "hourglas.icn",
            0,
            ICON_DRAW_NORMAL,
            BOTTOM_VIEW_FOREGROUND_ID,
            ICON_WIDGET_DRAW,
            1
        );
        if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
            ENEMY_TURN_HOURGLASS_Z
        );
    }

    if (gbForceUpdate || KBTickCount() - iLastSandAnimTime > ENEMY_TURN_ANIMATION_DELAY) {
        iLastSandAnimTime = KBTickCount();
        iLastAnimFrame = m_updateMaxX;
        if (KBTickCount() - iLastNewSandAnimTime > ENEMY_TURN_ANIMATION_DELAY) {
            iLastNewSandAnimTime = KBTickCount();
            iSandAnim++;
            if (iSandAnim >= ENEMY_TURN_SAND_FRAME_LIMIT)
                iSandAnim = ENEMY_TURN_SAND_RESTART_FRAME;
            updated = 1;
            if (m_bottomViewPrimaryWidgets[ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
                message.command = WIDGET_COMMAND_SET_FRAME;
                message.id = ENEMY_TURN_SAND_ID;
                message.value = iSandAnim + ENEMY_TURN_SAND_FRAME_OFFSET;
                m_adventureWindow->BroadcastMessage(message);
            } else {
                m_bottomViewPrimaryWidgets[ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                    new iconWidget(
                        ENEMY_TURN_ANIMATION_X,
                        ENEMY_TURN_ANIMATION_Y,
                        ENEMY_TURN_ANIMATION_WIDTH,
                        ENEMY_TURN_ANIMATION_HEIGHT,
                        "hourglas.icn",
                        iSandAnim + ENEMY_TURN_SAND_FRAME_OFFSET,
                        ICON_DRAW_NORMAL,
                        ENEMY_TURN_SAND_ID,
                        ICON_WIDGET_DRAW,
                        1
                    );
                if (!m_bottomViewPrimaryWidgets
                        [ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                    MemError();
                m_adventureWindow->AddWidget(
                    m_bottomViewPrimaryWidgets
                        [ENEMY_TURN_SAND_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                    ENEMY_TURN_SAND_Z
                );
            }
        }
    }

    if (gbForceUpdate || iCurBottomViewEnemy != giCurPlayer) {
        updated = 1;
        iCurBottomViewEnemy = giCurPlayer;
        if (iCurBottomViewEnemy != giCurPlayer)
            iCurHourGlassPhase = 0;
        if (m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = ENEMY_TURN_CREST_ID;
            message.value = gpGame->m_players[giCurPlayer].Color();
            m_adventureWindow->BroadcastMessage(message);
        } else {
            m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                new iconWidget(
                    ENEMY_TURN_CREST_X,
                    ENEMY_TURN_ANIMATION_Y,
                    ENEMY_TURN_ANIMATION_WIDTH,
                    ENEMY_TURN_ANIMATION_HEIGHT,
                    "brcrest.icn",
                    gpGame->m_players[giCurPlayer].Color(),
                    ICON_DRAW_NORMAL,
                    ENEMY_TURN_CREST_ID,
                    ICON_WIDGET_DRAW,
                    1
                );
            if (!m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                MemError();
            m_adventureWindow->AddWidget(
                m_bottomViewPrimaryWidgets[ENEMY_TURN_CREST_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                ENEMY_TURN_CREST_Z
            );
        }
    }

    if (gbForceUpdate || iCurHourGlassPhase < iLastHourGlassPhase || iLastHourGlassPhase < 0
        || (iCurHourGlassPhase > iLastHourGlassPhase
            && KBTickCount() - giLastHourGlassUpdateTime >= ENEMY_TURN_PHASE_DELAY)) {
        updated = 1;
        iLastHourGlassPhase = iCurHourGlassPhase;
        giLastHourGlassUpdateTime = KBTickCount();
        if (m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST]) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = ENEMY_TURN_PHASE_ID;
            message.value = iCurHourGlassPhase + ENEMY_TURN_PHASE_FRAME_OFFSET;
            m_adventureWindow->BroadcastMessage(message);
        } else {
            m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                new iconWidget(
                    ENEMY_TURN_ANIMATION_X,
                    ENEMY_TURN_ANIMATION_Y,
                    ENEMY_TURN_ANIMATION_WIDTH,
                    ENEMY_TURN_ANIMATION_HEIGHT,
                    "hourglas.icn",
                    iCurHourGlassPhase + ENEMY_TURN_PHASE_FRAME_OFFSET,
                    ICON_DRAW_NORMAL,
                    ENEMY_TURN_PHASE_ID,
                    ICON_WIDGET_DRAW,
                    1
                );
            if (!m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                MemError();
            m_adventureWindow->AddWidget(
                m_bottomViewPrimaryWidgets[ENEMY_TURN_PHASE_SLOT + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                ENEMY_TURN_PHASE_Z
            );
        }
    }
    return updated;
}

// donor PoL RVA 0x000613b0; preferred Buka symbol ?UpdBottomViewNewTurn@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.861364;margin=0.145966;shape=0.778;size=0.910;calls=0.840;strings=%s: %d|%s: %d  %s: %d|bigfont.fnt;alternate=pol20:int advManager::UpdBottomViewNewTurn(void)@0x000613b0
VA(0x0042d23d, 0x3e0)
signed char advManager::UpdBottomViewNewTurn(void) {
    int frameIndex;
    int month;
    char* weekStr;
    char* dayStr;

    frameIndex = 0;
    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_NEW_TURN)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_NEW_TURN;
    if (gpGame->m_day == 1 && (gpGame->m_month != 1 || gpGame->m_week != 1 || gpGame->m_day != 1))
        frameIndex = gpGame->m_week;

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_BACKGROUND_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "sunmoon.icn",
        frameIndex,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_FOREGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    weekStr = static_cast<char*>(malloc(BOTTOM_VIEW_TEXT_BUFFER_SIZE));
    sprintf(weekStr, "%s: %d  %s: %d", "Month", gpGame->m_month, "Week", gpGame->m_week);
    m_bottomViewSecondaryWidgets[0] = new textWidget(
        NEW_TURN_DATE_TEXT_X,
        NEW_TURN_WEEK_TEXT_Y,
        NEW_TURN_DATE_TEXT_WIDTH,
        NEW_TURN_WEEK_TEXT_HEIGHT,
        weekStr,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);

    dayStr = static_cast<char*>(malloc(BOTTOM_VIEW_TEXT_BUFFER_SIZE));
    sprintf(dayStr, "%s: %d", "Day", gpGame->m_day);
    m_bottomViewSecondaryWidgets[0] = new textWidget(
        NEW_TURN_DATE_TEXT_X,
        NEW_TURN_DAY_TEXT_Y,
        NEW_TURN_DATE_TEXT_WIDTH,
        NEW_TURN_DAY_TEXT_HEIGHT,
        dayStr,
        "bigfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);
    return 1;
}

// donor PoL RVA 0x00061716; preferred Buka symbol ?UpdBottomViewResMsg@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.790087;margin=0.239399;shape=0.649;size=0.884;calls=0.793;strings=resource.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewResMsg(void)@0x00061716
VA(0x0042d61d, 0x3fa)
signed char advManager::UpdBottomViewResMsg(void) {
    int iconW;
    int iconH;
    int y;
    int lineCnt;
    char* messageText;
    char* countString;
    font* smFont;

    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_RESOURCE)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_RESOURCE;
    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_BACKGROUND_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    y = 0;
    if (giBottomViewResource < 0) {
        y = RESOURCE_VIEW_MULTILINE_HEIGHT;
        smFont = gpResourceManager->GetFont("smalfont.fnt");
        lineCnt = smFont->LineLength(gcBottomViewText, BOTTOM_VIEW_PANEL_WIDTH);
        gpResourceManager->Dispose(smFont);
        y -= lineCnt * RESOURCE_VIEW_LINE_HEIGHT;
    }
    messageText = static_cast<char*>(malloc(strlen(gcBottomViewText) + 1));
    sprintf(messageText, gcBottomViewText);
    m_bottomViewSecondaryWidgets[0] = new textWidget(
        BOTTOM_VIEW_PANEL_X,
        y + RESOURCE_VIEW_TEXT_BASE_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        RESOURCE_VIEW_TEXT_HEIGHT,
        messageText,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);

    if (giBottomViewResource >= 0) {
        if (giBottomViewResource == RESOURCE_GOLD) {
            iconW = RESOURCE_VIEW_GOLD_WIDTH;
            iconH = RESOURCE_VIEW_GOLD_HEIGHT;
        } else {
            iconW = RESOURCE_VIEW_ICON_WIDTH;
            iconH = RESOURCE_VIEW_ICON_HEIGHT;
        }
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
            (BOTTOM_VIEW_PANEL_WIDTH - iconW) / 2 + BOTTOM_VIEW_PANEL_X,
            RESOURCE_VIEW_ICON_BOTTOM - iconH - RESOURCE_VIEW_ICON_BOTTOM_PADDING,
            iconW,
            iconH,
            "resource.icn",
            giBottomViewResource,
            ICON_DRAW_NORMAL,
            BOTTOM_VIEW_FOREGROUND_ID,
            ICON_WIDGET_DRAW,
            1
        );
        if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
            MemError();
        m_adventureWindow->AddWidget(
            m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
            WINDOW_Z_ORDER_APPEND
        );

        countString = static_cast<char*>(malloc(BOTTOM_VIEW_COUNT_BUFFER_SIZE));
        sprintf(countString, "%d", giBottomViewResourceQty);
        m_bottomViewSecondaryWidgets[1] = new textWidget(
            RESOURCE_VIEW_COUNT_X,
            RESOURCE_VIEW_COUNT_Y,
            RESOURCE_VIEW_COUNT_WIDTH,
            RESOURCE_VIEW_COUNT_HEIGHT,
            countString,
            "smalfont.fnt",
            1,
            BOTTOM_VIEW_TEXT_ID_2,
            WIDGET_KIND_TEXT
        );
        if (!m_bottomViewSecondaryWidgets[1])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[1], WINDOW_Z_ORDER_APPEND);
    }
    return 1;
}

// donor PoL RVA 0x00061a75; preferred Buka symbol ?UpdBottomViewKingdom@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.769757;margin=0.069402;shape=0.564;size=0.915;calls=0.850;strings=ressmall.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewKingdom(void)@0x00061a75
VA(0x0042da17, 0x3ce)
signed char advManager::UpdBottomViewKingdom(void) {
    int numVillages;
    int i;
    int nCastles;
    signed char rowY[KINGDOM_VIEW_ENTRY_COUNT];
    unsigned char colX[KINGDOM_VIEW_ENTRY_COUNT];
    char* texts[KINGDOM_VIEW_ENTRY_COUNT];

    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_KINGDOM)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_KINGDOM;
    rowY[RESOURCE_WOOD] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_MERCURY] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_ORE] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_SULFUR] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_CRYSTAL] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[RESOURCE_GEMS] = KINGDOM_VIEW_RESOURCE_TEXT_Y;
    rowY[6] = 28;
    rowY[KINGDOM_VIEW_CASTLE_ENTRY] = KINGDOM_VIEW_TOWN_TEXT_Y;
    rowY[KINGDOM_VIEW_TOWN_ENTRY] = KINGDOM_VIEW_TOWN_TEXT_Y;
    colX[RESOURCE_WOOD] = KINGDOM_VIEW_WOOD_TEXT_X;
    colX[RESOURCE_MERCURY] = KINGDOM_VIEW_MERCURY_TEXT_X;
    colX[RESOURCE_ORE] = KINGDOM_VIEW_ORE_TEXT_X;
    colX[RESOURCE_SULFUR] = KINGDOM_VIEW_SULFUR_TEXT_X;
    colX[RESOURCE_CRYSTAL] = KINGDOM_VIEW_CRYSTAL_TEXT_X;
    colX[RESOURCE_GEMS] = KINGDOM_VIEW_GEMS_TEXT_X;
    colX[RESOURCE_GOLD] = KINGDOM_VIEW_GOLD_TEXT_X;
    colX[KINGDOM_VIEW_CASTLE_ENTRY] = KINGDOM_VIEW_CASTLE_TEXT_X;
    colX[KINGDOM_VIEW_TOWN_ENTRY] = KINGDOM_VIEW_VILLAGE_TEXT_X;
    numVillages = 0;
    nCastles = 0;

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_BACKGROUND_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
        KINGDOM_VIEW_ICON_X,
        KINGDOM_VIEW_ICON_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "ressmall.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_FOREGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (gpGame->m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings
            & (1 << BUILDING_SLOT_CASTLE))
            nCastles++;
        else
            numVillages++;
    }

    for (i = 0; i < KINGDOM_VIEW_ENTRY_COUNT; i++) {
        texts[i] = static_cast<char*>(malloc(BOTTOM_VIEW_COUNT_BUFFER_SIZE));
        if (i < KINGDOM_VIEW_CASTLE_ENTRY)
            sprintf(texts[i], "%d", gpCurPlayer->m_resources[i]);
        else if (i == KINGDOM_VIEW_CASTLE_ENTRY)
            sprintf(texts[i], "%d", nCastles);
        else
            sprintf(texts[i], "%d", numVillages);
        m_bottomViewSecondaryWidgets[i] = new textWidget(
            colX[i] + KINGDOM_VIEW_TEXT_X_BASE,
            rowY[i] + KINGDOM_VIEW_TEXT_Y_BASE,
            KINGDOM_VIEW_TEXT_WIDTH,
            KINGDOM_VIEW_TEXT_HEIGHT,
            texts[i],
            "smalfont.fnt",
            1,
            i + BOTTOM_VIEW_TEXT_ID,
            WIDGET_KIND_TEXT
        );
        if (!m_bottomViewSecondaryWidgets[i])
            MemError();
        m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[i], WINDOW_Z_ORDER_APPEND);
    }
    return 1;
}

// donor PoL RVA 0x00061dd8; preferred Buka symbol ?UpdBottomViewHero@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.635193;margin=0.117414;shape=0.302;size=0.961;calls=0.625;strings=mons32.icn|smalfont.fnt|stonback.icn;alternate=pol20:int advManager::UpdBottomViewHero(void)@0x00061dd8
VA(0x0042dde5, 0x62c)
signed char advManager::UpdBottomViewHero(void) {
    short slotNum;
    signed char creatureType;
    int n;
    int qtyX;
    char* countStr[ARMY_GROUP_SLOT_COUNT];
    short nStacks;
    short iCrest;
    hero* targetHero;
    int y;
    int x;
    char* heroName;

    if (!gbForceUpdate && iCurBottomView == BOTTOM_VIEW_HERO)
        return 0;

    ClearBottomView();
    iCurBottomView = BOTTOM_VIEW_HERO;
    targetHero = gpGame->GetHero(gpCurPlayer->CurrentHero());
    nStacks = 0;

    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND] = new iconWidget(
        BOTTOM_VIEW_PANEL_X,
        BOTTOM_VIEW_PANEL_Y,
        BOTTOM_VIEW_PANEL_WIDTH,
        BOTTOM_VIEW_PANEL_HEIGHT,
        "stonback.icn",
        0,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_BACKGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_BACKGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    iCrest = gpCurPlayer->Color() * HERO_CLASS_COUNT + targetHero->m_heroClass;
    m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND] = new iconWidget(
        495,
        395,
        25,
        25,
        "smcrest.icn",
        iCrest,
        ICON_DRAW_NORMAL,
        BOTTOM_VIEW_FOREGROUND_ID,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND])
        MemError();
    m_adventureWindow->AddWidget(
        m_bottomViewPrimaryWidgets[ADVMGR_BOTTOM_VIEW_FOREGROUND],
        WINDOW_Z_ORDER_APPEND
    );

    heroName = static_cast<char*>(malloc(9));
    strcpy(heroName, targetHero->m_shortName);
    heroName[8] = 0;
    m_bottomViewSecondaryWidgets[0] = new textWidget(
        475,
        418,
        66,
        12,
        heroName,
        "smalfont.fnt",
        1,
        BOTTOM_VIEW_TEXT_ID,
        WIDGET_KIND_TEXT
    );
    if (!m_bottomViewSecondaryWidgets[0])
        MemError();
    m_adventureWindow->AddWidget(m_bottomViewSecondaryWidgets[0], WINDOW_Z_ORDER_APPEND);

    for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
        if (targetHero->m_army.m_creatureTypes[n] != CREATURE_NONE)
            nStacks++;
    }
    if (nStacks) {
        slotNum = 0;
        for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
            creatureType = targetHero->m_army.m_creatureTypes[n];
            if (creatureType != CREATURE_NONE) {
                countStr[slotNum] = static_cast<char*>(malloc(BOTTOM_HERO_LABEL_BYTES));
                sprintf(countStr[slotNum], "%d", targetHero->m_army.m_creatureCounts[n]);
                if (slotNum > 2)
                    y = 3;
                else
                    y = 38;
                if (slotNum == 0) {
                    if (nStacks > 2)
                        x = 101;
                    else
                        x = 77;
                } else if (slotNum == 1) {
                    if (nStacks == 2)
                        x = 28;
                    else
                        x = 52;
                } else if (slotNum == 2) {
                    x = 3;
                } else if (slotNum == 3) {
                    if (nStacks == 4)
                        x = 77;
                    else
                        x = 101;
                } else {
                    x = 52;
                }
                m_bottomViewPrimaryWidgets[slotNum + ADVMGR_BOTTOM_VIEW_ICON_FIRST] =
                    new iconWidget(
                        x + BOTTOM_VIEW_PANEL_X,
                        y + BOTTOM_VIEW_PANEL_Y,
                        BOTTOM_HERO_ICON_WIDTH,
                        BOTTOM_HERO_ICON_HEIGHT,
                        "mons32.icn",
                        creatureType,
                        ICON_DRAW_NORMAL,
                        slotNum + BOTTOM_HERO_FIRST_ICON_ID,
                        ICON_WIDGET_DRAW,
                        1
                    );
                if (!m_bottomViewPrimaryWidgets[slotNum + ADVMGR_BOTTOM_VIEW_ICON_FIRST])
                    MemError();
                if (gMons32Width[creatureType] < 28 && strlen(countStr[slotNum]) <= 2)
                    qtyX = x + 30;
                else
                    qtyX = gMons32Width[creatureType] + x + 2;
                m_bottomViewSecondaryWidgets[slotNum + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST] =
                    new textWidget(
                        qtyX + BOTTOM_VIEW_PANEL_X,
                        y + 414,
                        strlen(countStr[slotNum]) * BOTTOM_HERO_CHARACTER_WIDTH,
                        BOTTOM_HERO_LABEL_HEIGHT,
                        countStr[slotNum],
                        "smalfont.fnt",
                        1,
                        slotNum + BOTTOM_HERO_FIRST_TEXT_ID,
                        WIDGET_KIND_TEXT
                    );
                if (!m_bottomViewSecondaryWidgets[slotNum + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST])
                    MemError();
                m_adventureWindow->AddWidget(
                    m_bottomViewPrimaryWidgets[slotNum + ADVMGR_BOTTOM_VIEW_ICON_FIRST],
                    WINDOW_Z_ORDER_APPEND
                );
                m_adventureWindow->AddWidget(
                    m_bottomViewSecondaryWidgets[slotNum + ADVMGR_BOTTOM_VIEW_HERO_TEXT_FIRST],
                    WINDOW_Z_ORDER_APPEND
                );
                slotNum++;
            }
        }
    }
    return 1;
}

// donor PoL RVA 0x0006235b; preferred Buka symbol ?HeroQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:13;base=0.654225;margin=1.910013;shape=0.309;size=0.949;calls=0.880;strings=mons32.icn|qhero0.bin|qhero1.bin;alternate=pol20:void advManager::HeroQuickView(int, int, int, int)@0x0006235b
VA(0x0042e411, 0xd46)
void advManager::HeroQuickView(
    signed char heroId,
    signed char locatorSlot,
    short windowX,
    short windowY
) {
    short savedOriginX;
    short portraitId;
    short width;
    hero* heroPtr;
    short flagId;
    char* labelText[ARMY_GROUP_SLOT_COUNT];
    short armyW;
    textWidget* sizeTexts[ARMY_GROUP_SLOT_COUNT];
    short j;
    short leftEdge;
    tag_message message;
    short numArmies;
    short creatureY;
    iconWidget* monWidgets[ARMY_GROUP_SLOT_COUNT];
    heroWindow* viewWin;
    short savedOriginY;
    short creatureIconHeight;
    short enable;
    short statWidget;

    armyW = HERO_QUICK_ARMY_AREA_WIDTH;
    leftEdge = ARMY_QUICK_AREA_LEFT;
    creatureY = HERO_QUICK_DETAILED_CREATURE_Y;
    width = ARMY_QUICK_ICON_SIZE;
    creatureIconHeight = ARMY_QUICK_ICON_SIZE;
    enable = 1;
    portraitId = QUICK_VIEW_PORTRAIT;
    statWidget = QUICK_VIEW_STAT_FIRST;
    flagId = QUICK_VIEW_FLAG;
    message.type = MESSAGE_WIDGET;
    if (heroId == INVALID_HERO)
        return;
    heroPtr = gpGame->GetHero(heroId);
    if (heroPtr->m_owner == giCurPlayer || m_identifyHeroActive == 1) {
        if (windowX == QUICK_VIEW_AT_LOCATOR) {
            windowX = HERO_QUICK_DEFAULT_WINDOW_X;
            windowY = locatorSlot * HERO_QUICK_LOCATOR_ROW_HEIGHT + HERO_QUICK_LOCATOR_BASE_Y;
        }
        viewWin = new heroWindow(windowX, windowY, "qhero0.bin");
        if (!viewWin)
            MemError();
        SetWinText(viewWin, WINDOW_TEXT_HERO_QUICK_VIEW);
    } else {
        viewWin = new heroWindow(windowX, windowY, "qhero1.bin");
        if (!viewWin)
            MemError();
    }

    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = QUICK_VIEW_PORTRAIT;
    message.value = heroPtr->m_id;
    viewWin->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = QUICK_VIEW_FLAG;
    message.value = gpGame->m_players[heroPtr->m_owner].Color() * QUICK_VIEW_FLAG_COLOR_STRIDE;
    viewWin->BroadcastMessage(message);
    message.id++;
    message.value++;
    viewWin->BroadcastMessage(message);
    sprintf(gText, "%s", heroPtr->m_name);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = QUICK_VIEW_NAME;
    message.text = gText;
    viewWin->BroadcastMessage(message);

    numArmies = 0;
    for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
        if (heroPtr->m_army.m_creatureTypes[j] != CREATURE_NONE)
            numArmies++;
    }

    if (heroPtr->m_owner == giCurPlayer || m_identifyHeroActive == 1) {
        for (j = 0; j < HERO_PRIMARY_STAT_COUNT; j++) {
            sprintf(gText, "%d", heroPtr->m_primaryStats[j]);
            message.id = j + QUICK_VIEW_STAT_FIRST;
            message.text = gText;
            viewWin->BroadcastMessage(message);
        }
        if (numArmies) {
            signed char monster;
            short curIndex;
            short startPos;

            startPos = (HERO_QUICK_ARMY_AREA_WIDTH - numArmies * ARMY_QUICK_ICON_SIZE) / 2
                       + ARMY_QUICK_AREA_LEFT;
            curIndex = 0;
            for (j = 0; j < numArmies; j++) {
                while (heroPtr->m_army.m_creatureTypes[curIndex] == CREATURE_NONE)
                    curIndex++;
                monster = heroPtr->m_army.m_creatureTypes[curIndex];
                if (monster != CREATURE_NONE) {
                    monWidgets[j] = new iconWidget(
                        j * ARMY_QUICK_ICON_SIZE + startPos,
                        HERO_QUICK_DETAILED_CREATURE_Y,
                        ARMY_QUICK_ICON_SIZE,
                        ARMY_QUICK_ICON_SIZE,
                        "mons32.icn",
                        monster,
                        ICON_DRAW_NORMAL,
                        WIDGET_ID_NONE,
                        ICON_WIDGET_DRAW,
                        1
                    );
                    if (!monWidgets[j])
                        MemError();
                    labelText[j] = static_cast<char*>(malloc(HERO_QUICK_ARMY_LABEL_CAPACITY));
                    sprintf(labelText[j], "%d", heroPtr->m_army.m_creatureCounts[curIndex]);
                    sizeTexts[j] = new textWidget(
                        j * ARMY_QUICK_ICON_SIZE + startPos,
                        HERO_QUICK_DETAILED_LABEL_Y,
                        ARMY_QUICK_ICON_SIZE,
                        ARMY_QUICK_LABEL_HEIGHT,
                        labelText[j],
                        "smalfont.fnt",
                        1,
                        WIDGET_ID_NONE,
                        WIDGET_KIND_TEXT
                    );
                    if (!sizeTexts[j])
                        MemError();
                    viewWin->AddWidget(monWidgets[j], WINDOW_Z_ORDER_APPEND);
                    viewWin->AddWidget(sizeTexts[j], WINDOW_Z_ORDER_APPEND);
                }
                curIndex++;
            }
        }
    } else if (numArmies) {
        short firstRow;
        short slotIndex;
        signed char creatureId;
        short secondRow;
        short offsetX;
        short step;
        short rowY;

        rowY = HERO_QUICK_VAGUE_FIRST_ROW_Y;
        switch (numArmies) {
            case 1:
            case 2:
            case 3:
                rowY += ARMY_QUICK_FIRST_ROW_SHIFT;
                firstRow = numArmies;
                secondRow = 0;
                break;
            case 4:
                firstRow = ARMY_QUICK_FIRST_ROW_COUNT;
                secondRow = 2;
                break;
            default:
                firstRow = ARMY_QUICK_FIRST_ROW_COUNT;
                secondRow = 3;
                break;
        }
        slotIndex = 0;
        step = HERO_QUICK_ARMY_AREA_WIDTH / firstRow;
        offsetX = (step - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
        for (j = 0; j < firstRow; j++) {
            while (heroPtr->m_army.m_creatureTypes[slotIndex] == CREATURE_NONE)
                slotIndex++;
            creatureId = heroPtr->m_army.m_creatureTypes[slotIndex];
            monWidgets[j] = new iconWidget(
                j * step + offsetX,
                rowY,
                ARMY_QUICK_ICON_SIZE,
                ARMY_QUICK_ICON_SIZE,
                "mons32.icn",
                creatureId,
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!monWidgets[j])
                MemError();
            labelText[j] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
            strcpy(
                labelText[j],
                GetArmySizeName(heroPtr->m_army.m_creatureCounts[slotIndex], ARMY_SIZE_NAME_TITLE)
            );
            sizeTexts[j] = new textWidget(
                j * step + ARMY_QUICK_AREA_LEFT,
                rowY + ARMY_QUICK_ICON_BASELINE,
                step,
                ARMY_QUICK_LABEL_HEIGHT,
                labelText[j],
                "smalfont.fnt",
                1,
                WIDGET_ID_NONE,
                WIDGET_KIND_TEXT
            );
            if (!sizeTexts[j])
                MemError();
            viewWin->AddWidget(monWidgets[j], WINDOW_Z_ORDER_APPEND);
            viewWin->AddWidget(sizeTexts[j], WINDOW_Z_ORDER_APPEND);
            slotIndex++;
        }
        if (secondRow) {
            step = HERO_QUICK_ARMY_AREA_WIDTH / secondRow;
            offsetX = (step - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
            rowY += ARMY_QUICK_SECOND_ROW_SHIFT;
            for (j = firstRow; j < firstRow + secondRow; j++) {
                while (heroPtr->m_army.m_creatureTypes[slotIndex] == CREATURE_NONE)
                    slotIndex++;
                creatureId = heroPtr->m_army.m_creatureTypes[slotIndex];
                monWidgets[j] = new iconWidget(
                    (j - ARMY_QUICK_FIRST_ROW_COUNT) * step + offsetX,
                    rowY,
                    ARMY_QUICK_ICON_SIZE,
                    ARMY_QUICK_ICON_SIZE,
                    "mons32.icn",
                    creatureId,
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (!monWidgets[j])
                    MemError();
                labelText[j] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
                strcpy(
                    labelText[j],
                    GetArmySizeName(
                        heroPtr->m_army.m_creatureCounts[slotIndex],
                        ARMY_SIZE_NAME_TITLE
                    )
                );
                sizeTexts[j] = new textWidget(
                    (j - ARMY_QUICK_FIRST_ROW_COUNT) * step + ARMY_QUICK_AREA_LEFT,
                    rowY + ARMY_QUICK_ICON_BASELINE,
                    step,
                    ARMY_QUICK_LABEL_HEIGHT,
                    labelText[j],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    WIDGET_KIND_TEXT
                );
                if (!sizeTexts[j])
                    MemError();
                viewWin->AddWidget(monWidgets[j], WINDOW_Z_ORDER_APPEND);
                viewWin->AddWidget(sizeTexts[j], WINDOW_Z_ORDER_APPEND);
                slotIndex++;
            }
        }
    }

    savedOriginX = m_mapOriginX;
    savedOriginY = m_mapOriginY;
    m_mapOriginX = heroPtr->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = heroPtr->m_y - ADVMGR_VIEW_CENTER;
    UpdateRadar(1, 0);
    GrabScreen();
    gpWindowManager->AddWindow(viewWin, WINDOW_Z_ORDER_APPEND, 1);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(viewWin);
    delete viewWin;
    gpMouseManager->ShowSystemCursor();
    m_mapOriginX = savedOriginX;
    m_mapOriginY = savedOriginY;
    UpdateRadar(1, 0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (message.type == MESSAGE_LEFT_BUTTON_DOWN && heroPtr->m_owner == giCurPlayer)
        SetHeroContext(heroPtr->m_id, 0);
}

// donor PoL RVA 0x0006308d; preferred Buka symbol ?GetArmySizeName@advManager@@QAEPADHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463915;margin=0.489878;shape=0.171;size=0.973;calls=1.000;alternate=pol20:char * advManager::GetArmySizeName(int, int)@0x0006308d
VA(0x0042f157, 0xe2)
char* advManager::GetArmySizeName(
    short armySize,
    H1_ENUM_PARAM(ArmySizeNameVariant, signed char) grammar
) {
    if (giDebugLevel > 0) {
        sprintf(cArmySizeName, "%d", armySize);
        return cArmySizeName;
    }
    if (armySize < static_cast<int>(ARMY_FEW_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_FEW)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_SEVERAL_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_SEVERAL)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_PACK_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_PACK)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_LOTS_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_LOTS)][static_cast<int>(grammar)];
    if (armySize < static_cast<int>(ARMY_HORDE_LIMIT))
        return gArmySizeNames[static_cast<int>(ARMY_SIZE_HORDE)][static_cast<int>(grammar)];
    return gArmySizeNames[static_cast<int>(ARMY_SIZE_ZOUNDS)][static_cast<int>(grammar)];
}

// donor PoL RVA 0x000631ad; preferred Buka symbol ?TownQuickView@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.675336;margin=0.051432;shape=0.330;size=0.961;calls=0.941;strings=mons32.icn|qtown1.bin|smalfont.fnt;alternate=pol20:void advManager::TownQuickView(int, int, int, int)@0x000631ad
VA(0x0042f239, 0xc51)
void advManager::TownQuickView(signed char townId, signed char, short windowX, short windowY) {
    short portraitId;
    short creatureIconHeight;
    short numArmies;
    tag_message message;
    short i;
    short flag;
    short flagId;
    short savedOriginX;
    short width;
    heroWindow* viewWin;
    town* townPointer;
    short savedOriginY;
    int detailLevel;
    short armyW;
    short leftEdge;

    armyW = TOWN_QUICK_ARMY_AREA_WIDTH;
    leftEdge = ARMY_QUICK_AREA_LEFT;
    width = ARMY_QUICK_ICON_SIZE;
    creatureIconHeight = ARMY_QUICK_ICON_SIZE;
    flag = 1;
    portraitId = QUICK_VIEW_PORTRAIT;
    flagId = QUICK_VIEW_FLAG;
    if (townId == GAME_TOWN_NONE)
        return;
    townPointer = gpGame->GetTown(townId);
    if (windowX == QUICK_VIEW_AT_LOCATOR) {
        windowX = TOWN_QUICK_DEFAULT_WINDOW_X;
        windowY = TOWN_QUICK_DEFAULT_WINDOW_Y;
    }
    viewWin = new heroWindow(windowX, windowY, "qtown1.bin");
    if (!viewWin)
        MemError();
    if (townPointer->m_owner == giCurPlayer) {
        detailLevel = TOWN_QUICK_INFORMATION_EXACT;
    } else {
        detailLevel = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (detailLevel > TOWN_QUICK_INFORMATION_THIEVES_LAST)
            detailLevel = TOWN_QUICK_INFORMATION_THIEVES_LAST;
    }
    SetWinText(viewWin, WINDOW_TEXT_TOWN_QUICK_VIEW);

    numArmies = 0;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, QUICK_VIEW_PORTRAIT);
    message.value = townPointer->m_type + TOWN_QUICK_TYPE_FRAME_BASE;
    if (gpGame->GetTown(townId)->m_buildings & (1 << BUILDING_SLOT_CASTLE))
        message.value += TOWN_QUICK_CASTLE_FRAME_OFFSET;
    viewWin->BroadcastMessage(message);
    if (townPointer->m_owner == GAME_PLAYER_NONE) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = QUICK_VIEW_FLAG;
        message.value = WIDGET_FLAG_DRAW;
        viewWin->BroadcastMessage(message);
        message.id++;
        viewWin->BroadcastMessage(message);
    } else {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = QUICK_VIEW_FLAG;
        message.value =
            gpGame->m_players[townPointer->m_owner].Color() * QUICK_VIEW_FLAG_COLOR_STRIDE;
        viewWin->BroadcastMessage(message);
        message.id++;
        message.value++;
        viewWin->BroadcastMessage(message);
    }
    sprintf(gText, GetTownName(townPointer->m_id));
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = QUICK_VIEW_NAME;
    message.text = gText;
    viewWin->BroadcastMessage(message);

    numArmies = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (townPointer->m_army.m_creatureTypes[i] != CREATURE_NONE)
            numArmies++;
    }

    if (!detailLevel || !numArmies) {
        char* garrisonStr;
        textWidget* garrisonWidget;

        garrisonStr = static_cast<char*>(malloc(TOWN_QUICK_EMPTY_LABEL_CAPACITY));
        if (!detailLevel)
            sprintf(garrisonStr, "Unknown");
        else
            sprintf(garrisonStr, "None");
        garrisonWidget = new textWidget(
            TOWN_QUICK_EMPTY_LABEL_X,
            TOWN_QUICK_EMPTY_LABEL_Y,
            TOWN_QUICK_EMPTY_LABEL_WIDTH,
            ARMY_QUICK_LABEL_HEIGHT,
            garrisonStr,
            "smalfont.fnt",
            1,
            WIDGET_ID_NONE,
            WIDGET_KIND_TEXT
        );
        if (!garrisonWidget)
            MemError();
        viewWin->AddWidget(garrisonWidget, WINDOW_Z_ORDER_APPEND);
    } else {
        short slotIndex;
        short rowY;
        int xAdjust;
        short offsetX;
        signed char monster;
        short step;
        iconWidget* iconWgts[ARMY_GROUP_SLOT_COUNT];
        short row2;
        short row1;
        textWidget* texts[ARMY_GROUP_SLOT_COUNT];
        signed char dummy;
        char* labels[ARMY_GROUP_SLOT_COUNT];
        signed char slot;

        rowY = TOWN_QUICK_FIRST_ROW_Y;
        switch (numArmies) {
            case 1:
            case 2:
            case 3:
                rowY += ARMY_QUICK_FIRST_ROW_SHIFT;
                row1 = numArmies;
                row2 = 0;
                break;
            case 4:
                row1 = ARMY_QUICK_FIRST_ROW_COUNT;
                row2 = 2;
                break;
            default:
                row1 = ARMY_QUICK_FIRST_ROW_COUNT;
                row2 = 3;
                break;
        }
        dummy = 0;
        slotIndex = 0;
        slot = 0;
        step = TOWN_QUICK_ARMY_AREA_WIDTH / row1;
        offsetX = (step - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
        xAdjust = 0;
        for (i = 0; i < row1; i++) {
            if (numArmies == ARMY_GROUP_SLOT_COUNT) {
                if (i == 0)
                    xAdjust = ARMY_QUICK_FIVE_STACK_X_SHIFT;
                else
                    xAdjust = -ARMY_QUICK_FIVE_STACK_X_SHIFT;
            }
            while (townPointer->m_army.m_creatureTypes[slot] == CREATURE_NONE)
                slot++;
            monster = townPointer->m_army.m_creatureTypes[slot];
            iconWgts[slotIndex] = new iconWidget(
                step * slotIndex + offsetX + xAdjust,
                rowY,
                ARMY_QUICK_ICON_SIZE,
                ARMY_QUICK_ICON_SIZE,
                "mons32.icn",
                monster,
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconWgts[slotIndex])
                MemError();
            labels[slotIndex] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
            if (detailLevel == TOWN_QUICK_INFORMATION_EXACT)
                sprintf(labels[slotIndex], "%d", townPointer->m_army.m_creatureCounts[slot]);
            else if (detailLevel == TOWN_QUICK_INFORMATION_ESTIMATES)
                strcpy(
                    labels[slotIndex],
                    GetArmySizeName(
                        townPointer->m_army.m_creatureCounts[slot],
                        ARMY_SIZE_NAME_TITLE
                    )
                );
            else
                strcpy(labels[slotIndex], "???");
            texts[slotIndex] = new textWidget(
                step * slotIndex + offsetX + xAdjust - ARMY_QUICK_TEXT_X_ADJUSTMENT,
                rowY + ARMY_QUICK_ICON_BASELINE,
                ARMY_QUICK_TEXT_WIDTH,
                ARMY_QUICK_LABEL_HEIGHT,
                labels[slotIndex],
                "smalfont.fnt",
                1,
                WIDGET_ID_NONE,
                WIDGET_KIND_TEXT
            );
            if (!texts[slotIndex])
                MemError();
            viewWin->AddWidget(iconWgts[slotIndex], WINDOW_Z_ORDER_APPEND);
            viewWin->AddWidget(texts[slotIndex], WINDOW_Z_ORDER_APPEND);
            slotIndex++;
            slot++;
        }
        if (row2) {
            step = TOWN_QUICK_ARMY_AREA_WIDTH / row2;
            offsetX = (step - ARMY_QUICK_ICON_SIZE) / 2 + ARMY_QUICK_AREA_LEFT;
            rowY += ARMY_QUICK_SECOND_ROW_SHIFT;
            for (i = row1; i < row1 + row2; i++) {
                while (townPointer->m_army.m_creatureTypes[slot] == CREATURE_NONE)
                    slot++;
                monster = townPointer->m_army.m_creatureTypes[slot];
                iconWgts[slotIndex] = new iconWidget(
                    (slotIndex - row1) * step + offsetX,
                    rowY,
                    ARMY_QUICK_ICON_SIZE,
                    ARMY_QUICK_ICON_SIZE,
                    "mons32.icn",
                    monster,
                    ICON_DRAW_NORMAL,
                    WIDGET_ID_NONE,
                    ICON_WIDGET_DRAW,
                    1
                );
                if (!iconWgts[slotIndex])
                    MemError();
                labels[slotIndex] = static_cast<char*>(malloc(ARMY_QUICK_SIZE_LABEL_CAPACITY));
                if (detailLevel == TOWN_QUICK_INFORMATION_EXACT)
                    sprintf(labels[slotIndex], "%d", townPointer->m_army.m_creatureCounts[slot]);
                else if (detailLevel == TOWN_QUICK_INFORMATION_ESTIMATES)
                    strcpy(
                        labels[slotIndex],
                        GetArmySizeName(
                            townPointer->m_army.m_creatureCounts[slot],
                            ARMY_SIZE_NAME_TITLE
                        )
                    );
                else
                    strcpy(labels[slotIndex], "???");
                texts[slotIndex] = new textWidget(
                    (slotIndex - row1) * step + offsetX - ARMY_QUICK_TEXT_X_ADJUSTMENT,
                    rowY + ARMY_QUICK_ICON_BASELINE,
                    ARMY_QUICK_TEXT_WIDTH,
                    ARMY_QUICK_LABEL_HEIGHT,
                    labels[slotIndex],
                    "smalfont.fnt",
                    1,
                    WIDGET_ID_NONE,
                    WIDGET_KIND_TEXT
                );
                if (!texts[slotIndex])
                    MemError();
                viewWin->AddWidget(iconWgts[slotIndex], WINDOW_Z_ORDER_APPEND);
                viewWin->AddWidget(texts[slotIndex], WINDOW_Z_ORDER_APPEND);
                slotIndex++;
                slot++;
            }
        }
    }

    GrabScreen();
    gpWindowManager->AddWindow(viewWin, WINDOW_Z_ORDER_APPEND, 1);
    savedOriginX = m_mapOriginX;
    savedOriginY = m_mapOriginY;
    m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    UpdateRadar(1, 0);
    gpMouseManager->HideSystemCursor();
    QuickViewWait();
    gpWindowManager->RemoveWindow(viewWin);
    delete viewWin;
    gpMouseManager->ShowSystemCursor();
    m_mapOriginX = savedOriginX;
    m_mapOriginY = savedOriginY;
    UpdateRadar(1, 0);
    CompleteDraw(0);
    UpdateScreen(0, 0);
    if (message.type == MESSAGE_LEFT_BUTTON_DOWN && townPointer->m_owner == giCurPlayer)
        SetTownContext(townPointer->m_id);
}

// donor PoL RVA 0x00063dd6; preferred Buka symbol ?RedrawAdvScreen@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.562981;margin=0.429826;shape=0.444;size=0.906;calls=0.909;alternate=pol20:void advManager::RedrawAdvScreen(int, int)@0x00063dd6
VA(0x0042fe8a, 0xe8)
void advManager::RedrawAdvScreen(int update) {
    if (!bShowIt)
        return;
    gpResourceManager->GetBackdrop("bord.bmp", gpWindowManager->m_screen);
    SaveAdventureBorder();
    UpdateHeroLocators(0, 0);
    UpdateTownLocators(0, 0);
    UpdBottomView(1, 0, 0);
    m_adventureWindow->DrawWindow(0);
    if (update)
        gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    UpdateRadar(update, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    if (update)
        UpdateScreen(0, 0);
}

// Buka 2.1 DeactivateCurrTown clears the current player's town slot.
VA(0x0042ff72, 0x1f)
void advManager::DeactivateCurrTown(void) {
    gpCurPlayer->m_currentTown = GAME_TOWN_NONE;
}

// Buka 2.1 DeactivateCurrHero demobilizes before clearing the hero slot.
VA(0x0042ff91, 0x27)
void advManager::DeactivateCurrHero(void) {
    DemobilizeCurrHero();
    gpCurPlayer->m_currentHero = INVALID_HERO;
}

VA(0x0042ffb8, 0x59)
void advManager::MobilizeCurrHero(int update) {
    if (gpCurPlayer->m_currentHero == INVALID_HERO)
        return;
    if (m_heroContextLocked)
        return;
    SetHeroContext(gpCurPlayer->m_currentHero, update);
}

// donor PoL RVA 0x00063f95; preferred Buka symbol ?DemobilizeCurrHero@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.450729;margin=0.647673;shape=0.295;size=0.807;calls=0.800;alternate=pol20:void advManager::DemobilizeCurrHero(void)@0x00063f95
VA(0x00430011, 0x199)
void advManager::DemobilizeCurrHero(void) {
    if (gpCurPlayer->m_currentHero == INVALID_HERO)
        return;
    if (!m_heroContextLocked)
        return;

    m_heroContextLocked = 0;
    hero* currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    LogInt(currentHero->m_name, currentHero->m_x);
    StopCursor(1);
    currentHero->m_x = m_cursorMapX + m_mapOriginX;
    currentHero->m_y = m_cursorMapY + m_mapOriginY;
    mapCell* cell = GetCell(currentHero->m_x, currentHero->m_y);
    currentHero->m_locationType = cell->m_triggerType;
    currentHero->m_occupiedTown = cell->m_objectMetadata;
    currentHero->m_direction = m_cursorDirection;
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT)
        currentHero->m_eventFlags |= HERO_EVENT_EMBARKED;
    cell->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO);
    cell->m_objectMetadata = currentHero->m_id;
    cell->m_flags &= ~MAP_CELL_HERO_CURSOR;
    m_cursorActive = 0;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
}

// donor PoL RVA 0x00064101; preferred Buka symbol ?SetTownContext@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.489027;margin=0.082311;shape=0.312;size=0.827;calls=0.923;alternate=pol20:void advManager::SetTownContext(int)@0x00064101
VA(0x004301aa, 0x255)
void advManager::SetTownContext(signed char townId) {
    short k;
    signed char townNo;
    signed char wasVisible;
    town* townPointer;

    DeactivateCurrHero();
    wasVisible = gpMouseManager->IsVis();
    gpMouseManager->ReallyHidePointer();
    gpCurPlayer->m_currentTown = townId;
    townPointer = gpGame->GetTown(gpCurPlayer->m_currentTown);
    m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    townNo = 0;
    for (k = 0; k < gpCurPlayer->m_townCount; k++) {
        if (gpCurPlayer->m_townIds[k] == townId)
            townNo = k;
    }
    if (gpCurPlayer->m_townLocatorPage > townNo)
        gpCurPlayer->m_townLocatorPage = townNo;
    else if (gpCurPlayer->m_townLocatorPage + (LOCATOR_VISIBLE_COUNT - 1) < townNo)
        gpCurPlayer->m_townLocatorPage = townNo - (LOCATOR_VISIBLE_COUNT - 1);
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    HideRoute(0, 0, 1);
    UpdBottomView(1, 1, 1);
    UpdateRadar(1, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    townNo = CELL_TERRAIN(GetCell(townPointer->m_x, townPointer->m_y));
    if (m_currentTerrain != townNo) {
        m_currentTerrain = townNo;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    if (wasVisible)
        gpMouseManager->ReallyShowPointer();
    gpInputManager->m_field_0x34a = 1;
    m_lastHoverCell = 0;
}

// donor PoL RVA 0x00064318; preferred Buka symbol ?SetHeroContext@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.499995;margin=0.151356;shape=0.325;size=0.844;calls=0.947;alternate=pol20:void advManager::SetHeroContext(int, int)@0x00064318
VA(0x004303ff, 0x3e6)
void advManager::SetHeroContext(signed char heroId, signed char update) {
    signed char wasVisible;
    signed char heroSlot;
    short n;
    mapCell* cellPtr;
    hero* currentHero;

    if (heroId == INVALID_HERO)
        return;
    wasVisible = gpMouseManager->IsVis();
    gpMouseManager->ReallyHidePointer();
    DeactivateCurrTown();
    HideRoute(0, 0, 1);
    DeactivateCurrHero();
    m_heroContextLocked = 1;
    gpCurPlayer->m_currentHero = heroId;
    currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    m_mapOriginX = currentHero->m_x - ADVMGR_VIEW_CENTER;
    m_mapOriginY = currentHero->m_y - ADVMGR_VIEW_CENTER;
    m_cursorMapX = m_cursorMapY = ADVMGR_VIEW_CENTER;
    m_previousCursorMapX = m_previousCursorMapY = CURSOR_CELL_NONE;
    if (currentHero->m_eventFlags & HERO_EVENT_EMBARKED)
        m_cursorType = ADVMGR_HERO_ICON_BOAT;
    else
        m_cursorType = currentHero->m_heroClass;
    m_cursorDirection = currentHero->m_direction;
    m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
    cellPtr = GetCell(currentHero->m_x, currentHero->m_y);
    cellPtr->m_flags |= MAP_CELL_HERO_CURSOR;
    gpGame->RestoreCell(
        currentHero->m_x,
        currentHero->m_y,
        currentHero->m_locationType,
        currentHero->m_occupiedTown,
        NULL,
        4
    );
    heroSlot = 0;
    for (n = 0; n < gpCurPlayer->m_heroCount; n++) {
        if (gpCurPlayer->m_heroIds[n] == heroId)
            heroSlot = n;
    }
    if (gpCurPlayer->m_heroLocatorPage > heroSlot)
        gpCurPlayer->m_heroLocatorPage = heroSlot;
    else if (gpCurPlayer->m_heroLocatorPage + (LOCATOR_VISIBLE_COUNT - 1) < heroSlot)
        gpCurPlayer->m_heroLocatorPage = heroSlot - (LOCATOR_VISIBLE_COUNT - 1);
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    if (!update && (m_active == 1 || gbThisNetHumanPlayer[giCurPlayer])) {
        Reseed(0, 0);
        SeedTo(currentHero->m_destinationX, currentHero->m_destinationY);
        ShowRoute(0, 0, !update);
    }
    UpdBottomView(1, 1, 1);
    m_cursorActive = 1;
    UpdateRadar(1, 0);
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    heroSlot = CELL_TERRAIN(cellPtr);
    if (m_currentTerrain != heroSlot) {
        m_currentTerrain = heroSlot;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    if (!gbHeroMoving) {
        if (wasVisible)
            gpMouseManager->ReallyShowPointer();
        gpInputManager->m_field_0x34a = 1;
        m_lastHoverCell = 0;
    }
}

// donor PoL RVA 0x000646aa; preferred Buka symbol ?DoHeroKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.410865;margin=0.380092;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoHeroKnob(void)@0x000646aa
VA(0x004307e5, 0x290)
void advManager::DoHeroKnob(void) {
    double scale;
    short pg;
    tag_message message;
    short numHeroes;
    signed char prevPage;
    short x;
    short my;
    short offset;

    gpMouseManager->SetCursorShape(4);
    prevPage = gpCurPlayer->m_heroLocatorPage;
    numHeroes = gpCurPlayer->m_heroCount;
    scale = 73.0 / (numHeroes - LOCATOR_PAGE_DENOMINATOR_OFFSET);
    gpMouseManager->MouseCoords(x, my);
    offset = my - m_scrollLeftButton->m_y;
    gpInputManager->Flush();
    message = gpInputManager->GetEvent();
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + LOCATOR_SCROLL_BASE_Y)
                message.y = offset + LOCATOR_SCROLL_BASE_Y;
            if (message.y > offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN)
                message.y = offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN;
            gpMouseManager->Main(message);
            m_scrollLeftButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (numHeroes > LOCATOR_VISIBLE_COUNT) {
                pg = static_cast<short>((m_scrollLeftButton->m_y - LOCATOR_SCROLL_BASE_Y) / scale);
                if (prevPage != pg) {
                    gpCurPlayer->m_heroLocatorPage = pg;
                    if (numHeroes - (LOCATOR_VISIBLE_COUNT - 1) < pg)
                        pg = numHeroes - (LOCATOR_VISIBLE_COUNT - 1);
                    UpdateHeroLocators(0, 1);
                    m_scrollLeftButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pg;
                }
            }
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollLeftButton->m_flags &= ~WIDGET_FLAG_SELECTED;
    UpdateHeroLocators(1, 1);
}

// donor PoL RVA 0x000648d9; preferred Buka symbol ?DoTownKnob@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.410865;margin=0.000000;shape=0.246;size=0.742;calls=0.733;alternate=pol20:void advManager::DoTownKnob(void)@0x000648d9
VA(0x00430a75, 0x290)
void advManager::DoTownKnob(void) {
    double scale;
    short pg;
    tag_message message;
    short numHeroes;
    signed char prevPage;
    short x;
    short offset;
    short my;

    gpMouseManager->SetCursorShape(4);
    prevPage = gpCurPlayer->m_townLocatorPage;
    numHeroes = gpCurPlayer->m_townCount;
    scale = 73.0 / (numHeroes - LOCATOR_PAGE_DENOMINATOR_OFFSET);
    gpMouseManager->MouseCoords(x, my);
    offset = my - m_scrollRightButton->m_y;
    gpInputManager->Flush();
    message = gpInputManager->GetEvent();
    while (message.type != MESSAGE_LEFT_BUTTON_UP && message.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (message.type == MESSAGE_MOUSE_MOVE) {
            if (message.y < offset + LOCATOR_SCROLL_BASE_Y)
                message.y = offset + LOCATOR_SCROLL_BASE_Y;
            if (message.y > offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN)
                message.y = offset + LOCATOR_SCROLL_BASE_Y + LOCATOR_HERO_SCROLL_SPAN;
            gpMouseManager->Main(message);
            m_scrollRightButton->m_y = message.y - offset;
            m_adventureWindow->DrawWindow();
            if (numHeroes > LOCATOR_VISIBLE_COUNT) {
                pg = static_cast<short>((m_scrollRightButton->m_y - LOCATOR_SCROLL_BASE_Y) / scale);
                if (prevPage != pg) {
                    gpCurPlayer->m_townLocatorPage = pg;
                    if (numHeroes - (LOCATOR_VISIBLE_COUNT - 1) < pg)
                        pg = numHeroes - (LOCATOR_VISIBLE_COUNT - 1);
                    UpdateTownLocators(0, 1);
                    m_scrollRightButton->m_y = message.y - offset;
                    m_adventureWindow->DrawWindow();
                    prevPage = pg;
                }
            }
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollRightButton->m_flags &= ~WIDGET_FLAG_SELECTED;
    UpdateTownLocators(1, 1);
}

// donor PoL RVA 0x0006a1dd; preferred Buka symbol ?ViewPuzzle@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634399;margin=0.712778;shape=0.353;size=0.798;calls=0.917;strings=advmice.mse|puzzle.icn|viewpuzl.bin;alternate=pol20:void advManager::ViewPuzzle(void)@0x0006a1dd
VA(0x00430d05, 0x3da)
void advManager::ViewPuzzle(void) {
    int puzzleX;
    int puzzleY;
    signed char visibleCount;
    icon* puzzlePieces;
    heroWindow* pWin;
    short j;

    visibleCount = 0;
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_PUZZLE);
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    puzzlePieces = gpResourceManager->GetIcon("puzzle.icn");
    for (j = 0; j < PUZZLE_PIECE_COUNT; j++)
        puzzlePieces->DrawToBuffer(0, 0, j, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gpWindowManager->UpdateScreenRegion(
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_SIZE,
        UPDATE_VIEWPORT_SIZE
    );
    gpWindowManager->SaveFizzleSource(
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_SIZE,
        UPDATE_VIEWPORT_SIZE
    );
    pWin = new heroWindow(PUZZLE_WINDOW_X, PUZZLE_WINDOW_Y, "viewpuzl.bin");
    if (!pWin)
        MemError();
    gpWindowManager->AddWindow(pWin, WINDOW_Z_ORDER_APPEND, 1);

    puzzleX = gpGame->m_ultimateArtifactX - ADVMGR_VIEW_CENTER;
    puzzleY = gpGame->m_ultimateArtifactY - ADVMGR_VIEW_CENTER;
    int biasX = 0;
    int biasY = 0;
    biasX =
        (gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % PUZZLE_ALIGNMENT_DIVISOR - 1;
    biasY = (gpGame->m_ultimateArtifactY * PUZZLE_Y_ADJUST_Y_FACTOR
             + gpGame->m_ultimateArtifactX * PUZZLE_Y_ADJUST_X_FACTOR)
                % PUZZLE_ALIGNMENT_DIVISOR
            - 1;
    if ((gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % PUZZLE_ALIGNMENT_DIVISOR
        == 1) {
        if (biasX > 0)
            biasX++;
        else if (biasX < 0)
            biasX--;
    } else if ((gpGame->m_ultimateArtifactX + gpGame->m_ultimateArtifactY) % PUZZLE_PARITY_DIVISOR
               == 1) {
        if (biasY > 0)
            biasY++;
        else if (biasY < 0)
            biasY--;
    }
    puzzleX += biasX;
    puzzleY += biasY;
    PuzzleDraw(puzzleX, puzzleY, gpGame->m_ultimateArtifactX, gpGame->m_ultimateArtifactY);

    for (j = 0; j < PUZZLE_PIECE_COUNT; j++) {
        if (!BitTest(gpCurPlayer->m_obelisksVisited, j)) {
            puzzlePieces->DrawToBuffer(0, 0, j, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            visibleCount++;
        }
    }
    if (visibleCount != PUZZLE_PIECE_COUNT) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->FizzleForward(
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_SIZE,
            UPDATE_VIEWPORT_SIZE,
            PUZZLE_FIZZLE_TIME
        );
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->ReleaseFizzleSource();
    }

    gpWindowManager->DoDialog(pWin, EventWindowHandler, 0);
    delete pWin;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    UpdateRadar(1, 0);
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
}

// HoMM1 PuzzleDraw redraws the 15x15 cells itself, overlaying the puzzle's
// visible object/overlay frames and marking the target cell.
VA(0x004310df, 0x236)
void advManager::PuzzleDraw(int left, int top, int markX, int markY) {
    int y;
    mapCell* cell;
    int x;
    unsigned char tileset;
    short screenX;
    short screenY;

    for (y = 0; y < ADVMGR_VIEW_CELL_COUNT; y++) {
        for (x = 0; x < ADVMGR_VIEW_CELL_COUNT; x++) {
            DrawCell(x + left, top + y, x, y, ADVMGR_DRAW_GROUND, 1, 0);
            screenX = x * CELL_PIXELS;
            screenY = y * CELL_PIXELS;
            cell = GetCell(left + x, top + y);
            if (!(cell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)
                && cell->m_objectIndex != MAP_CELL_NO_FRAME) {
                tileset = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
                switch (tileset) {
                    case TILESET_OBJ32_04:
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        IconToBitmap(
                            m_objectIcons[tileset],
                            gpWindowManager->m_screen,
                            screenX,
                            screenY,
                            cell->m_objectIndex,
                            ICON_DRAW_OFFSET_FULL
                        );
                        break;
                    default:
                        break;
                }
            }
            if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                tileset = cell->m_overlayTileset & MAP_CELL_TILESET_MASK;
                switch (tileset) {
                    case TILESET_OBJ32_04:
                    case TILESET_MTN32:
                    case TILESET_TREE32:
                        IconToBitmap(
                            m_objectIcons[tileset],
                            gpWindowManager->m_screen,
                            screenX,
                            screenY,
                            cell->m_overlayIndex,
                            ICON_DRAW_OFFSET_FULL
                        );
                        break;
                    default:
                        break;
                }
            }
            if (left + x == markX && top + y == markY)
                IconToBitmap(
                    m_objectIcons[TILESET_ROUTE],
                    gpWindowManager->m_screen,
                    screenX,
                    screenY + ROUTE_DRAW_Y_OFFSET,
                    ROUTE_CELL_DESTINATION - 1,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
    DrawAdventureBorder();
}

// donor PoL RVA 0x00064b08; preferred Buka symbol ?CastSpell@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.339671;margin=1.295843;shape=0.207;size=0.620;calls=0.615;alternate=pol20:void advManager::CastSpell(int)@0x00064b08
VA(0x00431315, 0x1f2)
void advManager::CastSpell(signed char spell) {
    hero* caster;
    int guardianCount;

    if (gpCurPlayer->CurrentHero() != INVALID_HERO)
        caster = gpGame->GetHero(gpCurPlayer->m_currentHero);
    else
        caster = NULL;

    switch (spell) {
        case SPELL_VIEW_MINES:
        case SPELL_VIEW_RESOURCES:
        case SPELL_VIEW_ARTIFACTS:
        case SPELL_VIEW_TOWNS:
        case SPELL_VIEW_HEROES:
        case SPELL_VIEW_ALL:
            ViewWorld(spell, 1, spell == SPELL_VIEW_ALL);
            break;
        case SPELL_IDENTIFY_HERO:
            m_identifyHeroActive = 1;
            NormalDialog(
                "Enemy Heroes are now fully identifiable.",
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                0x91
            );
            break;
        case SPELL_SUMMON_BOAT:
            SummonBoat();
            break;
        case SPELL_DIMENSION_DOOR:
        case SPELL_TOWN_GATE:
            if (caster->m_remainingMobility == 0) {
                NormalDialog(
                    "Your hero is too tired to cast this spell today.  Try again tomorrow.",
                    NORMAL_DIALOG_TYPE_OK
                );
                return;
            }
            if (caster->m_remainingMobility < SPELL_TRAVEL_MOBILITY_COST)
                caster->m_remainingMobility = 0;
            else
                caster->m_remainingMobility -= SPELL_TRAVEL_MOBILITY_COST;
            UpdateHeroLocator(LOCATOR_SLOT_CURRENT_HERO, 1, 1);
            if (spell == SPELL_DIMENSION_DOOR)
                DimensionDoor();
            else
                TownGate();
            break;
        default:
            break;
    }

    if (spell != SPELL_DIMENSION_DOOR && spell != SPELL_TOWN_GATE)
        gpGame->GetHero(gpCurPlayer->m_currentHero)->UseSpell(spell);
}

// Buka 2.1 advManager::ViewWorld (SOURCE/Viewwrld).
// HoMM1's adventure ViewWorld lives in ADVMGR (ground6/flag6/spheres icons);
// CastSpell, AdvPanel, Main and the menu handler pass three signed bytes.
VA(0x00431507, 0x1127)
void advManager::ViewWorld(
    signed char spellType,
    signed char drawAllObjects,
    signed char drawAllTerrains
) {
    icon* flags;
    hero* curHero;
    signed char ts;
    signed char flip;
    icon* letters;
    short index;
    heroWindow* win;
    unsigned short mask;
    short x;
    short owner;
    mapCell* cell;
    icon* tilesets[VIEW_WORLD_TILESET_COUNT];
    short i;
    short y;
    short screenX;
    icon* spheres;
    short screenY;
    icon* ground;

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    mask = (1 << TILESET_MTN32) | (1 << TILESET_TREE32);
    if (spellType == SPELL_VIEW_TOWNS || spellType == SPELL_VIEW_ALL)
        mask |= 1 << TILESET_TOWN32;
    ground = gpResourceManager->GetIcon("ground6.icn");
    flags = gpResourceManager->GetIcon("flag6.icn");
    spheres = gpResourceManager->GetIcon("spheres.icn");
    letters = gpResourceManager->GetIcon("letters.icn");
    curHero = NULL;
    for (i = 0; i < VIEW_WORLD_TILESET_COUNT; i++)
        tilesets[i] = NULL;
    tilesets[TILESET_TREE32] = gpResourceManager->GetIcon("tree6.icn");
    tilesets[TILESET_MTN32] = gpResourceManager->GetIcon("mtn6.icn");
    tilesets[TILESET_TOWN32] = gpResourceManager->GetIcon("town6.icn");
    if (gpCurPlayer->CurrentHero() != INVALID_HERO)
        curHero = &gpGame->m_heroRecs[gpCurPlayer->CurrentHero()];
    FillBitmapArea(
        gpWindowManager->m_screen,
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_SIZE,
        UPDATE_VIEWPORT_SIZE,
        0
    );

    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gpGame->m_mapExtra[x][y] & giCurPlayerBit) || drawAllTerrains
                || (spellType == SPELL_VIEW_TOWNS
                    && (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TOWN)) {
                flip = 0;
                screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                screenY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                index = cell->m_tileIndex >> VIEW_WORLD_GROUND_TILE_SHIFT;
                if (cell->m_flags & MAP_CELL_GROUND_FLIP_HORIZONTAL)
                    flip = 1;
                if (cell->m_flags & MAP_CELL_GROUND_FLIP_VERTICAL)
                    index += VIEW_WORLD_GROUND_FLIPPED_FRAMES;
                ground->DrawToBuffer(
                    (flip == 1 ? VIEW_WORLD_CELL_PIXELS - 1 : 0) + screenX,
                    screenY,
                    index,
                    flip,
                    ICON_DRAW_OFFSET_FULL
                );
                if (cell->m_objectIndex != MAP_CELL_NO_FRAME) {
                    ts = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
                    if (mask & (1 << ts))
                        tilesets[ts]->DrawToBuffer(
                            screenX,
                            screenY,
                            cell->m_objectIndex,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            }
        }
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
            screenY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
            if ((drawAllObjects || (gpGame->m_mapExtra[x][y] & giCurPlayerBit))
                && (cell->m_triggerType & MAP_TRIGGER_EVENT)) {
                switch (spellType) {
                    case SPELL_VIEW_ALL:
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT))
                            flags->DrawToBuffer(
                                screenX,
                                screenY,
                                VIEW_WORLD_FLAG_ARTIFACT,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    screenY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        } else if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                                   && gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                          == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[gpGame->m_heroRecs[cell->m_objectMetadata]
                                                             .m_occupiedTown];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    screenY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                            case MAP_OBJECT_ALCHEMIST_LAB:
                            case MAP_OBJECT_MINE:
                            case MAP_OBJECT_SAWMILL:
                                owner = gpGame->m_mineOwners[cell->m_objectMetadata];
                                if (owner >= 0)
                                    index = gpGame->m_players[owner].m_color;
                                else
                                    index = PLAYER_COLOR_NEUTRAL;
                                spheres->DrawToBuffer(
                                    screenX,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                letters->DrawToBuffer(
                                    screenX,
                                    screenY,
                                    gpGame->m_mines[cell->m_objectMetadata].type,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                break;
                            case MAP_OBJECT_HERO:
                                switch (gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                        & MAP_TRIGGER_TYPE_MASK) {
                                    case MAP_OBJECT_ALCHEMIST_LAB:
                                    case MAP_OBJECT_MINE:
                                    case MAP_OBJECT_SAWMILL:
                                        owner = gpGame->m_mineOwners
                                                    [gpGame->m_heroRecs[cell->m_objectMetadata]
                                                         .m_occupiedTown];
                                        if (owner >= 0)
                                            index = gpGame->m_players[owner].m_color;
                                        else
                                            index = PLAYER_COLOR_NEUTRAL;
                                        spheres->DrawToBuffer(
                                            screenX,
                                            screenY,
                                            index,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        letters->DrawToBuffer(
                                            screenX,
                                            screenY,
                                            gpGame->m_mines[cell->m_objectMetadata].type,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        break;
                                    default:
                                        break;
                                }
                        }
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                            owner = gpGame->m_availableHeroes[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    case SPELL_VIEW_MINES:
                        switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                            case MAP_OBJECT_ALCHEMIST_LAB:
                            case MAP_OBJECT_MINE:
                            case MAP_OBJECT_SAWMILL:
                                owner = gpGame->m_mineOwners[cell->m_objectMetadata];
                                if (owner >= 0)
                                    index = gpGame->m_players[owner].m_color;
                                else
                                    index = PLAYER_COLOR_NEUTRAL;
                                spheres->DrawToBuffer(
                                    screenX,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                letters->DrawToBuffer(
                                    screenX,
                                    screenY,
                                    gpGame->m_mines[cell->m_objectMetadata].type,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                break;
                            case MAP_OBJECT_HERO:
                                switch (gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                        & MAP_TRIGGER_TYPE_MASK) {
                                    case MAP_OBJECT_ALCHEMIST_LAB:
                                    case MAP_OBJECT_MINE:
                                    case MAP_OBJECT_SAWMILL:
                                        owner = gpGame->m_mineOwners
                                                    [gpGame->m_heroRecs[cell->m_objectMetadata]
                                                         .m_occupiedTown];
                                        if (owner >= 0)
                                            index = gpGame->m_players[owner].m_color;
                                        else
                                            index = PLAYER_COLOR_NEUTRAL;
                                        spheres->DrawToBuffer(
                                            screenX,
                                            screenY,
                                            index,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        letters->DrawToBuffer(
                                            screenX,
                                            screenY,
                                            gpGame->m_mines[cell->m_objectMetadata].type,
                                            ICON_DRAW_NORMAL,
                                            ICON_DRAW_OFFSET_FULL
                                        );
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case SPELL_VIEW_RESOURCES:
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_RESOURCE)) {
                            spheres->DrawToBuffer(
                                screenX - VIEW_WORLD_RESOURCE_X_SHIFT,
                                screenY,
                                PLAYER_COLOR_NEUTRAL,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                            letters->DrawToBuffer(
                                screenX - VIEW_WORLD_RESOURCE_X_SHIFT,
                                screenY,
                                cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        }
                        break;
                    case SPELL_VIEW_ARTIFACTS:
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT))
                            flags->DrawToBuffer(
                                screenX,
                                screenY,
                                VIEW_WORLD_FLAG_ARTIFACT,
                                ICON_DRAW_NORMAL,
                                ICON_DRAW_OFFSET_FULL
                            );
                        break;
                    case SPELL_VIEW_TOWNS:
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    screenY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        } else if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                                   && gpGame->m_heroRecs[cell->m_objectMetadata].m_locationType
                                          == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                            owner = gpGame->m_townOwners[gpGame->m_heroRecs[cell->m_objectMetadata]
                                                             .m_occupiedTown];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX - VIEW_WORLD_TOWN_FLAG_LEFT,
                                    screenY,
                                    index,
                                    ICON_DRAW_FLIPPED,
                                    ICON_DRAW_OFFSET_FULL
                                );
                                flags->DrawToBuffer(
                                    screenX + VIEW_WORLD_TOWN_FLAG_RIGHT,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    case SPELL_VIEW_HEROES:
                        if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                            owner = gpGame->m_availableHeroes[cell->m_objectMetadata];
                            if (owner >= 0) {
                                index = gpGame->m_players[owner].m_color;
                                flags->DrawToBuffer(
                                    screenX,
                                    screenY,
                                    index,
                                    ICON_DRAW_NORMAL,
                                    ICON_DRAW_OFFSET_FULL
                                );
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            if (curHero && curHero->m_x == x && curHero->m_y == y)
                flags->DrawToBuffer(
                    screenX,
                    screenY,
                    VIEW_WORLD_FLAG_CURRENT_HERO,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
        for (x = MAP_CELL_GRID_SIZE - 1; x >= 0; x--) {
            cell = GetCell(x, y);
            if ((gpGame->m_mapExtra[x][y] & giCurPlayerBit) || drawAllTerrains
                || (cell->m_triggerType == MAP_OBJECT_TOWN && spellType == SPELL_VIEW_TOWNS)) {
                screenX = x * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                screenY = y * VIEW_WORLD_CELL_PIXELS + VIEW_WORLD_ORIGIN;
                if (cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                    ts = cell->m_overlayTileset & MAP_CELL_TILESET_MASK;
                    if (mask & (1 << ts))
                        tilesets[ts]->DrawToBuffer(
                            screenX,
                            screenY,
                            cell->m_overlayIndex,
                            ICON_DRAW_NORMAL,
                            ICON_DRAW_OFFSET_FULL
                        );
                }
            }
        }
    }

    gpWindowManager->UpdateScreenRegion(
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_ORIGIN,
        UPDATE_VIEWPORT_SIZE,
        UPDATE_VIEWPORT_SIZE
    );
    sprintf(gText, "view-%02d.bin", spellType - SPELL_VIEW_MINES);
    win = new heroWindow(WORLD_WINDOW_X, WORLD_WINDOW_Y, gText);
    if (!win)
        MemError();
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    UpdateRadar(1, 0);
    for (i = 0; i < VIEW_WORLD_TILESET_COUNT; i++) {
        if (tilesets[i])
            gpResourceManager->Dispose(tilesets[i]);
    }
    gpResourceManager->Dispose(ground);
    gpResourceManager->Dispose(flags);
    gpResourceManager->Dispose(spheres);
    gpResourceManager->Dispose(letters);
    RedrawAdvScreen(1);
}

// HoMM1-only helper: refresh the saved screen copy with the pointer hidden.
VA(0x0043262e, 0x41)
void advManager::GrabScreen(void) {
    gpMouseManager->ReallyHidePointer();
    GrabScreenBitmap(gpWindowManager->m_screen, 0, 0);
    gpMouseManager->ReallyShowPointer();
}

H1_ENUM_CONST_BEGIN(ControlPanelDialogConstant)
    CONTROL_NEW_GAME = 1,
    CONTROL_LOAD_GAME = 2,
    CONTROL_SAVE_GAME = 3,
    CONTROL_QUIT = 4,
    CONTROL_MUSIC_VOLUME = 5,
    CONTROL_SOUND_VOLUME = 6,
    CONTROL_WALK_SPEED = 7,
    CONTROL_MUSIC_SOURCE = 11,
    CONTROL_SHOW_ROUTE = 12,
    CONTROL_SHOW_ENEMY_MOVES = 13,
    CONTROL_SCENARIO_INFO = 17,
    CONTROL_MUSIC_VOLUME_TEXT = 8,
    CONTROL_SOUND_VOLUME_TEXT = 9,
    CONTROL_WALK_SPEED_TEXT = 10,
    CONTROL_MUSIC_SOURCE_TEXT = 14,
    CONTROL_SHOW_ROUTE_TEXT = 15,
    CONTROL_SHOW_ENEMY_MOVES_TEXT = 16
H1_ENUM_CONST_END(ControlPanelDialogConstant)

// UpdateCPanel's button frames: off/on pairs for music and sound, then one
// frame per walk speed, show-route and enemy-moves state and music source
// (the setting's value added to the _FIRST frame).
H1_ENUM_CONST_BEGIN(ControlPanelFrame)
    CPANEL_FRAME_MUSIC_OFF = 10,
    CPANEL_FRAME_MUSIC_ON = 11,
    CPANEL_FRAME_SOUND_OFF = 12,
    CPANEL_FRAME_SOUND_ON = 13,
    CPANEL_FRAME_WALK_SPEED_FIRST = 14,
    CPANEL_FRAME_SHOW_ROUTE_FIRST = 21,
    CPANEL_FRAME_ENEMY_MOVES_FIRST = 23,
    CPANEL_FRAME_MUSIC_SOURCE_FIRST = 27
H1_ENUM_CONST_END(ControlPanelFrame)

// CPanelHandler's right-click help: the gCPanelHelp row for each control.
H1_ENUM_BEGIN(ControlPanelHelp)
    CPANEL_HELP_NONE = -1,
    CPANEL_HELP_NEW_GAME = 0,
    CPANEL_HELP_LOAD_GAME = 1,
    CPANEL_HELP_QUIT = 2,
    CPANEL_HELP_CLOSE = 3,
    CPANEL_HELP_SAVE_GAME = 4,
    CPANEL_HELP_MUSIC_VOLUME = 5,
    CPANEL_HELP_SOUND_VOLUME = 6,
    CPANEL_HELP_WALK_SPEED = 7,
    CPANEL_HELP_MUSIC_SOURCE = 8,
    CPANEL_HELP_SHOW_ROUTE = 9,
    CPANEL_HELP_SHOW_ENEMY_MOVES = 10,
    CPANEL_HELP_SCENARIO_INFO = 11
H1_ENUM_END(ControlPanelHelp)

// HoMM1 merges Buka's ControlPanel and SystemOptions: one cpanel.bin dialog
// that also applies the walk-speed sample set and saves changed preferences.
VA(0x0043266f, 0x321)
short advManager::ControlPanel(void) {
    tag_message message;
    int mobilized;
    signed char oldSpeed;
    int gameCommand;
    int n;

    TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gameCommand = MAIN_MENU_NO_COMMAND;
    oldSpeed = gConfig.walkSpeed;
    bFreshSave = 0;
    mobilized = m_heroContextLocked;
    bPrefsChanged = 0;
    DemobilizeCurrHero();
    cPanel = new heroWindow(160, 10, "cpanel.bin");
    if (cPanel == NULL)
        MemError();
    SetWinText(cPanel, WINDOW_TEXT_CONTROL_PANEL);
    if (gbRemoteOn) {
        message.type = MESSAGE_WIDGET;
        message.id = CONTROL_NEW_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_COMMAND_DIMMED;
        cPanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        cPanel->BroadcastMessage(message);
        message.id = CONTROL_LOAD_GAME;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_COMMAND_DIMMED;
        cPanel->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        cPanel->BroadcastMessage(message);
    }
    UpdateCPanel(1);
    gpWindowManager->DoDialog(cPanel, CPanelHandler, 0);
    delete cPanel;
    switch (gpWindowManager->m_dialogResult) {
        case CONTROL_NEW_GAME:
        case CONTROL_LOAD_GAME:
        case CONTROL_QUIT:
            gameCommand = gpWindowManager->m_dialogResult;
            break;
        case CONTROL_SCENARIO_INFO:
            if (gpGame->m_campaignType > 0)
                gpGame->ShowCampaignInfo(gpGame->m_campaignScenario, 1, 0);
            else
                gpGame->ShowScenInfo();
            break;
        case CONTROL_SAVE_GAME:
            SaveGame();
            break;
    }
    if (oldSpeed != gConfig.walkSpeed) {
        for (n = 0; n < ADVMGR_CURSOR_SAMPLE_COUNT; n++)
            gpResourceManager->Dispose(m_cursorSamples[n]);
        GetCursorSampleSet(gConfig.walkSpeed);
    }
    if (bPrefsChanged)
        WritePrefs();
    if (mobilized)
        MobilizeCurrHero(0);
    if (gameCommand != MAIN_MENU_NO_COMMAND) {
        gGameCommand = gameCommand;
        return 1;
    }
    return 0;
}

// Buka 2.1 UpdateSystemOptions over HoMM1's six control-panel options.
VA(0x00432990, 0x227)
void UpdateCPanel(signed char initialDraw) {
    tag_message message;

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, CONTROL_MUSIC_VOLUME);
    message.value = gConfig.musicVolume ? CPANEL_FRAME_MUSIC_ON : CPANEL_FRAME_MUSIC_OFF;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME;
    message.value = gConfig.soundVolume ? CPANEL_FRAME_SOUND_ON : CPANEL_FRAME_SOUND_OFF;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED;
    message.value = gConfig.walkSpeed + CPANEL_FRAME_WALK_SPEED_FIRST;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE;
    message.value = gConfig.musicSource + CPANEL_FRAME_MUSIC_SOURCE_FIRST;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE;
    message.value = gConfig.showRoute + CPANEL_FRAME_SHOW_ROUTE_FIRST;
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES;
    message.value = gbRemoteOn ? CPANEL_FRAME_ENEMY_MOVES_FIRST
                               : 1 - gConfig.blackoutComputer + CPANEL_FRAME_ENEMY_MOVES_FIRST;
    cPanel->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = CONTROL_MUSIC_VOLUME_TEXT;
    message.text = onOffText[gConfig.musicVolume];
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SOUND_VOLUME_TEXT;
    message.text = onOffText[gConfig.soundVolume];
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_WALK_SPEED_TEXT;
    message.text = walkSpeedText[gConfig.walkSpeed];
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_MUSIC_SOURCE_TEXT;
    message.text = musicQualityText[gConfig.musicSource];
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ROUTE_TEXT;
    message.text = onOffText[gConfig.showRoute];
    cPanel->BroadcastMessage(message);
    message.id = CONTROL_SHOW_ENEMY_MOVES_TEXT;
    message.text = onOffText[1 - gConfig.blackoutComputer];
    cPanel->BroadcastMessage(message);
    if (!initialDraw)
        cPanel->MoveWindow(0, 0);
}

VA(0x00432bb7, 0x232)
signed char SaveGame(void) {
    short iResult;
    fileRequester* fileReq;
    char searchMask[16];
    signed char success;
    int humans;
    int plIdx;
    char extension[8];

    success = 0;
    humans = 0;
    gpAdvManager->DisableButtons();
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    for (plIdx = 0; plIdx < GAME_PLAYER_COUNT; plIdx++)
        if (!gpGame->m_playerDead[plIdx] && gbHumanPlayer[plIdx])
            humans++;
    if (giCampaignChoice > 0) {
        sprintf(extension, ".CGM");
        sprintf(searchMask, "*.CGM");
    } else {
        sprintf(extension, ".GM%d", humans);
        sprintf(searchMask, "*.GM*");
    }
    extern char gcGamePath[];
    fileReq = new fileRequester(0xa0, 0x28, FILE_REQUESTER_SAVE, searchMask, gcGamePath, extension);
    if (!fileReq)
        MemError();
    iResult = gpExec->DoDialog(fileReq);
    if (iResult == DIALOG_BUTTON_2) {
        success = 1;
        bFreshSave = 1;
        success = gpGame->SaveGame(gLastFilename, 0);
        if (success)
            NormalDialog("Game saved successfully.", NORMAL_DIALOG_TYPE_OK, 0xb1);
    }
    delete fileReq;
    gpAdvManager->EnableButtons();
    return success;
}

// Buka 2.1 CPanelHandler plus SystemOptionsHandler's option cycling.
VA(0x00432de9, 0x54b)
short CPanelHandler(struct tag_message& message) {
    signed char changed = 0;
    char question[120];
    signed char handled = 0;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                int helpIndex = CPANEL_HELP_NONE;
                switch (message.id) {
                    case CONTROL_NEW_GAME:
                        helpIndex = CPANEL_HELP_NEW_GAME;
                        break;
                    case CONTROL_LOAD_GAME:
                        helpIndex = CPANEL_HELP_LOAD_GAME;
                        break;
                    case CONTROL_QUIT:
                        helpIndex = CPANEL_HELP_QUIT;
                        break;
                    case PANEL_CLOSE_WIDGET:
                        helpIndex = CPANEL_HELP_CLOSE;
                        break;
                    case CONTROL_SAVE_GAME:
                        helpIndex = CPANEL_HELP_SAVE_GAME;
                        break;
                    case CONTROL_MUSIC_VOLUME:
                        helpIndex = CPANEL_HELP_MUSIC_VOLUME;
                        break;
                    case CONTROL_SOUND_VOLUME:
                        helpIndex = CPANEL_HELP_SOUND_VOLUME;
                        break;
                    case CONTROL_WALK_SPEED:
                        helpIndex = CPANEL_HELP_WALK_SPEED;
                        break;
                    case CONTROL_MUSIC_SOURCE:
                        helpIndex = CPANEL_HELP_MUSIC_SOURCE;
                        break;
                    case CONTROL_SHOW_ROUTE:
                        helpIndex = CPANEL_HELP_SHOW_ROUTE;
                        break;
                    case CONTROL_SHOW_ENEMY_MOVES:
                        helpIndex = CPANEL_HELP_SHOW_ENEMY_MOVES;
                        break;
                    case CONTROL_SCENARIO_INFO:
                        helpIndex = CPANEL_HELP_SCENARIO_INFO;
                        break;
                }
                if (helpIndex >= 0)
                    NormalDialog(gCPanelHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case CONTROL_NEW_GAME:
                            strcpy(
                                question,
                                "Are you sure you want to restart?  (Your current game will be "
                                "lost)"
                            );
                            goto confirm_reset;
                        case CONTROL_LOAD_GAME:
                            strcpy(
                                question,
                                "Are you sure you want to load a new game?  (Your current game "
                                "will be lost)"
                            );
                            goto confirm_reset;
                        case CONTROL_QUIT:
                            strcpy(question, "Are you sure you want to quit?");
                        confirm_reset:
                            handled = 1;
                            if (!bFreshSave) {
                                NormalDialog(question, NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x50);
                                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                                    handled = 0;
                            }
                            break;
                        case CONTROL_SAVE_GAME:
                            handled = 1;
                            break;
                        case CONTROL_SCENARIO_INFO:
                        case PANEL_CLOSE_WIDGET:
                            handled = 1;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case CONTROL_MUSIC_VOLUME:
                            gConfig.musicVolume =
                                (gConfig.musicVolume + 1) % (SOUND_VOLUME_LAST + 1);
                            gpSoundManager->AdjustMusicVolumes();
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SOUND_VOLUME:
                            gConfig.soundVolume =
                                (gConfig.soundVolume + 1) % (SOUND_VOLUME_LAST + 1);
                            gpSoundManager->AdjustSoundVolumes();
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_WALK_SPEED:
                            ++gConfig.walkSpeed;
                            gConfig.walkSpeed %= WALK_SPEED_COUNT;
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_MUSIC_SOURCE:
                            if (gConfig.musicSource == SOUND_MUSIC_SOURCE_CD) {
                                gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
                            } else {
                                if (gpSoundManager->m_cdStarted == 0) {
                                    NormalDialog(
                                        "Unable to set up CD stereo music.  Your CD player might "
                                        "be in use by another "
                                        "program, or your sound driver might not support CD "
                                        "stereo.",
                                        NORMAL_DIALOG_TYPE_OK
                                    );
                                    break;
                                }
                                gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
                            }
                            gpSoundManager->SetMusicQuality(gConfig.musicSource);
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SHOW_ROUTE:
                            gConfig.showRoute = 1 - gConfig.showRoute;
                            changed = 1;
                            bPrefsChanged = 1;
                            break;
                        case CONTROL_SHOW_ENEMY_MOVES:
                            if (!gbRemoteOn) {
                                gConfig.blackoutComputer = 1 - gConfig.blackoutComputer;
                                changed = 1;
                                bPrefsChanged = 1;
                            }
                            break;
                    }
                    break;
            }
        }
    }
    if (changed)
        UpdateCPanel(0);
    if (handled) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x000650eb; preferred Buka symbol ?CheckCastSpell@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.587490;margin=0.208808;shape=0.255;size=0.813;calls=0.857;strings=advmice.mse;alternate=pol20:void advManager::CheckCastSpell(void)@0x000650eb
VA(0x00433334, 0xab)
void advManager::CheckCastSpell(void) {
    if (gpCurPlayer->CurrentHero() != INVALID_HERO) {
        MobilizeCurrHero(0);
        CompleteDraw(0);
        UpdateScreen(0, 0);
        GrabScreen();
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        CastSpell(gpGame->ViewSpells(
            gpGame->GetHero(gpCurPlayer->m_currentHero),
            SPELL_TYPE_ADVENTURE,
            NullHandler,
            0
        ));
    }
}

// donor PoL RVA 0x0006a724; preferred Buka symbol ?AdvPanel@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.644374;margin=0.446733;shape=0.427;size=0.786;calls=0.720;strings=advmice.mse|apanel.bin;alternate=pol20:void advManager::AdvPanel(void)@0x0006a724
VA(0x004333df, 0x213)
void advManager::AdvPanel(void) {
    heroWindow* adventurePanel;
    struct tag_message message;
    int mobilized;

    TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    mobilized = m_heroContextLocked;
    DemobilizeCurrHero();

    adventurePanel = new heroWindow(160, 40, "apanel.bin");
    if (adventurePanel == NULL)
        MemError();
    if (gpCurPlayer->CurrentHero() == INVALID_HERO) {
        message.type = MESSAGE_WIDGET;
        message.id = PANEL_SEARCH;
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        adventurePanel->BroadcastMessage(message);
        message.id = PANEL_CAST_SPELL;
        adventurePanel->BroadcastMessage(message);
        message.id = PANEL_SEARCH;
        message.command = WIDGET_COMMAND_SET_FLAGS;
        message.value = WIDGET_COMMAND_DIMMED;
        adventurePanel->BroadcastMessage(message);
        message.id = PANEL_CAST_SPELL;
        adventurePanel->BroadcastMessage(message);
    }

    gpWindowManager->DoDialog(adventurePanel, APanelHandler, 0);
    delete adventurePanel;
    switch (gpWindowManager->m_dialogResult) {
        case PANEL_CAST_SPELL:
            CheckCastSpell();
            break;
        case PANEL_SEARCH:
            ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
            break;
        case PANEL_VIEW_WORLD:
            ViewWorld(SPELL_VIEW_ALL, 0, 0);
            break;
        case PANEL_VIEW_PUZZLE:
            ViewPuzzle();
            break;
    }

    if (mobilized)
        MobilizeCurrHero(0);
}

// Buka 2.1 APanelHandler; HoMM1 shares the search help text with Close and
// chains the dialog-select stores.
VA(0x004335f2, 0x1d3)
short APanelHandler(struct tag_message& message) {
    signed char handled = 0;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                int helpIndex = PANEL_NO_HELP;
                switch (message.id) {
                    case PANEL_VIEW_WORLD:
                        helpIndex = PANEL_VIEW_WORLD_HELP;
                        break;
                    case PANEL_VIEW_PUZZLE:
                        helpIndex = PANEL_VIEW_PUZZLE_HELP;
                        break;
                    case PANEL_CAST_SPELL:
                        helpIndex = PANEL_CAST_SPELL_HELP;
                        break;
                    case PANEL_SEARCH:
                        helpIndex = PANEL_SEARCH_HELP;
                        break;
                    case PANEL_CLOSE_WIDGET:
                        helpIndex = PANEL_SEARCH_HELP;
                        break;
                }
                if (helpIndex >= 0)
                    NormalDialog(gAPanelHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case PANEL_VIEW_WORLD:
                        case PANEL_VIEW_PUZZLE:
                        case PANEL_CAST_SPELL:
                        case PANEL_SEARCH:
                        case PANEL_CLOSE_WIDGET:
                            handled = 1;
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    if (handled) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// DimensionDoor's dimdoor.bin dialog (Buka 2.1 AdventureTravelSpellConstant
// names): hovering the map view (FIRST_BUTTON) sets m_dialogResult to
// ACCEPT over a free cell, else REJECT, as does the other area (LAST_BUTTON);
// a click with ACCEPT closes the dialog. TownGate starts its nearest-town
// search at DISTANCE_LIMIT.
H1_ENUM_CONST_BEGIN(AdventureTravelSpellConstant)
    TRAVEL_DIALOG_REJECT = 0,
    TRAVEL_DIALOG_ACCEPT = 1,
    DIMENSION_DOOR_FIRST_BUTTON = ADVENTURE_CONTROL_MAP_VIEW,
    DIMENSION_DOOR_LAST_BUTTON = 11,
    TOWN_PORTAL_DISTANCE_LIMIT = 1000
H1_ENUM_CONST_END(AdventureTravelSpellConstant)

VA(0x004337c5, 0x34b)
short DimensionDoorHandler(struct tag_message& message) {
    signed char result;
    short mouseX;
    short mouseY;
    mapCell* cell;

    if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT]) {
        gpAdvManager->CompleteDraw(gpAdvManager->m_mapOriginX, gpAdvManager->m_mapOriginY, 0);
        gpAdvManager->UpdateScreen(0, 0);
    }
    result = 0;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case DIMENSION_DOOR_FIRST_BUTTON:
                        case DIMENSION_DOOR_LAST_BUTTON:
                            if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
                            } else if (gpWindowManager->m_dialogResult == TRAVEL_DIALOG_ACCEPT) {
                                result = 1;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case WIDGET_COMMAND_HOVER:
                    switch (message.id) {
                        case DIMENSION_DOOR_LAST_BUTTON:
                            gpWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                            break;
                        case ADVENTURE_CONTROL_MAP_VIEW:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            mouseX /= CELL_PIXELS;
                            mouseY /= CELL_PIXELS;
                            if (mouseX < 0)
                                mouseX = 0;
                            if (mouseY < 0)
                                mouseY = 0;
                            if (mouseX > ADVMGR_VIEW_CELL_COUNT - 1)
                                mouseX = ADVMGR_VIEW_CELL_COUNT - 1;
                            if (mouseY > ADVMGR_VIEW_CELL_COUNT - 1)
                                mouseY = ADVMGR_VIEW_CELL_COUNT - 1;
                            if (gpAdvManager->m_lastHoverCell != mouseX
                                || gpAdvManager->m_hoverCellY != mouseY) {
                                gpAdvManager->m_lastHoverCell = mouseX;
                                gpAdvManager->m_hoverCellY = mouseY;
                                cell = gpAdvManager->GetCell(
                                    gpAdvManager->m_mapOriginX + mouseX,
                                    gpAdvManager->m_mapOriginY + mouseY
                                );
                                if ((cell->m_triggerType & MAP_TRIGGER_EVENT)
                                    || (cell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)) {
                                    gpWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                                    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
                                } else {
                                    gpWindowManager->m_dialogResult = TRAVEL_DIALOG_ACCEPT;
                                    gpMouseManager->SetPointer(ADVENTURE_POINTER_MOVE);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case PANEL_CLOSE_WIDGET:
                            gpWindowManager->m_dialogResult = TRAVEL_DIALOG_REJECT;
                            result = 1;
                            break;
                    }
                    break;
            }
            break;
    }
    if (result) {
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x000654ad; preferred Buka symbol ?ComboDraw@advManager@@QAEHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.384237;margin=0.212683;shape=0.299;size=0.586;calls=0.778;alternate=pol20:int advManager::ComboDraw(int, int, int)@0x000654ad
// HoMM1 retail returns the redraw flag in AL (xor al,al / mov al,1).
VA(0x00433b10, 0xaf6)
signed char advManager::ComboDraw(short originX, short originY, signed char animate) {
    DATA(0x004904a4)
    static int giFrameCount = 0;
    int updateCount;
    int drawY;
    int drawX;
    mapCell* cellPtr;

    PollSound();
    if (!bShowIt)
        return 0;
    if (m_forceCompleteDraw) {
        CompleteDraw(originX, originY, 0);
        return 1;
    }
    if (animate) {
        giFrameCount += giFrameStep;
        if (giFrameCount < COMBO_FRAME_LIMIT) {
            Process1WindowsMessage();
            if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT])
                glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
            PollSound();
            return 0;
        } else {
            giFrameCount = 0;
        }
    }

    m_previousOriginX = m_mapOriginX;
    m_previousOriginY = m_mapOriginY;
    memset(bComboDraw, 0, COMBO_CLEAR_BYTES);
    m_comboHeroDrawn = 0;

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
            if (originX + drawX >= 0 && originX + drawX < MAP_CELL_GRID_SIZE && originY + drawY >= 0
                && originY + drawY < MAP_CELL_GRID_SIZE) {
                cellPtr = GetCell(originX + drawX, originY + drawY);
                if (cellPtr->m_flags & (MAP_CELL_OBJECT_ANIMATED | MAP_CELL_OVERLAY_ANIMATED))
                    ++bComboDraw[drawX][drawY];
                if (cellPtr->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
                    ++bComboDraw[drawX][drawY];
                    if (GetCloudLookup(originX + drawX, originY + drawY)) {
                        bComboDraw[drawX + 1][drawY] += COMBO_CLOUD_MARK;
                        if (drawY >= 1) {
                            bComboDraw[drawX][drawY - 1] += COMBO_CLOUD_MARK;
                            bComboDraw[drawX + 1][drawY - 1] += COMBO_CLOUD_MARK;
                        }
                    } else {
                        ++bComboDraw[drawX + 1][drawY];
                        if (drawY >= 1) {
                            ++*(bComboDraw[drawX] + drawY - 1);
                            ++bComboDraw[drawX + 1][drawY - 1];
                        }
                    }
                }
                if (cellPtr->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                    || cellPtr->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)) {
                    ++bComboDraw[drawX][drawY];
                    if (GetCloudLookup(originX + drawX, originY + drawY)) {
                        bComboDraw[drawX + 1][drawY] += COMBO_CLOUD_MARK;
                        bComboDraw[drawX][drawY + 1] += COMBO_CLOUD_MARK;
                        if (drawY >= 1)
                            bComboDraw[drawX][drawY - 1] += COMBO_CLOUD_MARK;
                        if (drawX >= 1)
                            bComboDraw[drawX - 1][drawY] += COMBO_CLOUD_MARK;
                    } else {
                        ++bComboDraw[drawX + 1][drawY];
                        ++bComboDraw[drawX][drawY + 1];
                        if (drawY >= 1)
                            ++*(bComboDraw[drawX] + drawY - 1);
                        if (drawX >= 1)
                            ++bComboDraw[drawX - 1][drawY];
                    }
                }
            }
        }
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
            if (bComboDraw[drawX][drawY]) {
                if (originX + drawX < 0 || originX + drawX >= MAP_CELL_GRID_SIZE
                    || originY + drawY < 0 || originY + drawY >= MAP_CELL_GRID_SIZE)
                    bComboDraw[drawX][drawY] = 0;
                else if (bComboDraw[drawX][drawY] < COMBO_CLOUD_MARK
                         && !GetCloudLookup(originX + drawX, originY + drawY))
                    bComboDraw[drawX][drawY] = 0;
            }
        }
    }

    if (gpMouseManager->IsVis()) {
        drawX = gpMouseManager->m_unknown49 >> CELL_PIXEL_SHIFT;
        drawY = gpMouseManager->m_unknown4d >> CELL_PIXEL_SHIFT;
        ++bComboDraw[drawX][drawY];
        ++bComboDraw[drawX + 1][drawY];
        ++bComboDraw[drawX][drawY + 1];
        ++bComboDraw[drawX + 1][drawY + 1];
        ++bComboDraw[drawX + COMBO_FAR_NEIGHBOR_OFFSET][drawY + 1];
    }
    if (m_heroContextLocked) {
        for (drawY = ADVMGR_VIEW_CENTER - 1; drawY <= ADVMGR_VIEW_CENTER + 1; drawY++)
            for (drawX = ADVMGR_VIEW_CENTER - 1; drawX <= ADVMGR_VIEW_CENTER + 1; drawX++)
                ++bComboDraw[drawX][drawY];
    }
    if (m_cursorType == ADVMGR_HERO_ICON_BOAT) {
        ++bComboDraw[ADVMGR_VIEW_CENTER - 1][ADVMGR_VIEW_CENTER - 2];
        ++bComboDraw[ADVMGR_VIEW_CENTER][ADVMGR_VIEW_CENTER - 2];
        ++bComboDraw[ADVMGR_VIEW_CENTER + 1][ADVMGR_VIEW_CENTER - 2];
    }

    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        if (bComboDraw[drawX][0])
            DrawCell(
                originX + drawX,
                originY,
                drawX,
                0,
                ADVMGR_DRAW_GROUND | ADVMGR_DRAW_OBJECT,
                0,
                0
            );
    }
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_GROUND, 0, 0);
        }
    }
    for (drawY = 1; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        PollSound();
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY - 1])
                DrawCell(
                    originX + drawX,
                    originY + drawY - 1,
                    drawX,
                    drawY - 1,
                    ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                    0,
                    0
                );
        }
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_OBJECT, 0, 0);
        }
    }
    for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
        if (bComboDraw[drawX][ADVMGR_VIEW_CELL_COUNT - 1])
            DrawCell(
                originX + drawX,
                originY + ADVMGR_VIEW_CELL_COUNT - 1,
                drawX,
                ADVMGR_VIEW_CELL_COUNT - 1,
                ADVMGR_DRAW_OVERLAY | ADVMGR_DRAW_HERO,
                0,
                0
            );
    }
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY])
                DrawCell(originX + drawX, originY + drawY, drawX, drawY, ADVMGR_DRAW_CLOUD, 0, 0);
        }
    }

    PollSound();
    UpdBottomView(0, 1, 1);
    DrawAdventureBorder();
    giLimitUpdMinX = ADVMGR_VIEW_CELL_COUNT;
    giLimitUpdMinY = ADVMGR_VIEW_CELL_COUNT;
    giLimitUpdMaxX = 0;
    giLimitUpdMaxY = 0;
    updateCount = 0;
    for (drawY = 0; drawY < ADVMGR_VIEW_CELL_COUNT; drawY++) {
        for (drawX = 0; drawX < ADVMGR_VIEW_CELL_COUNT; drawX++) {
            if (bComboDraw[drawX][drawY]) {
                updateCount++;
                if (drawX < giLimitUpdMinX)
                    giLimitUpdMinX = drawX;
                if (drawX > giLimitUpdMaxX)
                    giLimitUpdMaxX = drawX;
                if (drawY < giLimitUpdMinY)
                    giLimitUpdMinY = drawY;
                if (drawY > giLimitUpdMaxY)
                    giLimitUpdMaxY = drawY;
            }
        }
    }
    giLimitUpdMinX <<= CELL_PIXEL_SHIFT;
    giLimitUpdMinY <<= CELL_PIXEL_SHIFT;
    giLimitUpdMaxX = ((giLimitUpdMaxX + 1) << CELL_PIXEL_SHIFT) - 1;
    giLimitUpdMaxY = ((giLimitUpdMaxY + 1) << CELL_PIXEL_SHIFT) - 1;
    if (giLimitUpdMinX < COMBO_UPDATE_MIN)
        giLimitUpdMinX = COMBO_UPDATE_MIN;
    if (giLimitUpdMaxX > COMBO_UPDATE_MAX)
        giLimitUpdMaxX = COMBO_UPDATE_MAX;
    if (giLimitUpdMinY < COMBO_UPDATE_MIN)
        giLimitUpdMinY = COMBO_UPDATE_MIN;
    if (giLimitUpdMaxY > COMBO_UPDATE_MAX)
        giLimitUpdMaxY = COMBO_UPDATE_MAX;
    if (giLimitUpdMaxX < giLimitUpdMinX || giLimitUpdMaxY < giLimitUpdMinY) {
        giLimitUpdMinX = giLimitUpdMaxX - 1;
        giLimitUpdMinY = giLimitUpdMaxY - 1;
        return 0;
    }
    return 1;
}

// Buka 2.1 ComboDraw(update) forwards the current map origin.
VA(0x00434606, 0x3a)
signed char advManager::ComboDraw(int update) {
    return ComboDraw(m_mapOriginX, m_mapOriginY, update);
}

// donor PoL RVA 0x0006668e; preferred Buka symbol ?SetEnvironmentOrigin@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.577153;margin=0.265756;shape=0.425;size=0.947;calls=1.000;alternate=pol20:void advManager::SetEnvironmentOrigin(int, int, int)@0x0006668e
VA(0x00434640, 0x2dd)
void advManager::SetEnvironmentOrigin(short originX, short originY, short stopSounds) {
    int soundRadius;
    int edgeOffset;
    int maxCells = ADVMGR_ACTIVE_SOUND_COUNT / 2;
    int layer;

    if (gpSoundManager->m_musicReady == 0)
        return;
    for (edgeOffset = 0; edgeOffset < ADVMGR_ACTIVE_SOUND_COUNT; ++edgeOffset) {
        if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE) {
            if (stopSounds) {
                gpSoundManager->StopSample(
                    m_loopingSamples[m_activeSounds[edgeOffset].soundId]
                        ->m_playbackData.activeSample
                );
                m_activeSounds[edgeOffset].soundId = MAP_SOUND_NONE;
                m_activeSounds[edgeOffset].volume = ENVIRONMENT_SOUND_DEFAULT_VOLUME;
            } else {
                m_activeSounds[edgeOffset].volume = ENVIRONMENT_SOUND_DEFAULT_VOLUME;
            }
        }
    }
    if (gConfig.soundVolume != SOUND_VOLUME_OFF) {
        m_activeSoundMask = 0;
        for (layer = ENVIRONMENT_SOUND_FIRST_LAYER; layer <= ENVIRONMENT_SOUND_LAYER_COUNT;
             ++layer) {
            InsertSound(originX, originY, 0, layer);
            for (soundRadius = 0; soundRadius < ENVIRONMENT_SOUND_RADIUS_COUNT; ++soundRadius) {
                for (edgeOffset = 0; edgeOffset < soundRadius * ENVIRONMENT_SOUND_EDGE_SPAN;
                     ++edgeOffset) {
                    InsertSound(
                        originX - soundRadius + edgeOffset,
                        originY - soundRadius,
                        soundRadius,
                        layer
                    );
                    InsertSound(
                        originX + soundRadius,
                        originY - soundRadius + edgeOffset,
                        soundRadius,
                        layer
                    );
                    InsertSound(
                        originX + soundRadius - edgeOffset,
                        originY + soundRadius,
                        soundRadius,
                        layer
                    );
                    InsertSound(
                        originX - soundRadius,
                        originY + soundRadius - edgeOffset,
                        soundRadius,
                        layer
                    );
                }
            }
        }
        for (edgeOffset = 0; edgeOffset < ADVMGR_ACTIVE_SOUND_COUNT; ++edgeOffset) {
            if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE
                && m_activeSounds[edgeOffset].volume > ENVIRONMENT_SOUND_MAX_DISTANCE) {
                gpSoundManager->StopSample(
                    m_loopingSamples[m_activeSounds[edgeOffset].soundId]
                        ->m_playbackData.activeSample
                );
                m_activeSounds[edgeOffset].soundId = MAP_SOUND_NONE;
            }
            if (m_activeSounds[edgeOffset].soundId != MAP_SOUND_NONE
                && (m_activeSoundMask & (1 << m_activeSounds[edgeOffset].soundId)) != 0) {
                gpSoundManager->ModifySample(
                    m_loopingSamples[m_activeSounds[edgeOffset].soundId]
                        ->m_playbackData.activeSample,
                    100,
                    glEnvironmentVolume[m_activeSounds[edgeOffset].volume]
                );
            }
        }
    }
}

// donor PoL RVA 0x000669c6; preferred Buka symbol ?CheckLoadSample@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.741786;margin=0.490066;shape=0.533;size=0.857;calls=1.000;strings=loop%04d.82M;alternate=pol20:void advManager::CheckLoadSample(int)@0x000669c6
VA(0x0043491d, 0x69)
void advManager::CheckLoadSample(int index) {
    if (m_loopingSamples[index] == NULL) {
        TrimLoopingSounds(ADVMGR_ACTIVE_SOUND_COUNT);
        sprintf(gText, "loop%04d.82M", index);
        m_loopingSamples[index] = gpResourceManager->GetSample(gText);
    }
}

// donor PoL RVA 0x00066ef0; preferred Buka symbol ?InsertSound@advManager@@QAEXHHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.476286;margin=0.526376;shape=0.266;size=0.902;calls=0.750;alternate=pol20:void advManager::InsertSound(int, int, int, int)@0x00066ef0
VA(0x00434986, 0x251)
void advManager::InsertSound(short x, short y, short distance, signed char soundLayer) {
    int slot;
    int distanceLimit;
    int i;
    int soundId;

    if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
        return;
    soundId = gpGame->m_mapSounds[x][y];
    if (soundId == MAP_SOUND_NONE)
        return;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].soundId == soundId) {
            if (m_activeSounds[i].volume > distance) {
                m_activeSounds[i].volume = distance;
                m_activeSoundMask |= 1 << m_activeSounds[i].soundId;
            }
            return;
        }
    }
    if (soundLayer == ENVIRONMENT_SOUND_FIRST_LAYER)
        return;
    distanceLimit = distance;
    slot = ENVIRONMENT_SOUND_NO_SLOT;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].volume > distanceLimit) {
            distanceLimit = m_activeSounds[i].volume;
            slot = i;
        }
    }
    if (slot != ENVIRONMENT_SOUND_NO_SLOT) {
        if (m_activeSounds[slot].soundId != MAP_SOUND_NONE)
            gpSoundManager->StopSample(
                m_loopingSamples[m_activeSounds[slot].soundId]->m_playbackData.activeSample
            );
        m_activeSounds[slot].soundId = soundId;
        m_activeSounds[slot].volume = distance;
        CheckLoadSample(soundId);
        m_loopingSamples[soundId]->m_playbackData.volume = glEnvironmentVolume[distance];
        m_loopingSamples[soundId]->m_playbackData.loopCount = 0;
        m_loopingSamples[soundId]->m_playbackData.channelType = ENVIRONMENT_SOUND_CHANNEL_TYPE;
        gpSoundManager->MemorySample(m_loopingSamples[soundId]);
        m_activeSoundMask ^= 1 << m_activeSounds[slot].soundId;
    }
}

// Retail ADVMGR .bss keeps four objects no HoMM1 code references: the
// Buka/PoL ADVMGR globals iThisMaxY, iThisMinY, USMsg and CDMsg (both donors
// declare them in advManager.h and never use them). VC4 orders .bss by name
// key, and these keys fit the unclaimed retail slots: 229, 317 and 351 between
// cPanel (203) and giFrameStep (414) fill 0x004c4f30-0x004c4f47 (4 + 4 + 16
// bytes; HoMM1 has no town portal, so giTownPortalChoice/townPortalWin are
// absent), and 892 between bComboDraw (596) and iLastAnimFrame (957) fills
// 0x004c50b0-0x004c50bf. .bss position does not move them, so they sit with
// the spell code (USMsg and CDMsg after TeleportTo).
DATA(0x004c4f30)
int iThisMaxY;
DATA(0x004c4f34)
int iThisMinY;

// donor PoL RVA 0x0006712a; preferred Buka symbol ?TeleportTo@advManager@@QAEXPAVhero@@HHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.494469;margin=0.364782;shape=0.352;size=0.864;calls=0.864;alternate=pol20:void advManager::TeleportTo(class hero *, int, int, int, int)@0x0006712a
VA(0x00434bd7, 0x340)
void advManager::TeleportTo(int x, int y, int) {
    int savedShow;
    int fizzle;
    mapCell* destinationCell;
    mapCell* oldCell;
    int tmp;
    signed char newTerrain;
    hero* mapHero;
    town* occupiedTown;

    savedShow = bShowIt;
    mapHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    destinationCell = GetCell(x, y);
    oldCell = GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER);
    if (mapHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
        occupiedTown = gpGame->GetTown(mapHero->m_occupiedTown);
        occupiedTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (oldCell->m_flags & MAP_CELL_HERO_CURSOR)
        oldCell->m_flags -= MAP_CELL_HERO_CURSOR;
    CompleteDraw(0);
    if (!gbHumanPlayer[giCurPlayer]) {
        if (!gConfig.blackoutComputer && !gbRemoteOn
            && (gpGame->m_mapExtra[mapHero->m_x][mapHero->m_y] & giCurWatchPlayerHighBit))
            bShowIt = 1;
        else
            bShowIt = 0;
    }
    if (savedShow)
        HideRoute(1, 1, 1);
    if (bShowIt) {
        m_mapOriginX = x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = y - ADVMGR_VIEW_CENTER;
        DelayMilli(90);
    }
    mapHero->m_x = x;
    mapHero->m_y = y;
    gpGame->SetVisibility(
        m_mapOriginX + ADVMGR_VIEW_CENTER,
        m_mapOriginY + ADVMGR_VIEW_CENTER,
        giCurPlayer,
        gHeroScoutRadius[mapHero->m_heroClass]
    );
    if (bShowIt) {
        destinationCell->m_flags |= MAP_CELL_HERO_CURSOR;
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->SaveFizzleSource(
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_SIZE,
            UPDATE_VIEWPORT_SIZE
        );
        CompleteDraw(0);
        PollSound();
        fizzle = TELEPORT_FIZZLE_TIME;
        if (!gbHumanPlayer[giCurPlayer])
            fizzle -= TELEPORT_REMOTE_FIZZLE_ADJUSTMENT;
        gpWindowManager->FizzleForward(
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_ORIGIN,
            UPDATE_VIEWPORT_SIZE,
            UPDATE_VIEWPORT_SIZE,
            FIZZLE_USE_DEFAULT_DELAY
        );
        PollSound();
        gpMouseManager->ReallyShowPointer();
    }
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    newTerrain = CELL_TERRAIN(destinationCell);
    if (m_currentTerrain != newTerrain) {
        m_currentTerrain = newTerrain;
        gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    }
    Reseed(0, 0);
    UpdateRadar(1, 0);
    CompleteDraw(0);
    ForceNewHover();
}

// donor PoL RVA 0x00067539; preferred Buka symbol ?DimensionDoor@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.671113;margin=0.501597;shape=0.372;size=0.883;calls=0.867;strings=dimdoor.bin;alternate=pol20:void advManager::DimensionDoor(void)@0x00067539
VA(0x00434f17, 0x246)
void advManager::DimensionDoor(void) {
    hero* heroPointer;
    heroWindow* win;
    short x;
    short y;
    mapCell* targetCell;

    win = new heroWindow(0, 0, "dimdoor.bin");
    if (win == NULL)
        MemError();
    SetWinText(win, WINDOW_TEXT_DIMENSION_DOOR);
    gpWindowManager->DoDialog(win, DimensionDoorHandler, 0);
    delete win;
    heroPointer = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (gpWindowManager->m_dialogResult == TRAVEL_DIALOG_ACCEPT) {
        x = m_mapOriginX + m_lastHoverCell;
        y = m_mapOriginY + m_hoverCellY;
        targetCell = GetCell(x, y);
        if (((heroPointer->m_eventFlags & HERO_EVENT_EMBARKED)
             && targetCell->m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
            || (!(heroPointer->m_eventFlags & HERO_EVENT_EMBARKED)
                && targetCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)) {
            NormalDialog("Dimension Door failed!!!", NORMAL_DIALOG_TYPE_OK, 0x61, 0x91);
            UpdateRadar(1, 0);
        } else {
            gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_TELEPORT);
            TeleportTo(x, y, 0);
            gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
        }
        gpGame->GetHero(gpCurPlayer->m_currentHero)->UseSpell(SPELL_DIMENSION_DOOR);
    } else {
        UpdateRadar(1, 0);
    }
}

DATA(0x004c4f38)
struct tag_message USMsg;
DATA(0x004c50b0)
struct tag_message CDMsg;

// donor PoL RVA 0x0006785d; preferred Buka symbol ?TownGate@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.385467;margin=0.208066;shape=0.290;size=0.657;calls=0.526;alternate=pol20:void advManager::TownGate(int)@0x0006785d
VA(0x0043515d, 0x2a6)
void advManager::TownGate(void) {
    int k;
    int bestDist;
    int bestTown;
    hero* heroPointer;
    int distance;

    bestDist = TOWN_PORTAL_DISTANCE_LIMIT;
    bestTown = -1;
    heroPointer = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (heroPointer->m_eventFlags & HERO_EVENT_EMBARKED) {
        NormalDialog(
            "Town Gate Failed!!!  You must be on land for this spell to work.",
            NORMAL_DIALOG_TYPE_OK
        );
        return;
    }
    for (k = 0; k < gpCurPlayer->m_townCount; k++) {
        distance = abs(gpGame->m_castleRecs[gpCurPlayer->m_townIds[k]].m_x - heroPointer->m_x)
                   + abs(gpGame->m_castleRecs[gpCurPlayer->m_townIds[k]].m_y - heroPointer->m_y);
        if (distance < bestDist) {
            bestDist = distance;
            bestTown = k;
        }
    }
    if (bestTown == -1)
        NormalDialog("No available town.  Town Gate Failed!!!", NORMAL_DIALOG_TYPE_OK);
    if (gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_occupyingHeroId
        != TOWN_OCCUPYING_HERO_NONE) {
        NormalDialog("Nearest town occupied.  Town Gate Failed!!!", NORMAL_DIALOG_TYPE_OK, 0x61);
        return;
    }
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_TELEPORT);
    TeleportTo(
        gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_x,
        gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_y,
        0
    );
    heroPointer->UseSpell(SPELL_TOWN_GATE);
    gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].m_occupyingHeroId = heroPointer->m_id;
    gpGame->m_castleRecs[gpCurPlayer->m_townIds[bestTown]].GiveSpells();
    heroPointer->m_locationType = (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN);
    heroPointer->m_occupiedTown = gpCurPlayer->m_townIds[bestTown];
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
}

// donor PoL RVA 0x00067c9b; preferred Buka symbol ?SummonBoat@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.502597;margin=0.051626;shape=0.294;size=0.965;calls=0.867;alternate=pol20:void advManager::SummonBoat(void)@0x00067c9b
VA(0x00435403, 0x51c)
void advManager::SummonBoat(void) {
    hero* pHero;
    signed char boatFound;
    mapCell* pCell;
    short iDirection;
    signed char foundCell;
    boatRecord* thisBoat;
    short slotIndex;
    signed char heroNum;
    mapCell* fromCell;
    short drawHeight;
    short drawY;
    short drawWidth;
    short drawX;

    pHero = &gpGame->m_heroRecs[gpCurPlayer->CurrentHero()];
    foundCell = 0;
    boatFound = 0;
    pCell = GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER);
    if (pCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
        goto summon_done;
    for (iDirection = 0; iDirection < MAP_DIRECTION_COUNT; iDirection++) {
        pCell = GetCell(
            normalDirTable[iDirection].x + m_mapOriginX + ADVMGR_VIEW_CENTER,
            normalDirTable[iDirection].y + m_mapOriginY + ADVMGR_VIEW_CENTER
        );
        if (pCell->m_objectIndex == MAP_CELL_NO_FRAME
            && pCell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN) {
            foundCell = 1;
            break;
        }
    }
    if (foundCell) {
        heroNum = gpCurPlayer->CurrentHero();
        for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
            if (gpGame->m_boatSlots[slotIndex] != GAME_TABLE_FREE
                && gpGame->m_boats[slotIndex].heroId == (heroNum | SUMMON_OCCUPIED_FLAG)) {
                boatFound = 1;
                break;
            }
        }
        if (!boatFound) {
            for (slotIndex = 0; slotIndex < GAME_BOAT_COUNT; slotIndex++) {
                if (gpGame->m_boatSlots[slotIndex] != GAME_TABLE_FREE
                    && (gpGame->m_boats[slotIndex].heroId & SUMMON_OCCUPIED_FLAG)
                    && gpGame->m_boats[slotIndex].owner == giCurPlayer) {
                    boatFound = 1;
                    break;
                }
            }
        }
        if (boatFound) {
            thisBoat = &gpGame->m_boats[slotIndex];
            fromCell = GetCell(thisBoat->x, thisBoat->y);
            gpGame->RestoreCell(
                thisBoat->x,
                thisBoat->y,
                thisBoat->savedTriggerType,
                thisBoat->savedEventData,
                NULL,
                SUMMON_RESTORE_MODE
            );
            if (thisBoat->x >= m_mapOriginX && thisBoat->x < m_mapOriginX + ADVMGR_VIEW_CELL_COUNT
                && thisBoat->y >= m_mapOriginY
                && thisBoat->y < m_mapOriginY + ADVMGR_VIEW_CELL_COUNT) {
                drawX = (thisBoat->x - m_mapOriginX) * CELL_PIXELS - SUMMON_FIZZLE_X_OFFSET;
                if (drawX < SUMMON_SCREEN_MARGIN)
                    drawX = SUMMON_SCREEN_MARGIN;
                drawY = (thisBoat->y - m_mapOriginY) * CELL_PIXELS - SUMMON_FIZZLE_Y_OFFSET;
                if (drawY < SUMMON_SCREEN_MARGIN)
                    drawY = SUMMON_SCREEN_MARGIN;
                drawWidth = SUMMON_FIZZLE_WIDTH;
                drawHeight = SUMMON_FIZZLE_HEIGHT;
                if (drawX + drawWidth >= SUMMON_SCREEN_LIMIT)
                    drawWidth = SUMMON_SCREEN_LIMIT - drawX;
                if (drawY + drawHeight >= SUMMON_SCREEN_LIMIT)
                    drawHeight = SUMMON_SCREEN_LIMIT - drawY;
                gpWindowManager->SaveFizzleSource(drawX, drawY, drawWidth, drawHeight);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager
                    ->FizzleForward(drawX, drawY, drawWidth, drawHeight, FIZZLE_USE_DEFAULT_DELAY);
            }
            thisBoat->x = normalDirTable[iDirection].x + m_mapOriginX + ADVMGR_VIEW_CENTER;
            thisBoat->y = normalDirTable[iDirection].y + m_mapOriginY + ADVMGR_VIEW_CENTER;
            thisBoat->savedTriggerType = pCell->m_triggerType;
            thisBoat->savedEventData = pCell->m_objectMetadata;
            pCell->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP);
            pCell->m_objectMetadata = slotIndex;
            gpWindowManager->SaveFizzleSource(176, 192, 128, 96);
            CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
            gpWindowManager->FizzleForward(
                SUMMON_TARGET_X,
                SUMMON_TARGET_Y,
                SUMMON_TARGET_WIDTH,
                SUMMON_TARGET_HEIGHT,
                FIZZLE_USE_DEFAULT_DELAY
            );
        }
    }

summon_done:
    UpdateScreen(0, 0);
    Reseed(0, 0);
    if (!boatFound)
        NormalDialog("Summon Boat failed!!!", NORMAL_DIALOG_TYPE_OK, 0x61, 0x91);
}

// donor PoL RVA 0x00068247; preferred Buka symbol ?ShowRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.438878;margin=0.464254;shape=0.279;size=0.721;calls=1.000;alternate=pol20:void advManager::ShowRoute(int, int, int)@0x00068247
VA(0x0043591f, 0x31b)
void advManager::ShowRoute(int redraw, int, int updateButton) {
    hero* pHero;
    int canReach;
    int fromDirection;
    int x;
    int y;
    int dir;
    int j;
    int remMob;
    int terr;
    // The widget command for the continue-route button; retail allocation
    // follows this local name (renaming it moves registers).
    short buttonFrame;

    canReach = 0;
    if (!gbThisNetHumanPlayer[giCurPlayer] && (!giDebugLevel || !giShowComputerRoute))
        return;
    if (gpCurPlayer->m_currentHero == INVALID_HERO) {
        HideRoute(redraw, 0, 1);
        return;
    }
    pHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (pHero->m_destinationX == HERO_DESTINATION_NONE) {
        HideRoute(redraw, 1, 1);
        return;
    }
    gpSearchArray->BuildPath(
        pHero->m_x,
        pHero->m_y,
        pHero->m_destinationX,
        pHero->m_destinationY,
        SEARCH_UNLIMITED_COST
    );
    if (gpSearchArray->m_pathLength > 0) {
        memset(m_visibilityMap, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
        m_routeShown = 1;
        remMob = pHero->m_remainingMobility;
        x = pHero->m_x;
        y = pHero->m_y;
        for (j = gpSearchArray->m_pathLength - 1; j >= 0; --j) {
            dir = gpSearchArray->m_directions[j];
            terr = CELL_TERRAIN(GetCell(x, y));
            remMob -=
                CalcTerrainCost(terr, dir & MAP_DIRECTION_DIAGONAL_BIT, remMob, pHero->m_heroClass);
            x += normalDirTable[dir].x;
            y += normalDirTable[dir].y;
            if (j == 0) {
                ADVMGR_VISIBILITY_AT(x, y) = ROUTE_CELL_DESTINATION;
            } else {
                fromDirection = gpSearchArray->m_directions[j - 1];
                ADVMGR_VISIBILITY_AT(x, y) = gRouteFrame[fromDirection][dir];
            }
            if (remMob >= 0) {
                ADVMGR_VISIBILITY_AT(x, y) =
                    ADVMGR_VISIBILITY_AT(x, y) + ROUTE_CELL_REACHABLE_OFFSET;
                canReach = 1;
            }
        }
        if (updateButton) {
            buttonFrame = canReach ? WIDGET_COMMAND_CLEAR_FLAGS : WIDGET_COMMAND_SET_FLAGS;
            gpWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                buttonFrame,
                ADVENTURE_CONTROL_CONTINUE_ROUTE,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        }
    } else {
        HideRoute(redraw, 1, 1);
    }
    if (redraw) {
        CompleteDraw(0);
        gpMouseManager->ReallyHidePointer();
        UpdateScreen(0, 0);
        gpMouseManager->ReallyShowPointer();
    }
}

// donor PoL RVA 0x00068720; preferred Buka symbol ?HideRoute@advManager@@QAEXHHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.524051;margin=0.948488;shape=0.403;size=0.809;calls=1.000;alternate=pol20:void advManager::HideRoute(int, int, int)@0x00068720
VA(0x00435c3a, 0x106)
void advManager::HideRoute(int redraw, int clearDestination, int updateButton) {
    hero* currentHero;

    if (!gbThisNetHumanPlayer[giCurPlayer] && (!giDebugLevel || !giShowComputerRoute))
        return;

    if (updateButton)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_SET_FLAGS,
            PANEL_CONTINUE_ROUTE,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );

    if (clearDestination && gpCurPlayer->m_currentHero != INVALID_HERO) {
        currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
        currentHero->m_destinationX = HERO_DESTINATION_NONE;
        currentHero->m_destinationY = HERO_DESTINATION_NONE;
    }

    if (!m_routeShown)
        return;

    m_routeShown = 0;
    if (redraw) {
        CompleteDraw(0);
        UpdateScreen(0, 0);
    }
}

// donor PoL RVA 0x00068827; preferred Buka symbol ?CheckDimHero@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.465466;margin=0.233620;shape=0.306;size=0.738;calls=1.000;alternate=pol20:void advManager::CheckDimHero(void)@0x00068827
VA(0x00435d40, 0x91)
void advManager::CheckDimHero(void) {
    if (!gbThisNetHumanPlayer[giCurPlayer] || gpCurPlayer->CurrentHero() == INVALID_HERO)
        return;
    if (!gpGame->IsMobile(gpCurPlayer->CurrentHero())) {
        ShowRoute(1, 0, 0);
        UpdateHeroLocators(1, 1);
        gpAdvManager->CheckDimNextHeroBut();
    }
}

// donor PoL RVA 0x000688b4; preferred Buka symbol ?CheckDimNextHeroBut@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.454682;margin=0.437763;shape=0.225;size=0.845;calls=1.000;alternate=pol20:void advManager::CheckDimNextHeroBut(void)@0x000688b4
VA(0x00435dd1, 0x6e)
void advManager::CheckDimNextHeroBut(void) {
    short frame;

    if (!gbThisNetHumanPlayer[giCurPlayer] || !gpCurPlayer->HasMobileHero())
        frame = WIDGET_COMMAND_SET_FLAGS;
    else
        frame = WIDGET_COMMAND_CLEAR_FLAGS;
    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        frame,
        BUTTON_BROADCAST_ARG,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
}

// donor PoL RVA 0x0006891f; preferred Buka symbol ?SeedTo@advManager@@QAEXHH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.530039;margin=0.750383;shape=0.400;size=0.820;calls=1.000;alternate=pol20:void advManager::SeedTo(int, int)@0x0006891f
VA(0x00435e3f, 0x152)
void advManager::SeedTo(int targetX, int targetY) {
    hero* currentHero;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    if (gpCurPlayer->m_currentHero == INVALID_HERO)
        return;

    currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    if (!giSeedingValid)
        gpSearchArray->SeedPosition(
            currentHero->m_x,
            currentHero->m_y,
            m_cursorDirection,
            SEARCH_UNLIMITED_COST,
            m_cursorType == ADVMGR_HERO_ICON_BOAT,
            0,
            currentHero->m_remainingMobility,
            currentHero->m_heroClass,
            targetX,
            targetY,
            0,
            1
        );
    else if (!giFullySeeded)
        gpSearchArray->SeedPosition(
            currentHero->m_x,
            currentHero->m_y,
            m_cursorDirection,
            SEARCH_UNLIMITED_COST,
            m_cursorType == ADVMGR_HERO_ICON_BOAT,
            0,
            currentHero->m_remainingMobility,
            currentHero->m_heroClass,
            targetX,
            targetY,
            1,
            1
        );
}

// Buka 2.1 ForceNewHover; HoMM1 routes the hover through a message record.
VA(0x00435f91, 0x4f)
void advManager::ForceNewHover(void) {
    struct tag_message msg;

    if (!gbThisNetHumanPlayer[giCurPlayer])
        return;
    m_lastHoverCell = CURSOR_INVALID_POSITION;
    msg.id = ADVENTURE_CONTROL_MAP_VIEW;
    ProcessHover(&msg);
}

VA(0x00435fe0, 0x1b6)
void advManager::ScreenScroll(signed char direction, int updatePointer) {
    short yOrigin;
    short xOrigin;

    xOrigin = m_mapOriginX;
    yOrigin = m_mapOriginY;
    iLastScrollTime = KBTickCount();

    switch (direction) {
        case MAP_DIRECTION_NORTH:
            --yOrigin;
            break;
        case MAP_DIRECTION_NORTH_EAST:
            ++xOrigin;
            --yOrigin;
            break;
        case MAP_DIRECTION_EAST:
            ++xOrigin;
            break;
        case MAP_DIRECTION_SOUTH_EAST:
            ++xOrigin;
            ++yOrigin;
            break;
        case MAP_DIRECTION_SOUTH:
            ++yOrigin;
            break;
        case MAP_DIRECTION_SOUTH_WEST:
            --xOrigin;
            ++yOrigin;
            break;
        case MAP_DIRECTION_WEST:
            --xOrigin;
            break;
        case MAP_DIRECTION_NORTH_WEST:
            --xOrigin;
            --yOrigin;
            break;
    }

    if (updatePointer)
        gpMouseManager->SetPointer(direction + HOVER_SCROLL_FRAME_FIRST);

    if (xOrigin < SCROLL_MIN_ORIGIN)
        xOrigin = SCROLL_MIN_ORIGIN;
    if (xOrigin > SCROLL_MAX_ORIGIN)
        xOrigin = SCROLL_MAX_ORIGIN;
    if (yOrigin < SCROLL_MIN_ORIGIN)
        yOrigin = SCROLL_MIN_ORIGIN;
    if (yOrigin > SCROLL_MAX_ORIGIN)
        yOrigin = SCROLL_MAX_ORIGIN;

    if (xOrigin != m_mapOriginX || yOrigin != m_mapOriginY) {
        DemobilizeCurrHero();
        m_mapOriginX = xOrigin;
        m_mapOriginY = yOrigin;
        UpdateRadar(1, 0);
        CompleteDraw(0);
        UpdateScreen(0, 0);
    }
}

// donor PoL RVA 0x00068c5c; preferred Buka symbol ?CheckScreenScroll@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.475412;margin=0.607960;shape=0.306;size=0.761;calls=1.000;alternate=pol20:void advManager::CheckScreenScroll(void)@0x00068c5c
VA(0x00436196, 0x1e1)
void advManager::CheckScreenScroll(void) {
    short mouseX;
    short mouseY;
    int oldX;
    int oldY;

    if (KBTickCount() - iLastScrollTime > SCROLL_TICK_INTERVAL) {
        iLastScrollTime = KBTickCount();
        oldX = m_mapOriginX;
        oldY = m_mapOriginY;
        gpMouseManager->MouseCoords(mouseX, mouseY);

        if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0
            && mouseY < LOGICAL_SCREEN_HEIGHT) {
            if (mouseX < SCROLL_BORDER) {
                if (mouseY < SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_NORTH_WEST, 1);
                else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_SOUTH_WEST, 1);
                else
                    ScreenScroll(MAP_DIRECTION_WEST, 1);
            } else if (mouseX > LOGICAL_SCREEN_WIDTH - SCROLL_BORDER - 1) {
                if (mouseY < SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_NORTH_EAST, 1);
                else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER)
                    ScreenScroll(MAP_DIRECTION_SOUTH_EAST, 1);
                else
                    ScreenScroll(MAP_DIRECTION_EAST, 1);
            } else if (mouseY < SCROLL_BORDER) {
                ScreenScroll(MAP_DIRECTION_NORTH, 1);
            } else if (mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER) {
                ScreenScroll(MAP_DIRECTION_SOUTH, 1);
            }
        }

        if (gpMouseManager->m_cursorFrame >= HOVER_SCROLL_FRAME_FIRST
            && gpMouseManager->m_cursorFrame < HOVER_SCROLL_FRAME_END && oldX == m_mapOriginX
            && oldY == m_mapOriginY)
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    }
}

// donor PoL RVA 0x00068e17; preferred Buka symbol ?MouseInScrollZone@advManager@@QAEHXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.376012;margin=0.441568;shape=0.180;size=0.620;calls=1.000;alternate=pol20:int advManager::MouseInScrollZone(void)@0x00068e17
VA(0x00436377, 0xa3)
int advManager::MouseInScrollZone(void) {
    short mouseX;
    short mouseY;

    gpMouseManager->MouseCoords(mouseX, mouseY);
    if (mouseX >= 0 && mouseX < LOGICAL_SCREEN_WIDTH && mouseY >= 0
        && mouseY < LOGICAL_SCREEN_HEIGHT) {
        if (mouseX < SCROLL_BORDER || mouseX > LOGICAL_SCREEN_WIDTH - SCROLL_BORDER - 1
            || mouseY < SCROLL_BORDER || mouseY > LOGICAL_SCREEN_HEIGHT - SCROLL_BORDER) {
            return 1;
        }
    }
    return 0;
}

// donor PoL RVA 0x00068ea8; preferred Buka symbol ?SetInitialMapOrigin@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.512925;margin=0.905583;shape=0.330;size=0.958;calls=0.778;alternate=pol20:void advManager::SetInitialMapOrigin(void)@0x00068ea8
VA(0x0043641a, 0x283)
void advManager::SetInitialMapOrigin(void) {
    short x;
    short y;
    game* gameState;
    hero* initialHero;
    town* townPointer;
    town* ownTown;

    gpWindowManager->BroadcastMessage(
        MESSAGE_WIDGET,
        WIDGET_COMMAND_SET_FLAGS,
        ADVENTURE_CONTROL_CONTINUE_ROUTE,
        WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
    );
    m_lastHoverCell = m_hoverCellY = 0;
    m_cursorActive = 0;
    gbHeroMoving = 0;
    if (gpCurPlayer->CurrentTown() != GAME_TOWN_NONE) {
        townPointer = gpGame->GetTown(gpCurPlayer->m_currentTown);
        m_mapOriginX = townPointer->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = townPointer->m_y - ADVMGR_VIEW_CENTER;
    } else if (gpCurPlayer->CurrentHero() != INVALID_HERO) {
        MobilizeCurrHero(0);
    } else if (gpCurPlayer->m_heroCount > 0) {
        initialHero = &gpGame->m_heroRecs[gpCurPlayer->m_heroIds[0]];
        m_mapOriginX = initialHero->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = initialHero->m_y - ADVMGR_VIEW_CENTER;
    } else if (gpCurPlayer->m_townCount > 0) {
        ownTown = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[0]];
        m_mapOriginX = ownTown->m_x - ADVMGR_VIEW_CENTER;
        m_mapOriginY = ownTown->m_y - ADVMGR_VIEW_CENTER;
    } else {
        m_mapOriginX = 0;
        m_mapOriginY = 0;
    }
    m_currentTerrain = giGroundToTerrain
        [GetCell(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER)
             ->m_tileIndex];
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    SetEnvironmentOrigin(m_mapOriginX + ADVMGR_VIEW_CENTER, m_mapOriginY + ADVMGR_VIEW_CENTER, 1);
    gpMouseManager->MouseCoords(x, y);
    gpMouseManager->WarpPointer(x - 20, y - 20);
    Reseed(0, 0);
    CheckDimNextHeroBut();
}

// donor PoL RVA 0x00069160; preferred Buka symbol ?LoadRemote@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.559158;margin=0.708697;shape=0.333;size=0.638;calls=0.706;strings=advmice.mse;alternate=pol20:void advManager::LoadRemote(void)@0x00069160
VA(0x0043669d, 0x152)
void advManager::LoadRemote(void) {
    gpMouseManager->ReallyHidePointer();
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gpGame->LoadGame("REMOTE.GAM", 0, 1);
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpGame->CancelComputerScreen();
    gpGame->DoNewTurn();
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    UpdateRadar(1, 0);
    gpMouseManager->ReallyShowPointer();
    UpdBottomView(1, 1, 1);
    if ((gpGame->m_day != 1 || (gpGame->m_week == 1 && gpGame->m_month == 1)) && gbRemoteOn
        && gbThisNetHumanPlayer[giCurPlayer] && giForceSwitchMusic == FORCED_MUSIC_IDLE) {
        gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NETWORK_TURN);
        giForceSwitchMusic = KBTickCount();
    }
}

// donor PoL RVA 0x0006931e; preferred Buka symbol ?CheckHandleNet@advManager@@QAEPADXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410587;margin=0.478746;shape=0.288;size=0.750;calls=0.692;alternate=pol20:char * advManager::CheckHandleNet(void)@0x0006931e
VA(0x004367ef, 0x178)
char* advManager::CheckHandleNet(void) {
    RemoteMessage* receivedPacket;
    int remotePlayerExited;

    // API-forced: GetRemoteData and DoNetCombat pass queue records as char*.
    receivedPacket = reinterpret_cast<RemoteMessage*>(GetRemoteData(1));
    if (receivedPacket && receivedPacket->type == REMOTE_MESSAGE_RELIABLE) {
        switch (receivedPacket->command) {
            case BOX_REMOTE_SAVE:
                remotePlayerExited = receivedPacket->payload.playerExited;
                if (!gpGame->ReceiveSaveGame(
                        receivedPacket->payload.saveSize,
                        receivedPacket->sender
                    ))
                    ShutDown(NULL);
                if (remotePlayerExited)
                    ReceiveRemotePlayerExit(receivedPacket->sender, 0, 1, 0);
                LoadRemote();
                break;
            case REMOTE_COMMAND_CHAT:
                PopNetBox(receivedPacket->payload.data);
                break;
            case REMOTE_COMMAND_HERO_TOWN_DATA:
                if (gbInCombat)
                    return reinterpret_cast<char*>(receivedPacket); // API-forced: char* record.
                else
                    DoNetCombat(
                        reinterpret_cast<char*>(receivedPacket)
                    ); // API-forced: char* record.
                break;
            case REMOTE_COMMAND_PLAYER_EXIT:
                LogStr("receive exit");
                ReceiveRemotePlayerExit(
                    receivedPacket->payload.data[0],
                    receivedPacket->payload.data[1],
                    0,
                    0
                );
                break;
            default:
                return reinterpret_cast<char*>(receivedPacket); // API-forced: char* record.
        }
    }
    return NULL;
}

// donor PoL RVA 0x0006952a; preferred Buka symbol ?CheckHandleNetPlayerWait@advManager@@QAEHAAUtag_message@@H@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.447497;margin=1.157842;shape=0.264;size=0.731;calls=1.000;alternate=pol20:int advManager::CheckHandleNetPlayerWait(struct tag_message &, int)@0x0006952a
VA(0x00436967, 0xd4)
short advManager::CheckHandleNetPlayerWait(struct tag_message& message, signed char doMain) {
    if (message.type == MESSAGE_MOUSE_MOVE)
        gpMouseManager->Main(message);

    CheckDoMain(1, doMain);
    if (message.type == MESSAGE_KEY_DOWN) {
        switch (message.keyCode) {
            case INPUT_SCAN_F1:
                PopNetBox(NULL);
                break;

            case INPUT_SCAN_Q:
                if (message.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS) {
                    message.type = MESSAGE_EXECUTIVE;
                    message.executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
                    return MESSAGE_DISPATCH_FORWARD;
                }

            default:
                break;
        }
    }

    UpdBottomView(0, 1, 1);
    return MESSAGE_DISPATCH_CONTINUE;
}

// donor PoL RVA 0x000695f7; preferred Buka symbol ?TrimLoopingSounds@advManager@@QAEXH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.593881;margin=0.540898;shape=0.483;size=0.978;calls=1.000;alternate=pol20:void advManager::TrimLoopingSounds(int)@0x000695f7
VA(0x00436a3b, 0x1c2)
void advManager::TrimLoopingSounds(int maxSamples) {
    if (giHighMemBuffer > 0)
        maxSamples += giHighMemBuffer / HIGH_MEMORY_BUFFER_DIVISOR;

    if (maxSamples >= ADVMGR_ENVIRONMENT_SOUND_COUNT)
        return;

    signed char keep[ADVMGR_ENVIRONMENT_SOUND_COUNT];
    int loaded = 0;
    memset(keep, 0, sizeof(keep));

    int i;
    for (i = 0; i < ADVMGR_ACTIVE_SOUND_COUNT; ++i) {
        if (m_activeSounds[i].soundId >= 0
            && m_activeSounds[i].soundId < ADVMGR_ENVIRONMENT_SOUND_COUNT)
            ++keep[m_activeSounds[i].soundId];
    }

    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
        if (keep[i] != 0)
            ++loaded;
    }

    if (loaded < maxSamples) {
        for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
            if (keep[i] == 0 && m_loopingSamples[i] != NULL) {
                ++keep[i];
                ++loaded;
                if (loaded >= maxSamples)
                    goto disposeSamples;
            }
        }
    }

disposeSamples:
    for (i = 0; i < ADVMGR_ENVIRONMENT_SOUND_COUNT; ++i) {
        if (m_loopingSamples[i] != NULL && keep[i] == 0) {
            gpResourceManager->Dispose(m_loopingSamples[i]);
            m_loopingSamples[i] = NULL;
        }
    }
}

// Buka 2.1 advManager::DisableButtons.
VA(0x00436bfd, 0xd0)
void advManager::DisableButtons(void) {
    if (gpAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_CLEAR_FLAGS);
}

VA(0x00436ccd, 0xd0)
void advManager::EnableButtons(void) {
    if (gpAdvManager->m_active != 1)
        return;
    struct tag_message message;
    SET_ADVENTURE_BUTTON_FLAGS(message, m_adventureWindow, WIDGET_COMMAND_SET_FLAGS);
}

VA(0x00436d9d, 0x138)
void advManager::SaveAdventureBorder(void) {
    if (m_adventureBorder != NULL)
        return;

    m_adventureBorder = static_cast<unsigned char*>(malloc(BORDER_BUFFER_SIZE));
    unsigned char* savedPixels = m_adventureBorder;
    signed char* src = gpWindowManager->m_screen->m_pixels;
    int row;
    for (row = 0; row < BORDER_EDGE_SIZE; ++row) {
        memcpy(savedPixels, src, ADVENTURE_VIEWPORT_EXTENT);
        src += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
    for (row = BORDER_EDGE_SIZE; row < BORDER_MIDDLE_END; ++row) {
        memcpy(savedPixels, src, BORDER_SIDE_BYTES);
        memcpy(savedPixels + BORDER_SIDE_BYTES, src + BORDER_MIDDLE_END, BORDER_SIDE_BYTES);
        src += LOGICAL_SCREEN_WIDTH;
        savedPixels += BORDER_SAVED_SIDE_BYTES;
    }
    for (row = BORDER_MIDDLE_END; row < LOGICAL_SCREEN_HEIGHT; ++row) {
        memcpy(savedPixels, src, ADVENTURE_VIEWPORT_EXTENT);
        src += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
}

// donor PoL RVA 0x00069abb; preferred Buka symbol ?DrawAdventureBorder@advManager@@QAEXXZ
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.567997;margin=0.477634;shape=0.397;size=0.978;calls=1.000;alternate=pol20:void advManager::DrawAdventureBorder(void)@0x00069abb
VA(0x00436ed5, 0x134)
void advManager::DrawAdventureBorder(void) {
    unsigned char* savedPixels;
    signed char* dest;
    int row;

    if (m_adventureBorder == NULL)
        return;
    if (gbNoBorder != 0)
        return;

    dest = gpWindowManager->m_screen->m_pixels;
    savedPixels = m_adventureBorder;
    for (row = 0; row < BORDER_EDGE_SIZE; ++row) {
        memcpy(dest, savedPixels, ADVENTURE_VIEWPORT_EXTENT);
        dest += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
    for (row = BORDER_EDGE_SIZE; row < BORDER_MIDDLE_END; ++row) {
        memcpy(dest, savedPixels, BORDER_SIDE_BYTES);
        memcpy(dest + BORDER_MIDDLE_END, savedPixels + BORDER_SIDE_BYTES, BORDER_SIDE_BYTES);
        dest += LOGICAL_SCREEN_WIDTH;
        savedPixels += BORDER_SAVED_SIDE_BYTES;
    }
    for (row = BORDER_MIDDLE_END; row < LOGICAL_SCREEN_HEIGHT; ++row) {
        memcpy(dest, savedPixels, ADVENTURE_VIEWPORT_EXTENT);
        dest += LOGICAL_SCREEN_WIDTH;
        savedPixels += ADVENTURE_VIEWPORT_EXTENT;
    }
}

// ADVMGR owns retail .data 0x0048f828-0x004905b7 and .bss 0x004c4f2c-0x004c50c3.
// Retail emits giCheatSeq, the sand-animation times and giFrameCount among the
// literals of their users.
DATA(0x0048f828)
int giLimitUpdMinX = UPDATE_NONE;
DATA(0x0048f82c)
long iLastScrollTime = 0;
DATA(0x0048f830)
int iSandAnim = 0;
DATA(0x0048f834)
long giLastHourGlassUpdateTime = 0;
DATA(0x0048f838)
int TrigX = 0;
DATA(0x0048f83c)
int TrigY = 0;
DATA(0x0048f840)
int iCurBottomView = BOTTOM_VIEW_NONE;
DATA(0x0048f844)
int iCurBottomViewEnemy = BOTTOM_VIEW_NO_ENEMY;
DATA(0x0048f848)
int iCurHourGlassPhase = 0;
DATA(0x0048f84c)
int iLastHourGlassPhase = 1;
DATA(0x0048f850)
int gbForceUpdate = 0;
DATA(0x004c4f2c)
class heroWindow* cPanel;
DATA(0x004c4f4c)
int giFrameStep;
DATA(0x004c4f50)
char cArmySizeName[12];
DATA(0x004c4f5c)
int giLimitUpdMaxX;
DATA(0x004c4f60)
int giLimitUpdMaxY;
DATA(0x004c4f6c)
signed char bPrefsChanged;
DATA(0x004c4f74)
int giLimitUpdMinY;
DATA(0x004c4f78)
signed char bComboDraw[17][17];
DATA(0x004c50a0)
signed char bFreshSave;
DATA(0x004c50c0)
int iLastAnimFrame;
// ADVMGR's .rdata: ambient-sound volume by distance (0x0048c390).
DATA(0x0048c390)
const long glEnvironmentVolume[5] = {64, 48, 32, 16, 10};
