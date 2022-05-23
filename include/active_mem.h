#ifndef ACTIVE_MEM_H
#define ACTIVE_MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <active_types.h>
#include <active_msg.h>
#include <active_timer.h>

#define ACTIVE_MEM_NUM_TIMEREVT 3
#define ACTIVE_MEM_NUM_SIGNALS 3
#define ACTIVE_MEM_NUM_MESSAGES 2
#define ACTIVE_MEM_NUM_OBJPOOLS 1

/* Allocate and initialize new signal from the Active global signal memory pool */
Signal *Signal_new(Active const *const me, uint16_t sig);
/* Allocate and initialize new message from the Active global message memory pool */
Message *Message_new(Active const *const me, uint16_t msgHeader, void *msgPayload, uint16_t payloadLen);
/* Allocate and initialize new time event from the Active global time event memory pool */
TimeEvt *TimeEvt_new(Event *const e, const Active *const me, const Active *const receiver, TimerExpiryHandler expFn);

/* Allocate memory pool to let active objects create memory pools for message payloads.
The memory buffer provided by the user must be aligned to an N-byte boundary, where N is a power of 2
larger than 2 (i.e. 4, 8, 16, …).

To ensure that all memory blocks in the buffer are similarly aligned to this boundary,
the object size must also be a multiple of N.
*/
Active_Mempool *Active_Mempool_new(void *memBuf, size_t objSize, size_t numObjects);

/* @internal - used by Active framework to increment reference counter on dynamic event */
void Active_mem_refinc(const Event *e);
/* @internal - used by Active framework to decrement reference counter on dynamic event */
void Active_mem_refdec(const Event *e);

/* @internal - used by Active framework tests */
atomic_t Active_mem_getRefCount(const Event *const e);
/* @internal - used by Active framework tests */
uint32_t Active_mem_Signal_getUsed();
/* @internal - used by Active framework tests */
uint32_t Active_mem_TimeEvt_getUsed();

/* Garbage collect / free unreferenced event. Must only to be used by application to free events that were
not posted by application or timer */
void Active_mem_gc(const Event *e);

#endif /* ACTIVE_MEM_H */
