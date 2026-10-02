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
// HoMM1 REMOTE.cpp defines the transport bring-up (Buka REMOTE and Netbios).
void RemoteMain(int);
int nbnet_init(void);

extern int gbDoModemConfig;
// KB-band setup state (Buka X_GLOBAL.h): the direct-connect flag and the
// multiplayer game type. They stay out of X_GLOBAL.h while GAME and KB still
// use these names for other retail objects (see the aliases there).
extern signed char gbDirectConnect;
extern signed char iMPExtendedType;

#endif // HOMM1_SOURCE_SETUP_H
