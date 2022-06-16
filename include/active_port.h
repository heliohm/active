#ifndef ACTIVE_PORT_H
#define ACTIVE_PORT_H

#include <stdalign.h>

#include <active_types.h>

/*******************************
 *  Compiler intrinsics
 ******************************/

#ifdef __GNUC__
#else
#error "No supported compiler found"
#endif /* __GNUC__ */

/*******************************
 *  Platform port
 ******************************/

#include <zephyr.h>

#ifdef __ZEPHYR__

/* Zephyr implemtation of asserts */
#define ACT_ASSERT(test, errMsg, ...) __ASSERT(test, errMsg, ##__VA_ARGS__) // Runtime assert
#define ACT_CASSERT(test, errMsg) BUILD_ASSERT(test, errMsg)                // Compile time assert

/**
 * @brief Zephyr RTOS port of a queue used by the Active framework
 *
 */

/* Declare a  message queue with name qSym.
Used by application to set up a queue */
#define ACT_Q(qSym) struct k_msgq qSym

/* @internal - Declare a pointer to a message queue with name qPtrSym. Used by generic active header file */
#define ACT_QPTR(qPtrSym) struct k_msgq *qPtrSym

/* Declare a message queue buffer with name bufName and room for maxMsg messages.
Used by application to set up a buffer for the queue */
#define ACT_QBUF(bufSym, maxMsg) char alignas(alignof(ACT_Evt *)) bufSym[sizeof(ACT_Evt *) * maxMsg]

/* @internal - Get an entry from the message queue. Used by active framework to get Events in Active objects.
Function blocks Active object forever until message is put on its queue  */
#define ACT_Q_GET(qPtrSym, evtPtrPtr) k_msgq_get((struct k_msgq *)qPtrSym, evtPtrPtr, K_FOREVER)
#define ACT_Q_GET_SUCCESS_STATUS 0

/* @internal - Put an entry on the message queue. Used by active framework to post Events to Active objects.
Function does not block but returns immediately */
#define ACT_Q_PUT(qPtrSym, evtPtrPtr) k_msgq_put((struct k_msgq *)qPtrSym, evtPtrPtr, K_NO_WAIT);
#define ACT_Q_PUT_SUCCESS_STATUS 0

/**
 * @brief Zephyr RTOS port of threads used by the Active framework
 *
 */

/* Declare a thread
Used by the application to set up a thread for the active object */
#define ACT_THREAD(threadSym) struct k_thread threadSym

/* @internal - Declare a pointer to thread handler. Used by generic active header file */
#define ACT_THREADPTR(threadPtrSym) k_tid_t threadPtrSym

/* Declare *and* initialize a thread stack.
Used by the application to set up a stack for the active object thread */
#define ACT_THREAD_STACK_DEFINE(stackSym, size) K_THREAD_STACK_DEFINE(stackSym, size)

/* @internal - Declare a thread stack pointer */
#define ACT_THREAD_STACKPTR(stackPtrSym) k_thread_stack_t *stackPtrSym;

/* Declares a size_t type with name stackSizeSym and initialize with the size of the thread stack.
   stackSym must point to the stack array directly.

   Used by the application to calculate the true stack size (excl overhead)
 */
#define ACT_THREAD_STACK_SIZE(stackSizeSym, stackSym) const size_t stackSizeSym = K_THREAD_STACK_SIZEOF(stackSym)

/* Returns a thread priority. Higher number -> lower pri in Zephyr.
Main thread has defauly pri 0. Non-negative numbers are preemptive threads
https://docs.zephyrproject.org/latest/kernel/services/threads/index.html#thread-priorities */
#define ACT_THREAD_PRI(x) (x)

/**
 * @brief Zephyr RTOS port of a timer
 *
 */

/* @internal - Declare a timer. Used by Time events */
#define ACT_TIMER(timerSym) struct k_timer timerSym

/* @internal - Declare a pointer to a timer */
#define ACT_TIMERPTR(timerPtrSym) struct k_timer *timerPtrSym

/* @internal - Initialize an ACT_Timer struct */
#define ACT_TIMER_INIT(timerPtr, expiryFn) k_timer_init(&(timerPtr->impl), expiryFn, NULL)

/* @internal - Set ACT_Timer application defined parameter */
#define ACT_TIMER_PARAM_SET(timerPtr, param) k_timer_user_data_set(&(timerPtr->impl), (void *)param)
/* @internal - Get application defined parameter from native (port) timer. */
#define ACT_TIMER_PARAM_GET(nativeTimerPtr) k_timer_user_data_get(nativeTimerPtr)

/* @internal - Start ACT_Timer */
#define ACT_TIMER_START(timerPtr, durationMs, periodMs) k_timer_start(&(timerPtr->impl), K_MSEC(durationMs), K_MSEC(periodMs))
/* @internal - Stop ACT_Timer */
#define ACT_TIMER_STOP(timerPtr) k_timer_stop(&(timerPtr->impl));

/**
 * @brief Zepphyr RTOS port of a memory pool with fixed size objects
 *
 */

/* @internal  - Declare *and* initialize a static memory pool */
#define ACT_MEMPOOL_DEFINE(memPoolSym, type, numObjects) K_MEM_SLAB_DEFINE(memPoolSym, sizeof(type), numObjects, alignof(type))

/* @internal - Get number of used entries in memory pool. Used for testing */
#define ACT_MEMPOOL_USED_GET(memPoolPtr) (memPoolPtr)->num_used

/* @internal - Allocate memory for an object from a specified memory pool */
#define ACT_MEMPOOL_ALLOC(memPoolPtr, dataPptr) k_mem_slab_alloc(memPoolPtr, (void *)dataPptr, K_NO_WAIT)
/* @internal - Free memory for an object from a specified memory pool */
#define ACT_MEMPOOL_FREE(memPoolPtr, dataPptr) k_mem_slab_free(memPoolPtr, (void *)dataPptr)
/* @internal - Allocation return status on success */
#define ACT_MEMPOOL_ALLOC_SUCCESS_STATUS 0

/* Declare a memory pool */
// #define ACT_MEMPOOL(memPoolSym) struct k_mem_slab memPoolSym;

/**
 * @brief Zephyr port of debug printing
 *
 */

/* Debug print macro. Using ##__VA_ARGS__ GNU extension to swallow comma on missing extra parameters. Supported by GCC, Clang, XLC */
#if ACT_CFG_DEBUG_PRINT == 1
#define ACT_DBGPRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define ACT_DBGPRINT(fmt, ...)
#endif

/**
 * @brief Zephyr port of thread sleep / time functions (for examples and tests)
 *
 */

/* Sleep for ms milliseconds */
#define ACT_SLEEPMS(ms) k_msleep(ms)

/* Sleep for us microseconds. Use with caution - minimum resolution in Zephyr RTOS
set by clock hardware and CONFIG_SYS_CLOCK_TICKS_PER_SEC */
#define ACT_SLEEPUS(us) k_usleep(us)

/* Get current time in ms - used by tests */
#define ACT_TIMEMS_GET() k_uptime_get()

/*******************************
 *  Platform specific functions
 **************************** */

/**
 * @brief Zephyr k_timer expiry function. Called by timer ISR when a k_timer expires.
 * Used as adapter between native timer and Active Time event
 *
 */
void ACT_NativeTimerExpiryFn(ACT_TIMERPTR(nativeTimerPtr));

#else
#error "No supported port of Active library found"
#endif // __ZEPHYR__

#endif /* ACTIVE_PORT_H */