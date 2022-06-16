#ifndef ACTIVE_TIMER_H
#define ACTIVE_TIMER_H

#include <stdbool.h>
#include <stddef.h>

#include <active_types.h>
#include <active_port.h>

/**
 * @brief Data structure for Active timer part of time events. Do not access members directly.
 *
 */
struct active_timerData
{
  ACT_TIMER(impl);
  size_t durationMs;
  size_t periodMs;
  volatile bool running;
  volatile bool sync;
};

/**
 * @brief Internal: Function to initialize timer part of time events. Must not be used by application.
 *
 * @param te
 */
void ACT_Timer_init(ACT_TimEvt *te);

/**
 * @brief Internal: Callback to handle timer expiry. Must not be used by application.
 * Used by Active framework to let native timer implementation call the generic framework.
 *
 * @param te The Time event whose timer expired
 */
void ACT_Timer_expiryCB(ACT_TimEvt *const te);

/**
 * @brief Internal: Callback to process time events. Must not be used by application.
 * Used by Active framework to process expired time events and post attached events.
 */
void ACT_TimeEvt_dispatch(ACT_TimEvt *te);

/**
 * @brief Time event expiry function prototype.
 * When a time event expires, an optional expiry function given during initialization is called to
 * let the application replace the attached event or keep track of state in the application.
 *
 * The expiry function runs in context of the Active object @a me that created the event.
 *
 * @param te The time event that expired
 *
 * @return Pointer to event that will replace existing attached event. Existing attached dynamic events are freed.
 * @return (ACT_Evt*)NULL - Indicate to framework to keep the existing attached event.
 *
 */
typedef ACT_Evt *(*ACT_TimerExpiryFn)(ACT_TimEvt const *const te);

/**
 * @brief Start a time event. The attached event will be posted when the timer expires
 *
 * This routine starts a time event, which when expires will call an optional user defined expiry function and then post the attached event.
 * When a one shot time event expires, a dynamic time event and attached dynamic events will be freed after processing.
 *
 * @param te  The initialized time event to start. Time event must be allocated and
 *            initialized with ACT_TimEvt_new (dynamic allocated event) or with ACT_TimEvt_init (static event)
 * @param durationMs The (minimum) duration until attached event is posted
 * @param periodMs The (minimum) duration until subsequent posting of event. Set to 0 for a one-shot timer.
 *
 * @warning Do not call function with a dynamic time event pointer that is previously stopped or expired, as those are freed.
 */
void ACT_TimeEvt_start(ACT_TimEvt *te, size_t durationMs, size_t periodMs);

/**
 *  @brief Stop a time event
 *
 *  This routine stops a running time event.
 *  All dynamic time events and dynamic attached events will be freed by stopping the time event. (see header for more details)
 *
 *  @param te pointer to the time event to stop
 *
 *  @return true: The timer was running when it was stopped.
 *  @return false: The timer was already expired (one-shot) or already stopped.
 *
 *  @warning Do not stop a one-shot dynamic timer event once it expired, as the time event and attached event will be freed.
 *    - Add an additional reference on it using ACT_mem_refinc before starting and decrement it using ACT_mem_refdec after stopping.
 *    - After expiry, only the time event pointer will be valid for stopping time event; the attached event will be freed.
 *  @warning Do not use dynamic timer event again after stopping it, as the time event and attached event will be freed.
 *  @warning Do not stop a timer event from an ISR.
 **/
bool ACT_TimeEvt_stop(ACT_TimEvt *te);

#endif /* ACTIVE_TIMER_H */
