#include <active.h>
#include "active_timer.h"

typedef enum TimerType
{
  ONESHOT,
  PERIODIC
} TimerType;

static TimerType getTimerType(Active_Timer *timer)
{
  return K_TIMEOUT_EQ(timer->impl.period, K_MSEC(0)) ? ONESHOT : PERIODIC;
}

// Runs in ISR context
static void Active_Timer_expiryCB(struct k_timer *timer)
{
  TimeEvt *te = (TimeEvt *)timer->user_data;

  te->timer.sync = false; // Timer stop synchronization

  // Post time event
  Active_postTimeEvt(te);

  // One shot time events - decrement reference immediately
  // after posting - posting adds new ref that keeps it alive
  if (getTimerType(&(te->timer)) == ONESHOT)
  {
    te->timer.running = false;
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

/*
Memory management:
Periodic timer
  Start: Add ref
  Expiry: Do nothing
  Stop: Remove timer ref after stopping -> garbage collect

Single ended timer
  Start: Do nothing
  Expiry: Post (will remove ref)
  Stop - already expired: Do nothing (event + time event already freed)
  Stop - GC
*/

/* Start a timer event.
Duration: Time to first expiry. Period: Time to subsequent expirations (0 for one shot timer)
Do not start dynamic TimeEvts again after expiry (one shot) as they are freed
Do not start dynamic TimerEvts after stopping them as they are freed.
*/
void Active_TimeEvt_start(TimeEvt *te, size_t durationMs, size_t periodMs)
{
  ACTIVE_ASSERT(te->super.type == TIMEREVT, "Timer event not initialized properly");

  // Add extra reference on timer event and any attached events
  // to have a trigger to free them when stopping
  if (!te->timer.running)
  {
    Active_mem_refinc(EVT_UPCAST(te));
  }

  te->timer.running = true;
  te->timer.sync = false;

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

  // Stop timer

  te->timer.sync = true;
  k_timer_stop(&(te->timer.impl));

  // Timer was stopped and did not expire first -> cleanup dynamic events
  if (te->timer.running)
  {

    te->timer.running = false;
    ret = true;

    // Timer did not expire (ISR) between after calling Active_TimeEvt_stop
    // and timer actually stopping

    // Decrement reference count on time event and attached event
    if (te->timer.sync == true)
    {
      Active_mem_refdec(EVT_UPCAST(te));

      if (te->e && getTimerType(&te->timer) == ONESHOT)
      {
        // TODO: Find out if this event has been posted or not already.

        // One shot: True if running and sync is true
        // Periodic: ?
        Active_mem_gc(te->e);
      }
    }
  }
  return ret;
}
