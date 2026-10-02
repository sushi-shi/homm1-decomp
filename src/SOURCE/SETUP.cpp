// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/SETUP.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/comwin.h>
#include <SOURCE/Modem.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <string.h>

// clang-format off
H1_ENUM_BEGIN(SetupDialogResult)
    DIALOG_CANCEL = 0x7801
H1_ENUM_END(SetupDialogResult)

// The setup dialogs' numbered choice buttons (BaseSetupHandler accepts ids
// 1..1000): each game::Setup* maps CHOICE_n to its option and the handlers
// show help row n - 1 (Buka 2.1 SETUP.cpp SetupDialogChoice/SetupHelpIndex).
H1_ENUM_BEGIN(SetupDialogChoice)
    CHOICE_ONE = 1,
    CHOICE_TWO = 2,
    CHOICE_THREE = 3,
    CHOICE_FOUR = 4
H1_ENUM_END(SetupDialogChoice)

H1_ENUM_BEGIN(SetupHelpIndex)
    NO_HELP = -1,
    HELP_FIRST = 0
H1_ENUM_END(SetupHelpIndex)
// clang-format on

// Retail stpcmpgn.bin dialog driven by SetupCampaignGameHandler: HoMM1's
// game::SetupCampaignGame, not the HoMM2 trading post the graph proposed.
VA(0x004567f0, 0x164)
signed char game::SetupCampaignGame(void) {
    heroWindow* window = new heroWindow(400, 35, "stpcmpgn.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupCampaignGameHandler, 0);
    delete window;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            giCampaignChoice = 1;
            break;
        case CHOICE_TWO:
            giCampaignChoice = 2;
            break;
        case CHOICE_THREE:
            giCampaignChoice = 3;
            break;
        case CHOICE_FOUR:
            giCampaignChoice = 4;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

// donor PoL RVA 0x00010ebf; preferred Buka symbol ?SetupBaud@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.648115;margin=0.123105;shape=0.395;size=0.777;calls=0.833;strings=stpbaud.bin;alternate=pol20:int game::SetupBaud(void)@0x00010ebf
VA(0x00456954, 0x190)
signed char game::SetupBaud(void) {
    heroWindow* window = new heroWindow(400, 35, "stpbaud.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupBaudHandler, 0);
    delete window;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            gConfig.baudRate[gbDirectConnect] = 2400;
            break;
        case CHOICE_TWO:
            gConfig.baudRate[gbDirectConnect] = 9600;
            break;
        case CHOICE_THREE:
            gConfig.baudRate[gbDirectConnect] = 19200;
            break;
        case CHOICE_FOUR:
            gConfig.baudRate[gbDirectConnect] = 38400;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

// donor PoL RVA 0x00011000; preferred Buka symbol ?SetupComPort@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.646978;margin=0.131942;shape=0.348;size=0.872;calls=0.750;strings=stpcom.bin;alternate=pol20:int game::SetupComPort(void)@0x00011000
VA(0x00456ae4, 0x222)
signed char game::SetupComPort(void) {
    char initStr[40];

    heroWindow* window = new heroWindow(400, 35, "stpcom.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupComPortHandler, 0);
    delete window;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            gConfig.comPort[gbDirectConnect] = 1;
            break;
        case CHOICE_TWO:
            gConfig.comPort[gbDirectConnect] = 2;
            break;
        case CHOICE_THREE:
            gConfig.comPort[gbDirectConnect] = 3;
            break;
        case CHOICE_FOUR:
            gConfig.comPort[gbDirectConnect] = 4;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    if (!SetupBaud())
        return 0;
    if (!gbDirectConnect) {
        strcpy(gConfig.modemInitString, "ATZ");
        sprintf(gText, "%s", gConfig.modemInitString);
        GetDataEntry("Please enter any special initialization string required by your modem, or "
                     "hit 'ENTER' to accept the default.",
                     initStr, 40, gText);
        strcpy(gConfig.modemInitString, initStr);
    }
    WritePrefs();
    return 1;
}

// donor PoL RVA 0x00011200; preferred Buka symbol ?SetupHotSeatGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.510359;margin=0.105262;shape=0.279;size=0.627;calls=0.545;strings=stphotst.bin;alternate=pol20:int game::SetupHotSeatGame(void)@0x00011200
VA(0x00456d06, 0x15d)
signed char game::SetupHotSeatGame(void) {
    heroWindow* window = new heroWindow(400, 35, "stphotst.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupHotSeatGameHandler, 0);
    delete window;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            giNumHumanPlayers = 2;
            break;
        case CHOICE_TWO:
            giNumHumanPlayers = 3;
            break;
        case CHOICE_THREE:
            giNumHumanPlayers = 4;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

// donor PoL RVA 0x00011438; preferred Buka symbol ?SetupNetworkGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.778953;margin=0.116684;shape=0.550;size=0.938;calls=1.000;strings=stpnet.bin;alternate=pol20:int game::SetupNetworkGame(void)@0x00011438
VA(0x00456e63, 0x133)
signed char game::SetupNetworkGame(void) {
    heroWindow* window = new heroWindow(400, 35, "stpnet.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupNetworkGameHandler, 0);
    delete window;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            iMPExtendedType = REMOTE_GAME_NETWORK_HOST;
            break;
        case CHOICE_TWO:
            iMPExtendedType = REMOTE_GAME_NETWORK_GUEST;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

// donor PoL RVA 0x00011795; preferred Buka symbol ?SetupModemGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.697331;margin=0.184673;shape=0.396;size=0.996;calls=0.720;strings=stpdc.bin|stpdccfg.bin|stpmcfg.bin;alternate=pol20:int game::SetupModemGame(void)@0x00011795
VA(0x00456f96, 0x333)
signed char game::SetupModemGame(void) {
    heroWindow* window;

    if (gbDirectConnect) {
        if (gConfig.comPort[gbDirectConnect] == 0)
            window = new heroWindow(400, 35, "stpdc.bin");
        else
            window = new heroWindow(400, 35, "stpdccfg.bin");
    } else {
        if (gConfig.comPort[gbDirectConnect] == 0)
            window = new heroWindow(400, 35, "stpmodem.bin");
        else
            window = new heroWindow(400, 35, "stpmcfg.bin");
    }
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupModemGameHandler, 0);
    delete window;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            iMPExtendedType = REMOTE_GAME_MODEM_HOST;
            if (gConfig.comPort[gbDirectConnect] == 0) {
                if (!SetupComPort())
                    return 0;
            }
            if (!gbDirectConnect)
                GetDataEntry("Please enter the telephone number.", numbuf, 35, NULL);
            break;
        case CHOICE_TWO:
            iMPExtendedType = REMOTE_GAME_MODEM_GUEST;
            if (gConfig.comPort[gbDirectConnect] == 0 && !SetupComPort())
                return 0;
            break;
        case CHOICE_THREE:
            gbDoModemConfig = 1;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

// donor PoL RVA 0x00011aac; preferred Buka symbol ?SetupMultiPlayerGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.675100;margin=0.160042;shape=0.444;size=0.973;calls=0.529;strings=stpmp.bin;alternate=pol20:int game::SetupMultiPlayerGame(void)@0x00011aac
VA(0x004572c9, 0x218)
signed char game::SetupMultiPlayerGame(void) {
    int loop;

    heroWindow* window = new heroWindow(400, 35, "stpmp.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupMultiPlayerGameHandler, 0);
    delete window;

    gbDirectConnect = 0;
    switch (gpWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            iMPBaseType = MULTIPLAYER_BASE_HOT_SEAT;
            if (!SetupHotSeatGame())
                return 0;
            break;
        case CHOICE_TWO:
            iMPBaseType = MULTIPLAYER_BASE_NETWORK;
            if (!SetupNetworkGame())
                return 0;
            break;
        case CHOICE_FOUR:
            gbDirectConnect = 1;
            goto setupModem;
        case CHOICE_THREE:
            gbDirectConnect = 0;
        setupModem:
            iMPBaseType = MULTIPLAYER_BASE_MODEM;
            loop = 1;
            while (loop) {
                if (!SetupModemGame())
                    return 0;
                if (gbDoModemConfig) {
                    gbDoModemConfig = 0;
                    if (!SetupComPort())
                        return 0;
                } else {
                    loop = 0;
                }
            }
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

// Buka 2.1 game::SetupGame without the expansion campaign; the menu shortcuts
// keep separate restart and load command ids.
VA(0x004574e1, 0x486)
signed char game::SetupGame(signed char newGame) {
    heroWindow* window;
    int result;

    result = 1;
    iMPExtendedType = 10;
    iMPBaseType = 10;
    giNumHumanPlayers = 1;
    gbWaitForRemoteReceive = 0;
    gbDirectConnect = 0;
    gbInSetupDialog = 1;

    if (giMenuCommand != -1) {
        switch (giMenuCommand) {
            case APP_MENU_NEW_CAMPAIGN_IRONFIST:
                giCampaignChoice = 1;
                break;
            case APP_MENU_NEW_CAMPAIGN_SLAYER:
                giCampaignChoice = 2;
                break;
            case APP_MENU_NEW_CAMPAIGN_LAMANDA:
                giCampaignChoice = 3;
                break;
            case APP_MENU_NEW_CAMPAIGN_ALAMAR:
                giCampaignChoice = 4;
                break;
            case APP_MENU_NEW_STANDARD_GAME:
            case APP_MENU_LOAD_STANDARD_GAME:
                break;
            case APP_MENU_LOAD_CAMPAIGN_GAME:
                giCampaignChoice = 1;
                break;
            case APP_MENU_NEW_HOT_SEAT_2:
            case APP_MENU_LOAD_HOT_SEAT_2:
                giNumHumanPlayers = 2;
                iMPBaseType = MULTIPLAYER_BASE_HOT_SEAT;
                break;
            case APP_MENU_NEW_HOT_SEAT_3:
            case APP_MENU_LOAD_HOT_SEAT_3:
                giNumHumanPlayers = 3;
                iMPBaseType = MULTIPLAYER_BASE_HOT_SEAT;
                break;
            case APP_MENU_NEW_HOT_SEAT_4:
            case APP_MENU_LOAD_HOT_SEAT_4:
                giNumHumanPlayers = 4;
                iMPBaseType = MULTIPLAYER_BASE_HOT_SEAT;
                break;
            case APP_MENU_NEW_NETWORK_HOST:
            case APP_MENU_LOAD_NETWORK_HOST:
                iMPBaseType = MULTIPLAYER_BASE_NETWORK;
                iMPExtendedType = REMOTE_GAME_NETWORK_HOST;
                goto remoteSetup;
            case APP_MENU_NEW_NETWORK_GUEST:
            case APP_MENU_LOAD_NETWORK_GUEST:
                iMPBaseType = MULTIPLAYER_BASE_NETWORK;
                iMPExtendedType = REMOTE_GAME_NETWORK_GUEST;
                goto remoteSetup;
            case APP_MENU_NEW_MODEM_HOST:
            case APP_MENU_LOAD_MODEM_HOST:
                iMPBaseType = MULTIPLAYER_BASE_MODEM;
                iMPExtendedType = REMOTE_GAME_MODEM_HOST;
                goto remoteSetup;
            case APP_MENU_NEW_MODEM_GUEST:
            case APP_MENU_LOAD_MODEM_GUEST:
                iMPBaseType = MULTIPLAYER_BASE_MODEM;
                iMPExtendedType = REMOTE_GAME_MODEM_GUEST;
                goto remoteSetup;
            case APP_MENU_NEW_DIRECT_HOST:
            case APP_MENU_LOAD_DIRECT_HOST:
                iMPBaseType = MULTIPLAYER_BASE_MODEM;
                iMPExtendedType = REMOTE_GAME_MODEM_HOST;
                gbDirectConnect = 1;
                goto remoteSetup;
            case APP_MENU_NEW_DIRECT_GUEST:
            case APP_MENU_LOAD_DIRECT_GUEST:
                iMPBaseType = MULTIPLAYER_BASE_MODEM;
                iMPExtendedType = REMOTE_GAME_MODEM_GUEST;
                gbDirectConnect = 1;
                goto remoteSetup;

            remoteSetup:
                RemoteMain(iMPExtendedType);
                if (iMPExtendedType == REMOTE_GAME_NETWORK_GUEST
                    || iMPExtendedType == REMOTE_GAME_MODEM_GUEST)
                    gbWaitForRemoteReceive = 1;
                break;
        }
        giMenuCommand = -1;
        result = 1;
        goto done;
    }

    window = new heroWindow(400, 35, "stpnewgm.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupGameHandler, 0);
    delete window;

    switch (static_cast<short>(gpWindowManager->m_dialogResult)) {
        case 1:
            break;
        case 2:
            giCampaignChoice = 1;
            if (newGame) {
                if (!SetupCampaignGame()) {
                    result = 0;
                    goto done;
                }
            }
            break;
        case 3:
            if (!SetupMultiPlayerGame()) {
                result = 0;
                goto done;
            }
            break;
        case DIALOG_CANCEL:
            result = 0;
            goto done;
    }

    if (iMPBaseType == MULTIPLAYER_BASE_NETWORK || iMPBaseType == MULTIPLAYER_BASE_MODEM) {
        RemoteMain(iMPExtendedType);
        if (iMPExtendedType == REMOTE_GAME_NETWORK_GUEST || iMPExtendedType == REMOTE_GAME_MODEM_GUEST)
            gbWaitForRemoteReceive = 1;
    }

done:
    gbInSetupDialog = 0;
    return result;
}

// donor PoL RVA 0x000123cc; preferred Buka symbol ?PickLoadGame@game@@QAEHXZ
// donor Buka TU SOURCE/SETUP; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.594268;margin=0.566491;shape=0.333;size=0.748;calls=0.722;strings=.\GAMES\;alternate=pol20:int game::PickLoadGame(void)@0x000123cc
VA(0x00457967, 0x1e7)
signed char game::PickLoadGame(void) {
    fileRequester* request;
    short result;

    if (!SetupGame(0))
        return 0;
    if (gbWaitForRemoteReceive)
        return 1;
    extern char gcGamePath[];
    request = new fileRequester(
        0x136,
        0xe,
        0,
        giCampaignChoice > 0 ? "*.CGM" : "*.GM*",
        gcGamePath,
        giCampaignChoice > 0 ? ".CGM" : ".GM*"
    );
    if (!request)
        MemError();
    gpMouseManager->ReallyShowPointer();
    result = gpExec->DoDialog(request);
    gpMouseManager->ReallyHidePointer();
    if (result == DIALOG_BUTTON_2) {
        gpGame->LoadGame(gLastFilename, 0, 0);
        delete request;
        return 1;
    } else {
        delete request;
        return 0;
    }
}

// Buka 2.1 SETUP help handlers; HoMM1 shows each help text as a type-4 dialog.
VA(0x00457b4e, 0x112)
short SetupCampaignGameHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case CHOICE_FOUR:
                helpIndex = 3;
                break;
            case DIALOG_CANCEL:
                helpIndex = 4;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(gSetupCampaignGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    }
    return BaseSetupHandler(message);
}

VA(0x00457c60, 0x149)
short SetupComPortHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case CHOICE_FOUR:
                helpIndex = 3;
                break;
            case DIALOG_CANCEL:
                helpIndex = 4;
                break;
        }
        if (helpIndex >= HELP_FIRST) {
            if (gbDirectConnect)
                NormalDialog(gSetupDCComPortHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            else
                NormalDialog(gSetupComPortHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        }
    }
    return BaseSetupHandler(message);
}

VA(0x00457da9, 0x149)
short SetupBaudHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case CHOICE_FOUR:
                helpIndex = 3;
                break;
            case DIALOG_CANCEL:
                helpIndex = 4;
                break;
        }
        if (helpIndex >= HELP_FIRST) {
            if (gbDirectConnect)
                NormalDialog(gSetupDCBaudHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            else
                NormalDialog(gSetupBaudHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        }
    }
    return BaseSetupHandler(message);
}

VA(0x00457ef2, 0x102)
short SetupHotSeatGameHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case DIALOG_CANCEL:
                helpIndex = 3;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(gSetupHotSeatGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    }
    return BaseSetupHandler(message);
}

VA(0x00457ff4, 0x139)
short SetupModemGameHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case DIALOG_CANCEL:
                helpIndex = 3;
                break;
        }
        if (helpIndex >= HELP_FIRST) {
            if (gbDirectConnect)
                NormalDialog(gSetupDCGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            else
                NormalDialog(gSetupModemGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        }
    }
    return BaseSetupHandler(message);
}

VA(0x0045812d, 0x112)
short SetupMultiPlayerGameHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case CHOICE_FOUR:
                helpIndex = 3;
                break;
            case DIALOG_CANCEL:
                helpIndex = 4;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(gSetupMultiPlayerGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    }
    return BaseSetupHandler(message);
}

VA(0x0045823f, 0xe1)
short SetupNetworkGameHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case DIALOG_CANCEL:
                helpIndex = 2;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(gSetupNetworkGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    }
    return BaseSetupHandler(message);
}

VA(0x00458320, 0x102)
short SetupGameHandler(tag_message& message) {
    int helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK)) {
        helpIndex = NO_HELP;
        switch (message.id) {
            case CHOICE_ONE:
                helpIndex = 0;
                break;
            case CHOICE_TWO:
                helpIndex = 1;
                break;
            case CHOICE_THREE:
                helpIndex = 2;
                break;
            case DIALOG_CANCEL:
                helpIndex = 3;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(gSetupGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    }
    return BaseSetupHandler(message);
}

VA(0x00458422, 0xf1)
short BaseSetupHandler(tag_message& message) {
    int handled = 0;

    PollSound();
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                if ((message.id > 0 && message.id <= 1000)
                    || message.id == DIALOG_CANCEL)
                    handled = 1;
        }
    }

    if (handled || giMenuCommand != -1) {
        gpWindowManager->m_dialogResult = message.id;
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        if (giMenuCommand != -1)
            gpWindowManager->m_dialogResult = DIALOG_CANCEL;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Retail's SETUP object ends at 0x00458513; RemoteCleanup starts the REMOTE
// object at 0x00458520.
DATA(0x0049f6b0)
int gbDoModemConfig = 0;
