#include <stdbool.h>
#include <active.h>

static void Event_init(Event *const e, Active const *const me, EvtType t)
{

  ACTP_ASSERT(e != NULL, "Event is NULL");
  ACTP_ASSERT(t != UNUSED, "Event type is UNUSED");
  ACTP_ASSERT(me != NULL, "Active object sender not set for event");

  e->type = t;
  e->_sender = (Active *)me;

  // Cast away const to clear dynamic field
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = false;
}
void Signal_init(Signal *s, Active const *const me, uint16_t sig)
{
  ACTP_ASSERT(sig >= (uint16_t)USER_SIG, "Signal type is invalid user defined signal");

  Event_init(EVT_UPCAST(s), me, SIGNAL);
  s->sig = sig;
}

void Message_init(Message *m, Active const *const me, uint16_t header, void *payload, uint16_t payloadLen)
{
  Event_init(EVT_UPCAST(m), me, MESSAGE);
  m->header = header;
  m->payload = payload;
  m->payloadLen = payloadLen;
}

void TimeEvt_init(TimeEvt *const te, const Active *const me, Event *const e, Active const *const receiver, TimerExpiryHandler expFn)
{

  ACTP_ASSERT((e == NULL && expFn != NULL) || e != NULL, "Event is null without expiry function set");

  // Initialize timer event
  Event_init(EVT_UPCAST(te), me, TIMEREVT);

  te->e = e;
  te->receiver = receiver;
  te->expFn = expFn;
  ACT_Timer_init(te);
}
