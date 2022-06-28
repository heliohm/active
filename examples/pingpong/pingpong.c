// pingpong.c

#include <active.h>
#include <active_app_messages.h>
#include <pingpong.h>

#include <console/console.h>

extern ACT_Signal pingSignal, pongSignal;

static void PingPong_dispatch(Active *const me, ACT_Evt const *const e)
{
  ACT_SLEEPMS(100);

  if (e->type == ACT_SIGNAL)
  {
    uint16_t sig = EVT_CAST(e, ACT_Signal)->sig;
    switch (sig)
    {
    case ACT_START_SIG:
    {
      ACT_DBGPRINT("Starting PingPong...\n\n");
      break;
    }
    case PING:
    {
      ACT_DBGPRINT("%p received Ping from %p!\n\n", me, e->_sender);
      if (e->_sender)
      {
        ACT_postEvt(e->_sender, EVT_UPCAST(&pongSignal));
      }
      break;
    }
    case PONG:
    {
      ACT_DBGPRINT("%p received Pong from %p!\n\n", me, e->_sender);
      if (e->_sender)
      {
        ACT_postEvt(e->_sender, EVT_UPCAST(&pingSignal));
      }
      break;
    }
    case PINGPONG:
    {
      ACT_DBGPRINT("Pingpong received: %i\n\n", sig);
      break;
    }
    case TIMEPING:
    {
      ACT_DBGPRINT("%p received TimePing from %p!\n\n", me, e->_sender);
      break;
    }
    case TIMEPONG:
    {
      ACT_DBGPRINT("%p received TimePong from %p!\n\n", me, e->_sender);
      break;
    }

    default:
    {
      ACT_DBGPRINT("What is this? %i\n\n", sig);
      break;
    }
    }
  }
}

void PingPong_init(PingPong *const me, ACT_QueueData const *qd, ACT_ThreadData const *td)
{
  ACT_init(&(me->super), PingPong_dispatch, qd, td);
}