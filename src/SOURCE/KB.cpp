#include <SOURCE/KB.h>

#include <match.h>

// Code-required identities only. These labels let the synthetic PDB name the
// relocation targets; initializer/data matching remains deferred.
extern DATA(0x00094180) char gbInPollSound;
extern DATA(0x0009fe78) int gbForegroundApp;
extern DATA(0x000c6a94) long gNextSoundPollTick;
extern DATA(0x000c7ca8) soundManager *gpSoundManager;

// Incremental fragment. HoMM2 KB.cpp provides the naming/behavior correspondence;
// HoMM1 differs in the timer comparison and placement of the re-entry guard.
// Original HoMM1 TU ownership remains a hypothesis. See evidence/poll-sound.md.
extern "C" RVA(0x0004f640, 0x72)
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

RVA(0x0004f6b2, 0x20)
void ForcePollSound()
{
    gNextSoundPollTick = KBTickCount() - 1;
    PollSound();
}
