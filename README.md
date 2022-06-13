# Active 

## What is Active?

Active is a portable, C-based messaging library for embedded, memory-constrained devices (microcontrollers) based on the Active Object (Actor model) design pattern: https://en.wikipedia.org/wiki/Active_object.

It simplifies building event-driven, multi-threaded systems by decoupling method execution from method invocation in order to
simplify synchronized access to an object that resides in its own thread of control. 

The Active Object pattern allows one or more independent threads of execution to interleave their access to data modeled as a single object. 
A broad class of producer/consumer and reader/writer applications are well suited to this model of concurrency

The Active framework can be used together with normal threads running in parallel.

The framework is inspired by the [QP/C framework](https://www.state-machine.com/qpc/) from Quantum Leaps - read more about the active object pattern and why you should use it in embedded programming here: https://www.state-machine.com/active-object

The key difference between QP/C and Active (beyond QP/C being a great production ready commercial framework), is that QP/C Active objects are a state machine themselves; the state machine receives the dispatch function and calls into the separate state handling functions of an Active object. 

Active decouples this and lets the aplication instead call into a state machine (such as Zephyr's State Machine Framework) to allow for more flexible uses of Active objects.

## Maintenance 

This repository should not be considered maintained. It is a hobby project for re-learning embedded programming.
Maintainers and pull requests are welcome.

## Functionality:

- Thread abstration into separate Active objects - each object is running in its own thread with its own private data.
- Event based architecture - all Active objects are blocking until they receive an event, which is then processed by the Active object's dispatch function
- Support for static allocation of events to ROM
- Support for dynamic allocation of events and payloads through global static memory pools with automatic garbage collection
- Several event types - Signals (no arguments) and Messages (with application defined header and pointer to payload)
- Timed messages (one-shot and periodic) for timeouts, system wide ticks and data streaming
- Direct message passing between objects


## Supported frameworks:
- Zephyr RTOS

All framework and compiler specific code is found in active_port files for simple extension to new frameworks.
The library make use of runtime "polymorphism" to represent Active objects and message types through function pointers and base
classes (structs) for Events and Active objects.

## Requirements:

- A compiler using the C11 standard or later
- A compiler supporting `__has_include` for letting an application configure the library in a separate active_config.h file. GCC and Clang has built-in support for this.
- A queue implementation (Zephyr RTOS: Message queue)
- A kernel supporting threads with priorities (Zephyr RTOS: Using k_thread)
- A method to yield to other threads (Zephyr RTOS: Built-in blocking when pending on queue using K_FOREVER wait)
- Memory allocation through fixed size static pools (Zephyr RTOS: Using memory slabs)
- A timer/scheduler implementation with expiry function and user data support for timed events (Zephyr RTOS: Using k_timer)

## Getting started

The library project is set up as a PlatformIO project (https://platformio.org/) for building and testing.

The project is set up using the STM32 NUCLEO L552ZE_Q board, using a custom board file located in the boards/ folder.
Refer to PlatformIO documentation for how to change boards for testing Active as a standalone project.

Usage examples are found in the examples/ folder. Select which example application to build together with framework in platformio.ini (build_src_filter)

The library can be configured by an application through adding a custom `active_config.h` in the compiler search path to override default settings. 
Available configuration settings and defaults are located in `active_config_loader.h`.

## Testing

Unit tests and integration tests are using Unity, setup through the PlatformIO IDE. 
PlatformIO does not support boards (for running tests) on a native (e.g. x86) platform using Zephyr RTOS.

To run tests:
- Modify zephyr/prj.conf to include `CONFIG_NEWLIB_LIBC=y` (Zephyr and Unity use different LibC's)
- Connect a target board
- Find your board's serial device (`ls /dev/tty*`) and update the `monitor_port`field in platformio.ini
- Run `pio test -e test` in a PlatformIO Terminal.

## Usage

### Set up one or more active objects

An Active object implementation typically consists of a structure where the super object `Active` is the first member, enabling polymorphism.

To define an Active object, set up a structure for one or more objects:

```C
/* pingpong.h */
#ifndef PINGPONG_H
#define PINGPONG_H

#include <active.h>

typedef struct pingpong PingPong;
struct pingpong
{
  Active super;
  PingPong *pingpong;
};

void PingPong_init(PingPong *const me, queueData const *qd, threadData const *td, PingPong *pp);

#endif
```

Then, implement the init function for the Active object and the dispatch function:

```C
/* pingpong.c */
#include <active.h>

static void PingPong_dispatch(Active *const me, ACT_Evt const *const e)
{
  PingPong *p = (PingPong *)me;
  if (e->type == SIGNAL)
  {
    uint16_t sig = EVT_CAST(e, Signal)->sig;
    switch (sig)
    {
      case START_SIG:
      {
        /* Do initialization work */
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

void PingPong_init(PingPong *const me, queueData const *qd, threadData const *td, PingPong *pp)
{
  ACTP_init(&(me->super), PingPong_dispatch, qd, td);
  me->pingpong = pp; /* Example private data structure */
}
```

At last, set up the structures needed for an Active object to run. Each Active object needs their own set of structures.

```C
/* main.c */
#include <active.h>
#include "pingpong.h"

PingPong ping, pong;

#define MAX_MSG 10

ACTP_QBUF(pingQbuf, MAX_MSG);
ACTP_Q(pingQ);
ACTP_THREAD(pingT);
ACTP_THREAD_STACK_DEFINE(pingStack, 512);
ACTP_THREAD_STACK_SIZE(pingStackSz, pingStack);

const static queueData qdping = {.maxMsg = MAX_MSG,
                                 .queBuf = pingQbuf,
                                 .queue = &pingQ};

const static threadData tdping = {.thread = &pingT,
                                  .pri = 1,
                                  .stack = pingStack,
                                  .stack_size = pingStackSz};

/* Duplicate for pong data structure */
// ...

int main(void)
{
  PingPong_init(&ping, &qdping, &tdping, &pong);
  PingPong_init(&pong, &qdpong, &tdpong, &ping);

  ...
}

```

### Start active objects

After Active objects are initialized, they are ready to be started. When an Active object is started, its dispatch function will immediately receive a `Signal` event with value START_SIG.

An Active object is typically either started by the `main()` function upon HW initialization being done, or by initializing HW using START_SIG and waiting for interrupts or a timeout (using time events). The application needs to ensure that all Active objects are initialized using the `ACTP_init` function before they start sending events to one another. Events that are sent to an Active object are not processed until the Active object is started, but rather remains in the queue.
 
 ```C
int main(void)
{
  PingPong_init(&ping, &qdping, &tdping, &pong);
  PingPong_init(&pong, &qdpong, &tdpong, &ping);

  ACTP_start(ACT_UPCAST(&ping));
  ACTP_start(ACT_UPCAST(&pong));
  
}

````
The `ACT_UPCAST()` macro is a helper macro to upcast application active objects into their parent class to prevent compiler warnings.

### Process start event

The START_SIG Signal will be the first event received by any active object. It is typically used to initialize data structures or state machines as needed or as a trigger to send the first application events.



### Creating events

There are three ways of creating events for Active objects:
- Allocated on the heap and initialized using the event's `_init` function
- Allocated in ROM (using const) or on the heap using the event's `_DEFINE` macro.
- Dynamically allocated and initialized using the event's `_new` function.

The application must itself ensure static storage types or const type for events if needed.

Interfaces and macros for statically allocated events are found in the `active_msg` header file. 
Interfaces for dynamically allocated events are found in the `active_mem` header file.

When choosing how to create events, it is important to know that an active object might not know when the event has been processed by all recipients. 
Therefore, if a statically allocated event or its payload needs to be modified over time, dynamically allocated events are to be preferred to avoid race conditions when modifying it later. Dynamic events allow the application to fire-and-forget the events with automatic garbage collection when processing is done.

### Creating events - memory pool sizing

Memory pools for each event type are instanciated private to the the `active_mem` module. They are allocated compile-time, with pool sizes defined in `active_mem.h` (should be refactored).
Pool sizes can be found through analyzing the application and monitoring the max usage of memory pools during development and stress testing. (TODO)

### Posting events

An event is posted using the `ACT_post` function:

```C
/* pingpong.c */
#include <active.h>

/* Available signals in application header file - first signal starts with USER_SIG*/
enum AppUserSignal 
{
  PING = USER_SIG,
  PONG
};

static const SIGNAL_DEFINE(pingSig, PING);
static const SIGNAL_DEFINE(pongSig, PONG);

static void PingPong_dispatch(Active *const me, ACT_Evt const *const e)
{
  PingPong *p = (PingPong *)me; /* To access internal data */

  if (e->type == SIGNAL)
  {
    uint16_t sig = EVT_CAST(e, Signal)->sig;
    switch (sig)
    {
      case START_SIG:
      {
        /* Do initialization work */
        break;
      }
      case PING:
      {
        ACT_post(me, EVT_UPCAST(&pongSig));
        break;
      }
      case PONG:
      {
        ACT_post(me, EVT_UPCAST(&pingSig));
        break;
      }
      default:
      {
        break;
      }
    }
  }
}
```

The `EVT_UPCAST()`is available to upcast pointers to specific events up to the base `ACT_Evt`type.

### Processing events

An object is available for processing when it is received by the Active object's dispatch function. Active objects should run to completion on every message processed with no to minimal blocking, as it prevents the Active object from processing further messages. Long running tasks can be deferred to lower priority work threads (such as Zephyr's `workqueue`) or split into multiple steps by having the Active object message itself.

The lifetime of an event should be assumed to be only while processing it in the dispatch function and will be freed at any time after the dispatch function is complete. The exception here is if the application by design use only static events that are never modified.

If the Active object for some reason needs to retain the event, it can extend the lifetime of it in the following ways:
- Post the received event again to itself in the dispatch function
- Post the received event again to itself as an attached event of a time event.
- Add a manual memory reference to the received event using `ACT_mem_refinc`. Take care to decrement the reference again later using `ACT_mem_refdec` to ensure event is freed correctly.

Forwarding an event to other active objects is allowed. When re-posting inside the dispatch function, Active's memory management will ensure the event is not freed prematurely.

### Time events

A Time event is a special event type used for posting normal events at a later time, either as a one-shot event or as a periodic event. 

Typical use cases for Time events are:
- Having a Active object create a timeout Signal event to oneself while waiting for input in a state machine
- Creating periodic system ticks for one or more Active objects using a Signal or Message event.
- Streaming data (e.g. double buffering) between two Active objects (producer / consumer) using a Message event with dynamic payloads

Creating a time event takes the following arguments:
- An attached `ACT_Evt *` that will be posted at timer expiry (one shot or periodic)
- The Active object `sender`, which is used for posting the event in the senders context (thread) at timer expiry
- The Active object `receiver`, which will receive the attached event
- An application defined expiry function that can be used to set or replace the attached event before it is being posted at expiry.

Time events can either be declared statically in the application and initialized by `TimeEvt_init` or created dynamically by `TimeEvt_new`.
The application can freely mix dynamic and static events between the time event and attached event.

A Time event is started by calling `ACT_TimeEvt_start`, with arguments in milliseconds for initial expiry and subsequent timeouts for periodic expiry.
The timer implementations typically guarantee that *at least* the time given as argument has passed. Asynchronous messaging cannot in itself guarantee that an exact amount of time has passed at processing time.

When starting a time event, the underlying framework's timer implementation will be started. At expiry, the Active framework posts the time event itself back to the sender in an ISR context. Then, the Active object will process the Time event in its own context/thread, call the expiry function if set and then post the attached event to the receiver.

To stop a one shot Time event before expiry or to stop a running periodic Time event, the `ACT_TimeEvt_stop` is used.


### Usage rules - Dynamic events

Active objects can only post a dynamic (allocated) events *once*. Dynamic events are freed and garbage collected once processed by all receiving Active objects.
If a dynamic event is to be posted multiple times, the Active object needs to set and remove a memory reference to it using `ACT_mem_refinc` (before first post) and `ACT_mem_refdec` (after all posts are complete).

Dynamic events should be considered immutable by the sender Active object once posted, as the receiving object might preempt the sender at any time.

When using time events, any dynamic time events or attached dynamic events will be freed when the timer expires (one shot) or when the timer is stopped by the application, which ever comes first. 

Since there is no guarantee for the Active object to know when a one shot Time event expired and was freed, an application must add its own memory reference to the Time event before starting it (and remove after stopping) if there is a need to stop a dynamic one-shot Time event. 

Periodic events using dynamic attached events can register an expiry function to replace the attached event on an expiration. Previously attached events will be then freed once there are no more references to it.


### Usage rules - Dynamic payloads

*TODO: Not supported yet!*

Dynamic payloads can be used with for example the Message event type to enable a data pipe between active objects.

Dynamic payloads can be used together with dynamic events only.
If the application is utilizing dynamic payloads in events created with `Object_new()`, (such as `Message->payload`), the Active object can register a free function during Active object initialiation that will be called just before a dynamic event is freed. This allows the Active object to know that all receivers are done processing the event and the payload object can safely be freed.

The free function can also be used to let application know that event is being freed, so that any payloads can be safely recycled or re-used. Note that for time events, a time event can be freed even if there are still references to an attached event (in a queue to other Active object). 

## Ideas Roadmap

- Finish refactoring framework into _port files (remaining: sleep functions)
- Improving asserts (assert levels, test coverage w/o asserts, ROM size usage (create LUT for file / line / message))
- Make max usage of AO queues available to the application
- Review atomic accesses (e.g. memory references) 
- Add more usage examples
- Simplify extension of framework with application defined message types.
- Dynamic payloads
- Pub/sub support (TBD if broker-less or internal broker) with enum based topics to reduce ROM size
- Considering: Service discovery for run-time boot strapping of application
- Considering: Build support for connection oriented Active object "bridges" to external interfaces (UART, Bluetooth Low Energy) supporting serialization for "networked" message passing


## License

This project is licensed under the MIT License. See LICENSE file for details.
