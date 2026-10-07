// Delay helpers.

#include <match.h>

#include <SOURCE/NOOPT.h>

#include <BASE/Misc.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00446370, 0x2e)
void DelayTicks(i32 ticks) {
    i32 unused = 0;

    gTimers[DELAY_TICKS_TIMER_SLOT] = KBTickCount() + ticks * DELAY_TICK_MILLISECONDS;
    DelayTil(gTimers + DELAY_TICKS_TIMER_SLOT);
}

VA(0x0044639e, 0x3b)
#line 15 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\NOOPT.CPP"
void DelayTil(i32* endTime) {
#line 16
    H1_ASSERT(*endTime > 10000);
    while (*endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x004463d9, 0x16)
void DelayMilli(i32 delay) {
    DelayTilMilli(KBTickCount() + delay);
}

VA(0x004463ef, 0x1b)
void DelayTilMilli(i32 endTime) {
    while (endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}
