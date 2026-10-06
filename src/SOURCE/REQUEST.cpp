#include <H1/Ints.h>

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

i32 ShowThisMap(char* fileName) {
#ifdef HOMM1_EDITOR
    if (strnicmp(fileName, "AES3", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "BEM2", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "CAMP", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "CNM5", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "DNL3", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "ENS1", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "FEL6", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "GHM4", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "HNM1", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "INM6", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "JEM7", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "KNS2", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "LNS4", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "MIS7", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "NHL5", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "ONL7", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "PNM3", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "QNL1", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "RNL4", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "SEL2", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "THS5", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH)
        && strnicmp(fileName, "UHS6", FILE_REQUESTER_SHIPPED_MAP_PREFIX_LENGTH))
#endif
        return 1;
    return 0;
}

fileRequester::fileRequester(
    i16 x,
    i16 y,
    i16 mode,
    const char* pattern,
    const char* directory,
    const char* defaultExtension
) {
    char fullFileName[412];
    i32 file;
    SMapHeader header;
    BOOL found;
    char unusedName[FILE_REQUESTER_UNUSED_NAME_SIZE];
    i32 shiftRow;
    i32 entryIndex;
    char extension[FILE_REQUESTER_EXTENSION_SIZE];
    WIN32_FIND_DATA findFileData;
    i32 insertCount;
    char* extensionStart;
    char nameBuffer[FILE_REQUESTER_LOCAL_NAME_SIZE];
    HANDLE findHandle;

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
        gRequestingGames = true;
    else
        gRequestingGames = false;
    m_mode = mode;

    sprintf(gText, "%s%s", directory, pattern);
    m_fileCount = 0;
    findHandle = FindFirstFile(gText, &findFileData);
    if (findHandle != INVALID_HANDLE_VALUE) {
        if (ShowThisMap(findFileData.cFileName))
            m_fileCount++;
        while (FindNextFile(findHandle, &findFileData)) {
            if (ShowThisMap(findFileData.cFileName))
                m_fileCount++;
        }
        FindClose(findHandle);
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
    findHandle = FindFirstFile(gText, &findFileData);
    if (findHandle != INVALID_HANDLE_VALUE) {
        found = TRUE;
        while (found) {
            if (ShowThisMap(findFileData.cFileName)) {
                strcpy(nameBuffer, findFileData.cFileName);
                extensionStart = FindLastToken(nameBuffer, '.');
                if (extensionStart) {
                    strcpy(extension, extensionStart);
                    *extensionStart = '\0';
                }
                for (entryIndex = 0; entryIndex < insertCount; entryIndex++) {
                    if (strcmpi(nameBuffer, m_fileNames[entryIndex].text) < 0) {
                        for (shiftRow = insertCount; shiftRow > entryIndex; shiftRow--) {
                            strcpy(m_fileNames[shiftRow].text, m_fileNames[shiftRow - 1].text);
                            strcpy(m_extensions[shiftRow].text, m_extensions[shiftRow - 1].text);
                        }
                        goto insert;
                    }
                }
            insert:
                strcpy(m_fileNames[entryIndex].text, nameBuffer);
                strcpy(m_extensions[entryIndex].text, extension);
                insertCount++;
            }
            found = FindNextFile(findHandle, &findFileData);
        }
        FindClose(findHandle);
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
            if (file == FILE_DESCRIPTOR_INVALID)
                FileError(fullFileName);
            READ_FILE_VALUE(file, header);
            if (header.id == MAP_HEADER_ID) {
                strcpy(m_mapNames[entryIndex].text, header.name[0]);
                strcpy(m_mapInfo[entryIndex].description, header.description[0]);
                m_mapInfo[entryIndex].difficulty = header.difficulty;
                m_mapInfo[entryIndex].size = header.size;
            } else {
                strcpy(m_mapNames[entryIndex].text, m_fileNames[entryIndex].text);
                strcpy(m_mapInfo[entryIndex].description, "");
                m_mapInfo[entryIndex].difficulty = MAP_DIFFICULTY_EASY;
                m_mapInfo[entryIndex].size = MAP_SIZE_SMALL;
            }
            close(file);
        }
    }

    KBChangeMenu(gDefaultMenu);
    m_acceptMask = FILE_REQUESTER_DISPATCH_MASK;
    m_result = FILE_REQUESTER_MAP_INFO_NONE;
}

fileRequester::~fileRequester() {}

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
    gWindowManager->RemoveWindow(m_window);
    delete m_window;
    m_active = 0;
}

i16 fileRequester::Open(i16 priority) {
    const i16 scrollKnobId = FILE_REQUESTER_SCROLL_KNOB;
    tag_message message;
    i32 i;
    const i16 nameLabelId = FILE_REQUESTER_FILENAME_LABEL;
    char* extensionStart;
    b8 enableOk;

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
#ifdef HOMM1_EDITOR
    enableOk = false;
    message.id = nameLabelId;
    sprintf(gText, localization::Tr("file.load.label"));
    message.text = gText;
    m_window->BroadcastMessage(message);
#else
    if (m_mode == FILE_REQUESTER_SAVE) {
        enableOk = true;
        const i16 textEntryId = FILE_REQUESTER_FILENAME_ENTRY;
        strcpy(m_filename, gGame->m_saveName);
        extensionStart = FindLastToken(m_filename, '.');
        if (extensionStart)
            *extensionStart = '\0';
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
                       == gNumHumanPlayers)
                m_selectedIndex = i;
        }
    } else {
        enableOk = false;
        if (m_mode == FILE_REQUESTER_LOAD && m_defaultExtension[1] == 'M') {
            for (i = 0; i < m_fileCount; i++) {
                if (!strnicmp(m_fileNames[i].text, gMapName, SAVE_FILE_BASE_NAME_LENGTH)) {
                    m_selectedIndex = i;
                    enableOk = true;
                }
            }
        }
        message.id = nameLabelId;
        sprintf(gText, localization::Tr("file.load.label"));
        message.text = gText;
        m_window->BroadcastMessage(message);
    }
#endif
    const i16 entryId = FILE_REQUESTER_FILENAME_ENTRY;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_MAX_LENGTH, entryId);
    message.value = FILE_REQUESTER_FILENAME_MAX_LENGTH;
    m_window->BroadcastMessage(message);
    Update(false);
#ifndef HOMM1_EDITOR
    if (gShowMapInfo)
        gWindowManager->AddWindow(gReqExtraWindow, WINDOW_Z_ORDER_APPEND, 1);
#endif
    gWindowManager->AddWindow(m_window, WINDOW_Z_ORDER_APPEND, 1);
    SetOK(enableOk);
    UpdateMapInfo();
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "fileRequester");
    return BASE_MANAGER_SUCCESS;
}

void fileRequester::SetOK(b8 enabled) {
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

i16 fileRequester::Main(tag_message& message) {
    i32 firstShown;
    i32 stepSize;
    i32 pageCount;
    const i16 arrowUpId = FILE_REQUESTER_SCROLL_UP;
    tag_message reply;
    i16 length;
    i16 key;
    b32 handled;
    i16 mouseY;
    i16 mouseX;
    char nameBuffer[FILE_REQUESTER_LOCAL_NAME_SIZE];
    const i16 downId = FILE_REQUESTER_SCROLL_DOWN;
    const i16 scrollBarId = FILE_REQUESTER_SCROLL_GUTTER;
    const i16 firstItemId = FILE_REQUESTER_LIST_FIRST;
    const i16 scrollerId = FILE_REQUESTER_SCROLL_KNOB;
    const i16 fileNameId = FILE_REQUESTER_FILENAME_ENTRY;

    handled = false;
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
                        Update(true);
                    }
                    break;
                case INPUT_SCAN_NUMPAD_2:
                    if (m_selectedIndex < m_fileCount - 1) {
                        m_selectedIndex++;
                        if (m_topIndex + FILE_REQUESTER_VISIBLE_ROWS <= m_selectedIndex)
                            m_topIndex++;
                        Update(true);
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
                                handled = true;
                            }
                            break;
                        case DIALOG_BUTTON_1:
                            message.value = message.id;
                            handled = true;
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
                                    nameBuffer[key] = '\0';
                            }
                            for (key = strlen(nameBuffer) - 1; key >= 0; key--) {
                                if (static_cast<u8>(nameBuffer[key]) == ' ')
                                    nameBuffer[key] = '\0';
                                else
                                    key = -1;
                            }
                            if (strlen(nameBuffer) > 0 && static_cast<u8>(nameBuffer[0]) > ' ') {
                                m_selectedIndex = FILE_REQUESTER_SELECTION_NONE;
                                strcpy(m_filename, nameBuffer);
                                SetOK(true);
                            }
                            reply.command = WIDGET_COMMAND_SET_TEXT;
                            reply.id = fileNameId;
                            reply.text = m_filename;
                            m_window->BroadcastMessage(reply);
                            Update(true);
                            break;
                        case arrowUpId:
                            if (m_topIndex > 0) {
                                m_topIndex--;
                                Update(true);
                            }
                            break;
                        case downId:
                            if (m_topIndex + FILE_REQUESTER_VISIBLE_ROWS < m_fileCount) {
                                m_topIndex++;
                                if (m_topIndex + FILE_REQUESTER_LAST_ROW_OFFSET >= m_fileCount)
                                    m_topIndex = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                                Update(true);
                            }
                            break;
                        case scrollBarId:
                            pageCount = m_fileCount - FILE_REQUESTER_LAST_ROW_OFFSET;
                            if (pageCount < 1)
                                pageCount = 1;
                            stepSize = FILE_REQUESTER_GUTTER_STEPS / pageCount;
                            gMouseManager->MouseCoords(mouseX, mouseY);
                            mouseY -= m_y + FILE_REQUESTER_GUTTER_TOP;
                            mouseY -= FILE_REQUESTER_SCROLL_KNOB_HALF_HEIGHT;
                            firstShown = mouseY * FILE_REQUESTER_GUTTER_SCALE / stepSize;
                            m_topIndex = firstShown;
                            if (m_topIndex + FILE_REQUESTER_LAST_ROW_OFFSET >= m_fileCount)
                                m_topIndex = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                            if (m_topIndex < 0)
                                m_topIndex = 0;
                            Update(true);
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
                                handled = true;
                                break;
                            }
                            m_selectedIndex = message.id - firstItemId + m_topIndex;
                            if (m_selectedIndex >= m_fileCount) {
                                m_selectedIndex = FILE_REQUESTER_SELECTION_NONE;
                                SetOK(false);
                            } else {
                                SetOK(true);
                            }
                            Update(true);
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

    if (handled == true) {
#ifndef HOMM1_EDITOR
        if (gCampaignChoice <= CAMPAIGN_NONE && m_mode == FILE_REQUESTER_LOAD
            && m_selectedIndex >= 0 && gRequestingGames && message.value != FILE_REQUESTER_CANCEL) {
            key = m_extensions[m_selectedIndex].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0';
            if (key < gNumHumanPlayers
                && gDebugLevel < FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH_MIN) {
                sprintf(gText, localization::Tr("file.humans.minimum"), key, gNumHumanPlayers);
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
                handled = false;
            }
            if (key > gNumHumanPlayers) {
                sprintf(
                    gText,
                    localization::Tr("file.humans.computer"),
                    key,
                    key - gNumHumanPlayers
                );
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                if (gWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                    handled = false;
            }
        }
#endif
        if (handled) {
            message.type = MESSAGE_EXECUTIVE;
            message.executiveCommand = EXECUTIVE_COMMAND_RETURN_RESULT;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    UpdateMapInfo();
    return MESSAGE_DISPATCH_CONSUME;
}

void fileRequester::UpdateMapInfo(void) {
    if (m_selectedIndex != m_result && gShowMapInfo) {
        if (m_selectedIndex >= 0)
            strcpy(gCurMapName, m_fileNames[m_selectedIndex].text);
        else
            strcpy(gCurMapName, "");
        ShowMapInfo();
    }
}

char* gFRDummy = "";

void fileRequester::DoKnob(void) {
    i32 lastTop;
    i16 topRow;
    double scale;
    tag_message event;
    i16 x;
    i16 grabPointerY;
    i16 grabOffset;

    gMouseManager->SetCursorShape(4);
    lastTop = m_topIndex;
    scale = 156.0 / (m_fileCount - FILE_REQUESTER_LAST_ROW_OFFSET);
    gMouseManager->MouseCoords(x, grabPointerY);
    grabOffset = grabPointerY - m_scrollKnob->m_y;
    gInputManager->Flush();
    event = gInputManager->GetEvent();
    while (!IS_BUTTON_RELEASE_MESSAGE(event.type)) {
        if (event.type == MESSAGE_MOUSE_MOVE) {
            if (event.y < grabOffset + FILE_REQUESTER_GUTTER_TOP)
                event.y = grabOffset + FILE_REQUESTER_GUTTER_TOP;
            if (event.y > grabOffset + FILE_REQUESTER_GUTTER_BOTTOM)
                event.y = grabOffset + FILE_REQUESTER_GUTTER_BOTTOM;
            gMouseManager->Main(event);
            m_scrollKnob->m_y = event.y - grabOffset;
            if (m_fileCount > FILE_REQUESTER_VISIBLE_ROWS) {
                topRow = (m_scrollKnob->m_y - FILE_REQUESTER_GUTTER_TOP) / scale;
                if (topRow != lastTop) {
                    if (topRow > m_fileCount - FILE_REQUESTER_VISIBLE_ROWS)
                        topRow = m_fileCount - FILE_REQUESTER_VISIBLE_ROWS;
                    if (topRow < 0)
                        topRow = 0;
                    m_topIndex = topRow;
                    Update(false);
                    m_scrollKnob->m_y = event.y - grabOffset;
                    m_window->DrawWindow();
                    lastTop = topRow;
                } else {
                    m_window->DrawWindow();
                }
            } else {
                m_window->DrawWindow();
            }
        }
        Process1WindowsMessage();
        event = gInputManager->GetEvent();
    }
    gMouseManager->SetCursorShape(6);
    m_scrollKnob->m_flags &= ~WIDGET_FLAG_SELECTED;
    Update(true);
}

void fileRequester::Update(b8 drawWindow) {
    double gutterFactor;
    i32 textLimit;
    const i16 firstRowId = FILE_REQUESTER_LIST_FIRST;
    const i16 filenameEntryId = FILE_REQUESTER_FILENAME_ENTRY;
    i32 length;
    const i16 selectedColor = 0xe8;
    i32 savedPlayerCount;
    b32 hasPlayerSuffix;
    const i16 plainColor = 1;
    tag_message message;
    char playersString[FILE_REQUESTER_UPDATE_STORAGE_SIZE];
    i32 theSuffixWidth;
    font* bigFont;
    i16 row;

    message.type = MESSAGE_WIDGET;
    bigFont = gResourceManager->GetFont("bigfont.fnt");
    for (row = 0; row < FILE_REQUESTER_VISIBLE_ROWS; row++) {
        message.id = row + firstRowId;
        if (m_topIndex + row >= m_fileCount) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_TEXT;
            if (gShowMapInfo)
                sprintf(gText, "%s", m_mapNames[m_topIndex + row].text);
            else
                sprintf(gText, "%s", m_fileNames[m_topIndex + row].text);
            savedPlayerCount =
                m_extensions[m_topIndex + row].text[FILE_REQUESTER_EXTENSION_PLAYER_DIGIT] - '0';
            hasPlayerSuffix = false;
#ifndef HOMM1_EDITOR
            if (savedPlayerCount != 1 && gCampaignChoice <= CAMPAIGN_NONE && gRequestingGames) {
                hasPlayerSuffix = true;
                sprintf(
                    playersString,
                    " (%d %s)",
                    savedPlayerCount,
                    localization::Tr("file.players.label")
                );
                theSuffixWidth = bigFont->LineWidth(playersString);
            }
#endif
            textLimit = FILE_REQUESTER_ROW_TEXT_WIDTH;
            if (hasPlayerSuffix)
                textLimit -= theSuffixWidth + FILE_REQUESTER_PLAYER_SUFFIX_GAP;
            length = strlen(gText);
            while (bigFont->LineWidth(gText) > textLimit) {
                length--;
                gText[length] = 0;
            }
            if (hasPlayerSuffix)
                strcat(gText, playersString);
            message.text = gText;
        }
        m_window->BroadcastMessage(message);
        message.command = WIDGET_COMMAND_SET_COLOR;
        if (m_selectedIndex == m_topIndex + row)
            message.value = selectedColor;
        else
            message.value = plainColor;
        m_window->BroadcastMessage(message);
    }

    message.id = filenameEntryId;
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.value = WIDGET_FLAG_ENABLED;
    m_window->BroadcastMessage(message);
    if (m_selectedIndex != FILE_REQUESTER_SELECTION_NONE) {
        message.command = WIDGET_COMMAND_SET_TEXT;
        if (gShowMapInfo)
            sprintf(gText, "%s", m_mapNames[m_selectedIndex].text);
        else
            sprintf(gText, "%s", m_fileNames[m_selectedIndex].text);
        message.text = gText;
        m_window->BroadcastMessage(message);
    }
    if (m_mode == FILE_REQUESTER_LOAD) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED;
        m_window->BroadcastMessage(message);
    }
    if (m_fileCount <= FILE_REQUESTER_VISIBLE_ROWS) {
        m_scrollKnob->m_y = 134;
    } else {
        gutterFactor = 156.0 / (m_fileCount - FILE_REQUESTER_VISIBLE_ROWS);
        m_scrollKnob->m_y = m_topIndex * gutterFactor + 56.0;
    }
    if (drawWindow)
        m_window->DrawWindow();
    gResourceManager->Dispose(bigFont);
}

char* fileRequester::GetMapName(void) {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_fileCount && m_mapNames)
        return m_mapNames[m_selectedIndex].text;
    else
        return gFRDummy;
}

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

void fileRequester::ShowMapInfo(void) {
#ifndef HOMM1_EDITOR
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
#endif
}

b8 gRequestingGames;
