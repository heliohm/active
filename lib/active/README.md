Active is a C-based messaging library for embedded, memory-constrained devices based on the Active Object (Actor model) design pattern:
https://en.wikipedia.org/wiki/Active_object.

The Active Object pattern decouples method execution from method invocation in order to
simplify synchronized access to an object that resides in its own thread of control. 
The Active Object pattern allows one or more independent threads of execution to interleave their access to data modeled as a single object. 
A broad class of producer/consumer and reader/writer applications are well suited to this model of concurrency


Functionality:

- Thread abstration into separate active objects - each object is running in its own thread
- Static allocation of messages to ROM
- Dynamic allocation of messages through static memory pools with automatic garbage collection
- Several message types - Signals (no arguments) and Messages (with header and pointer to payload)
- Timed messages (one-shot and periodic)
- Easily extensible with application defined message types.
- Direct message passing between objects
- Future: MQTT-like Pub/Sub functionality with enum based topics to reduce ROM size

Supported frameworks:
- Zephyr RTOS

All framework specific code is found in _port files for easy extension to new frameworks.


The library make use of dynamic polymorphism to represent active objects and message types.

Requirements:

- A queue implementation (Zephyr RTOS: Message queue)
- A method to yield to other threads (Zephyr RTOS: Built-in blocking in queue using K_FOREVER wait)
- Memory allocation (Zephyr RTOS: Using memory slabs)
- A kernel supporting threads and priorities (Zephyr RTOS: Using k_thread)
- A timer/scheduler implementation for timed events (Zephyr RTOS: Using k_timer)

Usage:

- Define Active app settings in active_app*
- Create an active object type and start the thread
- Post a message
- Receive a message

Usage notes:

Active objects can only post or publish dynamic (allocated) events *once*. Regardless of post status, they are garbage collected once processed by receiving object.

Active objects should run to completion on every message received