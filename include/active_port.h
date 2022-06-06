#ifndef ACTIVE_PORT_H
#define ACTIVE_PORT_H

#include <active_types.h>

/*******************************
 *  Compiler intrinsics
 ******************************/

#ifdef __GNUC__
#define _PACKED_ __attribute__((packed))

#define _INLINE_ inline __attribute__((always_inline))

#define ALIGNAS(n) __attribute__((aligned(n)))
#define ALIGNOF(sym) __alignof__(sym)

#else
#error "No supported compiler found"
#endif /* __GNUC__ */

/*******************************
 *  Platform port
 ******************************/

#include <zephyr.h>

#ifdef __ZEPHYR__

/* Zephyr implemtation of asserts */
#define ACTIVE_ASSERT(test, errMsg, ...) __ASSERT(test, errMsg, ##__VA_ARGS__) // Runtime assert
#define ACTIVE_CASSERT(test, errMsg) BUILD_ASSERT(test, errMsg)                // Compile time assert

/* Zephyr implementation of queues */

/* Declare a message queue with name qSym */
#define ACTIVE_Q(qSym) struct k_msgq qSym

/* Declare a message queue pointer with name qPtrSym. Used by generic active header file */
#define ACTIVE_QPTR(qPtrSym) struct k_msgq *qPtrSym

/* Declare a message queue buffer with name bufName and room for maxMsg messages */
#define ACTIVE_QBUF(bufSym, maxMsg) char ALIGNAS(ALIGNOF(Event *)) bufSym[sizeof(Event *) * maxMsg]

/* Zephyr implementation of threads */

/* Declare a thread handler object */
#define ACTIVE_THREAD(threadSym) struct k_thread threadSym
/* Declare a pointer to thread handler. Used by generic active header file */
#define ACTIVE_THREADPTR(threadPtrSym) k_tid_t threadPtrSym
/* Declare a thread stack */
#define ACTIVE_THREAD_STACK(stackSym, size) K_THREAD_STACK_DEFINE(stackSym, size)
/* Declare a thread stack pointer */
#define ACTIVE_THREAD_STACKPTR(stackPtrSym) k_thread_stack_t *stackPtrSym;

/* Declares a size_t type with name stackSizeSym and initialize with the size of the thread stack.
   Must point to the stack array directly.
 */
#define ACTIVE_THREAD_STACK_SIZE(stackSizeSym, stackSym) const size_t stackSizeSym = K_THREAD_STACK_SIZEOF(stackSym)

/* Zephyr thread priorities. Higher number -> lower pri. Main thread has pri 0. Non-negative numbers are preemptive threads */
#define ACTIVE_THREAD_PRI(x) (x)

/* Declare a timer instance */
#define ACTIVE_TIMER(timerSym) struct k_timer timerSym

/* Declare and initialize a memory pool */
#define ACTIVE_MEMPOOL_DEFINE(memPoolSym, type, numObjects) K_MEM_SLAB_DEFINE(memPoolSym, sizeof(type), numObjects, ALIGNOF(type))

/* Declare a memory pool */
#define ACTIVE_MEMPOOL(memPoolSym) struct k_mem_slab memPoolSym;

/*******************************
 *  Platform specific functions
 **************************** */

/* Thread safe port for pending on the queue until a message is received */
static _INLINE_ void Active_get(void const *qPtr, Event **e)
{

  int status = k_msgq_get((struct k_msgq *)qPtr, e, K_FOREVER);
  ACTIVE_ASSERT(status == 0, "Event was not retrieved. Error: %i", status);

  ARG_UNUSED(status);
}

/* Thread safe port for putting event pointers on the receiving queue */
static _INLINE_ int Active_put(void const *qPtr, Event const *const *const e)
{
  int status = k_msgq_put((struct k_msgq *)qPtr, e, K_NO_WAIT);

  // TODO: Tracing - log k_msgq_numfree_get

  ACTIVE_ASSERT(status == 0, "Event not put on queue %p. Error: %i\n\n", qPtr, status);

  return status;
}

#else
#error "No supported port of Active library found"
#endif // __ZEPHYR__

#endif /* ACTIVE_PORT_H */