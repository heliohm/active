#include <active.h>

/*
struct mempoolData
{
  ACTP_MEMPOOL(impl)
};
*/

static ACTP_MEMPOOL_DEFINE(TimeEvt_Mem, TimeEvt, ACT_MEM_NUM_TIMEEVT);
static ACTP_MEMPOOL_DEFINE(Signal_Mem, Signal, ACT_MEM_NUM_SIGNALS);
static ACTP_MEMPOOL_DEFINE(Message_Mem, Message, ACT_MEM_NUM_MESSAGES);
// static ACTP_MEMPOOL_DEFINE(MemPool_Mem, ACT_Mempool, ACT_MEM_NUM_OBJPOOLS);

/* @private - used by Active framework tests */
uint32_t ACT_mem_Signal_getUsed()
{
  return ACTP_MEMPOOL_USED_GET(&Signal_Mem);
}
/* @private - used by Active framework tests */
uint32_t ACT_mem_Message_getUsed()
{
  return ACTP_MEMPOOL_USED_GET(&Message_Mem);
}
/* @private - used by Active framework tests */
uint32_t ACT_mem_TimeEvt_getUsed()
{
  return ACTP_MEMPOOL_USED_GET(&TimeEvt_Mem);
}
static bool ACT_mem_isDynamic(const Event *const e)
{
  return e->_dynamic;
}

void ACT_mem_refinc(const Event *e)
{
  if (ACT_mem_isDynamic(e))
  {
    atomic_fetch_add((refCnt_t *)&(e->_refcnt), 1);
    ACTP_ASSERT(atomic_load(&(e->_refcnt)) != 0, "Overflow in reference counter. Event ptr: %p", (void *)e);
  }
}

void ACT_mem_refdec(const Event *e)
{
  if (ACT_mem_isDynamic(e))
  {
    ACTP_ASSERT(atomic_load(&(e->_refcnt)) > 0, "Underflow in reference counter. Event ptr: %p", (void *)e);
    atomic_fetch_sub((refCnt_t *)&(e->_refcnt), 1);
    ACT_mem_gc(e);
  }
}

refCnt_t ACT_mem_getRefCount(const Event *const e)
{
  return atomic_load(&e->_refcnt);
}

static void ACT_mem_setDynamic(const Event *const e)
{
  // Cast away const, set dynamic flag to true
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = true;

  atomic_init((refCnt_t *)&(e->_refcnt), 0);
}

void ACT_mem_gc(const Event *e)
{
  if (ACT_mem_getRefCount(e) != 0)
  {
    return;
  }
  if (!ACT_mem_isDynamic(e))
  {
    return;
  }

  switch (e->type)
  {
  case SIGNAL:
  {
    ACTP_ASSERT(ACTP_MEMPOOL_USED_GET(&Signal_Mem) > 0, "No Signal events to free");
    ACTP_MEMPOOL_FREE(&Signal_Mem, &e);
    break;
  }
  case MESSAGE:
  {
    ACTP_ASSERT(ACTP_MEMPOOL_USED_GET(&Message_Mem) > 0, "No Message events to free");
    ACTP_MEMPOOL_FREE(&Message_Mem, &e);
    break;
  }
  case TIMEREVT:
  {
    ACTP_ASSERT(ACTP_MEMPOOL_USED_GET(&TimeEvt_Mem) > 0, "No time events to free");
    ACTP_MEMPOOL_FREE(&TimeEvt_Mem, &e);
    break;
  }
  default:
    ACTP_ASSERT(0, "Invalid event type");
  }
}

Signal *Signal_new(Active const *const me, uint16_t sig)
{

  Signal *s = NULL;
  int status = ACTP_MEMPOOL_ALLOC(&Signal_Mem, &s);
  ACTP_ASSERT(status == ACTP_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Signal");

  // Initialize signal as static
  Signal_init(s, me, sig);

  // Set event dynamic *after* initialization
  ACT_mem_setDynamic(EVT_UPCAST(s));

  ARG_UNUSED(status);
  return s;
}

Message *Message_new(Active const *const me, uint16_t msgHeader, void *msgPayload, uint16_t payloadLen)
{
  Message *m;
  int status = ACTP_MEMPOOL_ALLOC(&Message_Mem, &m);
  ACTP_ASSERT(status == ACTP_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Message");

  // Initialize message as static
  Message_init(m, me, msgHeader, msgPayload, payloadLen);

  // Set event dynamic *after* initialization
  ACT_mem_setDynamic(EVT_UPCAST(m));

  ARG_UNUSED(status);
  return m;
}

// Event to fire on one-shot or periodic timer. For Pub-Sub messages, receiver should be set to NULL.
TimeEvt *TimeEvt_new(Event *const e, const Active *const me, const Active *const receiver, TimerExpiryHandler expFn)
{

  TimeEvt *te = NULL;
  int status = ACTP_MEMPOOL_ALLOC(&TimeEvt_Mem, &te);
  ACTP_ASSERT(status == ACTP_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Time Event");

  TimeEvt_init(te, me, e, receiver, expFn);

  // Set event dynamic *after* initialization
  ACT_mem_setDynamic(EVT_UPCAST(te));

  ARG_UNUSED(status);
  return te;
}
/*
ACT_Mempool *ACT_Mempool_new(void *memBuf, size_t objSize, size_t numObjects)
{

  ACT_Mempool *mem = NULL;
  int status_pool = ACTP_MEMPOOL_ALLOC(&MemPool_Mem, &mem);
  ACTP_ASSERT(status_pool == ACTP_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Memory pool");

  int status_init = k_mem_slab_init(&(mem->impl), memBuf, objSize, numObjects);
  ACTP_ASSERT(status_init == ACTP_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to init new Memory pool");

  ARG_UNUSED(status_pool);
  ARG_UNUSED(status_init);

  return mem;
}

void ACT_Mempool_free(ACT_Mempool *memPool)
{
  k_mem_slab_free(&MemPool_Mem, (void *)&memPool);
}

void *Object_new(ACT_Mempool *poolptr)
{
  void *obj;
  int status = k_mem_slab_alloc(&(poolptr->impl), &obj, K_NO_WAIT);
  ACTP_ASSERT(status == ACTP_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Object");

  ARG_UNUSED(status);
  return obj;
}
*/