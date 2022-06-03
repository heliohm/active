#ifndef ACTIVE_MSG_H
#define ACTIVE_MSG_H

#include <stdint.h>

#include <active_types.h>
#include <active_mem.h>
#include <active_timer.h>

#define EVT_UPCAST(ptr) ((Event *)(ptr))
#define EVT_CAST(ptr, type) ((type *)(ptr))

/**
 * @brief Statically and initialize allocate a signal object with no sender information
 *
 */
#define SIGNAL_DEFINE(symbol, userSignal) Signal symbol =                                                            \
                                              {.super = (Event){.type = SIGNAL, ._sender = NULL, ._dynamic = false}, \
                                               .sig = userSignal};

#define MESSAGE_DEFINE(symbol, msgheader, msgpayloadptr, msgpayloadLen) Message symbol =                                                            \
                                                                            {.super = (Event){.type = MESSAGE, ._sender = NULL, ._dynamic = false}, \
                                                                             .header = msgheader,                                                   \
                                                                             .payload = msgpayloadptr,                                              \
                                                                             .payloadLen = msgpayloadLen};

/******************************
 * Active object message types
 *****************************/

/* Base event, never to be used directly. Used for polymorphism of other
message types */
struct event
{
  Active *_sender;        // Sender of event
  EvtType type;           // Type of event
  const atomic_t _refcnt; // Number of memory references for event
  const bool _dynamic;    // Flag for memory management to know if event is dynamic or static
};

/* Time event for posting an attached event on a timer (one shot or periodic) */
struct timeevt
{
  Event super;              // Sender and type (for delegation) and memory tracking
  Event const *e;           // Attached event to send on expiry
  Active const *receiver;   // AO to send message to if sending a direct message
  TimerExpiryHandler expFn; // Expiry function to let AO replace attached event at timer expiry
  Active_Timer timer;       // Timer instance belonging to time event
} _PACKED_ ALIGNAS(4);

/* Signals active objects that something has happened. Takes no parameters */
struct signal
{
  Event super;
  uint16_t sig;
};

/* Generic message type consisting of header, payload and payload length */
struct message
{
  Event super;
  void *payload;
  uint16_t header;
  uint16_t payloadLen;
};

/* Reserved signals by Active framework */
enum ReservedSignals
{
  START_SIG = 0, /* Signal to AO that it has started */
  USER_SIG       /* First user signal starts here */
};

/**
 * @brief Initialize a Signal structure before using it
 *
 * Do not call function with signal pointer to an object allocated with Signal_new
 *
 * @param s Pointer to the statically allocated Signal structure to initialize
 * @param me The active object that will post the Signal
 * @param sig The signal to send. Must be >= ReservedSignals.USER_SIG
 *
 */
void Signal_init(Signal *s, Active const *const me, uint16_t sig);

/**
 * @brief Initialize a Message structure before using it
 *
 * Do not call function with Message pointer to an object allocated with Message_new
 *
 * @param m Pointer to the statically allocated Message structure to initialize
 * @param me The Active object that will post the Message
 * @param header Application defined header
 * @param payload Pointer to application defined message payload
 * @param payloadLen Length of application defined message payload
 *
 * @warning The Active object does not know when a message is processed by receivers.
 *          Take care of ensuring the payload data area is not modified until the application by design has ensured processing is done.
 *
 */
void Message_init(Message *m, Active const *const me, uint16_t header, void *payload, uint16_t payloadLen);

/**
 * @brief Initialize a Time event structure before using it
 *
 *  Do not call function with time event pointer to an object allocated with TimeEvt_new
 *
 * @param te A pointer to the statically allocated time event structure to initialize
 * @param me The active object that will process the time event and post the attached event
 * @param e A pointer to the event to be attached and posted on timer expiry. Can be NULL if expiry function is set.
 * @param receiver The receiving active object of the attached event
 * @param expFn Expiry function to that can update the attached event on timer expiry
 */
void TimeEvt_init(TimeEvt *const te, const Active *const me, Event *const e, Active const *const receiver, TimerExpiryHandler expFn);

#endif /* ACTIVE_MSG_H */
