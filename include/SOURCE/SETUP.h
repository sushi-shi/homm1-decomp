#ifndef HOMM1_SOURCE_SETUP_H
#define HOMM1_SOURCE_SETUP_H

struct tag_message;

// SETUP's dialog handlers (Buka SETUP.h).
short BaseSetupHandler(struct tag_message&);
short SetupCampaignGameHandler(struct tag_message&);
short SetupBaudHandler(struct tag_message&);
short SetupComPortHandler(struct tag_message&);
short SetupHotSeatGameHandler(struct tag_message&);
short SetupModemGameHandler(struct tag_message&);
short SetupMultiPlayerGameHandler(struct tag_message&);
short SetupNetworkGameHandler(struct tag_message&);
short SetupGameHandler(struct tag_message&);

extern int gbDoModemConfig;

#endif // HOMM1_SOURCE_SETUP_H
