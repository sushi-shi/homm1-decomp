// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <BASE/WINMGR_TYPES.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/smackManager.h>
#include <SOURCE/wingraph.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Retail score-dialog owner bytes; initializer coverage is deferred.
DATA(0x00494170)
signed char gbShowHighScore;
DATA(0x004c794c)
signed char gbStandardHighScore;
DATA(0x00494128)
signed char gbHeroWindShowing;
DATA(0x0049412c)
signed char gbOverviewShowing;
// InitVars proves seven terrain rows, ordinary/diagonal cost columns.
DATA(0x004c6d50)
signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];

// HoMM2 KB.cpp confirms the identity and behavior. HoMM1 differs in the timer
// comparison and placement of the re-entry guard.
extern "C" VA(0x0044f640, 0x72)
void PollSound() {
    if (KBTickCount() < gNextSoundPollTick)
        return;
    if (gbInPollSound)
        return;
    gbInPollSound = 1;
    gNextSoundPollTick = KBTickCount() + 30;
    if (gbForegroundApp)
        gpSoundManager->PollSound();
    PollRemote();
    gbInPollSound = 0;
}

VA(0x0044f6b2, 0x20)
void ForcePollSound() {
    gNextSoundPollTick = KBTickCount() - 1;
    PollSound();
}

// donor PoL RVA 0x000965be; preferred Buka symbol ?InitMainClasses@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.512387;margin=0.755802;shape=0.400;size=0.925;calls=0.653;alternate=pol20:void InitMainClasses(void)@0x000965be
VA(0x0044f6d2, 0x607)
void InitMainClasses(void) {
    gpExec = new executive;
    gpInputManager = new inputManager;
    gpMouseManager = new mouseManager;
    gpWindowManager = new heroWindowManager;
    gpResourceManager = new resourceManager;
    gpSoundManager = new soundManager;
    gpSmackManager = new smackManager;
    gpHighScoreManager = new highScoreManager;
    gpGame = new game;
    gpAdvManager = new advManager;
    gpCombatManager = new combatManager;
    gpTownManager = new townManager;
    gpSearchArray = new searchArray;
    gpPhilAI = new philAI;
    gpMonGroup = new armyGroup;
    gpBufferPalette = new palette;
}

// Buka 2.1 DeleteMainClasses; HoMM1 also owns the smacker manager and frees the
// resource manager before the window, mouse and input managers.
VA(0x0044fcd9, 0x36d)
void DeleteMainClasses(void) {
    if (gpBufferPalette)
        delete gpBufferPalette;
    gpBufferPalette = 0;
    if (gpMonGroup)
        delete gpMonGroup;
    gpMonGroup = 0;
    if (gpPhilAI)
        delete gpPhilAI;
    gpPhilAI = 0;
    if (gpSearchArray)
        delete gpSearchArray;
    gpSearchArray = 0;
    if (gpTownManager)
        delete gpTownManager;
    gpTownManager = 0;
    if (gpCombatManager)
        delete gpCombatManager;
    gpCombatManager = 0;
    if (gpAdvManager)
        delete gpAdvManager;
    gpAdvManager = 0;
    if (gpGame)
        delete gpGame;
    gpGame = 0;
    if (gpHighScoreManager)
        delete gpHighScoreManager;
    gpHighScoreManager = 0;
    if (gpSmackManager)
        delete gpSmackManager;
    gpSmackManager = 0;
    if (gpSoundManager)
        delete gpSoundManager;
    gpSoundManager = 0;
    if (gpResourceManager)
        delete gpResourceManager;
    gpResourceManager = 0;
    if (gpWindowManager)
        delete gpWindowManager;
    gpWindowManager = 0;
    if (gpMouseManager)
        delete gpMouseManager;
    gpMouseManager = 0;
    if (gpInputManager)
        delete gpInputManager;
    gpInputManager = 0;
    if (gpExec)
        delete gpExec;
    gpExec = 0;
}

// donor PoL RVA 0x00096e21; preferred Buka symbol ?EarlySetup@@YIHXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.257149;margin=0.511941;shape=0.213;size=0.338;calls=0.600;alternate=pol20:int EarlySetup(void)@0x00096e21
VA(0x00450046, 0x116)
int EarlySetup(void) {
    int iCDRomErr;

    if (bEarlySetupDone)
        return 0;
    InitMainClasses();
    GetGraphicsInfo();
    ReadPrefs();
    if (!InterpretCommandLine())
        return 1;
    LogTruncate();
    iCDRomErr = SetupCDDrive();
    if (iCDRomErr == 1) {
        MessageBoxA((HWND)hwndApp, "Unable to access CD Drive.", "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == 2) {
        MessageBoxA((HWND)hwndApp,
                    "You must have the Heroes Win95 CD in the CD-ROM drive to play \nHeroes of "
                    "Might and Magic.  \n\nPlease insert the CD and try again.",
                    "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == 3) {
        MessageBoxA((HWND)hwndApp,
                    "Unable to change to the Heroes directory.  Please run the installation "
                    "program.",
                    "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == 4) {
        MessageBoxA((HWND)hwndApp,
                    "Unable to find the Heroes data files.  Please run the installation program.",
                    "Startup Error", MB_ICONHAND);
        exit(0);
    }
    InitVars();
    return 1;
}

// Buka 2.1 toupper; HoMM1 keeps the narrow character form.
VA(0x00450f7e, 0x3e)
char toupper(char character) {
    if (character >= 'a' && character <= 'z')
        return character - 32;
    else
        return character;
}

// Buka 2.1 InterpretCommandLine reduced to HoMM1's /I, /C, /S and /B switches.
VA(0x00450fbc, 0x288)
int InterpretCommandLine(void) {
    int i;
    int helpRequested = 0;
    int size;

    giDebugLevel = 0;
    giShowIntro = 1;
    gbColorMice = 0;
    gbSpecialMouseMasks = 1;
    giScreenScroll = 1;
    gbCheatMenus = 0;
    gbBlackoutPlayer = 1;
    strcpy(gMapName, "AES31000.map");
    strcpy(gFullMapName, "Claw ( Easy )");
    strcpy(gMapDescription, "The Griffons will protect you until you are ready to make your move.");

    size = strlen(gcCommandLine);
    for (i = 0; i < size; i++) {
        if (gcCommandLine[i] == '/' && i + 1 < size) {
            switch (toupper(gcCommandLine[i + 1])) {
                case 'I':
                    if (i + 2 < size)
                        giShowIntro = gcCommandLine[i + 2] - '0';
                    break;
                case 'C':
                    if (i + 2 < size)
                        gbColorMice = gcCommandLine[i + 2] - '0';
                    break;
                case 'S':
                    if (i + 2 < size)
                        gbNoSound = 1 - (gcCommandLine[i + 2] - '0');
                    break;
                case 'B':
                    if (i + 2 < size)
                        gbSpecialMouseMasks = gcCommandLine[i + 2] - '0';
                    break;
            }
        }
    }

    sprintf(cAggPathName, "%s%s", ".\\DATA\\", "heroes.agg");
    DEFAULT_AGGREGATE_NAME = cAggPathName;
    giFrameStep = 6;
    for (i = 0; i < 4; i++) {
        if (giNumHumanPlayers > i)
            gbHumanPlayer[i] = 1;
        else
            gbHumanPlayer[i] = 0;
    }
    if (giNumHumanPlayers == 1)
        gbBlackoutPlayer = 0;
    helpRequested = 0;
    return 1;
}

extern char* gInitMenuHelp[];
extern int giMenuCommand;

// Buka 2.1 InitMenuHandler reduced to HoMM1's right-click help and button
// release; the main menu draws its own hover frames.
VA(0x00451244, 0x1b6)
short InitMenuHandler(tag_message& message) {
    int handled = 0;
    int helpIndex;

    PollSound();
    if (message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
        if (message.payload.widget.command == WIDGET_NOTIFY_SELECT
            || message.payload.widget.command == WIDGET_NOTIFY_RIGHT_CLICK) {
            helpIndex = -1;
            switch (message.payload.widget.id) {
                case 1:
                    helpIndex = 0;
                    break;
                case 2:
                    helpIndex = 1;
                    break;
                case 3:
                    helpIndex = 2;
                    break;
                case 4:
                    helpIndex = 3;
                    break;
                case 5:
                    helpIndex = 4;
                    break;
                case 6:
                    ;
            }
            if (helpIndex >= 0)
                NormalDialog(gInitMenuHelp[helpIndex], 4, -1, -1, -1, 0, -1, 0, -1);
        }
    } else if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_DESELECT:
                if (message.payload.widget.id > 0 && message.payload.widget.id <= 6)
                    handled = 1;
                break;
            default:
                break;
        }
    }

    if (handled || giMenuCommand != -1) {
        gpWindowManager->m_dialogResult = message.payload.widget.id;
        message.payload.widget.command = message.payload.widget.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004513fa, 0x14)
short NullHandler(tag_message&) {
    return MESSAGE_DISPATCH_CONSUME;
}

// HoMM1 has seven neutral building slots before six per-faction dwellings.
VA(0x004515d9, 0x47)
char* GetBuildingName(int race, short building) {
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return gNeutralBuildingNames[building];
    else
        return gDwellingNames[building - BUILDING_SLOT_DWELLING_FIRST + race * 6];
}

VA(0x00451620, 0x9f)
void GetBuildingCost(int race, short building, int* const destination, int mageLevel) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            memcpy(destination, gMageBuildingCosts[mageLevel], RESOURCE_COUNT * sizeof(int));
        else
            memcpy(destination, gNeutralBuildingCosts[building], RESOURCE_COUNT * sizeof(int));
    } else {
        memcpy(
            destination,
            gDwellingCosts[building - BUILDING_SLOT_DWELLING_FIRST + race * 6],
            RESOURCE_COUNT * sizeof(int)
        );
    }
}

VA(0x004516bf, 0x1a)
char* GetMonsterName(int monster) {
    return gArmyNames[monster];
}

// donor PoL RVA 0x0009992c; preferred Buka symbol ?GetMonsterCost@@YIXHQAH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.424205;margin=0.383727;shape=0.192;size=0.855;calls=1.000;alternate=pol20:void GetMonsterCost(int, int * const)@0x0009992c
VA(0x004516d9, 0xe6)
void GetMonsterCost(int monster, int* const cost) {
    int index;
    for (index = 0; index < RESOURCE_COUNT; index++)
        cost[index] = 0;
    cost[RESOURCE_GOLD] = gMonsterDatabase[monster].cost;
    switch (monster) {
        case CREATURE_GENIE:
            cost[RESOURCE_GEMS] = 1;
            break;
        case CREATURE_PHOENIX:
            cost[RESOURCE_MERCURY] = 1;
            break;
        case CREATURE_CYCLOPS:
            cost[RESOURCE_CRYSTAL] = 1;
            break;
        case CREATURE_DRAGON:
            cost[RESOURCE_SULFUR] = 1;
            break;
    }
}

// donor PoL RVA 0x00099a6c; preferred Buka symbol ?CanBuild@@YIHPAVtown@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.375672;margin=0.371383;shape=0.277;size=0.517;calls=1.000;alternate=pol20:int CanBuild(class town *, int)@0x00099a6c
// HoMM1 retail returns the result in AL (xor al,al / mov al,1).
VA(0x004517bf, 0x144)
signed char CanBuild(town* t, short building) {
    mapCell* cell;
    unsigned short required;
    if (BitTest(gpGame->m_townBuiltToday, t->m_id))
        return 0;
    if (building != 6 && !(t->m_buildings & 0x40))
        return 0;
    if (building == 3) {
        cell = gpAdvManager->GetCell(t->m_x - 1, t->m_y + 1);
        if (cell->m_tileIndex < 20)
            return 1;
        else
            return 0;
    }
    if (building == BUILDING_SLOT_MAGE_GUILD && t->m_buildState >= 3)
        return 0;
    if (building == 5)
        return 0;
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return 1;
    required = gDwellingRequirements[building - BUILDING_SLOT_DWELLING_FIRST + t->m_type * 6];
    if ((t->m_buildings & required) == required)
        return 1;
    return 0;
}

// donor PoL RVA 0x00099d21; preferred Buka symbol ?CanBuy@@YIHPAVtown@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.384626;margin=0.370647;shape=0.216;size=0.621;calls=1.000;alternate=pol20:int CanBuy(class town *, int)@0x00099d21
// Retail returns a byte flag (xor al,al / mov al,1); philAI::CanBuyBHC tests al.
VA(0x00451903, 0xce)
signed char CanBuy(town* t, short type) {
    int cost[RESOURCE_COUNT];
    playerData* rec;
    int i;
    GetBuildingCost(
        t->m_type,
        type,
        cost,
        (t->m_buildings & 1) ? (t->m_buildState >= 3 ? 3 : t->m_buildState + 1) : 0
    );
    rec = &gpGame->m_players[giCurPlayer];
    for (i = 0; i < RESOURCE_COUNT; ++i) {
        if (rec->m_resources[i] < cost[i])
    return 0;
}
    return 1;
}

// HoMM1 keeps seven neutral value slots ahead of six per-faction dwellings.
VA(0x004519d1, 0x60)
int GetBuildingBaseResourceValue(int race, int building, int level) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            return gMageBaseResourceValues[level];
        else
            return gNeutralBaseResourceValues[building];
    } else {
        return gDwellingBaseResourceValues[building - BUILDING_SLOT_DWELLING_FIRST + race * 6];
    }
}

short WaitHandler(tag_message&);

// Buka 2.1 NormalDialog without HoMM2's timeout, saved resource globals,
// primary-skill/monster/secondary-skill slots and centered x; HoMM1 measures
// the text with a temporary bigfont.fnt and frames heroes with port%04d.icn.
VA(0x00451a31, 0xf03)
void NormalDialog(
    char* text,
    int dialogType,
    int x,
    int y,
    int firstResourceType,
    int firstResourceValue,
    int secondResourceType,
    int secondResourceValue,
    int showOrText
) {
    int resourceYPos;
    int maxIconHeight;
    char szFilename[NORMAL_DIALOG_FILENAME_LENGTH];
    char* amountText[NORMAL_DIALOG_RESOURCE_COUNT];
    int sizingHeight;
    int kind[NORMAL_DIALOG_RESOURCE_COUNT];
    iconWidget* iconPanel;
    int width;
    int resWidth;
    short bShowMessage;
    font* bigFont;
    int height;
    int i;
    int contentSize;
    int id;
    int iHeight;
    int resourceQty[NORMAL_DIALOG_RESOURCE_COUNT];
    tag_message message;
    int heightIndex;
    int lineCount;
    int resourceFrame;
    int frameHeight;
    textWidget* captionWidget;
    int resCenterX;
    char* szOr;

    resCenterX = 0;
    resourceYPos = 0;
    resourceFrame = 0;
    id = NORMAL_DIALOG_TEXT_WIDGET_FIRST_ID;
    resWidth = 0;
    iHeight = 0;
    bShowMessage = 1;
    kind[0] = firstResourceType;
    resourceQty[0] = firstResourceValue;
    kind[1] = secondResourceType;
    resourceQty[1] = secondResourceValue;

    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    lineCount = bigFont->LineLength(text, NORMAL_DIALOG_TEXT_LINE_WIDTH);
    gpResourceManager->Dispose(bigFont);
    contentSize = lineCount * NORMAL_DIALOG_TEXT_LINE_HEIGHT;
    if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        contentSize += NORMAL_DIALOG_BUTTON_AREA_HEIGHT;

    maxIconHeight = 0;
    for (i = 0; i < NORMAL_DIALOG_RESOURCE_COUNT; i++) {
        switch (kind[i]) {
            case NORMAL_DIALOG_ARTIFACT:
                sizingHeight = 76;
                break;
            case NORMAL_DIALOG_LUCK_BONUS:
                sizingHeight = 28;
                break;
            case NORMAL_DIALOG_LUCK_PENALTY:
                sizingHeight = 57;
                break;
            case NORMAL_DIALOG_MORALE_BONUS:
                sizingHeight = 62;
                break;
            case NORMAL_DIALOG_MORALE_PENALTY:
                sizingHeight = 59;
                break;
            case NORMAL_DIALOG_EXPERIENCE:
                sizingHeight = 76;
                break;
            case NORMAL_DIALOG_CREST:
                sizingHeight = 55;
                break;
            case NORMAL_DIALOG_HERO:
                sizingHeight = 111;
                break;
            case NORMAL_DIALOG_RESOURCE_GOLD:
                sizingHeight = 26;
                break;
            case RESOURCE_WOOD:
            case RESOURCE_MERCURY:
            case RESOURCE_ORE:
            case RESOURCE_SULFUR:
            case RESOURCE_CRYSTAL:
            case RESOURCE_GEMS:
                sizingHeight = 44;
                break;
            case NORMAL_DIALOG_SPELL:
                sizingHeight = 52;
                break;
            default:
                sizingHeight = 0;
                break;
        }
        if (sizingHeight > maxIconHeight)
            maxIconHeight = sizingHeight;
    }

    if (maxIconHeight > 0)
        contentSize += maxIconHeight + 12;
    heightIndex = (contentSize - 12) / NORMAL_DIALOG_WINDOW_ROW_HEIGHT;
    if (heightIndex > NORMAL_DIALOG_MAX_ROWS)
        heightIndex = NORMAL_DIALOG_MAX_ROWS;
    if (heightIndex <= 0 && dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        heightIndex = 1;
    width = NORMAL_DIALOG_WINDOW_WIDTH;
    height = heightIndex * NORMAL_DIALOG_WINDOW_ROW_HEIGHT + NORMAL_DIALOG_WINDOW_BASE_HEIGHT;

    if (x == -1 || x + width >= 639) {
        if (gpAdvManager->m_active == 1 && !gbHeroWindShowing && !gbOverviewShowing)
            x = NORMAL_DIALOG_ADVENTURE_X;
        else
            x = (640 - width) / 2;
    }
    if (y == -1 || y + height >= 479) {
        y = (480 - height) / 2;
        if (y > NORMAL_DIALOG_MAX_TOP)
            y = NORMAL_DIALOG_MAX_TOP;
    }

    sprintf(szFilename, "evntwin%d.bin", heightIndex);
    pNormalDialogWindow = new heroWindow(x, y, szFilename);
    if (!pNormalDialogWindow)
        MemError();

    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.payload.widget.data.value = NORMAL_DIALOG_BUTTON_FLAGS;
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_CANCEL && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        message.payload.widget.id = NORMAL_DIALOG_BUTTON_OK;
        pNormalDialogWindow->BroadcastMessage(message);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_OK && dialogType != NORMAL_DIALOG_TYPE_OK
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        message.payload.widget.id = NORMAL_DIALOG_BUTTON_CANCEL;
        pNormalDialogWindow->BroadcastMessage(message);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_YES_NO) {
        message.payload.widget.id = NORMAL_DIALOG_BUTTON_YES;
        pNormalDialogWindow->BroadcastMessage(message);
        message.payload.widget.id = NORMAL_DIALOG_BUTTON_NO;
        pNormalDialogWindow->BroadcastMessage(message);
    }

    for (i = 0; i < NORMAL_DIALOG_RESOURCE_COUNT; i++) {
        iconPanel = 0;
        captionWidget = 0;
        if (kind[i] == NORMAL_DIALOG_NO_RESOURCE)
            break;

        amountText[i] = (char*)malloc(NORMAL_DIALOG_TEXT_LENGTH);
        if (kind[i] <= NORMAL_DIALOG_RESOURCE_LAST) {
            if (resourceQty[i] > 0)
                sprintf(amountText[i], "%d", resourceQty[i]);
            else if (resourceQty[i] == 0)
                strcpy(amountText[i], "");
            else
                sprintf(amountText[i], "%d/day", -resourceQty[i]);
            strcpy(szFilename, "resource.icn");
            resourceFrame = kind[i];
        } else if (kind[i] == NORMAL_DIALOG_SPELL) {
            sprintf(amountText[i], "%s", gSpellNames[resourceQty[i]]);
            strcpy(szFilename, "spells.icn");
            resourceFrame = resourceQty[i];
        } else if (kind[i] == NORMAL_DIALOG_CREST) {
            sprintf(amountText[i], "%s", "");
            strcpy(szFilename, "brcrest.icn");
            resourceFrame = resourceQty[i];
        } else if (kind[i] == NORMAL_DIALOG_HERO) {
            sprintf(amountText[i], "%s", "");
            sprintf(szFilename, "surrendr.icn");
            resourceFrame = 4;
        } else if (kind[i] == NORMAL_DIALOG_EXPERIENCE
                   || kind[i] == NORMAL_DIALOG_MORALE_BONUS
                   || kind[i] == NORMAL_DIALOG_MORALE_PENALTY
                   || kind[i] == NORMAL_DIALOG_LUCK_BONUS
                   || kind[i] == NORMAL_DIALOG_LUCK_PENALTY) {
            strcpy(amountText[i], "");
            strcpy(szFilename, "expmrl.icn");
            resourceFrame = kind[i] - NORMAL_DIALOG_EXPMRL_FIRST;
            if (kind[i] == NORMAL_DIALOG_EXPERIENCE
                && resourceQty[i] != NORMAL_DIALOG_NO_VALUE)
                sprintf(amountText[i], "%d", resourceQty[i]);
        } else {
            strcpy(amountText[i], "");
            strcpy(szFilename, "resource.icn");
            resourceFrame = kind[i];
        }

        switch (kind[i]) {
            case NORMAL_DIALOG_ARTIFACT:
                resWidth = 76;
                sizingHeight = 76;
                break;
            case NORMAL_DIALOG_LUCK_BONUS:
                resWidth = 64;
                sizingHeight = 28;
                break;
            case NORMAL_DIALOG_LUCK_PENALTY:
                resWidth = 64;
                sizingHeight = 57;
                break;
            case NORMAL_DIALOG_MORALE_BONUS:
                resWidth = 64;
                sizingHeight = 62;
                break;
            case NORMAL_DIALOG_MORALE_PENALTY:
                resWidth = 64;
                sizingHeight = 59;
                break;
            case NORMAL_DIALOG_EXPERIENCE:
                resWidth = 64;
                sizingHeight = 64;
                break;
            case NORMAL_DIALOG_CREST:
                resWidth = 50;
                sizingHeight = 55;
                break;
            case NORMAL_DIALOG_HERO:
                resWidth = 111;
                sizingHeight = 105;
                break;
            case NORMAL_DIALOG_RESOURCE_GOLD:
                resWidth = 76;
                sizingHeight = 26;
                break;
            case RESOURCE_WOOD:
            case RESOURCE_MERCURY:
            case RESOURCE_ORE:
            case RESOURCE_SULFUR:
            case RESOURCE_CRYSTAL:
            case RESOURCE_GEMS:
                resWidth = 38;
                sizingHeight = 32;
                break;
            case NORMAL_DIALOG_SPELL:
                resWidth = 38;
                sizingHeight = 40;
                break;
        }

        if (strlen(amountText[i]) > 0)
            sizingHeight += NORMAL_DIALOG_RESOURCE_LABEL_HEIGHT;
        if (i == 0) {
            if (kind[1] == NORMAL_DIALOG_NO_RESOURCE)
                resCenterX = width / 2;
            else
                resCenterX = width / 3;
        } else {
            resCenterX = width * 2 / 3;
        }
        resourceYPos = height - sizingHeight - NORMAL_DIALOG_RESOURCE_BOTTOM_INSET;
        if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
            resourceYPos -= NORMAL_DIALOG_BUTTON_AREA_HEIGHT;
        if (maxIconHeight > sizingHeight)
            resourceYPos -= (maxIconHeight - sizingHeight) / 2;

        iconPanel = new iconWidget(
            resCenterX - resWidth / 2, resourceYPos, resWidth,
            sizingHeight, szFilename, resourceFrame, 0, -1, ICON_WIDGET_DRAW, 1);
        if (!iconPanel)
            MemError();
        pNormalDialogWindow->AddWidget(iconPanel, -1);
        if (kind[i] == NORMAL_DIALOG_ARTIFACT) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 6, resourceYPos + 6, 76, 76,
                "artifact.icn", resourceQty[i], 0, -1, ICON_WIDGET_DRAW, 1);
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, -1);
        }
        if (kind[i] == NORMAL_DIALOG_CREST) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 - 4, resourceYPos - 4, 58, 55,
                "brcrest.icn", 4, 0, -1, ICON_WIDGET_DRAW, 1);
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, -1);
        }
        if (kind[i] == NORMAL_DIALOG_HERO) {
            sprintf(szFilename, "port%04d.icn", resourceQty[i]);
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 5, resourceYPos + 5, 101, 95,
                szFilename, 0, 0, -1, ICON_WIDGET_DRAW, 1);
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, -1);
        }
        captionWidget = new textWidget(
            resCenterX - 50, resourceYPos + sizingHeight - 10, 100, 12,
            amountText[i], "smalfont.fnt", 1, id++, 0x200);
        if (!captionWidget)
            MemError();
        pNormalDialogWindow->AddWidget(captionWidget, -1);
    }

    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = NORMAL_DIALOG_TEXT_WIDGET_ID;
    message.payload.widget.data.text = text;
    pNormalDialogWindow->BroadcastMessage(message);

    if (showOrText == NORMAL_DIALOG_SHOW_OR_TEXT) {
        szOr = (char*)malloc(3);
        strcpy(szOr, "or");
        captionWidget = new textWidget(
            width / 2 - 17, resourceYPos + 30, 40, 12, szOr, "smalfont.fnt", 1,
            id++, 0x200);
        if (!captionWidget)
            MemError();
        pNormalDialogWindow->AddWidget(captionWidget, -1);
    }

    if (gpAdvManager->m_active == 1)
        gpMouseManager->SetPointer(0);
    else if (gpCombatManager->m_active == 1)
        gpMouseManager->SetPointer(6);

    if (dialogType == NORMAL_DIALOG_TYPE_WAIT_CANCEL || dialogType == NORMAL_DIALOG_TYPE_WAIT_OK) {
        gpWindowManager->DoDialog(pNormalDialogWindow, WaitHandler, 0);
    } else if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(pNormalDialogWindow, -1, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(pNormalDialogWindow);
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->DoDialog(pNormalDialogWindow, EventWindowHandler, 0);
    }
    delete pNormalDialogWindow;
}

// donor PoL RVA 0x000a2565; preferred Buka symbol ?UpdateNormalDialog@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.563703;margin=0.348381;shape=0.417;size=0.972;calls=1.000;alternate=pol20:void UpdateNormalDialog(char *)@0x000a2565
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x00452934, 0x6b)
void UpdateNormalDialog(char* text) {
    tag_message message;
    {
        short show = 1; // Retained from donor and retail stack frame.
        message.type = MESSAGE_WIDGET;
        message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
        message.payload.widget.id = 1;
        message.payload.widget.data.text = text;
        pNormalDialogWindow->BroadcastMessage(message);
        pNormalDialogWindow->DrawWindow(0, 0, NORMAL_DIALOG_FOREGROUND_WIDGET_LIMIT);
        pNormalDialogWindow
            ->DrawWindow(1, WINDOW_ALL_WIDGETS_LOW, NORMAL_DIALOG_BACKGROUND_WIDGET_LAST_ID);
    }
}

// Modem.cpp's wait-loop steps; Modem.h does not export them (declaring them
// there ahead of their definitions reorders Modem's own compare operands).
signed char GUIModemCommandExec(void);
signed char GUIModemResponseExec(void);
int WaitForDirectConnect(void);

// donor PoL RVA 0x00099e81; preferred Buka symbol ?WaitHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.445743;margin=0.444520;shape=0.204;size=0.951;calls=0.688;alternate=pol20:int WaitHandler(struct tag_message &)@0x00099e81
VA(0x0045299f, 0x1c5)
short WaitHandler(tag_message& message) {
    signed char result = 0;
    gbFunctionComplete = 1;
    PollSound();
    if (!gpSoundManager->MusicPlaying())
        gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.payload.widget.id) {
                    case 0x7800:
                    case 0x7801:
                    case 0x7802:
                        gbFunctionComplete = 0;
                        result = 1;
                        break;
                }
        }
    }
    if (!result) {
        switch (giWaitType) {
            case 0:
                result = WaitForOtherPlayer();
                break;
            case 1:
                result = WaitForHost();
                break;
            case 2:
                result = WaitForGuest();
                break;
            case 3:
                result = InitNetGuest();
                break;
            case 4:
                result = InitNetHost();
                break;
            case 5:
                result = GUIModemCommandExec();
                break;
            case 6:
                result = GUIModemResponseExec();
                break;
            case 7:
                result = WaitForDirectConnect();
                break;
        }
    }
    if (result) {
        gpWindowManager->m_dialogResult = 0x7801;
        message.type = MESSAGE_WIDGET;
        message.payload.widget.command = message.payload.widget.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 EventWindowHandler without HoMM2's dialog timeout and resource help.
VA(0x00452b64, 0x114)
short EventWindowHandler(tag_message& message) {
    if (!gpSoundManager->MusicPlaying())
        gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.payload.widget.id) {
                    case 0x385:
                    case 0x7800:
                    case 0x7801:
                    case 0x7802:
                    case 0x7803:
                    case 0x7805:
                    case 0x7806:
                        gpWindowManager->m_dialogResult = message.payload.widget.id;
                        message.payload.widget.command = message.payload.widget.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 TrueFalseDialogHandler.
VA(0x00452c78, 0x1c)
short TrueFalseDialogHandler(tag_message& message) {
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0009a52f; preferred Buka symbol ?PlayerDead@@YIXH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488269;margin=0.466685;shape=0.274;size=0.981;calls=0.750;alternate=pol20:void PlayerDead(int)@0x0009a52f
VA(0x00452c94, 0x16c)
void PlayerDead(int player) {
    playerData* currentPlayer;
    int i;
    gbRetreatWin = 0;
    currentPlayer = &gpGame->m_players[player];
    gpGame->m_playerDead[player] = 1;
    ++gpGame->m_deadPlayerCount;
    for (i = 0; i < GAME_MINE_COUNT; ++i) {
        if (gpGame->m_mineOwners[i] == player)
            gpGame->ClaimMine(i, -1);
    }
    for (i = currentPlayer->m_heroCount - 1; i >= 0; --i)
        gpGame->GetHero(currentPlayer->m_heroIds[i])->Deallocate();
    for (i = 0; i < 2; ++i) {
        if (gpGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]] == 0x40)
            gpGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]] = -1;
    }
    if (gbRemoteOn && gbHumanPlayer[player])
        HandleRemoteDeadPlayerExit(player);
}

// HoMM1's three-byte exit notice: game position, control hand-off, next player.
#pragma pack(push, 1)
struct playerExitMessage {
    signed char gamePosition;
    signed char takesControl;
    signed char nextPlayer;
};
#pragma pack(pop)
extern playerExitMessage gPlayerExitMessage;
extern int giHostGamePos;

// Buka 2.1 HandleRemoteDeadPlayerExit for HoMM1's two-player transport.
VA(0x00452e00, 0x99)
void HandleRemoteDeadPlayerExit(int position) {
    if (position == giThisGamePos) {
        if (!gpGame->TransmitSaveGame(REMOTE_BROADCAST_PLAYER, 1))
            ShutDown(0);
        RemoteCleanup();
    } else if (giNumHumanPlayers == 2) {
        giNumHumanPlayers--;
        gPlayerExitMessage.gamePosition = position;
        gPlayerExitMessage.takesControl = 0;
        TransmitRemoteData((char*)&gPlayerExitMessage, REMOTE_BROADCAST_PLAYER, 3, 30, 0, 0, REMOTE_MESSAGE_RELIABLE, 1);
        RemoteCleanup();
        gbHumanPlayer[position] = 0;
    }
}

// Buka 2.1 HandleRemoteSuddenExit; HoMM1 names the next human player itself.
VA(0x00452e99, 0xf1)
void HandleRemoteSuddenExit(void) {
    int next;
    if (!gbGameInitialized)
        return;
    gPlayerExitMessage.gamePosition = giThisGamePos;
    if (gbThisNetHumanPlayer[giCurPlayer]
        || (!gbHumanPlayer[giCurPlayer] && giHostGamePos == giThisGamePos)) {
        gPlayerExitMessage.takesControl = 1;
        next = giCurPlayer;
        next = (next + 1) % gpGame->m_playerCount;
        while (!gbHumanPlayer[next])
            next = (next + 1) % gpGame->m_playerCount;
        gPlayerExitMessage.nextPlayer = next;
    } else {
        gPlayerExitMessage.takesControl = 0;
    }
    TransmitRemoteData((char*)&gPlayerExitMessage, REMOTE_BROADCAST_PLAYER, 3, 30, 0, 0, REMOTE_MESSAGE_RELIABLE, 1);
}

// donor PoL RVA 0x000a07e3; preferred Buka symbol ?ReceiveRemotePlayerExit@@YIXUSPlayerExit@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.368727;margin=0.249960;shape=0.192;size=0.687;calls=0.800;alternate=pol20:void ReceiveRemotePlayerExit(struct SPlayerExit)@0x000a07e3
extern char* gColorNames[];

VA(0x00452f8a, 0x1ea)
// HoMM1 callers push four byte-sized values: player, an unused flag,
// elimination and timeout.
void ReceiveRemotePlayerExit(signed char position, signed char, signed char eliminated, signed char timedOut) {
    if (position == giThisGamePos) {
        sprintf(gText, "You have been eliminated from the game!!!");
        NormalDialog(gText, 1, -1, -1, -1, 0, -1, 0, -1);
        RemoteCleanup();
        gbGameOver = 1;
        giEndSequence = 0;
        return;
    }
    if (giNumHumanPlayers <= 2) {
        gpGame->SaveGame("PLYREXIT", 1);
        if (eliminated) {
            sprintf(gText, "%s player has been vanquished!", gColorNames[gpGame->m_players[position].Color()]);
            gText[0] -= 32;
            NormalDialog(gText, 1, 0x61, -1, 9, gpGame->m_players[position].Color(), -1, 0, -1);
            goto dropPlayer;
        } else {
            if (timedOut)
                sprintf(
                    gText,
                    "Player %d has been logged out of the game.  The current game has been saved as "
                    "'PLYREXIT'.  Do you wish to continue playing with a computer player filling in for "
                    "player %d?",
                    position + 1,
                    position + 1
                );
            else
                sprintf(
                    gText,
                    "Player %d is exiting the game.  The current game has been saved as 'PLYREXIT'.  Do "
                    "you wish to continue playing with a computer player filling in for player %d?",
                    position + 1,
                    position + 1
                );
            NormalDialog(gText, 2, -1, -1, -1, 0, -1, 0, -1);
        }
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        dropPlayer:
            if (giNumHumanPlayers == 2) {
                giNumHumanPlayers--;
                RemoteCleanup();
                gbHumanPlayer[position] = 0;
            }
        } else {
            RemoteCleanup();
            ShutDown(0);
        }
    }
}

// donor PoL RVA 0x0009a6c1; preferred Buka symbol ?CheckEndGame@@YIXHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.237398;margin=0.276870;shape=0.229;size=0.353;calls=0.309;alternate=pol20:void CheckEndGame(int, int)@0x0009a6c1
VA(0x00453174, 0x7d4)
void CheckEndGame(int) {}

// donor PoL RVA 0x0009c07c; preferred Buka symbol ?QuickViewWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.435968;margin=0.219505;shape=0.250;size=0.859;calls=0.600;alternate=pol20:void QuickViewWait(void)@0x0009c07c
VA(0x00453948, 0x95)
void QuickViewWait(void) {
    tag_message event;
    int done = 0;
    while (!done) {
        PollSound();
        Process1WindowsMessage();
        event = gpInputManager->GetEvent();
        if (event.type == MESSAGE_RIGHT_BUTTON_UP || event.type == MESSAGE_LEFT_BUTTON_DOWN
            || event.type == MESSAGE_LEFT_BUTTON_UP)
            done = 1;
        else
            done = 0;
    }
}

// donor PoL RVA 0x0009c111; preferred Buka symbol ?InitVars@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.679533;margin=0.555045;shape=0.387;size=0.991;calls=0.692;strings=mnuAdv|mnuCmbt|mnuDflt;alternate=pol20:void InitVars(void)@0x0009c111
VA(0x004539dd, 0x1cb)
void InitVars(void) {
    int i;
    NULL_SAMPLE2.pSample = 0;
    NULL_SAMPLE2.pMem = (struct _SAMPLE*)NULL_SAMPLE2.pSample;
    gbMapExtraCleared = 1;
    gGameCommand = -1;
    gPalette = 0;
    gpPhilAI->m_debugFont = 0;
    gbCombatSurrender = 0;
    gpGame->m_viewArmyResult = 0;
    gbInNewGameSetup = 0;
    for (i = 0; i < 140; i++)
        giGroundToTerrain[i] = i / 20;
    for (i = 0; i < FINDPATH_TERRAIN_COUNT; i++) {
        giTerrainCost[i][0] = TerrainStepCost(i, 0);
        giTerrainCost[i][1] = TerrainStepCost(i, 1);
    }
    strcpy(cNetBoxLine[0], "");
    strcpy(cNetBoxLine[1], "");
    for (i = 0; i < 255; i++)
        ppMapExtra[i] = 0;
    hmnuDflt = LoadMenuA((HINSTANCE)hInstApp, "mnuDflt");
    hmnuCmbt = LoadMenuA((HINSTANCE)hInstApp, "mnuCmbt");
    hmnuAdv = LoadMenuA((HINSTANCE)hInstApp, "mnuAdv");
    hmnuTown = LoadMenuA((HINSTANCE)hInstApp, "mnuTown");
    LogStr("LoadMenus", (long)hmnuDflt, (long)hmnuCmbt, (long)hmnuAdv, (long)hmnuTown, (long)hInstApp);
}

// donor PoL RVA 0x0009c312; preferred Buka symbol ?ShowMoraleInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.469331;margin=0.613523;shape=0.400;size=0.774;calls=0.649;alternate=pol20:void game::ShowMoraleInfo(class hero *, int)@0x0009c312
VA(0x00453ba8, 0x450)
void game::ShowMoraleInfo(class hero*, int) {}

// donor PoL RVA 0x0009c92d; preferred Buka symbol ?ShowLuckInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.456267;margin=0.157936;shape=0.493;size=0.606;calls=0.556;alternate=pol20:void game::ShowLuckInfo(class hero *, int)@0x0009c92d
VA(0x00453ff8, 0x1f7)
void game::ShowLuckInfo(class hero*, int) {}

VA(0x004541ef, 0x70)
void ClearMapExtra(void) {
    int i;
    for (i = 0; i < 255; i++) {
        if (ppMapExtra[i]) {
            free(ppMapExtra[i]);
            ppMapExtra[i] = 0;
        }
    }
    gbMapExtraCleared = 1;
}

// HoMM1 score-to-monster tables pair a threshold word with a monster word.
VA(0x0045425f, 0x8e)
short GetMonType(int score, int highScoreType) {
    int index;
    for (index = 27; index >= 0; index--) {
        if (highScoreType == 0) {
            if (giScoreCampaignMon[index][0] >= score)
                return giScoreCampaignMon[index][1];
        } else {
            if (giScoreMon[index][0] <= score)
                return giScoreMon[index][1];
        }
    }
    return giScoreMon[0][1];
}

// donor PoL RVA 0x0009ce14; preferred Buka symbol ?AddScoreToHighScore@@YIHHHHHPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701795;margin=0.122445;shape=0.377;size=0.950;calls=0.929;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:int AddScoreToHighScore(int, int, int, int, char *)@0x0009ce14
VA(0x004542ed, 0x3d2)
int AddScoreToHighScore(int, int, int, int, char*) {
    return 0;
}

// donor PoL RVA 0x0009d2c0; preferred Buka symbol ?BVResMsg@@YIXPADHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.598508;margin=0.532475;shape=0.481;size=0.968;calls=1.000;alternate=pol20:void BVResMsg(char *, int, int)@0x0009d2c0
VA(0x004546bf, 0x5b)
void BVResMsg(char* s, int res, int qty) {
    giBottomViewOverride = 5;
    giBottomViewOverrideEndTime = KBTickCount() + 5000;
    giBottomViewResource = res;
    giBottomViewResourceQty = qty;
    strcpy(gcBottomViewText, s);
    gpAdvManager->UpdBottomView(1, 1, 1);
}

// Buka 2.1 GOut.
VA(0x0045471a, 0x2e)
void GOut(char* text) {
    if (gpAdvManager->m_active == 1)
        AiPrint(text);
}

// donor PoL RVA 0x0009d3a7; preferred Buka symbol ?WaitForOtherPlayer@@YIHXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.581419;margin=0.608732;shape=0.409;size=0.995;calls=1.000;alternate=pol20:int WaitForOtherPlayer(void)@0x0009d3a7
// HoMM1 maps every remote position other than the host to the one opponent slot.
VA(0x00454748, 0x39)
signed char NetPosToGamePos(int netPos) {
    if (netPos == 0)
        return 0;
    else if (netPos > 0)
        return 1;
    return -1;
}

VA(0x00454781, 0xda)
signed char WaitForOtherPlayer(void) {
    int result = 0;
    RemoteMessage* data;
    PollSound();
    data = (RemoteMessage*)GetRemoteData(1);
    if (data && data->type == REMOTE_MESSAGE_RELIABLE) {
        switch (data->command) {
            case BOX_REMOTE_SETUP:
                memcpy(gbGamePosToNetPos, data->payload.data, 4);
                giThisGamePos = NetPosToGamePos(giThisNetPos);
                giHostGamePos = NetPosToGamePos(0);
                break;
            case BOX_REMOTE_SAVE:
                result = gpGame->ReceiveSaveGame(data->payload.saveSize, data->sender);
                break;
        }
    }
    return result;
}

// donor PoL RVA 0x0009d4a6; preferred Buka symbol ?PopNetBox@@YIXPADH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.593152;margin=0.055238;shape=0.393;size=0.624;calls=0.688;strings=netbox.bin;alternate=pol20:void PopNetBox(char *, int)@0x0009d4a6
VA(0x0045485b, 0x6f4)
void PopNetBox(char *) {}

// Buka 2.1 AddNetBoxLine reduced to HoMM1's two uncoloured lines.
VA(0x00454f4f, 0x3b)
void AddNetBoxLine(char* text) {
    strcpy(cNetBoxLine[0], cNetBoxLine[1]);
    strcpy(cNetBoxLine[1], text);
}

// donor PoL RVA 0x0009e0f2; preferred Buka symbol ?ShutDown@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.466886;margin=0.632520;shape=0.403;size=0.708;calls=0.667;alternate=pol20:void ShutDown(char *)@0x0009e0f2
VA(0x00454f8a, 0x14f)
void ShutDown(char* message) {
    char buffer[768];
    if (bInShutDown)
        return;
    bInShutDown = 1;
    gbClosingApp = 1;
    buffer[0] = 0;
    if (message) {
        strcpy(buffer, message);
        SetFullScreenStatus(0);
        LogStr(buffer);
        MessageBoxA((HWND)hwndApp, buffer, "Unexpected Program Termination", MB_ICONHAND);
    }
    ClearMapExtra();
    UnloadSystemwideIcons();
    if (gbRemoteOn)
        HandleRemoteSuddenExit();
    if (gPalette) {
        gpResourceManager->Dispose(gPalette);
        gPalette = 0;
    }
    if (gpPhilAI->m_debugFont) {
        gpResourceManager->Dispose(gpPhilAI->m_debugFont);
        gpPhilAI->m_debugFont = 0;
    }
    gpExec->ShutDownSystem();
    RemoteCleanup();
    if (gEventHandle) {
        CloseHandle(gEventHandle);
        gEventHandle = 0;
    }
    DeleteMainClasses();
    AppExit();
    PrintMemoryLeaks();
    exit(0);
}

// donor PoL RVA 0x0009e306; preferred Buka symbol ?FileError@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.316461;margin=0.125092;shape=0.216;size=0.484;calls=0.500;alternate=pol20:void FileError(char *)@0x0009e306
VA(0x004550d9, 0x4a)
void FileError(char* filename) {
    char message[200];
    LogStr("File Error");
    sprintf(message, "Error opening file %s!", filename);
    ShutDown(message);
}

// @early-stop
// tu-cumulative: logic + all 14 frame slots byte-exact (od_oracle-verified). The only
// residual (coffcmp: 40 bytes, all in the two brightness averages + the minDist test)
// is a /Od operand-evaluation-order difference this cl renders vs retail: the 3-term
// sum `p[2]+p[0]+p[1]` reads +2,+1,+0 here but +2,+0,+1 in retail, and the `d>p`
// compare loads the other operand first. Not source-steerable (probed every term
// ordering, explicit grouping, `|0`, and an inline helper — all identical here).

// donor PoL RVA 0x00009e89; preferred Buka symbol ?Overview@game@@QAEXXZ
// donor Buka TU SOURCE/Overview; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.482449;margin=0.238001;shape=0.359;size=0.797;calls=0.763;alternate=pol20:void game::Overview(void)@0x00009e89
VA(0x00455123, 0x3be)
void game::Overview(void) {}

// donor PoL RVA 0x0009e900; preferred Buka symbol ?CongratsWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.447463;margin=0.065171;shape=0.300;size=0.684;calls=1.000;alternate=pol20:void CongratsWait(void)@0x0009e900
VA(0x004554e1, 0xb1)
void CongratsWait(void) {
    int cmd = 0;
    signed char finished = 0;
    tag_message message;
    gpInputManager->Flush();
    while (!finished) {
        PollSound();
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        if (message.type == MESSAGE_KEY_DOWN || message.type == MESSAGE_LEFT_BUTTON_DOWN
            || message.type == MESSAGE_LEFT_BUTTON_UP || message.type == MESSAGE_RIGHT_BUTTON_DOWN
            || message.type == MESSAGE_RIGHT_BUTTON_UP)
            finished = 1;
    }
}

// Buka 2.1 GetDataEntry without the prompt-sized window and textEntryWidget.
VA(0x00455592, 0x1c7)
void GetDataEntry(char* prompt, char* destination, int maximumLength, char* initialText) {
    short widgetId = 10;
    tag_message message;
    char textBuffer[100];

    gpMouseManager->SetPointer("advmice.mse", 0);
    cDEDest = destination;
    iDEMaxLen = maximumLength;
    strcpy(cDEDest, "");
    DataEntryWin = new heroWindow(0xb1, 0x14, "dataentr.bin");
    if (!DataEntryWin)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 1;
    message.payload.widget.data.text = prompt;
    DataEntryWin->BroadcastMessage(message);
    if (initialText)
        strcpy(textBuffer, initialText);
    else
        strcpy(textBuffer, "");
    message.payload.widget.id = 10;
    message.payload.widget.data.text = textBuffer;
    DataEntryWin->BroadcastMessage(message);
    strcpy(destination, textBuffer);
    bDataEntryTime = 0;
    gpWindowManager->DoDialog(DataEntryWin, DataEntryWindowHandler, 0);
    delete DataEntryWin;
}

VA(0x00455759, 0x1d9)
short DataEntryWindowHandler(tag_message& message) {
    short widgetId = 10;

    if (bDataEntryTime == 0) {
        ++bDataEntryTime;
        message.type = MESSAGE_LEFT_BUTTON_DOWN;
        message.payload.mouse.x = 0xc3;
        message.payload.mouse.y = 0x9a;
        DataEntryWin->BroadcastMessage(message);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (bDataEntryTime == 1) {
        ++bDataEntryTime;
        goto gotText;
    }
    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.payload.widget.id) {
                    case 10:
                    gotText:
                        message.type = MESSAGE_WIDGET;
                        message.payload.widget.id = 10;
                        message.payload.widget.command = WIDGET_COMMAND_GET_TEXT;
                        DataEntryWin->BroadcastMessage(message);
                        if (strlen(message.payload.widget.data.text) == 0) {
                            break;
                        } else {
                            memset(cDEDest, 0, iDEMaxLen);
                            strncpy(cDEDest, message.payload.widget.data.text, iDEMaxLen - 1);
                        }
                        message.type = MESSAGE_WIDGET;
                        message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
                        message.payload.widget.id = 10;
                        message.payload.widget.data.text = cDEDest;
                        DataEntryWin->BroadcastMessage(message);
                        DataEntryWin->DrawWindow(1, 10, 10);
                        gpWindowManager->m_dialogResult = message.payload.widget.id;
                        message.payload.widget.command = message.payload.widget.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                }
        }
    }
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0009e999; preferred Buka symbol ?LoadPlaySample@@YIPAVsample@@PAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.524829;margin=1.189024;shape=0.423;size=0.802;calls=1.000;alternate=pol20:struct SAMPLE2 LoadPlaySample(char *)@0x0009e999
VA(0x00455932, 0x51)
SAMPLE2 LoadPlaySample(char* name) {
    SAMPLE2 s;
    s.pSample = gpResourceManager->GetSample(name);
    if (s.pSample) {
        s.pSample->m_playbackData.channelType = SAMPLE_PLAYBACK_CHANNEL_GROUP;
        s.pMem = gpSoundManager->MemorySample(s.pSample);
    }
    return s;
}

// donor PoL RVA 0x0009e9ed; preferred Buka symbol ?WaitEndSample@@YIXPAPAVsample@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479563;margin=0.490944;shape=0.207;size=0.957;calls=1.000;alternate=pol20:void WaitEndSample(struct SAMPLE2, int)@0x0009e9ed
VA(0x00455983, 0x8a)
void WaitEndSample(SAMPLE2 s, int waitTime) {
    if (waitTime < 0)
        waitTime = 4000;
    long endTime = KBTickCount() + waitTime;
    if (s.pMem) {
        while (gpSoundManager->DigitalReport(s.pMem, 4) && KBTickCount() < endTime) {
            Process1WindowsMessage();
            PollSound();
        }
    }
    if (s.pSample)
        gpResourceManager->Dispose(s.pSample);
}

// donor PoL RVA 0x0009ea7c; preferred Buka symbol ?MemError@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.499168;margin=0.828160;shape=0.176;size=0.610;calls=1.000;strings=Out of Memory;alternate=pol20:void MemError(void)@0x0009ea7c
VA(0x00455a0d, 0x7b)
void MemError(void) {
    if (gbInMemError)
        return;
    gbInMemError = 1;
    LogStr("Out of Memory");
    sprintf(
        gText,
        "\n\n%s\n%s\n%d%s\n%d%s\n\n",
        gcMemoryErrorTitle,
        gcMemoryRequirements,
        giRequiredExtendedMemory,
        gcExtendedMemoryUnits,
        giRequiredConventionalMemory,
        gcConventionalMemoryUnits
    );
    ShutDown(gText);
}

// Buka 2.1 MiscRuntime MemSize: a fixed reported memory size.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x00455a88, 0x15)
int MemSize(int) {
    return 16034;
}

// Buka 2.1 CheckMem without HoMM2's memory globals.
VA(0x00455a9d, 0x12)
signed char CheckMem(void) {
    return 1;
}

// Campaign maps rename the town at a fixed position (x, y, then the name).
#pragma pack(push, 1)
struct campaignTownName {
    signed char x;
    signed char y;
    char name[83];
};
#pragma pack(pop)
extern campaignTownName gCampaignTownNames[];
extern char* gTownNames[];

// Buka 2.1 GetTownName; HoMM1 towns carry a name index, and campaign maps
// override one town by position.
VA(0x00455aaf, 0xdc)
char* GetTownName(signed char i) {
    town* townPointer = gpGame->GetTown(i);
    if (gpGame->m_campaignType > 0
        && gCampaignTownNames[gpGame->m_campaignScenario].x >= 0
        && gCampaignTownNames[gpGame->m_campaignScenario].x == townPointer->m_x
        && gCampaignTownNames[gpGame->m_campaignScenario].y == townPointer->m_y)
        return gCampaignTownNames[gpGame->m_campaignScenario].name;
    return gTownNames[townPointer->m_threat];
}

// Buka 2.1 Misc IsCDDrive.
VA(0x00455b8b, 0x51)
int IsCDDrive(int driveIndex) {
    sprintf(gText, "A:\\");
    gText[0] += driveIndex;
    return GetDriveTypeA(gText) == DRIVE_CDROM;
}

VA(0x00455bdc, 0x64)
void LoadSystemwideIcons(void) {
    gBuyBuildIcons = gpResourceManager->GetIcon("buybuild.icn");
    gSystemIcons = gpResourceManager->GetIcon("system.icn");
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    smallFont = gpResourceManager->GetFont("smalfont.fnt");
}

VA(0x00455c40, 0x54)
void UnloadSystemwideIcons(void) {
    gpResourceManager->Dispose(gBuyBuildIcons);
    gpResourceManager->Dispose(gSystemIcons);
    gpResourceManager->Dispose(bigFont);
    gpResourceManager->Dispose(smallFont);
}

// Retail empty lifecycle hook; Buka and PoL KB correspondence.
VA(0x00455c94, 0x10)
void EarlyShutDownSystem(void) {}

// Buka 2.1 GameUnsaved.
VA(0x00455ca4, 0x7e)
int GameUnsaved(void) {
    if ((gpAdvManager && gpAdvManager->m_active == 1)
        || (gpCombatManager && gpCombatManager->m_active == 1)
        || (gpTownManager && gpTownManager->m_active == 1))
        return 1;
    else
        return 0;
}

// donor PoL RVA 0x0009ec05; preferred Buka symbol ?HandleAppSpecificMenuCommands@@YIHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.410709;margin=0.595745;shape=0.257;size=0.699;calls=0.542;alternate=pol20:int HandleAppSpecificMenuCommands(int)@0x0009ec05
VA(0x00455d22, 0x629)
int HandleAppSpecificMenuCommands(int) {
    return 0;
}

// HoMM1 menu ids: music 0x9c50-0x9c5a, sound 0x9c5c-0x9c66, walk speed
// 0x9c68-0x9c6c, then the music-source, route and blackout toggles.
VA(0x0045634b, 0x3b7)
void UpdateSystemOptionsMenu(void) {
    int checkedCommand;
    int menuCommand;

    if (!gConfig.gfx[giCurExe].showMenu)
        return;
    if (!hmnuApp)
        return;
    if (hmnuApp != hmnuAdv)
        return;

    for (menuCommand = 0x9c50; menuCommand <= 0x9c5a; menuCommand++)
        CheckMenuItem((HMENU)hmnuApp, menuCommand, MF_UNCHECKED);
    switch (gConfig.musicVolume) {
        case 1:
            checkedCommand = 0x9c51;
            break;
        case 2:
            checkedCommand = 0x9c52;
            break;
        case 3:
            checkedCommand = 0x9c53;
            break;
        case 4:
            checkedCommand = 0x9c54;
            break;
        case 5:
            checkedCommand = 0x9c55;
            break;
        case 6:
            checkedCommand = 0x9c56;
            break;
        case 7:
            checkedCommand = 0x9c57;
            break;
        case 8:
            checkedCommand = 0x9c58;
            break;
        case 9:
            checkedCommand = 0x9c59;
            break;
        case 10:
            checkedCommand = 0x9c5a;
            break;
        default:
            checkedCommand = 0x9c50;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);

    for (menuCommand = 0x9c5c; menuCommand <= 0x9c66; menuCommand++)
        CheckMenuItem((HMENU)hmnuApp, menuCommand, MF_UNCHECKED);
    switch (gConfig.soundVolume) {
        case 1:
            checkedCommand = 0x9c5d;
            break;
        case 2:
            checkedCommand = 0x9c5e;
            break;
        case 3:
            checkedCommand = 0x9c5f;
            break;
        case 4:
            checkedCommand = 0x9c60;
            break;
        case 5:
            checkedCommand = 0x9c61;
            break;
        case 6:
            checkedCommand = 0x9c62;
            break;
        case 7:
            checkedCommand = 0x9c63;
            break;
        case 8:
            checkedCommand = 0x9c64;
            break;
        case 9:
            checkedCommand = 0x9c65;
            break;
        case 10:
            checkedCommand = 0x9c66;
            break;
        default:
            checkedCommand = 0x9c5c;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);

    for (menuCommand = 0x9c68; menuCommand <= 0x9c6c; menuCommand++)
        CheckMenuItem((HMENU)hmnuApp, menuCommand, MF_UNCHECKED);
    switch (gConfig.walkSpeed) {
        case 4:
            checkedCommand = 0x9c68;
            break;
        case 3:
            checkedCommand = 0x9c69;
            break;
        case 2:
            checkedCommand = 0x9c6a;
            break;
        case 1:
            checkedCommand = 0x9c6b;
            break;
        default:
            checkedCommand = 0x9c6c;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);
    CheckMenuItem((HMENU)hmnuApp, 0x9c6d, gConfig.musicSource ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem((HMENU)hmnuApp, 0x9c6e, gConfig.showRoute ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem((HMENU)hmnuApp, 0x9c6f,
                  1 - gConfig.blackoutComputer ? MF_CHECKED : MF_UNCHECKED);
}

VA(0x00456702, 0x99)
void CleanUpMenus(void) {
    if (hmnuApp) {
        SetMenu((HWND)hwndApp, 0);
        if (hmnuAdv)
            DestroyMenu((HMENU)hmnuAdv);
        if (hmnuDflt)
            DestroyMenu((HMENU)hmnuDflt);
        if (hmnuCmbt)
            DestroyMenu((HMENU)hmnuCmbt);
        if (hmnuTown)
            DestroyMenu((HMENU)hmnuTown);
    }
    hmnuApp = 0;
}

VA(0x0045679b, 0x24)
void UpdateAppSpecificMenus(void* hMenu) {
    if (hmnuAdv == hMenu)
        UpdateSystemOptionsMenu();
}

VA(0x004567bf, 0x22)
void EarlyResizeWindow(int, int, int, int) {
    if (gbClosingApp)
        return;
}
