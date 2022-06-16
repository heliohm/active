#include <active.h>

#include "pingpong.h"
#include "active_app_messages.h"

PingPong ao_ping, ao_pong;

/* Ping Active Object */
ACT_QBUF(pingQbuf, 10);
ACT_Q(pingQ);
ACT_THREAD(pingT);
ACT_THREAD_STACK_DEFINE(pingStack, 512);
ACT_THREAD_STACK_SIZE(pingStackSz, pingStack);

const static ACT_QueueData qdping = {.maxMsg = 10,
                                     .queBuf = pingQbuf,
                                     .queue = &pingQ};

const static ACT_ThreadData tdping = {.thread = &pingT,
                                      .pri = 1,
                                      .stack = pingStack,
                                      .stack_size = pingStackSz};

/* Pong Active Object */
ACT_QBUF(pongQbuf, 10);
ACT_Q(pongQ);
ACT_THREAD(pongT);
ACT_THREAD_STACK_DEFINE(pongStack, 512);
ACT_THREAD_STACK_SIZE(pongStackSz, pongStack);

const static ACT_QueueData qdpong = {.maxMsg = 10,
                                     .queBuf = pongQbuf,
                                     .queue = &pongQ};

const static ACT_ThreadData tdpong = {.thread = &pongT,
                                      .pri = 2,
                                      .stack = pongStack,
                                      .stack_size = pongStackSz};

/* Global signals */
const ACT_Signal pingSignal = {.super = {.type = ACT_SIGNAL, ._sender = ACT_UPCAST(&ao_pong), ._dynamic = false}, .sig = PING};
const ACT_Signal pongSignal = {.super = {.type = ACT_SIGNAL, ._sender = ACT_UPCAST(&ao_ping), ._dynamic = false}, .sig = PONG};

const ACT_Signal timPingSignal = {.super = {.type = ACT_SIGNAL, ._sender = ACT_UPCAST(&ao_pong), ._dynamic = false}, .sig = TIMEPING};
const ACT_Signal timPongSignal = {.super = {.type = ACT_SIGNAL, ._sender = ACT_UPCAST(&ao_ping), ._dynamic = false}, .sig = TIMEPONG};

ACT_Evt *expiryFn(ACT_TimEvt const *const te)
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

  ACT_Signal *s = ACT_Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);
  ACT_Signal *s2 = ACT_Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);
  ACT_Signal *s3 = ACT_Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);

  PingPong_init(&ao_ping, &qdping, &tdping);
  ACT_start(ACT_UPCAST(&ao_ping));

  PingPong_init(&ao_pong, &qdpong, &tdpong);
  ACT_start(ACT_UPCAST(&ao_pong));

  ACT_postEvt(ACT_UPCAST(&ao_ping), EVT_UPCAST(&pingSignal));
  ACT_postEvt(ACT_UPCAST(&ao_pong), EVT_UPCAST(s));
  ACT_postEvt(ACT_UPCAST(&ao_ping), EVT_UPCAST(s2));
  ACT_postEvt(ACT_UPCAST(&ao_pong), EVT_UPCAST(s3));

  // Wait until all some messages are freed (PingPong waits 1sec per msg)
  ACT_SLEEPMS(500);

  s = ACT_Signal_new(ACT_UPCAST(&ao_ping), PING);
  s2 = ACT_Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);
  s3 = ACT_Signal_new(ACT_UPCAST(&ao_pong), PINGPONG);

  ACT_postEvt(ACT_UPCAST(&ao_pong), EVT_UPCAST(s));
  ACT_postEvt(ACT_UPCAST(&ao_pong), EVT_UPCAST(s2));
  ACT_postEvt(ACT_UPCAST(&ao_pong), EVT_UPCAST(s3));

  ACT_SLEEPMS(500);

  s = ACT_Signal_new(ACT_UPCAST(&ao_ping), PING);

  ACT_TimEvt *te = ACT_TimEvt_new(EVT_UPCAST(s), ACT_UPCAST(&ao_ping), ACT_UPCAST(&ao_pong), expiryFn);
  ACT_TimeEvt_start(te, 200, 200);

  ACT_SLEEPMS(1000);
  ACT_TimeEvt_stop(te);
  ACT_SLEEPMS(1000);

  return 0;
}
#endif