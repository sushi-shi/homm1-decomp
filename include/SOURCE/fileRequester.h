#ifndef HOMM1_SOURCE_FILEREQUESTER_H
#define HOMM1_SOURCE_FILEREQUESTER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>

struct tag_message;
class heroWindow;
class iconWidget;

enum FileRequesterStorageConstant {
    FILE_REQUESTER_NAME_SIZE = 0x15f,
    FILE_REQUESTER_EXTENSION_SIZE = 5,
    FILE_REQUESTER_LOCAL_NAME_SIZE = 352,
    FILE_REQUESTER_LOCAL_EXTENSION_SIZE = 208,
    FILE_REQUESTER_MAP_DESCRIPTION_SIZE = 101,
    FILE_REQUESTER_UPDATE_STORAGE_SIZE = 372
};

enum FileRequesterMode {
    FILE_REQUESTER_LOAD = 0,
    FILE_REQUESTER_SAVE = 1
};

enum FileRequesterSelectionConstant {
    FILE_REQUESTER_SELECTION_NONE = -1,
    FILE_REQUESTER_MAP_INFO_NONE = -2,
    FILE_REQUESTER_EXTENSION_PLAYER_DIGIT = 3,
    FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH = 2
};

enum FileRequesterScrollGeometry {
    FILE_REQUESTER_ROW_TEXT_WIDTH = 207,
    FILE_REQUESTER_PLAYER_SUFFIX_GAP = 6,
    FILE_REQUESTER_GUTTER_TOP = 56,
    FILE_REQUESTER_GUTTER_BOTTOM = 212,
    FILE_REQUESTER_SCROLL_KNOB_HALF_HEIGHT = 9,
    FILE_REQUESTER_GUTTER_SCALE = 100,
    FILE_REQUESTER_GUTTER_STEPS = 15700
};

enum MapHeaderConstant {
    MAP_HEADER_ID = 1000,
    MAP_EXTRA_VERSION = 1112
};

enum MapSize {
    MAP_SIZE_SMALL = 0,
    MAP_SIZE_MEDIUM = 1,
    MAP_SIZE_LARGE = 2
};

enum MapDifficulty {
    MAP_DIFFICULTY_EASY = 0,
    MAP_DIFFICULTY_NORMAL = 1,
    MAP_DIFFICULTY_TOUGH = 2,
    MAP_DIFFICULTY_IMPOSSIBLE = 3,
    MAP_DIFFICULTY_FORGET_IT = 4
};

enum FileRequesterControlId {
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
};

enum FileRequesterListConstant {
    FILE_REQUESTER_VISIBLE_ROWS = 10,
    FILE_REQUESTER_LAST_ROW_OFFSET = 9,
    FILE_REQUESTER_DISPATCH_MASK = 0x32f,
    FILE_REQUESTER_FILENAME_MAX_LENGTH = 255
};

struct FileRequesterName {
    char text[FILE_REQUESTER_NAME_SIZE];
};

struct FileRequesterExtension {
    char text[FILE_REQUESTER_EXTENSION_SIZE];
};

#pragma pack(push, 1)
struct FileRequesterMapInfo {
    i8 difficulty;
    i8 size;
    char description[FILE_REQUESTER_MAP_DESCRIPTION_SIZE];
};

struct SMapHeader {
    i16 id;
    i8 difficulty;
    i8 size;
    char name[0x96];
    char description[0x4ba];
};

class fileRequester : public baseManager {
public:
    heroWindow* m_window;
    i16 m_x;
    i16 m_y;
    i16 m_mode;
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
    fileRequester(
        i16 x,
        i16 y,
        i16 mode,
        const char* pattern,
        const char* directory,
        const char* defaultExtension
    );
    ~fileRequester();
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void SetOK(i8 enabled);
    void UpdateMapInfo(void);
    void DoKnob(void);
    void Update(i8 drawWindow);
    char* GetMapName(void);
    char* GetFilename(void);
    void ShowMapInfo(void);
};
#pragma pack(pop)

extern i8 gRequestingGames;
extern char* gFRDummy;

#endif
