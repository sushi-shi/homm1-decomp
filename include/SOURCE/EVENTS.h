#ifndef HOMM1_SOURCE_EVENTS_H
#define HOMM1_SOURCE_EVENTS_H

// EVENTS data (Buka EVENTS.h owner): the assertion records (file literals and
// line base, as in MOUSEMGR), the parked music volume DoEvent and DoCombat
// restore (-1 when none) and the event-music flag.
extern short gEventsAssertLine;
extern char gEventsAssertFile1[];
extern char gEventsAssertFile2[];
extern int giEventMusicVolume;
extern signed char gbEventMusicPlaying;

#endif // HOMM1_SOURCE_EVENTS_H
