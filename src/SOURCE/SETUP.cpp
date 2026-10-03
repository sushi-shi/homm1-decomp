// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/SETUP.h>

#include <BASE/executive.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <SOURCE/advManager.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/comwin.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>

#include <stdio.h>
#include <string.h>

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
    CHOICE_FOUR = 4,
    CHOICE_ID_LAST = 1000
H1_ENUM_END(SetupDialogChoice)

H1_ENUM_BEGIN(SetupHelpIndex)
    NO_HELP = -1,
    HELP_FIRST = 0
H1_ENUM_END(SetupHelpIndex)

    // Each setup handler's help row (the gSetup*Help table texts name them); the
    // rows follow CHOICE_ONE.. and end with the cancel row.

// gSetupCampaignGameHelp: the four campaign heroes.
H1_ENUM_BEGIN(SetupCampaignHelp)
    SETUP_CAMPAIGN_HELP_IRONFIST = 0,
    SETUP_CAMPAIGN_HELP_SLAYER = 1,
    SETUP_CAMPAIGN_HELP_LAMANDA = 2,
    SETUP_CAMPAIGN_HELP_ALAMAR = 3,
    SETUP_CAMPAIGN_HELP_CANCEL = 4
H1_ENUM_END(SetupCampaignHelp)

// gSetupBaudHelp / gSetupDCBaudHelp: the four connection speeds.
H1_ENUM_BEGIN(SetupBaudHelp)
    SETUP_BAUD_HELP_2400 = 0,
    SETUP_BAUD_HELP_9600 = 1,
    SETUP_BAUD_HELP_19200 = 2,
    SETUP_BAUD_HELP_38400 = 3,
    SETUP_BAUD_HELP_CANCEL = 4
H1_ENUM_END(SetupBaudHelp)

// gSetupComPortHelp / gSetupDCComPortHelp: COM ports 1..4.
H1_ENUM_BEGIN(SetupComPortHelp)
    SETUP_COM_PORT_HELP_COM1 = 0,
    SETUP_COM_PORT_HELP_COM2 = 1,
    SETUP_COM_PORT_HELP_COM3 = 2,
    SETUP_COM_PORT_HELP_COM4 = 3,
    SETUP_COM_PORT_HELP_CANCEL = 4
H1_ENUM_END(SetupComPortHelp)

// gSetupHotSeatGameHelp: 2..4 human players.
H1_ENUM_BEGIN(SetupHotSeatHelp)
    SETUP_HOT_SEAT_HELP_TWO_PLAYERS = 0,
    SETUP_HOT_SEAT_HELP_THREE_PLAYERS = 1,
    SETUP_HOT_SEAT_HELP_FOUR_PLAYERS = 2,
    SETUP_HOT_SEAT_HELP_CANCEL = 3
H1_ENUM_END(SetupHotSeatHelp)

// gSetupModemGameHelp / gSetupDCGameHelp: host, guest, port configuration.
H1_ENUM_BEGIN(SetupModemHelp)
    SETUP_MODEM_HELP_HOST = 0,
    SETUP_MODEM_HELP_GUEST = 1,
    SETUP_MODEM_HELP_CONFIGURE = 2,
    SETUP_MODEM_HELP_CANCEL = 3
H1_ENUM_END(SetupModemHelp)

// gSetupMultiPlayerGameHelp: the four link kinds.
H1_ENUM_BEGIN(SetupMultiPlayerHelp)
    SETUP_MULTIPLAYER_HELP_HOT_SEAT = 0,
    SETUP_MULTIPLAYER_HELP_NETWORK = 1,
    SETUP_MULTIPLAYER_HELP_MODEM = 2,
    SETUP_MULTIPLAYER_HELP_DIRECT_CONNECT = 3,
    SETUP_MULTIPLAYER_HELP_CANCEL = 4
H1_ENUM_END(SetupMultiPlayerHelp)

// gSetupNetworkGameHelp: host or guest.
H1_ENUM_BEGIN(SetupNetworkHelp)
    SETUP_NETWORK_HELP_HOST = 0,
    SETUP_NETWORK_HELP_GUEST = 1,
    SETUP_NETWORK_HELP_CANCEL = 2
H1_ENUM_END(SetupNetworkHelp)

// gSetupGameHelp: standard, campaign or multi-player game.
H1_ENUM_BEGIN(SetupGameHelp)
    SETUP_GAME_HELP_STANDARD = 0,
    SETUP_GAME_HELP_CAMPAIGN = 1,
    SETUP_GAME_HELP_MULTIPLAYER = 2,
    SETUP_GAME_HELP_CANCEL = 3
H1_ENUM_END(SetupGameHelp)

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
            gCampaignChoice = CAMPAIGN_IRONFIST;
            break;
        case CHOICE_TWO:
            gCampaignChoice = CAMPAIGN_SLAYER;
            break;
        case CHOICE_THREE:
            gCampaignChoice = CAMPAIGN_LAMANDA;
            break;
        case CHOICE_FOUR:
            gCampaignChoice = CAMPAIGN_ALAMAR;
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
            gConfig.baudRate[gDirectConnect] = CBR_2400;
            break;
        case CHOICE_TWO:
            gConfig.baudRate[gDirectConnect] = CBR_9600;
            break;
        case CHOICE_THREE:
            gConfig.baudRate[gDirectConnect] = CBR_19200;
            break;
        case CHOICE_FOUR:
            gConfig.baudRate[gDirectConnect] = CBR_38400;
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
            gConfig.comPort[gDirectConnect] = 1;
            break;
        case CHOICE_TWO:
            gConfig.comPort[gDirectConnect] = 2;
            break;
        case CHOICE_THREE:
            gConfig.comPort[gDirectConnect] = 3;
            break;
        case CHOICE_FOUR:
            gConfig.comPort[gDirectConnect] = 4;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    if (!SetupBaud())
        return 0;
    if (!gDirectConnect) {
        strcpy(gConfig.modemInitString, "ATZ");
        sprintf(gText, "%s", gConfig.modemInitString);
        GetDataEntry(
            "Please enter any special initialization string required by your modem, or "
            "hit 'ENTER' to accept the default.",
            initStr,
            40,
            gText
        );
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

    if (gDirectConnect) {
        if (gConfig.comPort[gDirectConnect] == 0)
            window = new heroWindow(400, 35, "stpdc.bin");
        else
            window = new heroWindow(400, 35, "stpdccfg.bin");
    } else {
        if (gConfig.comPort[gDirectConnect] == 0)
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
            if (gConfig.comPort[gDirectConnect] == 0) {
                if (!SetupComPort())
                    return 0;
            }
            if (!gDirectConnect)
                GetDataEntry("Please enter the telephone number.", numbuf, 35, NULL);
            break;
        case CHOICE_TWO:
            iMPExtendedType = REMOTE_GAME_MODEM_GUEST;
            if (gConfig.comPort[gDirectConnect] == 0 && !SetupComPort())
                return 0;
            break;
        case CHOICE_THREE:
            gDoModemConfig = 1;
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

    gDirectConnect = 0;
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
            gDirectConnect = 1;
            goto setupModem;
        case CHOICE_THREE:
            gDirectConnect = 0;
        setupModem:
            iMPBaseType = MULTIPLAYER_BASE_MODEM;
            loop = 1;
            while (loop) {
                if (!SetupModemGame())
                    return 0;
                if (gDoModemConfig) {
                    gDoModemConfig = 0;
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
    iMPExtendedType = REMOTE_GAME_UNSET;
    iMPBaseType = MULTIPLAYER_BASE_UNSET;
    giNumHumanPlayers = 1;
    gbWaitForRemoteReceive = 0;
    gDirectConnect = 0;
    gInSetupDialog = 1;

    if (gMenuCommand != APP_MENU_NONE) {
        switch (gMenuCommand) {
            case APP_MENU_NEW_CAMPAIGN_IRONFIST:
                gCampaignChoice = CAMPAIGN_IRONFIST;
                break;
            case APP_MENU_NEW_CAMPAIGN_SLAYER:
                gCampaignChoice = CAMPAIGN_SLAYER;
                break;
            case APP_MENU_NEW_CAMPAIGN_LAMANDA:
                gCampaignChoice = CAMPAIGN_LAMANDA;
                break;
            case APP_MENU_NEW_CAMPAIGN_ALAMAR:
                gCampaignChoice = CAMPAIGN_ALAMAR;
                break;
            case APP_MENU_NEW_STANDARD_GAME:
            case APP_MENU_LOAD_STANDARD_GAME:
                break;
            case APP_MENU_LOAD_CAMPAIGN_GAME:
                gCampaignChoice = CAMPAIGN_IRONFIST;
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
                gDirectConnect = 1;
                goto remoteSetup;
            case APP_MENU_NEW_DIRECT_GUEST:
            case APP_MENU_LOAD_DIRECT_GUEST:
                iMPBaseType = MULTIPLAYER_BASE_MODEM;
                iMPExtendedType = REMOTE_GAME_MODEM_GUEST;
                gDirectConnect = 1;
                goto remoteSetup;

            remoteSetup:
                RemoteMain(iMPExtendedType);
                if (iMPExtendedType == REMOTE_GAME_NETWORK_GUEST
                    || iMPExtendedType == REMOTE_GAME_MODEM_GUEST)
                    gbWaitForRemoteReceive = 1;
                break;
        }
        gMenuCommand = APP_MENU_NONE;
        result = 1;
        goto done;
    }

    window = new heroWindow(400, 35, "stpnewgm.bin");
    if (!window)
        MemError();
    gpWindowManager->DoDialog(window, SetupGameHandler, 0);
    delete window;

    switch (static_cast<short>(gpWindowManager->m_dialogResult)) {
        case CHOICE_ONE:
            break;
        case CHOICE_TWO:
            gCampaignChoice = CAMPAIGN_IRONFIST;
            if (newGame) {
                if (!SetupCampaignGame()) {
                    result = 0;
                    goto done;
                }
            }
            break;
        case CHOICE_THREE:
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
        if (iMPExtendedType == REMOTE_GAME_NETWORK_GUEST
            || iMPExtendedType == REMOTE_GAME_MODEM_GUEST)
            gbWaitForRemoteReceive = 1;
    }

done:
    gInSetupDialog = 0;
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
    extern char gGamePath[];
    request = new fileRequester(
        0x136,
        0xe,
        FILE_REQUESTER_LOAD,
        gCampaignChoice > 0 ? "*.CGM" : "*.GM*",
        gGamePath,
        gCampaignChoice > 0 ? ".CGM" : ".GM*"
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
                helpIndex = SETUP_CAMPAIGN_HELP_IRONFIST;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_CAMPAIGN_HELP_SLAYER;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_CAMPAIGN_HELP_LAMANDA;
                break;
            case CHOICE_FOUR:
                helpIndex = SETUP_CAMPAIGN_HELP_ALAMAR;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_CAMPAIGN_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(
                gSetupCampaignGameHelp[helpIndex],
                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
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
                helpIndex = SETUP_COM_PORT_HELP_COM1;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_COM_PORT_HELP_COM2;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_COM_PORT_HELP_COM3;
                break;
            case CHOICE_FOUR:
                helpIndex = SETUP_COM_PORT_HELP_COM4;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_COM_PORT_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST) {
            if (gDirectConnect)
                NormalDialog(
                    gSetupDCComPortHelp[helpIndex],
                    NORMAL_DIALOG_TYPE_QUICK_VIEW,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            else
                NormalDialog(
                    gSetupComPortHelp[helpIndex],
                    NORMAL_DIALOG_TYPE_QUICK_VIEW,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
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
                helpIndex = SETUP_BAUD_HELP_2400;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_BAUD_HELP_9600;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_BAUD_HELP_19200;
                break;
            case CHOICE_FOUR:
                helpIndex = SETUP_BAUD_HELP_38400;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_BAUD_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST) {
            if (gDirectConnect)
                NormalDialog(
                    gSetupDCBaudHelp[helpIndex],
                    NORMAL_DIALOG_TYPE_QUICK_VIEW,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            else
                NormalDialog(
                    gSetupBaudHelp[helpIndex],
                    NORMAL_DIALOG_TYPE_QUICK_VIEW,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
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
                helpIndex = SETUP_HOT_SEAT_HELP_TWO_PLAYERS;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_HOT_SEAT_HELP_THREE_PLAYERS;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_HOT_SEAT_HELP_FOUR_PLAYERS;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_HOT_SEAT_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(
                gSetupHotSeatGameHelp[helpIndex],
                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
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
                helpIndex = SETUP_MODEM_HELP_HOST;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_MODEM_HELP_GUEST;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_MODEM_HELP_CONFIGURE;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_MODEM_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST) {
            if (gDirectConnect)
                NormalDialog(
                    gSetupDCGameHelp[helpIndex],
                    NORMAL_DIALOG_TYPE_QUICK_VIEW,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            else
                NormalDialog(
                    gSetupModemGameHelp[helpIndex],
                    NORMAL_DIALOG_TYPE_QUICK_VIEW,
                    -1,
                    -1,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
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
                helpIndex = SETUP_MULTIPLAYER_HELP_HOT_SEAT;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_MULTIPLAYER_HELP_NETWORK;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_MULTIPLAYER_HELP_MODEM;
                break;
            case CHOICE_FOUR:
                helpIndex = SETUP_MULTIPLAYER_HELP_DIRECT_CONNECT;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_MULTIPLAYER_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(
                gSetupMultiPlayerGameHelp[helpIndex],
                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
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
                helpIndex = SETUP_NETWORK_HELP_HOST;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_NETWORK_HELP_GUEST;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_NETWORK_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(
                gSetupNetworkGameHelp[helpIndex],
                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
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
                helpIndex = SETUP_GAME_HELP_STANDARD;
                break;
            case CHOICE_TWO:
                helpIndex = SETUP_GAME_HELP_CAMPAIGN;
                break;
            case CHOICE_THREE:
                helpIndex = SETUP_GAME_HELP_MULTIPLAYER;
                break;
            case DIALOG_CANCEL:
                helpIndex = SETUP_GAME_HELP_CANCEL;
                break;
        }
        if (helpIndex >= HELP_FIRST)
            NormalDialog(
                gSetupGameHelp[helpIndex],
                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
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
                if ((message.id > 0 && message.id <= CHOICE_ID_LAST) || message.id == DIALOG_CANCEL)
                    handled = 1;
        }
    }

    if (handled || gMenuCommand != APP_MENU_NONE) {
        FINISH_DIALOG_MESSAGE(message);
        if (gMenuCommand != APP_MENU_NONE)
            gpWindowManager->m_dialogResult = DIALOG_CANCEL;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Retail's SETUP object ends at 0x00458513; RemoteCleanup starts the REMOTE
// object at 0x00458520.
DATA(0x0049f6b0)
int gDoModemConfig = 0;
