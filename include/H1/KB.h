#ifndef HOMM1_H1_KB_H
#define HOMM1_H1_KB_H

class soundManager;
class heroWindowManager;
class resourceManager;

extern char gbInPollSound;
extern int bShowIt;
extern char gText[];
extern int gbForegroundApp;
extern long gNextSoundPollTick;
extern char *DEFAULT_AGGREGATE_NAME;
extern resourceManager *gpResourceManager;
extern soundManager *gpSoundManager;
extern heroWindowManager *gpWindowManager;

long KBTickCount();
void PollRemote();
extern "C" void PollSound();
void ForcePollSound();
void ShutDown(char *);

#endif
