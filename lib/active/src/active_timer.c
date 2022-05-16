#include <active.h>
#include "active_timer.h"

typedef enum TimerType
{
  ONESHOT,
  PERIODIC
} TimerType;

static TimerType getTimerType(Active_Timer *timer)
{
  return timer->periodMs == 0 ? ONESHOT : PERIODIC;
}

// Runs in ISR context
static void Active_Timer_expiryCB(struct k_timer *timer)
{
  TimeEvt *te = (TimeEvt *)timer->user_data;

  te->timer.sync = false; // Timer stop synchronization

  // Post time event
  Active_postTimeEvt(te);

  if (getTimerType(&(te->timer)) == ONESHOT)
  {
    te->timer.running = false;

    // One shot time events - decrement reference immediately
    // after posting - posting adds new ref that keeps it alive
    Active_mem_refdec(EVT_UPCAST(te));
  }
}

/* @private. Initialize the timer part of the Time Event. Not to be called by the application */
void Timer_init(TimeEvt *te)
{
  k_timer_init(&(te->timer.impl), Active_Timer_expiryCB, NULL);
  k_timer_user_data_set(&(te->timer.impl), te);
  te->timer.running = false;
  te->timer.sync = false;
}


void Active_TimeEvt_start(TimeEvt *te, size_t durationMs, size_t periodMs)
{
  ACTIVE_ASSERT(te != NULL, "Timer event is NULL");
  ACTIVE_ASSERT(te->super.type == TIMEREVT, "Timer event not initialized properly");

  if (te->timer.running)
  {
    return;
  }
  // Add extra reference on timer event and any attached events
  // to have a trigger to free them when stopping

  Active_mem_refinc(EVT_UPCAST(te));

  // Add reference to any attached event
  if (te->e)
  {
    Active_mem_refinc(te->e);
  }

  te->timer.running = true;
  te->timer.sync = false;
  te->timer.durationMs = durationMs;
  te->timer.periodMs = periodMs;

  k_timer_start(&(te->timer.impl), K_MSEC(durationMs), K_MSEC(periodMs));
}

/* Stop a timer.
Do not use dynamic timer event again after stopping it
@return true: The timer was running when it was stopped. A dynamic attached event must be freed by the application
        false:The timer was already expired (one-shot) or already stopped.
*/
bool Active_TimeEvt_stop(TimeEvt *te)
{
  bool ret = false;

  if (!te->timer.running)
  {
    return ret;
  }
  // Stop timer

  te->timer.sync = true;

  k_timer_stop(&(te->timer.impl));

  // Timer was stopped and did not expire first -> cleanup dynamic event

  te->timer.running = false;
  ret = true;

  // Timer did not expire (ISR) between after calling Active_TimeEvt_stop
  // and timer actually stopping

  // Decrement reference count on time event and attached event
  if (te->timer.sync == true)
  {
    Active_mem_refdec(EVT_UPCAST(te));

    if (te->e)
    {
      Active_mem_refdec(te->e);
    }
  }

  return ret;
}
