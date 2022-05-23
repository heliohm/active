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
void Signal_init(Signal *s, Active const *const me, uint16_t sig)
{
  Event_init(EVT_UPCAST(s), me, SIGNAL);
  ACTIVE_ASSERT(sig >= (uint16_t)USER_SIG, "Signal type is invalid user defined signal");
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

  ACTIVE_ASSERT((e == NULL && expFn != NULL) || e != NULL, "Event is null without expiry function set");

  // Initialize timer event
  Event_init(EVT_UPCAST(te), me, TIMEREVT);

  te->e = e;
  te->receiver = receiver;
  te->expFn = expFn;
  Active_Timer_init(te);
}
