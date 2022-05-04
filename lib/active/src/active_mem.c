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

uint32_t Active_mem_TimeEvt_getUsed()
{
  return TimeEvt_Mem.num_used;
}

static bool Active_mem_isDynamic(const Event *const e)
{
  return e->_dynamic;
}

void Active_mem_refinc(const Event *e)
{
  if (Active_mem_isDynamic(e))
  {
    // ATOMIC_ADD((RefCnt *)&e->_refcnt, 1);
    atomic_add((atomic_t *)&(e->_refcnt), 1);
    // ACTIVE_ASSERT(ATOMIC_GET((RefCnt *)&(e->_refcnt)) != 0, "Overflow in reference counter. Event ptr: %p", (void *)e);
    ACTIVE_ASSERT(atomic_get(&(e->_refcnt)) != 0, "Overflow in reference counter. Event ptr: %p", (void *)e);
  }
}

void Active_mem_refdec(const Event *e)
{
  if (Active_mem_isDynamic(e))
  {
    // ACTIVE_ASSERT(ATOMIC_GET((RefCnt *)&(e->_refcnt)) > 0, "Underflow in reference counter. Event ptr: %p", (void *)e);
    ACTIVE_ASSERT(atomic_get(&(e->_refcnt)) > 0, "Underflow in reference counter. Event ptr: %p", (void *)e);
    // ATOMIC_DEC((RefCnt *)&(e->_refcnt), 1);
    atomic_dec((atomic_t *)&(e->_refcnt));

    Active_mem_gc(e);
  }
}

/* RefCnt Active_mem_getRefCount(const Event *const e)
{
  return e->_refcnt;
}
*/
atomic_t Active_mem_getRefCount(const Event *const e)
{
  return atomic_get(&e->_refcnt);
}

static void Active_mem_setDynamic(const Event *const e)
{
  // Cast away const, set dynamic flag to true
  bool *dyn = (bool *)&(e->_dynamic);
  *dyn = true;

  // Cast away const, set ref counter to 0
  // RefCnt *ref = (RefCnt *)&(e->_refcnt);
  // *ref = 0;
  atomic_t *ref = (atomic_t *)&e->_refcnt;
  atomic_clear(ref);
}

void Active_mem_gc(const Event *e)
{
  if (Active_mem_isDynamic(e))
  {
    // No more references -> free event
    // if (ATOMIC_GET((RefCnt *)&(e->_refcnt)) == 0)
    if (Active_mem_getRefCount(e) == 0)
    {

      switch (e->type)
      {

      case TIMEREVT:
      {
        k_mem_slab_free(&TimeEvt_Mem, (void *)&e);
        break;
      }
      case SIGNAL:
      {

        k_mem_slab_free(&Signal_Mem, (void *)&e);
        break;
      }
      case MESSAGE:
      {
        ACTIVE_ASSERT(0, "Not implemented");
        if (EVT_CAST(e, Message)->header & 0x0) // MSG_HEADER_DYNAMIC_PAYLOAD)
        {

          // e->_sender->freeFxn(EVT_CAST(e, Message)->payLoad);
        }
        // Then free the message struct
        k_mem_slab_free(&Message_Mem, (void *)&e);
        break;
      }

      default:
        ACTIVE_ASSERT(0, "Invalid event type");
      }
    }
  }
}

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

ACTIVE_CASSERT(sizeof(TimeEvt) == 92, "TimeEvt type is not the right size.");
ACTIVE_CASSERT(ALIGNOF(TimeEvt) == 4, "Alignment TimeEvt type");

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

/* Zephyr limitation of alignment of memory buffers and object size:
https://docs.zephyrproject.org/latest/reference/kernel/memory/slabs.html#concepts */
ACTIVE_CASSERT(sizeof(Signal) == 20, "Signal type is not the right size.");
ACTIVE_CASSERT(ALIGNOF(Signal) == 4, "Alignment Signal type");

Message *Message_new()
{
  Message *m = NULL;
  ACTIVE_ASSERT(0, "not implemented yet");
  return m;
}
ACTIVE_CASSERT(sizeof(Message) == 28, "Message type is not the right size.");
ACTIVE_CASSERT(ALIGNOF(Message) == 4, "Alignment Message type");

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

  return obj;
}
