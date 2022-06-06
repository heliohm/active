#include <active.h>

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

void Active_TimeEvt_dispatch(TimeEvt *te)
{
  if (te->expFn)
  {
    // Let Active objects expiry function update attached event
    Event *updated_evt = te->expFn(te);

    if (updated_evt)
    {
      Event *last_evt = (Event *)te->e;
      te->e = updated_evt;

      // Add ref on new event (might be same as in initial timer start) to persist across posts
      Active_mem_refinc(te->e);

      // Clear ref on last attached event (set by start or previous exp fn) and GC it.
      if (last_evt)
      {
        Active_mem_refdec(last_evt);
      }
    }
    // Ensure not both initial attached event and return from expiry function was NULL
    ACTIVE_ASSERT(te->e != NULL, "Attached event is NULL");
  }

  Active_post(te->receiver, te->e);

  /* One shot events: Remove ref for freeing once posted */
  if (getTimerType(&(te->timer)) == ONESHOT)
  {
    Active_mem_refdec(te->e);
  }
}

/* @private. Initialize the timer part of a Time Event. Not to be called by the application */
void Active_Timer_init(TimeEvt *te)
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
