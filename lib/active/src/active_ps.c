/* Pub / Sub protocol */
#include <active.h>

void Active_publish(PubMessage *msg)
{

  Topic *topic = msg->topic;
  // SubscriberList *lst = fetchSubscribers(topic);
}

void Active_subscribe(Active *me, Topic *topic)
{
}
