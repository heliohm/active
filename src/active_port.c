#include <active.h>

#ifdef __ZEPHYR__

#include <stdalign.h>
#include <zephyr.h>

/* Zephyr puts limits on aligment of queue buffer and size of queue content (Event *):
https://docs.zephyrproject.org/latest/reference/kernel/data_passing/message_queues.html */

ACTIVE_CASSERT(sizeof(Event) == 12, "Event type is not the right size.");
ACTIVE_CASSERT(alignof(Event *) == 4, "Alignment of Event pointer type must be a power of 2");

/* Zephyr limitations on memory slab alignment and object size:
https://docs.zephyrproject.org/latest/kernel/memory_management/slabs.html */

ACTIVE_CASSERT(sizeof(TimeEvt) == 96, "TimeEvt type is not the right size.");
ACTIVE_CASSERT(alignof(TimeEvt) == 8, "Alignment TimeEvt type");

ACTIVE_CASSERT(sizeof(Message) == 20, "Message type is not the right size.");
ACTIVE_CASSERT(alignof(Message) == 4, "Alignment Message type");

ACTIVE_CASSERT(sizeof(Signal) == 16, "Signal type is not the right size.");
ACTIVE_CASSERT(alignof(Signal) == 4, "Alignment Signal type");

/* Zephyr thread entry function */
static void active_entry(void *arg1, void *arg2, void *arg3)
{
  Active_eventLoop((Active *const)arg1);
}

void Active_init(Active *const me, DispatchHandler dispatch, queueData const *qd, threadData const *td)
{
  ACTIVE_ASSERT(me != NULL, "Active object is null)");
  ACTIVE_ASSERT(dispatch != NULL, "Dispatch handler is null");

  me->dispatch = dispatch;

  k_msgq_init(qd->queue, qd->queBuf, sizeof(Event *), qd->maxMsg);

  me->queue = qd->queue;
  me->thread = k_thread_create(td->thread, td->stack, td->stack_size, active_entry, (void *)me, NULL, NULL, td->pri, 0, K_FOREVER);
}

void Active_start(Active *const me)
{
  k_thread_start(me->thread);
}

/* Hook to stop program on assert failures */
void assert_post_action(const char *file, unsigned int line)
{
  while (1)
  {
  }
}

#endif /* __ZEPHYR__ */