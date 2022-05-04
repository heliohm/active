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
struct active
{
  ACTIVE_THREADPTR(thread);
  ACTIVE_QPTR(queue);
  DispatchHandler dispatch;
};

/* Thread data structures to set up an active object */
struct threadData
{
  ACTIVE_THREADPTR(thread);
  ACTIVE_THREAD_STACKPTR(stack);
  size_t stack_size;
  int pri;
};

/* Queue data structures to set up an active object */
struct queueData
{
  ACTIVE_QPTR(queue);
  char *queBuf;
  size_t maxMsg;
};

void Active_init(Active *const me, DispatchHandler dispatch);
void Active_start(Active *const me, queueData const *qd, threadData const *td);

/* @private - Used by Active framework ports */
void Active_eventLoop(Active *me);

/* Post event directly to receiver */
int Active_post(Active const *const receiver, Event const *const e);

/* @private: Interface for Active timer to post time back to sender object (delegation)*/
int Active_postTimeEvt(TimeEvt *te);

#endif /* ACTIVE_H */
