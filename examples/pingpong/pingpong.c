// pingpong.c

#include <active.h>
#include "active_app_messages.h"
#include "pingpong.h"

#include <console/console.h>

extern Signal pingSignal, pongSignal;

static void PingPong_dispatch(Active *const me, Event const *const e)
{
  k_msleep(100);

  if (e->type == SIGNAL)
  {
    uint16_t sig = EVT_CAST(e, Signal)->sig;
    switch (sig)
    {
    case START_SIG:
    {
      printk("Starting PingPong...\n\n");
      break;
    }
    case PING:
    {
      printk("%p received Ping from %p!\n\n", me, e->_sender);
      if (e->_sender)
      {
        ACT_post(e->_sender, EVT_UPCAST(&pongSignal));
      }
      break;
    }
    case PONG:
    {
      printk("%p received Pong from %p!\n\n", me, e->_sender);
      if (e->_sender)
      {
        ACT_post(e->_sender, EVT_UPCAST(&pingSignal));
      }
      break;
    }
    case PINGPONG:
    {
      printk("Pingpong received: %i\n\n", sig);
      break;
    }
    case TIMEPING:
    {
      printk("%p received TimePing from %p!\n\n", me, e->_sender);
      break;
    }
    case TIMEPONG:
    {
      printk("%p received TimePong from %p!\n\n", me, e->_sender);
      break;
    }

    default:
    {
      printk("What is this? %i\n\n", sig);
      break;
    }
    }
  }
}

void PingPong_init(PingPong *const me, queueData const *qd, threadData const *td)
{
  ACTP_init(&(me->super), PingPong_dispatch, qd, td);
}