#include <H1/Ints.h>

#include <SOURCE/NOOPT.h>

#include <BASE/Misc.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

void DelayTicks(i32 ticks) {
    i32 unused = 0;

    gTimers[DELAY_TICKS_TIMER_SLOT] = KBTickCount() + ticks * DELAY_TICK_MILLISECONDS;
    DelayTil(gTimers + DELAY_TICKS_TIMER_SLOT);
}

void DelayTil(i32* endTime) {
    H1_ASSERT(*endTime > 10000);
    while (*endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

void DelayMilli(i32 delay) {
    DelayTilMilli(KBTickCount() + delay);
}

void DelayTilMilli(i32 endTime) {
    while (endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}
