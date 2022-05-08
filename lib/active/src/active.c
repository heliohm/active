#include <active.h>

void Active_init(Active *const me, DispatchHandler dispatch)
{
  ACTIVE_ASSERT(me != NULL, "Active object is null)");
  ACTIVE_ASSERT(dispatch != NULL, "Dispatch handler is null");

  me->dispatch = dispatch;
}

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

    // Timer events are not processed by the dispatch function.
    // Instead the attached event is posted in the context of the
    // active object that started it
    if (e->type == TIMEREVT)
    {
      TimeEvt *te = EVT_CAST(e, TimeEvt);

      const Event *last_evt = te->e;

      if (te->expFn)
      {
        // Let Active objects expiry function update attached event
        te->e = te->expFn(te);
        ACTIVE_ASSERT(te->e != NULL, "Attached event is NULL");

        // Add ref on returned event (might be same as in initial timer start)
        Active_mem_refinc(te->e);

        // If there was an unprocessed attached event, remove reference
        if (last_evt)
        {
          Active_mem_refdec(last_evt);
        }
      }

      Active_post(te->receiver, te->e);

      // Un-attach dynamic event
      bool shouldDetach = te->e->_dynamic;

      // Clear reference set when attaching in expiry function
      Active_mem_refdec(te->e);

      if (shouldDetach)
      {
        te->e = NULL;
      }
    }
    // Default: Let AO process event
    else
    {
      me->dispatch(me, e);
    }

    // Decrement reference counter after event is processed
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
  (which would decrement the ref counter and potentially free it) */
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