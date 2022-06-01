#include <active.h>

void Active_eventLoop(Active *const me)
{
  ACTIVE_ASSERT(me != NULL, "Active object is null)");

  static const SIGNAL_DEFINE(startSignal, START_SIG);

  // Initialize active object
  me->dispatch(me, EVT_UPCAST(&startSignal));

  Event *e;
  while (1)
  {
    e = NULL;
    Active_get(me->queue, &e);

    ACTIVE_ASSERT(e != NULL, "Event object is null");

    // Timer events are not processed by the AO dispatch function.
    // Instead the attached event is processed in the context of the
    // active object that started the timer event
    if (e->type == TIMEREVT)
    {
      TimeEvt *te = EVT_CAST(e, TimeEvt);
      Active_TimeEvt_dispatch(te);
    }
    // Default: Let AO process event
    else
    {
      me->dispatch(me, e);
    }

    // Decrement reference counter added by Active_post after event is processed
    Active_mem_refdec(e);
  }
}

int Active_post(Active const *const receiver, Event const *const e)
{
  ACTIVE_ASSERT(receiver != NULL || e->type > MAX_DIRECT_EVTTYPE, "Receiver active object is null for direct message");
  ACTIVE_ASSERT(e != NULL, "Event object is null");
  ACTIVE_ASSERT(e->type != UNUSED, "Event object is not initialized");

  /* Adding a memory ref must be done before putting it on the receiving queue,
  in case receiving object is higher priority than running object
  (which would decrement the ref counter while processingand potentially free it) */
  Active_mem_refinc(e);

  // NB: Pointer to pointer
  int status = Active_put(receiver->queue, &e);

  // Message was not sent, remove memory ref again
  if (status != 0)
  {
    Active_mem_refdec(e);
  }

  return status;
}

int Active_postTimeEvt(TimeEvt *te)
{
  // Post time event to AO sender's queue so AO framework can
  // update and post the attached event in the sender's context
  return Active_post(te->super._sender, EVT_UPCAST(te));
}