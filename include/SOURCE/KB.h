#ifndef HOMM1_KB_H
#define HOMM1_KB_H

#include "Domains.h"

enum SoundPollTiming { SOUND_POLL_INTERVAL = 30 };

// Confirmed member-call ABI; class layout is still unknown. Do not instantiate,
// pass by value, take sizeof, or add fields before recovering that layout.
class soundManager {
public:
    void PollSound();
};

extern char gbInPollSound;
extern int gbForegroundApp;
extern soundManager *gpSoundManager;
// Only the accessed deadline slot is admitted; its enclosing storage is open.
extern long gNextSoundPollTick;

long KBTickCount();
void PollRemote();
extern "C" void PollSound();
void ForcePollSound();

#endif
