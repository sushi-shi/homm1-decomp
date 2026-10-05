#ifndef HOMM1_SOURCE_NOOPT_H
#define HOMM1_SOURCE_NOOPT_H

#include <Domains.h>

void DelayTicks(i32 ticks);
void DelayTil(i32* endTime);
void DelayMilli(i32 delay);
void DelayTilMilli(i32 endTime);

// DelayTicks waits on DELAY_TICKS_TIMER_SLOT (KB.h), in ticks of 15
// milliseconds.
H1_ENUM_CONST_BEGIN(DelayTicksConstant)
    DELAY_TICK_MILLISECONDS = 15
H1_ENUM_CONST_END(DelayTicksConstant)

#endif
