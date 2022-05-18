#ifndef ACTIVE_H
#define ACTIVE_H

#include <active_mem.h>
#include <active_msg.h>
#include <active_psmsg.h>
#include <active_port.h>
#include <active_timer.h>
#include <active_types.h>

/***************************
 * Active object definitions
 ***************************/

/* Upcast Active object implementatios */
#define ACTIVE_UPCAST(ptr) ((Active *const)(ptr))

/* Dispatch handler function pointer type for active object implementations */
typedef void (*DispatchHandler)(Active *me, Event const *const e);

/* Active object data structure. To be used by active object implementations through polymorphism */
/**
 * @brief Active object data structure. Do not access from application
 *
 */
struct active
{
  ACTIVE_THREADPTR(thread);
  ACTIVE_QPTR(queue);
  DispatchHandler dispatch;
};

/**
 * @brief Thread/task related data structures for active object.
 * @param thread Pointer to thread data structure declared by ACTIVE_THREAD macro
 * @param stack Pointer to task stack declared by ACTIVE_THREAD_STACK
 * @param stack_size Size of task stack calculated by ACTIVE_THREAD_STACK_SIZE
 * @param pri Task priority (port specific)
 */
struct threadData
{
  ACTIVE_THREADPTR(thread);
  ACTIVE_THREAD_STACKPTR(stack);
  size_t stack_size;
  int pri;
};

/* Queue data structures to set up an active object */
/**
 * @brief Queue related data structures for active object.
 * @param queue Pointer to queue data structure declared by ACTIVE_Q
 * @param queBuf Pointer to message buffer that holds Event pointers sent to active object declared by ACTIVE_QBUF
 * @param maxMsg Maximum number of unprocessed messages that the active object message queue can hold
 */
struct queueData
{
  ACTIVE_QPTR(queue);
  char *queBuf;
  size_t maxMsg;
};

/**
 * @brief Initialize an active object data structure beforing using it.
 *
 * @param me Pointer to the active object data structure to initialize
 * @param dispatch Application dispatch function that will receive events
 * @param qd Pointer to queue related data needed by active object
 * @param td Pointer to thread/task related data needed by active object
 */
void Active_init(Active *const me, DispatchHandler dispatch, queueData const *qd, threadData const *td);
/**
 * @brief Start the execution of an active object. When the active object starts, the dispatch function will
 * receive a signal with START_SIG payload that can be used to initialize the application.
 *
 * @param me Pointer to the active object that should be started
 */
void Active_start(Active *const me);

/* @private - Used by Active framework ports */
void Active_eventLoop(Active *me);

/* Post event directly to receiver */
/**
 * @brief Post an event directly to a receiving active object
 *
 * @param receiver Pointer to the receiving active object
 * @param e Pointer to the event to post to the receiving active objects queue
 * @return int Port specific status code
 */
int Active_post(Active const *const receiver, Event const *const e);

/* @private: Interface for Active timer to post time back to sender object (delegation)*/
int Active_postTimeEvt(TimeEvt *te);

#endif /* ACTIVE_H */
