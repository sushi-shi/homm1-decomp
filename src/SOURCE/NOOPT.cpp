// Retail delay helpers; Buka NOOPT correspondence with HoMM1 assertion.

#include <match.h>

#include <SOURCE/NOOPT.h>

#include <BASE/Misc.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

// DelayTicks waits on its own glTimers slot, in ticks of 15 milliseconds.
H1_ENUM_CONST_BEGIN(DelayTicksConstant)
    DELAY_TICKS_TIMER_SLOT = 1,
    DELAY_TICK_MILLISECONDS = 15
H1_ENUM_CONST_END(DelayTicksConstant)

// No direct caller survives in retail; the HoMM2 timer slot names glTimers.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00446370, 0x2e)
void DelayTicks(i32 ticks) {
    i32 unused = 0;

    glTimers[DELAY_TICKS_TIMER_SLOT] = KBTickCount() + ticks * DELAY_TICK_MILLISECONDS;
    DelayTil(glTimers + DELAY_TICKS_TIMER_SLOT);
}

VA(0x0044639e, 0x3b)
#line 15 "F:\\h1w95src\\source\\NOOPT.CPP"
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

// NOOPT's retail .data 0x004a0d04-0x004a081f is DelayTil's /Gi line static (15)
// and its NOOPT.CPP literal (docs/patterns/vc4-gi-line-var.md).
