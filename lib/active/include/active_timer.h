#ifndef ACTIVE_TIMER_H
#define ACTIVE_TIMER_H

#include <stddef.h>
#include <active_port.h>
#include <active_msg.h>

struct timerData
{
  ACTIVE_TIMER(impl);
  volatile bool running;
  volatile bool sync;
};

/* @private - used by Active framework - interface to initialize timer part of time events */
void Timer_init(TimeEvt *te);

/* ISR Function that takes Time Event pointer as argument and returns an Event pointer that should be attached on this expiry */
typedef Event *(*TimerExpiryHandler)(TimeEvt const *const te);

void Active_TimeEvt_start(TimeEvt *te, size_t durationMs, size_t periodMs);
bool Active_TimeEvt_stop(TimeEvt *te);

#endif /* ACTIVE_TIMER_H */
