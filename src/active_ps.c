/* Pub / Sub protocol */
#include <active.h>

void ACT_publish(PubMessage *msg)
{

  Topic *topic = msg->topic;
  // SubscriberList *lst = fetchSubscribers(topic);
}

void ACT_subscribe(Active *me, Topic *topic)
{
}
