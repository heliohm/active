#include <active.h>

void ACT_threadFn(Active *const me)
{
  ACT_ASSERT(me != NULL, "Active object is null)");

  static const ACT_SIGNAL_DEFINE(startSignal, ACT_START_SIG);

  // Initialize active object
  me->dispatch(me, EVT_UPCAST(&startSignal));

  while (1)
  {
    ACT_Evt *e = NULL;
    /* Blocking wait for events */
    int status = ACT_Q_GET(me->queue, &e);

    ACT_ASSERT(status == ACT_Q_GET_SUCCESS_STATUS, "ACT_Evt was not retrieved. Error: %i", status);
    ACT_ARG_UNUSED(status);

    ACT_ASSERT(e != NULL, "ACT_Evt pointer is null");

    // Timer events are not processed by the AO dispatch function.
    // Instead the attached event is processed in the context of the
    // active object that started the timer event
    if (e->type == ACT_TIMEVT)
    {
      ACT_TimEvt *te = EVT_CAST(e, ACT_TimEvt);
      ACT_TimeEvt_dispatch(te);
    }
    // Default: Let AO process event
    else
    {
      me->dispatch(me, e);
    }

    // Decrement reference counter added by ACT_postEvt after event is processed
    ACT_mem_refdec(e);
  }
}

int ACT_postEvt(Active const *const receiver, ACT_Evt const *const e)
{
  ACT_ASSERT(receiver != NULL, "Receiver is null");
  ACT_ASSERT(e != NULL, "ACT_Evt object is null");
  ACT_ASSERT(e->type != ACT_UNUSED, "ACT_Evt object is not initialized");

  /* Adding a memory ref must be done before putting it on the receiving queue,
  in case receiving object is higher priority than running object
  (which would decrement the ref counter while processingand potentially free it) */
  ACT_mem_refinc(e);

  int status = ACT_Q_PUT(receiver->queue, &e);
  ACT_ASSERT(status == ACT_Q_PUT_SUCCESS_STATUS, "Event not put on queue %p. Error: %i\n\n", receiver->queue, status);

  // Event was not sent, remove memory ref again
  if (status != ACT_Q_PUT_SUCCESS_STATUS)
  {
    ACT_mem_refdec(e);
  }

  return status;
}

inline int ACT_postTimEvt(ACT_TimEvt *te)
{
  // Post time event to AO sender's queue so AO framework can
  // update and post the attached event in the sender's context
  return ACT_postEvt(te->super._sender, EVT_UPCAST(te));
}