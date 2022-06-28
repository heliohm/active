#ifndef PINGPONG_H
#define PINGPONG_H

#include <active.h>

typedef struct
{
  Active super;
} PingPong;

void PingPong_init(PingPong *const me, ACT_QueueData const *qd, ACT_ThreadData const *td);

#endif