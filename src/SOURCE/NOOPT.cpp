// Retail delay helpers; Buka NOOPT correspondence with HoMM1 assertion.

#include <match.h>

#include <SOURCE/NOOPT.h>

#include <BASE/Misc.h>
#include <H1/KB.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/kbwin.h>

// DelayTicks waits on its own glTimers slot, in ticks of 15 milliseconds.
H1_ENUM_CONST_BEGIN(DelayTicksConstant)
    DELAY_TICKS_TIMER_SLOT = 1,
    DELAY_TICK_MILLISECONDS = 15
H1_ENUM_CONST_END(DelayTicksConstant)

// No direct caller survives in retail; the HoMM2 timer slot names glTimers.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00464430, 0x40)
void DelayTicks(int ticks) {
    int unused = 0;

    glTimers[DELAY_TICKS_TIMER_SLOT] = KBTickCount() + ticks * DELAY_TICK_MILLISECONDS;
    DelayTil(glTimers + DELAY_TICKS_TIMER_SLOT);
}

VA(0x00464470, 0x54)
#line 15 "D:\\Heroes\\Source\\NOOPT.CPP"
void DelayTil(int* endTime) {
#line 16
    ProcessAssert(*endTime > 10000, __FILE__, __LINE__);
    while (*endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x004644c4, 0x23)
void DelayMilli(long delay) {
    DelayTilMilli(KBTickCount() + delay);
}

VA(0x004644e7, 0x2d)
void DelayTilMilli(long endTime) {
    while (endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

// NOOPT's retail .data 0x004a0800-0x004a081f is DelayTil's /Gi line static (15)
// and its NOOPT.CPP literal (docs/patterns/vc4-gi-line-var.md).
