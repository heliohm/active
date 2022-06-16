#ifndef ACTIVE_MSG_H
#define ACTIVE_MSG_H

#include <stdbool.h>
#include <stdint.h>

#include <active_types.h>
#include <active_timer.h>

#define EVT_UPCAST(ptr) ((ACT_Evt *)(ptr))
#define EVT_CAST(ptr, type) ((type *)(ptr))

/**
 * @brief Statically and initialize allocate a signal object with no sender information
 *
 */
#define ACT_SIGNAL_DEFINE(symbol, signal) ACT_Signal symbol =                                                              \
                                              {.super = (ACT_Evt){.type = ACT_SIGNAL, ._sender = NULL, ._dynamic = false}, \
                                               .sig = signal};

#define ACT_MESSAGE_DEFINE(symbol, msgheader, msgpayloadptr, msgpayloadLen) ACT_Message symbol =                                                              \
                                                                                {.super = (ACT_Evt){.type = ACT_MESSAGE, ._sender = NULL, ._dynamic = false}, \
                                                                                 .header = msgheader,                                                         \
                                                                                 .payload = msgpayloadptr,                                                    \
                                                                                 .payloadLen = msgpayloadLen};

/******************************
 * Active object message types
 *****************************/

/* Base event, never to be used directly. Used for polymorphism of other
message types */
struct active_event
{
  Active *_sender;        // Sender of event
  ACT_EvtType type;       // Type of event
  const refCnt_t _refcnt; // Number of memory references for event. Const to avoid application modifying by accident.
  const bool _dynamic;    // Flag for memory management to know if event is dynamic or static. Const to avoid application modifying by accident.
};

/* Time event for posting an attached event on a timer (one shot or periodic) */
struct active_timevt
{
  const ACT_Evt super;     // Sender and type (for delegation) and memory tracking
  const ACT_Evt *e;        // Attached event to send on expiry
  const Active *receiver;  // AO to send message to if sending a direct message
  ACT_TimerExpiryFn expFn; // Expiry function to let AO replace attached event at timer expiry
  ACT_Timer timer;         // Timer instance belonging to time event
};

/* Signals active objects that something has happened. Takes no parameters */
struct active_signal
{
  const ACT_Evt super;
  uint16_t sig;
};

/* Generic message type consisting of header, payload and payload length */
struct active_message
{
  const ACT_Evt super;
  void *payload;
  uint16_t header;
  uint16_t payloadLen;
};

/* Reserved signals by Active framework */
enum ReservedSignals
{
  ACT_START_SIG = 0, /* Signal to AO that it has started */
  ACT_USER_SIG       /* First user signal starts here */
};

/**
 * @brief Initialize a ACT_Signal structure before using it
 *
 * Do not call function with signal pointer to an object allocated with ACT_Signal_new
 *
 * @param s Pointer to the statically allocated ACT_Signal structure to initialize
 * @param me The active object that will post the ACT_Signal
 * @param sig The signal to send. Must be >= ACT_USER_SIG
 *
 */
void ACT_Signal_init(ACT_Signal *s, Active const *const me, uint16_t sig);

/**
 * @brief Initialize a ACT_Message structure before using it
 *
 * Do not call function with ACT_Message pointer to an object allocated with ACT_Message_new
 *
 * @param m Pointer to the statically allocated ACT_Message structure to initialize
 * @param me The Active object that will post the ACT_Message
 * @param header Application defined header
 * @param payload Pointer to application defined message payload
 * @param payloadLen Length of application defined message payload
 *
 * @warning The Active object does not know when a message is processed by receivers.
 *          Take care of ensuring the payload data area is not modified until the application by design has ensured processing is done.
 *
 */
void ACT_Message_init(ACT_Message *m, Active const *const me, uint16_t header, void *payload, uint16_t payloadLen);

/**
 * @brief Initialize a Time event structure before using it
 *
 *  Do not call function with time event pointer to an object allocated with ACT_TimEvt_new
 *
 * @param te A pointer to the statically allocated time event structure to initialize
 * @param me The active object that will process the time event and post the attached event
 * @param e A pointer to the event to be attached and posted on timer expiry. Can be NULL if expiry function is set.
 * @param receiver The receiving active object of the attached event
 * @param expFn Optional expiry function to that can update the attached event on timer expiry
 */
void ACT_TimEvt_init(ACT_TimEvt *const te, const Active *const me, ACT_Evt *const e, Active const *const receiver, ACT_TimerExpiryFn expFn);

#endif /* ACTIVE_MSG_H */
