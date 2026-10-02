// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/X_GLOBAL.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <BASE/soundmgr.h>
#include <SOURCE/kbwin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

// Buka 2.1 ShowThisMap; HoMM1 keeps an unreachable rejecting return.
VA(0x00448020, 0x1c)
int ShowThisMap(char*) {
    return 1;
    return 0;
}

// Buka 2.1 constructor with InitializeFiles folded in: HoMM1 counts and sorts
// every match of the pattern and reads .MAP headers only for GetMap's list.
VA(0x0044803c, 0x83d)
fileRequester::fileRequester(
    short x,
    short y,
    short mode,
    const char* pattern,
    const char* directory,
    const char* defaultExtension
) {
    HANDLE dirHandle;
    int fd;
    char fullPath[412];
    SMapHeader header;
    int findResult;
    WIN32_FIND_DATA fileData;
    char* ptr;
    char extStr[FILE_REQUESTER_LOCAL_EXTENSION_SIZE];
    char fileName[FILE_REQUESTER_LOCAL_NAME_SIZE];
    int moveValue;
    int sortedCount;
    int index;

    m_selectedIndex = -1;
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
        gbRequestingGames = 1;
    else
        gbRequestingGames = 0;
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
    if (gbShowMapInfo) {
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

    if (gbShowMapInfo) {
        for (index = 0; index < sortedCount; index++) {
            sprintf(
                fullPath, "%s%s%s", directory, m_fileNames[index].text,
                m_extensions[index].text);
            fd = open(fullPath, O_BINARY);
            if (fd == -1)
                FileError(fullPath);
            read(fd, &header, sizeof(header));
            if (header.id == 1000) {
                strcpy(m_mapNames[index].text, header.name);
                strcpy(m_mapInfo[index].description, header.description);
                m_mapInfo[index].difficulty = header.difficulty;
                m_mapInfo[index].size = header.size;
            } else {
                strcpy(m_mapNames[index].text, m_fileNames[index].text);
                strcpy(m_mapInfo[index].description, "");
                m_mapInfo[index].difficulty = 0;
                m_mapInfo[index].size = 0;
            }
            close(fd);
        }
    }

    KBChangeMenu(hmnuDflt);
    m_acceptMask = 0x32f;
    m_result = -2;
}

VA(0x00448879, 0x1f)
fileRequester::~fileRequester() {}

// Buka 2.1 Close with CleanUpData folded in; HoMM1 also remembers the
// chosen map's title.
VA(0x00448898, 0x12c)
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
VA(0x004489c4, 0x431)
short fileRequester::Open(short priority) {
    const short scrollId = 14;
    int i;
    const short promptId = 16;
    tag_message message;
    signed char enable;
    char* period;

    strcpy(gLastMapName, "");
    strcpy(gLastFilename, "");
    m_window = new heroWindow(m_x, m_y, "request.bin");
    if (!m_window)
        MemError();
    m_scrollKnob = new iconWidget(283, 56, 8, 17, "scroll.icn", 4, ICON_DRAW_NORMAL, scrollId, ICON_WIDGET_DRAW, 1);
    if (!m_scrollKnob)
        MemError();
    m_window->AddWidget(m_scrollKnob, -1);

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    if (m_mode == 1) {
        enable = 1;
        const short textEntryId = 15;
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
                && m_extensions[i].text[3] - '0' == giNumHumanPlayers)
                m_selectedIndex = i;
        }
    } else {
        enable = 0;
        if (m_mode == 0 && m_defaultExtension[1] == 'M') {
            for (i = 0; i < m_fileCount; i++) {
                if (!strnicmp(m_fileNames[i].text, gMapName, 8)) {
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
    const short nameId = 15;
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_MAX_LENGTH;
    message.id = nameId;
    message.value = 255;
    m_window->BroadcastMessage(message);
    Update(0);
    if (gbShowMapInfo)
        gpWindowManager->AddWindow(gpReqExtraWindow, -1, 1);
    gpWindowManager->AddWindow(m_window, -1, 1);
    SetOK(enable);
    UpdateMapInfo();
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "fileRequester");
    return 0;
}

// Buka 2.1 SetOK with HoMM1's fixed dimming flags.
VA(0x00448df5, 0x8a)
void fileRequester::SetOK(signed char enabled) {
    tag_message message;

    message.type = MESSAGE_WIDGET;
    if (enabled)
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    else
        message.command = WIDGET_COMMAND_SET_FLAGS;
    message.id = DIALOG_BUTTON_2;
    message.value = WIDGET_FLAG_DIMMED;
    m_window->BroadcastMessage(message);
    if (enabled)
        message.command = WIDGET_COMMAND_SET_FLAGS;
    else
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED;
    m_window->BroadcastMessage(message);
}

// Buka 2.1 Main without the map-size filter; HoMM1 checks a saved game's
// human count, encoded as its extension digit, before accepting it.
VA(0x00448e7f, 0xa8c)
short fileRequester::Main(tag_message& message) {
    int newTop;
    int stepSize;
    int numPages;
    const short arrowUpId = 1;
    tag_message msg;
    const short downArrowId = 2;
    const short railId = 3;
    const short firstRowId = 4;
    const short scrollerId = 14;
    const short nameId = 15;
    short ch;
    short len;
    int finished;
    short ptrY;
    short ptrX;
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
                case 0x48:
                    if (m_selectedIndex > 0) {
                        m_selectedIndex--;
                        if (m_topIndex > m_selectedIndex)
                            m_topIndex--;
                        Update(1);
                    }
                    break;
                case 0x50:
                    if (m_selectedIndex < m_fileCount - 1) {
                        m_selectedIndex++;
                        if (m_topIndex + 10 <= m_selectedIndex)
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
                        case 0x7802:
                            if (m_selectedIndex == -1 && !m_filename[0]) {
                                NormalDialog(
                                    "Please make a selection from the list, or press cancel.", NORMAL_DIALOG_TYPE_OK,
                                    -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                                break;
                            } else {
                                message.value = message.id;
                                finished = 1;
                            }
                            break;
                        case 0x7801:
                            message.value = message.id;
                            finished = 1;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case nameId:
                            msg.type = MESSAGE_WIDGET;
                            msg.command = WIDGET_COMMAND_GET_TEXT;
                            msg.id = nameId;
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
                                m_selectedIndex = -1;
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
                            if (m_topIndex + 10 < m_fileCount) {
                                m_topIndex++;
                                if (m_topIndex + 9 >= m_fileCount)
                                    m_topIndex = m_fileCount - 10;
                                Update(1);
                            }
                            break;
                        case railId:
                            numPages = m_fileCount - 9;
                            if (numPages < 1)
                                numPages = 1;
                            stepSize = 15700 / numPages;
                            gpMouseManager->MouseCoords(ptrX, ptrY);
                            ptrY -= m_y + 56;
                            ptrY -= 9;
                            newTop = ptrY * 100 / stepSize;
                            m_topIndex = newTop;
                            if (m_topIndex + 9 >= m_fileCount)
                                m_topIndex = m_fileCount - 10;
                            if (m_topIndex < 0)
                                m_topIndex = 0;
                            Update(1);
                            break;
                        case scrollerId:
                            DoKnob();
                            break;
                        case firstRowId:
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 13:
                            if (message.id - firstRowId + m_topIndex
                                == m_selectedIndex) {
                                message.value = 0x7802;
                                message.id = 0x7802;
                                finished = 1;
                                break;
                            }
                            m_selectedIndex = message.id - firstRowId + m_topIndex;
                            if (m_selectedIndex >= m_fileCount) {
                                m_selectedIndex = -1;
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
        if (giCampaignChoice <= 0 && m_mode == 0 && m_selectedIndex >= 0 && gbRequestingGames
            && message.value != 0x7801) {
            ch = m_extensions[m_selectedIndex].text[3] - '0';
            if (ch < giNumHumanPlayers && giDebugLevel < 2) {
                sprintf(
                    gText,
                    "The game you have chosen only has slots for %d human(s).  You need one "
                    "with room for at least %d humans.",
                    ch, giNumHumanPlayers);
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                finished = 0;
            }
            if (ch > giNumHumanPlayers) {
                sprintf(
                    gText,
                    "The game you have chosen was being played with %d humans. Is it OK if the "
                    "computer takes the place of the last %d human(s)?",
                    ch, ch - giNumHumanPlayers);
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                if (gpWindowManager->m_dialogResult != 0x7805)
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

VA(0x0044990b, 0x9d)
void fileRequester::UpdateMapInfo(void) {
    if (m_selectedIndex != m_result && gbShowMapInfo) {
        if (m_selectedIndex >= 0)
            strcpy(gcCurMapName, m_fileNames[m_selectedIndex].text);
        else
            strcpy(gcCurMapName, "");
        ShowMapInfo();
    }
}

// GetMapName/GetFilename's no-selection result; data coverage is deferred.
DATA(0x00490cfc)
char* cFRDummy = "";

// Buka 2.1 DoKnob with HoMM1's ten-row list and 156-pixel gutter.
VA(0x004499a8, 0x2b2)
void fileRequester::DoKnob(void) {
    short offset;
    int lastTop;
    short index;
    double scale;
    short x;
    short my;
    tag_message event;

    gpMouseManager->SetCursorShape(4);
    lastTop = m_topIndex;
    scale = 157.0 / (m_fileCount - 9);
    gpMouseManager->MouseCoords(x, my);
    offset = my - m_scrollKnob->m_y;
    gpInputManager->Flush();
    event = gpInputManager->GetEvent();
    while (event.type != MESSAGE_LEFT_BUTTON_UP && event.type != MESSAGE_RIGHT_BUTTON_UP) {
        if (event.type == MESSAGE_MOUSE_MOVE) {
            if (offset + 56 > event.y)
                event.y = offset + 56;
            if (offset + 212 < event.y)
                event.y = offset + 212;
            gpMouseManager->Main(event);
            m_scrollKnob->m_y = event.y - offset;
            if (m_fileCount > 10) {
                index = static_cast<short>((m_scrollKnob->m_y - 56) / scale);
                if (index != lastTop) {
                    if (index > m_fileCount - 10)
                        index = m_fileCount - 10;
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
    m_scrollKnob->m_flags &= ~1;
    Update(1);
}

// Buka 2.1 Update for HoMM1's ten text rows; saved games append their human
// count and map lists show the header title.
VA(0x00449c5a, 0x55e)
void fileRequester::Update(signed char drawWindow) {
    double gutterFactor;
    int nHumans;
    int showPlayers;
    int limit;
    int pos;
    const short firstId = 4;
    const short nameId = 15;
    char extra[372];
    const short hiliteColor = 0xe8;
    const short textColor = 1;
    tag_message event;
    int suffixWidth;
    font* bigFont;
    short row;

    event.type = MESSAGE_WIDGET;
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    for (row = 0; row < 10; row++) {
        event.id = row + firstId;
        if (m_topIndex + row >= m_fileCount) {
            event.command = WIDGET_COMMAND_CLEAR_FLAGS;
            event.value = WIDGET_FLAG_DRAW;
        } else {
            event.command = WIDGET_COMMAND_SET_FLAGS;
            event.value = WIDGET_FLAG_DRAW;
            m_window->BroadcastMessage(event);
            event.command = WIDGET_COMMAND_SET_TEXT;
            if (gbShowMapInfo)
                sprintf(gText, "%s", m_mapNames[m_topIndex + row].text);
            else
                sprintf(gText, "%s", m_fileNames[m_topIndex + row].text);
            nHumans = m_extensions[m_topIndex + row].text[3] - '0';
            showPlayers = 0;
            if (nHumans != 1 && giCampaignChoice <= 0 && gbRequestingGames) {
                showPlayers = 1;
                sprintf(extra, " (%d %s)", nHumans, "Players");
                suffixWidth = bigFont->LineWidth(extra);
            }
            limit = 207;
            if (showPlayers)
                limit -= suffixWidth + 6;
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
    if (m_selectedIndex != -1) {
        event.command = WIDGET_COMMAND_SET_TEXT;
        if (gbShowMapInfo)
            sprintf(gText, "%s", m_mapNames[m_selectedIndex].text);
        else
            sprintf(gText, "%s", m_fileNames[m_selectedIndex].text);
        event.text = gText;
        m_window->BroadcastMessage(event);
    }
    if (m_mode == 0) {
        event.command = WIDGET_COMMAND_CLEAR_FLAGS;
        event.value = WIDGET_FLAG_ENABLED;
        m_window->BroadcastMessage(event);
    }
    if (m_fileCount <= 10) {
        m_scrollKnob->m_y = 134;
    } else {
        gutterFactor = 156.0 / (m_fileCount - 10);
        m_scrollKnob->m_y = m_topIndex * gutterFactor + 56.0;
    }
    if (drawWindow)
        m_window->DrawWindow();
    gpResourceManager->Dispose(bigFont);
}

VA(0x0044a1b8, 0x7d)
char* fileRequester::GetMapName(void) {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_fileCount && m_mapNames)
        return m_mapNames[m_selectedIndex].text;
    else
        return cFRDummy;
}

// Buka 2.1 GetFilename for HoMM1's two modes.
VA(0x0044a235, 0x13f)
char* fileRequester::GetFilename(void) {
    if (m_mode != 1 && (m_selectedIndex < 0 || m_selectedIndex >= m_fileCount))
        return cFRDummy;

    if (m_selectedIndex == -1)
        sprintf(gText, "%s%s", m_filename, m_defaultExtension);
    else if (m_mode == 0)
        sprintf(
            gText, "%s%s", m_fileNames[m_selectedIndex].text,
            m_extensions[m_selectedIndex].text);
    else
        sprintf(gText, "%s%s", m_fileNames[m_selectedIndex].text, m_defaultExtension);
    strcpy(m_filename, gText);
    return m_filename;
}

// Fills GetMap's reqextra.bin window with the selected map's size,
// difficulty and description.
VA(0x0044a374, 0x269)
void fileRequester::ShowMapInfo(void) {
    const int sizeId = 100;
    const int levelId = 101;
    const int descriptionId = 102;
    tag_message message;

    sprintf(gText, "");
    message.text = gText;
    if (m_selectedIndex != -1)
        giMapSize = m_mapInfo[m_selectedIndex].size;
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = sizeId;
    if (m_selectedIndex != -1)
        message.text = gMapSizeNames[m_mapInfo[m_selectedIndex].size];
    gpReqExtraWindow->BroadcastMessage(message);
    if (m_selectedIndex != -1)
        giMapDifficulty = m_mapInfo[m_selectedIndex].difficulty;
    sprintf(gText, gcCurMapName);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = levelId;
    if (m_selectedIndex != -1)
        message.text =
            gMapDifficultyNames[m_mapInfo[m_selectedIndex].difficulty];
    gpReqExtraWindow->BroadcastMessage(message);
    if (m_selectedIndex != -1)
        strcpy(gFullMapName, m_mapNames[m_selectedIndex].text);
    if (m_selectedIndex != -1)
        strcpy(gMapDescription, m_mapInfo[m_selectedIndex].description);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = descriptionId;
    if (m_selectedIndex != -1)
        message.text = m_mapInfo[m_selectedIndex].description;
    gpReqExtraWindow->BroadcastMessage(message);
    gpReqExtraWindow->DrawWindow();
}

// REQUEST owns retail .bss 0x004c5130-0x004c5137.
DATA(0x004c5130)
signed char gbRequestingGames;
