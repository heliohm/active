#ifndef _ACTIVE_PSMSG_H_
#define _ACTIVE_PSMSG_H_

/***********************************
 * Publish / subscribe functionality
 **********************************/

enum ReservedTopics
{
  WILDCARD = 0,
  USER_TOPIC // First user topic for pub sub starts here
};

/* Topic data structure for PubSub functionality */
typedef struct topic Topic; /* Forward declaration */

struct topic
{
  uint16_t node;
  Topic *child;
};
// ACTIVE_CASSERT(sizeof(Topic) == 8, "Topic type is not the right size.");

/* Message to send to a pubsub active object to subscribe or
unsubscribe to a message */
typedef struct SubMessage
{
  Event super;
  Topic *topic;
} SubMessage;
// ACTIVE_CASSERT(sizeof(SubMessage) == 12, "SubMessage type is not the right size.");

/* Wrapper message to send to pubsub active object to publish a message.
Includes the metadata of the message and a pointer to the message itself */
typedef struct PubMessage
{
  Event super;  /* Publish message metadata */
  Event *e;     /* Attached message to be published to subscribers */
  Topic *topic; /* Topic to publish message to */
  bool sticky;  /* Should the message be retained for new subscribers? */
} PubMessage;

// ACTIVE_CASSERT(sizeof(PubMessage) == 20, "PubMessage type is not the right size.");

#endif /* _ACTIVE_PSMSG_H_ */
