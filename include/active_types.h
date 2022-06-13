#ifndef ACTIVE_TYPES_H
#define ACTIVE_TYPES_H

#include <stdatomic.h>

/* Base event for polymorphism of other objects */
typedef struct event Event;
typedef struct timeevt TimeEvt;
typedef struct signal Signal;
typedef struct message Message;

/* Types of events supported by Active framework */
typedef enum evtType
{
  UNUSED = 0, // For asserts on uninitialized events of static storage class
  TIMEREVT,
  SIGNAL,
  MESSAGE
} EvtType;

/* Event memory reference count */
typedef atomic_ushort refCnt_t;

/* The active object (actors) in the program */
typedef struct active Active;

/* Thread data structure for an Active object */
typedef struct threadData threadData;

/* Queue data structure for an Active object */
typedef struct queueData queueData;

/* Timer data struture type */
typedef struct timerData ACT_Timer;

/* Memory pool type */
typedef struct mempoolData ACT_Mempool;

#endif /* ACTIVE_TYPES_H */
