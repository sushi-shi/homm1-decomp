#ifndef HOMM1_SOURCE_FILEREQUESTER_H
#define HOMM1_SOURCE_FILEREQUESTER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;
class heroWindow;
class iconWidget;

H1_ENUM_CONST_BEGIN(FileRequesterStorageConstant)
    FILE_REQUESTER_NAME_SIZE = 0x15f,
    FILE_REQUESTER_EXTENSION_SIZE = 5,
    FILE_REQUESTER_LOCAL_NAME_SIZE = 352,
    // Size of the constructor's unused name buffer.
    FILE_REQUESTER_UNUSED_NAME_SIZE = 200,
    FILE_REQUESTER_MAP_DESCRIPTION_SIZE = 101,
    FILE_REQUESTER_UPDATE_STORAGE_SIZE = 372
H1_ENUM_CONST_END(FileRequesterStorageConstant)

// fileRequester::m_mode (the constructor's mode): pick a game or map to load,
// or name the game to save.
H1_ENUM_BEGIN(FileRequesterMode)
    FILE_REQUESTER_LOAD = 0,
    FILE_REQUESTER_SAVE = 1
H1_ENUM_END(FileRequesterMode)

// m_selectedIndex with no list row picked; m_result's "no map info shown
// yet" start value and the player-count digit of a ".GM4" extension (the
// debug level that lets a game load with another player count is KB.h
// DebugLevel).
H1_ENUM_CONST_BEGIN(FileRequesterSelectionConstant)
    FILE_REQUESTER_SELECTION_NONE = -1,
    FILE_REQUESTER_MAP_INFO_NONE = -2,
    FILE_REQUESTER_EXTENSION_PLAYER_DIGIT = 3,
    // EDITOR.EXE's ShowThisMap hides a map whose name starts with one of the
    // shipped scenario or campaign prefixes (this many characters).
    FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH = 4
H1_ENUM_CONST_END(FileRequesterSelectionConstant)

// The list rows' text width, the gutter the scroll knob travels (56..212
// with the knob centred at 134 for a short list) and the click-to-page
// arithmetic.
H1_ENUM_CONST_BEGIN(FileRequesterScrollGeometry)
    FILE_REQUESTER_ROW_TEXT_WIDTH = 207,
    FILE_REQUESTER_PLAYER_SUFFIX_GAP = 6,
    FILE_REQUESTER_GUTTER_TOP = 56,
    FILE_REQUESTER_GUTTER_BOTTOM = 212,
    FILE_REQUESTER_SCROLL_KNOB_HALF_HEIGHT = 9,
    FILE_REQUESTER_GUTTER_SCALE = 100,
    FILE_REQUESTER_GUTTER_STEPS = 15700
H1_ENUM_CONST_END(FileRequesterScrollGeometry)

// SMapHeader::id of a valid .MAP (game::LoadMap also reads it as the map
// version; versions from MAP_EXTRA_VERSION carry map-extra records).
H1_ENUM_CONST_BEGIN(MapHeaderConstant)
    MAP_HEADER_ID = 1000,
    MAP_EXTRA_VERSION = 1112,
    MAP_HEADER_NAME_COUNT = 10,
    MAP_HEADER_NAME_SIZE = 15,
    // The map's name and description in each of eight languages (the
    // requester shows the first).
    MAP_HEADER_LANGUAGE_COUNT = 8,
    MAP_HEADER_DESCRIPTION_SIZE = 121
H1_ENUM_CONST_END(MapHeaderConstant)

// SMapHeader::size, gMapSize and game::m_mapSize: retail gMapSizeNames
// ("Small", "Medium", "Large"); CalcDifficultyRating scores them.
H1_ENUM_BEGIN(MapSize)
    MAP_SIZE_SMALL = 0,
    MAP_SIZE_MEDIUM = 1,
    MAP_SIZE_LARGE = 2
H1_ENUM_END(MapSize)

// SMapHeader::difficulty, gMapDifficulty and game::m_mapDifficulty: retail
// gMapDifficultyNames ("Easy", "Normal", "Tough", "Impossible",
// "Forget It"); CalcDifficultyRating scores the first four.
H1_ENUM_BEGIN(MapDifficulty)
    MAP_DIFFICULTY_EASY = 0,
    MAP_DIFFICULTY_NORMAL = 1,
    MAP_DIFFICULTY_TOUGH = 2,
    MAP_DIFFICULTY_IMPOSSIBLE = 3,
    MAP_DIFFICULTY_FORGET_IT = 4
H1_ENUM_END(MapDifficulty)

// request.bin widget ids: the scroll arrows, gutter and knob, the ten list
// rows from LIST_FIRST, the filename entry and its prompt, and the map-info
// window's size, level and description fields; OK/CANCEL are the dialog role
// buttons.
H1_ENUM_BEGIN(FileRequesterControlId)
    FILE_REQUESTER_OK = DIALOG_BUTTON_2,
    FILE_REQUESTER_CANCEL = DIALOG_BUTTON_1,
    FILE_REQUESTER_SCROLL_UP = 1,
    FILE_REQUESTER_SCROLL_DOWN = 2,
    FILE_REQUESTER_SCROLL_GUTTER = 3,
    FILE_REQUESTER_LIST_FIRST = 4,
    FILE_REQUESTER_SCROLL_KNOB = 14,
    FILE_REQUESTER_FILENAME_ENTRY = 15,
    FILE_REQUESTER_FILENAME_LABEL = 16,
    FILE_REQUESTER_MAP_SIZE = 100,
    FILE_REQUESTER_MAP_LEVEL = 101,
    FILE_REQUESTER_MAP_DESCRIPTION = 102
H1_ENUM_END(FileRequesterControlId)

H1_ENUM_CONST_BEGIN(FileRequesterListConstant)
    FILE_REQUESTER_VISIBLE_ROWS = 10,
    FILE_REQUESTER_LAST_ROW_OFFSET = 9,
    FILE_REQUESTER_DISPATCH_MASK = 0x32f,
    FILE_REQUESTER_FILENAME_MAX_LENGTH = 255
H1_ENUM_CONST_END(FileRequesterListConstant)

struct FileRequesterName {
    char text[FILE_REQUESTER_NAME_SIZE];
};

struct FileRequesterExtension {
    char text[FILE_REQUESTER_EXTENSION_SIZE];
};

// The map-info requester copies the header's difficulty, size and
// description into one 103-byte record per listed map.
#pragma pack(push, 1)
struct FileRequesterMapInfo {
    i8 difficulty;
    i8 size;
    char description[FILE_REQUESTER_MAP_DESCRIPTION_SIZE];
};

// .MAP header: 0x554 bytes, id 1000 marks a valid map. A name and a
// description per language (the requester shows the first; the editor fills
// eight of each), and the editor's maps end the header with the format word
// (1000 for the editor's own format; maps without it are two bytes shorter).
// No code reads the two name slots after the languages' or the bytes between
// the descriptions and the format.
struct SMapHeader {
    i16 id;
    i8 difficulty;
    i8 size;
    char name[MAP_HEADER_NAME_COUNT][MAP_HEADER_NAME_SIZE];
    char description[MAP_HEADER_LANGUAGE_COUNT][MAP_HEADER_DESCRIPTION_SIZE];
    u8 unknown462[0xf0];
    i16 format;
};

// PickLoadGame allocates 0x1bc bytes; constructor, Open, Main and Update fix
// the packed members after baseManager.
class fileRequester : public baseManager {
public:
    heroWindow* m_window;
    i16 m_x;
    i16 m_y;
    // 0 lists files to load, 1 saves the current game.
    H1_ENUM_STORAGE(FileRequesterMode, i16) m_mode;
    FileRequesterName* m_fileNames;
    FileRequesterExtension* m_extensions;
    FileRequesterName* m_mapNames;
    FileRequesterMapInfo* m_mapInfo;
    char m_defaultExtension[FILE_REQUESTER_EXTENSION_SIZE];
    char m_filename[FILE_REQUESTER_NAME_SIZE];
    i16 m_fileCount;
    i16 m_topIndex;
    i16 m_selectedIndex;
    i16 m_result;
    iconWidget* m_scrollKnob;
    i16 m_acceptMask;
    // --- constructors ---
    fileRequester(
        i16 x,
        i16 y,
        H1_ENUM_PARAM(FileRequesterMode, i16) mode,
        const char* pattern,
        const char* directory,
        const char* defaultExtension
    );
    ~fileRequester();
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void SetOK(i8 enabled);
    void UpdateMapInfo(void);
    void DoKnob(void);
    void Update(i8 drawWindow);
    char* GetMapName(void);
    char* GetFilename(void);
    void ShowMapInfo(void);
};
#pragma pack(pop)

// Set while the default extension is a saved-game one (".G??").
extern i8 gRequestingGames;
extern char* gFRDummy;

#endif // HOMM1_SOURCE_FILEREQUESTER_H
