#ifndef ACTIVE_MSG_H
#define ACTIVE_MSG_H

#include <stdint.h>

#include <active_types.h>
#include <active_mem.h>
#include <active_timer.h>

#define EVT_UPCAST(ptr) ((Event *)(ptr))
#define EVT_CAST(ptr, type) ((type *)(ptr))

/* Statically allocate a signal object with no sender information */
#define SIGNAL_DEFINE(symbol, signal) Signal symbol =                                                            \
                                          {.super = (Event){.type = SIGNAL, ._sender = NULL, ._dynamic = false}, \
                                           .sig = signal};

/***************************
 * Actor message types
 **************************/

/* Base event, never to be used directly. Used for polymorphism of other
message types */
struct event
{
  Active *_sender;
  EvtType type;
  const atomic_t _refcnt;
  const bool _dynamic;
};

/* Time event for posting an attached event on a timer (one shot or periodic) */
struct timeevt
{
  Event super;              // Sender and type (for delegation) and memory tracking
  Event const *e;           // Attached message to send
  Active const *receiver;   // AO to send message to if sending a direct message
  TimerExpiryHandler expFn; // Expiry function to let AO replace attached event to before posting it
  Active_Timer timer;       // Timer instance belonging to time event
} _PACKED_ ALIGNAS(4);

/* Signals active objects that something has happened. Takes no parameters */
struct signal
{
  Event super;
  uint16_t sig;
};

/* Generic message type consisting of header, payload and payload length */
typedef struct message
{
  Event super;
  uint16_t header;
  void *payLoad;
  uint32_t payLoadLen;
} Message;

/* Reserved signals by Active framework */
enum ReservedSignals
{
  START_SIG = 0, /* Signal to AO that it has started */
  USER_SIG       /* First user signal starts here */
};

/* Initialize a static signal before using it */
void Signal_init(Signal *s, Active const *const me, uint16_t sig);
void TimeEvt_init(TimeEvt *const te, const Active *const me, Event *const e, Active const *const receiver, TimerExpiryHandler expFn);
#endif /* ACTIVE_MSG_H */
