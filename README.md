# Active 

## What is Active?

Active is a portable, C-based messaging library for embedded, memory-constrained devices (microcontrollers) based on the Active Object (Actor model) design pattern: https://en.wikipedia.org/wiki/Active_object.

It simplifies building event-driven, multi-threaded systems by decoupling method execution from method invocation in order to
simplify synchronized access to an object that resides in its own thread of control. 

The Active Object pattern allows one or more independent threads of execution to interleave their access to data modeled as a single object. 
A broad class of producer/consumer and reader/writer applications are well suited to this model of concurrency

The Active framework can be used together with normal threads running in parallel.

The framework is inspired by the QP/C framework from Quantum Leaps - read more about the active object pattern and why you should use it in embedded programming here: https://www.state-machine.com/active-object

## Maintenance 

This repository should not be considered maintained. It is a hobby project for re-learning embedded programming.
Maintainers and pull requests are welcome.

## Functionality:

- Thread abstration into separate Active objects - each object is running in its own thread with its own private data.
- Event based architecture - all Active objects are blocking until they receive an event, which is then processed by the Active object's dispatch function
- Support for static allocation of events to ROM
- Support for dynamic allocation of events and payloads through static memory pools with automatic garbage collection
- Several event types - Signals (no arguments) and Messages (with header and pointer to payload)
- Timed messages (one-shot and periodic) for timeouts, system wide ticks and data streaming
- Direct message passing between objects


## Supported frameworks:
- Zephyr RTOS

All framework and compiler specific code is found in active_port files for simple extension to new frameworks.
The library make use of runtime "polymorphism" to represent Active objects and message types through function pointers and base
classes (structs) for Events and Active objects.

## Porting requirements:

- A queue implementation (Zephyr RTOS: Message queue)
- A method to yield to other threads (Zephyr RTOS: Built-in blocking when pending on queue using K_FOREVER wait)
- Memory allocation through fixed size static pools (Zephyr RTOS: Using memory slabs)
- A kernel supporting threads with priorities (Zephyr RTOS: Using k_thread)
- A timer/scheduler implementation for timed events (Zephyr RTOS: Using k_timer)

## Getting started

The library project is set up as a PlatformIO project (https://platformio.org/) for building and testing.
Install Visual Studio Code and the PlatformIO project before cloning the project.

The project is set up using the STM32 NUCLEO L552ZE_Q board, using a custom board file located in the boards/ folder.
Refer to PlatformIO documentation for how to change boards for testing Active as a standalone project.

Usage examples are found in the examples/ folder. Select which example application to build together with framework in platformio.ini (build_src_filter)

## Testing

Unit tests and integration tests are using Unity, setup through the PlatformIO IDE. 
PlatformIO does not support boards (for running tests) on a native (e.g. x86) platform using Zephyr RTOS.

To run tests, connect a target board and run `pio test -e test` in a PlatformIO Terminal.

## Usage

### Set up one or more active objects

An Active object implementation typically consists of a structure where the super object `Active` is the first member, enabling polymorphism.

To define an Active object, set up a structure for one or more objects:

```C
/* pingpong.h */
#ifndef PINGPONG_H
#define PINGPONG_H

#include <active.h>

typedef struct
{
  Active super;
  uint32_t myData;
} PingPong;

void PingPong_init(PingPong *const me, queueData const *qd, threadData const *td, uint32_t data);

#endif
```

Then, implement the init function for the Active object and the dispatch function:

```C
/* pingpong.c */
#include <active.h>

static void PingPong_dispatch(Active *const me, Event const *const e)
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

void PingPong_init(PingPong *const me, queueData const *qd, threadData const *td, uint32_t data)
{
  Active_init(&(me->super), PingPong_dispatch, qd, td);
  me->myData = data; /* Example private data structure */
}
```

At last, set up the structures needed for an Active object to run. Each Active object needs their own set of structures.

```C
/* main.c */
#include <active.h>
#include "pingpong.h"

PingPong ao_ping;

#define MAX_MSG 10

ACTIVE_QBUF(pingQbuf, MAX_MSG);
ACTIVE_Q(pingQ);
ACTIVE_THREAD(pingT);
ACTIVE_THREAD_STACK(pingStack, 512);
ACTIVE_THREAD_STACK_SIZE(pingStackSz, pingStack);

const static queueData qdping = {.maxMsg = MAX_MSG,
                                 .queBuf = pingQbuf,
                                 .queue = &pingQ};

const static threadData tdping = {.thread = &pingT,
                                  .pri = 1,
                                  .stack = pingStack,
                                  .stack_size = pingStackSz};

int main(void)
{
  PingPong_init(&ao_ping, &qdping, &tdping);

  ...
}

```

### Start active objects

After Active objects are initialized, they are ready to be started. When an Active object is started, its dispatch function will immediately receive a `Signal` event with value START_SIG.

An Active object is typically either started by the `main()` function upon HW initialization being done, or by initializing HW using START_SIG and waiting for interrupts or a timeout (using time events). The application needs to ensure that all Active objects are initialized using the `Active_init` function before they start sending events to one another. Events that are sent to an Active object are not processed until the Active object is started, but rather remains in the queue.
 
 ```C
int main(void)
{
  PingPong_init(&ao_ping, &qdping, &tdping);
  Active_start(ACTIVE_UPCAST(&ao_ping));
  
}

````
The `ACTIVE_UPCAST()` macro is a helper macro to upcast application active objects into their parent class to prevent compiler warnings.

### Process start event

The START_SIG Signal will be the first event received by any active object. It is typically used to initialize data structures or state machines as needed or as a trigger to send the first application events.



### Creating events

There are three ways of creating events for Active objects:
- Allocated on the heap and initialized using the event's `_init` function
- Allocated in ROM (using const) or on the heap using the event's `_DEFINE` macro.
- Dynamically allocated and initialized using the event's `_new` function

The application must itself ensure static storage types or const type for events if needed.

Interfaces and macros for statically allocated events are found in the `active_msg` header file. 
Interfaces for dynamically allocated events are found in the `active_mem` header file.

When choosing how to create events, it is important to know that an active object might not know when the event has been processed by all recipients. 
Therefore, if a statically allocated event needs to be modified over time, dynamically allocated events are to be preferred to avoid race conditions when. These allow the application to fire-and-forget the events with automatic garbage collection when processing is done.

### Creating events - memory pool sizing

Memory pools for each event type are instanciated private to the the `active_mem` module. They are allocated compile-time, with pool sizes defined in `active_mem.h` (should be refactored).
Pool sizes can be found through analyzing the application and monitoring the max usage of memory pools (TODO)

### Posting events

### Processing events

Active objects should run to completion on every message processed with no to minimal blocking. Any blocking in an active object will introduce potential concurrency issues and block the object from processing new messages.

### Time events



### Usage rules - Dynamic events

Active objects can only post a dynamic (allocated) events *once*. Dynamic events are freed and garbage collected once processed by receiving object.
Dynamic events should be considered immutable by the sender Active object once posted, as the receiving object might preempt the sender at any time.

When using time events, any dynamic time events or attached dynamic events will be freed when the timer expires (one shot) or when the timer is stopped by the application, which ever comes first. 

Periodic events using dynamic attached events can register an expiry function to replace the attached event on an expiration. Previously attached events will be freed.


### Usage rules - Dynamic payloads

*TODO: Not supported yet!*

Dynamic payloads can be used together with dynamic events only.
If the application is utilizing dynamic payloads in events created with `Object_new()`, (such as `Message->payload`), the Active object can register a free function during Active object initialiation that will be called just before a dynamic event is freed. This allows the Active object to know that all receivers are done processing the event and the payload can safely be freed.

The free function can also be used to let application know that event is being freed, so that any payloads can be safely recycled or re-used. Note that for time events, a time event can be freed even if there are still references to an attached event (in a queue to other Active object). 

## Roadmap

- Complete refactoring framework and compiler ports into _port files (zephyr, gcc)
- Make it possible to configure the library using an application header file (memory pools, asserts)
- Review / refactor atomic accesses (e.g. memory references) using C11
- Add more usage examples
- Simplify extension of framework with application defined message types.
- Dynamic payloads
- Pub/sub support (TBD if broker-less or internal broker) with enum based topics to reduce ROM size
- Considering: Service discovery for run-time boot strapping of application
- Considering: Build support for Active object "bridges" with connection state to external interfaces (UART, Bluetooth Low Energy) supporting serialization for "networked" message passing


## License

This project is licensed under the MIT License. See LICENSE file for details.
