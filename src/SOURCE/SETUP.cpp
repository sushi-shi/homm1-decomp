#include <H1/Ints.h>

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

i8 game::SetupCampaignGame(void) {
    heroWindow* window = new heroWindow(400, 35, "stpcmpgn.bin");
    if (!window)
        MemError();
    gWindowManager->DoDialog(window, SetupCampaignGameHandler, false);
    delete window;
    switch (gWindowManager->m_dialogResult) {
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

i8 game::SetupBaud(void) {
    heroWindow* window = new heroWindow(400, 35, "stpbaud.bin");
    if (!window)
        MemError();
    gWindowManager->DoDialog(window, SetupBaudHandler, false);
    delete window;
    switch (gWindowManager->m_dialogResult) {
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

i8 game::SetupComPort(void) {
    char initString[40];

    heroWindow* setupWindow = new heroWindow(400, 35, "stpcom.bin");
    if (!setupWindow)
        MemError();
    gWindowManager->DoDialog(setupWindow, SetupComPortHandler, false);
    delete setupWindow;
    switch (gWindowManager->m_dialogResult) {
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
        GetDataEntry(localization::Tr("setup.modem.initialization_prompt"), initString, 40, gText);
        strcpy(gConfig.modemInitString, initString);
    }
    WritePrefs();
    return 1;
}

i8 game::SetupHotSeatGame(void) {
    heroWindow* window = new heroWindow(400, 35, "stphotst.bin");
    if (!window)
        MemError();
    gWindowManager->DoDialog(window, SetupHotSeatGameHandler, false);
    delete window;
    switch (gWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            gNumHumanPlayers = 2;
            break;
        case CHOICE_TWO:
            gNumHumanPlayers = 3;
            break;
        case CHOICE_THREE:
            gNumHumanPlayers = 4;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

i8 game::SetupNetworkGame(void) {
    heroWindow* window = new heroWindow(400, 35, "stpnet.bin");
    if (!window)
        MemError();
    gWindowManager->DoDialog(window, SetupNetworkGameHandler, false);
    delete window;
    switch (gWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            gMapExtendedType = REMOTE_GAME_NETWORK_HOST;
            break;
        case CHOICE_TWO:
            gMapExtendedType = REMOTE_GAME_NETWORK_GUEST;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

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
    gWindowManager->DoDialog(window, SetupModemGameHandler, false);
    delete window;
    switch (gWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            gMapExtendedType = REMOTE_GAME_MODEM_HOST;
            if (gConfig.comPort[gDirectConnect] == 0) {
                if (!SetupComPort())
                    return 0;
            }
            if (!gDirectConnect)
                GetDataEntry(localization::Tr("modem.telephone.required"), gPhoneNumber, 35, NULL);
            break;
        case CHOICE_TWO:
            gMapExtendedType = REMOTE_GAME_MODEM_GUEST;
            if (gConfig.comPort[gDirectConnect] == 0
                && !SetupComPort())
                return 0;
            break;
        case CHOICE_THREE:
            gDoModemConfig = true;
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

i8 game::SetupMultiPlayerGame(void) {
    b32 loop;

    heroWindow* window = new heroWindow(400, 35, "stpmp.bin");
    if (!window)
        MemError();
    gWindowManager->DoDialog(window, SetupMultiPlayerGameHandler, false);
    delete window;

    gDirectConnect = 0;
    switch (gWindowManager->m_dialogResult) {
        case CHOICE_ONE:
            gMapBaseType = MULTIPLAYER_BASE_HOT_SEAT;
            if (!SetupHotSeatGame())
                return 0;
            break;
        case CHOICE_TWO:
            gMapBaseType = MULTIPLAYER_BASE_NETWORK;
            if (!SetupNetworkGame())
                return 0;
            break;
        case CHOICE_FOUR:
            gDirectConnect = 1;
            goto setupModem;
        case CHOICE_THREE:
            gDirectConnect = 0;
        setupModem:
            gMapBaseType = MULTIPLAYER_BASE_MODEM;
            loop = true;
            while (loop) {
                if (!SetupModemGame())
                    return 0;
                if (gDoModemConfig) {
                    gDoModemConfig = false;
                    if (!SetupComPort())
                        return 0;
                } else {
                    loop = false;
                }
            }
            break;
        case DIALOG_CANCEL:
            return 0;
    }
    return 1;
}

i8 game::SetupGame(b8 newGame) {
    heroWindow* window;
    b32 result;

    result = true;
    gMapExtendedType = REMOTE_GAME_UNSET;
    gMapBaseType = MULTIPLAYER_BASE_UNSET;
    gNumHumanPlayers = 1;
    gWaitForRemoteReceive = false;
    gDirectConnect = 0;
    gInSetupDialog = true;

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
                gNumHumanPlayers = 2;
                gMapBaseType = MULTIPLAYER_BASE_HOT_SEAT;
                break;
            case APP_MENU_NEW_HOT_SEAT_3:
            case APP_MENU_LOAD_HOT_SEAT_3:
                gNumHumanPlayers = 3;
                gMapBaseType = MULTIPLAYER_BASE_HOT_SEAT;
                break;
            case APP_MENU_NEW_HOT_SEAT_4:
            case APP_MENU_LOAD_HOT_SEAT_4:
                gNumHumanPlayers = 4;
                gMapBaseType = MULTIPLAYER_BASE_HOT_SEAT;
                break;
            case APP_MENU_NEW_NETWORK_HOST:
            case APP_MENU_LOAD_NETWORK_HOST:
                gMapBaseType = MULTIPLAYER_BASE_NETWORK;
                gMapExtendedType = REMOTE_GAME_NETWORK_HOST;
                goto remoteSetup;
            case APP_MENU_NEW_NETWORK_GUEST:
            case APP_MENU_LOAD_NETWORK_GUEST:
                gMapBaseType = MULTIPLAYER_BASE_NETWORK;
                gMapExtendedType = REMOTE_GAME_NETWORK_GUEST;
                goto remoteSetup;
            case APP_MENU_NEW_MODEM_HOST:
            case APP_MENU_LOAD_MODEM_HOST:
                gMapBaseType = MULTIPLAYER_BASE_MODEM;
                gMapExtendedType = REMOTE_GAME_MODEM_HOST;
                goto remoteSetup;
            case APP_MENU_NEW_MODEM_GUEST:
            case APP_MENU_LOAD_MODEM_GUEST:
                gMapBaseType = MULTIPLAYER_BASE_MODEM;
                gMapExtendedType = REMOTE_GAME_MODEM_GUEST;
                goto remoteSetup;
            case APP_MENU_NEW_DIRECT_HOST:
            case APP_MENU_LOAD_DIRECT_HOST:
                gMapBaseType = MULTIPLAYER_BASE_MODEM;
                gMapExtendedType = REMOTE_GAME_MODEM_HOST;
                gDirectConnect = 1;
                goto remoteSetup;
            case APP_MENU_NEW_DIRECT_GUEST:
            case APP_MENU_LOAD_DIRECT_GUEST:
                gMapBaseType = MULTIPLAYER_BASE_MODEM;
                gMapExtendedType = REMOTE_GAME_MODEM_GUEST;
                gDirectConnect = 1;
                goto remoteSetup;

            remoteSetup:
                RemoteMain(gMapExtendedType);
                if (gMapExtendedType == REMOTE_GAME_NETWORK_GUEST
                    || gMapExtendedType == REMOTE_GAME_MODEM_GUEST)
                    gWaitForRemoteReceive = true;
                break;
        }
        gMenuCommand = APP_MENU_NONE;
        result = true;
        goto done;
    }

    window = new heroWindow(400, 35, "stpnewgm.bin");
    if (!window)
        MemError();
    gWindowManager->DoDialog(window, SetupGameHandler, false);
    delete window;

    switch (static_cast<i16>(gWindowManager->m_dialogResult)) {
        case CHOICE_ONE:
            break;
        case CHOICE_TWO:
            gCampaignChoice = CAMPAIGN_IRONFIST;
            if (newGame) {
                if (!SetupCampaignGame()) {
                    result = false;
                    goto done;
                }
            }
            break;
        case CHOICE_THREE:
            if (!SetupMultiPlayerGame()) {
                result = false;
                goto done;
            }
            break;
        case DIALOG_CANCEL:
            result = false;
            goto done;
    }

    if (gMapBaseType == MULTIPLAYER_BASE_NETWORK || gMapBaseType == MULTIPLAYER_BASE_MODEM) {
        RemoteMain(gMapExtendedType);
        if (gMapExtendedType == REMOTE_GAME_NETWORK_GUEST
            || gMapExtendedType == REMOTE_GAME_MODEM_GUEST)
            gWaitForRemoteReceive = true;
    }

done:
    gInSetupDialog = false;
    return result;
}

i8 game::PickLoadGame(void) {
    fileRequester* fileReq;
    i16 dialogResult;

    if (!SetupGame(false))
        return 0;
    if (gWaitForRemoteReceive)
        return 1;
    fileReq = new fileRequester(
        0x136,
        0xe,
        FILE_REQUESTER_LOAD,
        gCampaignChoice > CAMPAIGN_NONE ? "*.CGM" : "*.GM*",
        gGamePath,
        gCampaignChoice > CAMPAIGN_NONE ? ".CGM" : ".GM*"
    );
    if (!fileReq)
        MemError();
    gMouseManager->ReallyShowPointer();
    dialogResult = gExec->DoDialog(fileReq);
    gMouseManager->ReallyHidePointer();
    if (dialogResult == DIALOG_BUTTON_2) {
        gGame->LoadGame(gLastFilename, false, false);
        delete fileReq;
        return 1;
    } else {
        delete fileReq;
        return 0;
    }
}

i16 SetupCampaignGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_CAMPAIGN_HELP_NONE;
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
        if (helpIndex >= SETUP_CAMPAIGN_HELP_FIRST)
            NormalDialog(gSetupCampaignGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

i16 SetupComPortHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_COM_PORT_HELP_NONE;
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
        if (helpIndex >= SETUP_COM_PORT_HELP_FIRST) {
            if (gDirectConnect)
                NormalDialog(gSetupDCComPortHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            else
                NormalDialog(gSetupComPortHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    }
    return BaseSetupHandler(message);
}

i16 SetupBaudHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_BAUD_HELP_NONE;
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
        if (helpIndex >= SETUP_BAUD_HELP_FIRST) {
            if (gDirectConnect)
                NormalDialog(gSetupDCBaudHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            else
                NormalDialog(gSetupBaudHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    }
    return BaseSetupHandler(message);
}

i16 SetupHotSeatGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_HOT_SEAT_HELP_NONE;
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
        if (helpIndex >= SETUP_HOT_SEAT_HELP_FIRST)
            NormalDialog(gSetupHotSeatGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

i16 SetupModemGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_MODEM_HELP_NONE;
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
        if (helpIndex >= SETUP_MODEM_HELP_FIRST) {
            if (gDirectConnect)
                NormalDialog(gSetupDCGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            else
                NormalDialog(gSetupModemGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    }
    return BaseSetupHandler(message);
}

i16 SetupMultiPlayerGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_MULTIPLAYER_HELP_NONE;
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
        if (helpIndex >= SETUP_MULTIPLAYER_HELP_FIRST)
            NormalDialog(gSetupMultiPlayerGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

i16 SetupNetworkGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_NETWORK_HELP_NONE;
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
        if (helpIndex >= SETUP_NETWORK_HELP_FIRST)
            NormalDialog(gSetupNetworkGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

i16 SetupGameHandler(tag_message& message) {
    i32 helpIndex;

    if ((message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        && IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
        helpIndex = SETUP_GAME_HELP_NONE;
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
        if (helpIndex >= SETUP_GAME_HELP_FIRST)
            NormalDialog(gSetupGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
    }
    return BaseSetupHandler(message);
}

i16 BaseSetupHandler(tag_message& message) {
    b32 handled = false;

    PollSound();
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                if ((message.id > 0 && message.id <= CHOICE_ID_LAST) || message.id == DIALOG_CANCEL)
                    handled = true;
        }
    }

    if (handled || gMenuCommand != APP_MENU_NONE) {
        FINISH_DIALOG_MESSAGE(message);
        if (gMenuCommand != APP_MENU_NONE)
            gWindowManager->m_dialogResult = DIALOG_CANCEL;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

b32 gDoModemConfig = false;
