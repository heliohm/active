#ifndef ACTIVE_MEM_H
#define ACTIVE_MEM_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <active_config_loader.h>
#include <active_types.h>
#include <active_timer.h>

/* Allocate and initialize new signal from the Active global signal memory pool */
ACT_Signal *Signal_new(Active const *const me, uint16_t sig);
/* Allocate and initialize new message from the Active global message memory pool */
Message *Message_new(Active const *const me, uint16_t msgHeader, void *msgPayload, uint16_t payloadLen);
/* Allocate and initialize new time event from the Active global time event memory pool */
TimeEvt *TimeEvt_new(ACT_Evt *const e, const Active *const me, const Active *const receiver, TimerExpiryHandler expFn);

/* Allocate memory pool to let active objects create memory pools for message payloads.
The memory buffer provided by the user must be aligned to an N-byte boundary, where N is a power of 2
larger than 2 (i.e. 4, 8, 16, …).

To ensure that all memory blocks in the buffer are similarly aligned to this boundary,
the object size must also be a multiple of N.
*/
ACT_Mempool *ACT_Mempool_new(void *memBuf, size_t objSize, size_t numObjects);

/* @internal - used by Active framework to increment reference counter on dynamic event */
void ACT_mem_refinc(const ACT_Evt *e);
/* @internal - used by Active framework to decrement reference counter on dynamic event and trigger freeing*/
void ACT_mem_refdec(const ACT_Evt *e);

/* @internal - used by Active framework GC and tests */
refCnt_t ACT_mem_getRefCount(const ACT_Evt *const e);
/* @internal - used by Active framework tests */
uint32_t ACT_mem_Signal_getUsed();
/* @internal - used by Active framework tests */
uint32_t ACT_mem_Message_getUsed();
/* @internal - used by Active framework tests */
uint32_t ACT_mem_TimeEvt_getUsed();

/* Garbage collect / free unreferenced event. Must only be used by application to free events that were
never posted by application or attached to a posted time event */
void ACT_mem_gc(const ACT_Evt *e);

#endif /* ACTIVE_MEM_H */
