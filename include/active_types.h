#ifndef ACTIVE_TYPES_H
#define ACTIVE_TYPES_H

#include <stdatomic.h>

/* Base event for polymorphism of other objects */
typedef struct active_event ACT_Evt;
typedef struct active_signal ACT_Signal;
typedef struct active_message ACT_Message;
typedef struct active_timevt ACT_TimEvt;

/* Types of events supported by Active framework */
typedef enum evtType
{
  ACT_UNUSED = 0, // For asserts on uninitialized events of static storage class
  ACT_SIGNAL,
  ACT_MESSAGE,
  ACT_TIMEVT
} ACT_EvtType;

/* Event memory reference count */
typedef atomic_ushort refCnt_t;

/* The active object (actors) in the program */
typedef struct active_object Active;

/* Thread data structure for an Active object */
typedef struct active_threadData ACT_ThreadData;

/* Queue data structure for an Active object */
typedef struct active_queueData ACT_QueueData;

/* Timer data struture type */
typedef struct active_timerData ACT_Timer;

/* Memory pool type */
typedef struct active_mempoolData ACT_Mempool;

/* Assert handler function prototype */
typedef struct active_assertinfo Active_AssertInfo;
#endif /* ACTIVE_TYPES_H */
