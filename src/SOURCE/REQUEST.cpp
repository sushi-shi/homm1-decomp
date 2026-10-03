// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/widget.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/soundmgr.h>
#include <SOURCE/kbwin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

// Buka 2.1 ShowThisMap; HoMM1 keeps an unreachable rejecting return.
VA(0x00467e00, 0x1c)
i32 ShowThisMap(char*) {
    return 1;
    return 0;
}

// Buka 2.1 constructor with InitializeFiles folded in: HoMM1 counts and sorts
// every match of the pattern and reads .MAP headers only for GetMap's list.
VA(0x00467e1c, 0x825)
fileRequester::fileRequester(
    i16 x,
    i16 y,
    H1_ENUM_PARAM(FileRequesterMode, i16) mode,
    const char* pattern,
    const char* directory,
    const char* defaultExtension
) {
    HANDLE dirHandle;
    i32 fd;
    char fullPath[412];
    SMapHeader header;
    i32 findResult;
    WIN32_FIND_DATA fileData;
    char* ptr;
    char extStr[FILE_REQUESTER_LOCAL_EXTENSION_SIZE];
    char fileName[FILE_REQUESTER_LOCAL_NAME_SIZE];
    i32 moveValue;
    i32 sortedCount;
    i32 index;

    m_selectedIndex = FILE_REQUESTER_SELECTION_NONE;
    m_fileCount = 0;
    m_topIndex = 0;
    m_fileNames = NULL;
    m_extensions = NULL;
    m_mapNames = NULL;
    m_mapInfo = NULL;
    m_x = x;
    m_y = y;
    strcpy(m_defaultExtension, defaultExtension);
    if (m_defaultExtension[1] == 'G')
        gRequestingGames = 1;
    else
        gRequestingGames = 0;
    m_mode = mode;

    sprintf(gText, "%s%s", directory, pattern);
    m_fileCount = 0;
    dirHandle = FindFirstFile(gText, &fileData);
    if (dirHandle != INVALID_HANDLE_VALUE) {
        if (ShowThisMap(fileData.cFileName))
            m_fileCount++;
        while (FindNextFile(dirHandle, &fileData)) {
            if (ShowThisMap(fileData.cFileName))
                m_fileCount++;
        }
        FindClose(dirHandle);
    }

    m_fileNames = new FileRequesterName[m_fileCount + 1];
    if (!m_fileNames)
        MemError();
    m_extensions = new FileRequesterExtension[m_fileCount + 1];
    if (!m_extensions)
        MemError();
    if (gShowMapInfo) {
        m_mapNames = new FileRequesterName[m_fileCount + 1];
        if (!m_mapNames)
            MemError();
        m_mapInfo = new FileRequesterMapInfo[m_fileCount + 1];
        if (!m_mapInfo)
            MemError();
    }
    for (index = 0; index < m_fileCount; index++) {
        strcpy(m_fileNames[index].text, "");
        strcpy(m_extensions[index].text, "");
    }

    sortedCount = 0;
    sprintf(gText, "%s%s", directory, pattern);
    dirHandle = FindFirstFile(gText, &fileData);
    if (dirHandle != INVALID_HANDLE_VALUE) {
        findResult = 1;
        while (findResult) {
            if (ShowThisMap(fileData.cFileName)) {
                strcpy(fileName, fileData.cFileName);
                ptr = FindLastToken(fileName, '.');
                if (ptr) {
                    strcpy(extStr, ptr);
                    *ptr = 0;
                }
                for (index = 0; index < sortedCount; index++) {
                    if (strcmpi(fileName, m_fileNames[index].text) < 0) {
                        for (moveValue = sortedCount; moveValue > index; moveValue--) {
                            strcpy(m_fileNames[moveValue].text, m_fileNames[moveValue - 1].text);
                            strcpy(m_extensions[moveValue].text, m_extensions[moveValue - 1].text);
                        }
                        goto insert;
                    }
                }
            insert:
                strcpy(m_fileNames[index].text, fileName);
                strcpy(m_extensions[index].text, extStr);
                sortedCount++;
            }
            findResult = FindNextFile(dirHandle, &fileData);
        }
        FindClose(dirHandle);
    }

    if (gShowMapInfo) {
        for (index = 0; index < sortedCount; index++) {
            sprintf(
                fullPath,
                "%s%s%s",
                directory,
                m_fileNames[index].text,
                m_extensions[index].text
            );
            fd = open(fullPath, O_BINARY);
            if (fd == -1)
                FileError(fullPath);
            read(fd, &header, sizeof(header));
            if (header.id == MAP_HEADER_ID) {
                strcpy(m_mapNames[index].text, header.name);
                strcpy(m_mapInfo[index].description, header.description);
                m_mapInfo[index].difficulty = header.difficulty;
                m_mapInfo[index].size = header.size;
            } else {
                strcpy(m_mapNames[index].text, m_fileNames[index].text);
                strcpy(m_mapInfo[index].description, "");
                m_mapInfo[index].difficulty = MAP_DIFFICULTY_EASY;
                m_mapInfo[index].size = MAP_SIZE_SMALL;
            }
            close(fd);
        }
    }

    KBChangeMenu(hmnuDflt);
    m_acceptMask = FILE_REQUESTER_DISPATCH_MASK;
    m_result = FILE_REQUESTER_MAP_INFO_NONE;
}

VA(0x00468641, 0x1f)
fileRequester::~fileRequester() {}

// Buka 2.1 Close with CleanUpData folded in; HoMM1 also remembers the
// chosen map's title.
VA(0x00468660, 0x12c)
void fileRequester::Close(void) {
    if (!m_active)
        return;
    strcpy(gLastMapName, GetMapName());
    strcpy(gLastFilename, GetFilename());
    if (m_fileNames)
        delete[] m_fileNames;
    if (m_extensions)
        delete[] m_extensions;
    if (m_mapNames)
        delete[] m_mapNames;
    if (m_mapInfo)
        delete[] m_mapInfo;
    gpWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
}

// Buka 2.1 Open without the map-size filter buttons; HoMM1 selects the save
// slot whose extension digit matches the human player count.
VA(0x0046878c, 0x431)
i16 fileRequester::Open(i16 priority) {
    const i16 scrollId = FILE_REQUESTER_SCROLL_KNOB;
    i32 i;
    const i16 promptId = FILE_REQUESTER_FILENAME_LABEL;
    tag_message message;
    i8 enable;
    char* period;

    strcpy(gLastMapName, "");
    strcpy(gLastFilename, "");
    m_window = new heroWindow(m_x, m_y, "request.bin");
    if (!m_window)
        MemError();
    m_scrollKnob = new iconWidget(
        283,
        56,
        8,
        17,
        "scroll.icn",
        4,
        ICON_DRAW_NORMAL,
        scrollId,
        ICON_WIDGET_DRAW,
        1
    );
    if (!m_scrollKnob)
        MemError();
    m_window->AddWidget(m_scrollKnob, WINDOW_Z_ORDER_APPEND);

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    if (m_mode == FILE_REQUESTER_SAVE) {
        enable = 1;
        const i16 textEntryId = FILE_REQUESTER_FILENAME_ENTRY;
        strcpy(m_filename, gpGame->m_saveName);
        period = FindLastToken(m_filename, '.');
        if (period)
            *period = 0;
        message.id = textEntryId;
        message.text = m_filename;
        m_window->BroadcastMessage(message);
        message.id = promptId;
        sprintf(gText, "File to Save:");
        message.text = gText;
        m_window->BroadcastMessage(message);
        for (i = 0; i < m_fileCount; i++) {
            if (!strcmpi(m_fileNames[i].text, m_filename)
                && m_extensions[i].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0'
                       == giNumHumanPlayers)
                m_selectedIndex = i;
        }
    } else {
        enable = 0;
        if (m_mode == FILE_REQUESTER_LOAD && m_defaultExtension[1] == 'M') {
            for (i = 0; i < m_fileCount; i++) {
                if (!strnicmp(m_fileNames[i].text, gMapName, SAVE_FILE_BASE_NAME_LENGTH)) {
                    m_selectedIndex = i;
                    enable = 1;
                }
            }
        }
        message.id = promptId;
        sprintf(gText, "File to Load:");
        message.text = gText;
        m_window->BroadcastMessage(message);
    }
    const i16 nameId = FILE_REQUESTER_FILENAME_ENTRY;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_MAX_LENGTH, nameId);
    message.value = FILE_REQUESTER_FILENAME_MAX_LENGTH;
    m_window->BroadcastMessage(message);
    Update(0);
    if (gShowMapInfo)
        gpWindowManager->AddWindow(gReqExtraWindow, WINDOW_Z_ORDER_APPEND, 1);
    gpWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    SetOK(enable);
    UpdateMapInfo();
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "fileRequester");
    return 0;
}

// Buka 2.1 SetOK with HoMM1's fixed dimming flags.
VA(0x00468bbd, 0x8a)
void fileRequester::SetOK(i8 enabled) {
    tag_message message;

    SET_WIDGET_MESSAGE(
        message,
        enabled ? WIDGET_COMMAND_CLEAR_FLAGS : WIDGET_COMMAND_SET_FLAGS,
        DIALOG_BUTTON_2
    );
    message.value = WIDGET_FLAG_DIMMED;
    m_window->BroadcastMessage(message);
    message.command = enabled ? WIDGET_COMMAND_SET_FLAGS : WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED;
    m_window->BroadcastMessage(message);
}

// Buka 2.1 Main without the map-size filter; HoMM1 checks a saved game's
// human count, encoded as its extension digit, before accepting it.
VA(0x00468c47, 0xa86)
i16 fileRequester::Main(tag_message& message) {
    i32 newTop;
    i32 stepSize;
    i32 numPages;
    const i16 arrowUpId = FILE_REQUESTER_SCROLL_UP;
    tag_message msg;
    const i16 downArrowId = FILE_REQUESTER_SCROLL_DOWN;
    const i16 railId = FILE_REQUESTER_SCROLL_GUTTER;
    const i16 firstRowId = FILE_REQUESTER_LIST_FIRST;
    const i16 scrollerId = FILE_REQUESTER_SCROLL_KNOB;
    const i16 nameId = FILE_REQUESTER_FILENAME_ENTRY;
    i16 ch;
    i16 len;
    i32 finished;
    i16 ptrY;
    i16 ptrX;
    char fileName[FILE_REQUESTER_LOCAL_NAME_SIZE];

    finished = 0;
    if (!(message.type & m_acceptMask)) {
        if (message.type) {
            message.type = MESSAGE_NONE;
            return MESSAGE_DISPATCH_FORWARD;
        }
        return MESSAGE_DISPATCH_CONTINUE;
    }
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        return MESSAGE_DISPATCH_CONSUME;

    switch (message.type) {
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_NUMPAD_8:
                    if (m_selectedIndex > 0) {
                        m_selectedIndex--;
                        if (m_topIndex > m_selectedIndex)
                            m_topIndex--;
                        Update(1);
                    }
                    break;
                case INPUT_SCAN_NUMPAD_2:
                    if (m_selectedIndex < m_fileCount - 1) {
                        m_selectedIndex++;
                        if (m_topIndex + FILE_REQUESTER_VISIBLE_ROWS <= m_selectedIndex)
                            m_topIndex++;
                        Update(1);
                    }
                    break;
            }
            break;
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case DIALOG_BUTTON_2:
                            if (m_selectedIndex == FILE_REQUESTER_SELECTION_NONE
                                && !m_filename[0]) {
                                NormalDialog(
                                    "Please make a selection from the list, or press cancel.",
                                    NORMAL_DIALOG_TYPE_OK,
                                    -1,
                                    -1,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_OR_TEXT
                                );
                                break;
                            } else {
                                message.value = message.id;
                                finished = 1;
                            }
                            break;
                        case DIALOG_BUTTON_1:
                            message.value = message.id;
                            finished = 1;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case nameId:
                            SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_GET_TEXT, nameId);
                            m_window->BroadcastMessage(msg);
                            memset(fileName, 0, 9);
                            strcpy(fileName, msg.text);
                            len = strlen(fileName);
                            for (ch = 0; ch < len; ch++) {
                                if ((fileName[ch] < 'A' || fileName[ch] > 'Z')
                                    && (fileName[ch] < 'a' || fileName[ch] > 'z')
                                    && (fileName[ch] < '0' || fileName[ch] > '9')
                                    && fileName[ch] != '_' && fileName[ch] != ' '
                                    && !FindToken("$%'-_@~`!(){}^#&+,;=[].", fileName[ch]))
                                    fileName[ch] = 0;
                            }
                            for (ch = strlen(fileName) - 1; ch >= 0; ch--) {
                                if (fileName[ch] == ' ')
                                    fileName[ch] = 0;
                                else
                                    ch = -1;
                            }
                            if (strlen(fileName) && fileName[0] > ' ') {
                                m_selectedIndex = FILE_REQUESTER_SELECTION_NONE;
                                strcpy(m_filename, fileName);
                                SetOK(1);
                            }
                            msg.command = WIDGET_COMMAND_SET_TEXT;
                            msg.id = nameId;
                            msg.text = m_filename;
                            m_window->BroadcastMessage(msg);
                            Update(1);
                            break;
                        case arrowUpId:
                            if (m_topIndex > 0) {
                                m_topIndex--;
                                Update(1);
                            }
                            break;
                        case downArrowId:
                            if (m_topIndex + FILE_REQUESTER_VISIBLE_ROWS < m_fileCount) {
                                m_topIndex++;
                                if (m_topIndex + FILE_REQUESTER_LAST_ROW_OFFSET >= m_fileCount)
                                    m_topIndex = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                                Update(1);
                            }
                            break;
                        case railId:
                            numPages = m_fileCount - FILE_REQUESTER_LAST_ROW_OFFSET;
                            if (numPages < 1)
                                numPages = 1;
                            stepSize = FILE_REQUESTER_GUTTER_STEPS / numPages;
                            gpMouseManager->MouseCoords(ptrX, ptrY);
                            ptrY -= m_y + FILE_REQUESTER_GUTTER_TOP;
                            ptrY -= FILE_REQUESTER_SCROLL_KNOB_HALF_HEIGHT;
                            newTop = ptrY * FILE_REQUESTER_GUTTER_SCALE / stepSize;
                            m_topIndex = newTop;
                            if (m_topIndex + FILE_REQUESTER_LAST_ROW_OFFSET >= m_fileCount)
                                m_topIndex = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                            if (m_topIndex < 0)
                                m_topIndex = 0;
                            Update(1);
                            break;
                        case scrollerId:
                            DoKnob();
                            break;
                        case firstRowId:
                        case firstRowId + 1:
                        case firstRowId + 2:
                        case firstRowId + 3:
                        case firstRowId + 4:
                        case firstRowId + 5:
                        case firstRowId + 6:
                        case firstRowId + 7:
                        case firstRowId + 8:
                        case firstRowId + 9:
                            if (message.id - firstRowId + m_topIndex == m_selectedIndex) {
                                message.value = DIALOG_BUTTON_2;
                                message.id = DIALOG_BUTTON_2;
                                finished = 1;
                                break;
                            }
                            m_selectedIndex = message.id - firstRowId + m_topIndex;
                            if (m_selectedIndex >= m_fileCount) {
                                m_selectedIndex = FILE_REQUESTER_SELECTION_NONE;
                                SetOK(0);
                            } else {
                                SetOK(1);
                            }
                            Update(1);
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    if (finished == 1) {
        if (gCampaignChoice <= 0 && m_mode == FILE_REQUESTER_LOAD && m_selectedIndex >= 0
            && gRequestingGames && message.value != FILE_REQUESTER_CANCEL) {
            ch = m_extensions[m_selectedIndex].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0';
            if (ch < giNumHumanPlayers
                && giDebugLevel < FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH) {
                sprintf(
                    gText,
                    "The game you have chosen only has slots for %d human(s).  You need one "
                    "with room for at least %d humans.",
                    ch,
                    giNumHumanPlayers
                );
                NormalDialog(
                    gText,
                    NORMAL_DIALOG_TYPE_OK,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                finished = 0;
            }
            if (ch > giNumHumanPlayers) {
                sprintf(
                    gText,
                    "The game you have chosen was being played with %d humans. Is it OK if the "
                    "computer takes the place of the last %d human(s)?",
                    ch,
                    ch - giNumHumanPlayers
                );
                NormalDialog(
                    gText,
                    NORMAL_DIALOG_TYPE_YES_NO,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                if (gpWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                    finished = 0;
            }
        }
        if (finished) {
            message.type = MESSAGE_EXECUTIVE;
            message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    UpdateMapInfo();
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004696cd, 0x9d)
void fileRequester::UpdateMapInfo(void) {
    if (m_selectedIndex != m_result && gShowMapInfo) {
        if (m_selectedIndex >= 0)
            strcpy(gCurMapName, m_fileNames[m_selectedIndex].text);
        else
            strcpy(gCurMapName, "");
        ShowMapInfo();
    }
}

// GetMapName/GetFilename's no-selection result; data coverage is deferred.
DATA(0x004a2824)
char* gFRDummy = "";

// Buka 2.1 DoKnob with HoMM1's ten-row list and 156-pixel gutter.
VA(0x0046976a, 0x280)
void fileRequester::DoKnob(void) {
    i32 lastTop;
    i16 index;
    double scale;
    tag_message event;
    i16 x;
    i16 my;
    i16 offset;

    gpMouseManager->SetCursorShape(4);
    lastTop = m_topIndex;
    scale = 156.0 / (m_fileCount - FILE_REQUESTER_LAST_ROW_OFFSET);
    gpMouseManager->MouseCoords(x, my);
    offset = my - m_scrollKnob->m_y;
    gpInputManager->Flush();
    event = gpInputManager->GetEvent();
    while (event.type != MESSAGE_LEFT_BUTTON_UP && event.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (event.type == MESSAGE_MOUSE_MOVE) {
            if (offset + FILE_REQUESTER_GUTTER_TOP > event.y)
                event.y = offset + FILE_REQUESTER_GUTTER_TOP;
            if (offset + FILE_REQUESTER_GUTTER_BOTTOM < event.y)
                event.y = offset + FILE_REQUESTER_GUTTER_BOTTOM;
            gpMouseManager->Main(event);
            m_scrollKnob->m_y = event.y - offset;
            if (m_fileCount > FILE_REQUESTER_VISIBLE_ROWS) {
                index = static_cast<i16>((m_scrollKnob->m_y - FILE_REQUESTER_GUTTER_TOP) / scale);
                if (index != lastTop) {
                    if (index > m_fileCount - FILE_REQUESTER_VISIBLE_ROWS)
                        index = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                    if (index < 0)
                        index = 0;
                    m_topIndex = index;
                    Update(0);
                    m_scrollKnob->m_y = event.y - offset;
                    m_window->DrawWindow();
                    lastTop = index;
                } else {
                    m_window->DrawWindow();
                }
            } else {
                m_window->DrawWindow();
            }
        }
        Process1WindowsMessage();
        event = gpInputManager->GetEvent();
    }
    gpMouseManager->SetCursorShape(6);
    m_scrollKnob->m_flags &= ~WIDGET_FLAG_SELECTED;
    Update(1);
}

// Buka 2.1 Update for HoMM1's ten text rows; saved games append their human
// count and map lists show the header title.
VA(0x004699ea, 0x4f1)
void fileRequester::Update(i8 drawWindow) {
    double gutterFactor;
    i32 nHumans;
    i32 showPlayers;
    i32 limit;
    i32 pos;
    const i16 firstId = FILE_REQUESTER_LIST_FIRST;
    const i16 nameId = FILE_REQUESTER_FILENAME_ENTRY;
    char extra[FILE_REQUESTER_UPDATE_STORAGE_SIZE];
    const i16 hiliteColor = 0xe8;
    const i16 textColor = 1;
    tag_message event;
    i32 suffixWidth;
    font* bigFont;
    i16 row;

    event.type = MESSAGE_WIDGET;
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    for (row = 0; row < FILE_REQUESTER_VISIBLE_ROWS; row++) {
        event.id = row + firstId;
        if (m_topIndex + row >= m_fileCount) {
            event.command = WIDGET_COMMAND_CLEAR_FLAGS;
            event.value = WIDGET_FLAG_DRAW;
        } else {
            event.command = WIDGET_COMMAND_SET_FLAGS;
            event.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(event);
            event.command = WIDGET_COMMAND_SET_TEXT;
            if (gShowMapInfo)
                sprintf(gText, "%s", m_mapNames[m_topIndex + row].text);
            else
                sprintf(gText, "%s", m_fileNames[m_topIndex + row].text);
            nHumans =
                m_extensions[m_topIndex + row].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0';
            showPlayers = 0;
            if (nHumans != 1 && gCampaignChoice <= 0 && gRequestingGames) {
                showPlayers = 1;
                sprintf(extra, " (%d %s)", nHumans, "Players");
                suffixWidth = bigFont->LineWidth(extra);
            }
            limit = FILE_REQUESTER_ROW_TEXT_WIDTH;
            if (showPlayers)
                limit -= suffixWidth + FILE_REQUESTER_PLAYER_SUFFIX_GAP;
            pos = strlen(gText);
            while (bigFont->LineWidth(gText) > limit) {
                pos--;
                gText[pos] = 0;
            }
            if (showPlayers)
                strcat(gText, extra);
            event.text = gText;
        }
        m_window->BroadcastMessage(event);
        event.command = WIDGET_COMMAND_SET_COLOR;
        if (m_topIndex + row == m_selectedIndex)
            event.value = hiliteColor;
        else
            event.value = textColor;
        m_window->BroadcastMessage(event);
    }

    event.id = nameId;
    event.command = WIDGET_COMMAND_SET_FLAGS;
    event.value = WIDGET_FLAG_ENABLED;
    m_window->BroadcastMessage(event);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE) {
        event.command = WIDGET_COMMAND_SET_TEXT;
        if (gShowMapInfo)
            sprintf(gText, "%s", m_mapNames[m_selectedIndex].text);
        else
            sprintf(gText, "%s", m_fileNames[m_selectedIndex].text);
        event.text = gText;
        m_window->BroadcastMessage(event);
    }
    if (m_mode == FILE_REQUESTER_LOAD) {
        event.command = WIDGET_COMMAND_CLEAR_FLAGS;
        event.value = WIDGET_FLAG_ENABLED;
        m_window->BroadcastMessage(event);
    }
    if (m_fileCount <= FILE_REQUESTER_VISIBLE_ROWS) {
        m_scrollKnob->m_y = 134;
    } else {
        gutterFactor = 156.0 / (m_fileCount - FILE_REQUESTER_VISIBLE_ROWS);
        m_scrollKnob->m_y = m_topIndex * gutterFactor + 56.0;
    }
    if (drawWindow)
        m_window->DrawWindow();
    gpResourceManager->Dispose(bigFont);
}

VA(0x00469edb, 0x7d)
char* fileRequester::GetMapName(void) {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_fileCount && m_mapNames)
        return m_mapNames[m_selectedIndex].text;
    else
        return gFRDummy;
}

// Buka 2.1 GetFilename for HoMM1's two modes.
VA(0x00469f58, 0x13f)
char* fileRequester::GetFilename(void) {
    if (m_mode != FILE_REQUESTER_SAVE && (m_selectedIndex < 0 || m_selectedIndex >= m_fileCount))
        return gFRDummy;

    if (m_selectedIndex == FILE_REQUESTER_SELECTION_NONE)
        sprintf(gText, "%s%s", m_filename, m_defaultExtension);
    else if (m_mode == FILE_REQUESTER_LOAD)
        sprintf(
            gText,
            "%s%s",
            m_fileNames[m_selectedIndex].text,
            m_extensions[m_selectedIndex].text
        );
    else
        sprintf(gText, "%s%s", m_fileNames[m_selectedIndex].text, m_defaultExtension);
    strcpy(m_filename, gText);
    return m_filename;
}

// Fills GetMap's reqextra.bin window with the selected map's size,
// difficulty and description.
VA(0x0046a097, 0x269)
void fileRequester::ShowMapInfo(void) {
    const i32 sizeId = FILE_REQUESTER_MAP_SIZE;
    const i32 levelId = FILE_REQUESTER_MAP_LEVEL;
    const i32 descriptionId = FILE_REQUESTER_MAP_DESCRIPTION;
    tag_message message;

    sprintf(gText, "");
    message.text = gText;
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        gMapSize = m_mapInfo[m_selectedIndex].size;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, sizeId);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        message.text = gMapSizeNames[m_mapInfo[m_selectedIndex].size];
    gReqExtraWindow->BroadcastMessage(message);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        gMapDifficulty = m_mapInfo[m_selectedIndex].difficulty;
    sprintf(gText, gCurMapName);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, levelId);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        message.text = gMapDifficultyNames[m_mapInfo[m_selectedIndex].difficulty];
    gReqExtraWindow->BroadcastMessage(message);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        strcpy(gFullMapName, m_mapNames[m_selectedIndex].text);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        strcpy(gMapDescription, m_mapInfo[m_selectedIndex].description);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, descriptionId);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        message.text = m_mapInfo[m_selectedIndex].description;
    gReqExtraWindow->BroadcastMessage(message);
    gReqExtraWindow->DrawWindow();
}

// REQUEST owns retail .bss 0x004c5130-0x004c5137.
DATA(0x004cb184)
i8 gRequestingGames;
