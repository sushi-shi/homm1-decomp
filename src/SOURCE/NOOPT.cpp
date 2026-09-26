// Retail delay helpers; Buka NOOPT correspondence with HoMM1 assertion.

#include <match.h>

#include <SOURCE/NOOPT.h>

#include <BASE/Misc.h>
#include <H1/KB.h>

VA(0x00464470, 0x54)
void DelayTil(int *endTime)
{
    ProcessAssert(*endTime > 10000, gNooptAssertFile, gNooptAssertLine + 1);
    while (*endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x004644c4, 0x23)
void DelayMilli(long delay)
{
    DelayTilMilli(KBTickCount() + delay);
}

VA(0x004644e7, 0x2d)
void DelayTilMilli(long endTime)
{
    while (endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}
