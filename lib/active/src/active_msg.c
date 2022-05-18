#include <stdbool.h>
#include <active.h>

static void Event_init(Event *const e, Active const *const me, EvtType t)
{

  ACTIVE_ASSERT(e != NULL, "Event is NULL");
  ACTIVE_ASSERT(t != UNUSED, "Event type is UNUSED");
  ACTIVE_ASSERT(me != NULL, "Active object sender not set for event");

  e->type = t;
  e->_sender = (Active *)me;

  // Cast away const to clear dynamic field
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = false;
}

/* Initialize a TimeEvt object as non-dynamic before using it.
   This function must *not* be called by the application for TimeEvt objects allocated by TimeEvt_New.
   This function must *not* be called on a running TimeEvt.

   TimeEvt *const te:  Pointer to the time event to initialize
   const Active *const me: The sender of the time event. The attached event will be posted from this active object's context
   const Event *e:
      The attached event to sent on timer expiration.
      Parameter is optional if expiry handler is set - expiry handler then returns which event to post.
      If replacing event from _init on first expiry callback, the initial event will not be freed.
      The attached event should not be accessed after
   const Active *const receiver: Receiver of the attached event.
   TimerExpiryHandler expFn: Expiry function called at every timer expiry. Can be used to attach an event to the timer that will be posted on this expiry. Runs in ISR context.
 */
void TimeEvt_init(TimeEvt *const te, const Active *const me, Event *const e, Active const *const receiver, TimerExpiryHandler expFn)
{

  ACTIVE_ASSERT((e == NULL && expFn != NULL) || e != NULL, "Event is null without expiry function set");

  // Initialize timer event
  Event_init(EVT_UPCAST(te), me, TIMEREVT);

  te->e = e;
  te->receiver = receiver;
  te->expFn = expFn;
  Active_Timer_init(te);
}

/* Initialize a Signal object as non-dynamic before using it. This function should not be called by the application for
   dynamic Signal objects allocated by Signal_new */
void Signal_init(Signal *s, Active const *const me, uint16_t sig)
{
  Event_init(EVT_UPCAST(s), me, SIGNAL);
  ACTIVE_ASSERT(sig >= (uint16_t)USER_SIG, "Signal type is invalid user defined signal");
  s->sig = sig;
}
