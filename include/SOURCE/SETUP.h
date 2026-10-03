#ifndef HOMM1_SOURCE_SETUP_H
#define HOMM1_SOURCE_SETUP_H

struct tag_message;

// SETUP's dialog handlers (Buka SETUP.h).
i16 BaseSetupHandler(struct tag_message&);
i16 SetupCampaignGameHandler(struct tag_message&);
i16 SetupBaudHandler(struct tag_message&);
i16 SetupComPortHandler(struct tag_message&);
i16 SetupHotSeatGameHandler(struct tag_message&);
i16 SetupModemGameHandler(struct tag_message&);
i16 SetupMultiPlayerGameHandler(struct tag_message&);
i16 SetupNetworkGameHandler(struct tag_message&);
i16 SetupGameHandler(struct tag_message&);

extern i32 gDoModemConfig;

#endif // HOMM1_SOURCE_SETUP_H
