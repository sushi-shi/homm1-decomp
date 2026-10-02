#ifndef HOMM1_SOURCE_FILEREQUESTER_H
#define HOMM1_SOURCE_FILEREQUESTER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 12 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;
class heroWindow;
class iconWidget;

// clang-format off
H1_ENUM_CONST_BEGIN(FileRequesterStorageConstant)
    FILE_REQUESTER_NAME_SIZE = 0x15f,
    FILE_REQUESTER_EXTENSION_SIZE = 5,
    FILE_REQUESTER_LOCAL_NAME_SIZE = 352,
    FILE_REQUESTER_LOCAL_EXTENSION_SIZE = 208,
    FILE_REQUESTER_MAP_DESCRIPTION_SIZE = 101
H1_ENUM_CONST_END(FileRequesterStorageConstant)
// clang-format on

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
    signed char difficulty;
    signed char size;
    char description[FILE_REQUESTER_MAP_DESCRIPTION_SIZE];
};

// .MAP header as the requester reads it: 0x554 bytes, id 1000 marks a valid
// map; the name and description offsets are fixed by the constructor.
struct SMapHeader {
    short id;
    signed char difficulty;
    signed char size;
    char name[0x96];
    char description[0x4ba];
};

// PickLoadGame allocates 0x1bc bytes; constructor, Open, Main and Update fix
// the packed members after baseManager.
class fileRequester : public baseManager {
public:
    heroWindow* m_window;
    short m_x;
    short m_y;
    // 0 lists files to load, 1 saves the current game.
    short m_mode;
    FileRequesterName* m_fileNames;
    FileRequesterExtension* m_extensions;
    FileRequesterName* m_mapNames;
    FileRequesterMapInfo* m_mapInfo;
    char m_defaultExtension[FILE_REQUESTER_EXTENSION_SIZE];
    char m_filename[FILE_REQUESTER_NAME_SIZE];
    short m_fileCount;
    short m_topIndex;
    short m_selectedIndex;
    short m_result;
    iconWidget* m_scrollKnob;
    short m_acceptMask;
    // --- constructors ---
    fileRequester(short, short, short, const char*, const char*, const char*);
    ~fileRequester();
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void SetOK(signed char);
    void UpdateMapInfo(void);
    void DoKnob(void);
    void Update(signed char);
    char* GetMapName(void);
    char* GetFilename(void);
    void ShowMapInfo(void);
};
#pragma pack(pop)

// GetMap raises gbShowMapInfo around its .MAP requester and owns the
// reqextra.bin side window the requester fills.
extern signed char gbShowMapInfo;
extern heroWindow* gpReqExtraWindow;
extern char gcCurMapName[];
// Set while the default extension is a saved-game one (".G??").
extern signed char gbRequestingGames;
extern char gLastFilename[];
extern char gLastMapName[];
extern char* cFRDummy;
extern char* gMapSizeNames[];
extern char* gDifficultyNames[];
extern int giMapSize;
extern int giMapDifficulty;

#endif // HOMM1_SOURCE_FILEREQUESTER_H
