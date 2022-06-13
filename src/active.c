#include <active.h>

void ACT_eventLoop(Active *const me)
{
  ACTP_ASSERT(me != NULL, "Active object is null)");

  static const SIGNAL_DEFINE(startSignal, START_SIG);

  // Initialize active object
  me->dispatch(me, EVT_UPCAST(&startSignal));

  while (1)
  {
    Event *e = NULL;
    /* Blocking wait for events */
    int status = ACTP_Q_GET(me->queue, &e);

    ACTP_ASSERT(status == ACTP_Q_GET_SUCCESS_STATUS, "Event was not retrieved. Error: %i", status);
    ARG_UNUSED(status);

    ACTP_ASSERT(e != NULL, "Event object is null");

    // Timer events are not processed by the AO dispatch function.
    // Instead the attached event is processed in the context of the
    // active object that started the timer event
    if (e->type == TIMEREVT)
    {
      TimeEvt *te = EVT_CAST(e, TimeEvt);
      ACT_TimeEvt_dispatch(te);
    }
    // Default: Let AO process event
    else
    {
      me->dispatch(me, e);
    }

    // Decrement reference counter added by ACT_post after event is processed
    ACT_mem_refdec(e);
  }
}

inline int ACT_post(Active const *const receiver, Event const *const e)
{
  ACTP_ASSERT(receiver != NULL, "Receiver is null");
  ACTP_ASSERT(e != NULL, "Event object is null");
  ACTP_ASSERT(e->type != UNUSED, "Event object is not initialized");

  /* Adding a memory ref must be done before putting it on the receiving queue,
  in case receiving object is higher priority than running object
  (which would decrement the ref counter while processingand potentially free it) */
  ACT_mem_refinc(e);

  int status = ACTP_Q_PUT(receiver->queue, &e);
  ACTP_ASSERT(status == ACTP_Q_PUT_SUCCESS_STATUS, "Event not put on queue %p. Error: %i\n\n", receiver->queue, status);

  // Event was not sent, remove memory ref again
  if (status != ACTP_Q_PUT_SUCCESS_STATUS)
  {
    ACT_mem_refdec(e);
  }

  return status;
}

inline int ACT_postTimeEvt(TimeEvt *te)
{
  // Post time event to AO sender's queue so AO framework can
  // update and post the attached event in the sender's context
  return ACT_post(te->super._sender, EVT_UPCAST(te));
}