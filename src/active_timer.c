#include <active.h>

typedef enum TimerType
{
  ONESHOT,
  PERIODIC
} TimerType;

static TimerType getTimerType(const ACT_Timer *const timer)
{
  return timer->periodMs == 0 ? ONESHOT : PERIODIC;
}

// Runs in ISR context - called from underlying port/framework
void ACT_Timer_expiryCB(TimeEvt *const te)
{

  // Post time event
  ACT_postTimeEvt(te);

  if (getTimerType(&(te->timer)) == ONESHOT)
  {
    te->timer.running = false;
    te->timer.sync = false; // Timer stop synchronization

    // One shot time events - decrement reference immediately
    // after posting - posting adds new ref that keeps it alive
    ACT_mem_refdec(EVT_UPCAST(te));
  }
}

void ACT_TimeEvt_dispatch(TimeEvt *const te)
{
  if (te->expFn)
  {
    // Let Active objects expiry function update attached event
    ACT_Evt *updated_evt = te->expFn(te);

    if (updated_evt)
    {
      ACT_Evt *last_evt = (ACT_Evt *)te->e;
      te->e = updated_evt;

      // Add ref on new event (might be same as in initial timer start) to persist across posts
      ACT_mem_refinc(te->e);

      // Clear ref on last attached event (set by start or previous exp fn) and GC it.
      if (last_evt)
      {
        ACT_mem_refdec(last_evt);
      }
    }
    // Ensure not both initial attached event and return from expiry function was NULL
    ACTP_ASSERT(te->e != NULL, "Attached event is NULL");
  }

  ACT_post(te->receiver, te->e);

  /* One shot events: Remove ref for freeing once posted */
  if (getTimerType(&(te->timer)) == ONESHOT)
  {
    ACT_mem_refdec(te->e);
  }
}

/* @private. Initialize the timer part of a Time ACT_Evt. Not to be called by the application */
void ACT_Timer_init(TimeEvt *te)
{
  ACT_Timer *tp = &(te->timer);
  ACTP_TIMER_INIT(tp, ACTP_TimerExpiryFn);
  ACTP_TIMER_PARAM_SET(tp, te);

  te->timer.running = false;
  te->timer.sync = false;
}

void ACT_TimeEvt_start(TimeEvt *te, size_t durationMs, size_t periodMs)
{
  ACTP_ASSERT(te != NULL, "Timer event is NULL");
  ACTP_ASSERT(te->super.type == TIMEREVT, "Timer event not initialized properly");

  if (te->timer.running)
  {
    return;
  }
  // Add extra reference on timer event and any attached events
  // to have a trigger to free them when stopping

  ACT_mem_refinc(EVT_UPCAST(te));

  // Add reference to any attached event
  if (te->e)
  {
    ACT_mem_refinc(te->e);
  }

  te->timer.running = true;
  te->timer.sync = false;
  te->timer.durationMs = durationMs;
  te->timer.periodMs = periodMs;

  ACT_Timer *tp = &(te->timer);
  ACTP_TIMER_START(tp, durationMs, periodMs);
}

bool ACT_TimeEvt_stop(TimeEvt *te)
{
  bool ret = false;

  if (!te->timer.running)
  {
    return ret;
  }
  // Stop timer
  te->timer.sync = true;

  ACT_Timer *tp = &(te->timer);
  ACTP_TIMER_STOP(tp);

  // Timer was stopped and did not expire first -> cleanup dynamic event

  te->timer.running = false;
  ret = true;

  // Timer did not expire (ISR) between after calling ACT_TimeEvt_stop
  // and timer actually stopping

  // Decrement reference count on time event and attached event
  if (te->timer.sync == true)
  {
    ACT_mem_refdec(EVT_UPCAST(te));

    if (te->e)
    {
      ACT_mem_refdec(te->e);
    }
  }

  return ret;
}
