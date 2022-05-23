#ifndef ACTIVE_APP_MESSAGES_H
#define ACTIVE_APP_MESSAGES_H

#include <active.h>
#include <active_msg.h>
#include <active_psmsg.h>

enum AppUserSignal
{
  PING = USER_SIG,
  PONG,
  PINGPONG,
  TIMEPING,
  TIMEPONG
};

enum AppMessage
{
  SENSOR_READING
};

enum PSRootTopics
{
  SYSTEM = USER_TOPIC,
  SENSOR,
  UI,
  MAX_ROOT_TOPICS,
};

enum PSSystemTopics
{
  REBOOT = MAX_ROOT_TOPICS,
  MAX_SYSTEM_TOPICS,
};

enum PSSensorTopics
{
  TEMPERATURE = MAX_SYSTEM_TOPICS,
};

#endif /* ACTIVE_APP_MESSAGES_H */