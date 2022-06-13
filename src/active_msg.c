#include <stdbool.h>
#include <active.h>

static void ACT_Evt_init(ACT_Evt *const e, Active const *const me, ACT_EvtType type)
{

  ACTP_ASSERT(e != NULL, "ACT_Evt is NULL");
  ACTP_ASSERT(type != UNUSED, "ACT_EvtT type is UNUSED");
  ACTP_ASSERT(me != NULL, "Active object sender not set for event");

  e->type = type;
  e->_sender = (Active *)me;

  // Cast away const to clear dynamic field
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = false;

  // Cast away const to clear reference count
  refCnt_t *cnt = (refCnt_t *)&(e->_refcnt);
  *cnt = 0;
}
void Signal_init(ACT_Signal *s, Active const *const me, uint16_t sig)
{
  ACTP_ASSERT(sig >= (uint16_t)ACT_USER_SIG, "Signal type is invalid user defined signal");

  ACT_Evt_init(EVT_UPCAST(s), me, SIGNAL);
  s->sig = sig;
}

void Message_init(Message *m, Active const *const me, uint16_t header, void *payload, uint16_t payloadLen)
{
  ACT_Evt_init(EVT_UPCAST(m), me, MESSAGE);
  m->header = header;
  m->payload = payload;
  m->payloadLen = payloadLen;
}

void TimeEvt_init(TimeEvt *const te, const Active *const me, ACT_Evt *const e, Active const *const receiver, TimerExpiryHandler expFn)
{

  ACTP_ASSERT((e == NULL && expFn != NULL) || e != NULL, "ACT_Evt is null without expiry function set");

  // Initialize timer event
  ACT_Evt_init(EVT_UPCAST(te), me, TIMEREVT);

  te->e = e;
  te->receiver = receiver;
  te->expFn = expFn;
  ACT_Timer_init(te);
}
