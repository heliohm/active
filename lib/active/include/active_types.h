#ifndef ACTIVE_TYPES_H
#define ACTIVE_TYPES_H

#include <stdint.h>

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
  MAX_SYSTEM_EVTTYPE,
  SIGNAL,
  MESSAGE,
  MAX_DIRECT_EVTTYPE,
  PUBLISH,
  SUBSCRIBE,
  UNSUBSCRIBE,
  MAX_PUBSUB_EVTTYPE,
} EvtType;

/* The active object (actors) in the program */
typedef struct active Active;

/* Thread data structure for an Active object */
typedef struct threadData threadData;

/* Queue data structure for an Active object */
typedef struct queueData queueData;

/* Timer data struture type */
typedef struct timerData Active_Timer;

/* Memory pool type */
typedef struct mempoolData Active_Mempool;

/* Memory reference counter type for events */
typedef /*_Atomic*/ uint8_t RefCnt;

#endif /* ACTIVE_TYPES_H */
