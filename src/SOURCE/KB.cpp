#include "match.h"
#include "SOURCE/KB.h"

// Incremental fragment. HoMM2 KB.cpp provides the naming/behavior correspondence;
// HoMM1 differs in the timer comparison and placement of the re-entry guard.
// Original HoMM1 TU ownership remains a hypothesis. See evidence/poll-sound.md.
extern "C" RVA(0x0004F640, 0x72)
void PollSound()
{
    if (KBTickCount() < gNextSoundPollTick)
        return;
    if (gbInPollSound)
        return;
    gbInPollSound = 1;
    gNextSoundPollTick = KBTickCount() + SOUND_POLL_INTERVAL;
    if (gbForegroundApp)
        gpSoundManager->PollSound();
    PollRemote();
    gbInPollSound = 0;
}

RVA(0x0004F6B2, 0x20)
void ForcePollSound()
{
    gNextSoundPollTick = KBTickCount() - 1;
    PollSound();
}
