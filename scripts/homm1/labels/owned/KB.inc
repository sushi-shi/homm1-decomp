// HoMM2 KB.cpp confirms the identity and behavior. HoMM1 differs in the timer
// comparison and placement of the re-entry guard.
extern "C" VA(0x0044f640, 0x72)
void PollSound()
{
    if (KBTickCount() < gNextSoundPollTick)
        return;
    if (gbInPollSound)
        return;
    gbInPollSound = 1;
    gNextSoundPollTick = KBTickCount() + 30;
    if (gbForegroundApp)
        gpSoundManager->PollSound();
    PollRemote();
    gbInPollSound = 0;
}

VA(0x0044f6b2, 0x20)
void ForcePollSound()
{
    gNextSoundPollTick = KBTickCount() - 1;
    PollSound();
}
