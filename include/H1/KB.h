#ifndef HOMM1_H1_KB_H
#define HOMM1_H1_KB_H

class soundManager;

extern char gbInPollSound;
extern int gbForegroundApp;
extern long gNextSoundPollTick;
extern soundManager *gpSoundManager;

long KBTickCount();
void PollRemote();
extern "C" void PollSound();
void ForcePollSound();

#endif
