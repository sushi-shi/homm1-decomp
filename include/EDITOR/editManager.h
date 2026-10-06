#ifndef HOMM1_EDITOR_EDITMANAGER_H
#define HOMM1_EDITOR_EDITMANAGER_H

// The scenario editor's main manager (Editor\EDITMGR.CPP). EDITOR.CPP
// allocates one (InitMainClasses: 0x2546d bytes) and runs it as the only
// executive manager. The class name is descriptive: the retail strings give
// none. Members are recovered from the constructor and the editor units read
// so far; spans no recovered code reads yet are m_unknown<offset>.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>

class font;
class heroWindow;
struct tag_message;

H1_ENUM_CONST_BEGIN(EditManagerConstant)
    EDIT_MANAGER_NO_TOOL = -1,
    // The editor edits one map of this many cells per side.
    EDIT_MANAGER_MAP_SIZE = 72
H1_ENUM_CONST_END(EditManagerConstant)

#pragma pack(push, 1)
// One cell of the edited map (the constructor's map reset writes every byte).
struct editMapCell {
    u8 ground;
    u8 unknown1;
    u8 unknown2;
    u8 unknown3;
    u8 unknown4;
    u8 unknown5;
    u8 unknown6;
    u8 unknown7;
    u8 unknown8;
    u8 unknown9;
};

// A per-cell word pair the map reset clears.
struct editMapCellPair {
    i16 first;
    i16 second;
};

class editManager : public baseManager {
public:
    // The selected tool (-1 none); SelectTool replaces the tool's manager.
    i16 m_tool;
    u8 m_unknown32[4];
    // The bottom bar's status text is drawn with this font.
    font* m_statusFont;
    u8 m_unknown3a[0xc8];
    i8 m_unknown102;
    // clearManager sets it after changing the map.
    i16 m_mapChanged;
    i16 m_unknown105;
    i16 m_unknown107;
    i16 m_unknown109;
    // The widget id of the last tool command (-1 none).
    i16 m_lastCommandId;
    i16 m_unknown10d;
    // The executive manager of the selected tool.
    baseManager* m_toolManager;
    heroWindow* m_window;
    editMapCell m_cells[EDIT_MANAGER_MAP_SIZE][EDIT_MANAGER_MAP_SIZE];
    editMapCellPair m_cellPairs[EDIT_MANAGER_MAP_SIZE][EDIT_MANAGER_MAP_SIZE];
    u8 m_unknown11c97[0x11b80];
    i32 m_unknown23817;
    u8 m_unknown2381b[0x1c38];
    // The map cell shown at the view's top-left corner.
    i16 m_viewX;
    i16 m_viewY;
    // The map cell under the cursor.
    i16 m_cursorX;
    i16 m_cursorY;
    u8 m_unknown2545b[0x10];
    i16 m_dispatchMask;

    editManager(void);
    void SelectTool(i16 tool);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(tag_message& message) OVERRIDE;
};
#pragma pack(pop)

extern editManager* gEditManager;

#endif // HOMM1_EDITOR_EDITMANAGER_H
