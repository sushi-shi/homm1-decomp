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

    // Each setup handler's help row (the gSetup*Help table texts name them); the
    // rows follow CHOICE_ONE.. and end with the cancel row.

// Retail stpcmpgn.bin dialog driven by SetupCampaignGameHandler: HoMM1's
// game::SetupCampaignGame, not the HoMM2 trading post the graph proposed.
VA(0x00456b10, 0x10d)
i8 game::SetupCampaignGame(void) {
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

VA(0x00456c1d, 0x139)
i8 game::SetupBaud(void) {
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

VA(0x00456d56, 0x1ae)
i8 game::SetupComPort(void) {
    char initString[40];

    heroWindow* setupWindow = new heroWindow(400, 35, "stpcom.bin");
    if (!setupWindow)
        MemError();
    gpWindowManager->DoDialog(setupWindow, SetupComPortHandler, 0);
    delete setupWindow;
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
            localization::Tr("setup.modem.initialization_prompt"),
            initString,
            40,
            gText
        );
        strcpy(gConfig.modemInitString, initString);
    }
    WritePrefs();
    return 1;
}

VA(0x00456f04, 0x107)
i8 game::SetupHotSeatGame(void) {
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

VA(0x0045700b, 0xe7)
i8 game::SetupNetworkGame(void) {
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

VA(0x004570f2, 0x299)
i8 game::SetupModemGame(void) {
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
                GetDataEntry(localization::Tr("modem.telephone.required"), numbuf, 35, NULL);
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

VA(0x0045738b, 0x1a2)
i8 game::SetupMultiPlayerGame(void) {
    i32 loop;

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
VA(0x0045752d, 0x372)
i8 game::SetupGame(i8 newGame) {
    heroWindow* window;
    i32 result;

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

    switch (static_cast<i16>(gpWindowManager->m_dialogResult)) {
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

VA(0x0045789f, 0x1c1)
i8 game::PickLoadGame(void) {
    fileRequester* fileReq;
    i16 dialogResult;

    if (!SetupGame(0))
        return 0;
    if (gbWaitForRemoteReceive)
        return 1;
    fileReq = new fileRequester(
        0x136,
        0xe,
        FILE_REQUESTER_LOAD,
        gCampaignChoice > 0 ? "*.CGM" : "*.GM*",
        gGamePath,
        gCampaignChoice > 0 ? ".CGM" : ".GM*"
    );
    if (!fileReq)
        MemError();
    gpMouseManager->ReallyShowPointer();
    dialogResult = gpExec->DoDialog(fileReq);
    gpMouseManager->ReallyHidePointer();
    if (dialogResult == DIALOG_BUTTON_2) {
        gpGame->LoadGame(gLastFilename, 0, 0);
        delete fileReq;
        return 1;
    } else {
        delete fileReq;
        return 0;
    }
}

// Buka 2.1 SETUP help handlers; HoMM1 shows each help text as a type-4 dialog.
VA(0x00457a60, 0xd7)
i16 SetupCampaignGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
            NormalDialog(gSetupCampaignGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

VA(0x00457b37, 0x107)
i16 SetupComPortHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
                NormalDialog(gSetupDCComPortHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            else
                NormalDialog(gSetupComPortHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    }
    return BaseSetupHandler(message);
}

VA(0x00457c3e, 0x107)
i16 SetupBaudHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
                NormalDialog(gSetupDCBaudHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            else
                NormalDialog(gSetupBaudHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    }
    return BaseSetupHandler(message);
}

VA(0x00457d45, 0xc8)
i16 SetupHotSeatGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
            NormalDialog(gSetupHotSeatGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

VA(0x00457e0d, 0xf8)
i16 SetupModemGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
                NormalDialog(gSetupDCGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            else
                NormalDialog(gSetupModemGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    }
    return BaseSetupHandler(message);
}

VA(0x00457f05, 0xd7)
i16 SetupMultiPlayerGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
            NormalDialog(gSetupMultiPlayerGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

VA(0x00457fdc, 0xad)
i16 SetupNetworkGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
            NormalDialog(gSetupNetworkGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

VA(0x00458089, 0xc8)
i16 SetupGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
            NormalDialog(gSetupGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

VA(0x00458151, 0xb4)
i16 BaseSetupHandler(tag_message& message) {
    i32 handled = 0;

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
DATA(0x004cc8cc)
i32 gDoModemConfig = 0;
