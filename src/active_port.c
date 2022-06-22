#include <stdalign.h>
#include <active.h>

#ifdef __ZEPHYR__

#include <zephyr.h>

/* Zephyr puts limits on aligment of queue buffer and size of queue content (ACT_Evt *):
https://docs.zephyrproject.org/latest/reference/kernel/data_passing/message_queues.html */

_Static_assert(sizeof(ACT_Evt) == 12, "ACT_Evt type is not the right size.");
_Static_assert(alignof(ACT_Evt *) == 4, "Alignment of ACT_Evt pointer type must be a power of 2");

/* Zephyr limitations on memory slab alignment and object size:
https://docs.zephyrproject.org/latest/kernel/memory_management/slabs.html */

_Static_assert(sizeof(ACT_TimEvt) == 96, "ACT_TimEvt type is not the right size.");
_Static_assert(alignof(ACT_TimEvt) == 8, "Alignment ACT_TimEvt type");

_Static_assert(sizeof(ACT_Message) == 20, "ACT_Message type is not the right size.");
_Static_assert(alignof(ACT_Message) == 4, "Alignment ACT_Message type");

_Static_assert(sizeof(ACT_Signal) == 16, "ACT_Signal type is not the right size.");
_Static_assert(alignof(ACT_Signal) == 4, "Alignment ACT_Signal type");

/* Zephyr thread entry function */
static void active_entry(void *arg1, void *arg2, void *arg3)
{
  ACT_threadFn((Active *const)arg1);
}

void ACT_init(Active *const me, ACT_DispatchFn dispatch, ACT_QueueData const *qd, ACT_ThreadData const *td)
{
  ACT_ASSERT(me != NULL, "Active object is null)");
  ACT_ASSERT(dispatch != NULL, "Dispatch handler is null");

  me->dispatch = dispatch;

  k_msgq_init(qd->queue, qd->queBuf, sizeof(ACT_Evt *), qd->maxMsg);

  me->queue = qd->queue;
  me->thread = k_thread_create(td->thread, td->stack, td->stack_size, active_entry, (void *)me, NULL, NULL, td->pri, 0, K_FOREVER);
}

void ACT_start(Active *const me)
{
  k_thread_start(me->thread);
}

void ACT_NativeTimerExpiryFn(ACT_TIMERPTR(nativeTimerPtr))
{
  ACT_TimEvt *te = (ACT_TimEvt *)ACT_TIMER_PARAM_GET(nativeTimerPtr);
  ACT_Timer_expiryCB(te);
}

/* Hook to stop program on assert failures */
void assert_post_action(const char *file, unsigned int line)
{
  while (1)
  {
  }
}

#endif /* __ZEPHYR__ */