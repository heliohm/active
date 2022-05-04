#ifndef PINGPONG_H
#define PINGPONG_H

#include <active.h>

typedef struct
{
  Active super;
} PingPong;

void PingPong_init(PingPong *const me);

#endif