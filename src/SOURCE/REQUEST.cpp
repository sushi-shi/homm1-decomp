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
#include <BASE/Misc.h>
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
VA(0x00453b70, 0xa)
i32 ShowThisMap(char*) {
    return 1;
    return 0;
}

// Buka 2.1 constructor with InitializeFiles folded in: HoMM1 counts and sorts
// every match of the pattern and reads .MAP headers only for GetMap's list.
VA(0x00453b7a, 0x7da)
fileRequester::fileRequester(
    i16 x,
    i16 y,
    H1_ENUM_PARAM(FileRequesterMode, i16) mode,
    const char* pattern,
    const char* directory,
    const char* defaultExtension
) {
    char fullFileName[412];
    i32 file;
    SMapHeader headerData;
    i32 found;
    char unusedName[FILE_REQUESTER_UNUSED_NAME_SIZE];
    i32 moveValue;
    i32 entryIndex;
    char extension[FILE_REQUESTER_EXTENSION_SIZE];
    WIN32_FIND_DATA findFileData;
    i32 insertCount;
    char* dotPtr;
    char nameBuffer[FILE_REQUESTER_LOCAL_NAME_SIZE];
    HANDLE findHandleWork;

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
    findHandleWork = FindFirstFile(gText, &findFileData);
    if (findHandleWork != INVALID_HANDLE_VALUE) {
        if (ShowThisMap(findFileData.cFileName))
            m_fileCount++;
        while (FindNextFile(findHandleWork, &findFileData)) {
            if (ShowThisMap(findFileData.cFileName))
                m_fileCount++;
        }
        FindClose(findHandleWork);
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
    for (entryIndex = 0; entryIndex < m_fileCount; entryIndex++) {
        strcpy(m_fileNames[entryIndex].text, "");
        strcpy(m_extensions[entryIndex].text, "");
    }

    insertCount = 0;
    sprintf(gText, "%s%s", directory, pattern);
    findHandleWork = FindFirstFile(gText, &findFileData);
    if (findHandleWork != INVALID_HANDLE_VALUE) {
        found = 1;
        while (found) {
            if (ShowThisMap(findFileData.cFileName)) {
                strcpy(nameBuffer, findFileData.cFileName);
                dotPtr = FindLastToken(nameBuffer, '.');
                if (dotPtr) {
                    strcpy(extension, dotPtr);
                    *dotPtr = 0;
                }
                for (entryIndex = 0; entryIndex < insertCount; entryIndex++) {
                    if (strcmpi(nameBuffer, m_fileNames[entryIndex].text) < 0) {
                        for (moveValue = insertCount; moveValue > entryIndex; moveValue--) {
                            strcpy(m_fileNames[moveValue].text, m_fileNames[moveValue - 1].text);
                            strcpy(m_extensions[moveValue].text, m_extensions[moveValue - 1].text);
                        }
                        goto insert;
                    }
                }
            insert:
                strcpy(m_fileNames[entryIndex].text, nameBuffer);
                strcpy(m_extensions[entryIndex].text, extension);
                insertCount++;
            }
            found = FindNextFile(findHandleWork, &findFileData);
        }
        FindClose(findHandleWork);
    }

    if (gShowMapInfo) {
        for (entryIndex = 0; entryIndex < insertCount; entryIndex++) {
            sprintf(
                fullFileName,
                "%s%s%s",
                directory,
                m_fileNames[entryIndex].text,
                m_extensions[entryIndex].text
            );
            file = open(fullFileName, O_BINARY);
            if (file == -1)
                FileError(fullFileName);
            READ_FILE_VALUE(file, headerData);
            if (headerData.id == MAP_HEADER_ID) {
                strcpy(m_mapNames[entryIndex].text, headerData.name);
                strcpy(m_mapInfo[entryIndex].description, headerData.description);
                m_mapInfo[entryIndex].difficulty = headerData.difficulty;
                m_mapInfo[entryIndex].size = headerData.size;
            } else {
                strcpy(m_mapNames[entryIndex].text, m_fileNames[entryIndex].text);
                strcpy(m_mapInfo[entryIndex].description, "");
                m_mapInfo[entryIndex].difficulty = MAP_DIFFICULTY_EASY;
                m_mapInfo[entryIndex].size = MAP_SIZE_SMALL;
            }
            close(file);
        }
    }

    KBChangeMenu(hmnuDflt);
    m_acceptMask = FILE_REQUESTER_DISPATCH_MASK;
    m_result = FILE_REQUESTER_MAP_INFO_NONE;
}

VA(0x00454354, 0x14)
fileRequester::~fileRequester() {}

// Buka 2.1 Close with CleanUpData folded in; HoMM1 also remembers the
// chosen map's title.
VA(0x00454368, 0xf1)
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
VA(0x00454459, 0x3cd)
i16 fileRequester::Open(i16 priority) {
    const i16 scrollKnobId = FILE_REQUESTER_SCROLL_KNOB;
    tag_message message;
    i32 i;
    const i16 nameLabelId = FILE_REQUESTER_FILENAME_LABEL;
    char* dotPtr;
    i8 enable;

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
        scrollKnobId,
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
        dotPtr = FindLastToken(m_filename, '.');
        if (dotPtr)
            *dotPtr = 0;
        message.id = textEntryId;
        message.text = m_filename;
        m_window->BroadcastMessage(message);
        message.id = nameLabelId;
        sprintf(gText, localization::Tr("file.save.label"));
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
        message.id = nameLabelId;
        sprintf(gText, localization::Tr("file.load.label"));
        message.text = gText;
        m_window->BroadcastMessage(message);
    }
    const i16 entryId = FILE_REQUESTER_FILENAME_ENTRY;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_MAX_LENGTH, entryId);
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
VA(0x00454826, 0x67)
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
VA(0x0045488d, 0xa9c)
i16 fileRequester::Main(tag_message& message) {
    i32 firstShown;
    i32 stepSize;
    i32 pageCount;
    const i16 arrowUpId = FILE_REQUESTER_SCROLL_UP;
    tag_message reply;
    i16 length;
    i16 key;
    i32 handled;
    i16 ptrY;
    i16 ptrX;
    char nameBuffer[FILE_REQUESTER_LOCAL_NAME_SIZE];
    const i16 downId = FILE_REQUESTER_SCROLL_DOWN;
    const i16 scrollBarId = FILE_REQUESTER_SCROLL_GUTTER;
    const i16 firstItemId = FILE_REQUESTER_LIST_FIRST;
    const i16 scrollerId = FILE_REQUESTER_SCROLL_KNOB;
    const i16 fileNameId = FILE_REQUESTER_FILENAME_ENTRY;

    handled = 0;
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
                                    localization::Tr("file.selection.required"),
                                    NORMAL_DIALOG_TYPE_OK
                                );
                                break;
                            } else {
                                message.value = message.id;
                                handled = 1;
                            }
                            break;
                        case DIALOG_BUTTON_1:
                            message.value = message.id;
                            handled = 1;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case fileNameId:
                            SET_WIDGET_MESSAGE(reply, WIDGET_COMMAND_GET_TEXT, fileNameId);
                            m_window->BroadcastMessage(reply);
                            memset(nameBuffer, 0, 9);
                            strcpy(nameBuffer, reply.text);
                            length = strlen(nameBuffer);
                            for (key = 0; key < length; key++) {
                                if ((static_cast<u8>(nameBuffer[key]) < 'A'
                                     || static_cast<u8>(nameBuffer[key]) > 'Z')
                                    && (static_cast<u8>(nameBuffer[key]) < 'a'
                                        || static_cast<u8>(nameBuffer[key]) > 'z')
                                    && (static_cast<u8>(nameBuffer[key]) < '0'
                                        || static_cast<u8>(nameBuffer[key]) > '9')
                                    && (static_cast<u8>(nameBuffer[key]) < CYRILLIC_CAPITAL_A
                                        || static_cast<u8>(nameBuffer[key]) > CYRILLIC_CAPITAL_YA)
                                    && (static_cast<u8>(nameBuffer[key]) < CYRILLIC_SMALL_A
                                        || static_cast<u8>(nameBuffer[key]) > CYRILLIC_SMALL_YA)
                                    && static_cast<u8>(nameBuffer[key]) != CYRILLIC_CAPITAL_YO
                                    && static_cast<u8>(nameBuffer[key]) != CYRILLIC_SMALL_YO
                                    && static_cast<u8>(nameBuffer[key]) != '_'
                                    && static_cast<u8>(nameBuffer[key]) != ' '
                                    && !FindToken("$%'-_@~`!(){}^#&+,;=[].", nameBuffer[key]))
                                    nameBuffer[key] = 0;
                            }
                            for (key = strlen(nameBuffer) - 1; key >= 0; key--) {
                                if (static_cast<u8>(nameBuffer[key]) == ' ')
                                    nameBuffer[key] = 0;
                                else
                                    key = -1;
                            }
                            if (strlen(nameBuffer) > 0 && static_cast<u8>(nameBuffer[0]) > ' ') {
                                m_selectedIndex = FILE_REQUESTER_SELECTION_NONE;
                                strcpy(m_filename, nameBuffer);
                                SetOK(1);
                            }
                            reply.command = WIDGET_COMMAND_SET_TEXT;
                            reply.id = fileNameId;
                            reply.text = m_filename;
                            m_window->BroadcastMessage(reply);
                            Update(1);
                            break;
                        case arrowUpId:
                            if (m_topIndex > 0) {
                                m_topIndex--;
                                Update(1);
                            }
                            break;
                        case downId:
                            if (m_topIndex + FILE_REQUESTER_VISIBLE_ROWS < m_fileCount) {
                                m_topIndex++;
                                if (m_topIndex + FILE_REQUESTER_LAST_ROW_OFFSET >= m_fileCount)
                                    m_topIndex = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                                Update(1);
                            }
                            break;
                        case scrollBarId:
                            pageCount = m_fileCount - FILE_REQUESTER_LAST_ROW_OFFSET;
                            if (pageCount < 1)
                                pageCount = 1;
                            stepSize = FILE_REQUESTER_GUTTER_STEPS / pageCount;
                            gpMouseManager->MouseCoords(ptrX, ptrY);
                            ptrY -= m_y + FILE_REQUESTER_GUTTER_TOP;
                            ptrY -= FILE_REQUESTER_SCROLL_KNOB_HALF_HEIGHT;
                            firstShown = ptrY * FILE_REQUESTER_GUTTER_SCALE / stepSize;
                            m_topIndex = firstShown;
                            if (m_topIndex + FILE_REQUESTER_LAST_ROW_OFFSET >= m_fileCount)
                                m_topIndex = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                            if (m_topIndex < 0)
                                m_topIndex = 0;
                            Update(1);
                            break;
                        case scrollerId:
                            DoKnob();
                            break;
                        case firstItemId:
                        case firstItemId + 1:
                        case firstItemId + 2:
                        case firstItemId + 3:
                        case firstItemId + 4:
                        case firstItemId + 5:
                        case firstItemId + 6:
                        case firstItemId + 7:
                        case firstItemId + 8:
                        case firstItemId + 9:
                            if (message.id - firstItemId + m_topIndex == m_selectedIndex) {
                                message.value = DIALOG_BUTTON_2;
                                message.id = DIALOG_BUTTON_2;
                                handled = 1;
                                break;
                            }
                            m_selectedIndex = message.id - firstItemId + m_topIndex;
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

    if (handled == 1) {
        if (gCampaignChoice <= 0 && m_mode == FILE_REQUESTER_LOAD && m_selectedIndex >= 0
            && gRequestingGames && message.value != FILE_REQUESTER_CANCEL) {
            key = m_extensions[m_selectedIndex].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0';
            if (key < giNumHumanPlayers
                && giDebugLevel < FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH) {
                sprintf(gText, localization::Tr("file.humans.minimum"), key, giNumHumanPlayers);
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
                handled = 0;
            }
            if (key > giNumHumanPlayers) {
                sprintf(
                    gText,
                    localization::Tr("file.humans.computer"),
                    key,
                    key - giNumHumanPlayers
                );
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                if (gpWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                    handled = 0;
            }
        }
        if (handled) {
            message.type = MESSAGE_EXECUTIVE;
            message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    UpdateMapInfo();
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x00455329, 0x7e)
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
DATA(0x0049f4e0)
char* gFRDummy = "";

// Buka 2.1 DoKnob with HoMM1's ten-row list and 156-pixel gutter.
VA(0x004553a7, 0x25b)
void fileRequester::DoKnob(void) {
    i32 lastTop;
    i16 pos;
    double scale;
    tag_message event;
    i16 x;
    i16 n;
    i16 offset;

    gpMouseManager->SetCursorShape(4);
    lastTop = m_topIndex;
    scale = 156.0 / (m_fileCount - FILE_REQUESTER_LAST_ROW_OFFSET);
    gpMouseManager->MouseCoords(x, n);
    offset = n - m_scrollKnob->m_y;
    gpInputManager->Flush();
    event = gpInputManager->GetEvent();
    while (event.type != MESSAGE_LEFT_BUTTON_UP && event.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (event.type == MESSAGE_MOUSE_MOVE) {
            if (event.y < offset + FILE_REQUESTER_GUTTER_TOP)
                event.y = offset + FILE_REQUESTER_GUTTER_TOP;
            if (event.y > offset + FILE_REQUESTER_GUTTER_BOTTOM)
                event.y = offset + FILE_REQUESTER_GUTTER_BOTTOM;
            gpMouseManager->Main(event);
            m_scrollKnob->m_y = event.y - offset;
            if (m_fileCount > FILE_REQUESTER_VISIBLE_ROWS) {
                pos = static_cast<i16>((m_scrollKnob->m_y - FILE_REQUESTER_GUTTER_TOP) / scale);
                if (pos != lastTop) {
                    if (pos > m_fileCount - FILE_REQUESTER_VISIBLE_ROWS)
                        pos = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                    if (pos < 0)
                        pos = 0;
                    m_topIndex = pos;
                    Update(0);
                    m_scrollKnob->m_y = event.y - offset;
                    m_window->DrawWindow();
                    lastTop = pos;
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
VA(0x00455602, 0x49e)
void fileRequester::Update(i8 drawWindow) {
    double gutterFactor;
    i32 oldHumans;
    i32 newPlayers;
    i32 limit;
    i32 newPos;
    const i16 firstIdIdx = FILE_REQUESTER_LIST_FIRST;
    const i16 nameIdIdx = FILE_REQUESTER_FILENAME_ENTRY;
    char prevExtra[FILE_REQUESTER_UPDATE_STORAGE_SIZE];
    const i16 colorVal = 0xe8;
    const i16 textColorValue = 1;
    tag_message eventRec;
    i32 theSuffixWidth;
    font* bigFont;
    i16 y;

    eventRec.type = MESSAGE_WIDGET;
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    for (y = 0; y < FILE_REQUESTER_VISIBLE_ROWS; y++) {
        eventRec.id = y + firstIdIdx;
        if (m_topIndex + y >= m_fileCount) {
            eventRec.command = WIDGET_COMMAND_CLEAR_FLAGS;
            eventRec.value = WIDGET_FLAG_DRAW;
        } else {
            eventRec.command = WIDGET_COMMAND_SET_FLAGS;
            eventRec.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(eventRec);
            eventRec.command = WIDGET_COMMAND_SET_TEXT;
            if (gShowMapInfo)
                sprintf(gText, "%s", m_mapNames[m_topIndex + y].text);
            else
                sprintf(gText, "%s", m_fileNames[m_topIndex + y].text);
            oldHumans =
                m_extensions[m_topIndex + y].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0';
            newPlayers = 0;
            if (oldHumans != 1 && gCampaignChoice <= 0 && gRequestingGames) {
                newPlayers = 1;
                sprintf(prevExtra, " (%d %s)", oldHumans, localization::Tr("file.players.label"));
                theSuffixWidth = bigFont->LineWidth(prevExtra);
            }
            limit = FILE_REQUESTER_ROW_TEXT_WIDTH;
            if (newPlayers)
                limit -= theSuffixWidth + FILE_REQUESTER_PLAYER_SUFFIX_GAP;
            newPos = strlen(gText);
            while (bigFont->LineWidth(gText) > limit) {
                newPos--;
                gText[newPos] = 0;
            }
            if (newPlayers)
                strcat(gText, prevExtra);
            eventRec.text = gText;
        }
        m_window->BroadcastMessage(eventRec);
        eventRec.command = WIDGET_COMMAND_SET_COLOR;
        if (m_selectedIndex == m_topIndex + y)
            eventRec.value = colorVal;
        else
            eventRec.value = textColorValue;
        m_window->BroadcastMessage(eventRec);
    }

    eventRec.id = nameIdIdx;
    eventRec.command = WIDGET_COMMAND_SET_FLAGS;
    eventRec.value = WIDGET_FLAG_ENABLED;
    m_window->BroadcastMessage(eventRec);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE) {
        eventRec.command = WIDGET_COMMAND_SET_TEXT;
        if (gShowMapInfo)
            sprintf(gText, "%s", m_mapNames[m_selectedIndex].text);
        else
            sprintf(gText, "%s", m_fileNames[m_selectedIndex].text);
        eventRec.text = gText;
        m_window->BroadcastMessage(eventRec);
    }
    if (m_mode == FILE_REQUESTER_LOAD) {
        eventRec.command = WIDGET_COMMAND_CLEAR_FLAGS;
        eventRec.value = WIDGET_FLAG_ENABLED;
        m_window->BroadcastMessage(eventRec);
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

VA(0x00455aa0, 0x59)
char* fileRequester::GetMapName(void) {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_fileCount && m_mapNames)
        return m_mapNames[m_selectedIndex].text;
    else
        return gFRDummy;
}

// Buka 2.1 GetFilename for HoMM1's two modes.
VA(0x00455af9, 0x115)
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
VA(0x00455c0e, 0x206)
void fileRequester::ShowMapInfo(void) {
    const i32 sizeIdPos = FILE_REQUESTER_MAP_SIZE;
    const i32 levelId = FILE_REQUESTER_MAP_LEVEL;
    const i32 descriptionId = FILE_REQUESTER_MAP_DESCRIPTION;
    tag_message msg;

    sprintf(gText, "");
    msg.text = gText;
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        gMapSize = m_mapInfo[m_selectedIndex].size;
    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, sizeIdPos);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        msg.text = gMapSizeNames[m_mapInfo[m_selectedIndex].size];
    gReqExtraWindow->BroadcastMessage(msg);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        gMapDifficulty = m_mapInfo[m_selectedIndex].difficulty;
    sprintf(gText, gCurMapName);
    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, levelId);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        msg.text = gMapDifficultyNames[m_mapInfo[m_selectedIndex].difficulty];
    gReqExtraWindow->BroadcastMessage(msg);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        strcpy(gFullMapName, m_mapNames[m_selectedIndex].text);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        strcpy(gMapDescription, m_mapInfo[m_selectedIndex].description);
    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, descriptionId);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE)
        msg.text = m_mapInfo[m_selectedIndex].description;
    gReqExtraWindow->BroadcastMessage(msg);
    gReqExtraWindow->DrawWindow();
}

// REQUEST owns retail .bss 0x004c5130-0x004c5137.
DATA(0x004cc82c)
i8 gRequestingGames;
