#ifndef HOMM1_H1_KB_H
#define HOMM1_H1_KB_H

class soundManager;

extern char gbInPollSound;
extern char gText[];
extern int gbForegroundApp;
extern long gNextSoundPollTick;
extern char *DEFAULT_AGGREGATE_NAME;
extern soundManager *gpSoundManager;

long KBTickCount();
void PollRemote();
extern "C" void PollSound();
void ForcePollSound();
void ShutDown(char *);

#endif
