#ifndef HOMM1_SOURCE_SETUP_H
#define HOMM1_SOURCE_SETUP_H

#include <BASE/dialog.h>
#include <BASE/message.h>
#include <Domains.h>

struct tag_message;

// SETUP's dialog handlers.
H1_ENUM_RETURN(MessageDispatchResult, i16) BaseSetupHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupCampaignGameHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupBaudHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupComPortHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupHotSeatGameHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupModemGameHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupMultiPlayerGameHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupNetworkGameHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) SetupGameHandler(struct tag_message& message);

extern b32 gDoModemConfig;

// The setup dialogs' results: the numbered choice buttons (BaseSetupHandler
// accepts ids 1..1000; each game::Setup* maps CHOICE_n to its option and the
// handlers show help row n - 1) or the cancel slot.
H1_ENUM_ID_BEGIN(SetupDialogChoice)
DIALOG_CANCEL = DIALOG_BUTTON_1,
    CHOICE_ONE = 1, CHOICE_TWO = 2, CHOICE_THREE = 3, CHOICE_FOUR = 4,
    CHOICE_ID_LAST = 1000 H1_ENUM_ID_END(SetupDialogChoice)

    // Each setup handler's right-click help row: NONE for a control without
    // help, otherwise a row of the handler's own gSetup*Help table from FIRST.
    // gSetupCampaignGameHelp: the four campaign heroes.
    H1_ENUM_BEGIN(SetupCampaignHelp)
    SETUP_CAMPAIGN_HELP_NONE = -1,
    SETUP_CAMPAIGN_HELP_FIRST = 0,
    SETUP_CAMPAIGN_HELP_IRONFIST = 0,
    SETUP_CAMPAIGN_HELP_SLAYER = 1,
    SETUP_CAMPAIGN_HELP_LAMANDA = 2,
    SETUP_CAMPAIGN_HELP_ALAMAR = 3,
    SETUP_CAMPAIGN_HELP_CANCEL = 4,
    SETUP_CAMPAIGN_HELP_COUNT = 5
H1_ENUM_END(SetupCampaignHelp)

// gSetupBaudHelp / gSetupDCBaudHelp: the four connection speeds.
H1_ENUM_BEGIN(SetupBaudHelp)
    SETUP_BAUD_HELP_NONE = -1,
    SETUP_BAUD_HELP_FIRST = 0,
    SETUP_BAUD_HELP_2400 = 0,
    SETUP_BAUD_HELP_9600 = 1,
    SETUP_BAUD_HELP_19200 = 2,
    SETUP_BAUD_HELP_38400 = 3,
    SETUP_BAUD_HELP_CANCEL = 4,
    SETUP_BAUD_HELP_COUNT = 5
H1_ENUM_END(SetupBaudHelp)

// gSetupComPortHelp / gSetupDCComPortHelp: COM ports 1..4.
H1_ENUM_BEGIN(SetupComPortHelp)
    SETUP_COM_PORT_HELP_NONE = -1,
    SETUP_COM_PORT_HELP_FIRST = 0,
    SETUP_COM_PORT_HELP_COM1 = 0,
    SETUP_COM_PORT_HELP_COM2 = 1,
    SETUP_COM_PORT_HELP_COM3 = 2,
    SETUP_COM_PORT_HELP_COM4 = 3,
    SETUP_COM_PORT_HELP_CANCEL = 4,
    SETUP_COM_PORT_HELP_COUNT = 5
H1_ENUM_END(SetupComPortHelp)

// gSetupHotSeatGameHelp: 2..4 human players.
H1_ENUM_BEGIN(SetupHotSeatHelp)
    SETUP_HOT_SEAT_HELP_NONE = -1,
    SETUP_HOT_SEAT_HELP_FIRST = 0,
    SETUP_HOT_SEAT_HELP_TWO_PLAYERS = 0,
    SETUP_HOT_SEAT_HELP_THREE_PLAYERS = 1,
    SETUP_HOT_SEAT_HELP_FOUR_PLAYERS = 2,
    SETUP_HOT_SEAT_HELP_CANCEL = 3,
    SETUP_HOT_SEAT_HELP_COUNT = 4
H1_ENUM_END(SetupHotSeatHelp)

// gSetupModemGameHelp / gSetupDCGameHelp: host, guest, port configuration.
H1_ENUM_BEGIN(SetupModemHelp)
    SETUP_MODEM_HELP_NONE = -1,
    SETUP_MODEM_HELP_FIRST = 0,
    SETUP_MODEM_HELP_HOST = 0,
    SETUP_MODEM_HELP_GUEST = 1,
    SETUP_MODEM_HELP_CONFIGURE = 2,
    SETUP_MODEM_HELP_CANCEL = 3,
    SETUP_MODEM_HELP_COUNT = 4
H1_ENUM_END(SetupModemHelp)

// gSetupMultiPlayerGameHelp: the four link kinds.
H1_ENUM_BEGIN(SetupMultiPlayerHelp)
    SETUP_MULTIPLAYER_HELP_NONE = -1,
    SETUP_MULTIPLAYER_HELP_FIRST = 0,
    SETUP_MULTIPLAYER_HELP_HOT_SEAT = 0,
    SETUP_MULTIPLAYER_HELP_NETWORK = 1,
    SETUP_MULTIPLAYER_HELP_MODEM = 2,
    SETUP_MULTIPLAYER_HELP_DIRECT_CONNECT = 3,
    SETUP_MULTIPLAYER_HELP_CANCEL = 4,
    SETUP_MULTIPLAYER_HELP_COUNT = 5
H1_ENUM_END(SetupMultiPlayerHelp)

// gSetupNetworkGameHelp: host or guest.
H1_ENUM_BEGIN(SetupNetworkHelp)
    SETUP_NETWORK_HELP_NONE = -1,
    SETUP_NETWORK_HELP_FIRST = 0,
    SETUP_NETWORK_HELP_HOST = 0,
    SETUP_NETWORK_HELP_GUEST = 1,
    SETUP_NETWORK_HELP_CANCEL = 2,
    SETUP_NETWORK_HELP_COUNT = 3
H1_ENUM_END(SetupNetworkHelp)

// gSetupGameHelp: standard, campaign or multi-player game.
H1_ENUM_BEGIN(SetupGameHelp)
    SETUP_GAME_HELP_NONE = -1,
    SETUP_GAME_HELP_FIRST = 0,
    SETUP_GAME_HELP_STANDARD = 0,
    SETUP_GAME_HELP_CAMPAIGN = 1,
    SETUP_GAME_HELP_MULTIPLAYER = 2,
    SETUP_GAME_HELP_CANCEL = 3,
    SETUP_GAME_HELP_COUNT = 4
H1_ENUM_END(SetupGameHelp)

#endif // HOMM1_SOURCE_SETUP_H
