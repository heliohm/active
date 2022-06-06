#include <active.h>
#include <zephyr.h>

struct mempoolData
{
  ACTIVE_MEMPOOL(impl)
};

static ACTIVE_MEMPOOL_DEFINE(TimeEvt_Mem, TimeEvt, ACTIVE_MEM_NUM_TIMEREVT);
static ACTIVE_MEMPOOL_DEFINE(Signal_Mem, Signal, ACTIVE_MEM_NUM_SIGNALS);
static ACTIVE_MEMPOOL_DEFINE(Message_Mem, Message, ACTIVE_MEM_NUM_MESSAGES);
static ACTIVE_MEMPOOL_DEFINE(MemPool_Mem, Active_Mempool, ACTIVE_MEM_NUM_OBJPOOLS);

/* @private - used by Active framework tests */
uint32_t Active_mem_Signal_getUsed()
{
  return Signal_Mem.num_used;
}
/* @private - used by Active framework tests */
uint32_t Active_mem_TimeEvt_getUsed()
{
  return TimeEvt_Mem.num_used;
}
/* @private - used by Active framework tests */
uint32_t Active_mem_Message_getUsed()
{
  return Message_Mem.num_used;
}

static bool Active_mem_isDynamic(const Event *const e)
{
  return e->_dynamic;
}

void Active_mem_refinc(const Event *e)
{
  if (Active_mem_isDynamic(e))
  {
    atomic_fetch_add((refCnt_t *)&(e->_refcnt), 1);
    ACTIVE_ASSERT(atomic_load(&(e->_refcnt)) != 0, "Overflow in reference counter. Event ptr: %p", (void *)e);
  }
}

void Active_mem_refdec(const Event *e)
{
  if (Active_mem_isDynamic(e))
  {
    ACTIVE_ASSERT(atomic_load(&(e->_refcnt)) > 0, "Underflow in reference counter. Event ptr: %p", (void *)e);
    atomic_fetch_sub((refCnt_t *)&(e->_refcnt), 1);
    Active_mem_gc(e);
  }
}

refCnt_t Active_mem_getRefCount(const Event *const e)
{
  return atomic_load(&e->_refcnt);
}

static void Active_mem_setDynamic(const Event *const e)
{
  // Cast away const, set dynamic flag to true
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = true;

  atomic_init((refCnt_t *)&(e->_refcnt), 0);
}

void Active_mem_gc(const Event *e)
{
  if (Active_mem_getRefCount(e) != 0)
  {
    return;
  }
  if (!Active_mem_isDynamic(e))
  {
    return;
  }

  switch (e->type)
  {
  case TIMEREVT:
  {
    ACTIVE_ASSERT(TimeEvt_Mem.num_used > 0, "No time events to free");
    k_mem_slab_free(&TimeEvt_Mem, (void *)&e);
    break;
  }
  case SIGNAL:
  {
    ACTIVE_ASSERT(Signal_Mem.num_used > 0, "No Signal events to free");
    k_mem_slab_free(&Signal_Mem, (void *)&e);
    break;
  }
  case MESSAGE:
  {
    ACTIVE_ASSERT(Message_Mem.num_used > 0, "No Message events to free");
    k_mem_slab_free(&Message_Mem, (void *)&e);
    break;
  }

  default:
    ACTIVE_ASSERT(0, "Invalid event type");
  }
}

Signal *Signal_new(Active const *const me, uint16_t sig)
{

  Signal *s = NULL;
  int status = k_mem_slab_alloc(&Signal_Mem, (void *)&s, K_NO_WAIT);
  ACTIVE_ASSERT(status == 0, "Failed to allocate new Signal");

  // Initialize signal as static
  Signal_init(s, me, sig);

  // Set event dynamic *after* initialization
  Active_mem_setDynamic(EVT_UPCAST(s));

  ARG_UNUSED(status);
  return s;
}
ACTIVE_CASSERT(sizeof(Signal) == 16, "Signal type is not the right size.");
ACTIVE_CASSERT(ALIGNOF(Signal) == 4, "Alignment Signal type");

Message *Message_new(Active const *const me, uint16_t msgHeader, void *msgPayload, uint16_t payloadLen)
{
  Message *m;
  int status = k_mem_slab_alloc(&Message_Mem, (void *)&m, K_NO_WAIT);
  ACTIVE_ASSERT(status == 0, "Failed to allocate new Message");

  // Initialize message as static
  Message_init(m, me, msgHeader, msgPayload, payloadLen);

  // Set event dynamic *after* initialization
  Active_mem_setDynamic(EVT_UPCAST(m));

  ARG_UNUSED(status);
  return m;
}
ACTIVE_CASSERT(sizeof(Message) == 20, "Message type is not the right size.");
ACTIVE_CASSERT(ALIGNOF(Message) == 4, "Alignment Message type");

// Event to fire on one-shot or periodic timer. For Pub-Sub messages, receiver should be set to NULL.
TimeEvt *TimeEvt_new(Event *const e, const Active *const me, const Active *const receiver, TimerExpiryHandler expFn)
{

  TimeEvt *te = NULL;
  int status = k_mem_slab_alloc(&TimeEvt_Mem, (void *)&te, K_NO_WAIT);
  ACTIVE_ASSERT(status == 0, "Failed to allocate new Time Event");

  TimeEvt_init(te, me, e, receiver, expFn);

  // Set event dynamic *after* initialization
  Active_mem_setDynamic(EVT_UPCAST(te));

  ARG_UNUSED(status);
  return te;
}
ACTIVE_CASSERT(sizeof(TimeEvt) == 96, "TimeEvt type is not the right size.");
ACTIVE_CASSERT(ALIGNOF(TimeEvt) == 4, "Alignment TimeEvt type");

Active_Mempool *Active_Mempool_new(void *memBuf, size_t objSize, size_t numObjects)
{

  Active_Mempool *mem = NULL;
  int status_pool = k_mem_slab_alloc(&MemPool_Mem, (void *)&mem, K_NO_WAIT);
  ACTIVE_ASSERT(status_pool == 0, "Failed to allocate new Memory pool");

  int status_init = k_mem_slab_init(&(mem->impl), memBuf, objSize, numObjects);
  ACTIVE_ASSERT(status_init == 0, "Failed to init new Memory pool");

  ARG_UNUSED(status_pool);
  ARG_UNUSED(status_init);

  return mem;
}

void Active_Mempool_free(Active_Mempool *memPool)
{
  k_mem_slab_free(&MemPool_Mem, (void *)&memPool);
}

void *Object_new(Active_Mempool *poolptr)
{
  void *obj;
  int status = k_mem_slab_alloc(&(poolptr->impl), &obj, K_NO_WAIT);
  ACTIVE_ASSERT(status == 0, "Failed to allocate new Object");

  ARG_UNUSED(status);
  return obj;
}
