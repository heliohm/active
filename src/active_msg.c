#include <stdbool.h>
#include <active.h>

static void ACT_Evt_init(ACT_Evt *const e, Active const *const me, ACT_EvtType type)
{

  ACT_ASSERT(e != NULL, "ACT_Evt is NULL");
  ACT_ASSERT(me != NULL, "Active object sender not set for event");

  e->type = type;
  e->_sender = (Active *)me;

  // Cast away const to clear dynamic field
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = false;

  // Cast away const to clear reference count
  refCnt_t *cnt = (refCnt_t *)&(e->_refcnt);
  *cnt = 0;
}
void ACT_Signal_init(ACT_Signal *s, Active const *const me, uint16_t sig)
{
  ACT_ASSERT(sig >= (uint16_t)ACT_USER_SIG, "Signal type is invalid user defined signal");

  ACT_Evt_init(EVT_UPCAST(s), me, ACT_SIGNAL);
  s->sig = sig;
}

void ACT_Message_init(ACT_Message *m, Active const *const me, uint16_t header, void *payload, uint16_t payloadLen)
{
  ACT_Evt_init(EVT_UPCAST(m), me, ACT_MESSAGE);
  m->header = header;
  m->payload = payload;
  m->payloadLen = payloadLen;
}

void ACT_TimEvt_init(ACT_TimEvt *const te, const Active *const me, ACT_Evt *const e, Active const *const receiver, ACT_TimerExpiryFn expFn)
{

  ACT_ASSERT((e == NULL && expFn != NULL) || e != NULL, "ACT_Evt is null without expiry function set");

  // Initialize timer event
  ACT_Evt_init(EVT_UPCAST(te), me, ACT_TIMEVT);

  te->e = e;
  te->receiver = receiver;
  te->expFn = expFn;
  ACT_Timer_init(te);
}
