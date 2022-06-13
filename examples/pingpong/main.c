#include <zephyr.h>
#include <string.h>
#include <console/console.h>

#include <active.h>

#include "pingpong.h"
#include "active_app_messages.h"

PingPong ao_ping, ao_pong;

/* Ping Active Object */
ACTP_QBUF(pingQbuf, 10);
ACTP_Q(pingQ);
ACTP_THREAD(pingT);
ACTP_THREAD_STACK_DEFINE(pingStack, 512);
ACTP_THREAD_STACK_SIZE(pingStackSz, pingStack);

const static queueData qdping = {.maxMsg = 10,
                                 .queBuf = pingQbuf,
                                 .queue = &pingQ};

const static threadData tdping = {.thread = &pingT,
                                  .pri = 1,
                                  .stack = pingStack,
                                  .stack_size = pingStackSz};

/* Pong Active Object */
ACTP_QBUF(pongQbuf, 10);
ACTP_Q(pongQ);
ACTP_THREAD(pongT);
ACTP_THREAD_STACK_DEFINE(pongStack, 512);
ACTP_THREAD_STACK_SIZE(pongStackSz, pongStack);

const static queueData qdpong = {.maxMsg = 10,
                                 .queBuf = pongQbuf,
                                 .queue = &pongQ};

const static threadData tdpong = {.thread = &pongT,
                                  .pri = 2,
                                  .stack = pongStack,
                                  .stack_size = pongStackSz};

/* Global signals */
const Signal pingSignal = {.super = {.type = SIGNAL, ._sender = ACT_UPCAST(&ao_pong), ._dynamic = false}, .sig = PING};
const Signal pongSignal = {.super = {.type = SIGNAL, ._sender = ACT_UPCAST(&ao_ping), ._dynamic = false}, .sig = PONG};

const Signal timPingSignal = {.super = {.type = SIGNAL, ._sender = ACT_UPCAST(&ao_pong), ._dynamic = false}, .sig = TIMEPING};
const Signal timPongSignal = {.super = {.type = SIGNAL, ._sender = ACT_UPCAST(&ao_ping), ._dynamic = false}, .sig = TIMEPONG};

Event *expiryFn(TimeEvt const *const te)
{
  if (te->e == EVT_UPCAST(&timPongSignal))
  {
    return EVT_UPCAST(&timPingSignal);
  }

  return EVT_UPCAST(&timPongSignal);
}
#ifndef PIO_UNIT_TESTING
int main(void)
{

  Signal *s = Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);
  Signal *s2 = Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);
  Signal *s3 = Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);

  PingPong_init(&ao_ping, &qdping, &tdping);
  ACTP_start(ACT_UPCAST(&ao_ping));

  PingPong_init(&ao_pong, &qdpong, &tdpong);
  ACTP_start(ACT_UPCAST(&ao_pong));

  ACT_post(ACT_UPCAST(&ao_ping), EVT_UPCAST(&pingSignal));
  ACT_post(ACT_UPCAST(&ao_pong), EVT_UPCAST(s));
  ACT_post(ACT_UPCAST(&ao_ping), EVT_UPCAST(s2));
  ACT_post(ACT_UPCAST(&ao_pong), EVT_UPCAST(s3));

  // Wait until all some messages are freed (PingPong waits 1sec per msg)
  k_msleep(500);

  s = Signal_new(ACT_UPCAST(&ao_ping), PING);
  s2 = Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);
  s3 = Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);

  ACT_post(ACT_UPCAST(&ao_pong), EVT_UPCAST(s));
  ACT_post(ACT_UPCAST(&ao_pong), EVT_UPCAST(s2));
  ACT_post(ACT_UPCAST(&ao_pong), EVT_UPCAST(s3));

  k_msleep(500);

  s = Signal_new(ACT_UPCAST(&ao_ping), PING);

  TimeEvt *te = TimeEvt_new(EVT_UPCAST(s), ACT_UPCAST(&ao_ping), ACT_UPCAST(&ao_pong), expiryFn);
  ACT_TimeEvt_start(te, 200, 200);

  k_msleep(1000);
  ACT_TimeEvt_stop(te);
  k_msleep(1000);

  k_sleep(K_FOREVER);
  return 0;
}
#endif