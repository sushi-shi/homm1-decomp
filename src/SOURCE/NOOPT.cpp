// Retail delay helpers; Buka NOOPT correspondence with HoMM1 assertion.

#include <match.h>

#include <SOURCE/NOOPT.h>

#include <BASE/Misc.h>
#include <H1/KB.h>
#include <SOURCE/highScoreRuntime.h>

// clang-format off
// DelayTicks waits on its own glTimers slot, in ticks of 15 milliseconds.
H1_ENUM_CONST_BEGIN(DelayTicksConstant)
    DELAY_TICKS_TIMER_SLOT = 1,
    DELAY_TICK_MILLISECONDS = 15
H1_ENUM_CONST_END(DelayTicksConstant)
    // clang-format on

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
void DelayTil(int* endTime) {
    ProcessAssert(*endTime > 10000, "D:\\Heroes\\Source\\NOOPT.CPP", gNooptAssertLine + 1);
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

// NOOPT owns retail .data 0x004a0800-0x004a081f: DelayTil's source-line base
// (15) and its NOOPT.CPP literal.
DATA(0x004a0800)
short gNooptAssertLine = 15;
