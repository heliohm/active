#include <active.h>

/*
struct active_mempoolData
{
  ACT_MEMPOOL(impl)
};
*/

static ACT_MEMPOOL_DEFINE(Signal_Mem, ACT_Signal, ACT_MEM_NUM_SIGNALS);
static ACT_MEMPOOL_DEFINE(Message_Mem, ACT_Message, ACT_MEM_NUM_MESSAGES);
static ACT_MEMPOOL_DEFINE(TimeEvt_Mem, ACT_TimEvt, ACT_MEM_NUM_TIMEEVT);
// static ACT_MEMPOOL_DEFINE(MemPool_Mem, ACT_Mempool, ACT_MEM_NUM_OBJPOOLS);

/* @private - used by Active framework tests */
uint32_t ACT_mem_Signal_getUsed()
{
  return ACT_MEMPOOL_USED_GET(&Signal_Mem);
}
/* @private - used by Active framework tests */
uint32_t ACT_mem_Message_getUsed()
{
  return ACT_MEMPOOL_USED_GET(&Message_Mem);
}
/* @private - used by Active framework tests */
uint32_t ACT_mem_TimeEvt_getUsed()
{
  return ACT_MEMPOOL_USED_GET(&TimeEvt_Mem);
}
static bool ACT_mem_isDynamic(const ACT_Evt *const e)
{
  return e->_dynamic;
}

void ACT_mem_refinc(const ACT_Evt *e)
{
  if (ACT_mem_isDynamic(e))
  {
    atomic_fetch_add((refCnt_t *)&(e->_refcnt), 1);
    ACT_ASSERT(atomic_load(&(e->_refcnt)) != 0, "Overflow in reference counter. ACT_Evt ptr: %p", (void *)e);
  }
}

void ACT_mem_refdec(const ACT_Evt *e)
{
  if (ACT_mem_isDynamic(e))
  {
    ACT_ASSERT(atomic_load(&(e->_refcnt)) > 0, "Underflow in reference counter. ACT_Evt ptr: %p", (void *)e);
    atomic_fetch_sub((refCnt_t *)&(e->_refcnt), 1);
    ACT_mem_gc(e);
  }
}

refCnt_t ACT_mem_getRefCount(const ACT_Evt *const e)
{
  return atomic_load(&e->_refcnt);
}

static void ACT_mem_setDynamic(const ACT_Evt *const e)
{
  // Cast away const, set dynamic flag to true
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = true;
}

void ACT_mem_gc(const ACT_Evt *e)
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
  case ACT_SIGNAL:
  {
    ACT_ASSERT(ACT_MEMPOOL_USED_GET(&Signal_Mem) > 0, "No Signal events to free");
    ACT_MEMPOOL_FREE(&Signal_Mem, &e);
    break;
  }
  case ACT_MESSAGE:
  {
    ACT_ASSERT(ACT_MEMPOOL_USED_GET(&Message_Mem) > 0, "No Message events to free");
    ACT_MEMPOOL_FREE(&Message_Mem, &e);
    break;
  }
  case ACT_TIMEVT:
  {
    ACT_ASSERT(ACT_MEMPOOL_USED_GET(&TimeEvt_Mem) > 0, "No time events to free");
    ACT_MEMPOOL_FREE(&TimeEvt_Mem, &e);
    break;
  }
  default:
    ACT_ASSERT(0, "Invalid event type");
  }
}

ACT_Signal *ACT_Signal_new(Active const *const me, uint16_t sig)
{

  ACT_Signal *s = NULL;
  int status = ACT_MEMPOOL_ALLOC(&Signal_Mem, &s);
  ACT_ASSERT(status == ACT_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Signal");

  // Initialize signal as static
  ACT_Signal_init(s, me, sig);

  // Set event dynamic *after* initialization
  ACT_mem_setDynamic(EVT_UPCAST(s));

  ACT_ARG_UNUSED(status);
  return s;
}

ACT_Message *ACT_Message_new(Active const *const me, uint16_t msgHeader, void *msgPayload, uint16_t payloadLen)
{
  ACT_Message *m = NULL;
  int status = ACT_MEMPOOL_ALLOC(&Message_Mem, &m);
  ACT_ASSERT(status == ACT_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Message");

  // Initialize message as static
  ACT_Message_init(m, me, msgHeader, msgPayload, payloadLen);

  // Set event dynamic *after* initialization
  ACT_mem_setDynamic(EVT_UPCAST(m));

  ACT_ARG_UNUSED(status);
  return m;
}

ACT_TimEvt *ACT_TimEvt_new(ACT_Evt *const e, const Active *const me, const Active *const receiver, ACT_TimerExpiryFn expFn)
{

  ACT_TimEvt *te = NULL;
  int status = ACT_MEMPOOL_ALLOC(&TimeEvt_Mem, &te);
  ACT_ASSERT(status == ACT_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Time Event");

  ACT_TimEvt_init(te, me, e, receiver, expFn);

  // Set event dynamic *after* initialization
  ACT_mem_setDynamic(EVT_UPCAST(te));

  ACT_ARG_UNUSED(status);
  return te;
}
/*
ACT_Mempool *ACT_Mempool_new(void *memBuf, size_t objSize, size_t numObjects)
{

  ACT_Mempool *mem = NULL;
  int status_pool = ACT_MEMPOOL_ALLOC(&MemPool_Mem, &mem);
  ACT_ASSERT(status_pool == ACT_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Memory pool");

  int status_init = k_mem_slab_init(&(mem->impl), memBuf, objSize, numObjects);
  ACT_ASSERT(status_init == ACT_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to init new Memory pool");

  ACT_ARG_UNUSED(status_pool);
  ACT_ARG_UNUSED(status_init);

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
  ACT_ASSERT(status == ACT_MEMPOOL_ALLOC_SUCCESS_STATUS, "Failed to allocate new Object");

  ACT_ARG_UNUSED(status);
  return obj;
}
*/